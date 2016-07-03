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
    Connection::Connection( Allocator & allocator, PacketFactory & packetFactory, MessageFactory & messageFactory, const ConnectionConfig & config ) : m_config( config )
    {
        m_allocator = &allocator;

        m_packetFactory = &packetFactory;

        m_messageFactory = &messageFactory;
        
        m_error = CONNECTION_ERROR_NONE;

        m_sentPackets = YOJIMBO_NEW( *m_allocator, ConnectionSentPackets, *m_allocator, m_config.slidingWindowSize );
        
        m_receivedPackets = YOJIMBO_NEW( *m_allocator, ConnectionReceivedPackets, *m_allocator, m_config.slidingWindowSize );

        m_messageSendQueue = YOJIMBO_NEW( *m_allocator, SequenceBuffer<MessageSendQueueEntry>, *m_allocator, m_config.messageSendQueueSize );
        
        m_messageSentPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<MessageSentPacketEntry>, *m_allocator, m_config.messageSentPacketsSize );
        
        m_messageReceiveQueue = YOJIMBO_NEW( *m_allocator, SequenceBuffer<MessageReceiveQueueEntry>, *m_allocator, m_config.messageReceiveQueueSize );

        const int maxMessageType = m_messageFactory->GetNumTypes() - 1;

        const int MessageIdBits = 16;
        
        const int MessageTypeBits = bits_required( 0, maxMessageType );

        m_messageOverheadBits = MessageIdBits + MessageTypeBits;
        
        m_sentPacketMessageIds = YOJIMBO_NEW_ARRAY( *m_allocator, uint16_t, m_config.maxMessagesPerPacket * m_config.messageSendQueueSize );

        Reset();
    }

    Connection::~Connection()
    {
        Reset();

        assert( m_sentPackets );
        assert( m_receivedPackets );

        YOJIMBO_DELETE( *m_allocator, ConnectionSentPackets, m_sentPackets );
        YOJIMBO_DELETE( *m_allocator, ConnectionReceivedPackets, m_receivedPackets );

        assert( m_messageSendQueue );
        assert( m_messageSentPackets );
        assert( m_messageReceiveQueue );
        assert( m_sentPacketMessageIds );

        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<MessageSendQueueEntry>, m_messageSendQueue );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<MessageSentPacketEntry>, m_messageSentPackets );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<MessageReceiveQueueEntry>, m_messageReceiveQueue );
        YOJIMBO_DELETE_ARRAY( *m_allocator, m_sentPacketMessageIds, m_config.maxMessagesPerPacket * m_config.messageSendQueueSize );
    }

    void Connection::Reset()
    {
        m_error = CONNECTION_ERROR_NONE;

        m_time = 0.0;

        m_sentPackets->Reset();
        m_receivedPackets->Reset();

        memset( m_counters, 0, sizeof( m_counters ) );
    }

    bool Connection::CanSendMessage() const
    {
        assert( m_messageSendQueue );

        if ( GetError() != CONNECTION_ERROR_NONE )
            return true;

        return m_messageSendQueue->IsAvailable( m_sendMessageId );
    }

    void Connection::SendMessage( Message * message )
    {
        assert( message );
        assert( CanSendMessage() );

        if ( GetError() != CONNECTION_ERROR_NONE )
        {
            m_messageFactory->Release( message );
            return;
        }

        if ( !CanSendMessage() )
        {
            m_error = CONNECTION_ERROR_MESSAGE_SEND_QUEUE_FULL;
            m_messageFactory->Release( message );
            return;
        }

        message->SetId( m_sendMessageId );

        MessageSendQueueEntry * entry = m_messageSendQueue->Insert( m_sendMessageId );

        assert( entry );

        const int blockMessage = message->IsBlockMessage();
        
        entry->message = message;
        entry->blockMessage = blockMessage;
        entry->measuredBits = 0;
        entry->timeLastSent = -1.0;

        if ( !blockMessage )
        {
            MeasureStream measureStream( m_config.maxSerializedMessageSize * 2 );
            message->SerializeMeasure( measureStream );
            if ( measureStream.GetError() )
            {
                m_error = CONNECTION_ERROR_MESSAGE_SERIALIZE_MEASURE_FAILED;
                m_messageFactory->Release( message );
                return;
            }

            // todo: how are we going to handle context here? context need to be the same for all connected clients as it is in the network interface right now.                        
            //measureStream.SetContext( GetContext() );

            entry->measuredBits = measureStream.GetBitsProcessed() + m_messageOverheadBits;

            printf( "message %d estimates at %d bits\n", (int) m_sendMessageId, entry->measuredBits );
        }

        m_counters[CONNECTION_COUNTER_MESSAGES_SENT]++;

        m_sendMessageId++;
    }

    Message * ReceiveMessage()
    {
        // todo
        return NULL;
    }

    ConnectionPacket * Connection::WritePacket()
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return NULL;

        ConnectionPacket * packet = (ConnectionPacket*) m_packetFactory->CreatePacket( m_config.packetType );

        if ( !packet )
            return NULL;

        packet->sequence = m_sentPackets->GetSequence();

        GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

        ConnectionSentPacketData * entry = m_sentPackets->Insert( packet->sequence );
        assert( entry );
        entry->acked = 0;

        m_counters[CONNECTION_COUNTER_PACKETS_WRITTEN]++;

        return packet;
    }

    bool Connection::ReadPacket( ConnectionPacket * packet )
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return false;

        assert( packet );
        assert( packet->GetType() == m_config.packetType );

        ProcessAcks( packet->ack, packet->ack_bits );

        m_counters[CONNECTION_COUNTER_PACKETS_READ]++;

        bool discardPacket = false;

        if ( discardPacket || !m_receivedPackets->Insert( packet->sequence ) )
        {
            m_counters[CONNECTION_COUNTER_PACKETS_DISCARDED]++;
            return false;            
        }

        return true;
    }

    void Connection::AdvanceTime( double time )
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return;

        m_time = time;
    }

    double Connection::GetTime() const
    {
        return m_time;
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

        m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;
    }
}
