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
        m_currentTime = 0.0;
        m_latency = 0.0f;
        m_jitter = 0.0f;
        m_packetLoss = 0.0f;
        m_duplicates = 0.0f;
        m_currentIndex = 0;
        m_numEntries = numPackets;
        m_entries = new Entry[numPackets];
        memset( m_entries, 0, sizeof( Entry ) * numPackets );
    }

    NetworkSimulator::~NetworkSimulator()
    {
        assert( m_entries );
        assert( m_numEntries > 0 );
        for ( int i = 0; i < m_numEntries; ++i )
        {
            if ( m_entries[i].packetData )
                delete [] m_entries[i].packetData;
        }
        delete [] m_entries;
        m_entries = NULL;
        m_numEntries = 0;
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

        Entry & entry = m_entries[m_currentIndex];

        if ( entry.packetData )
        {
            delete [] entry.packetData;
            entry = Entry();
        }

        double delay = m_latency / 1000.0;

        delay += random_float( -m_jitter, +m_jitter ) / 1000.0;

        entry.from = from;
        entry.to = to;
        entry.packetData = packetData;
        entry.packetSize = packetSize;
        entry.deliveryTime = m_currentTime + delay;

        m_currentIndex = ( m_currentIndex + 1 ) % m_numEntries;

        if ( random_float( 0.0f, 100.0f ) <= m_duplicates )
        {
            uint8_t *duplicatePacketData = new uint8_t[packetSize];

            memcpy( duplicatePacketData, packetData, packetSize );

            Entry & nextEntry = m_entries[m_currentIndex];

            nextEntry.from = from;
            nextEntry.to = to;
            nextEntry.packetData = duplicatePacketData;
            nextEntry.packetSize = packetSize;
            nextEntry.deliveryTime = m_currentTime + delay + random_float( -1.0, +1.0 );

            m_currentIndex = ( m_currentIndex + 1 ) % m_numEntries;
        }
    }

    uint8_t * NetworkSimulator::ReceivePacket( Address & from, Address & to, int & packetSize )
    { 
        int oldestEntryIndex = -1;
        double oldestEntryTime = 0;

        for ( int i = 0; i < m_numEntries; ++i )
        {
            const Entry & entry = m_entries[i];

            if ( !entry.packetData )
                continue;

            if ( oldestEntryIndex == -1 || m_entries[i].deliveryTime < oldestEntryTime )
            {
                oldestEntryIndex = i;
                oldestEntryTime = entry.deliveryTime;
            }
        }

        Entry & entry = m_entries[oldestEntryIndex];

        if ( oldestEntryIndex == -1 || entry.deliveryTime > m_currentTime )
            return NULL;

        assert( entry.packetData );

        uint8_t *packetData = entry.packetData;

        to = entry.to;
        from = entry.from;
        packetSize = entry.packetSize;

        entry = Entry();

        return packetData;
    }

    void NetworkSimulator::Update( double t )
    {
        m_currentTime = t;
    }
}

#endif // #if YOJIMBO_NETWORK_SIMULATOR
