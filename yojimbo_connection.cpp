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

        /*
        assert( config.maxSmallBlockSize <= MaxSmallBlockSize );

        m_maxBlockFragments = (int) ceil( m_config.maxLargeBlockSize / (float) m_config.blockFragmentSize );

        m_sendLargeBlock.time_fragment_last_sent = CORE_NEW_ARRAY( *m_allocator, double, m_maxBlockFragments );
        m_sendLargeBlock.acked_fragment = CORE_NEW( *m_allocator, BitArray, *m_allocator, m_maxBlockFragments );
        m_receiveLargeBlock.received_fragment = CORE_NEW( *m_allocator, BitArray, *m_allocator, m_maxBlockFragments );
        */

        Reset();
    }

    Connection::~Connection()
    {
        assert( m_sentPackets );
        assert( m_receivedPackets );

        YOJIMBO_DELETE( *m_allocator, ConnectionSentPackets, m_sentPackets );
        YOJIMBO_DELETE( *m_allocator, ConnectionReceivedPackets, m_receivedPackets );

        m_sentPackets = nullptr;
        m_receivedPackets = nullptr;
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
        // todo
        return true;
    }

    void Connection::SendMessage( Message * message )
    {
        (void)message;
        // todo
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
