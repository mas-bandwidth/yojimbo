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

#ifndef YOJIMBO_SIMULATOR_H
#define YOJIMBO_SIMULATOR_H

#include "yojimbo_config.h"
#include "yojimbo_common.h"
#include "yojimbo_address.h"
#include "yojimbo_allocator.h"
#include "yojimbo_transport.h"

namespace yojimbo
{
#if YOJIMBO_NETWORK_SIMULATOR

    class NetworkSimulator
    {
    public:

        NetworkSimulator( Allocator & allocator = GetDefaultAllocator(), int numPackets = 4096 );

        ~NetworkSimulator();

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicate( float percent );
        
        void SendPacket( const Address & from, const Address & to, uint8_t * packetData, int packetSize );

        uint8_t * ReceivePacket( Address & from, const Address & to, int & packetSize );

        void DiscardPackets( const Address & address );

        void AdvanceTime( double time );

        Allocator & GetAllocator() { assert( m_allocator ); return *m_allocator; }

    private:

        Allocator * m_allocator;

        float m_latency;                                // latency in milliseconds

        float m_jitter;                                 // jitter in milliseconds +/-

        float m_packetLoss;                             // packet loss percentage

        float m_duplicates;                             // duplicate packet percentage

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
    };

    // todo: rename this to "LocalTransport", and then move functionality for passing packets through simulator into base transport so all transports can use it.

    class SimulatorTransport : public BaseTransport
    {
    public:

        SimulatorTransport( Allocator & allocator,
                            NetworkSimulator & networkSimulator,
                            const Address & address,
                            uint32_t protocolId,
                            int maxPacketSize = 4 * 1024,
                            int sendQueueSize = 1024,
                            int receiveQueueSize = 1024 );

        ~SimulatorTransport();

        void AdvanceTime( double time );

    protected:

        virtual bool InternalSendPacket( const Address & to, const void * packetData, int packetBytes );
    
        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        NetworkSimulator * m_networkSimulator;
    };

#endif // #if YOJIMBO_NETWORK_SIMULATOR
}

#endif // #ifndef YOJIMBO_NETWORK_SIMULATOR_H
