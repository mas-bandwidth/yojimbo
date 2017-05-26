/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#include "yojimbo_config.h"
#include "yojimbo_connection.h"

namespace yojimbo
{
    struct ConnectionPacket
    {
        int numChannelEntries;
        ChannelPacketData * channelEntry;
        MessageFactory * messageFactory;

        ConnectionPacket()
        {
            messageFactory = NULL;
            numChannelEntries = 0;
            channelEntry = NULL;
        }

        ~ConnectionPacket()
        {
            if ( messageFactory )
            {
                for ( int i = 0; i < numChannelEntries; ++i )
                {
                    channelEntry[i].Free( *messageFactory );
                }
                YOJIMBO_FREE( messageFactory->GetAllocator(), channelEntry );
                messageFactory = NULL;
            }        
        }

        bool AllocateChannelData( MessageFactory & messageFactory, int numEntries )
        {
            assert( numEntries > 0 );
            assert( numEntries <= MaxChannels );
            this->messageFactory = &messageFactory;
            Allocator & allocator = messageFactory.GetAllocator();
            channelEntry = (ChannelPacketData*) YOJIMBO_ALLOCATE( allocator, sizeof( ChannelPacketData ) * numEntries );
            if ( channelEntry == NULL )
                return false;
            for ( int i = 0; i < numEntries; ++i )
            {
                channelEntry[i].Initialize();
            }
            numChannelEntries = numEntries;
            return true;
        }

        template <typename Stream> bool Serialize( Stream & stream, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig )
        {
            const int numChannels = connectionConfig.numChannels;
            serialize_int( stream, numChannelEntries, 0, connectionConfig.numChannels );
#if YOJIMBO_DEBUG_MESSAGE_BUDGET
            assert( stream.GetBitsProcessed() <= ConservativeConnectionPacketHeaderEstimate );
#endif // #if YOJIMBO_DEBUG_MESSAGE_BUDGET
            if ( numChannelEntries > 0 )
            {
                if ( Stream::IsReading )
                {
                    if ( !AllocateChannelData( messageFactory, numChannelEntries ) )
                    {
                        debug_printf( "error: failed to allocate channel data (ConnectionPacket)\n" );
                        return false;
                    }
                    for ( int i = 0; i < numChannelEntries; ++i )
                    {
                        assert( channelEntry[i].messageFailedToSerialize == 0 );
                    }
                }
                for ( int i = 0; i < numChannelEntries; ++i )
                {
                    assert( channelEntry[i].messageFailedToSerialize == 0 );
                    if ( !channelEntry[i].SerializeInternal( stream, messageFactory, connectionConfig.channel, numChannels ) )
                    {
                        debug_printf( "error: failed to serialize channel %d\n", i );
                        return false;
                    }
                }
            }
            return true;
        }

        bool SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig )
        {
            return Serialize( stream, messageFactory, connectionConfig );
        }

        bool SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig )
        {
            return Serialize( stream, messageFactory, connectionConfig );            
        }

        bool SerializeInternal( MeasureStream & stream, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig )
        {
            return Serialize( stream, messageFactory, connectionConfig );            
        }

    private:

        ConnectionPacket( const ConnectionPacket & other );

        const ConnectionPacket & operator = ( const ConnectionPacket & other );
    };

    // ------------------------------------------------------------------------------

    Connection::Connection( Allocator & allocator, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, double time ) : m_connectionConfig( connectionConfig )
    {
        m_allocator = &allocator;
        m_messageFactory = &messageFactory;
        m_errorLevel = CONNECTION_ERROR_NONE;
        memset( m_channel, 0, sizeof( m_channel ) );
        assert( m_connectionConfig.numChannels >= 1 );
        assert( m_connectionConfig.numChannels <= MaxChannels );
        for ( int channelIndex = 0; channelIndex < m_connectionConfig.numChannels; ++channelIndex )
        {
            switch ( m_connectionConfig.channel[channelIndex].type )
            {
                case CHANNEL_TYPE_RELIABLE_ORDERED: 
                    m_channel[channelIndex] = YOJIMBO_NEW( *m_allocator, ReliableOrderedChannel, *m_allocator, messageFactory, m_connectionConfig.channel[channelIndex], channelIndex, time ); 
                    break;

                case CHANNEL_TYPE_UNRELIABLE_UNORDERED: 
                    m_channel[channelIndex] = YOJIMBO_NEW( *m_allocator, UnreliableUnorderedChannel, *m_allocator, messageFactory, m_connectionConfig.channel[channelIndex], channelIndex, time ); 
                    break;
                // todo: unreliable ordered channel
                default: 
                    assert( !"unknown channel type" );
            }
        }
    }

    Connection::~Connection()
    {
        assert( m_allocator );
        Reset();
        for ( int i = 0; i < m_connectionConfig.numChannels; ++i )
        {
            YOJIMBO_DELETE( *m_allocator, Channel, m_channel[i] );
        }
        m_allocator = NULL;
    }

    void Connection::Reset()
    {
        m_errorLevel = CONNECTION_ERROR_NONE;
        for ( int i = 0; i < m_connectionConfig.numChannels; ++i )
        {
            m_channel[i]->Reset();
        }
    }

    bool Connection::CanSendMessage( int channelIndex ) const
    {
        assert( channelIndex >= 0 );
        assert( channelIndex < m_connectionConfig.numChannels );
        return m_channel[channelIndex]->CanSendMessage();
    }

    void Connection::SendMessage( int channelIndex, Message * message )
    {
        assert( channelIndex >= 0 );
        assert( channelIndex < m_connectionConfig.numChannels );
        return m_channel[channelIndex]->SendMessage( message );
    }

    Message * Connection::ReceiveMessage( int channelIndex )
    {
        assert( channelIndex >= 0 );
        assert( channelIndex < m_connectionConfig.numChannels );
        return m_channel[channelIndex]->ReceiveMessage();
    }

    void Connection::ReleaseMessage( Message * message )
    {
        assert( message );
        m_messageFactory->ReleaseMessage( message );
    }

    static int WritePacket( void * context, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, ConnectionPacket & packet, uint8_t * buffer, int bufferSize )
    {
        WriteStream stream( messageFactory.GetAllocator(), buffer, bufferSize );

        stream.SetContext( context );

        if ( !packet.SerializeInternal( stream, messageFactory, connectionConfig ) )
        {
            debug_printf( "serialize connection packet failed (write packet)\n" );
            return 0;
        }

#if YOJIMBO_SERIALIZE_CHECKS
        if ( !stream.SerializeCheck() )
        {
            debug_printf( "serialize check at end of connection packed failed (write packet)\n" );
            return 0;
        }
#endif // #if YOJIMBO_SERIALIZE_CHECKS

        stream.Flush();

        return stream.GetBytesProcessed();
    }

    bool Connection::GeneratePacket( void * context, uint16_t packetSequence, uint8_t * packetData, int maxPacketBytes, int & packetBytes )
    {
        ConnectionPacket packet;

        if ( m_connectionConfig.numChannels > 0 )
        {
            int numChannelsWithData = 0;
            bool channelHasData[MaxChannels];
            memset( channelHasData, 0, sizeof( channelHasData ) );
            ChannelPacketData channelData[MaxChannels];

            int availableBits = maxPacketBytes * 8;

            availableBits -= ConservativeConnectionPacketHeaderEstimate;

            for ( int channelIndex = 0; channelIndex < m_connectionConfig.numChannels; ++channelIndex )
            {
                int packetDataBits = m_channel[channelIndex]->GetPacketData( channelData[channelIndex], packetSequence, availableBits );

                if ( packetDataBits > 0 )
                {
                    availableBits -= ConservativeChannelHeaderEstimate;

                    availableBits -= packetDataBits;

                    channelHasData[channelIndex] = true;

                    numChannelsWithData++;
                }
            }

            if ( numChannelsWithData > 0 )
            {
                if ( !packet.AllocateChannelData( *m_messageFactory, numChannelsWithData ) )
                {
                    m_errorLevel = CONNECTION_ERROR_ALLOCATOR;
                    return false;
                }

                int index = 0;

                for ( int channelIndex = 0; channelIndex < m_connectionConfig.numChannels; ++channelIndex )
                {
                    if ( channelHasData[channelIndex] )
                    {
                        memcpy( &packet.channelEntry[index], &channelData[channelIndex], sizeof( ChannelPacketData ) );
                        index++;
                    }
                }
            }
        }

        packetBytes = WritePacket( context, *m_messageFactory, m_connectionConfig, packet, packetData, maxPacketBytes );

        return true;
    }

    static bool ReadPacket( void * context, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, ConnectionPacket & packet, const uint8_t * buffer, int bufferSize )
    {
        assert( buffer );
        assert( bufferSize > 0 );

        ReadStream stream( messageFactory.GetAllocator(), buffer, bufferSize );

        stream.SetContext( context );

        if ( !packet.SerializeInternal( stream, messageFactory, connectionConfig ) )
        {
            debug_printf( "serialize connection packet failed (read packet)\n" );
            return false;
        }

#if YOJIMBO_SERIALIZE_CHECKS
        if ( !stream.SerializeCheck() )
        {
            debug_printf( "serialize check failed at end of connection packet (read packet)\n" );
            return false;
        }
#endif // #if YOJIMBO_SERIALIZE_CHECKS

        return true;
    }

    bool Connection::ProcessPacket( void * context, uint16_t packetSequence, const uint8_t * packetData, int packetBytes )
    {
        if ( m_errorLevel != CONNECTION_ERROR_NONE )
            return false;

        ConnectionPacket packet;

        if ( !ReadPacket( context, *m_messageFactory, m_connectionConfig, packet, packetData, packetBytes ) )
        {
            // todo: probably want to set a fatal error here
            return false;            
        }

        for ( int i = 0; i < packet.numChannelEntries; ++i )
        {
            const int channelIndex = packet.channelEntry[i].channelIndex;
            assert( channelIndex >= 0 );
            assert( channelIndex <= m_connectionConfig.numChannels );
            m_channel[channelIndex]->ProcessPacketData( packet.channelEntry[i], packetSequence );
            if ( m_channel[channelIndex]->GetErrorLevel() != CHANNEL_ERROR_NONE )
                return false;
        }

        return true;
    }

    void Connection::ProcessAcks( const uint16_t * acks, int numAcks )
    {
        for ( int i = 0; i < numAcks; ++i )
        {
            for ( int channelIndex = 0; channelIndex < m_connectionConfig.numChannels; ++channelIndex )
            {
                m_channel[channelIndex]->ProcessAck( acks[i] );
            }
        }
    }

    void Connection::AdvanceTime( double time )
    {
        for ( int i = 0; i < m_connectionConfig.numChannels; ++i )
        {
            m_channel[i]->AdvanceTime( time );

            if ( m_channel[i]->GetErrorLevel() != CHANNEL_ERROR_NONE )
            {
                m_errorLevel = CONNECTION_ERROR_CHANNEL;
                return;
            }
        }
        if ( m_allocator->GetErrorLevel() != ALLOCATOR_ERROR_NONE )
        {
            m_errorLevel = CONNECTION_ERROR_ALLOCATOR;
            return;
        }
        if ( m_messageFactory->GetErrorLevel() != MESSAGE_FACTORY_ERROR_NONE )
        {
            m_errorLevel = CONNECTION_ERROR_MESSAGE_FACTORY;
            return;
        }
    }
}
