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
    ConnectionPacket::~ConnectionPacket()
    {
        if ( messageFactory )
        {
            for ( int i = 0; i < numMessages; ++i )
            {
                assert( messages[i] );
                messageFactory->Release( messages[i] );
            }

            Allocator & allocator = messageFactory->GetAllocator();
            allocator.Free( messages );
            messages = NULL;

            if ( blockMessage )
            {
                messageFactory->Release( blockMessage );
                blockMessage = NULL;
            }

            if ( blockFragmentData )
            {
                allocator.Free( blockFragmentData );
                blockFragmentData = NULL;
            }
        }
        else
        {
            assert( messages == NULL );
            assert( blockMessage == NULL );
            assert( blockFragmentData == NULL );
        }
    }

    template <typename Stream> bool ConnectionPacket::Serialize( Stream & stream )
    {
        ConnectionContext * context = (ConnectionContext*) stream.GetContext();

        assert( context );

        messageFactory = context->messageFactory;

        const int maxMessageType = messageFactory->GetNumTypes() - 1;

        // ack system

        bool perfect_acks = Stream::IsWriting ? ( ack_bits == 0xFFFFFFFF ) : 0;

        serialize_bool( stream, perfect_acks );

        if ( !perfect_acks )
            serialize_bits( stream, ack_bits, 32 );
        else
            ack_bits = 0xFFFFFFFF;

        serialize_bits( stream, sequence, 16 );

        serialize_ack_relative( stream, sequence, ack );

        serialize_align( stream );

        // serialize messages

        bool hasMessages = numMessages != 0;

        serialize_bool( stream, hasMessages );

        if ( hasMessages )
        {
            serialize_int( stream, numMessages, 1, context->connectionConfig->maxMessagesPerPacket );

            int * messageTypes = (int*) alloca( sizeof( int ) * numMessages );

            uint16_t * messageIds = (uint16_t*) alloca( sizeof( uint16_t ) * numMessages );

            if ( Stream::IsWriting )
            {
                assert( messages );

                for ( int i = 0; i < numMessages; ++i )
                {
                    assert( messages[i] );
                    messageTypes[i] = messages[i]->GetType();
                    messageIds[i] = messages[i]->GetId();
                }
            }
            else
            {
                Allocator & allocator = context->messageFactory->GetAllocator();

                messages = (Message**) allocator.Allocate( sizeof( Message*) * numMessages );
            }

            serialize_bits( stream, messageIds[0], 16 );

            for ( int i = 1; i < numMessages; ++i )
            {
                serialize_sequence_relative( stream, messageIds[i-1], messageIds[i] );
            }

            if ( Stream::IsReading )
            {
                messageFactory = context->messageFactory;
            }

            for ( int i = 0; i < numMessages; ++i )
            {
                if ( maxMessageType > 0 )
                {
                    serialize_int( stream, messageTypes[i], 0, maxMessageType );
                }
                else
                {
                    messageTypes[i] = 0;
                }

                if ( Stream::IsReading )
                {
                    messages[i] = context->messageFactory->Create( messageTypes[i] );

                    if ( !messages[i] )
                        return false;

                    messages[i]->AssignId( messageIds[i] );
                }

                assert( messages[i] );

                if ( !messages[i]->SerializeInternal( stream ) )
                    return false;
            }

            serialize_check( stream, "messages" );
        }

        // block message + fragment

        bool hasFragment = Stream::IsWriting && blockFragmentData;

        serialize_bool( stream, hasFragment );

        if ( hasFragment )
        {
            serialize_bits( stream, blockMessageId, 16 );

            serialize_int( stream, blockNumFragments, 1, context->connectionConfig->GetMaxFragmentsPerBlock() );

            if ( blockNumFragments > 1 )
            {
                serialize_int( stream, blockFragmentId, 0, blockNumFragments - 1 );
            }
            else
            {
                blockFragmentId = 0;
            }

            serialize_int( stream, blockFragmentSize, 1, context->connectionConfig->fragmentSize );

            if ( Stream::IsReading )
            {
                blockFragmentData = (uint8_t*) messageFactory->GetAllocator().Allocate( blockFragmentSize );

                if ( !blockFragmentData )
                    return false;
            }

            serialize_bytes( stream, blockFragmentData, blockFragmentSize );

            if ( blockFragmentId == 0 )
            {
                // block message

                serialize_int( stream, blockMessageType, 0, maxMessageType );

                if ( Stream::IsReading )
                {
                    Message * message = context->messageFactory->Create( blockMessageType );

                    if ( !message || !message->IsBlockMessage() )
                        return false;

                    blockMessage = (BlockMessage*) message;
                }

                assert( blockMessage );

                if ( !blockMessage->SerializeInternal( stream ) )
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
        assert( ( 65536 % config.messageSendQueueSize ) == 0 );
        assert( ( 65536 % config.messageReceiveQueueSize ) == 0 );

        m_allocator = &allocator;

        m_packetFactory = &packetFactory;

        m_messageFactory = &messageFactory;

        m_listener = NULL;
        
        m_error = CONNECTION_ERROR_NONE;

        m_clientIndex = 0;

        m_messageOverheadBits = CalculateMessageOverheadBits();

        m_sentPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionSentPacketData>, *m_allocator, m_config.slidingWindowSize );
        
        m_receivedPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionReceivedPacketData>, *m_allocator, m_config.slidingWindowSize );

        m_messageSendQueue = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionMessageSendQueueEntry>, *m_allocator, m_config.messageSendQueueSize );
        
        m_messageSentPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionMessageSentPacketEntry>, *m_allocator, m_config.slidingWindowSize );
        
        m_messageReceiveQueue = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionMessageReceiveQueueEntry>, *m_allocator, m_config.messageReceiveQueueSize );
        
        m_sentPacketMessageIds = (uint16_t*) m_allocator->Allocate( sizeof( uint16_t ) * m_config.maxMessagesPerPacket * m_config.messageSendQueueSize );

        m_sendBlock.Allocate( *m_allocator, m_config.maxBlockSize, m_config.GetMaxFragmentsPerBlock() );
        
        m_receiveBlock.Allocate( *m_allocator, m_config.maxBlockSize, m_config.GetMaxFragmentsPerBlock() );

        Reset();
    }

    Connection::~Connection()
    {
        Reset();

        assert( m_sentPackets );
        assert( m_receivedPackets );

        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionSentPacketData>, m_sentPackets );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionReceivedPacketData>, m_receivedPackets );

        assert( m_messageSendQueue );
        assert( m_messageSentPackets );
        assert( m_messageReceiveQueue );
        assert( m_sentPacketMessageIds );

        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionMessageSendQueueEntry>, m_messageSendQueue );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionMessageSentPacketEntry>, m_messageSentPackets );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionMessageReceiveQueueEntry>, m_messageReceiveQueue );
        
		m_allocator->Free( m_sentPacketMessageIds );	m_sentPacketMessageIds = NULL;

        m_sendBlock.Free( *m_allocator );

        m_receiveBlock.Free( *m_allocator );
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
            ConnectionMessageSendQueueEntry * entry = m_messageSendQueue->GetAtIndex( i );
            if ( entry && entry->message )
                m_messageFactory->Release( entry->message );
        }

        for ( int i = 0; i < m_messageReceiveQueue->GetSize(); ++i )
        {
            ConnectionMessageReceiveQueueEntry * entry = m_messageReceiveQueue->GetAtIndex( i );
            if ( entry && entry->message )
                m_messageFactory->Release( entry->message );
        }

        m_messageSendQueue->Reset();
        m_messageSentPackets->Reset();
        m_messageReceiveQueue->Reset();

        m_sendBlock.Reset();

        m_receiveBlock.Reset();

        if ( m_receiveBlock.blockMessage )
        {
            m_messageFactory->Release( m_receiveBlock.blockMessage );
            m_receiveBlock.blockMessage = NULL;
        }

        memset( m_counters, 0, sizeof( m_counters ) );
    }

    bool Connection::CanSendMessage() const
    {
        assert( m_messageSendQueue );

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

        ConnectionMessageSendQueueEntry * entry = m_messageSendQueue->Insert( m_sendMessageId );

        assert( entry );

        entry->block = message->IsBlockMessage();
        entry->message = message;
        entry->measuredBits = 0;
        entry->timeLastSent = -1.0;

        if ( message->IsBlockMessage() )
        {
            assert( ((BlockMessage*)message)->GetBlockSize() > 0 );
            assert( ((BlockMessage*)message)->GetBlockSize() <= m_config.maxBlockSize );
        }

        MeasureStream measureStream( m_config.messagePacketBudget / 2 );

        message->SerializeInternal( measureStream );

        if ( measureStream.GetError() )
        {
            m_error = CONNECTION_ERROR_MESSAGE_SERIALIZE_MEASURE_FAILED;
            m_messageFactory->Release( message );
            return;
        }

        entry->measuredBits = measureStream.GetBitsProcessed() + m_messageOverheadBits;

        m_counters[CONNECTION_COUNTER_MESSAGES_SENT]++;

        m_sendMessageId++;
    }

    Message * Connection::ReceiveMessage()
    {
        if ( GetError() != CONNECTION_ERROR_NONE )
            return NULL;

        ConnectionMessageReceiveQueueEntry * entry = m_messageReceiveQueue->Find( m_receiveMessageId );
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

        packet->sequence = m_sentPackets->GetSequence();

        GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

        InsertAckPacketEntry( packet->sequence );

        int numMessageIds = 0;
        
        uint16_t * messageIds = (uint16_t*) alloca( m_config.maxMessagesPerPacket * sizeof( uint16_t ) );

        if ( HasMessagesToSend() )
        {
            if ( SendingBlockMessage() )
            {
                uint16_t messageId;
                uint16_t fragmentId;
                int fragmentBytes;
                int numFragments;
                int messageType;

                uint8_t * fragmentData = GetFragmentToSend( messageId, fragmentId, fragmentBytes, numFragments, messageType );

                if ( fragmentData )
                {
                    AddFragmentToPacket( messageId, fragmentId, fragmentData, fragmentBytes, numFragments, messageType, packet );

                    AddFragmentPacketEntry( messageId, fragmentId, packet->sequence );
                }
            }
            else
            {
                GetMessagesToSend( messageIds, numMessageIds );

                AddMessagesToPacket( messageIds, numMessageIds, packet );

                AddMessagePacketEntry( messageIds, numMessageIds, packet->sequence );
            }
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

        m_counters[CONNECTION_COUNTER_PACKETS_READ]++;

        if ( m_listener )
            m_listener->OnConnectionPacketReceived( this, packet->sequence );

        if ( !m_receivedPackets->Insert( packet->sequence ) )
            return false;

        ProcessAcks( packet->ack, packet->ack_bits );

        ProcessPacketMessages( packet );

        ProcessPacketFragment( packet );

        return true;
    }

    void Connection::AdvanceTime( double time )
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return;

        m_time = time;

        m_sentPackets->RemoveOldEntries();
        m_receivedPackets->RemoveOldEntries();
        m_messageSentPackets->RemoveOldEntries();
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

        ProcessMessageAck( sequence );

        m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;
    }

    bool Connection::HasMessagesToSend() const
    {
        return m_oldestUnackedMessageId != m_sendMessageId;
    }

    void Connection::GetMessagesToSend( uint16_t * messageIds, int & numMessageIds )
    {
        assert( HasMessagesToSend() );

        numMessageIds = 0;

        const int GiveUpBits = 8 * 8;

        int availableBits = m_config.messagePacketBudget * 8;

        const int messageLimit = min( m_config.messageSendQueueSize, m_config.messageReceiveQueueSize ) / 2;

        for ( int i = 0; i < messageLimit; ++i )
        {
            const uint16_t messageId = m_oldestUnackedMessageId + i;

            ConnectionMessageSendQueueEntry * entry = m_messageSendQueue->Find( messageId );

            if ( !entry )
                continue;

            if ( entry->block )
                break;
            
            if ( entry->timeLastSent + m_config.messageResendRate <= m_time && availableBits >= entry->measuredBits )
            {
                messageIds[numMessageIds++] = messageId;
                entry->timeLastSent = m_time;
                availableBits -= entry->measuredBits;
            }

            if ( availableBits <= GiveUpBits )
                break;
            
            if ( numMessageIds == m_config.maxMessagesPerPacket )
                break;
        }
    }

    void Connection::AddMessagesToPacket( const uint16_t * messageIds, int numMessageIds, ConnectionPacket * packet )
    {
        assert( packet );
        assert( messageIds );

        if ( numMessageIds == 0 )
            return;

        packet->messageFactory = m_messageFactory;
        
        packet->numMessages = numMessageIds;
        
        packet->messages = (Message**) m_messageFactory->GetAllocator().Allocate( sizeof( Message* ) * numMessageIds );

        for ( int i = 0; i < numMessageIds; ++i )
        {
            ConnectionMessageSendQueueEntry * entry = m_messageSendQueue->Find( messageIds[i] );
            assert( entry && entry->message );
            packet->messages[i] = entry->message;
            m_messageFactory->AddRef( entry->message );
        }
    }

    void Connection::AddMessagePacketEntry( const uint16_t * messageIds, int numMessageIds, uint16_t sequence )
    {
        ConnectionMessageSentPacketEntry * sentPacket = m_messageSentPackets->Insert( sequence );
        
        assert( sentPacket );

        if ( sentPacket )
        {
            sentPacket->acked = 0;
            sentPacket->block = 0;
            sentPacket->timeSent = m_time;
            sentPacket->messageIds = &m_sentPacketMessageIds[ m_sentPackets->GetIndex( sequence ) * m_config.maxMessagesPerPacket ];
            sentPacket->numMessageIds = numMessageIds;            
            for ( int i = 0; i < numMessageIds; ++i )
                sentPacket->messageIds[i] = messageIds[i];
        }
    }

    void Connection::ProcessPacketMessages( const ConnectionPacket * packet )
    {
        const uint16_t minMessageId = m_receiveMessageId;
        const uint16_t maxMessageId = m_receiveMessageId + m_config.messageReceiveQueueSize - 1;

        for ( int i = 0; i < (int) packet->numMessages; ++i )
        {
            Message * message = packet->messages[i];

            assert( message );

            const uint16_t messageId = message->GetId();

            if ( sequence_less_than( messageId, minMessageId ) )
                continue;

            if ( sequence_greater_than( messageId, maxMessageId ) )
            {
                m_error = CONNECTION_ERROR_MESSAGE_DESYNC;
                return;
            }

            if ( m_messageReceiveQueue->Find( messageId ) )
                continue;

            ConnectionMessageReceiveQueueEntry * entry = m_messageReceiveQueue->Insert( messageId );

            entry->message = message;

            m_messageFactory->AddRef( message );

            m_counters[CONNECTION_COUNTER_MESSAGES_RECEIVED]++;
        }
    }

    void Connection::ProcessMessageAck( uint16_t ack )
    {
        ConnectionMessageSentPacketEntry * sentPacketEntry = m_messageSentPackets->Find( ack );

        if ( !sentPacketEntry )
            return;

        assert( !sentPacketEntry->acked );

        for ( int i = 0; i < (int) sentPacketEntry->numMessageIds; ++i )
        {
            const uint16_t messageId = sentPacketEntry->messageIds[i];

            ConnectionMessageSendQueueEntry * sendQueueEntry = m_messageSendQueue->Find( messageId );
            
            if ( sendQueueEntry )
            {
                assert( sendQueueEntry->message );
                assert( sendQueueEntry->message->GetId() == messageId );

                m_messageFactory->Release( sendQueueEntry->message );

                m_messageSendQueue->Remove( messageId );

                UpdateOldestUnackedMessageId();
            }
        }

        if ( sentPacketEntry->block && m_sendBlock.active && m_sendBlock.blockMessageId == sentPacketEntry->blockMessageId )
        {        
            const int messageId = sentPacketEntry->blockMessageId;
            const int fragmentId = sentPacketEntry->blockFragmentId;

            if ( !m_sendBlock.ackedFragment->GetBit( fragmentId ) )
            {
                m_sendBlock.ackedFragment->SetBit( fragmentId );

                m_sendBlock.numAckedFragments++;

                if ( m_sendBlock.numAckedFragments == m_sendBlock.numFragments )
                {
                    m_sendBlock.active = false;

                    ConnectionMessageSendQueueEntry * sendQueueEntry = m_messageSendQueue->Find( messageId );

                    assert( sendQueueEntry );

                    m_messageFactory->Release( sendQueueEntry->message );

                    m_messageSendQueue->Remove( messageId );

                    UpdateOldestUnackedMessageId();
                }
            }
        }
    }

    void Connection::UpdateOldestUnackedMessageId()
    {
        const uint16_t stopMessageId = m_messageSendQueue->GetSequence();

        while ( true )
        {
            if ( m_oldestUnackedMessageId == stopMessageId )
                break;

            ConnectionMessageSendQueueEntry * entry = m_messageSendQueue->Find( m_oldestUnackedMessageId );
            if ( entry )
                break;
           
            ++m_oldestUnackedMessageId;
        }

        assert( !sequence_greater_than( m_oldestUnackedMessageId, stopMessageId ) );
    }

    int Connection::CalculateMessageOverheadBits()
    {
        const int maxMessageType = m_messageFactory->GetNumTypes() - 1;

        const int MessageIdBits = 16;
        
        const int MessageTypeBits = bits_required( 0, maxMessageType );

        return MessageIdBits + MessageTypeBits;
    }

    bool Connection::SendingBlockMessage()
    {
        assert( HasMessagesToSend() );

        ConnectionMessageSendQueueEntry * entry = m_messageSendQueue->Find( m_oldestUnackedMessageId );

        return entry->block;
    }

    uint8_t * Connection::GetFragmentToSend( uint16_t & messageId, uint16_t & fragmentId, int & fragmentBytes, int & numFragments, int & messageType )
    {
        ConnectionMessageSendQueueEntry * entry = m_messageSendQueue->Find( m_oldestUnackedMessageId );

        assert( entry );
        assert( entry->block );

        BlockMessage * blockMessage = (BlockMessage*) entry->message;

        assert( blockMessage );

        messageId = blockMessage->GetId();

        const int blockSize = blockMessage->GetBlockSize();

        if ( !m_sendBlock.active )
        {
            // start sending this block

            m_sendBlock.active = true;
            m_sendBlock.blockSize = blockSize;
            m_sendBlock.blockMessageId = messageId;
            m_sendBlock.numFragments = (int) ceil( blockSize / float( m_config.fragmentSize ) );
            m_sendBlock.numAckedFragments = 0;

            const int MaxFragmentsPerBlock = m_config.GetMaxFragmentsPerBlock();

            assert( m_sendBlock.numFragments > 0 );
            assert( m_sendBlock.numFragments <= MaxFragmentsPerBlock );

            m_sendBlock.ackedFragment->Clear();

            for ( int i = 0; i < MaxFragmentsPerBlock; ++i )
                m_sendBlock.fragmentSendTime[i] = -1.0;
        }

        numFragments = m_sendBlock.numFragments;

        // find the next fragment to send (there may not be one)

        fragmentId = 0xFFFF;

        for ( int i = 0; i < m_sendBlock.numFragments; ++i )
        {
            if ( !m_sendBlock.ackedFragment->GetBit( i ) && m_sendBlock.fragmentSendTime[i] + m_config.fragmentResendRate < m_time )
            {
                fragmentId = uint16_t( i );
                break;
            }
        }

        if ( fragmentId == 0xFFFF )
            return NULL;

        // allocate and return a copy of the fragment data

        messageType = blockMessage->GetType();

        fragmentBytes = m_config.fragmentSize;
        
        const int fragmentRemainder = blockSize % m_config.fragmentSize;

        if ( fragmentRemainder && fragmentId == m_sendBlock.numFragments - 1 )
            fragmentBytes = fragmentRemainder;

        uint8_t * fragmentData = (uint8_t*) m_messageFactory->GetAllocator().Allocate( fragmentBytes );

        if ( fragmentData )
        {
            memcpy( fragmentData, blockMessage->GetBlockData() + fragmentId * m_config.fragmentSize, fragmentBytes );

            m_sendBlock.fragmentSendTime[fragmentId] = m_time;
        }

        return fragmentData;
    }

    void Connection::AddFragmentToPacket( uint16_t messageId, uint16_t fragmentId, uint8_t * fragmentData, int fragmentSize, int numFragments, int messageType, ConnectionPacket * packet )
    {
        assert( packet );

        packet->messageFactory = m_messageFactory;
        packet->blockFragmentData = fragmentData;
        packet->blockMessageId = messageId;
        packet->blockFragmentId = fragmentId;
        packet->blockFragmentSize = fragmentSize;
        packet->blockNumFragments = numFragments;
        packet->blockMessageType = messageType;

        if ( fragmentId == 0 )
        {
            ConnectionMessageSendQueueEntry * entry = m_messageSendQueue->Find( messageId );

            assert( entry );
            assert( entry->block );
            assert( entry->message );
            assert( entry->message->IsBlockMessage() );

            packet->blockMessage = (BlockMessage*) entry->message;

            m_messageFactory->AddRef( packet->blockMessage );
        }
    }

    void Connection::AddFragmentPacketEntry( uint16_t messageId, uint16_t fragmentId, uint16_t sequence )
    {
        ConnectionMessageSentPacketEntry * sentPacket = m_messageSentPackets->Insert( sequence );
        
        assert( sentPacket );

        if ( sentPacket )
        {
            sentPacket->numMessageIds = 0;
            sentPacket->messageIds = NULL;
            sentPacket->timeSent = m_time;
            sentPacket->acked = 0;
            sentPacket->block = 1;
            sentPacket->blockMessageId = messageId;
            sentPacket->blockFragmentId = fragmentId;
        }
    }

    void Connection::ProcessPacketFragment( const ConnectionPacket * packet )
    {  
        if ( packet->blockFragmentData )
        {
            const uint16_t messageId = packet->blockMessageId;
            const uint16_t expectedMessageId = m_messageReceiveQueue->GetSequence();

            if ( messageId != expectedMessageId )
                return;

            // start receiving a new block

            if ( !m_receiveBlock.active )
            {
                const int numFragments = packet->blockNumFragments;

                assert( numFragments >= 0 );
                assert( numFragments <= m_config.GetMaxFragmentsPerBlock() );

                m_receiveBlock.active = true;
                m_receiveBlock.numFragments = numFragments;
                m_receiveBlock.numReceivedFragments = 0;
                m_receiveBlock.messageId = messageId;
                m_receiveBlock.blockSize = 0;
                m_receiveBlock.receivedFragment->Clear();
            }

            // validate fragment

            if ( packet->blockFragmentId >= m_receiveBlock.numFragments )
            {
                m_error = CONNECTION_ERROR_MESSAGE_DESYNC;
                return;
            }

            if ( packet->blockNumFragments != m_receiveBlock.numFragments )
            {
                m_error = CONNECTION_ERROR_MESSAGE_DESYNC;
                return;
            }

            // receive the fragment

            const uint16_t fragmentId = packet->blockFragmentId;

            if ( !m_receiveBlock.receivedFragment->GetBit( fragmentId ) )
            {
                if ( m_listener )
                    m_listener->OnConnectionFragmentReceived( this, messageId, fragmentId );

                m_receiveBlock.receivedFragment->SetBit( fragmentId );

                const int fragmentBytes = packet->blockFragmentSize;

                memcpy( m_receiveBlock.blockData + fragmentId * m_config.fragmentSize, packet->blockFragmentData, fragmentBytes );

                if ( fragmentId == 0 )
                {
                    m_receiveBlock.messageType = packet->blockMessageType;
                }

                if ( fragmentId == m_receiveBlock.numFragments - 1 )
                {
                    m_receiveBlock.blockSize = ( m_receiveBlock.numFragments - 1 ) * m_config.fragmentSize + fragmentBytes;

                    assert( m_receiveBlock.blockSize <= (uint32_t) m_config.maxBlockSize );
                }

                m_receiveBlock.numReceivedFragments++;

                if ( fragmentId == 0 )
                {
                    // save block message (sent with fragment 0)

                    m_receiveBlock.blockMessage = packet->blockMessage;

                    m_messageFactory->AddRef( m_receiveBlock.blockMessage );
                }

                if ( m_receiveBlock.numReceivedFragments == m_receiveBlock.numFragments )
                {
                    // finished receiving block

                    BlockMessage * blockMessage = m_receiveBlock.blockMessage;

                    assert( blockMessage );

                    uint8_t * blockData = (uint8_t*) m_messageFactory->GetAllocator().Allocate( m_receiveBlock.blockSize );

                    if ( !blockData )
                    {
                        m_error = CONNECTION_ERROR_OUT_OF_MEMORY;
                        return;
                    }

                    memcpy( blockData, m_receiveBlock.blockData, m_receiveBlock.blockSize );

                    blockMessage->AttachBlock( m_messageFactory->GetAllocator(), blockData, m_receiveBlock.blockSize );

                    blockMessage->AssignId( messageId );

                    ConnectionMessageReceiveQueueEntry * entry = m_messageReceiveQueue->Insert( messageId );

                    assert( entry );

                    if ( !entry )
                    {
                        m_error = CONNECTION_ERROR_MESSAGE_DESYNC;
                        return;
                    }

                    m_receiveBlock.active = false;
                    m_receiveBlock.blockMessage = NULL;

                    entry->message = blockMessage;
                }
            }
        }
    }
}
