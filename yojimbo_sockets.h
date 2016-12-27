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

    /// Socket error values.

    enum SocketError
    {
        SOCKET_ERROR_NONE,                                                  ///< No socket error. All is well!
        SOCKET_ERROR_CREATE_FAILED,                                         ///< Create socket failed.
        SOCKET_ERROR_SET_NON_BLOCKING_FAILED,                               ///< Setting the socket as non-blocking failed.
        SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED,                              ///< Setting the socket as IPv6 only failed.
        SOCKET_ERROR_SOCKOPT_RCVBUF_FAILED,                                 ///< Setting the socket receive buffer size failed.
        SOCKET_ERROR_SOCKOPT_SNDBUF_FAILED,                                 ///< Setting the socket send buffer size failed.
        SOCKET_ERROR_BIND_IPV4_FAILED,                                      ///< Failed to bind the socket (IPv4).
        SOCKET_ERROR_BIND_IPV6_FAILED,                                      ///< Failed to bind the socket (IPv6).
        SOCKET_ERROR_GET_SOCKNAME_IPV4_FAILED,                              ///< Call to getsockname failed on the socket (IPv4).
        SOCKET_ERROR_GET_SOCKNAME_IPV6_FAILED                               ///< Call to getsockname failed on the socket (IPv6).
    };

    /// Platfrom independent socket handle.

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
    typedef uint64_t SocketHandle;
#else // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
    typedef int SocketHandle;
#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

    /**
         Simple wrapper around a non-blocking UDP socket.

         IMPORTANT: You must initialize the network layer before creating any sockets.

         @see InitializeNetwork
     */
                           
    class Socket
    {
    public:

        /**
            Creates a socket.

            Please check Socket::IsError after creating a socket, as it could be in an error state.

            IMPORTANT: You must initialize the network layer before creating sockets. See yojimbo::InitializeNetwork.

            @param address The address to bind the socket to.
            @param sendBufferSize The size of the send buffer to set on the socket (SO_SNDBUF).
            @param receiveBufferSize The size of the receive buffer to set on the socket (SO_RCVBUF).
         */

        explicit Socket( const Address & address, int sendBufferSize = 1024*1024, int receiveBufferSize = 1024*1024 );

        /**
            Socket destructor.
         */

        ~Socket();

        /**
            Is the socket in an error state?

            Corresponds to the socket error state being something other than yojimbo::SOCKET_ERROR_NONE.

            @returns True if the socket is in an error state, false otherwise.
         */

        bool IsError() const;

        /**
            Get the socket error state.

            These error states correspond to things that can go wrong in the socket constructor when it's created.

            IMPORTANT: Please make sure you always check for error state after you create a socket.

            @returns The socket error state.

            @see Socket::IsError
         */

        SocketError GetError() const;

        /**
            Send a packet to an address using this socket.

            The packet is sent unreliably over UDP. It may arrive out of order, in duplicate or not at all.

            @param to The address to send the packet to.
            @param packetData The packet data to send.
            @param packetBytes The size of the packet data to send (bytes).
         */

        void SendPacket( const Address & to, const void * packetData, size_t packetBytes );
    
        /**
            Receive a packet from the network (non-blocking).

            @param from The address that sent the packet [out]
            @param packetData The buffer where the packet data will be copied to. Must be at least maxPacketSize large in bytes.
            @param maxPacketSize The maximum packet size to read in bytes. Any packets received larger than this are discarded.

            @returns The size of the packet received in [1,maxPacketSize], or 0 if no packet is available to read.
         */

        int ReceivePacket( Address & from, void * packetData, int maxPacketSize );

        /**
            Get the socket address including the dynamically assigned port # for sockets bound to port 0.

            @returns The socket address. Port is guaranteed to be non-zero, provided the socket is not in an error state.
         */

        const Address & GetAddress() const;

    private:

        SocketError m_error;										///< The socket error level.

        Address m_address;                                          ///< The address the socket is bound on. If the socket was bound to 0, the port number is resolved to the actual port number assigned by the system.
        
        SocketHandle m_socket;                                      ///< The socket handle in a platform independent form.
    };

#endif // #if YOJIMBO_SOCKETS
}

#endif // #ifndef YOJIMBO_SOCKETS_H
