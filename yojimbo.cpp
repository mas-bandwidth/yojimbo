/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2024, Mas Bandwidth LLC.

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

#include "yojimbo.h"
#include "yojimbo_utils.h"

#ifdef _MSC_VER
#define SODIUM_STATIC
#endif // #ifdef _MSC_VER

#include <sodium.h>

#if YOJIMBO_DEBUG_MEMORY_LEAKS
#include <map>
#endif // YOJIMBO_DEBUG_MEMORY_LEAKS

static yojimbo::Allocator * g_defaultAllocator = NULL;

namespace yojimbo
{
    Allocator & GetDefaultAllocator()
    {
        yojimbo_assert( g_defaultAllocator );
        return *g_defaultAllocator;
    }
}

extern "C" int netcode_init();
extern "C" int reliable_init();
extern "C" void netcode_term();
extern "C" void reliable_term();

#define NETCODE_OK 1
#define RELIABLE_OK 1

bool InitializeYojimbo()
{
    g_defaultAllocator = new yojimbo::DefaultAllocator();

    if ( netcode_init() != NETCODE_OK )
        return false;

    if ( reliable_init() != RELIABLE_OK )
        return false;

    return sodium_init() != -1;
}

void ShutdownYojimbo()
{
    reliable_term();

    netcode_term();

    yojimbo_assert( g_defaultAllocator );
    delete g_defaultAllocator;
    g_defaultAllocator = NULL;
}

// ---------------------------------------------------------------------------------

#ifdef _MSC_VER
#include <malloc.h>
#endif // #ifdef _MSC_VER
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>

    // ------------------------------------------------

namespace yojimbo
{
    UnreliableUnorderedChannel::UnreliableUnorderedChannel( Allocator & allocator, 
                                                            MessageFactory & messageFactory, 
                                                            const ChannelConfig & config, 
                                                            int channelIndex, 
                                                            double time ) 
        : Channel( allocator, 
                   messageFactory, 
                   config, 
                   channelIndex, 
                   time )
    {
        yojimbo_assert( config.type == CHANNEL_TYPE_UNRELIABLE_UNORDERED );
        m_messageSendQueue = YOJIMBO_NEW( *m_allocator, Queue<Message*>, *m_allocator, m_config.messageSendQueueSize );
        m_messageReceiveQueue = YOJIMBO_NEW( *m_allocator, Queue<Message*>, *m_allocator, m_config.messageReceiveQueueSize );
        Reset();
    }

    UnreliableUnorderedChannel::~UnreliableUnorderedChannel()
    {
        Reset();
        YOJIMBO_DELETE( *m_allocator, Queue<Message*>, m_messageSendQueue );
        YOJIMBO_DELETE( *m_allocator, Queue<Message*>, m_messageReceiveQueue );
    }

    void UnreliableUnorderedChannel::Reset()
    {
        SetErrorLevel( CHANNEL_ERROR_NONE );

        for ( int i = 0; i < m_messageSendQueue->GetNumEntries(); ++i )
            m_messageFactory->ReleaseMessage( (*m_messageSendQueue)[i] );

        for ( int i = 0; i < m_messageReceiveQueue->GetNumEntries(); ++i )
            m_messageFactory->ReleaseMessage( (*m_messageReceiveQueue)[i] );

        m_messageSendQueue->Clear();
        m_messageReceiveQueue->Clear();
  
        ResetCounters();
    }

    bool UnreliableUnorderedChannel::CanSendMessage() const
    {
        yojimbo_assert( m_messageSendQueue );
        return !m_messageSendQueue->IsFull();
    }

    bool UnreliableUnorderedChannel::HasMessagesToSend() const
    {
        yojimbo_assert( m_messageSendQueue );
        return !m_messageSendQueue->IsEmpty();
    }

    void UnreliableUnorderedChannel::SendMessage( Message * message, void *context )
    {
        yojimbo_assert( message );
        yojimbo_assert( CanSendMessage() );
		(void)context;

        if ( GetErrorLevel() != CHANNEL_ERROR_NONE )
        {
            m_messageFactory->ReleaseMessage( message );
            return;
        }

        if ( !CanSendMessage() )
        {
            SetErrorLevel( CHANNEL_ERROR_SEND_QUEUE_FULL );
            m_messageFactory->ReleaseMessage( message );
            return;
        }

        yojimbo_assert( !( message->IsBlockMessage() && m_config.disableBlocks ) );

        if ( message->IsBlockMessage() && m_config.disableBlocks )
        {
            SetErrorLevel( CHANNEL_ERROR_BLOCKS_DISABLED );
            m_messageFactory->ReleaseMessage( message );
            return;
        }

        if ( message->IsBlockMessage() )
        {
            yojimbo_assert( ((BlockMessage*)message)->GetBlockSize() > 0 );
            yojimbo_assert( ((BlockMessage*)message)->GetBlockSize() <= m_config.maxBlockSize );
        }

        m_messageSendQueue->Push( message );

        m_counters[CHANNEL_COUNTER_MESSAGES_SENT]++;
    }

    Message * UnreliableUnorderedChannel::ReceiveMessage()
    {
        if ( GetErrorLevel() != CHANNEL_ERROR_NONE )
            return NULL;

        if ( m_messageReceiveQueue->IsEmpty() )
            return NULL;

        m_counters[CHANNEL_COUNTER_MESSAGES_RECEIVED]++;

        return m_messageReceiveQueue->Pop();
    }

    void UnreliableUnorderedChannel::AdvanceTime( double time )
    {
        (void) time;
    }
    
    int UnreliableUnorderedChannel::GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits )
    {
        (void) packetSequence;

        if ( m_messageSendQueue->IsEmpty() )
            return 0;

        if ( m_config.packetBudget > 0 )
            availableBits = yojimbo_min( m_config.packetBudget * 8, availableBits );

        const int giveUpBits = 4 * 8;

        const int messageTypeBits = bits_required( 0, m_messageFactory->GetNumTypes() - 1 );

        int usedBits = ConservativeMessageHeaderBits;
        int numMessages = 0;
        Message ** messages = (Message**) alloca( sizeof( Message* ) * m_config.maxMessagesPerPacket );

        while ( true )
        {
            if ( m_messageSendQueue->IsEmpty() )
                break;

            if ( availableBits - usedBits < giveUpBits )
                break;

            if ( numMessages == m_config.maxMessagesPerPacket )
                break;

            Message * message = m_messageSendQueue->Pop();

            yojimbo_assert( message );

            MeasureStream measureStream;
			measureStream.SetContext( context );
            measureStream.SetAllocator( &m_messageFactory->GetAllocator() );
            message->SerializeInternal( measureStream );
            
            if ( message->IsBlockMessage() )
            {
                BlockMessage * blockMessage = (BlockMessage*) message;
                SerializeMessageBlock( measureStream, *m_messageFactory, blockMessage, m_config.maxBlockSize );
            }

            const int messageBits = messageTypeBits + measureStream.GetBitsProcessed();
            
            if ( usedBits + messageBits > availableBits )
            {
                m_messageFactory->ReleaseMessage( message );
                continue;
            }

            usedBits += messageBits;        

            yojimbo_assert( usedBits <= availableBits );

            messages[numMessages++] = message;
        }

        if ( numMessages == 0 )
            return 0;

        Allocator & allocator = m_messageFactory->GetAllocator();

        packetData.Initialize();
        packetData.channelIndex = GetChannelIndex();
        packetData.message.numMessages = numMessages;
        packetData.message.messages = (Message**) YOJIMBO_ALLOCATE( allocator, sizeof( Message* ) * numMessages );
        for ( int i = 0; i < numMessages; ++i )
        {
            packetData.message.messages[i] = messages[i];
        }

        return usedBits;
    }

    void UnreliableUnorderedChannel::ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence )
    {
        if ( m_errorLevel != CHANNEL_ERROR_NONE )
            return;
        
        if ( packetData.messageFailedToSerialize )
        {
            SetErrorLevel( CHANNEL_ERROR_FAILED_TO_SERIALIZE );
            return;
        }

        for ( int i = 0; i < (int) packetData.message.numMessages; ++i )
        {
            Message * message = packetData.message.messages[i];
            yojimbo_assert( message );  
            message->SetId( packetSequence );
            if ( !m_messageReceiveQueue->IsFull() )
            {
                m_messageFactory->AcquireMessage( message );
                m_messageReceiveQueue->Push( message );
            }
        }
    }

    void UnreliableUnorderedChannel::ProcessAck( uint16_t ack )
    {
        (void) ack;
    }
}

// ---------------------------------------------------------------------------------

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

        bool AllocateChannelData( MessageFactory & _messageFactory, int numEntries )
        {
            yojimbo_assert( numEntries > 0 );
            yojimbo_assert( numEntries <= MaxChannels );
            messageFactory = &_messageFactory;
            Allocator & allocator = messageFactory->GetAllocator();
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
            yojimbo_assert( stream.GetBitsProcessed() <= ConservativePacketHeaderBits );
#endif // #if YOJIMBO_DEBUG_MESSAGE_BUDGET
            if ( numChannelEntries > 0 )
            {
                if ( Stream::IsReading )
                {
                    if ( !AllocateChannelData( messageFactory, numChannelEntries ) )
                    {
                        yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to allocate channel data (ConnectionPacket)\n" );
                        return false;
                    }
                    for ( int i = 0; i < numChannelEntries; ++i )
                    {
                        yojimbo_assert( channelEntry[i].messageFailedToSerialize == 0 );
                    }
                }
                for ( int i = 0; i < numChannelEntries; ++i )
                {
                    yojimbo_assert( channelEntry[i].messageFailedToSerialize == 0 );
                    if ( !channelEntry[i].SerializeInternal( stream, messageFactory, connectionConfig.channel, numChannels ) )
                    {
                        yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to serialize channel %d\n", i );
                        return false;
                    }
                }
            }
            return true;
        }

        bool SerializeInternal( ReadStream & stream, MessageFactory & _messageFactory, const ConnectionConfig & connectionConfig )
        {
            return Serialize( stream, _messageFactory, connectionConfig );
        }

        bool SerializeInternal( WriteStream & stream, MessageFactory & _messageFactory, const ConnectionConfig & connectionConfig )
        {
            return Serialize( stream, _messageFactory, connectionConfig );            
        }

        bool SerializeInternal( MeasureStream & stream, MessageFactory & _messageFactory, const ConnectionConfig & connectionConfig )
        {
            return Serialize( stream, _messageFactory, connectionConfig );            
        }

    private:

        ConnectionPacket( const ConnectionPacket & other );

        const ConnectionPacket & operator = ( const ConnectionPacket & other );
    };

    // ------------------------------------------------------------------------------

    Connection::Connection( Allocator & allocator, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, double time ) 
        : m_connectionConfig( connectionConfig )
    {
        m_allocator = &allocator;
        m_messageFactory = &messageFactory;
        m_errorLevel = CONNECTION_ERROR_NONE;
        memset( m_channel, 0, sizeof( m_channel ) );
        yojimbo_assert( m_connectionConfig.numChannels >= 1 );
        yojimbo_assert( m_connectionConfig.numChannels <= MaxChannels );
        for ( int channelIndex = 0; channelIndex < m_connectionConfig.numChannels; ++channelIndex )
        {
            switch ( m_connectionConfig.channel[channelIndex].type )
            {
                case CHANNEL_TYPE_RELIABLE_ORDERED: 
                {
                    m_channel[channelIndex] = YOJIMBO_NEW( *m_allocator, 
                                                           ReliableOrderedChannel, 
                                                           *m_allocator, 
                                                           messageFactory, 
                                                           m_connectionConfig.channel[channelIndex],
                                                           channelIndex, 
                                                           time ); 
                }
                break;

                case CHANNEL_TYPE_UNRELIABLE_UNORDERED: 
                {
                    m_channel[channelIndex] = YOJIMBO_NEW( *m_allocator, 
                                                           UnreliableUnorderedChannel, 
                                                           *m_allocator, 
                                                           messageFactory, 
                                                           m_connectionConfig.channel[channelIndex], 
                                                           channelIndex, 
                                                           time ); 
                }
                break;

                default: 
                    yojimbo_assert( !"unknown channel type" );
            }
        }
    }

    Connection::~Connection()
    {
        yojimbo_assert( m_allocator );
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
        yojimbo_assert( channelIndex >= 0 );
        yojimbo_assert( channelIndex < m_connectionConfig.numChannels );
        return m_channel[channelIndex]->CanSendMessage();
    }

    bool Connection::HasMessagesToSend( int channelIndex ) const {
        yojimbo_assert( channelIndex >= 0 );
        yojimbo_assert( channelIndex < m_connectionConfig.numChannels );
        return m_channel[channelIndex]->HasMessagesToSend();
    }

    void Connection::SendMessage( int channelIndex, Message * message, void *context)
    {
        yojimbo_assert( channelIndex >= 0 );
        yojimbo_assert( channelIndex < m_connectionConfig.numChannels );
        return m_channel[channelIndex]->SendMessage( message, context );
    }

    Message * Connection::ReceiveMessage( int channelIndex )
    {
        yojimbo_assert( channelIndex >= 0 );
        yojimbo_assert( channelIndex < m_connectionConfig.numChannels );
        return m_channel[channelIndex]->ReceiveMessage();
    }

    void Connection::ReleaseMessage( Message * message )
    {
        yojimbo_assert( message );
        m_messageFactory->ReleaseMessage( message );
    }

    static int WritePacket( void * context, 
                            MessageFactory & messageFactory, 
                            const ConnectionConfig & connectionConfig, 
                            ConnectionPacket & packet, 
                            uint8_t * buffer, 
                            int bufferSize )
    {
        WriteStream stream( buffer, bufferSize );
        
        stream.SetContext( context );

        stream.SetAllocator( &messageFactory.GetAllocator() );
        
        if ( !packet.SerializeInternal( stream, messageFactory, connectionConfig ) )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: serialize connection packet failed (write packet)\n" );
            return 0;
        }

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
            
            int availableBits = maxPacketBytes * 8 - ConservativePacketHeaderBits;
            
            for ( int channelIndex = 0; channelIndex < m_connectionConfig.numChannels; ++channelIndex )
            {
                int packetDataBits = m_channel[channelIndex]->GetPacketData( context, channelData[channelIndex], packetSequence, availableBits );
                if ( packetDataBits > 0 )
                {
                    availableBits -= ConservativeChannelHeaderBits;
                    availableBits -= packetDataBits;
                    channelHasData[channelIndex] = true;
                    numChannelsWithData++;
                }
            }

            if ( numChannelsWithData > 0 )
            {
                if ( !packet.AllocateChannelData( *m_messageFactory, numChannelsWithData ) )
                {
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to allocate channel data\n" );
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

    static bool ReadPacket( void * context, 
                            MessageFactory & messageFactory, 
                            const ConnectionConfig & connectionConfig, 
                            ConnectionPacket & packet, 
                            const uint8_t * buffer, 
                            int bufferSize )
    {
        yojimbo_assert( buffer );
        yojimbo_assert( bufferSize > 0 );

        ReadStream stream( buffer, bufferSize );
        
        stream.SetContext( context );

        stream.SetAllocator( &messageFactory.GetAllocator() );
        
        if ( !packet.SerializeInternal( stream, messageFactory, connectionConfig ) )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: serialize connection packet failed (read packet)\n" );
            return false;
        }

        return true;
    }

    bool Connection::ProcessPacket( void * context, uint16_t packetSequence, const uint8_t * packetData, int packetBytes )
    {
        if ( m_errorLevel != CONNECTION_ERROR_NONE )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_DEBUG, "failed to read packet because connection is in error state\n" );
            return false;
        }

        ConnectionPacket packet;

        if ( !ReadPacket( context, *m_messageFactory, m_connectionConfig, packet, packetData, packetBytes ) )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to read packet\n" );
            m_errorLevel = CONNECTION_ERROR_READ_PACKET_FAILED;
            return false;            
        }

        for ( int i = 0; i < packet.numChannelEntries; ++i )
        {
            const int channelIndex = packet.channelEntry[i].channelIndex;
            yojimbo_assert( channelIndex >= 0 );
            yojimbo_assert( channelIndex <= m_connectionConfig.numChannels );
            m_channel[channelIndex]->ProcessPacketData( packet.channelEntry[i], packetSequence );
            if ( m_channel[channelIndex]->GetErrorLevel() != CHANNEL_ERROR_NONE )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_DEBUG, "failed to read packet because channel %d is in error state\n", channelIndex );
                return false;
            }
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

// ---------------------------------------------------------------------------------

#include "reliable.h"

namespace yojimbo
{
    BaseClient::BaseClient( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time ) : m_config( config )
    {
        m_allocator = &allocator;
        m_adapter = &adapter;
        m_time = time;
        m_context = NULL;
        m_clientMemory = NULL;
        m_clientAllocator = NULL;
        m_endpoint = NULL;
        m_connection = NULL;
        m_messageFactory = NULL;
        m_networkSimulator = NULL;
        m_clientState = CLIENT_STATE_DISCONNECTED;
        m_clientIndex = -1;
        m_packetBuffer = (uint8_t*) YOJIMBO_ALLOCATE( allocator, config.maxPacketSize );
    }

    BaseClient::~BaseClient()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        yojimbo_assert( m_clientState <= CLIENT_STATE_DISCONNECTED );
        YOJIMBO_FREE( *m_allocator, m_packetBuffer );
        m_allocator = NULL;
    }

    void BaseClient::Disconnect()
    {
        SetClientState( CLIENT_STATE_DISCONNECTED );
    }

    void BaseClient::AdvanceTime( double time )
    {
        m_time = time;
        if ( m_endpoint )
        {
            m_connection->AdvanceTime( time );
            if ( m_connection->GetErrorLevel() != CONNECTION_ERROR_NONE )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_DEBUG, "connection error. disconnecting client\n" );
                Disconnect();
                return;
            }
            reliable_endpoint_update( m_endpoint, m_time );
            int numAcks;
            const uint16_t * acks = reliable_endpoint_get_acks( m_endpoint, &numAcks );
            m_connection->ProcessAcks( acks, numAcks );
            reliable_endpoint_clear_acks( m_endpoint );
        }
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator )
        {
            networkSimulator->AdvanceTime( time );
        }
    }

    void BaseClient::SetLatency( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetLatency( milliseconds );
        }
    }

    void BaseClient::SetJitter( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetJitter( milliseconds );
        }
    }

    void BaseClient::SetPacketLoss( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetPacketLoss( percent );
        }
    }

    void BaseClient::SetDuplicates( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetDuplicates( percent );
        }
    }

    void BaseClient::SetClientState( ClientState clientState )
    {
        m_clientState = clientState;
    }

    void BaseClient::CreateInternal()
    {
        yojimbo_assert( m_allocator );
        yojimbo_assert( m_adapter );
        yojimbo_assert( m_clientMemory == NULL );
        yojimbo_assert( m_clientAllocator == NULL );
        yojimbo_assert( m_messageFactory == NULL );
        m_clientMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.clientMemory );
        m_clientAllocator = m_adapter->CreateAllocator( *m_allocator, m_clientMemory, m_config.clientMemory );
        m_messageFactory = m_adapter->CreateMessageFactory( *m_clientAllocator );
        m_connection = YOJIMBO_NEW( *m_clientAllocator, Connection, *m_clientAllocator, *m_messageFactory, m_config, m_time );
        yojimbo_assert( m_connection );
        if ( m_config.networkSimulator )
        {
            m_networkSimulator = YOJIMBO_NEW( *m_clientAllocator, NetworkSimulator, *m_clientAllocator, m_config.maxSimulatorPackets, m_time );
        }
        reliable_config_t reliable_config;
        reliable_default_config( &reliable_config );
        strcpy( reliable_config.name, "client endpoint" );
        reliable_config.context = (void*) this;
        reliable_config.max_packet_size = m_config.maxPacketSize;
        reliable_config.fragment_above = m_config.fragmentPacketsAbove;
        reliable_config.max_fragments = m_config.maxPacketFragments;
        reliable_config.fragment_size = m_config.packetFragmentSize; 
        reliable_config.ack_buffer_size = m_config.ackedPacketsBufferSize;
        reliable_config.received_packets_buffer_size = m_config.receivedPacketsBufferSize;
        reliable_config.fragment_reassembly_buffer_size = m_config.packetReassemblyBufferSize;
        reliable_config.rtt_smoothing_factor = m_config.rttSmoothingFactor;
        reliable_config.transmit_packet_function = BaseClient::StaticTransmitPacketFunction;
        reliable_config.process_packet_function = BaseClient::StaticProcessPacketFunction;
        reliable_config.allocator_context = m_clientAllocator;
        reliable_config.allocate_function = BaseClient::StaticAllocateFunction;
        reliable_config.free_function = BaseClient::StaticFreeFunction;
        m_endpoint = reliable_endpoint_create( &reliable_config, m_time );
        reliable_endpoint_reset( m_endpoint );
    }

    void BaseClient::DestroyInternal()
    {
        yojimbo_assert( m_allocator );
        if ( m_endpoint )
        {
            reliable_endpoint_destroy( m_endpoint ); 
            m_endpoint = NULL;
        }
        YOJIMBO_DELETE( *m_clientAllocator, NetworkSimulator, m_networkSimulator );
        YOJIMBO_DELETE( *m_clientAllocator, Connection, m_connection );
        YOJIMBO_DELETE( *m_clientAllocator, MessageFactory, m_messageFactory );
        YOJIMBO_DELETE( *m_allocator, Allocator, m_clientAllocator );
        YOJIMBO_FREE( *m_allocator, m_clientMemory );
    }

    void BaseClient::StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) index;
        BaseClient * client = (BaseClient*) context;
        client->TransmitPacketFunction( packetSequence, packetData, packetBytes );
    }
    
    int BaseClient::StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) index;
        BaseClient * client = (BaseClient*) context;
        return client->ProcessPacketFunction( packetSequence, packetData, packetBytes );
    }

    void * BaseClient::StaticAllocateFunction( void * context, uint64_t bytes )
    {
        yojimbo_assert( context );
        Allocator * allocator = (Allocator*) context;
        return YOJIMBO_ALLOCATE( *allocator, bytes );
    }
    
    void BaseClient::StaticFreeFunction( void * context, void * pointer )
    {
        yojimbo_assert( context );
        yojimbo_assert( pointer );
        Allocator * allocator = (Allocator*) context;
        YOJIMBO_FREE( *allocator, pointer );
    }

    Message * BaseClient::CreateMessage( int type )
    {
        yojimbo_assert( m_messageFactory );
        return m_messageFactory->CreateMessage( type );
    }

    uint8_t * BaseClient::AllocateBlock( int bytes )
    {
        return (uint8_t*) YOJIMBO_ALLOCATE( *m_clientAllocator, bytes );
    }

    void BaseClient::AttachBlockToMessage( Message * message, uint8_t * block, int bytes )
    {
        yojimbo_assert( message );
        yojimbo_assert( block );
        yojimbo_assert( bytes > 0 );
        yojimbo_assert( message->IsBlockMessage() );
        BlockMessage * blockMessage = (BlockMessage*) message;
        blockMessage->AttachBlock( *m_clientAllocator, block, bytes );
    }

    void BaseClient::FreeBlock( uint8_t * block )
    {
        YOJIMBO_FREE( *m_clientAllocator, block );
    }

    bool BaseClient::CanSendMessage( int channelIndex ) const
    {
        yojimbo_assert( m_connection );
        return m_connection->CanSendMessage( channelIndex );
    }

    bool BaseClient::HasMessagesToSend( int channelIndex ) const
    {
        yojimbo_assert( m_connection );
        return m_connection->HasMessagesToSend( channelIndex );
    }

    void BaseClient::SendMessage( int channelIndex, Message * message )
    {
        yojimbo_assert( m_connection );
        m_connection->SendMessage( channelIndex, message, GetContext() );
    }

    Message * BaseClient::ReceiveMessage( int channelIndex )
    {
        yojimbo_assert( m_connection );
        return m_connection->ReceiveMessage( channelIndex );
    }

    void BaseClient::ReleaseMessage( Message * message )
    {
        yojimbo_assert( m_connection );
        m_connection->ReleaseMessage( message );
    }

    void BaseClient::GetNetworkInfo( NetworkInfo & info ) const
    {
        memset( &info, 0, sizeof( info ) );
        if ( m_connection )
        {
            yojimbo_assert( m_endpoint );
            const uint64_t * counters = reliable_endpoint_counters( m_endpoint );
            info.numPacketsSent = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT];
            info.numPacketsReceived = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED];
            info.numPacketsAcked = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED];
            info.RTT = reliable_endpoint_rtt( m_endpoint );
            info.packetLoss = reliable_endpoint_packet_loss( m_endpoint );
            reliable_endpoint_bandwidth( m_endpoint, &info.sentBandwidth, &info.receivedBandwidth, &info.ackedBandwidth );
        }
    }
}

    // ------------------------------------------------------------------------------------------------------------------

#include "netcode.h"

namespace yojimbo
{
    Client::Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time ) 
        : BaseClient( allocator, config, adapter, time ), m_config( config ), m_address( address )
    {
        m_clientId = 0;
        m_client = NULL;
        m_boundAddress = m_address;
    }

    Client::~Client()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        yojimbo_assert( m_client == NULL );
    }

    void Client::InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address & address )
    {
        InsecureConnect( privateKey, clientId, &address, 1 );
    }

    void Client::InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses )
    {
        yojimbo_assert( serverAddresses );
        yojimbo_assert( numServerAddresses > 0 );
        yojimbo_assert( numServerAddresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        if ( !m_client )
        {
            Disconnect();
            return;
        }
        uint8_t connectToken[NETCODE_CONNECT_TOKEN_BYTES];
        if ( !GenerateInsecureConnectToken( connectToken, privateKey, clientId, serverAddresses, numServerAddresses ) )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to generate insecure connect token\n" );
            SetClientState( CLIENT_STATE_ERROR );
            return;
        }
        netcode_client_connect( m_client, connectToken );
        SetClientState( CLIENT_STATE_CONNECTING );
    }

    bool Client::GenerateInsecureConnectToken( uint8_t * connectToken, 
                                               const uint8_t privateKey[], 
                                               uint64_t clientId, 
                                               const Address serverAddresses[], 
                                               int numServerAddresses )
    {
        char serverAddressStrings[NETCODE_MAX_SERVERS_PER_CONNECT][MaxAddressLength];
        const char * serverAddressStringPointers[NETCODE_MAX_SERVERS_PER_CONNECT];
        for ( int i = 0; i < numServerAddresses; ++i ) 
        {
            serverAddresses[i].ToString( serverAddressStrings[i], MaxAddressLength );
            serverAddressStringPointers[i] = serverAddressStrings[i];
        }

        uint8_t userData[256];
        memset( &userData, 0, sizeof(userData) );

        return netcode_generate_connect_token( numServerAddresses, 
                                               serverAddressStringPointers, 
                                               serverAddressStringPointers, 
                                               m_config.timeout,
                                               m_config.timeout, 
                                               clientId, 
                                               m_config.protocolId, 
                                               (uint8_t*)privateKey,
                                               &userData[0], 
                                               connectToken ) == NETCODE_OK;
    }

    void Client::Connect( uint64_t clientId, uint8_t * connectToken )
    {
        yojimbo_assert( connectToken );
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        netcode_client_connect( m_client, connectToken );
        if ( netcode_client_state( m_client ) > NETCODE_CLIENT_STATE_DISCONNECTED )
        {
            SetClientState( CLIENT_STATE_CONNECTING );
        }
        else
        {
            Disconnect();
        }
    }

    void Client::Disconnect()
    {
        BaseClient::Disconnect();
        DestroyClient();
        DestroyInternal();
        m_clientId = 0;
    }

    void Client::SendPackets()
    {
        if ( !IsConnected() )
            return;
        yojimbo_assert( m_client );
        uint8_t * packetData = GetPacketBuffer();
        int packetBytes;
        uint16_t packetSequence = reliable_endpoint_next_packet_sequence( GetEndpoint() );
        if ( GetConnection().GeneratePacket( GetContext(), packetSequence, packetData, m_config.maxPacketSize, packetBytes ) )
        {
            reliable_endpoint_send_packet( GetEndpoint(), packetData, packetBytes );
        }
    }

    void Client::ReceivePackets()
    {
        if ( !IsConnected() )
            return;
        yojimbo_assert( m_client );
        while ( true )
        {
            int packetBytes;
            uint64_t packetSequence;
            uint8_t * packetData = netcode_client_receive_packet( m_client, &packetBytes, &packetSequence );
            if ( !packetData )
                break;
            reliable_endpoint_receive_packet( GetEndpoint(), packetData, packetBytes );
            netcode_client_free_packet( m_client, packetData );
        }
    }

    void Client::AdvanceTime( double time )
    {
        BaseClient::AdvanceTime( time );
        if ( m_client )
        {
            netcode_client_update( m_client, time );
            const int state = netcode_client_state( m_client );
            if ( state < NETCODE_CLIENT_STATE_DISCONNECTED )
            {
                Disconnect();
                SetClientState( CLIENT_STATE_ERROR );
            }
            else if ( state == NETCODE_CLIENT_STATE_DISCONNECTED )
            {
                Disconnect();
                SetClientState( CLIENT_STATE_DISCONNECTED );
            }
            else if ( state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST || state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE )
            {
                SetClientState( CLIENT_STATE_CONNECTING );
            }
            else
            {
                SetClientState( CLIENT_STATE_CONNECTED );
            }
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator && networkSimulator->IsActive() )
            {
                uint8_t ** packetData = (uint8_t**) alloca( sizeof( uint8_t*) * m_config.maxSimulatorPackets );
                int * packetBytes = (int*) alloca( sizeof(int) * m_config.maxSimulatorPackets );
                int numPackets = networkSimulator->ReceivePackets( m_config.maxSimulatorPackets, packetData, packetBytes, NULL );
                for ( int i = 0; i < numPackets; ++i )
                {
                    netcode_client_send_packet( m_client, (uint8_t*) packetData[i], packetBytes[i] );
                    YOJIMBO_FREE( networkSimulator->GetAllocator(), packetData[i] );
                }
            }
        }
    }

    int Client::GetClientIndex() const
    {
        return m_client ? netcode_client_index( m_client ) : -1;
    }

    void Client::ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients )
    {
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        netcode_client_connect_loopback( m_client, clientIndex, maxClients );
        SetClientState( CLIENT_STATE_CONNECTED );
    }

    void Client::DisconnectLoopback()
    {
        netcode_client_disconnect_loopback( m_client );
        BaseClient::Disconnect();
        DestroyClient();
        DestroyInternal();
        m_clientId = 0;
    }

    bool Client::IsLoopback() const
    {
        return netcode_client_loopback( m_client ) != 0;
    }

    void Client::ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        netcode_client_process_loopback_packet( m_client, packetData, packetBytes, packetSequence );
    }

    void Client::CreateClient( const Address & address )
    {
        DestroyClient();
        char addressString[MaxAddressLength];
        address.ToString( addressString, MaxAddressLength );

        struct netcode_client_config_t netcodeConfig;
        netcode_default_client_config(&netcodeConfig);
        netcodeConfig.allocator_context             = &GetClientAllocator();
        netcodeConfig.allocate_function             = StaticAllocateFunction;
        netcodeConfig.free_function                 = StaticFreeFunction;
        netcodeConfig.callback_context              = this;
        netcodeConfig.state_change_callback         = StaticStateChangeCallbackFunction;
        netcodeConfig.send_loopback_packet_callback = StaticSendLoopbackPacketCallbackFunction;
        m_client = netcode_client_create(addressString, &netcodeConfig, GetTime());
        
        if ( m_client )
        {
            m_boundAddress.SetPort( netcode_client_get_port( m_client ) );
        }
    }

    void Client::DestroyClient()
    {
        if ( m_client )
        {
            m_boundAddress = m_address;
            netcode_client_destroy( m_client );
            m_client = NULL;
        }
    }

    void Client::StateChangeCallbackFunction( int previous, int current )
    {
        (void) previous;
        (void) current;
    }

    void Client::StaticStateChangeCallbackFunction( void * context, int previous, int current )
    {
        Client * client = (Client*) context;
        client->StateChangeCallbackFunction( previous, current );
    }

    void Client::TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) packetSequence;
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator && networkSimulator->IsActive() )
        {
            networkSimulator->SendPacket( 0, packetData, packetBytes );
        }
        else
        {
            netcode_client_send_packet( m_client, packetData, packetBytes );
        }
    }

    int Client::ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        return (int) GetConnection().ProcessPacket( GetContext(), packetSequence, packetData, packetBytes );
    }

    void Client::SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        GetAdapter().ClientSendLoopbackPacket( clientIndex, packetData, packetBytes, packetSequence );
    }

    void Client::StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        Client * client = (Client*) context;
        client->SendLoopbackPacketCallbackFunction( clientIndex, packetData, packetBytes, packetSequence );
    }
}

// ---------------------------------------------------------------------------------

namespace yojimbo
{
    BaseServer::BaseServer( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time ) : m_config( config )
    {
        m_allocator = &allocator;
        m_adapter = &adapter;
        m_context = NULL;
        m_time = time;
        m_running = false;
        m_maxClients = 0;
        m_globalMemory = NULL;
        m_globalAllocator = NULL;
        for ( int i = 0; i < MaxClients; ++i )
        {
            m_clientMemory[i] = NULL;
            m_clientAllocator[i] = NULL;
            m_clientMessageFactory[i] = NULL;
            m_clientConnection[i] = NULL;
            m_clientEndpoint[i] = NULL;
        }
        m_networkSimulator = NULL;
        m_packetBuffer = NULL;
    }

    BaseServer::~BaseServer()
    {
        // IMPORTANT: Please stop the server before destroying it!
        yojimbo_assert( !IsRunning () );
        m_allocator = NULL;
    }

    void BaseServer::SetContext( void * context )
    {
        yojimbo_assert( !IsRunning() );
        m_context = context;
    }

    void BaseServer::Start( int maxClients )
    {
        Stop();
        m_running = true;
        m_maxClients = maxClients;
        yojimbo_assert( !m_globalMemory );
        yojimbo_assert( !m_globalAllocator );
        m_globalMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverGlobalMemory );
        m_globalAllocator = m_adapter->CreateAllocator( *m_allocator, m_globalMemory, m_config.serverGlobalMemory );
        yojimbo_assert( m_globalAllocator );
        if ( m_config.networkSimulator )
        {
            m_networkSimulator = YOJIMBO_NEW( *m_globalAllocator, NetworkSimulator, *m_globalAllocator, m_config.maxSimulatorPackets, m_time );
        }
        for ( int i = 0; i < m_maxClients; ++i )
        {
            yojimbo_assert( !m_clientMemory[i] );
            yojimbo_assert( !m_clientAllocator[i] );
            
            m_clientMemory[i] = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverPerClientMemory );
            m_clientAllocator[i] = m_adapter->CreateAllocator( *m_allocator, m_clientMemory[i], m_config.serverPerClientMemory );
            yojimbo_assert( m_clientAllocator[i] );
            
            m_clientMessageFactory[i] = m_adapter->CreateMessageFactory( *m_clientAllocator[i] );
            yojimbo_assert( m_clientMessageFactory[i] );
            
            m_clientConnection[i] = YOJIMBO_NEW( *m_clientAllocator[i], Connection, *m_clientAllocator[i], *m_clientMessageFactory[i], m_config, m_time );
            yojimbo_assert( m_clientConnection[i] );

            reliable_config_t reliable_config;
            reliable_default_config( &reliable_config );
            strcpy( reliable_config.name, "server endpoint" );
            reliable_config.context = (void*) this;
            reliable_config.index = i;
            reliable_config.max_packet_size = m_config.maxPacketSize;
            reliable_config.fragment_above = m_config.fragmentPacketsAbove;
            reliable_config.max_fragments = m_config.maxPacketFragments;
            reliable_config.fragment_size = m_config.packetFragmentSize; 
            reliable_config.ack_buffer_size = m_config.ackedPacketsBufferSize;
            reliable_config.received_packets_buffer_size = m_config.receivedPacketsBufferSize;
            reliable_config.fragment_reassembly_buffer_size = m_config.packetReassemblyBufferSize;
            reliable_config.rtt_smoothing_factor = m_config.rttSmoothingFactor;
            reliable_config.transmit_packet_function = BaseServer::StaticTransmitPacketFunction;
            reliable_config.process_packet_function = BaseServer::StaticProcessPacketFunction;
            reliable_config.allocator_context = &GetGlobalAllocator();
            reliable_config.allocate_function = BaseServer::StaticAllocateFunction;
            reliable_config.free_function = BaseServer::StaticFreeFunction;
            m_clientEndpoint[i] = reliable_endpoint_create( &reliable_config, m_time );
            reliable_endpoint_reset( m_clientEndpoint[i] );
        }
        m_packetBuffer = (uint8_t*) YOJIMBO_ALLOCATE( *m_globalAllocator, m_config.maxPacketSize );
    }

    void BaseServer::Stop()
    {
        if ( IsRunning() )
        {
            YOJIMBO_FREE( *m_globalAllocator, m_packetBuffer );
            yojimbo_assert( m_globalMemory );
            yojimbo_assert( m_globalAllocator );
            YOJIMBO_DELETE( *m_globalAllocator, NetworkSimulator, m_networkSimulator );
            for ( int i = 0; i < m_maxClients; ++i )
            {
                yojimbo_assert( m_clientMemory[i] );
                yojimbo_assert( m_clientAllocator[i] );
                yojimbo_assert( m_clientMessageFactory[i] );
                yojimbo_assert( m_clientEndpoint[i] );
                reliable_endpoint_destroy( m_clientEndpoint[i] ); m_clientEndpoint[i] = NULL;
                YOJIMBO_DELETE( *m_clientAllocator[i], Connection, m_clientConnection[i] );
                YOJIMBO_DELETE( *m_clientAllocator[i], MessageFactory, m_clientMessageFactory[i] );
                YOJIMBO_DELETE( *m_allocator, Allocator, m_clientAllocator[i] );
                YOJIMBO_FREE( *m_allocator, m_clientMemory[i] );
            }
            YOJIMBO_DELETE( *m_allocator, Allocator, m_globalAllocator );
            YOJIMBO_FREE( *m_allocator, m_globalMemory );
        }
        m_running = false;
        m_maxClients = 0;
        m_packetBuffer = NULL;
    }

    void BaseServer::AdvanceTime( double time )
    {
        m_time = time;
        if ( IsRunning() )
        {
            for ( int i = 0; i < m_maxClients; ++i )
            {
                m_clientConnection[i]->AdvanceTime( time );
                if ( m_clientConnection[i]->GetErrorLevel() != CONNECTION_ERROR_NONE )
                {
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "client %d connection is in error state. disconnecting client\n", m_clientConnection[i]->GetErrorLevel() );
                    DisconnectClient( i );
                    continue;
                }
                reliable_endpoint_update( m_clientEndpoint[i], m_time );
                int numAcks;
                const uint16_t * acks = reliable_endpoint_get_acks( m_clientEndpoint[i], &numAcks );
                m_clientConnection[i]->ProcessAcks( acks, numAcks );
                reliable_endpoint_clear_acks( m_clientEndpoint[i] );
            }
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator )
            {
                networkSimulator->AdvanceTime( time );
            }        
        }
    }

    void BaseServer::SetLatency( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetLatency( milliseconds );
        }
    }

    void BaseServer::SetJitter( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetJitter( milliseconds );
        }
    }

    void BaseServer::SetPacketLoss( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetPacketLoss( percent );
        }
    }

    void BaseServer::SetDuplicates( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetDuplicates( percent );
        }
    }

    Message * BaseServer::CreateMessage( int clientIndex, int type )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientMessageFactory[clientIndex] );
        return m_clientMessageFactory[clientIndex]->CreateMessage( type );
    }

    uint8_t * BaseServer::AllocateBlock( int clientIndex, int bytes )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientAllocator[clientIndex] );
        return (uint8_t*) YOJIMBO_ALLOCATE( *m_clientAllocator[clientIndex], bytes );
    }

    void BaseServer::AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( message );
        yojimbo_assert( block );
        yojimbo_assert( bytes > 0 );
        yojimbo_assert( message->IsBlockMessage() );
        BlockMessage * blockMessage = (BlockMessage*) message;
        blockMessage->AttachBlock( *m_clientAllocator[clientIndex], block, bytes );
    }

    void BaseServer::FreeBlock( int clientIndex, uint8_t * block )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        YOJIMBO_FREE( *m_clientAllocator[clientIndex], block );
    }

    bool BaseServer::CanSendMessage( int clientIndex, int channelIndex ) const
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->CanSendMessage( channelIndex );
    }

    bool BaseServer::HasMessagesToSend( int clientIndex, int channelIndex ) const
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->HasMessagesToSend( channelIndex );
    }

    void BaseServer::SendMessage( int clientIndex, int channelIndex, Message * message )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->SendMessage( channelIndex, message, GetContext() );
    }

    Message * BaseServer::ReceiveMessage( int clientIndex, int channelIndex )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->ReceiveMessage( channelIndex );
    }

    void BaseServer::ReleaseMessage( int clientIndex, Message * message )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        m_clientConnection[clientIndex]->ReleaseMessage( message );
    }

    void BaseServer::GetNetworkInfo( int clientIndex, NetworkInfo & info ) const
    {
        yojimbo_assert( IsRunning() );
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        memset( &info, 0, sizeof( info ) );
        if ( IsClientConnected( clientIndex ) )
        {
            yojimbo_assert( m_clientEndpoint[clientIndex] );
            const uint64_t * counters = reliable_endpoint_counters( m_clientEndpoint[clientIndex] );
            info.numPacketsSent = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT];
            info.numPacketsReceived = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED];
            info.numPacketsAcked = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED];
            info.RTT = reliable_endpoint_rtt( m_clientEndpoint[clientIndex] );
            info.packetLoss = reliable_endpoint_packet_loss( m_clientEndpoint[clientIndex] );
            reliable_endpoint_bandwidth( m_clientEndpoint[clientIndex], &info.sentBandwidth, &info.receivedBandwidth, &info.ackedBandwidth );
        }
    }

    MessageFactory & BaseServer::GetClientMessageFactory( int clientIndex ) 
    { 
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        return *m_clientMessageFactory[clientIndex];
    }

    reliable_endpoint_t * BaseServer::GetClientEndpoint( int clientIndex )
    {
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        return m_clientEndpoint[clientIndex];
    }

    Connection & BaseServer::GetClientConnection( int clientIndex )
    {
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return *m_clientConnection[clientIndex];
    }

    void BaseServer::StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        BaseServer * server = (BaseServer*) context;
        server->TransmitPacketFunction( index, packetSequence, packetData, packetBytes );
    }
    
    int BaseServer::StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        BaseServer * server = (BaseServer*) context;
        return server->ProcessPacketFunction( index, packetSequence, packetData, packetBytes );
    }

    void * BaseServer::StaticAllocateFunction( void * context, uint64_t bytes )
    {
        yojimbo_assert( context );
        Allocator * allocator = (Allocator*) context;
        return YOJIMBO_ALLOCATE( *allocator, bytes );
    }
    
    void BaseServer::StaticFreeFunction( void * context, void * pointer )
    {
        yojimbo_assert( context );
        yojimbo_assert( pointer );
        Allocator * allocator = (Allocator*) context;
        YOJIMBO_FREE( *allocator, pointer );
    }

    // -----------------------------------------------------------------------------------------------------

    Server::Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, Adapter & adapter, double time ) 
        : BaseServer( allocator, config, adapter, time )
    {
        yojimbo_assert( KeyBytes == NETCODE_KEY_BYTES );
        memcpy( m_privateKey, privateKey, NETCODE_KEY_BYTES );
        m_address = address;
        m_boundAddress = address;
        m_config = config;
        m_server = NULL;
    }

    Server::~Server()
    {
        // IMPORTANT: Please stop the server before destroying it!
        yojimbo_assert( !m_server );
    }

    void Server::Start( int maxClients )
    {
        if ( IsRunning() )
            Stop();
        
        BaseServer::Start( maxClients );
        
        char addressString[MaxAddressLength];
        m_address.ToString( addressString, MaxAddressLength );
        
        struct netcode_server_config_t netcodeConfig;
        netcode_default_server_config(&netcodeConfig);
        netcodeConfig.protocol_id = m_config.protocolId;
        memcpy(netcodeConfig.private_key, m_privateKey, NETCODE_KEY_BYTES);
        netcodeConfig.allocator_context = &GetGlobalAllocator();
        netcodeConfig.allocate_function = StaticAllocateFunction;
        netcodeConfig.free_function     = StaticFreeFunction;
        netcodeConfig.callback_context = this;
        netcodeConfig.connect_disconnect_callback = StaticConnectDisconnectCallbackFunction;
        netcodeConfig.send_loopback_packet_callback = StaticSendLoopbackPacketCallbackFunction;
        
        m_server = netcode_server_create(addressString, &netcodeConfig, GetTime());
        
        if ( !m_server )
        {
            Stop();
            return;
        }
        
        netcode_server_start( m_server, maxClients );

        m_boundAddress.SetPort( netcode_server_get_port( m_server ) );
    }

    void Server::Stop()
    {
        if ( m_server )
        {
            m_boundAddress = m_address;
            netcode_server_stop( m_server );
            netcode_server_destroy( m_server );
            m_server = NULL;
        }
        BaseServer::Stop();
    }

    void Server::DisconnectClient( int clientIndex )
    {
        yojimbo_assert( m_server );
        netcode_server_disconnect_client( m_server, clientIndex );
    }

    void Server::DisconnectAllClients()
    {
        yojimbo_assert( m_server );
        netcode_server_disconnect_all_clients( m_server );
    }

    void Server::SendPackets()
    {
        if ( m_server )
        {
            const int maxClients = GetMaxClients();
            for ( int i = 0; i < maxClients; ++i )
            {
                if ( IsClientConnected( i ) )
                {
                    uint8_t * packetData = GetPacketBuffer();
                    int packetBytes;
                    uint16_t packetSequence = reliable_endpoint_next_packet_sequence( GetClientEndpoint(i) );
                    if ( GetClientConnection(i).GeneratePacket( GetContext(), packetSequence, packetData, m_config.maxPacketSize, packetBytes ) )
                    {
                        reliable_endpoint_send_packet( GetClientEndpoint(i), packetData, packetBytes );
                    }
                }
            }
        }
    }

    void Server::ReceivePackets()
    {
        if ( m_server )
        {
            const int maxClients = GetMaxClients();
            for ( int clientIndex = 0; clientIndex < maxClients; ++clientIndex )
            {
                while ( true )
                {
                    int packetBytes;
                    uint64_t packetSequence;
                    uint8_t * packetData = netcode_server_receive_packet( m_server, clientIndex, &packetBytes, &packetSequence );
                    if ( !packetData )
                        break;
                    reliable_endpoint_receive_packet( GetClientEndpoint( clientIndex ), packetData, packetBytes );
                    netcode_server_free_packet( m_server, packetData );
                }
            }
        }
    }

    void Server::AdvanceTime( double time )
    {
        if ( m_server )
        {
            netcode_server_update( m_server, time );
        }
        BaseServer::AdvanceTime( time );
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator && networkSimulator->IsActive() )
        {
            uint8_t ** packetData = (uint8_t**) alloca( sizeof( uint8_t*) * m_config.maxSimulatorPackets );
            int * packetBytes = (int*) alloca( sizeof(int) * m_config.maxSimulatorPackets );
            int * to = (int*) alloca( sizeof(int) * m_config.maxSimulatorPackets );
            int numPackets = networkSimulator->ReceivePackets( m_config.maxSimulatorPackets, packetData, packetBytes, to );
            for ( int i = 0; i < numPackets; ++i )
            {
                netcode_server_send_packet( m_server, to[i], (uint8_t*) packetData[i], packetBytes[i] );
                YOJIMBO_FREE( networkSimulator->GetAllocator(), packetData[i] );
            }
        }
    }

    bool Server::IsClientConnected( int clientIndex ) const
    {
        return netcode_server_client_connected( m_server, clientIndex ) != 0;
    }

    uint64_t Server::GetClientId( int clientIndex ) const
    {
        return netcode_server_client_id( m_server, clientIndex );
    }

    netcode_address_t * Server::GetClientAddress( int clientIndex ) const
    {
        return netcode_server_client_address( m_server, clientIndex );
    }

    int Server::GetNumConnectedClients() const
    {
        return netcode_server_num_connected_clients( m_server );
    }

    void Server::ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData )
    {
        netcode_server_connect_loopback_client( m_server, clientIndex, clientId, userData );
    }

    void Server::DisconnectLoopbackClient( int clientIndex )
    {
        netcode_server_disconnect_loopback_client( m_server, clientIndex );
    }

    bool Server::IsLoopbackClient( int clientIndex ) const
    {
        return netcode_server_client_loopback( m_server, clientIndex ) != 0;
    }

    void Server::ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        netcode_server_process_loopback_packet( m_server, clientIndex, packetData, packetBytes, packetSequence );
    }

    void Server::TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) packetSequence;
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator && networkSimulator->IsActive() )
        {
            networkSimulator->SendPacket( clientIndex, packetData, packetBytes );
        }
        else
        {
            netcode_server_send_packet( m_server, clientIndex, packetData, packetBytes );
        }
    }

    int Server::ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        return (int) GetClientConnection(clientIndex).ProcessPacket( GetContext(), packetSequence, packetData, packetBytes );
    }

    void Server::ConnectDisconnectCallbackFunction( int clientIndex, int connected )
    {
        if ( connected == 0 )
        {
            GetAdapter().OnServerClientDisconnected( clientIndex );
            reliable_endpoint_reset( GetClientEndpoint( clientIndex ) );
            GetClientConnection( clientIndex ).Reset();
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator && networkSimulator->IsActive() )
            {
                networkSimulator->DiscardClientPackets( clientIndex );
            }
        }
        else
        {
            GetAdapter().OnServerClientConnected( clientIndex );
        }
    }

    void Server::SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        GetAdapter().ServerSendLoopbackPacket( clientIndex, packetData, packetBytes, packetSequence );
    }

    void Server::StaticConnectDisconnectCallbackFunction( void * context, int clientIndex, int connected )
    {
        Server * server = (Server*) context;
        server->ConnectDisconnectCallbackFunction( clientIndex, connected );
    }

    void Server::StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        Server * server = (Server*) context;
        server->SendLoopbackPacketCallbackFunction( clientIndex, packetData, packetBytes, packetSequence );
    }
}

// ---------------------------------------------------------------------------------
