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

#include "yojimbo_network_simulator.h"

#if YOJIMBO_NETWORK_SIMULATOR

namespace yojimbo
{
    NetworkSimulator::NetworkSimulator( Allocator & allocator, int numPackets )
    {
        m_allocator = &allocator;

        assert( numPackets > 0 );

        m_numPacketEntries = numPackets;
        m_packetEntries = (PacketEntry*) allocator.Allocate( sizeof( PacketEntry ) * numPackets );
        memset( m_packetEntries, 0, sizeof( PacketEntry ) * numPackets );

        m_numPendingReceivePackets = 0;
        m_pendingReceivePackets = (PacketEntry*) allocator.Allocate( sizeof( PacketEntry ) * numPackets );
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

        for ( int i = 0; i < m_numPacketEntries; ++i )
        {
            if ( m_packetEntries[i].packetData )
            {
                m_allocator->Free( m_packetEntries[i].packetData );
            }
        }

        for ( int i = 0; i < m_numPendingReceivePackets; ++i )
        {
            if ( m_pendingReceivePackets[i].packetData )
            {
                m_allocator->Free( m_pendingReceivePackets[i].packetData );
            }
        }

        m_allocator->Free( m_pendingReceivePackets );    
        m_allocator->Free( m_packetEntries );

        m_pendingReceivePackets = NULL;
        m_packetEntries = NULL;
        
        m_numPacketEntries = 0;
        m_numPendingReceivePackets = 0;
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
            if ( m_pendingReceivePackets[i].packetData )
            {
                m_allocator->Free( m_pendingReceivePackets[i].packetData );
                m_pendingReceivePackets[i].packetData = NULL;
            }
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
        assert( from.IsValid() );
        assert( to.IsValid() );

        assert( packetData );
        assert( packetSize > 0 );

        if ( random_float( 0.0f, 100.0f ) <= m_packetLoss )
        {
            m_allocator->Free( packetData );
            return;
        }

        PacketEntry & packetEntry = m_packetEntries[m_currentIndex];

        if ( packetEntry.packetData )
        {
            m_allocator->Free( packetEntry.packetData );
            packetEntry = PacketEntry();
        }

        double delay = m_latency / 1000.0;

        if ( m_jitter > 0 )
            delay += random_float( -m_jitter, +m_jitter ) / 1000.0;

        packetEntry.from = from;
        packetEntry.to = to;
        packetEntry.packetData = packetData;
        packetEntry.packetSize = packetSize;
        packetEntry.deliveryTime = m_time + delay;

        m_currentIndex = ( m_currentIndex + 1 ) % m_numPacketEntries;

        if ( random_float( 0.0f, 100.0f ) <= m_duplicate )
        {
            uint8_t * duplicatePacketData = (uint8_t*) m_allocator->Allocate( packetSize );

            memcpy( duplicatePacketData, packetData, packetSize );

            PacketEntry & nextPacketEntry = m_packetEntries[m_currentIndex];

            nextPacketEntry.from = from;
            nextPacketEntry.to = to;
            nextPacketEntry.packetData = duplicatePacketData;
            nextPacketEntry.packetSize = packetSize;
            nextPacketEntry.deliveryTime = m_time + delay + random_float( -10.0, +10.0 );

            m_currentIndex = ( m_currentIndex + 1 ) % m_numPacketEntries;
        }
    }

    uint8_t * NetworkSimulator::ReceivePacket( Address & from, Address & to, int & packetSize )
    { 
        for ( int i = 0; i < m_numPendingReceivePackets; ++i )
        {
            if ( !m_pendingReceivePackets[i].packetData )
                continue;

            uint8_t * packetData = m_pendingReceivePackets[i].packetData;

            to = m_pendingReceivePackets[i].to;

            from = m_pendingReceivePackets[i].from;            
            
            packetSize = m_pendingReceivePackets[i].packetSize;

            m_pendingReceivePackets[i].packetData = NULL;

            return packetData;
        }

        return NULL;
    }

    uint8_t * NetworkSimulator::ReceivePacketSentToAddress( Address & from, const Address & to, int & packetSize )
    { 
        for ( int i = 0; i < m_numPendingReceivePackets; ++i )
        {
            if ( !m_pendingReceivePackets[i].packetData )
                continue;

            if ( m_pendingReceivePackets[i].to != to )
                continue;

            uint8_t * packetData = m_pendingReceivePackets[i].packetData;

            from = m_pendingReceivePackets[i].from;
            
            packetSize = m_pendingReceivePackets[i].packetSize;

            m_pendingReceivePackets[i].packetData = NULL;

            return packetData;
        }

        return NULL;
    }

    void NetworkSimulator::DiscardPackets()
    {
        for ( int i = 0; i < m_numPacketEntries; ++i )
        {
            PacketEntry & packetEntry = m_packetEntries[i];

            if ( !packetEntry.packetData )
                continue;

            m_allocator->Free( packetEntry.packetData );

            packetEntry = PacketEntry();
        }

        for ( int i = 0; i < m_numPendingReceivePackets; ++i )
        {
            PacketEntry & packetEntry = m_pendingReceivePackets[i];

            if ( !packetEntry.packetData )
                continue;

            m_allocator->Free( packetEntry.packetData );

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

            if ( packetEntry.to != address && packetEntry.from != address )
                continue;

            m_allocator->Free( packetEntry.packetData );

            packetEntry = PacketEntry();
        }

        for ( int i = 0; i < m_numPendingReceivePackets; ++i )
        {
            PacketEntry & packetEntry = m_pendingReceivePackets[i];

            if ( !packetEntry.packetData )
                continue;

            if ( packetEntry.to != address && packetEntry.from != address )
                continue;

            m_allocator->Free( packetEntry.packetData );

            packetEntry = PacketEntry();
        }

        m_numPendingReceivePackets = 0;
    }

    void NetworkSimulator::AdvanceTime( double time )
    {
        assert( time >= m_time );

        m_time = time;

        UpdatePendingReceivePackets();
    }
}

#endif // #if YOJIMBO_NETWORK_SIMULATOR
