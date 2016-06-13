/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#include "yojimbo_network_simulator.h"

#if YOJIMBO_NETWORK_SIMULATOR

namespace yojimbo
{
    NetworkSimulator::NetworkSimulator( int numPackets )
    {
        assert( numPackets > 0 );
        m_numPacketEntries = numPackets;
        m_packetEntries = new PacketEntry[numPackets];
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
                delete [] m_packetEntries[i].packetData;
            }
        }
        delete [] m_packetEntries;
        m_packetEntries = NULL;
        m_numPacketEntries = 0;
    }

    // hack test
    void NetworkSimulator::Reset()
    {
        for ( int i = 0; i < m_numPacketEntries; ++i )
        {
            if ( m_packetEntries[i].packetData )
            {
                delete [] m_packetEntries[i].packetData;
                m_packetEntries[i] = PacketEntry();
            }
        }
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
            delete [] packetData;
            return;
        }

        PacketEntry & packetEntry = m_packetEntries[m_currentIndex];

        if ( packetEntry.packetData )
        {
            delete [] packetEntry.packetData;
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
            uint8_t * duplicatePacketData = new uint8_t[packetSize];

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

            if ( packetEntry.to != address & packetEntry.from != address )
                continue;

            delete [] packetEntry.packetData;

            packetEntry = PacketEntry();
        }
    }

    void NetworkSimulator::AdvanceTime( double time )
    {
        assert( time >= m_time );
        m_time = time;
    }
}

#endif // #if YOJIMBO_NETWORK_SIMULATOR
