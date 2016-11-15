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
#include "yojimbo_network_simulator.h"

namespace yojimbo
{
    NetworkSimulator::NetworkSimulator( Allocator & allocator, int numPackets )
    {
        m_allocator = &allocator;

        assert( numPackets > 0 );

        m_numPacketEntries = numPackets;
        m_packetEntries = (PacketEntry*) YOJIMBO_ALLOCATE( allocator, sizeof( PacketEntry ) * numPackets );
        memset( m_packetEntries, 0, sizeof( PacketEntry ) * numPackets );

        m_numPendingReceivePackets = 0;
        m_pendingReceivePackets = (PacketEntry*) YOJIMBO_ALLOCATE( allocator, sizeof( PacketEntry ) * numPackets );
        memset( m_pendingReceivePackets, 0, sizeof( PacketEntry ) * numPackets );

        m_lastPendingReceiveTime = -10000.0;

        m_currentIndex = 0;

        m_time = 0.0;

        m_latency = 0.0f;
        m_jitter = 0.0f;
        m_packetLoss = 0.0f;
        m_duplicate = 0.0f;

        UpdateActive();
    }

    NetworkSimulator::~NetworkSimulator()
    {
        assert( m_packetEntries );
        assert( m_numPacketEntries > 0 );

        DiscardPackets();

        YOJIMBO_FREE( *m_allocator, m_pendingReceivePackets );
        YOJIMBO_FREE( *m_allocator, m_packetEntries );

        m_numPacketEntries = 0;
        m_numPendingReceivePackets = 0;

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

    void NetworkSimulator::SetDuplicate( float percent )
    {
        m_duplicate = percent;
        UpdateActive();
    }

    bool NetworkSimulator::IsActive() const
    {
        return m_active;
    }

    void NetworkSimulator::UpdateActive()
    {
        m_active = m_latency != 0.0f || m_jitter != 0.0f || m_packetLoss != 0.0f || m_duplicate != 0.0f;
    }

    void NetworkSimulator::UpdatePendingReceivePackets()
    {
        // has time moved forward? double updates can happen with multiple local transports wrapping the same packet simulator

        if ( m_lastPendingReceiveTime >= m_time )
            return;

        m_lastPendingReceiveTime = m_time;
        
        // free any pending receive packets that are still in the buffer

        for ( int i = 0; i < m_numPendingReceivePackets; ++i )
        {
            YOJIMBO_FREE( *m_allocator, m_pendingReceivePackets[i].packetData );
        }

        m_numPendingReceivePackets = 0;

        // walk across packet entries and move any that are ready to be received into the pending receive buffer

        for ( int i = 0; i < m_numPacketEntries; ++i )
        {
            if ( !m_packetEntries[i].packetData )
                continue;

            if ( m_packetEntries[i].deliveryTime < m_time )
            {
                m_pendingReceivePackets[m_numPendingReceivePackets] = m_packetEntries[i];
                m_numPendingReceivePackets++;
                m_packetEntries[i].packetData = NULL;
            }
        }
    }

    void NetworkSimulator::SendPacket( const Address & from, const Address & to, uint8_t * packetData, int packetSize )
    {
        assert( m_allocator );

        assert( from.IsValid() );
        assert( to.IsValid() );

        assert( packetData );
        assert( packetSize > 0 );

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
            delay += random_float( 0, +m_jitter ) / 1000.0;

        packetEntry.from = from;
        packetEntry.to = to;
        packetEntry.packetData = packetData;
        packetEntry.packetSize = packetSize;
        packetEntry.deliveryTime = m_time + delay;

        m_currentIndex = ( m_currentIndex + 1 ) % m_numPacketEntries;

        if ( random_float( 0.0f, 100.0f ) <= m_duplicate )
        {
            uint8_t * duplicatePacketData = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, packetSize );

            memcpy( duplicatePacketData, packetData, packetSize );

            PacketEntry & nextPacketEntry = m_packetEntries[m_currentIndex];

            nextPacketEntry.from = from;
            nextPacketEntry.to = to;
            nextPacketEntry.packetData = duplicatePacketData;
            nextPacketEntry.packetSize = packetSize;
            nextPacketEntry.deliveryTime = m_time + delay + random_float( 0.0, +1.0 );

            m_currentIndex = ( m_currentIndex + 1 ) % m_numPacketEntries;
        }
    }

    int NetworkSimulator::ReceivePackets( int maxPackets, uint8_t * packetData[], int packetSize[], Address from[], Address to[] )
    {
        int numPackets = 0;

        for ( int i = 0; i < min( maxPackets, m_numPendingReceivePackets ); ++i )
        {
            if ( !m_pendingReceivePackets[i].packetData )
                continue;

            packetData[numPackets] = m_pendingReceivePackets[i].packetData;
            packetSize[numPackets] = m_pendingReceivePackets[i].packetSize;
            from[numPackets] = m_pendingReceivePackets[i].from;
            to[numPackets] = m_pendingReceivePackets[i].to;

            m_pendingReceivePackets[i].packetData = NULL;

            numPackets++;
        }

        return numPackets;
    }

    int NetworkSimulator::ReceivePacketsSentToAddress( int maxPackets, const Address & to, uint8_t * packetData[], int packetSize[], Address from[] )
    {
        int numPackets = 0;

        for ( int i = 0; i < min( maxPackets, m_numPendingReceivePackets ); ++i )
        {
            if ( !m_pendingReceivePackets[i].packetData )
                continue;

            if ( m_pendingReceivePackets[i].to != to )
                continue;

            packetData[numPackets] = m_pendingReceivePackets[i].packetData;
            packetSize[numPackets] = m_pendingReceivePackets[i].packetSize;
            from[numPackets] = m_pendingReceivePackets[i].from;

            m_pendingReceivePackets[i].packetData = NULL;

            numPackets++;
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

        for ( int i = 0; i < m_numPendingReceivePackets; ++i )
        {
            PacketEntry & packetEntry = m_pendingReceivePackets[i];

            if ( !packetEntry.packetData )
                continue;

            YOJIMBO_FREE( *m_allocator, packetEntry.packetData );

            packetEntry = PacketEntry();
        }

        m_numPendingReceivePackets = 0;
    }

    void NetworkSimulator::DiscardPacketsFromAddress( const Address & address )
    {
        for ( int i = 0; i < m_numPacketEntries; ++i )
        {
            PacketEntry & packetEntry = m_packetEntries[i];

            if ( !packetEntry.packetData )
                continue;

            if ( packetEntry.from != address )
                continue;

            YOJIMBO_FREE( *m_allocator, packetEntry.packetData );

            packetEntry = PacketEntry();
        }

        for ( int i = 0; i < m_numPendingReceivePackets; ++i )
        {
            PacketEntry & packetEntry = m_pendingReceivePackets[i];

            if ( !packetEntry.packetData )
                continue;

            if ( packetEntry.from != address )
                continue;

            YOJIMBO_FREE( *m_allocator, packetEntry.packetData );

            packetEntry = PacketEntry();
        }
    }

    void NetworkSimulator::AdvanceTime( double time )
    {
        assert( time >= m_time );

        m_time = time;

        UpdatePendingReceivePackets();
    }
}
