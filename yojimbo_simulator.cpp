/*
    Yojimbo Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.
*/

#include "yojimbo_config.h"
#include "yojimbo_simulator.h"
#include "yojimbo_platform.h"

namespace yojimbo
{
    NetworkSimulator::NetworkSimulator( Allocator & allocator, int numPackets )
    {
        assert( numPackets > 0 );
        m_allocator = &allocator;
        m_currentIndex = 0;
        // todo: should probably pass time into ctor
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
            printf( "error: not enough memory to allocate network simulator\n" );
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

    void NetworkSimulator::SendPacket( int to, uint8_t * packetData, int packetBytes )
    {
        assert( m_allocator );
        assert( packetData );
        assert( packetBytes > 0 );

        if ( random_float( 0.0f, 100.0f ) <= m_packetLoss )
        {
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
        packetEntry.packetData = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, packetBytes );
        memcpy( packetEntry.packetData, packetData, packetBytes );
        packetEntry.packetBytes = packetBytes;
        packetEntry.deliveryTime = m_time + delay;
        m_currentIndex = ( m_currentIndex + 1 ) % m_numPacketEntries;

        if ( random_float( 0.0f, 100.0f ) <= m_duplicates )
        {
            PacketEntry & nextPacketEntry = m_packetEntries[m_currentIndex];
            nextPacketEntry.to = to;
            nextPacketEntry.packetData = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, packetBytes );
            memcpy( nextPacketEntry.packetData, packetData, packetBytes );
            nextPacketEntry.packetBytes = packetBytes;
            nextPacketEntry.deliveryTime = m_time + delay + random_float( 0, +1.0 );
            m_currentIndex = ( m_currentIndex + 1 ) % m_numPacketEntries;
        }
    }

    int NetworkSimulator::ReceivePackets( int maxPackets, uint8_t * packetData[], int packetBytes[], int to[] )
    {
        if ( !IsActive() )
            return 0;

        int numPackets = 0;

        for ( int i = 0; i < yojimbo_min( m_numPacketEntries, maxPackets ); ++i )
        {
            if ( !m_packetEntries[i].packetData )
                continue;

            if ( m_packetEntries[i].deliveryTime < m_time )
            {
                packetData[numPackets] = m_packetEntries[i].packetData;
                packetBytes[numPackets] = m_packetEntries[i].packetBytes;
                if ( to )
                {
                    to[numPackets] = m_packetEntries[i].to;
                }
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

    void NetworkSimulator::DiscardClientPackets( int clientIndex )
    {
        for ( int i = 0; i < m_numPacketEntries; ++i )
        {
            PacketEntry & packetEntry = m_packetEntries[i];
            if ( !packetEntry.packetData || packetEntry.to != clientIndex )
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
