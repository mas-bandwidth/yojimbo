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

#include "yojimbo_socket_interface.h"

namespace yojimbo
{
    SocketInterface::SocketInterface( Allocator & allocator, 
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
        m_socket = YOJIMBO_NEW( allocator, Socket, address );
    }

    SocketInterface::~SocketInterface()
    {
        YOJIMBO_DELETE( GetAllocator(), Socket, m_socket );
    }

    bool SocketInterface::IsError() const
    {
        return m_socket->IsError();
    }

    int SocketInterface::GetError() const
    {
        return m_socket->GetError();
    }

    bool SocketInterface::InternalSendPacket( const Address & to, const void * packetData, int packetBytes )
    {
        return m_socket->SendPacket( to, packetData, packetBytes );
    }

    int SocketInterface::InternalReceivePacket( Address & from, void * packetData, int maxPacketSize )
    {
        return m_socket->ReceivePacket( from, packetData, maxPacketSize );
    }
}
