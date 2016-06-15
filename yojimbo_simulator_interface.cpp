/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.

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

#include "yojimbo_simulator_interface.h"

namespace yojimbo
{
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
