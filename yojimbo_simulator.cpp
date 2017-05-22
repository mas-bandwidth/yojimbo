/*
    Yojimbo Client/Server Network Protocol Library.
    
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

#include "yojimbo_config.h"
#include "yojimbo_simulator.h"

namespace yojimbo
{
    NetworkSimulator::NetworkSimulator( Allocator & allocator, int numPackets )
    {
        assert( numPackets > 0 );
        m_allocator = &allocator;
        m_currentIndex = 0;
        m_time = 0.0;
        m_latency = 0.0f;
        m_jitter = 0.0f;
        m_packetLoss = 0.0f;
        m_duplicates = 0.0f;
        m_active = false;
        m_numPacketEntries = numPackets;
        m_packetEntries = (PacketEntry*) YOJIMBO_ALLOCATE( allocator, sizeof( PacketEntry ) * numPackets );
        if ( !m_packetEntries )
        {
            // todo: this should print out
            debug_printf( "error: not enough memory to allocate network simulator\n" );
        }
        assert( m_packetEntries );
        memset( m_packetEntries, 0, sizeof( PacketEntry ) * numPackets );
    }

    NetworkSimulator::~NetworkSimulator()
    {
        assert( m_allocator );
        assert( m_packetEntries );
        assert( m_numPacketEntries > 0 );
        DiscardPackets();
        YOJIMBO_FREE( *m_allocator, m_packetEntries );
        m_numPacketEntries = 0;
        m_allocator = NULL;
    }

    void NetworkSimulator::SetLatency( float milliseconds )
    {
        m_latency = milliseconds;
        UpdateActive();
    }

    void NetworkSimulator::SetJitter( float milliseconds )
    {
        m_jitter = milliseconds;
        UpdateActive();
    }

    void NetworkSimulator::SetPacketLoss( float percent )
    {
        m_packetLoss = percent;
        UpdateActive();
    }

    void NetworkSimulator::SetDuplicates( float percent )
    {
        m_duplicates = percent;
        UpdateActive();
    }

    bool NetworkSimulator::IsActive() const
    {
        return m_active;
    }

    void NetworkSimulator::UpdateActive()
    {
        bool previous = m_active;
        m_active = m_latency != 0.0f || m_jitter != 0.0f || m_packetLoss != 0.0f || m_duplicates != 0.0f;
        if ( previous && !m_active )
        {
            DiscardPackets();
        }
    }

    void NetworkSimulator::SendPacket( const Address & to, uint8_t * packetData, int packetBytes )
    {
        assert( m_allocator );
        assert( to.IsValid() );
        assert( packetData );
        assert( packetBytes > 0 );

        if ( random_float( 0.0f, 100.0f ) <= m_packetLoss )
        {
            YOJIMBO_FREE( *m_allocator, packetData );
            return;
        }

        PacketEntry & packetEntry = m_packetEntries[m_currentIndex];

        if ( packetEntry.packetData )
        {
            YOJIMBO_FREE( *m_allocator, packetEntry.packetData );
            packetEntry = PacketEntry();
        }

        double delay = m_latency / 1000.0;

        if ( m_jitter > 0 )
            delay += random_float( -m_jitter, +m_jitter ) / 1000.0;

        packetEntry.to = to;
        packetEntry.packetData = packetData;
        packetEntry.packetBytes = packetBytes;
        packetEntry.deliveryTime = m_time + delay;

        m_currentIndex = ( m_currentIndex + 1 ) % m_numPacketEntries;

        if ( random_float( 0.0f, 100.0f ) <= m_duplicates )
        {
            uint8_t * duplicatePacketData = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, packetBytes );

            memcpy( duplicatePacketData, packetData, packetBytes );

            PacketEntry & nextPacketEntry = m_packetEntries[m_currentIndex];

            nextPacketEntry.to = to;
            nextPacketEntry.packetData = duplicatePacketData;
            nextPacketEntry.packetBytes = packetBytes;
            nextPacketEntry.deliveryTime = m_time + delay + random_float( 0, +1.0 );

            m_currentIndex = ( m_currentIndex + 1 ) % m_numPacketEntries;
        }
    }

    int NetworkSimulator::ReceivePackets( int maxPackets, uint8_t * packetData[], int packetBytes[], Address to[] )
    {
        if ( !IsActive() )
            return 0;

        int numPackets = 0;

        for ( int i = 0; i < min( m_numPacketEntries, maxPackets ); ++i )
        {
            if ( !m_packetEntries[i].packetData )
                continue;

            if ( m_packetEntries[i].deliveryTime < m_time )
            {
                packetData[numPackets] = m_packetEntries[i].packetData;
                packetBytes[numPackets] = m_packetEntries[i].packetBytes;
                to[numPackets] = m_packetEntries[i].to;
                m_packetEntries[i].packetData = NULL;
                numPackets++;
            }
        }

        return numPackets;
    }

    void NetworkSimulator::DiscardPackets()
    {
        for ( int i = 0; i < m_numPacketEntries; ++i )
        {
            PacketEntry & packetEntry = m_packetEntries[i];

            if ( !packetEntry.packetData )
                continue;

            YOJIMBO_FREE( *m_allocator, packetEntry.packetData );

            packetEntry = PacketEntry();
        }
    }

    void NetworkSimulator::AdvanceTime( double time )
    {
        m_time = time;
    }
}
