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
    template <typename Stream> bool ConnectionPacket::Serialize( Stream & stream )
    {
        ConnectionContext * context = (ConnectionContext*) stream.GetContext();
        if ( !context )
            return false;

        // serialize ack system

        serialize_check( stream, "ack system (begin)" );

        bool perfect;
        if ( Stream::IsWriting )
             perfect = ack_bits == 0xFFFFFFFF;

        serialize_bool( stream, perfect );

        if ( !perfect )
            serialize_bits( stream, ack_bits, 32 );
        else
            ack_bits = 0xFFFFFFFF;

        serialize_bits( stream, sequence, 16 );

        // todo: messy. move this off into a separate function: serialize_ack

        int ack_delta = 0;
        bool ack_in_range = false;

        if ( Stream::IsWriting )
        {
            if ( ack < sequence )
                ack_delta = sequence - ack;
            else
                ack_delta = (int)sequence + 65536 - ack;

            assert( ack_delta > 0 );
            assert( sequence - ack_delta == ack );
            
            ack_in_range = ack_delta <= 64;
        }

        serialize_bool( stream, ack_in_range );

        if ( ack_in_range )
        {
            serialize_int( stream, ack_delta, 1, 64 );
            if ( Stream::IsReading )
                ack = sequence - ack_delta;
        }
        else
        {
            serialize_bits( stream, ack, 16 );
        }

        serialize_align( stream );

        serialize_check( stream, "ack system (end)" );

        // serialize messages

        bool hasMessages = numMessages != 0;

        serialize_bool( stream, hasMessages );

        if ( hasMessages )
        {
            serialize_check( stream, "message system (begin)" );

            serialize_int( stream, numMessages, 1, MaxMessagesPerPacket );

            int messageTypes[MaxMessagesPerPacket];

            uint16_t messageIds[MaxMessagesPerPacket];

            if ( Stream::IsWriting )
            {
                for ( int i = 0; i < (int) numMessages; ++i )
                {
                    assert( messages[i] );
                    messageTypes[i] = messages[i]->GetType();
                    messageIds[i] = messages[i]->GetId();
                }
            }

            serialize_bits( stream, messageIds[0], 16 );

            for ( int i = 1; i < (int) numMessages; ++i )
            {
                // todo: replace this with a nice serialize sequence relative that handles this wrap around
                if ( Stream::IsWriting )
                {
                    uint32_t a = messageIds[i-1];
                    uint32_t b = messageIds[i] + ( messageIds[i-1] > messageIds[i] ? 65536 : 0 );
                    serialize_int_relative( stream, a, b );
                }
                else
                {
                    uint32_t a = messageIds[i-1];
                    uint32_t b;
                    serialize_int_relative( stream, a, b );
                    if ( b >= 65536 )
                        b -= 65536;
                    messageIds[i] = uint16_t( b );
                }
            }

            for ( int i = 0; i < (int) numMessages; ++i )
            {
                const int maxMessageType = context->messageFactory->GetNumTypes() - 1;

                serialize_int( stream, messageTypes[i], 0, maxMessageType );

                if ( Stream::IsReading )
                {
                    messages[i] = context->messageFactory->Create( messageTypes[i] );

                    assert( messages[i] );
                    assert( messages[i]->GetType() == messageTypes[i] );

                    messages[i]->AssignId( messageIds[i] );

                    /*
                    if ( Stream::IsReading && messageTypes[i] == BlockMessageType )
                    {
                        CORE_ASSERT( config.smallBlockAllocator );
                        BlockMessage * blockMessage = static_cast<BlockMessage*>( messages[i] );
                        blockMessage->SetAllocator( *config.smallBlockAllocator );
                    }
                    */
                }

                assert( messages[i] );

                if ( !messages[i]->SerializeInternal( stream ) )
                    return false;
            }

            serialize_check( stream, "message system (end)" );
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

    bool ConnectionPacket::operator ==( const ConnectionPacket & other ) const
    {
        return sequence == other.sequence &&
                    ack == other.ack &&
               ack_bits == other.ack_bits;
    }

    bool ConnectionPacket::operator !=( const ConnectionPacket & other ) const
    {
        return !( *this == other );
    }

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
        
        m_sentPacketMessageIds = YOJIMBO_NEW_ARRAY( *m_allocator, uint16_t, MaxMessagesPerPacket * m_config.messageSendQueueSize );

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
        YOJIMBO_DELETE_ARRAY( *m_allocator, m_sentPacketMessageIds, MaxMessagesPerPacket * m_config.messageSendQueueSize );
    }

    void Connection::Reset()
    {
        m_error = CONNECTION_ERROR_NONE;

        m_time = 0.0;

        m_sentPackets->Reset();
        m_receivedPackets->Reset();

        m_sendMessageId = 0;
        m_receiveMessageId = 0;
        m_oldestUnackedMessageId = 0;

        for ( int i = 0; i < m_messageSendQueue->GetSize(); ++i )
        {
            MessageSendQueueEntry * entry = m_messageSendQueue->GetAtIndex( i );
            if ( entry && entry->message )
                m_messageFactory->Release( entry->message );
        }

        for ( int i = 0; i < m_messageReceiveQueue->GetSize(); ++i )
        {
            MessageReceiveQueueEntry * entry = m_messageReceiveQueue->GetAtIndex( i );
            if ( entry && entry->message )
                m_messageFactory->Release( entry->message );
        }

        m_messageSendQueue->Reset();
        m_messageSentPackets->Reset();
        m_messageReceiveQueue->Reset();

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

        message->AssignId( m_sendMessageId );

        MessageSendQueueEntry * entry = m_messageSendQueue->Insert( m_sendMessageId );

        assert( entry );

        const int blockMessage = message->IsBlockMessage();
        
        entry->message = message;
        entry->blockMessage = blockMessage;
        entry->measuredBits = 0;
        entry->timeLastSent = -1.0;

        if ( !blockMessage )
        {
            // todo: context

            MeasureStream measureStream( m_config.maxSerializedMessageSize * 2 );
            message->SerializeInternal( measureStream );
            if ( measureStream.GetError() )
            {
                m_error = CONNECTION_ERROR_MESSAGE_SERIALIZE_MEASURE_FAILED;
                m_messageFactory->Release( message );
                return;
            }

            entry->measuredBits = measureStream.GetBitsProcessed() + m_messageOverheadBits;

            printf( "message %d estimates at %d bits\n", (int) m_sendMessageId, entry->measuredBits );
        }

        m_counters[CONNECTION_COUNTER_MESSAGES_SENT]++;

        m_sendMessageId++;
    }

    Message * Connection::ReceiveMessage()
    {
        if ( GetError() != CONNECTION_ERROR_NONE )
            return NULL;

        MessageReceiveQueueEntry * entry = m_messageReceiveQueue->Find( m_receiveMessageId );
        if ( !entry )
            return NULL;

        Message * message = entry->message;

        assert( message );
        assert( message->GetId() == m_receiveMessageId );

        m_messageReceiveQueue->Remove( m_receiveMessageId );

        m_counters[CONNECTION_COUNTER_MESSAGES_RECEIVED]++;

        m_receiveMessageId++;

        return message;
    }

    ConnectionPacket * Connection::WritePacket()
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return NULL;

        ConnectionPacket * packet = (ConnectionPacket*) m_packetFactory->CreatePacket( m_config.packetType );

        if ( !packet )
            return NULL;

        // ack system

        packet->sequence = m_sentPackets->GetSequence();

        GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

        InsertAckPacketEntry( packet->sequence );

        // message system

        int numMessageIds;
        
        uint16_t messageIds[MaxMessagesPerPacket];

        GetMessagesToSend( messageIds, numMessageIds );

        AddMessagePacketEntry( messageIds, numMessageIds, packet->sequence );

        packet->numMessages = numMessageIds;

        for ( int i = 0; i < numMessageIds; ++i )
        {
            MessageSendQueueEntry * entry = m_messageSendQueue->Find( messageIds[i] );
            assert( entry && entry->message );
            packet->messages[i] = entry->message;
            m_messageFactory->AddRef( entry->message );
        }

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

        m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;
    }

    void Connection::GetMessagesToSend( uint16_t * messageIds, int & numMessageIds )
    {
        numMessageIds = 0;

        MessageSendQueueEntry * firstEntry = m_messageSendQueue->Find( m_oldestUnackedMessageId );

        if ( !firstEntry )
            return;

        int availableBits = 256 * 8; // todo - m_config.packetBudget * 8;
        
        for ( int i = 0; i < m_config.messageSendQueueSize; ++i )
        {
            if ( availableBits < 16 * 8 ) // todo - m_config.messageGiveUpBits )
                break;
            
            const uint16_t messageId = m_oldestUnackedMessageId + i;

            MessageSendQueueEntry * entry = m_messageSendQueue->Find( messageId );
            
            if ( !entry )
                break;

            if ( entry->blockMessage )
                break;

            if ( entry->timeLastSent + m_config.messageResendRate <= m_time && availableBits - entry->measuredBits >= 0 )
            {
                messageIds[numMessageIds++] = messageId;
                entry->timeLastSent = m_time;
                availableBits -= entry->measuredBits;
            }

            if ( numMessageIds == MaxMessagesPerPacket )
                break;
        }
    }

    void Connection::AddMessagePacketEntry( const uint16_t * messageIds, int & numMessageIds, uint16_t sequence )
    {
        MessageSentPacketEntry * sentPacket = m_messageSentPackets->Insert( sequence );
        
        assert( sentPacket );

        sentPacket->acked = 0;
        sentPacket->blockMessage = 0;
        sentPacket->timeSent = m_time;
        const int sentPacketIndex = m_sentPackets->GetIndex( sequence );
        sentPacket->messageIds = &m_sentPacketMessageIds[sentPacketIndex*MaxMessagesPerPacket];
        sentPacket->numMessageIds = numMessageIds;
        for ( int i = 0; i < numMessageIds; ++i )
            sentPacket->messageIds[i] = messageIds[i];
    }
}
