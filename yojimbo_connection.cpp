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
    Connection::Connection( const ConnectionConfig & config ) : m_config( config )
    {
        assert( config.packetFactory );
        //assert( config.channelStructure );

        //m_error = CONNECTION_ERROR_NONE;

        m_sentPackets = NULL;

        m_receivedPackets = NULL;
        
        //m_numChannels = 0;

        assert( config.allocator );
        m_allocator = config.allocator;

        m_sentPackets = YOJIMBO_NEW( *m_allocator, SentPackets, *m_allocator, m_config.slidingWindowSize );
        
        m_receivedPackets = YOJIMBO_NEW( *m_allocator, ReceivedPackets, *m_allocator, m_config.slidingWindowSize );

        /*
        m_numChannels = config.channelStructure->GetNumChannels();
        for ( int i = 0; i < m_numChannels; ++i )
        {
            m_channels[i] = config.channelStructure->CreateChannel( i );
            m_channels[i]->SetContext( config.context );
            CORE_ASSERT( m_channels[i] );
        }
        */

        Reset();
    }

    Connection::~Connection()
    {
        assert( m_sentPackets );
        assert( m_receivedPackets );
//        assert( m_channels );

        /*
        for ( int i = 0; i < m_numChannels; ++i )
        {
            CORE_ASSERT( m_channels[i] );
            m_config.channelStructure->DestroyChannel( m_channels[i] );
        }
        */

        YOJIMBO_DELETE( *m_allocator, SentPackets, m_sentPackets );
        YOJIMBO_DELETE( *m_allocator, ReceivedPackets, m_receivedPackets );

        m_sentPackets = nullptr;
        m_receivedPackets = nullptr;
    }

    /*
    Channel * Connection::GetChannel( int index )
    {
        CORE_ASSERT( index >= 0 );
        CORE_ASSERT( index < m_numChannels );
        return m_channels[index];
    }
    */

    void Connection::Reset()
    {
        //m_error = CONNECTION_ERROR_NONE;

        //m_timeBase = core::TimeBase();
        m_time = 0.0;

        m_sentPackets->Reset();
        m_receivedPackets->Reset();

        /*
        for ( int i = 0; i < m_numChannels; ++i )
            m_channels[i]->Reset();
            */

        //memset( m_counters, 0, sizeof( m_counters ) );
    }

// todo
    /*
    void Connection::Update( const core::TimeBase & timeBase )
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return;

        m_timeBase = timeBase;

        for ( int i = 0; i < m_numChannels; ++i )
        {
            m_channels[i]->Update( timeBase );

            if ( m_channels[i]->GetError() != 0 )
            {
                m_error = CONNECTION_ERROR_CHANNEL;
                return;
            }
        }
    }
    */

    /*
    ConnectionError Connection::GetError() const
    {
        return m_error;
    }
    */

    /*
    int Connection::GetChannelError( int channelIndex ) const
    {
        CORE_ASSERT( channelIndex >= 0 );
        CORE_ASSERT( channelIndex < m_numChannels );
        return m_channels[channelIndex]->GetError();
    }
    */

    /*
    double Connection::GetTime() const
    {
        return m_time;
    }
    */

    ConnectionPacket * Connection::WritePacket()
    {
        /*
        if ( m_error != CONNECTION_ERROR_NONE )
            return nullptr;
            */

#if 0

        ConnectionPacket * packet = static_cast<ConnectionPacket*>( m_config.packetFactory->Create( CONNECTION_PACKET ) );

        packet->sequence = m_sentPackets->GetSequence();

        GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

        for ( int i = 0; i < m_numChannels; ++i )
            packet->channelData[i] = m_channels[i]->GetData( packet->sequence );

        SentPacketData * entry = m_sentPackets->Insert( packet->sequence );
        CORE_ASSERT( entry );
        entry->acked = 0;

        m_counters[CONNECTION_COUNTER_PACKETS_WRITTEN]++;

        return packet;

#endif

        return NULL;
    }

    bool Connection::ReadPacket( ConnectionPacket * packet )
    {
        /*
        if ( m_error != CONNECTION_ERROR_NONE )
            return false;
            */

        assert( packet );
        assert( packet->GetType() == CONNECTION_PACKET );

//            printf( "read packet %d\n", (int) packet->sequence );

        ProcessAcks( packet->ack, packet->ack_bits );

//        m_counters[CONNECTION_COUNTER_PACKETS_READ]++;

        bool discardPacket = false;

        /*
        for ( int i = 0; i < m_numChannels; ++i )
        {
            if ( !packet->channelData[i] )
                continue;

            bool result = m_channels[i]->ProcessData( packet->sequence, packet->channelData[i] );

            if ( !result )
                discardPacket = true;
        }
        */

        if ( discardPacket || !m_receivedPackets->Insert( packet->sequence ) )
        {
            //m_counters[CONNECTION_COUNTER_PACKETS_DISCARDED]++;
            return false;            
        }

        return true;
    }

    /*
    uint64_t Connection::GetCounter( int index ) const
    {
        assert( index >= 0 );
        assert( index < CONNECTION_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }
    */

    void Connection::ProcessAcks( uint16_t ack, uint32_t ack_bits )
    {
//            printf( "process acks: %d - %x\n", (int)ack, ack_bits );

        for ( int i = 0; i < 32; ++i )
        {
            if ( ack_bits & 1 )
            {                    
                const uint16_t sequence = ack - i;
                SentPacketData * packetData = m_sentPackets->Find( sequence );
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
        (void)sequence;

//            printf( "packet %d acked\n", (int) sequence );

        //m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;

        /*
        for ( int i = 0; i < m_numChannels; ++i )
            m_channels[i]->ProcessAck( sequence );
            */
    }
}
