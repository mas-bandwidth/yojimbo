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

#include "yojimbo_network.h"

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
    
#endif

#include <memory.h>
#include <string.h>

namespace yojimbo
{
    static bool s_networkInitialized = false;

    bool InitializeNetwork()     
    {         
        assert( !s_networkInitialized );

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
        WSADATA WsaData;         
        const bool result = WSAStartup( MAKEWORD(2,2), &WsaData ) == NO_ERROR;
#else // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
        const bool result = true;
#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

        if ( result )
            s_networkInitialized = result;   

        return result;
    }

    void ShutdownNetwork()
    {
        assert( s_networkInitialized );

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
        WSACleanup();
#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

        s_networkInitialized = false;
    }

    bool IsNetworkInitialized()
    {
        return s_networkInitialized;
    }

    bool FilterAddress( const Address & address, AddressFilter filter )
    {
        if ( !address.IsValid() )
            return false;

        if ( address.IsLoopback() )
            return false;

        if ( filter == ADDRESS_FILTER_IPV4_ONLY && address.GetType() != ADDRESS_IPV4 )
            return false;

        if ( filter == ADDRESS_FILTER_IPV6_ONLY && address.GetType() != ADDRESS_IPV6 )
            return false;

        if ( address.GetType() == ADDRESS_IPV6 && !address.IsGlobalUnicast() )
            return false;

        return true;
    }

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

    #define WORKING_BUFFER_SIZE 15000
    #define MAX_TRIES 3

    void GetNetworkAddresses( Address * addresses, int & numAddresses, int maxAddresses, AddressFilter filter )
    {
        assert( addresses );
        assert( maxAddresses >= 0 );

        numAddresses = 0;

        DWORD dwRetVal = 0;

        ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;

        ULONG family = AF_UNSPEC;

        PIP_ADAPTER_ADDRESSES pAddresses = NULL;
        ULONG outBufLen = 0;
        ULONG iterations = 0;

        PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

        outBufLen = WORKING_BUFFER_SIZE;

        do
        {
            pAddresses = (IP_ADAPTER_ADDRESSES*) malloc( outBufLen );
            if ( pAddresses == NULL ) 
                return;

            dwRetVal = GetAdaptersAddresses( family, flags, NULL, pAddresses, &outBufLen );

            if ( dwRetVal == ERROR_BUFFER_OVERFLOW ) 
            {
                free( pAddresses );
                pAddresses = NULL;
            } 
            else 
            {
                break;
            }

            iterations++;

        } while ( ( dwRetVal == ERROR_BUFFER_OVERFLOW ) && ( iterations < MAX_TRIES ) );

        if ( dwRetVal == NO_ERROR )
        {
            pCurrAddresses = pAddresses;

            for ( ; pCurrAddresses; pCurrAddresses = pCurrAddresses->Next )
            {
                if ( pCurrAddresses->OperStatus != IfOperStatusUp )
                    continue;

                pUnicast = pCurrAddresses->FirstUnicastAddress;
                if ( pUnicast == NULL )
                    continue;

                for ( ; pUnicast != NULL; pUnicast = pUnicast->Next )
                {
                    if ( numAddresses >= maxAddresses )
                        break;

                    Address address( (sockaddr_storage*) pUnicast->Address.lpSockaddr );

                    if ( !FilterAddress( address, filter ) )
                        continue;

                    addresses[numAddresses++] = address;
                }
            }
        }

        if ( pAddresses)
        {
            free( pAddresses );
        }
    }

#elif YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_MAC || YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

    void GetNetworkAddresses( Address * addresses, int & numAddresses, int maxAddresses, AddressFilter filter )
    {
        assert( addresses );
        assert( maxAddresses >= 0 );

        struct ifaddrs *ifaddr, *ifa;

        numAddresses = 0;

        if ( getifaddrs( &ifaddr ) == -1 )
            return;

        for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next )
        {
            if ( numAddresses >= maxAddresses )
                break;

            if ( ( ifa->ifa_flags & IFF_RUNNING ) == 0 )
                continue;

            if ( ifa->ifa_addr == NULL || ( ifa->ifa_addr->sa_family != AF_INET && ifa->ifa_addr->sa_family != AF_INET6 ) ) 
                continue;

            Address address( (sockaddr_storage*) ifa->ifa_addr );

            if ( !FilterAddress( address, filter ) )
                continue;

            addresses[numAddresses++] = address;
        }

        freeifaddrs( ifaddr );
    }

#else // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

    void GetNetworkAddresses( Address * /*addresses*/, int & numAddresses, int /*maxAddresses*/ )
    {
        numAddresses = 0;
    }

#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
}
