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
        messageFactory = NULL;
        sequence = 0;
        ack = 0;
        ack_bits = 0;
    }

    ConnectionPacket::~ConnectionPacket()
    {
        if ( messageFactory )
        {
            channelData.Free( *messageFactory );
        }
    }

    template <typename Stream> bool ConnectionPacket::Serialize( Stream & stream )
    {
        ConnectionContext * context = (ConnectionContext*) stream.GetContext();

        assert( context );

        messageFactory = context->messageFactory;

        // ack system

        bool perfect_acks = Stream::IsWriting ? ( ack_bits == 0xFFFFFFFF ) : 0;

        serialize_bool( stream, perfect_acks );

        if ( !perfect_acks )
            serialize_bits( stream, ack_bits, 32 );
        else
            ack_bits = 0xFFFFFFFF;

        serialize_bits( stream, sequence, 16 );

        serialize_ack_relative( stream, sequence, ack );

        // channel data

        // todo: need to iterate across available channel data
        channelData.channelId = 0;

        if ( !channelData.Serialize( stream, *messageFactory, context->connectionConfig->channelConfig[0] ) )
            return false;

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

        for ( int i = 0; i < m_config.numChannels; ++i )
        {
            m_channel[i] = YOJIMBO_NEW( *m_allocator, Channel, *m_allocator, messageFactory, m_config.channelConfig[i], i );
            m_channel[i]->SetListener( this );
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

        int numMessageIds = 0;

        // todo
        const int channelId = 0;
        
        uint16_t * messageIds = (uint16_t*) alloca( m_config.channelConfig[channelId].maxMessagesPerPacket * sizeof( uint16_t ) );

        if ( m_channel[channelId]->HasMessagesToSend() )
        {
            if ( m_channel[channelId]->SendingBlockMessage() )
            {
                uint16_t messageId;
                uint16_t fragmentId;
                int fragmentBytes;
                int numFragments;
                int messageType;

                uint8_t * fragmentData = m_channel[channelId]->GetFragmentToSend( messageId, fragmentId, fragmentBytes, numFragments, messageType );

                if ( fragmentData )
                {
                    packet->messageFactory = m_messageFactory;

                    m_channel[channelId]->GetFragmentPacketData( packet->channelData, messageId, fragmentId, fragmentData, fragmentBytes, numFragments, messageType );

                    m_channel[channelId]->AddFragmentPacketEntry( messageId, fragmentId, packet->sequence );
                }
            }
            else
            {
                m_channel[channelId]->GetMessagesToSend( messageIds, numMessageIds );

                if ( numMessageIds > 0 )
                {
                    packet->messageFactory = m_messageFactory;

                    m_channel[channelId]->GetMessagePacketData( packet->channelData, messageIds, numMessageIds );

                    m_channel[channelId]->AddMessagePacketEntry( messageIds, numMessageIds, packet->sequence );
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

        // todo
        const int channelId = 0;

        m_channel[channelId]->ProcessPacketData( packet->channelData );

        return true;
    }

    void Connection::AdvanceTime( double time )
    {
        for ( int i = 0; i < m_config.numChannels; ++i )
        {
            m_channel[i]->AdvanceTime( time );
        
            if ( m_channel[i]->GetError() )
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

        for ( int i = 0; i < m_config.numChannels; ++i )
            m_channel[i]->ProcessAck( sequence );

        m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;
    }

    void Connection::OnChannelFragmentReceived( class Channel * /*channel*/, uint16_t messageId, uint16_t fragmentId )
    {
        if ( m_listener )
        {
            m_listener->OnConnectionFragmentReceived( this, messageId, fragmentId );      // todo: will want to pass in channel id as well
        }
    }
}
