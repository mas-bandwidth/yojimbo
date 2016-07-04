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

#include "yojimbo_simulator.h"

#if YOJIMBO_NETWORK_SIMULATOR

namespace yojimbo
{
    NetworkSimulator::NetworkSimulator( Allocator & allocator, int numPackets )
    {
        m_allocator = &allocator;

        assert( numPackets > 0 );

        m_numPacketEntries = numPackets;

        m_packetEntries = YOJIMBO_NEW_ARRAY( allocator, PacketEntry, numPackets );

        m_time = 0.0;
        m_latency = 0.0f;
        m_jitter = 0.0f;
        m_packetLoss = 0.0f;
        m_duplicates = 0.0f;
        m_currentIndex = 0;
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

        YOJIMBO_DELETE_ARRAY( *m_allocator, m_packetEntries, m_numPacketEntries );
        
        m_numPacketEntries = 0;
    }

    void NetworkSimulator::SetLatency( float milliseconds )
    {
        m_latency = milliseconds;
    }

    void NetworkSimulator::SetJitter( float milliseconds )
    {
        m_jitter = milliseconds;
    }

    void NetworkSimulator::SetPacketLoss( float percent )
    {
        m_packetLoss = percent;
    }

    void NetworkSimulator::SetDuplicates( float percent )
    {
        m_duplicates = percent;
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

        if ( random_float( 0.0f, 100.0f ) <= m_duplicates )
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

    uint8_t * NetworkSimulator::ReceivePacket( Address & from, const Address & to, int & packetSize )
    { 
        int oldestEntryIndex = -1;
        double oldestEntryTime = 0;

        for ( int i = 0; i < m_numPacketEntries; ++i )
        {
            const PacketEntry & packetEntry = m_packetEntries[i];

            if ( !packetEntry.packetData )
                continue;

            if ( packetEntry.to != to )
                continue;

            if ( oldestEntryIndex == -1 || m_packetEntries[i].deliveryTime < oldestEntryTime )
            {
                oldestEntryIndex = i;
                oldestEntryTime = packetEntry.deliveryTime;
            }
        }

        PacketEntry & packetEntry = m_packetEntries[oldestEntryIndex];

        if ( oldestEntryIndex == -1 || packetEntry.deliveryTime > m_time )
            return NULL;

        assert( packetEntry.packetData );

        uint8_t *packetData = packetEntry.packetData;

        from = packetEntry.from;
        packetSize = packetEntry.packetSize;

        packetEntry = PacketEntry();

        return packetData;
    }

    void NetworkSimulator::DiscardPackets( const Address & address )
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
    }

    void NetworkSimulator::AdvanceTime( double time )
    {
        assert( time >= m_time );
        m_time = time;
    }

    SimulatorInterface::SimulatorInterface( Allocator & allocator, 
                                            NetworkSimulator & networkSimulator,
                                            PacketFactory & packetFactory, 
                                            const Address & address,
                                            uint32_t protocolId,
                                            int maxPacketSize, 
                                            int sendQueueSize, 
                                            int receiveQueueSize )
        : BaseInterface( allocator, 
                         packetFactory, 
                         address,
                         protocolId,
                         maxPacketSize,
                         sendQueueSize,
                         receiveQueueSize )
    {
        assert( address.IsValid() );

        m_networkSimulator = &networkSimulator;
    }

    SimulatorInterface::~SimulatorInterface()
    {
        assert( m_networkSimulator );

        m_networkSimulator->DiscardPackets( GetAddress() );

        m_networkSimulator = NULL;
    }

    void SimulatorInterface::AdvanceTime( double time )
    {
        BaseInterface::AdvanceTime( time );

        m_networkSimulator->AdvanceTime( time );
    }

    bool SimulatorInterface::InternalSendPacket( const Address & to, const void * packetData, int packetBytes )
    {
        assert( m_networkSimulator );

        Allocator & allocator = m_networkSimulator->GetAllocator();

        uint8_t * packetDataCopy = (uint8_t*) allocator.Allocate( packetBytes );

        if ( !packetDataCopy )
            return false;

        memcpy( packetDataCopy, packetData, packetBytes );

        m_networkSimulator->SendPacket( GetAddress(), to, packetDataCopy, packetBytes );

        return true;
    }

    int SimulatorInterface::InternalReceivePacket( Address & from, void * packetData, int maxPacketSize )
    {
        (void) maxPacketSize;

        assert( m_networkSimulator );

        int packetSize = 0;

        uint8_t * simulatorPacketData = m_networkSimulator->ReceivePacket( from, GetAddress(), packetSize );

        if ( !simulatorPacketData )
            return 0;

        assert( packetSize > 0 );
        assert( packetSize <= maxPacketSize );

        memcpy( packetData, simulatorPacketData, packetSize );

        Allocator & allocator = m_networkSimulator->GetAllocator();

        allocator.Free( simulatorPacketData );

        return packetSize;
    }
}

#endif // #if YOJIMBO_NETWORK_SIMULATOR
