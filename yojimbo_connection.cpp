/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "yojimbo_connection.h"

namespace yojimbo
{
    ConnectionPacket::ConnectionPacket()
    {
        m_messageFactory = NULL;
        sequence = 0;
        ack = 0;
        ack_bits = 0;
        numChannelEntries = 0;
        channelEntry = NULL;
    }

    ConnectionPacket::~ConnectionPacket()
    {
        if ( m_messageFactory )
        {
            for ( int i = 0; i < numChannelEntries; ++i )
            {
                channelEntry[i].Free( *m_messageFactory );
            }

            Allocator & allocator = m_messageFactory->GetAllocator();

            allocator.Free( channelEntry );
        }
    }

    bool ConnectionPacket::AllocateChannelData( MessageFactory & messageFactory, int numEntries )
    {
        assert( numEntries > 0 );
        assert( numEntries <= MaxChannels );

        SetMessageFactory( messageFactory );

        Allocator & allocator = messageFactory.GetAllocator();

        channelEntry = (ChannelPacketData*) allocator.Allocate( sizeof( ChannelPacketData ) * numEntries );

        numChannelEntries = numEntries;

        return channelEntry != NULL;
    }

    template <typename Stream> bool ConnectionPacket::Serialize( Stream & stream )
    {
        ConnectionContext * context = (ConnectionContext*) stream.GetContext();

        assert( context );
        assert( context->messageFactory );
        
        // ack system

        bool perfect_acks = Stream::IsWriting ? ( ack_bits == 0xFFFFFFFF ) : 0;

        serialize_bool( stream, perfect_acks );

        if ( !perfect_acks )
            serialize_bits( stream, ack_bits, 32 );
        else
            ack_bits = 0xFFFFFFFF;

        serialize_bits( stream, sequence, 16 );

        serialize_ack_relative( stream, sequence, ack );

        // channel entries

        const int numChannels = context->connectionConfig->numChannels;

        serialize_int( stream, numChannelEntries, 0, context->connectionConfig->numChannels );

#if YOJIMBO_VALIDATE_PACKET_BUDGET
        assert( stream.GetBitsProcessed() <= ConservativeConnectionPacketHeaderEstimate );
#endif // #if YOJIMBO_VALIDATE_PACKET_BUDGET

        if ( numChannelEntries > 0 )
        {
            if ( Stream::IsReading )
            {
                if ( !AllocateChannelData( *context->messageFactory, numChannelEntries ) )
                    return false;
            }

            for ( int i = 0; i < numChannelEntries; ++i )
            {
                if ( !channelEntry[i].SerializeInternal( stream, *m_messageFactory, context->connectionConfig->channelConfig, numChannels ) )
                    return false;
            }
        }

        return true;
    }

    bool ConnectionPacket::SerializeInternal( ReadStream & stream )
    {
        return Serialize( stream );
    }

    bool ConnectionPacket::SerializeInternal( WriteStream & stream )
    {
        return Serialize( stream );
    }

    bool ConnectionPacket::SerializeInternal( MeasureStream & stream )
    {
        return Serialize( stream );
    }

    Connection::Connection( Allocator & allocator, PacketFactory & packetFactory, MessageFactory & messageFactory, const ConnectionConfig & config ) : m_config( config )
    {
        assert( ( 65536 % config.slidingWindowSize ) == 0 );

        m_allocator = &allocator;

        m_packetFactory = &packetFactory;

        m_messageFactory = &messageFactory;

        m_listener = NULL;
        
        m_error = CONNECTION_ERROR_NONE;

        m_clientIndex = 0;

        memset( m_channel, 0, sizeof( m_channel ) );

        assert( m_config.numChannels >= 1 );
        assert( m_config.numChannels <= MaxChannels );

        for ( int channelId = 0; channelId < m_config.numChannels; ++channelId )
        {
            switch ( m_config.channelConfig[channelId].type )
            {
                case CHANNEL_TYPE_RELIABLE_ORDERED: 
                    m_channel[channelId] = YOJIMBO_NEW( *m_allocator, ReliableOrderedChannel, *m_allocator, messageFactory, m_config.channelConfig[channelId], channelId ); 
                    break;

                case CHANNEL_TYPE_UNRELIABLE_UNORDERED: 
                    m_channel[channelId] = YOJIMBO_NEW( *m_allocator, UnreliableUnorderedChannel, *m_allocator, messageFactory, m_config.channelConfig[channelId], channelId ); 
                    break;

                default: 
                    assert( !"unknown channel type" );
            }

            m_channel[channelId]->SetListener( this );
        }

        m_sentPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionSentPacketData>, *m_allocator, m_config.slidingWindowSize );
        
        m_receivedPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionReceivedPacketData>, *m_allocator, m_config.slidingWindowSize );

        Reset();
    }

    Connection::~Connection()
    {
        Reset();

        for ( int i = 0; i < m_config.numChannels; ++i )
            YOJIMBO_DELETE( *m_allocator, Channel, m_channel[i] );

        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionSentPacketData>, m_sentPackets );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionReceivedPacketData>, m_receivedPackets );
    }

    void Connection::Reset()
    {
        m_error = CONNECTION_ERROR_NONE;

        for ( int i = 0; i < m_config.numChannels; ++i )
            m_channel[i]->Reset();

        m_sentPackets->Reset();
        m_receivedPackets->Reset();

        memset( m_counters, 0, sizeof( m_counters ) );
    }

    bool Connection::CanSendMessage( int channelId ) const
    {
        assert( channelId >= 0 );
        assert( channelId < m_config.numChannels );
        return m_channel[channelId]->CanSendMessage();
    }

    void Connection::SendMessage( Message * message, int channelId )
    {
        assert( channelId >= 0 );
        assert( channelId < m_config.numChannels );
        return m_channel[channelId]->SendMessage( message );
    }

    Message * Connection::ReceiveMessage( int channelId )
    {
        assert( channelId >= 0 );
        assert( channelId < m_config.numChannels );
        return m_channel[channelId]->ReceiveMessage();
    }

    ConnectionPacket * Connection::WritePacket()
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return NULL;

        ConnectionPacket * packet = (ConnectionPacket*) m_packetFactory->CreatePacket( m_config.connectionPacketType );

        if ( !packet )
            return NULL;

        packet->sequence = m_sentPackets->GetSequence();

        GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

        InsertAckPacketEntry( packet->sequence );

        m_counters[CONNECTION_COUNTER_PACKETS_WRITTEN]++;

        if ( m_config.numChannels == 0 )
            return packet;

        int numChannelsWithData = 0;
        bool channelHasData[MaxChannels];
        memset( channelHasData, 0, sizeof( channelHasData ) );
        ChannelPacketData channelData[MaxChannels];

        int availableBits = m_config.maxPacketSize * 8;

        availableBits -= ConservativeConnectionPacketHeaderEstimate;

        for ( int channelId = 0; channelId < m_config.numChannels; ++channelId )
        {
            int packetDataBits = m_channel[channelId]->GetPacketData( channelData[channelId], packet->sequence, availableBits );

            if ( packetDataBits > 0 )
            {
                availableBits -= ConservativeChannelHeaderEstimate;

                availableBits -= packetDataBits;

                channelHasData[channelId] = true;

                numChannelsWithData++;
            }
        }

        if ( numChannelsWithData > 0 )
        {
            if ( !packet->AllocateChannelData( *m_messageFactory, numChannelsWithData ) )
            {
                m_error = CONNECTION_ERROR_OUT_OF_MEMORY;
                return NULL;
            }

            int index = 0;

            for ( int channelId = 0; channelId < m_config.numChannels; ++channelId )
            {
                if ( channelHasData[channelId] )
                {
                    memcpy( &packet->channelEntry[index], &channelData[channelId], sizeof( ChannelPacketData ) );
                    index++;
                }
            }
        }

        return packet;
    }

    bool Connection::ReadPacket( ConnectionPacket * packet )
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return false;

        assert( packet );
        assert( packet->GetType() == m_config.connectionPacketType );

        m_counters[CONNECTION_COUNTER_PACKETS_READ]++;

        if ( m_listener )
            m_listener->OnConnectionPacketReceived( this, packet->sequence );

        if ( !m_receivedPackets->Insert( packet->sequence ) )
            return false;

        ProcessAcks( packet->ack, packet->ack_bits );

        for ( int i = 0; i < packet->numChannelEntries; ++i )
        {
            const int channelId = packet->channelEntry[i].channelId;

            assert( channelId >= 0 );
            assert( channelId <= m_config.numChannels );

            m_channel[channelId]->ProcessPacketData( packet->channelEntry[i], packet->sequence );
        }

        return true;
    }

    void Connection::AdvanceTime( double time )
    {
        for ( int i = 0; i < m_config.numChannels; ++i )
        {
            m_channel[i]->AdvanceTime( time );

            ChannelError error = m_channel[i]->GetError();

            if ( error != CHANNEL_ERROR_NONE )
            {
                m_error = CONNECTION_ERROR_CHANNEL;
                return;
            }
        }
    }
    
    uint64_t Connection::GetCounter( int index ) const
    {
        assert( index >= 0 );
        assert( index < CONNECTION_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }

    ConnectionError Connection::GetError() const
    {
        return m_error;
    }

    void Connection::InsertAckPacketEntry( uint16_t sequence )
    {
        ConnectionSentPacketData * entry = m_sentPackets->Insert( sequence );
        
        assert( entry );

        if ( entry )
        {
            entry->acked = 0;
        }
    }

    void Connection::ProcessAcks( uint16_t ack, uint32_t ack_bits )
    {
        for ( int i = 0; i < 32; ++i )
        {
            if ( ack_bits & 1 )
            {                    
                const uint16_t sequence = ack - i;
                ConnectionSentPacketData * packetData = m_sentPackets->Find( sequence );
                if ( packetData && !packetData->acked )
                {
                    PacketAcked( sequence );
                    packetData->acked = 1;
                }
            }
            ack_bits >>= 1;
        }
    }

    void Connection::PacketAcked( uint16_t sequence )
    {
        OnPacketAcked( sequence );

        for ( int channelId = 0; channelId < m_config.numChannels; ++channelId )
            m_channel[channelId]->ProcessAck( sequence );

        m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;
    }

    void Connection::OnChannelFragmentReceived( class Channel * channel, uint16_t messageId, uint16_t fragmentId )
    {
        if ( m_listener )
        {
            m_listener->OnConnectionFragmentReceived( this, messageId, fragmentId, channel->GetChannelId() );
        }
    }
}
