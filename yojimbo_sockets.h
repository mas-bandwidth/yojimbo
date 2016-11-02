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

#ifndef YOJIMBO_SOCKETS_H
#define YOJIMBO_SOCKETS_H

#include "yojimbo_config.h"
#include "yojimbo_address.h"
#include "yojimbo_transport.h"

#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace yojimbo
{
#if YOJIMBO_SOCKETS

    enum SocketError
    {
        SOCKET_ERROR_NONE,
        SOCKET_ERROR_CREATE_FAILED,
        SOCKET_ERROR_SET_NON_BLOCKING_FAILED,
        SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED,
        SOCKET_ERROR_SOCKOPT_RCVBUF_FAILED,
        SOCKET_ERROR_SOCKOPT_SNDBUF_FAILED,
        SOCKET_ERROR_BIND_IPV4_FAILED,
        SOCKET_ERROR_BIND_IPV6_FAILED,
        SOCKET_ERROR_GET_SOCKNAME_IPV4_FAILED,
        SOCKET_ERROR_GET_SOCKNAME_IPV6_FAILED
    };

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
    typedef uint64_t SocketHandle;
#else // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
    typedef int SocketHandle;
#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
                           
    class Socket
    {
    public:

        explicit Socket( const Address & address, int bufferSize = 1024*1024 );

        ~Socket();

        bool IsError() const;

        int GetError() const;

        bool SendPacket( const Address & to, const void * packetData, size_t packetBytes );
    
        int ReceivePacket( Address & from, void * packetData, int maxPacketSize );

        const Address & GetAddress() const;

    private:

        int m_error;
        Address m_address;
        SocketHandle m_socket;
    };

    class SocketTransport : public BaseTransport
    {
    public:

        SocketTransport( Allocator & allocator,
                         const Address & address,
                         uint32_t protocolId,
                         int maxPacketSize = 4 * 1024,
                         int sendQueueSize = 1024,
                         int receiveQueueSize = 1024,
                         int bufferSize = 1024*1024 );

        ~SocketTransport();

        bool IsError() const;

        int GetError() const;

        const Address & GetAddress() const;

    protected:

        virtual bool InternalSendPacket( const Address & to, const void * packetData, int packetBytes );
    
        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        Socket * m_socket;
    };

#endif // #if YOJIMBO_SOCKETS
}

#endif // #ifndef YOJIMBO_SOCKETS_H
