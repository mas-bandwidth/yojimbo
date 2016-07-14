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

        m_channel = YOJIMBO_NEW( *m_allocator, Channel, *m_allocator, messageFactory );

        m_sentPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionSentPacketData>, *m_allocator, m_config.slidingWindowSize );
        
        m_receivedPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<ConnectionReceivedPacketData>, *m_allocator, m_config.slidingWindowSize );

        Reset();
    }

    Connection::~Connection()
    {
        Reset();

        YOJIMBO_DELETE( *m_allocator, Channel, m_channel );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionSentPacketData>, m_sentPackets );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<ConnectionReceivedPacketData>, m_receivedPackets );
    }

    void Connection::Reset()
    {
        m_error = CONNECTION_ERROR_NONE;

        m_channel->Reset();
        m_sentPackets->Reset();
        m_receivedPackets->Reset();

        memset( m_counters, 0, sizeof( m_counters ) );
    }

    bool Connection::CanSendMessage() const
    {
        return m_channel->CanSendMessage();
    }

    void Connection::SendMessage( Message * message )
    {
        return m_channel->SendMessage( message );
    }

    Message * Connection::ReceiveMessage()
    {
        return m_channel->ReceiveMessage();
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
        
        // todo: this actually has to come from the channel config
        uint16_t * messageIds = (uint16_t*) alloca( m_config.maxMessagesPerPacket * sizeof( uint16_t ) );

        if ( m_channel->HasMessagesToSend() )
        {
            if ( m_channel->SendingBlockMessage() )
            {
                uint16_t messageId;
                uint16_t fragmentId;
                int fragmentBytes;
                int numFragments;
                int messageType;

                uint8_t * fragmentData = m_channel->GetFragmentToSend( messageId, fragmentId, fragmentBytes, numFragments, messageType );

                if ( fragmentData )
                {
                    AddFragmentToPacket( messageId, fragmentId, fragmentData, fragmentBytes, numFragments, messageType, packet );

                    m_channel->AddFragmentPacketEntry( messageId, fragmentId, packet->sequence );
                }
            }
            else
            {
                m_channel->GetMessagesToSend( messageIds, numMessageIds );

                AddMessagesToPacket( messageIds, numMessageIds, packet );

                m_channel->AddMessagePacketEntry( messageIds, numMessageIds, packet->sequence );
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

        m_channel->ProcessPacketMessages( packet->numMessages, packet->messages );

        m_channel->ProcessPacketFragment( packet->blockMessageType, packet->blockMessageId, packet->blockNumFragments, packet->blockFragmentId, packet->blockFragmentData, packet->blockFragmentSize, packet->blockMessage );

        return true;
    }

    void Connection::AdvanceTime( double time )
    {
        m_channel->AdvanceTime( time );

        if ( m_channel->GetError() )
        {
            m_error = CONNECTION_ERROR_CHANNEL;
            return;
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

        m_channel->ProcessAck( sequence );

        m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;
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
            packet->messages[i] = m_channel->GetSendQueueMessage( messageIds[i] );

            m_messageFactory->AddRef( packet->messages[i] );
        }
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
            packet->blockMessage = (BlockMessage*) m_channel->GetSendQueueMessage( messageId );
            
            m_messageFactory->AddRef( packet->blockMessage );
        }
    }
}
