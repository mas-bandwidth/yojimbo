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

#include "yojimbo_address.h"

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
    Address::Address()
    {
        Clear();
    }

    Address::Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port )
        : m_type( ADDRESS_IPV4 )
    {
        
        m_address_ipv4 = uint32_t(a) | (uint32_t(b)<<8) | (uint32_t(c)<<16) | (uint32_t(d)<<24);
        m_port = port;
    }

    Address::Address( uint32_t address, int16_t port )
        : m_type( ADDRESS_IPV4 )
    {
        m_address_ipv4 = htonl( address );        // IMPORTANT: stored in network byte order. eg. big endian!
        m_port = port;
    }

    Address::Address( uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                      uint16_t e, uint16_t f, uint16_t g, uint16_t h,
                      uint16_t port )
        : m_type( ADDRESS_IPV6 )
    {
        m_address_ipv6[0] = htons( a );
        m_address_ipv6[1] = htons( b );
        m_address_ipv6[2] = htons( c );
        m_address_ipv6[3] = htons( d );
        m_address_ipv6[4] = htons( e );
        m_address_ipv6[5] = htons( f );
        m_address_ipv6[6] = htons( g );
        m_address_ipv6[7] = htons( h );
        m_port = port;
    }

    Address::Address( const uint16_t address[], uint16_t port )
        : m_type( ADDRESS_IPV6 )
    {
        for ( int i = 0; i < 8; ++i )
            m_address_ipv6[i] = htons( address[i] );
        m_port = port;
    }

    Address::Address( const sockaddr_storage * addr )
    {
        assert( addr );

        if ( addr->ss_family == AF_INET )
        {
            const sockaddr_in * addr_ipv4 = (const sockaddr_in*) addr;
            m_type = ADDRESS_IPV4;
            m_address_ipv4 = addr_ipv4->sin_addr.s_addr;
            m_port = ntohs( addr_ipv4->sin_port );
        }
        else if ( addr->ss_family == AF_INET6 )
        {
            const sockaddr_in6 * addr_ipv6 = (const sockaddr_in6*) addr;
            m_type = ADDRESS_IPV6;
            memcpy( m_address_ipv6, &addr_ipv6->sin6_addr, 16 );
            m_port = ntohs( addr_ipv6->sin6_port );
        }
        else
        {
            assert( false );
            Clear();
        }
    }

    Address::Address( const char * address )
    {
        Parse( address );
    }

    Address::Address( const char * address, uint16_t port )
    {
        Parse( address );
        m_port = port;
    }

    void Address::Parse( const char * address_in )
    {
        // first try to parse as an IPv6 address:
        // 1. if the first character is '[' then it's probably an ipv6 in form "[addr6]:portnum"
        // 2. otherwise try to parse as raw IPv6 address, parse using inet_pton

        assert( address_in );

        char buffer[256];
        char * address = buffer;
        strncpy( address, address_in, 255 );
        address[255] = '\0';

        int addressLength = (int) strlen( address );
        m_port = 0;
        if ( address[0] == '[' )
        {
            const int base_index = addressLength - 1;
            for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
            {
                const int index = base_index - i;
                if ( index < 3 )
                    break;
                if ( address[index] == ':' )
                {
                    m_port = uint16_t( atoi( &address[index + 1] ) );
                    address[index-1] = '\0';
                }
            }
            address += 1;
        }
        struct in6_addr sockaddr6;
        if ( inet_pton( AF_INET6, address, &sockaddr6 ) == 1 )
        {
            memcpy( m_address_ipv6, &sockaddr6, 16 );
            m_type = ADDRESS_IPV6;
            return;
        }

        // otherwise it's probably an IPv4 address:
        // 1. look for ":portnum", if found save the portnum and strip it out
        // 2. parse remaining ipv4 address via inet_pton

        addressLength = (int) strlen( address );
        const int base_index = addressLength - 1;
        for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
        {
            const int index = base_index - i;
            if ( index < 0 )
                break;
            if ( address[index] == ':' )
            {
                m_port = (uint16_t) atoi( &address[index+1] );
                address[index] = '\0';
            }
        }

        struct sockaddr_in sockaddr4;
        if ( inet_pton( AF_INET, address, &sockaddr4.sin_addr ) == 1 )
        {
            m_type = ADDRESS_IPV4;
            m_address_ipv4 = sockaddr4.sin_addr.s_addr;
        }
        else
        {
            // nope: it's not an IPv4 address. maybe it's a hostname? set address as invalid.
            Clear();
        }
    }

    void Address::Clear()
    {
        m_type = ADDRESS_NONE;
        memset( m_address_ipv6, 0, sizeof( m_address_ipv6 ) );
        m_port = 0;
    }

    uint32_t Address::GetAddress4() const
    {
        assert( m_type == ADDRESS_IPV4 );
        return m_address_ipv4;
    }

    const uint16_t * Address::GetAddress6() const
    {
        assert( m_type == ADDRESS_IPV6 );
        return m_address_ipv6;
    }

    void Address::SetPort( uint16_t port )
    {
        m_port = port;
    }

    uint16_t Address::GetPort() const 
    {
        return m_port;
    }

    AddressType Address::GetType() const
    {
        return m_type;
    }

    const char * Address::ToString( char buffer[], int bufferSize ) const
    {
        if ( m_type == ADDRESS_IPV4 )
        {
            const uint8_t a =   m_address_ipv4 & 0xff;
            const uint8_t b = ( m_address_ipv4 >> 8  ) & 0xff;
            const uint8_t c = ( m_address_ipv4 >> 16 ) & 0xff;
            const uint8_t d = ( m_address_ipv4 >> 24 ) & 0xff;
            if ( m_port != 0 )
                snprintf( buffer, bufferSize, "%d.%d.%d.%d:%d", a, b, c, d, m_port );
            else
                snprintf( buffer, bufferSize, "%d.%d.%d.%d", a, b, c, d );
            return buffer;
        }
        else if ( m_type == ADDRESS_IPV6 )
        {
            if ( m_port == 0 )
            {
                inet_ntop( AF_INET6, (void*) &m_address_ipv6, buffer, bufferSize );
                return buffer;
            }
            else
            {
                char addressString[INET6_ADDRSTRLEN];
                inet_ntop( AF_INET6, (void*) &m_address_ipv6, addressString, INET6_ADDRSTRLEN );
                snprintf( buffer, bufferSize, "[%s]:%d", addressString, m_port );
                return buffer;
            }
        }
        else
        {
            snprintf( buffer, bufferSize, "%s", "NONE" );
			return buffer;
        }
    }

    bool Address::IsValid() const
    {
        return m_type != ADDRESS_NONE;
    }

    bool Address::IsLinkLocal() const
    {
        return m_type == ADDRESS_IPV6 && m_address_ipv6[0] == htons( 0xfe80 );
    }

    bool Address::IsSiteLocal() const
    {
        return m_type == ADDRESS_IPV6 && m_address_ipv6[0] == htons( 0xfec0 );
    }

    bool Address::IsMulticast() const
    {
        return m_type == ADDRESS_IPV6 && m_address_ipv6[0] == htons( 0xff00 );
    }

	bool Address::IsLoopback() const
	{
		return ( m_type == ADDRESS_IPV4 && m_address_ipv4 == htonl( 0x7F000001 ) ) 
											||
			   ( m_type == ADDRESS_IPV6 && m_address_ipv6[0] == 0
				  				        && m_address_ipv6[1] == 0
									    && m_address_ipv6[2] == 0
									    && m_address_ipv6[3] == 0
									    && m_address_ipv6[4] == 0
									    && m_address_ipv6[5] == 0
									    && m_address_ipv6[6] == 0
									    && m_address_ipv6[7] == htons( 0x0001 ) );
	}

    bool Address::IsGlobalUnicast() const
    {
        return m_type == ADDRESS_IPV6 && m_address_ipv6[0] != htons( 0xfe80 )
                                      && m_address_ipv6[0] != htons( 0xfec0 )
                                      && m_address_ipv6[0] != htons( 0xff00 )
								      && !IsLoopback();
    }

    bool Address::operator ==( const Address & other ) const
    {
        if ( m_type != other.m_type )
            return false;
        if ( m_port != other.m_port )
            return false;
        if ( m_type == ADDRESS_IPV4 && m_address_ipv4 == other.m_address_ipv4 )
            return true;
        else if ( m_type == ADDRESS_IPV6 && memcmp( m_address_ipv6, other.m_address_ipv6, sizeof( m_address_ipv6 ) ) == 0 )
            return true;
        else
            return false;
    }

    bool Address::operator !=( const Address & other ) const
    {
        return !( *this == other );
    }
}
