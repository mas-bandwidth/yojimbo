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

#ifndef YOJIMBO_SIMULATOR_INTERFACE_H
#define YOJIMBO_SIMULATOR_INTERFACE_H

#include "yojimbo_config.h"
#include "yojimbo_network.h"
#include "yojimbo_base_interface.h"
#include "yojimbo_network_simulator.h"

namespace yojimbo
{
    class SimulatorInterface : public BaseInterface
    {
    public:

        SimulatorInterface( Allocator & allocator,
                            NetworkSimulator & networkSimulator,
                            PacketFactory & packetFactory, 
                            const Address & address,
                            uint32_t protocolId,
                            int maxPacketSize = 4 * 1024,
                            int sendQueueSize = 1024,
                            int receiveQueueSize = 1024 );

        ~SimulatorInterface();

        void AdvanceTime( double time );

    protected:

        virtual bool InternalSendPacket( const Address & to, const void * packetData, int packetBytes );
    
        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        NetworkSimulator * m_networkSimulator;
    };
}

#endif // #ifndef YOJIMBO_SOCKET_INTERFACE_H
