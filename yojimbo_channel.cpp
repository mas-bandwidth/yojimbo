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

#include "yojimbo_channel.h"

namespace yojimbo
{
    Channel::Channel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config ) : m_config( config )
    {
        assert( ( 65536 % config.slidingWindowSize ) == 0 );
        assert( ( 65536 % config.messageSendQueueSize ) == 0 );
        assert( ( 65536 % config.messageReceiveQueueSize ) == 0 );

        m_allocator = &allocator;

        m_messageFactory = &messageFactory;

        m_error = CHANNEL_ERROR_NONE;

        m_messageOverheadBits = CalculateMessageOverheadBits();

        m_messageSendQueue = YOJIMBO_NEW( *m_allocator, SequenceBuffer<MessageSendQueueEntry>, *m_allocator, m_config.messageSendQueueSize );
        
        m_messageSentPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<MessageSentPacketEntry>, *m_allocator, m_config.slidingWindowSize );
        
        m_messageReceiveQueue = YOJIMBO_NEW( *m_allocator, SequenceBuffer<MessageReceiveQueueEntry>, *m_allocator, m_config.messageReceiveQueueSize );
        
        m_sentPacketMessageIds = (uint16_t*) m_allocator->Allocate( sizeof( uint16_t ) * m_config.maxMessagesPerPacket * m_config.messageSendQueueSize );

        m_sendBlock = YOJIMBO_NEW( *m_allocator, SendBlockData, *m_allocator, m_config.maxBlockSize, m_config.GetMaxFragmentsPerBlock() );
        
        m_receiveBlock = YOJIMBO_NEW( *m_allocator, ReceiveBlockData, *m_allocator, m_config.maxBlockSize, m_config.GetMaxFragmentsPerBlock() );

        Reset();
    }

    Channel::~Channel()
    {
        Reset();

        YOJIMBO_DELETE( *m_allocator, SendBlockData, m_sendBlock );
        YOJIMBO_DELETE( *m_allocator, ReceiveBlockData, m_receiveBlock );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<MessageSendQueueEntry>, m_messageSendQueue );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<MessageSentPacketEntry>, m_messageSentPackets );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<MessageReceiveQueueEntry>, m_messageReceiveQueue );
        
		m_allocator->Free( m_sentPacketMessageIds );

        m_sentPacketMessageIds = NULL;
    }

    void Channel::Reset()
    {
        m_error = CHANNEL_ERROR_NONE;

        m_time = 0.0;

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

        m_sendBlock->Reset();

        m_receiveBlock->Reset();

        if ( m_receiveBlock->blockMessage )
        {
            m_messageFactory->Release( m_receiveBlock->blockMessage );
            m_receiveBlock->blockMessage = NULL;
        }

        memset( m_counters, 0, sizeof( m_counters ) );
    }

    bool Channel::CanSendMessage() const
    {
        assert( m_messageSendQueue );

        return m_messageSendQueue->Available( m_sendMessageId );
    }

    void Channel::SendMessage( Message * message )
    {
        assert( message );
        assert( CanSendMessage() );

        if ( GetError() != CHANNEL_ERROR_NONE )
        {
            m_messageFactory->Release( message );
            return;
        }

        if ( !CanSendMessage() )
        {
            m_error = CHANNEL_ERROR_SEND_QUEUE_FULL;
            m_messageFactory->Release( message );
            return;
        }

        message->AssignId( m_sendMessageId );

        MessageSendQueueEntry * entry = m_messageSendQueue->Insert( m_sendMessageId );

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
            m_error = CHANNEL_ERROR_SERIALIZE_MEASURE_FAILED;
            m_messageFactory->Release( message );
            return;
        }

        entry->measuredBits = measureStream.GetBitsProcessed() + m_messageOverheadBits;

        m_counters[CHANNEL_COUNTER_MESSAGES_SENT]++;

        m_sendMessageId++;
    }

    Message * Channel::ReceiveMessage()
    {
        if ( GetError() != CHANNEL_ERROR_NONE )
            return NULL;

        MessageReceiveQueueEntry * entry = m_messageReceiveQueue->Find( m_receiveMessageId );
        if ( !entry )
            return NULL;

        Message * message = entry->message;

        assert( message );
        assert( message->GetId() == m_receiveMessageId );

        m_messageReceiveQueue->Remove( m_receiveMessageId );

        m_counters[CHANNEL_COUNTER_MESSAGES_RECEIVED]++;

        m_receiveMessageId++;

        return message;
    }

    void Channel::AdvanceTime( double time )
    {
        m_time = time;
    }
    
    ChannelError Channel::GetError() const
    {
        return m_error;
    }

    bool Channel::HasMessagesToSend() const
    {
        return m_oldestUnackedMessageId != m_sendMessageId;
    }

    void Channel::GetMessagesToSend( uint16_t * messageIds, int & numMessageIds )
    {
        assert( HasMessagesToSend() );

        numMessageIds = 0;

        const int GiveUpBits = 8 * 8;

        int availableBits = m_config.messagePacketBudget * 8;

        const int messageLimit = min( m_config.messageSendQueueSize, m_config.messageReceiveQueueSize ) / 2;

        for ( int i = 0; i < messageLimit; ++i )
        {
            const uint16_t messageId = m_oldestUnackedMessageId + i;

            MessageSendQueueEntry * entry = m_messageSendQueue->Find( messageId );

            if ( !entry )
                continue;

            if ( entry->block )
                break;
            
            if ( entry->timeLastSent + m_config.messageResendRate <= m_time && availableBits >= (int) entry->measuredBits )
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

    void Channel::AddMessagePacketEntry( const uint16_t * messageIds, int numMessageIds, uint16_t sequence )
    {
        MessageSentPacketEntry * sentPacket = m_messageSentPackets->Insert( sequence );
        
        assert( sentPacket );

        if ( sentPacket )
        {
            sentPacket->acked = 0;
            sentPacket->block = 0;
            sentPacket->timeSent = m_time;
            sentPacket->messageIds = &m_sentPacketMessageIds[ ( sequence % m_config.slidingWindowSize ) * m_config.maxMessagesPerPacket ];
            sentPacket->numMessageIds = numMessageIds;            
            for ( int i = 0; i < numMessageIds; ++i )
                sentPacket->messageIds[i] = messageIds[i];
        }
    }

    void Channel::ProcessPacketMessages( int numMessages, Message ** messages )
    {
        const uint16_t minMessageId = m_receiveMessageId;
        const uint16_t maxMessageId = m_receiveMessageId + m_config.messageReceiveQueueSize - 1;

        for ( int i = 0; i < (int) numMessages; ++i )
        {
            Message * message = messages[i];

            assert( message );

            const uint16_t messageId = message->GetId();

            if ( sequence_less_than( messageId, minMessageId ) )
                continue;

            if ( sequence_greater_than( messageId, maxMessageId ) )
            {
                m_error = CHANNEL_ERROR_DESYNC;
                return;
            }

            if ( m_messageReceiveQueue->Find( messageId ) )
                continue;

            MessageReceiveQueueEntry * entry = m_messageReceiveQueue->Insert( messageId );

            entry->message = message;

            m_messageFactory->AddRef( message );
        }
    }

    void Channel::ProcessAck( uint16_t ack )
    {
        MessageSentPacketEntry * sentPacketEntry = m_messageSentPackets->Find( ack );

        if ( !sentPacketEntry )
            return;

        assert( !sentPacketEntry->acked );

        for ( int i = 0; i < (int) sentPacketEntry->numMessageIds; ++i )
        {
            const uint16_t messageId = sentPacketEntry->messageIds[i];

            MessageSendQueueEntry * sendQueueEntry = m_messageSendQueue->Find( messageId );
            
            if ( sendQueueEntry )
            {
                assert( sendQueueEntry->message );
                assert( sendQueueEntry->message->GetId() == messageId );

                m_messageFactory->Release( sendQueueEntry->message );

                m_messageSendQueue->Remove( messageId );

                UpdateOldestUnackedMessageId();
            }
        }

        if ( sentPacketEntry->block && m_sendBlock->active && m_sendBlock->blockMessageId == sentPacketEntry->blockMessageId )
        {        
            const int messageId = sentPacketEntry->blockMessageId;
            const int fragmentId = sentPacketEntry->blockFragmentId;

            if ( !m_sendBlock->ackedFragment->GetBit( fragmentId ) )
            {
                m_sendBlock->ackedFragment->SetBit( fragmentId );

                m_sendBlock->numAckedFragments++;

                if ( m_sendBlock->numAckedFragments == m_sendBlock->numFragments )
                {
                    m_sendBlock->active = false;

                    MessageSendQueueEntry * sendQueueEntry = m_messageSendQueue->Find( messageId );

                    assert( sendQueueEntry );

                    m_messageFactory->Release( sendQueueEntry->message );

                    m_messageSendQueue->Remove( messageId );

                    UpdateOldestUnackedMessageId();
                }
            }
        }
    }

    void Channel::UpdateOldestUnackedMessageId()
    {
        const uint16_t stopMessageId = m_messageSendQueue->GetSequence();

        while ( true )
        {
            if ( m_oldestUnackedMessageId == stopMessageId )
                break;

            MessageSendQueueEntry * entry = m_messageSendQueue->Find( m_oldestUnackedMessageId );
            if ( entry )
                break;
           
            ++m_oldestUnackedMessageId;
        }

        assert( !sequence_greater_than( m_oldestUnackedMessageId, stopMessageId ) );
    }

    int Channel::CalculateMessageOverheadBits()
    {
        const int maxMessageType = m_messageFactory->GetNumTypes() - 1;

        const int MessageIdBits = 16;
        
        const int MessageTypeBits = bits_required( 0, maxMessageType );

        return MessageIdBits + MessageTypeBits;
    }

    bool Channel::SendingBlockMessage()
    {
        assert( HasMessagesToSend() );

        MessageSendQueueEntry * entry = m_messageSendQueue->Find( m_oldestUnackedMessageId );

        return entry->block;
    }

    uint8_t * Channel::GetFragmentToSend( uint16_t & messageId, uint16_t & fragmentId, int & fragmentBytes, int & numFragments, int & messageType )
    {
        MessageSendQueueEntry * entry = m_messageSendQueue->Find( m_oldestUnackedMessageId );

        assert( entry );
        assert( entry->block );

        BlockMessage * blockMessage = (BlockMessage*) entry->message;

        assert( blockMessage );

        messageId = blockMessage->GetId();

        const int blockSize = blockMessage->GetBlockSize();

        if ( !m_sendBlock->active )
        {
            // start sending this block

            m_sendBlock->active = true;
            m_sendBlock->blockSize = blockSize;
            m_sendBlock->blockMessageId = messageId;
            m_sendBlock->numFragments = (int) ceil( blockSize / float( m_config.fragmentSize ) );
            m_sendBlock->numAckedFragments = 0;

            const int MaxFragmentsPerBlock = m_config.GetMaxFragmentsPerBlock();

            assert( m_sendBlock->numFragments > 0 );
            assert( m_sendBlock->numFragments <= MaxFragmentsPerBlock );

            m_sendBlock->ackedFragment->Clear();

            for ( int i = 0; i < MaxFragmentsPerBlock; ++i )
                m_sendBlock->fragmentSendTime[i] = -1.0;
        }

        numFragments = m_sendBlock->numFragments;

        // find the next fragment to send (there may not be one)

        fragmentId = 0xFFFF;

        for ( int i = 0; i < m_sendBlock->numFragments; ++i )
        {
            if ( !m_sendBlock->ackedFragment->GetBit( i ) && m_sendBlock->fragmentSendTime[i] + m_config.fragmentResendRate < m_time )
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

        if ( fragmentRemainder && fragmentId == m_sendBlock->numFragments - 1 )
            fragmentBytes = fragmentRemainder;

        uint8_t * fragmentData = (uint8_t*) m_messageFactory->GetAllocator().Allocate( fragmentBytes );

        if ( fragmentData )
        {
            memcpy( fragmentData, blockMessage->GetBlockData() + fragmentId * m_config.fragmentSize, fragmentBytes );

            m_sendBlock->fragmentSendTime[fragmentId] = m_time;
        }

        return fragmentData;
    }

    void Channel::AddFragmentPacketEntry( uint16_t messageId, uint16_t fragmentId, uint16_t sequence )
    {
        MessageSentPacketEntry * sentPacket = m_messageSentPackets->Insert( sequence );
        
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

    void Channel::ProcessPacketFragment( int messageType, uint16_t messageId, int numFragments, uint16_t fragmentId, const uint8_t * fragmentData, int fragmentBytes, BlockMessage * blockMessage )
    {  
        if ( fragmentData )
        {
            const uint16_t expectedMessageId = m_messageReceiveQueue->GetSequence();

            if ( messageId != expectedMessageId )
                return;

            // start receiving a new block

            if ( !m_receiveBlock->active )
            {
                assert( numFragments >= 0 );
                assert( numFragments <= m_config.GetMaxFragmentsPerBlock() );

                m_receiveBlock->active = true;
                m_receiveBlock->numFragments = numFragments;
                m_receiveBlock->numReceivedFragments = 0;
                m_receiveBlock->messageId = messageId;
                m_receiveBlock->blockSize = 0;
                m_receiveBlock->receivedFragment->Clear();
            }

            // validate fragment

            if ( fragmentId >= m_receiveBlock->numFragments )
            {
                m_error = CHANNEL_ERROR_DESYNC;
                return;
            }

            if ( numFragments != m_receiveBlock->numFragments )
            {
                m_error = CHANNEL_ERROR_DESYNC;
                return;
            }

            // receive the fragment

            if ( !m_receiveBlock->receivedFragment->GetBit( fragmentId ) )
            {
                // todo
                /*
                if ( m_listener )
                    m_listener->OnConnectionFragmentReceived( this, messageId, fragmentId );
                    */

                m_receiveBlock->receivedFragment->SetBit( fragmentId );

                memcpy( m_receiveBlock->blockData + fragmentId * m_config.fragmentSize, fragmentData, fragmentBytes );

                if ( fragmentId == 0 )
                {
                    m_receiveBlock->messageType = messageType;
                }

                if ( fragmentId == m_receiveBlock->numFragments - 1 )
                {
                    m_receiveBlock->blockSize = ( m_receiveBlock->numFragments - 1 ) * m_config.fragmentSize + fragmentBytes;

                    assert( m_receiveBlock->blockSize <= (uint32_t) m_config.maxBlockSize );
                }

                m_receiveBlock->numReceivedFragments++;

                if ( fragmentId == 0 )
                {
                    // save block message (sent with fragment 0)

                    m_receiveBlock->blockMessage = blockMessage;

                    m_messageFactory->AddRef( m_receiveBlock->blockMessage );
                }

                if ( m_receiveBlock->numReceivedFragments == m_receiveBlock->numFragments )
                {
                    // finished receiving block

                    BlockMessage * blockMessage = m_receiveBlock->blockMessage;

                    assert( blockMessage );

                    uint8_t * blockData = (uint8_t*) m_messageFactory->GetAllocator().Allocate( m_receiveBlock->blockSize );

                    if ( !blockData )
                    {
                        m_error = CHANNEL_ERROR_OUT_OF_MEMORY;
                        return;
                    }

                    memcpy( blockData, m_receiveBlock->blockData, m_receiveBlock->blockSize );

                    blockMessage->AttachBlock( m_messageFactory->GetAllocator(), blockData, m_receiveBlock->blockSize );

                    blockMessage->AssignId( messageId );

                    MessageReceiveQueueEntry * entry = m_messageReceiveQueue->Insert( messageId );

                    assert( entry );

                    if ( !entry )
                    {
                        m_error = CHANNEL_ERROR_DESYNC;
                        return;
                    }

                    m_receiveBlock->active = false;
                    m_receiveBlock->blockMessage = NULL;

                    entry->message = blockMessage;
                }
            }
        }
    }

    Message * Channel::GetSendQueueMessage( uint16_t messageId )
    {
        MessageSendQueueEntry * entry = m_messageSendQueue->Find( messageId );
        return entry ? entry->message : NULL;
    }

    uint64_t Channel::GetCounter( int index ) const
    {
        assert( index >= 0 );
        assert( index < CHANNEL_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }
}
