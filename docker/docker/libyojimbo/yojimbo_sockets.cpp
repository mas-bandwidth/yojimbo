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

#include "yojimbo_sockets.h"

#if YOJIMBO_SOCKETS

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

    #define NOMINMAX
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <ws2ipdef.h>
    #pragma comment( lib, "WS2_32.lib" )

    #ifdef SetPort
    #undef SetPort
    #endif // #ifdef SetPort

	#if YOJIMBO_SOCKETS
	#include <iphlpapi.h>
	#pragma comment( lib, "IPHLPAPI.lib" )
	#endif // #if YOJIMBO_SOCKETS

#elif YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_MAC || YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX

    #include <netdb.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    
#else

    #error yojimbo unknown platform!

#endif

#include <memory.h>
#include <string.h>

namespace yojimbo
{
	Socket::Socket( const Address & address )
    {
        assert( address.IsValid() );
        assert( IsNetworkInitialized() );

        m_error = SOCKET_ERROR_NONE;

        // create socket

        m_socket = socket( ( address.GetType() == ADDRESS_IPV6 ) ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP );

        if ( m_socket <= 0 )
        {
            m_error = SOCKET_ERROR_CREATE_FAILED;
            return;
        }

        // force IPv6 only if necessary

        if ( address.GetType() == ADDRESS_IPV6 )
        {
            int yes = 1;
            if ( setsockopt( m_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&yes, sizeof(yes) ) != 0 )
            {
                m_error = SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED;
                return;
            }
        }

        // bind to port

        if ( address.GetType() == ADDRESS_IPV6 )
        {
            sockaddr_in6 sock_address;
            memset( &sock_address, 0, sizeof( sockaddr_in6 ) );
            sock_address.sin6_family = AF_INET6;
            memcpy( &sock_address.sin6_addr, address.GetAddress6(), sizeof( sock_address.sin6_addr ) );
            sock_address.sin6_port = htons( address.GetPort() );

            if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
            {
                m_socket = SOCKET_ERROR_BIND_IPV6_FAILED;
                return;
            }
        }
        else
        {
            sockaddr_in sock_address;
            sock_address.sin_family = AF_INET;
            sock_address.sin_addr.s_addr = address.GetAddress4();
            sock_address.sin_port = htons( address.GetPort() );

            if ( ::bind( m_socket, (const sockaddr*) &sock_address, sizeof(sock_address) ) < 0 )
            {
                m_error = SOCKET_ERROR_BIND_IPV4_FAILED;
                return;
            }
        }

        m_address = address;

        // if bound to port 0 find the actual port we got

        if ( address.GetPort() == 0 )
        {
            if ( address.GetType() == ADDRESS_IPV6 )
            {
                struct sockaddr_in6 sin;
                socklen_t len = sizeof( sin );
                if ( getsockname( m_socket, (struct sockaddr*)&sin, &len ) == -1 )
                {
                    m_error = SOCKET_ERROR_GET_SOCKNAME_IPV6_FAILED;
                    return;
                }
                m_address.SetPort( sin.sin6_port );
            }
            else
            {
                struct sockaddr_in sin;
                socklen_t len = sizeof( sin );
                if ( getsockname( m_socket, (struct sockaddr*)&sin, &len ) == -1 )
                {
                    m_error = SOCKET_ERROR_GET_SOCKNAME_IPV4_FAILED;
                    return;
                }
                m_address.SetPort( sin.sin_port );
            }
        }

        // set non-blocking io

        #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_MAC || YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX
    
            int nonBlocking = 1;
            if ( fcntl( m_socket, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
            {
                m_error = SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
                return;
            }
        
        #elif YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
    
            DWORD nonBlocking = 1;
            if ( ioctlsocket( m_socket, FIONBIO, &nonBlocking ) != 0 )
            {
                m_error = SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
                return;
            }

        #else

            #error unsupported platform

        #endif
    }

    Socket::~Socket()
    {
        if ( m_socket != 0 )
        {
            #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_MAC || YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX
            close( m_socket );
            #elif YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
            closesocket( m_socket );
            #else
            #error unsupported platform
            #endif
            m_socket = 0;
        }
    }

    bool Socket::IsError() const
    {
        return m_error != SOCKET_ERROR_NONE;
    }

    int Socket::GetError() const
    {
        return m_error;
    }

    bool Socket::SendPacket( const Address & to, const void * packetData, size_t packetBytes )
    {
        assert( packetData );
        assert( packetBytes > 0 );
        assert( to.IsValid() );
        assert( m_socket );
        assert( !IsError() );

        bool result = false;

        if ( to.GetType() == ADDRESS_IPV6 )
        {
            sockaddr_in6 socket_address;
            memset( &socket_address, 0, sizeof( socket_address ) );
            socket_address.sin6_family = AF_INET6;
            socket_address.sin6_port = htons( to.GetPort() );
            memcpy( &socket_address.sin6_addr, to.GetAddress6(), sizeof( socket_address.sin6_addr ) );
            size_t sent_bytes = sendto( m_socket, (const char*)packetData, (int) packetBytes, 0, (sockaddr*)&socket_address, sizeof( sockaddr_in6 ) );
            result = sent_bytes == packetBytes;
        }
        else if ( to.GetType() == ADDRESS_IPV4 )
        {
            sockaddr_in socket_address;
            memset( &socket_address, 0, sizeof( socket_address ) );
            socket_address.sin_family = AF_INET;
            socket_address.sin_addr.s_addr = to.GetAddress4();
            socket_address.sin_port = htons( (unsigned short) to.GetPort() );
            size_t sent_bytes = sendto( m_socket, (const char*)packetData, (int) packetBytes, 0, (sockaddr*)&socket_address, sizeof(sockaddr_in) );
            result = sent_bytes == packetBytes;
        }

        return result;
    }

    int Socket::ReceivePacket( Address & from, void * packetData, int maxPacketSize )
    {
        assert( m_socket );
        assert( packetData );
        assert( maxPacketSize > 0 );

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
        typedef int socklen_t;
#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
        
        sockaddr_storage sockaddr_from;
        socklen_t fromLength = sizeof( sockaddr_from );

        int result = recvfrom( m_socket, (char*)packetData, maxPacketSize, 0, (sockaddr*)&sockaddr_from, &fromLength );

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
        if ( result == SOCKET_ERROR )
        {
            int error = WSAGetLastError();

            if ( error == WSAEWOULDBLOCK )
                return 0;

            return 0;
        }
#else // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
        if ( result <= 0 )
        {
            if ( errno == EAGAIN )
                return 0;

            return 0;
        }
#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

        from = Address( &sockaddr_from );

        assert( result >= 0 );

        const int bytesRead = result;

        return bytesRead;
    }

    const Address & Socket::GetAddress() const
    {
        return m_address;
    }

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
#endif // #if YOJIMBO_SOCKETS
}
