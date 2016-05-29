/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#ifndef YOJIMBO_NETWORK_SIMULATOR_H
#define YOJIMBO_NETWORK_SIMULATOR_H

#include "yojimbo_config.h"
#include "yojimbo_common.h"
#include "yojimbo_network.h"

namespace yojimbo
{
#if YOJIMBO_NETWORK_SIMULATOR

    class NetworkSimulator
    {
        float m_latency;                                // latency in milliseconds
        float m_jitter;                                 // jitter in milliseconds +/-
        float m_packetLoss;                             // packet loss percentage
        float m_duplicates;                             // duplicate packet percentage

        int m_numEntries;                               // number of elements in the packet entry array.
        int m_currentIndex;                             // current index in the packet entry array. new packets are inserted here.

        struct Entry
        {
            Entry()
            {
                deliveryTime = 0.0;
                packetData = NULL;
                packetSize = 0;
            }

            Address from;                               // address this packet is from
            Address to;                                 // address this packet is sent to
            double deliveryTime;                        // delivery time for this packet
            uint8_t *packetData;                        // packet data (owns pointer)
            int packetSize;                             // size of packet in bytes
        };

        Entry * m_entries;                              // pointer to dynamically allocated packet entries. this is where buffered packets are stored.

        double m_currentTime;                           // current time from last call to update. initially 0.0

    public:

        NetworkSimulator( int numPackets = 1024 );

        ~NetworkSimulator();

        void SetLatency( float milliseconds );
        void SetJitter( float milliseconds );
        void SetPacketLoss( float percent );
        void SetDuplicates( float percent );
        
        void SendPacket( const Address & from, const Address & to, uint8_t * packetData, int packetSize );

        uint8_t * ReceivePacket( Address & from, Address & to, int & packetSize );

        void Update( double t );
    };

#endif // #if YOJIMBO_NETWORK_SIMULATOR
}

#endif // #ifndef YOJIMBO_NETWORK_SIMULATOR_H
