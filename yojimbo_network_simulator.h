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

#ifndef YOJIMBO_NETWORK_SIMULATOR_H
#define YOJIMBO_NETWORK_SIMULATOR_H

#include "yojimbo_config.h"
#include "yojimbo_common.h"
#include "yojimbo_address.h"
#include "yojimbo_allocator.h"
#include "yojimbo_transport.h"

namespace yojimbo
{
    class NetworkSimulator
    {
    public:

        NetworkSimulator( Allocator & allocator, int numPackets = 4096 );

        ~NetworkSimulator();

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicate( float percent );

        bool IsActive() const;
        
        void SendPacket( const Address & from, const Address & to, uint8_t * packetData, int packetSize );

        int ReceivePackets( int maxPackets, uint8_t * packetData[], int packetSize[], Address from[], Address to[] );

        int ReceivePacketsSentToAddress( int maxPackets, const Address & to, uint8_t * packetData[], int packetSize[], Address from[] );

        void DiscardPackets();

        void AdvanceTime( double time );

        Allocator & GetAllocator() { assert( m_allocator ); return *m_allocator; }

    protected:

        void UpdateActive();

        void UpdatePendingReceivePackets();

    private:

        Allocator * m_allocator;

        float m_latency;                                // latency in milliseconds

        float m_jitter;                                 // jitter in milliseconds +/-

        float m_packetLoss;                             // packet loss percentage

        float m_duplicate;                              // duplicate packet percentage

        bool m_active;                                  // true if network simulator is active, eg. if any of the network settings above are enabled.

        struct PacketEntry
        {
            PacketEntry()
            {
                deliveryTime = 0.0;
                packetData = NULL;
                packetSize = 0;
            }

            Address from;                               // address this packet is from
            Address to;                                 // address this packet is sent to
            double deliveryTime;                        // delivery time for this packet
            uint8_t * packetData;                       // packet data (owns pointer)
            int packetSize;                             // size of packet in bytes
        };

        double m_time;                                  // current time from last call to advance time. initially 0.0

        int m_currentIndex;                             // current index in the packet entry array. new packets are inserted here.

        int m_numPacketEntries;                         // number of elements in the packet entry array.

        PacketEntry * m_packetEntries;                  // pointer to dynamically allocated packet entries. this is where buffered packets are stored.

        int m_numPendingReceivePackets;                 // number of pending receive packets.

        PacketEntry * m_pendingReceivePackets;          // list of packets pending receive.

        double m_lastPendingReceiveTime;                // time of last pending receive, used to work around multiple simulator updates from transports.
    };
}

#endif // #ifndef YOJIMBO_NETWORK_SIMULATOR_H
