/*
    netcode

    Copyright © 2017 - 2026, Más Bandwidth LLC

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

#include "netcode.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <time.h>

#ifdef _MSC_VER
#define SODIUM_STATIC
#pragma warning(disable:4996)
#endif // #ifdef _MSC_VER

#include <sodium.h>

#define NETCODE_CONNECT_TOKEN_NONCE_BYTES 24
#define NETCODE_CONNECT_TOKEN_PRIVATE_BYTES 1024
#define NETCODE_CONNECT_SALT_BYTES 8
#define NETCODE_SERVER_NONCE_BYTES 8
#define NETCODE_CHALLENGE_TOKEN_BYTES 300
#define NETCODE_VERSION_INFO_BYTES 13
#define NETCODE_MAX_PACKET_BYTES 1300
#define NETCODE_MAX_PAYLOAD_BYTES 1200
#define NETCODE_PACKET_QUEUE_SIZE 256
#define NETCODE_REPLAY_PROTECTION_BUFFER_SIZE 256
#define NETCODE_CLIENT_MAX_RECEIVE_PACKETS 64
#define NETCODE_SERVER_MAX_RECEIVE_PACKETS ( 64 * NETCODE_MAX_CLIENTS )
#define NETCODE_CLIENT_SOCKET_SNDBUF_SIZE ( 4 * 1024 * 1024 )
#define NETCODE_CLIENT_SOCKET_RCVBUF_SIZE ( 4 * 1024 * 1024 )
#define NETCODE_SERVER_SOCKET_SNDBUF_SIZE ( 4 * 1024 * 1024 )
#define NETCODE_SERVER_SOCKET_RCVBUF_SIZE ( 4 * 1024 * 1024 )

#define NETCODE_VERSION_INFO ( (uint8_t*) "NETCODE 1.03" )
#define NETCODE_PACKET_SEND_RATE 10.0
#define NETCODE_NUM_DISCONNECT_PACKETS 10

#ifndef NETCODE_ENABLE_TESTS
#define NETCODE_ENABLE_TESTS 0
#endif // #ifndef NETCODE_ENABLE_TESTS

#ifndef NETCODE_ENABLE_LOGGING
#define NETCODE_ENABLE_LOGGING 1
#endif // #ifndef NETCODE_ENABLE_LOGGING

#if NETCODE_ENABLE_TESTS

static int netcode_test_real_socket_count;
static int netcode_test_real_sleep_count;
static int netcode_test_real_time_count;
static int netcode_test_fake_time_enabled;
static uint64_t netcode_test_fake_time_value;

#endif // #if NETCODE_ENABLE_TESTS

static uint64_t netcode_current_timestamp()
{
#if NETCODE_ENABLE_TESTS
    if ( netcode_test_fake_time_enabled )
        return netcode_test_fake_time_value;
    netcode_test_real_time_count++;
#endif // #if NETCODE_ENABLE_TESTS
    return (uint64_t) time( NULL );
}

// ------------------------------------------------------------------

#if NETCODE_PACKET_TAGGING

static int netcode_packet_tagging_enabled = 0;

void netcode_enable_packet_tagging()
{
    netcode_packet_tagging_enabled = 1;
}

#else

void netcode_enable_packet_tagging() {}

#endif // #if NETCODE_PACKET_TAGGING

// ------------------------------------------------------------------

static void netcode_default_assert_handler( NETCODE_CONST char * condition, NETCODE_CONST char * function, NETCODE_CONST char * file, int line )
{
    printf( "assert failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
    #if defined( __GNUC__ )
    __builtin_trap();
    #elif defined( _MSC_VER )
    __debugbreak();
    #endif
    exit( 1 );
}

static int log_level;
static int (*printf_function)( NETCODE_CONST char *, ... ) = ( int (*)( NETCODE_CONST char *, ... ) ) printf;
void (*netcode_assert_function)( NETCODE_CONST char *, NETCODE_CONST char *, NETCODE_CONST char * file, int line ) = netcode_default_assert_handler;

void netcode_log_level( int level )
{
    log_level = level;
}

void netcode_set_printf_function( int (*function)( NETCODE_CONST char *, ... ) )
{
    netcode_assert( function );
    printf_function = function;
}

void netcode_set_assert_function( void (*function)( NETCODE_CONST char *, NETCODE_CONST char *, NETCODE_CONST char * file, int line ) )
{
    netcode_assert_function = function;
}

#if NETCODE_ENABLE_LOGGING

void netcode_printf( int level, NETCODE_CONST char * format, ... ) 
{
    if ( level > log_level )
        return;
    va_list args;
    va_start( args, format );
    char buffer[4*1024];
    vsnprintf( buffer, sizeof(buffer), format, args );
    printf_function( "%s", buffer );
    va_end( args );
}

#else // #if NETCODE_ENABLE_LOGGING

void netcode_printf( int level, NETCODE_CONST char * format, ... ) 
{
    (void) level;
    (void) format;
}

#endif // #if NETCODE_ENABLE_LOGGING

void * netcode_default_allocate_function( void * context, size_t bytes )
{
    (void) context;
    return malloc( bytes );
}

void netcode_default_free_function( void * context, void * pointer )
{
    (void) context;
    free( pointer );
}

// ------------------------------------------------------------------

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    #ifndef NOMINMAX
    #define NOMINMAX
    #endif // #ifndef NOMINMAX
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2def.h>
    #include <ws2tcpip.h>
    #include <ws2ipdef.h>
    #include <iphlpapi.h>
    #ifdef _MSC_VER
    #pragma comment( lib, "WS2_32.lib" )
    #pragma comment( lib, "IPHLPAPI.lib" )
    #endif // #ifdef _MSC_VER

    #ifdef SetPort
    #undef SetPort
    #endif // #ifdef SetPort

    #include <iphlpapi.h>
    #ifdef _MSC_VER
    #pragma comment( lib, "IPHLPAPI.lib" )
    #endif // #ifdef _MSC_VER
    
#elif NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX

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

    #error netcode - unknown platform!

#endif

static int netcode_parse_port( NETCODE_CONST char * string, uint16_t * port )
{
    // the port must be all digits and fit in [0,65535]. anything else is an error,
    // rather than whatever atoi truncation used to produce.

    if ( string[0] == '\0' )
        return NETCODE_ERROR;

    int value = 0;
    int i;
    for ( i = 0; string[i] != '\0'; i++ )
    {
        if ( string[i] < '0' || string[i] > '9' )
            return NETCODE_ERROR;
        value = value * 10 + ( string[i] - '0' );
        if ( value > 65535 )
            return NETCODE_ERROR;
    }

    *port = (uint16_t) value;

    return NETCODE_OK;
}

int netcode_parse_address( NETCODE_CONST char * address_string_in, struct netcode_address_t * address )
{
    netcode_assert( address_string_in );
    netcode_assert( address );

    memset( address, 0, sizeof( struct netcode_address_t ) );

    // first try to parse the string as an IPv6 address:
    // 1. if the first character is '[' then it's probably an ipv6 in form "[addr6]:portnum"
    // 2. otherwise try to parse as a raw IPv6 address using inet_pton

    char buffer[NETCODE_MAX_ADDRESS_STRING_LENGTH];

    char * address_string = buffer;
    strncpy( address_string, address_string_in, NETCODE_MAX_ADDRESS_STRING_LENGTH - 1 );
    address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH-1] = '\0';

    int address_string_length = (int) strlen( address_string );

    if ( address_string[0] == '[' )
    {
        int base_index = address_string_length - 1;

        int i;
        for ( i = 0; i < 6; i++ )         // note: no need to search past 6 characters as ":65535" is longest possible port value
        {
            int index = base_index - i;
            if ( index < 3 )
                break;
            if ( address_string[index] == ':' && address_string[index-1] == ']' )
            {
                if ( netcode_parse_port( &address_string[index+1], &address->port ) != NETCODE_OK )
                    return NETCODE_ERROR;
                address_string[index-1] = '\0';
                break;
            }
        }

        // if a port is omitted, it is assumed to be zero. strip the trailing ']' so "[addr]" parses as just the address

        if ( address_string[base_index] == ']' )
            address_string[base_index] = '\0';

        address_string += 1;
    }

    struct in6_addr sockaddr6;
    if ( inet_pton( AF_INET6, address_string, &sockaddr6 ) == 1 )
    {
        address->type = NETCODE_ADDRESS_IPV6;
        uint16_t ipv6_network_order[8];
        memcpy( ipv6_network_order, &sockaddr6, sizeof( ipv6_network_order ) );
        int i;
        for ( i = 0; i < 8; i++ )
        {
            address->data.ipv6[i] = ntohs( ipv6_network_order[i] );
        }
        return NETCODE_OK;
    }

    // otherwise it's probably an IPv4 address:
    // 1. look for ":portnum", if found save the portnum and strip it out
    // 2. parse remaining ipv4 address via inet_pton

    address_string_length = (int) strlen( address_string );
    int base_index = address_string_length - 1;
    int i;
    for ( i = 0; i < 6; i++ )
    {
        int index = base_index - i;
        if ( index < 0 )
            break;
        if ( address_string[index] == ':' )
        {
            if ( netcode_parse_port( &address_string[index+1], &address->port ) != NETCODE_OK )
                return NETCODE_ERROR;
            address_string[index] = '\0';
            break;
        }
    }

    struct sockaddr_in sockaddr4;
    if ( inet_pton( AF_INET, address_string, &sockaddr4.sin_addr ) == 1 )
    {
        address->type = NETCODE_ADDRESS_IPV4;
        uint32_t ipv4_host_order = ntohl( sockaddr4.sin_addr.s_addr );
        address->data.ipv4[0] = (uint8_t) ( ( ipv4_host_order >> 24 ) & 0xFF );
        address->data.ipv4[1] = (uint8_t) ( ( ipv4_host_order >> 16 ) & 0xFF );
        address->data.ipv4[2] = (uint8_t) ( ( ipv4_host_order >> 8  ) & 0xFF );
        address->data.ipv4[3] = (uint8_t) ( ( ipv4_host_order       ) & 0xFF );
        return NETCODE_OK;
    }

    return NETCODE_ERROR;
}

char * netcode_address_to_string( struct netcode_address_t * address, char * buffer )
{
    netcode_assert( address );
    netcode_assert( buffer );

    if ( address->type == NETCODE_ADDRESS_IPV6 )
    {
        if ( address->port == 0 )
        {
            uint16_t ipv6_network_order[8];
            int i;
            for ( i = 0; i < 8; i++ )
                ipv6_network_order[i] = htons( address->data.ipv6[i] );
            struct in6_addr sockaddr6;
            memcpy( &sockaddr6, ipv6_network_order, sizeof( ipv6_network_order ) );
            inet_ntop( AF_INET6, &sockaddr6, buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH );
            return buffer;
        }
        else
        {
            char address_string[INET6_ADDRSTRLEN];
            uint16_t ipv6_network_order[8];
            int i;
            for ( i = 0; i < 8; i++ )
                ipv6_network_order[i] = htons( address->data.ipv6[i] );
            struct in6_addr sockaddr6;
            memcpy( &sockaddr6, ipv6_network_order, sizeof( ipv6_network_order ) );
            inet_ntop( AF_INET6, &sockaddr6, address_string, INET6_ADDRSTRLEN );
            snprintf( buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH, "[%s]:%d", address_string, address->port );
            return buffer;
        }
    }
    else if ( address->type == NETCODE_ADDRESS_IPV4 )
    {
        if ( address->port != 0 )
        {
            snprintf( buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH, "%d.%d.%d.%d:%d", 
                address->data.ipv4[0], 
                address->data.ipv4[1], 
                address->data.ipv4[2], 
                address->data.ipv4[3], 
                address->port );
        }
        else
        {
            snprintf( buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH, "%d.%d.%d.%d", 
                address->data.ipv4[0], 
                address->data.ipv4[1], 
                address->data.ipv4[2], 
                address->data.ipv4[3] );
        }
        return buffer;
    }
    else
    {
        snprintf( buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH, "%s", "NONE" );
        return buffer;
    }
}

int netcode_address_equal( struct netcode_address_t * a, struct netcode_address_t * b )
{
    netcode_assert( a );
    netcode_assert( b );

    if ( a->type != b->type )
        return 0;

    if ( a->port != b->port )
        return 0;

    if ( a->type == NETCODE_ADDRESS_IPV4 )
    {
        int i;
        for ( i = 0; i < 4; i++ )
        {
            if ( a->data.ipv4[i] != b->data.ipv4[i] )
                return 0;
        }
    }
    else if ( a->type == NETCODE_ADDRESS_IPV6 )
    {
        int i;
        for ( i = 0; i < 8; i++ )
        {
            if ( a->data.ipv6[i] != b->data.ipv6[i] )
                return 0;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

// ----------------------------------------------------------------

struct netcode_t
{
    int initialized;
};

static struct netcode_t netcode;

int netcode_init()
{
    // reference counted so multiple subsystems in the same application can call
    // netcode_init and netcode_term independently

    if ( netcode.initialized )
    {
        netcode.initialized++;
        return NETCODE_OK;
    }

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    WSADATA WsaData;
    if ( WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR )
        return NETCODE_ERROR;
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    if ( sodium_init() == -1 )
        return NETCODE_ERROR;

    netcode.initialized = 1;

    return NETCODE_OK;
}

void netcode_term()
{
    netcode_assert( netcode.initialized );

    if ( !netcode.initialized )
        return;

    netcode.initialized--;

    if ( netcode.initialized )
        return;

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    WSACleanup();
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
}

// ----------------------------------------------------------------

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
typedef SOCKET netcode_socket_handle_t;
#else // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
typedef int netcode_socket_handle_t;
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

// -1 on posix (descriptor 0 is a valid socket), INVALID_SOCKET on windows

#define NETCODE_SOCKET_HANDLE_INVALID ( (netcode_socket_handle_t) -1 )

struct netcode_socket_t
{
    struct netcode_address_t address;
    netcode_socket_handle_t handle;
};

static void netcode_socket_init( struct netcode_socket_t * socket )
{
    memset( socket, 0, sizeof( struct netcode_socket_t ) );
    socket->handle = NETCODE_SOCKET_HANDLE_INVALID;
}

static int netcode_socket_handle_valid( netcode_socket_handle_t handle )
{
    return handle != NETCODE_SOCKET_HANDLE_INVALID;
}

struct netcode_socket_holder_t
{
    struct netcode_socket_t ipv4;
    struct netcode_socket_t ipv6;
};

#define NETCODE_SOCKET_ERROR_NONE                                    0
#define NETCODE_SOCKET_ERROR_CREATE_FAILED                           1
#define NETCODE_SOCKET_ERROR_SET_NON_BLOCKING_FAILED                 2
#define NETCODE_SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED                3
#define NETCODE_SOCKET_ERROR_BIND_IPV4_FAILED                        6
#define NETCODE_SOCKET_ERROR_BIND_IPV6_FAILED                        7
#define NETCODE_SOCKET_ERROR_GET_SOCKNAME_IPV4_FAILED                8
#define NETCODE_SOCKET_ERROR_GET_SOCKNAME_IPV6_FAILED                9
#define NETCODE_SOCKET_ERROR_DISABLE_UDP_PORT_CONNRESET_FAILED      10
#define NETCODE_SOCKET_ERROR_ENABLE_PACKET_TAGGING_FAILED           11

#define NETCODE_SOCKET_BUFFER_SIZE_MINIMUM                ( 64 * 1024 )

typedef int (*netcode_socket_buffer_size_try_t)( void * context, int size );

// linux and windows clamp a socket buffer request that exceeds the OS limit, but the BSDs reject it
// instead (a stock NetBSD rejects anything over about 227 KiB). halve the request until one is accepted.
// a larger buffer is an optimisation, never a requirement: if no request down to the minimum is accepted,
// the socket keeps the OS default. returns the size accepted, or 0 if the OS default was kept.

static int netcode_socket_buffer_size_back_off( int requested_size, netcode_socket_buffer_size_try_t try_size, void * context )
{
    for ( int size = requested_size; size >= NETCODE_SOCKET_BUFFER_SIZE_MINIMUM; size /= 2 )
    {
        if ( try_size( context, size ) )
            return size;
    }
    return 0;
}

struct netcode_socket_buffer_option_t
{
    netcode_socket_handle_t handle;
    int option;
};

static int netcode_socket_buffer_size_set( void * context, int size )
{
    struct netcode_socket_buffer_option_t * buffer_option = (struct netcode_socket_buffer_option_t*) context;
    return setsockopt( buffer_option->handle, SOL_SOCKET, buffer_option->option, (char*)&size, sizeof(int) ) == 0;
}

static void netcode_socket_increase_buffer_size( netcode_socket_handle_t handle, int option, int requested_size, const char * name )
{
    struct netcode_socket_buffer_option_t buffer_option;
    buffer_option.handle = handle;
    buffer_option.option = option;
    const int size = netcode_socket_buffer_size_back_off( requested_size, netcode_socket_buffer_size_set, &buffer_option );
    if ( size == 0 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "socket %s buffer size kept at the OS default (%d was not accepted)\n", name, requested_size );
    }
    else if ( size != requested_size )
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "socket %s buffer size reduced from %d to %d\n", name, requested_size, size );
    }
}

void netcode_socket_destroy( struct netcode_socket_t * socket )
{
    netcode_assert( socket );
    netcode_assert( netcode.initialized );

    if ( netcode_socket_handle_valid( socket->handle ) )
    {
        #if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX
        close( socket->handle );
        #elif NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
        closesocket( socket->handle );
        #else
        #error unsupported platform
        #endif
        socket->handle = NETCODE_SOCKET_HANDLE_INVALID;
    }
}

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS && NETCODE_PACKET_TAGGING

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <wininet.h>
#include <iphlpapi.h>

#ifdef __MINGW32__
typedef UINT32 QOS_FLOWID, *PQOS_FLOWID;
#ifndef QOS_NON_ADAPTIVE_FLOW
#define QOS_NON_ADAPTIVE_FLOW 0x00000002
#endif // #ifndef QOS_NON_ADAPTIVE_FLOW
#endif // #ifdef __MINGW32__
#include <qos2.h>

#ifdef _MSC_VER
#pragma comment( lib, "Qwave.lib" )
#endif // #ifdef _MSC_VER

static int netcode_set_socket_codepoint( SOCKET socket, QOS_TRAFFIC_TYPE trafficType, QOS_FLOWID flowId, PSOCKADDR addr )
{
    // IMPORTANT: closing the QOS handle removes all flows added on it, so we create one handle
    // on first use and share it across all sockets for the lifetime of the process.

    static HANDLE qosHandle = NULL;
    if ( qosHandle == NULL )
    {
        QOS_VERSION QosVersion = { 1 , 0 };
        if ( QOSCreateHandle( &QosVersion, &qosHandle ) == FALSE )
        {
            qosHandle = NULL;
            return GetLastError();
        }
    }
    if ( QOSAddSocketToFlow( qosHandle, socket, addr, trafficType, QOS_NON_ADAPTIVE_FLOW, &flowId ) == FALSE )
    {
        return GetLastError();
    }
    return 0;
}

#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS && NETCODE_PACKET_TAGGING

static socklen_t netcode_address_to_sockaddr( NETCODE_CONST struct netcode_address_t * address, struct sockaddr_storage * sockaddr )
{
    netcode_assert( address );
    netcode_assert( sockaddr );
    netcode_assert( address->type == NETCODE_ADDRESS_IPV4 || address->type == NETCODE_ADDRESS_IPV6 );

    memset( sockaddr, 0, sizeof( struct sockaddr_storage ) );

    if ( address->type == NETCODE_ADDRESS_IPV6 )
    {
        struct sockaddr_in6 * addr_ipv6 = (struct sockaddr_in6*) sockaddr;
        addr_ipv6->sin6_family = AF_INET6;
        int i;
        uint16_t ipv6_network_order[8];
        for ( i = 0; i < 8; i++ )
        {
            ipv6_network_order[i] = htons( address->data.ipv6[i] );
        }
        memcpy( &addr_ipv6->sin6_addr, ipv6_network_order, sizeof( ipv6_network_order ) );
        addr_ipv6->sin6_port = htons( address->port );
        return sizeof( struct sockaddr_in6 );
    }
    else
    {
        struct sockaddr_in * addr_ipv4 = (struct sockaddr_in*) sockaddr;
        addr_ipv4->sin_family = AF_INET;
        addr_ipv4->sin_addr.s_addr = htonl( ( ( (uint32_t) address->data.ipv4[0] ) << 24 ) |
                                            ( ( (uint32_t) address->data.ipv4[1] ) << 16 ) |
                                            ( ( (uint32_t) address->data.ipv4[2] ) << 8  ) |
                                            ( ( (uint32_t) address->data.ipv4[3] )       ) );
        addr_ipv4->sin_port = htons( address->port );
        return sizeof( struct sockaddr_in );
    }
}

static int netcode_sockaddr_to_address( NETCODE_CONST struct sockaddr_storage * sockaddr, struct netcode_address_t * address )
{
    netcode_assert( sockaddr );
    netcode_assert( address );

    if ( sockaddr->ss_family == AF_INET6 )
    {
        NETCODE_CONST struct sockaddr_in6 * addr_ipv6 = (NETCODE_CONST struct sockaddr_in6*) sockaddr;
        address->type = NETCODE_ADDRESS_IPV6;
        uint16_t ipv6_network_order[8];
        memcpy( ipv6_network_order, &addr_ipv6->sin6_addr, sizeof( ipv6_network_order ) );
        int i;
        for ( i = 0; i < 8; i++ )
        {
            address->data.ipv6[i] = ntohs( ipv6_network_order[i] );
        }
        address->port = ntohs( addr_ipv6->sin6_port );
        return NETCODE_OK;
    }
    else if ( sockaddr->ss_family == AF_INET )
    {
        NETCODE_CONST struct sockaddr_in * addr_ipv4 = (NETCODE_CONST struct sockaddr_in*) sockaddr;
        address->type = NETCODE_ADDRESS_IPV4;
        uint32_t ipv4_host_order = ntohl( addr_ipv4->sin_addr.s_addr );
        address->data.ipv4[0] = (uint8_t) ( ( ipv4_host_order >> 24 ) & 0xFF );
        address->data.ipv4[1] = (uint8_t) ( ( ipv4_host_order >> 16 ) & 0xFF );
        address->data.ipv4[2] = (uint8_t) ( ( ipv4_host_order >> 8  ) & 0xFF );
        address->data.ipv4[3] = (uint8_t) ( ( ipv4_host_order       ) & 0xFF );
        address->port = ntohs( addr_ipv4->sin_port );
        return NETCODE_OK;
    }

    return NETCODE_ERROR;
}

int netcode_socket_create( struct netcode_socket_t * s, struct netcode_address_t * address, int send_buffer_size, int receive_buffer_size )
{
    netcode_assert( s );
    netcode_assert( address );
    netcode_assert( netcode.initialized );

    netcode_assert( address->type != NETCODE_ADDRESS_NONE );

#if NETCODE_ENABLE_TESTS
    netcode_test_real_socket_count++;
#endif // #if NETCODE_ENABLE_TESTS

    s->address = *address;

    // create socket

    s->handle = socket( ( address->type == NETCODE_ADDRESS_IPV6 ) ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( !netcode_socket_handle_valid( s->handle ) )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to create socket\n" );
        s->handle = NETCODE_SOCKET_HANDLE_INVALID;
        return NETCODE_SOCKET_ERROR_CREATE_FAILED;
    }

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    // IMPORTANT: tell windows we don't want to receive any connection reset messages for this socket
    // If we don't do this, clients disconnecting hard will cause recvfrom on the server to repeatedly error out 
    // due to ICMP disconnected packets, causing long periods where the server doesn't receive any packets from clients.

    #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    if ( WSAIoctl( s->handle, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL ) != 0 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to disable UDP CONNRESET (port unreachable) message reporting on socket\n" );
        netcode_socket_destroy( s );
        return NETCODE_SOCKET_ERROR_DISABLE_UDP_PORT_CONNRESET_FAILED;
    }

#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    // force IPv6 only if necessary

    if ( address->type == NETCODE_ADDRESS_IPV6 )
    {
        int yes = 1;
        if ( setsockopt( s->handle, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&yes, sizeof(yes) ) != 0 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to set socket ipv6 only\n" );
            netcode_socket_destroy( s );
            return NETCODE_SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED;
        }
    }

    // increase socket send and receive buffer sizes (see netcode_socket_buffer_size_back_off)

    netcode_socket_increase_buffer_size( s->handle, SO_SNDBUF, send_buffer_size, "send" );
    netcode_socket_increase_buffer_size( s->handle, SO_RCVBUF, receive_buffer_size, "receive" );

    // bind to port

    {
        struct sockaddr_storage socket_address;
        socklen_t socket_address_length = netcode_address_to_sockaddr( address, &socket_address );

        if ( bind( s->handle, (struct sockaddr*) &socket_address, socket_address_length ) < 0 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to bind socket (%s)\n", ( address->type == NETCODE_ADDRESS_IPV6 ) ? "ipv6" : "ipv4" );
            netcode_socket_destroy( s );
            return ( address->type == NETCODE_ADDRESS_IPV6 ) ? NETCODE_SOCKET_ERROR_BIND_IPV6_FAILED : NETCODE_SOCKET_ERROR_BIND_IPV4_FAILED;
        }
    }

    // if bound to port 0 find the actual port we got

    if ( address->port == 0 )
    {
        struct sockaddr_storage socket_address;
        socklen_t socket_address_length = sizeof( socket_address );
        struct netcode_address_t bound_address;

        if ( getsockname( s->handle, (struct sockaddr*) &socket_address, &socket_address_length ) == -1 ||
             netcode_sockaddr_to_address( &socket_address, &bound_address ) != NETCODE_OK )
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to get socket port (%s)\n", ( address->type == NETCODE_ADDRESS_IPV6 ) ? "ipv6" : "ipv4" );
            netcode_socket_destroy( s );
            return ( address->type == NETCODE_ADDRESS_IPV6 ) ? NETCODE_SOCKET_ERROR_GET_SOCKNAME_IPV6_FAILED : NETCODE_SOCKET_ERROR_GET_SOCKNAME_IPV4_FAILED;
        }

        s->address.port = bound_address.port;
    }

    // set non-blocking io

#if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX

    int non_blocking = 1;
    if ( fcntl( s->handle, F_SETFL, O_NONBLOCK, non_blocking ) == -1 )
    {
        netcode_socket_destroy( s );
        return NETCODE_SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
    }

#elif NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    DWORD nonBlocking = 1;
    if ( ioctlsocket( s->handle, FIONBIO, &nonBlocking ) != 0 )
    {
        netcode_socket_destroy( s );
        return NETCODE_SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
    }

#else

    #error unsupported platform

#endif

#if NETCODE_PACKET_TAGGING

    // tag packets as low latency

#if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC

    if ( netcode_packet_tagging_enabled )
    {
        if ( address->type == NETCODE_ADDRESS_IPV6 )
        {
            int tos = 46;
            if ( setsockopt( s->handle, IPPROTO_IPV6, IPV6_TCLASS, (NETCODE_CONST char *)&tos, sizeof(tos) ) != 0 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to enable packet tagging (ipv6)\n" );
                netcode_socket_destroy( s );
                return NETCODE_SOCKET_ERROR_ENABLE_PACKET_TAGGING_FAILED;
            }
        }
        else
        {
            int tos = 46;
            if ( setsockopt( s->handle, IPPROTO_IP, IP_TOS, (NETCODE_CONST char *)&tos, sizeof(tos) ) != 0 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to enable packet tagging (ipv4)\n" );
                netcode_socket_destroy( s );
                return NETCODE_SOCKET_ERROR_ENABLE_PACKET_TAGGING_FAILED;
            }
        }
    }

#elif NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX

    if ( netcode_packet_tagging_enabled )
    {
        if ( address->type == NETCODE_ADDRESS_IPV6 )
        {
            int tos = 46;
            if ( setsockopt( s->handle, IPPROTO_IPV6, IPV6_TCLASS, (NETCODE_CONST char *)&tos, sizeof(tos) ) != 0 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to enable packet tagging (ipv6)\n" );
                netcode_socket_destroy( s );
                return NETCODE_SOCKET_ERROR_ENABLE_PACKET_TAGGING_FAILED;
            }
        }
        else
        {
            int tos = 46;
            if ( setsockopt( s->handle, IPPROTO_IP, IP_TOS, (NETCODE_CONST char *)&tos, sizeof(tos) ) != 0 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to enable packet tagging (ipv4)\n" );
                netcode_socket_destroy( s );
                return NETCODE_SOCKET_ERROR_ENABLE_PACKET_TAGGING_FAILED;
            }
        }
    }

#elif NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    if ( netcode_packet_tagging_enabled )
    {
        struct sockaddr_storage addr;
        memset( &addr, 0, sizeof(addr) );
        addr.ss_family = ( address->type == NETCODE_ADDRESS_IPV6 ) ? AF_INET6 : AF_INET;
        // the qWAVE calls fail on some Windows hosts, so tagging here is best effort:
        // a failure is logged and the socket is kept, untagged, as in earlier releases

        if ( netcode_set_socket_codepoint( s->handle, QOSTrafficTypeAudioVideo, 0, (PSOCKADDR) &addr ) != 0 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_INFO, "failed to enable packet tagging. continuing without it\n" );
        }
    }

#endif

#endif // #if NETCODE_PACKET_TAGGING

    return NETCODE_SOCKET_ERROR_NONE;
}

void netcode_socket_send_packet( struct netcode_socket_t * socket, struct netcode_address_t * to, void * packet_data, int packet_bytes )
{
    netcode_assert( socket );
    netcode_assert( netcode_socket_handle_valid( socket->handle ) );
    netcode_assert( to );
    netcode_assert( to->type == NETCODE_ADDRESS_IPV6 || to->type == NETCODE_ADDRESS_IPV4 );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes > 0 );

    struct sockaddr_storage socket_address;
    socklen_t socket_address_length = netcode_address_to_sockaddr( to, &socket_address );

    int result = sendto( socket->handle, (NETCODE_CONST char*) packet_data, packet_bytes, 0, (struct sockaddr*) &socket_address, socket_address_length );

    (void) result;
}

int netcode_socket_receive_packet( struct netcode_socket_t * socket, struct netcode_address_t * from, void * packet_data, int max_packet_size )
{
    netcode_assert( socket );
    netcode_assert( netcode_socket_handle_valid( socket->handle ) );
    netcode_assert( from );
    netcode_assert( packet_data );
    netcode_assert( max_packet_size > 0 );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    typedef int socklen_t;
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    
    while ( 1 )
    {
        struct sockaddr_storage sockaddr_from;
        memset( &sockaddr_from, 0, sizeof(sockaddr_from) );

        socklen_t from_length = sizeof( sockaddr_from );

        int result = recvfrom( socket->handle, (char*) packet_data, max_packet_size, 0, (struct sockaddr*) &sockaddr_from, &from_length );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
        if ( result == SOCKET_ERROR )
        {
            int error = WSAGetLastError();

            if ( error == WSAEWOULDBLOCK )
                return 0;

            if ( error == WSAEMSGSIZE )
                continue;

            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: recvfrom failed with error %d\n", error );

            return 0;
        }
#else // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
        if ( result < 0 )
        {
            if ( errno == EAGAIN || errno == EWOULDBLOCK )
                return 0;

            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: recvfrom failed with error %d\n", errno );

            return 0;
        }
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

        if ( result == 0 )
            continue;

        if ( netcode_sockaddr_to_address( &sockaddr_from, from ) != NETCODE_OK )
        {
            netcode_assert( 0 );
            return 0;
        }

        netcode_assert( result > 0 );

        return result;
    }
}

// ----------------------------------------------------------------

void netcode_write_uint8( uint8_t ** p, uint8_t value )
{
    **p = value;
    ++(*p);
}

void netcode_write_uint16( uint8_t ** p, uint16_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = value >> 8;
    *p += 2;
}

void netcode_write_uint32( uint8_t ** p, uint32_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = ( value >> 8  ) & 0xFF;
    (*p)[2] = ( value >> 16 ) & 0xFF;
    (*p)[3] = value >> 24;
    *p += 4;
}

void netcode_write_uint64( uint8_t ** p, uint64_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = ( value >> 8  ) & 0xFF;
    (*p)[2] = ( value >> 16 ) & 0xFF;
    (*p)[3] = ( value >> 24 ) & 0xFF;
    (*p)[4] = ( value >> 32 ) & 0xFF;
    (*p)[5] = ( value >> 40 ) & 0xFF;
    (*p)[6] = ( value >> 48 ) & 0xFF;
    (*p)[7] = value >> 56;
    *p += 8;
}

void netcode_write_bytes( uint8_t ** p, uint8_t * byte_array, int num_bytes )
{
    int i;
    for ( i = 0; i < num_bytes; i++ )
    {
        netcode_write_uint8( p, byte_array[i] );
    }
}

uint8_t netcode_read_uint8( uint8_t ** p )
{
    uint8_t value = **p;
    ++(*p);
    return value;
}

uint16_t netcode_read_uint16( uint8_t ** p )
{
    uint16_t value;
    value = (*p)[0];
    value |= ( ( (uint16_t)( (*p)[1] ) ) << 8 );
    *p += 2;
    return value;
}

uint32_t netcode_read_uint32( uint8_t ** p )
{
    uint32_t value;
    value  = (*p)[0];
    value |= ( ( (uint32_t)( (*p)[1] ) ) << 8 );
    value |= ( ( (uint32_t)( (*p)[2] ) ) << 16 );
    value |= ( ( (uint32_t)( (*p)[3] ) ) << 24 );
    *p += 4;
    return value;
}

uint64_t netcode_read_uint64( uint8_t ** p )
{
    uint64_t value;
    value  = (*p)[0];
    value |= ( ( (uint64_t)( (*p)[1] ) ) << 8  );
    value |= ( ( (uint64_t)( (*p)[2] ) ) << 16 );
    value |= ( ( (uint64_t)( (*p)[3] ) ) << 24 );
    value |= ( ( (uint64_t)( (*p)[4] ) ) << 32 );
    value |= ( ( (uint64_t)( (*p)[5] ) ) << 40 );
    value |= ( ( (uint64_t)( (*p)[6] ) ) << 48 );
    value |= ( ( (uint64_t)( (*p)[7] ) ) << 56 );
    *p += 8;
    return value;
}

void netcode_read_bytes( uint8_t ** p, uint8_t * byte_array, int num_bytes )
{
    int i;
    for ( i = 0; i < num_bytes; i++ )
    {
        byte_array[i] = netcode_read_uint8( p );
    }
}

// ----------------------------------------------------------------

void netcode_generate_key( uint8_t * key )
{
    netcode_assert( key );
    randombytes_buf( key, NETCODE_KEY_BYTES );
}

void netcode_generate_nonce( uint8_t * nonce )
{
    netcode_assert( nonce );
    randombytes_buf( nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
}

void netcode_random_bytes( uint8_t * data, int bytes )
{
    netcode_assert( data );
    netcode_assert( bytes > 0 );
    randombytes_buf( data, bytes );
}

int netcode_encrypt_aead_bignonce( uint8_t * message, uint64_t message_length, 
                                   uint8_t * additional, uint64_t additional_length,
                                   NETCODE_CONST uint8_t * nonce,
                                   NETCODE_CONST uint8_t * key )
{
    unsigned long long encrypted_length;

    int result = crypto_aead_xchacha20poly1305_ietf_encrypt( message, &encrypted_length,
                                                             message, (unsigned long long) message_length,
                                                             additional, (unsigned long long) additional_length,
                                                             NULL, nonce, key );
    
    if ( result != 0 )
        return NETCODE_ERROR;

    netcode_assert( encrypted_length == message_length + NETCODE_MAC_BYTES );

    return NETCODE_OK;
}

int netcode_decrypt_aead_bignonce( uint8_t * message, uint64_t message_length, 
                                   uint8_t * additional, uint64_t additional_length,
                                   uint8_t * nonce,
                                   uint8_t * key )
{
    unsigned long long decrypted_length;

    int result = crypto_aead_xchacha20poly1305_ietf_decrypt( message, &decrypted_length,
                                                             NULL,
                                                             message, (unsigned long long) message_length,
                                                             additional, (unsigned long long) additional_length,
                                                             nonce, key );

    if ( result != 0 )
        return NETCODE_ERROR;

    netcode_assert( decrypted_length == message_length - NETCODE_MAC_BYTES );

    return NETCODE_OK;
}

int netcode_encrypt_aead( uint8_t * message, uint64_t message_length, 
                          uint8_t * additional, uint64_t additional_length,
                          NETCODE_CONST uint8_t * nonce,
                          NETCODE_CONST uint8_t * key )
{
    unsigned long long encrypted_length;

    int result = crypto_aead_chacha20poly1305_ietf_encrypt( message, &encrypted_length,
                                                            message, (unsigned long long) message_length,
                                                            additional, (unsigned long long) additional_length,
                                                            NULL, nonce, key );
    
    if ( result != 0 )
        return NETCODE_ERROR;

    netcode_assert( encrypted_length == message_length + NETCODE_MAC_BYTES );

    return NETCODE_OK;
}

int netcode_decrypt_aead( uint8_t * message, uint64_t message_length, 
                          uint8_t * additional, uint64_t additional_length,
                          uint8_t * nonce,
                          uint8_t * key )
{
    unsigned long long decrypted_length;

    int result = crypto_aead_chacha20poly1305_ietf_decrypt( message, &decrypted_length,
                                                            NULL,
                                                            message, (unsigned long long) message_length,
                                                            additional, (unsigned long long) additional_length,
                                                            nonce, key );

    if ( result != 0 )
        return NETCODE_ERROR;

    netcode_assert( decrypted_length == message_length - NETCODE_MAC_BYTES );

    return NETCODE_OK;
}

// ----------------------------------------------------------------

struct netcode_connect_token_private_t
{
    uint64_t client_id;
    int timeout_seconds;
    int num_server_addresses;
    struct netcode_address_t server_addresses[NETCODE_MAX_SERVERS_PER_CONNECT];
    uint8_t client_to_server_key[NETCODE_KEY_BYTES];
    uint8_t server_to_client_key[NETCODE_KEY_BYTES];
    uint8_t user_data[NETCODE_USER_DATA_BYTES];
};

void netcode_generate_connect_token_private( struct netcode_connect_token_private_t * connect_token, 
                                             uint64_t client_id, 
                                             int timeout_seconds,
                                             int num_server_addresses, 
                                             struct netcode_address_t * server_addresses, 
                                             uint8_t * user_data )
{
    netcode_assert( connect_token );
    netcode_assert( num_server_addresses > 0 );
    netcode_assert( num_server_addresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
    netcode_assert( server_addresses );
    netcode_assert( user_data );

    connect_token->client_id = client_id;
    connect_token->timeout_seconds = timeout_seconds;
    connect_token->num_server_addresses = num_server_addresses;
    
    int i;
    for ( i = 0; i < num_server_addresses; i++ )
    {
        memcpy( &connect_token->server_addresses[i], &server_addresses[i], sizeof( struct netcode_address_t ) );
    }

    netcode_generate_key( connect_token->client_to_server_key );
    netcode_generate_key( connect_token->server_to_client_key );

    if ( user_data != NULL )
    {
        memcpy( connect_token->user_data, user_data, NETCODE_USER_DATA_BYTES );
    }
    else
    {
        memset( connect_token->user_data, 0, NETCODE_USER_DATA_BYTES );
    }
}

void netcode_write_connect_token_private( struct netcode_connect_token_private_t * connect_token, uint8_t * buffer, int buffer_length )
{
    (void) buffer_length;

    netcode_assert( connect_token );
    netcode_assert( connect_token->num_server_addresses > 0 );
    netcode_assert( connect_token->num_server_addresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    uint8_t * start = buffer;

    (void) start;

    netcode_write_uint64( &buffer, connect_token->client_id );

    netcode_write_uint32( &buffer, connect_token->timeout_seconds );

    netcode_write_uint32( &buffer, connect_token->num_server_addresses );

    int i,j;

    for ( i = 0; i < connect_token->num_server_addresses; i++ )
    {
        if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV4 )
        {
            netcode_write_uint8( &buffer, NETCODE_ADDRESS_IPV4 );
            for ( j = 0; j < 4; j++ )
            {
                netcode_write_uint8( &buffer, connect_token->server_addresses[i].data.ipv4[j] );
            }
            netcode_write_uint16( &buffer, connect_token->server_addresses[i].port );
        }
        else if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV6 )
        {
            netcode_write_uint8( &buffer, NETCODE_ADDRESS_IPV6 );
            for ( j = 0; j < 8; j++ )
            {
                netcode_write_uint16( &buffer, connect_token->server_addresses[i].data.ipv6[j] );
            }
            netcode_write_uint16( &buffer, connect_token->server_addresses[i].port );
        }
        else
        {
            netcode_assert( 0 );
        }
    }

    netcode_write_bytes( &buffer, connect_token->client_to_server_key, NETCODE_KEY_BYTES );

    netcode_write_bytes( &buffer, connect_token->server_to_client_key, NETCODE_KEY_BYTES );

    netcode_write_bytes( &buffer, connect_token->user_data, NETCODE_USER_DATA_BYTES );

    netcode_assert( buffer - start <= NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES );

    memset( buffer, 0, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - ( buffer - start ) );
}

int netcode_encrypt_connect_token_private( uint8_t * buffer, 
                                           int buffer_length, 
                                           uint8_t * version_info, 
                                           uint64_t protocol_id, 
                                           uint64_t expire_timestamp, 
                                           NETCODE_CONST uint8_t * nonce, 
                                           NETCODE_CONST uint8_t * key )
{
    netcode_assert( buffer );
    netcode_assert( buffer_length == NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    netcode_assert( key );

    (void) buffer_length;

    uint8_t additional_data[NETCODE_VERSION_INFO_BYTES+8+8];
    {
        uint8_t * p = additional_data;
        netcode_write_bytes( &p, version_info, NETCODE_VERSION_INFO_BYTES );
        netcode_write_uint64( &p, protocol_id );
        netcode_write_uint64( &p, expire_timestamp );
    }

    return netcode_encrypt_aead_bignonce( buffer, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES, additional_data, sizeof( additional_data ), nonce, key );
}

int netcode_decrypt_connect_token_private( uint8_t * buffer, 
                                           int buffer_length, 
                                           uint8_t * version_info, 
                                           uint64_t protocol_id, 
                                           uint64_t expire_timestamp, 
                                           uint8_t * nonce, 
                                           uint8_t * key )
{
    netcode_assert( buffer );
    netcode_assert( buffer_length == NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    netcode_assert( key );

    (void) buffer_length;

    uint8_t additional_data[NETCODE_VERSION_INFO_BYTES+8+8];
    {
        uint8_t * p = additional_data;
        netcode_write_bytes( &p, version_info, NETCODE_VERSION_INFO_BYTES );
        netcode_write_uint64( &p, protocol_id );
        netcode_write_uint64( &p, expire_timestamp );
    }
    return netcode_decrypt_aead_bignonce( buffer, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, additional_data, sizeof( additional_data ), nonce, key );
}

int netcode_read_connect_token_private( uint8_t * buffer, int buffer_length, struct netcode_connect_token_private_t * connect_token )
{
    netcode_assert( buffer );
    netcode_assert( connect_token );

    if ( buffer_length < NETCODE_CONNECT_TOKEN_PRIVATE_BYTES )
        return NETCODE_ERROR;
    
    connect_token->client_id = netcode_read_uint64( &buffer );

    connect_token->timeout_seconds = (int) netcode_read_uint32( &buffer );

    connect_token->num_server_addresses = netcode_read_uint32( &buffer );

    if ( connect_token->num_server_addresses <= 0 )
        return NETCODE_ERROR;

    if ( connect_token->num_server_addresses > NETCODE_MAX_SERVERS_PER_CONNECT )
        return NETCODE_ERROR;

    int i,j;

    for ( i = 0; i < connect_token->num_server_addresses; i++ )
    {
        connect_token->server_addresses[i].type = netcode_read_uint8( &buffer );

        if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV4 )
        {
            for ( j = 0; j < 4; j++ )
            {
                connect_token->server_addresses[i].data.ipv4[j] = netcode_read_uint8( &buffer );
            }
            connect_token->server_addresses[i].port = netcode_read_uint16( &buffer );
        }
        else if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV6 )
        {
            for ( j = 0; j < 8; j++ )
            {
                connect_token->server_addresses[i].data.ipv6[j] = netcode_read_uint16( &buffer );
            }
            connect_token->server_addresses[i].port = netcode_read_uint16( &buffer );
        }
        else
        {
            return NETCODE_ERROR;
        }
    }

    netcode_read_bytes( &buffer, connect_token->client_to_server_key, NETCODE_KEY_BYTES );

    netcode_read_bytes( &buffer, connect_token->server_to_client_key, NETCODE_KEY_BYTES );

    netcode_read_bytes( &buffer, connect_token->user_data, NETCODE_USER_DATA_BYTES );

    return NETCODE_OK;
}

// -----------------------------------------------

struct netcode_challenge_token_t
{
    uint64_t client_id;
    uint8_t user_data[NETCODE_USER_DATA_BYTES];
};

void netcode_write_challenge_token( struct netcode_challenge_token_t * challenge_token, uint8_t * buffer, int buffer_length )
{
    (void) buffer_length;

    netcode_assert( challenge_token );
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CHALLENGE_TOKEN_BYTES );

    memset( buffer, 0, NETCODE_CHALLENGE_TOKEN_BYTES );

    uint8_t * start = buffer;

    (void) start;

    netcode_write_uint64( &buffer, challenge_token->client_id );

    netcode_write_bytes( &buffer, challenge_token->user_data, NETCODE_USER_DATA_BYTES ); 

    netcode_assert( buffer - start <= NETCODE_CHALLENGE_TOKEN_BYTES - NETCODE_MAC_BYTES );
}

int netcode_encrypt_challenge_token( uint8_t * buffer, int buffer_length, uint64_t sequence, uint8_t * key )
{
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CHALLENGE_TOKEN_BYTES );
    netcode_assert( key );

    (void) buffer_length;

    uint8_t nonce[12];
    {
        uint8_t * p = nonce;
        netcode_write_uint32( &p, 0 );
        netcode_write_uint64( &p, sequence );
    }

    return netcode_encrypt_aead( buffer, NETCODE_CHALLENGE_TOKEN_BYTES - NETCODE_MAC_BYTES, NULL, 0, nonce, key );
}

int netcode_decrypt_challenge_token( uint8_t * buffer, int buffer_length, uint64_t sequence, uint8_t * key )
{
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CHALLENGE_TOKEN_BYTES );
    netcode_assert( key );

    (void) buffer_length;

    uint8_t nonce[12];
    {
        uint8_t * p = nonce;
        netcode_write_uint32( &p, 0 );
        netcode_write_uint64( &p, sequence );
    }

    return netcode_decrypt_aead( buffer, NETCODE_CHALLENGE_TOKEN_BYTES, NULL, 0, nonce, key );
}

int netcode_read_challenge_token( uint8_t * buffer, int buffer_length, struct netcode_challenge_token_t * challenge_token )
{
    netcode_assert( buffer );
    netcode_assert( challenge_token );

    if ( buffer_length < NETCODE_CHALLENGE_TOKEN_BYTES )
        return NETCODE_ERROR;

    uint8_t * start = buffer;

    (void) start;
    
    challenge_token->client_id = netcode_read_uint64( &buffer );

    netcode_read_bytes( &buffer, challenge_token->user_data, NETCODE_USER_DATA_BYTES );

    netcode_assert( buffer - start == 8 + NETCODE_USER_DATA_BYTES );

    return NETCODE_OK;
}

// ----------------------------------------------------------------

#define NETCODE_CONNECTION_REQUEST_PACKET           0
#define NETCODE_CONNECTION_DENIED_PACKET            1
#define NETCODE_CONNECTION_CHALLENGE_PACKET         2
#define NETCODE_CONNECTION_RESPONSE_PACKET          3
#define NETCODE_CONNECTION_KEEP_ALIVE_PACKET        4
#define NETCODE_CONNECTION_PAYLOAD_PACKET           5
#define NETCODE_CONNECTION_DISCONNECT_PACKET        6
#define NETCODE_CONNECTION_NUM_PACKETS              7

struct netcode_connection_request_packet_t
{
    uint8_t packet_type;
    uint8_t version_info[NETCODE_VERSION_INFO_BYTES];
    uint64_t protocol_id;
    uint64_t connect_token_expire_timestamp;
    uint8_t connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    uint8_t connect_token_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    uint8_t client_salt[NETCODE_CONNECT_SALT_BYTES];
};

struct netcode_connection_denied_packet_t
{
    uint8_t packet_type;
    uint8_t server_nonce[NETCODE_SERVER_NONCE_BYTES];
};

struct netcode_connection_challenge_packet_t
{
    uint8_t packet_type;
    uint8_t server_nonce[NETCODE_SERVER_NONCE_BYTES];
    uint64_t challenge_token_sequence;
    uint8_t challenge_token_data[NETCODE_CHALLENGE_TOKEN_BYTES];
};

struct netcode_connection_response_packet_t
{
    uint8_t packet_type;
    uint64_t challenge_token_sequence;
    uint8_t challenge_token_data[NETCODE_CHALLENGE_TOKEN_BYTES];
};

struct netcode_connection_keep_alive_packet_t
{
    uint8_t packet_type;
    int client_index;
    int max_clients;
};

struct netcode_connection_payload_packet_t
{
    uint8_t packet_type;
    uint32_t payload_bytes;
    /* serialize's BitReader loads an 8-byte window from the current byte, so a
       read in the last payload byte reaches 7 past it. This array is the last
       member; sizeof(*packet)+payload_bytes is allocated, leaving 8 bytes
       behind the returned payload pointer. Do not shrink it. */
    uint8_t payload_data[8];
};

typedef char netcode_payload_packet_tail_holds_reader_slack[
    ( sizeof( struct netcode_connection_payload_packet_t )
      - offsetof( struct netcode_connection_payload_packet_t, payload_data ) >= 8 ) ? 1 : -1 ];

struct netcode_connection_disconnect_packet_t
{
    uint8_t packet_type;
};

struct netcode_connection_payload_packet_t * netcode_create_payload_packet( int payload_bytes, void * allocator_context, void* (*allocate_function)(void*,size_t) )
{
    netcode_assert( payload_bytes >= 0 );
    netcode_assert( payload_bytes <= NETCODE_MAX_PAYLOAD_BYTES );

    if ( allocate_function == NULL )
    {
        allocate_function = netcode_default_allocate_function;
    }

    struct netcode_connection_payload_packet_t * packet = (struct netcode_connection_payload_packet_t*) 
        allocate_function( allocator_context, sizeof( struct netcode_connection_payload_packet_t ) + payload_bytes );

    if ( !packet )
        return NULL;
    
    packet->packet_type = NETCODE_CONNECTION_PAYLOAD_PACKET;
    packet->payload_bytes = payload_bytes;

    return packet;
}

struct netcode_context_t
{
    uint8_t write_packet_key[NETCODE_KEY_BYTES];
    uint8_t read_packet_key[NETCODE_KEY_BYTES];
};

static int netcode_bytes_equal_8( NETCODE_CONST uint8_t * a, NETCODE_CONST uint8_t * b )
{
    uint8_t diff = 0;
    int i;
    for ( i = 0; i < 8; i++ )
    {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

static void netcode_derive_packet_key( uint8_t * packet_key,
                                       NETCODE_CONST uint8_t * token_key,
                                       NETCODE_CONST uint8_t * client_salt,
                                       NETCODE_CONST uint8_t * server_nonce )
{
    netcode_assert( packet_key );
    netcode_assert( token_key );
    netcode_assert( client_salt );
    netcode_assert( server_nonce );

    uint8_t input[NETCODE_CONNECT_SALT_BYTES + NETCODE_SERVER_NONCE_BYTES];
    memcpy( input, client_salt, NETCODE_CONNECT_SALT_BYTES );
    memcpy( input + NETCODE_CONNECT_SALT_BYTES, server_nonce, NETCODE_SERVER_NONCE_BYTES );

    crypto_core_hchacha20( packet_key, input, token_key, NULL );

    sodium_memzero( input, sizeof( input ) );
}

int netcode_sequence_number_bytes_required( uint64_t sequence )
{
    int i;
    uint64_t mask = 0xFF00000000000000UL;
    for ( i = 0; i < 7; i++ )
    {
        if ( sequence & mask )
            break;
        mask >>= 8;
    }
    return 8 - i;
}

#if NETCODE_ENABLE_NONCE_AUDIT

/*
    Test-only instrumentation, compiled in by the NETCODE_NONCE_AUDIT build option.

    Records the key and nonce of every packet netcode_write_packet encrypts and counts how
    many times a pair repeats. A repeated key and nonce pair under AEAD is what a connect
    token used for two sessions produces, so the whole test suite running with zero repeats
    is the property worth pinning. Not built into the library.
*/

#define NETCODE_NONCE_AUDIT_MAX_RECORDS ( 1 << 18 )
#define NETCODE_NONCE_AUDIT_HASH_SIZE ( 1 << 19 )
#define NETCODE_NONCE_AUDIT_NONCE_BYTES 12

struct netcode_nonce_audit_record_t
{
    uint8_t key[NETCODE_KEY_BYTES];
    uint8_t nonce[NETCODE_NONCE_AUDIT_NONCE_BYTES];
};

static struct netcode_nonce_audit_record_t netcode_nonce_audit_records[NETCODE_NONCE_AUDIT_MAX_RECORDS];
static int netcode_nonce_audit_hash[NETCODE_NONCE_AUDIT_HASH_SIZE];
static int netcode_nonce_audit_hash_initialized;
static int netcode_nonce_audit_num_records;
static int netcode_nonce_audit_num_repeats;
static int netcode_nonce_audit_overflowed;

static void netcode_nonce_audit_write_packet( uint8_t * key, uint8_t * nonce )
{
    if ( !netcode_nonce_audit_hash_initialized )
    {
        int i;
        for ( i = 0; i < NETCODE_NONCE_AUDIT_HASH_SIZE; i++ )
        {
            netcode_nonce_audit_hash[i] = -1;
        }
        netcode_nonce_audit_hash_initialized = 1;
    }

    uint64_t hash = 0xCBF29CE484222325ULL;
    int i;
    for ( i = 0; i < NETCODE_KEY_BYTES; i++ )
    {
        hash ^= key[i];
        hash *= 0x00000100000001B3ULL;
    }
    for ( i = 0; i < NETCODE_NONCE_AUDIT_NONCE_BYTES; i++ )
    {
        hash ^= nonce[i];
        hash *= 0x00000100000001B3ULL;
    }

    int slot = (int) ( hash & ( NETCODE_NONCE_AUDIT_HASH_SIZE - 1 ) );

    while ( netcode_nonce_audit_hash[slot] != -1 )
    {
        struct netcode_nonce_audit_record_t * record = &netcode_nonce_audit_records[netcode_nonce_audit_hash[slot]];
        if ( memcmp( record->key, key, NETCODE_KEY_BYTES ) == 0 && memcmp( record->nonce, nonce, NETCODE_NONCE_AUDIT_NONCE_BYTES ) == 0 )
        {
            netcode_nonce_audit_num_repeats++;
            printf( "NONCE AUDIT: repeated key and nonce pair: key %.2x%.2x%.2x%.2x.. sequence %d\n", 
                key[0], key[1], key[2], key[3], (int) nonce[4] );
            return;
        }
        slot = ( slot + 1 ) & ( NETCODE_NONCE_AUDIT_HASH_SIZE - 1 );
    }

    if ( netcode_nonce_audit_num_records == NETCODE_NONCE_AUDIT_MAX_RECORDS )
    {
        netcode_nonce_audit_overflowed = 1;
        return;
    }

    struct netcode_nonce_audit_record_t * record = &netcode_nonce_audit_records[netcode_nonce_audit_num_records];
    memcpy( record->key, key, NETCODE_KEY_BYTES );
    memcpy( record->nonce, nonce, NETCODE_NONCE_AUDIT_NONCE_BYTES );
    netcode_nonce_audit_hash[slot] = netcode_nonce_audit_num_records;
    netcode_nonce_audit_num_records++;
}

int netcode_nonce_audit_num_pairs()
{
    return netcode_nonce_audit_num_records;
}

int netcode_nonce_audit_repeats()
{
    return netcode_nonce_audit_num_repeats;
}

int netcode_nonce_audit_overflow()
{
    return netcode_nonce_audit_overflowed;
}

#endif // #if NETCODE_ENABLE_NONCE_AUDIT

int netcode_write_packet( void * packet, uint8_t * buffer, int buffer_length, uint64_t sequence, uint8_t * write_packet_key, uint64_t protocol_id )
{
    netcode_assert( packet );
    netcode_assert( buffer );
    netcode_assert( write_packet_key );

    (void) buffer_length;

    uint8_t packet_type = ((uint8_t*)packet)[0];

    if ( packet_type == NETCODE_CONNECTION_REQUEST_PACKET )
    {
        // connection request packet: first byte is zero

        netcode_assert( buffer_length >= 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES + NETCODE_CONNECT_SALT_BYTES );

        struct netcode_connection_request_packet_t * p = (struct netcode_connection_request_packet_t*) packet;

        uint8_t * start = buffer;

        netcode_write_uint8( &buffer, NETCODE_CONNECTION_REQUEST_PACKET );
        netcode_write_bytes( &buffer, p->version_info, NETCODE_VERSION_INFO_BYTES );
        netcode_write_uint64( &buffer, p->protocol_id );
        netcode_write_uint64( &buffer, p->connect_token_expire_timestamp );
        netcode_write_bytes( &buffer, p->connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
        netcode_write_bytes( &buffer, p->connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
        netcode_write_bytes( &buffer, p->client_salt, NETCODE_CONNECT_SALT_BYTES );

        netcode_assert( buffer - start == 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES + NETCODE_CONNECT_SALT_BYTES );

        return (int) ( buffer - start );
    }
    else
    {
        // *** encrypted packets ***

        // write the prefix byte (this is a combination of the packet type and number of sequence bytes)

        uint8_t * start = buffer;

        uint8_t sequence_bytes = (uint8_t) netcode_sequence_number_bytes_required( sequence );

        netcode_assert( sequence_bytes >= 1 );
        netcode_assert( sequence_bytes <= 8 );

        netcode_assert( packet_type <= 0xF );

        uint8_t prefix_byte = packet_type | ( sequence_bytes << 4 );

        netcode_write_uint8( &buffer, prefix_byte );

        // write the variable length sequence number [1,8] bytes.

        uint64_t sequence_temp = sequence;

        int i;
        for ( i = 0; i < sequence_bytes; i++ )
        {
            netcode_write_uint8( &buffer, (uint8_t) ( sequence_temp & 0xFF ) );
            sequence_temp >>= 8;
        }

        // packet types 1 (denied) and 2 (challenge) carry an 8-byte cleartext server_nonce header

        NETCODE_CONST uint8_t * header_server_nonce = NULL;

        if ( packet_type == NETCODE_CONNECTION_DENIED_PACKET )
        {
            struct netcode_connection_denied_packet_t * p = (struct netcode_connection_denied_packet_t*) packet;
            header_server_nonce = p->server_nonce;
            netcode_write_bytes( &buffer, p->server_nonce, NETCODE_SERVER_NONCE_BYTES );
        }
        else if ( packet_type == NETCODE_CONNECTION_CHALLENGE_PACKET )
        {
            struct netcode_connection_challenge_packet_t * p = (struct netcode_connection_challenge_packet_t*) packet;
            header_server_nonce = p->server_nonce;
            netcode_write_bytes( &buffer, p->server_nonce, NETCODE_SERVER_NONCE_BYTES );
        }

        // write packet data according to type. this data will be encrypted.

        uint8_t * encrypted_start = buffer;

        switch ( packet_type )
        {
            case NETCODE_CONNECTION_DENIED_PACKET:
            {
                // ...
            }
            break;

            case NETCODE_CONNECTION_CHALLENGE_PACKET:
            {
                struct netcode_connection_challenge_packet_t * p = (struct netcode_connection_challenge_packet_t*) packet;
                netcode_write_uint64( &buffer, p->challenge_token_sequence );
                netcode_write_bytes( &buffer, p->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
            }
            break;

            case NETCODE_CONNECTION_RESPONSE_PACKET:
            {
                struct netcode_connection_response_packet_t * p = (struct netcode_connection_response_packet_t*) packet;
                netcode_write_uint64( &buffer, p->challenge_token_sequence );
                netcode_write_bytes( &buffer, p->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
            }
            break;

            case NETCODE_CONNECTION_KEEP_ALIVE_PACKET:
            {
                struct netcode_connection_keep_alive_packet_t * p = (struct netcode_connection_keep_alive_packet_t*) packet;
                netcode_write_uint32( &buffer, p->client_index );
                netcode_write_uint32( &buffer, p->max_clients );
            }
            break;

            case NETCODE_CONNECTION_PAYLOAD_PACKET:
            {
                struct netcode_connection_payload_packet_t * p = (struct netcode_connection_payload_packet_t*) packet;

                netcode_assert( p->payload_bytes <= NETCODE_MAX_PAYLOAD_BYTES );

                netcode_write_bytes( &buffer, p->payload_data, p->payload_bytes );
            }
            break;

            case NETCODE_CONNECTION_DISCONNECT_PACKET:
            {
                // ...
            }
            break;

            default:
                netcode_assert( 0 );
        }

        netcode_assert( buffer - start <= buffer_length - NETCODE_MAC_BYTES );

        uint8_t * encrypted_finish = buffer;

        // encrypt the per-packet packet written with the prefix byte, protocol id, version, and
        // (for denied and challenge packets) server_nonce as the associated data. this must match to decrypt.

        uint8_t additional_data[NETCODE_VERSION_INFO_BYTES + 8 + 1 + NETCODE_SERVER_NONCE_BYTES];
        int additional_data_bytes;
        {
            uint8_t * p = additional_data;
            netcode_write_bytes( &p, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
            netcode_write_uint64( &p, protocol_id );
            netcode_write_uint8( &p, prefix_byte );
            if ( header_server_nonce )
            {
                netcode_write_bytes( &p, (uint8_t*) header_server_nonce, NETCODE_SERVER_NONCE_BYTES );
            }
            additional_data_bytes = (int) ( p - additional_data );
        }

        uint8_t nonce[12];
        {
            uint8_t * p = nonce;
            netcode_write_uint32( &p, 0 );
            netcode_write_uint64( &p, sequence );
        }

        #if NETCODE_ENABLE_NONCE_AUDIT
        netcode_nonce_audit_write_packet( write_packet_key, nonce );
        #endif // #if NETCODE_ENABLE_NONCE_AUDIT

        if ( netcode_encrypt_aead( encrypted_start, 
                                   encrypted_finish - encrypted_start, 
                                   additional_data, additional_data_bytes, 
                                   nonce, write_packet_key ) != NETCODE_OK )
        {
            return NETCODE_ERROR;
        }

        buffer += NETCODE_MAC_BYTES;

        netcode_assert( buffer - start <= buffer_length );

        return (int) ( buffer - start );
    }
}

struct netcode_replay_protection_t
{
    uint64_t most_recent_sequence;
    uint64_t received_packet[NETCODE_REPLAY_PROTECTION_BUFFER_SIZE];
};

void netcode_replay_protection_reset( struct netcode_replay_protection_t * replay_protection )
{
    netcode_assert( replay_protection );
    replay_protection->most_recent_sequence = 0;
    memset( replay_protection->received_packet, 0xFF, sizeof( replay_protection->received_packet ) );
}

int netcode_replay_protection_already_received( struct netcode_replay_protection_t * replay_protection, uint64_t sequence )
{
    netcode_assert( replay_protection );

    // The maximum value is reserved for an empty receive slot.

    if ( sequence == UINT64_MAX )
        return 1;

    // written so it cannot overflow: "sequence + BUFFER_SIZE <= most_recent" wraps for
    // sequence values near UINT64_MAX and falsely rejects them as replays

    if ( replay_protection->most_recent_sequence >= NETCODE_REPLAY_PROTECTION_BUFFER_SIZE &&
         sequence <= replay_protection->most_recent_sequence - NETCODE_REPLAY_PROTECTION_BUFFER_SIZE )
        return 1;
    
    int index = (int) ( sequence % NETCODE_REPLAY_PROTECTION_BUFFER_SIZE );

    if ( replay_protection->received_packet[index] == UINT64_MAX )
        return 0;

    if ( replay_protection->received_packet[index] >= sequence )
        return 1;

    return 0;
}

void netcode_replay_protection_advance_sequence( struct netcode_replay_protection_t * replay_protection, uint64_t sequence )
{
    netcode_assert( replay_protection );

    if ( sequence == UINT64_MAX )
        return;

    if ( sequence > replay_protection->most_recent_sequence )
        replay_protection->most_recent_sequence = sequence;

    int index = (int) ( sequence % NETCODE_REPLAY_PROTECTION_BUFFER_SIZE );

    replay_protection->received_packet[index] = sequence;
}

void * netcode_read_packet( uint8_t * buffer, 
                            int buffer_length, 
                            uint64_t * sequence, 
                            uint8_t * read_packet_key, 
                            uint64_t protocol_id, 
                            uint64_t current_timestamp, 
                            uint64_t min_connect_token_expire_timestamp, 
                            uint8_t * private_key, 
                            uint8_t * allowed_packets, 
                            struct netcode_replay_protection_t * replay_protection, 
                            void * allocator_context, 
                            void* (*allocate_function)(void*,size_t) )
{
    netcode_assert( sequence );
    netcode_assert( allowed_packets );

    *sequence = 0;

    if ( allocate_function == NULL )
    {
        allocate_function = netcode_default_allocate_function;
    }

    if ( buffer_length < 1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored packet. buffer length is less than 1\n" );
        return NULL;
    }

    uint8_t * start = buffer;

    uint8_t prefix_byte = netcode_read_uint8( &buffer );

    if ( prefix_byte == NETCODE_CONNECTION_REQUEST_PACKET )
    {
        // connection request packet: first byte is zero

        if ( !allowed_packets[NETCODE_CONNECTION_REQUEST_PACKET] )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. packet type is not allowed\n" );
            return NULL;
        }

        if ( buffer_length != 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES + NETCODE_CONNECT_SALT_BYTES )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. bad packet length (expected %d, got %d)\n", 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES + NETCODE_CONNECT_SALT_BYTES, buffer_length );
            return NULL;
        }

        if ( !private_key )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. no private key\n" );
            return NULL;
        }

        uint8_t version_info[NETCODE_VERSION_INFO_BYTES];
        netcode_read_bytes( &buffer, version_info, NETCODE_VERSION_INFO_BYTES );
        if ( version_info[0]  != 'N' || 
             version_info[1]  != 'E' || 
             version_info[2]  != 'T' || 
             version_info[3]  != 'C' || 
             version_info[4]  != 'O' ||
             version_info[5]  != 'D' ||
             version_info[6]  != 'E' ||
             version_info[7]  != ' ' || 
             version_info[8]  != '1' ||
             version_info[9]  != '.' ||
             version_info[10] != '0' ||
             version_info[11] != '3' ||
             version_info[12] != '\0' )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. bad version info\n" );
            return NULL;
        }

        uint64_t packet_protocol_id = netcode_read_uint64( &buffer );
        if ( packet_protocol_id != protocol_id )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. wrong protocol id. expected %.16" PRIx64 ", got %.16" PRIx64 "\n", 
                protocol_id, packet_protocol_id );
            return NULL;
        }

        uint64_t packet_connect_token_expire_timestamp = netcode_read_uint64( &buffer );
        if ( packet_connect_token_expire_timestamp <= current_timestamp )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. connect token expired\n" );
            return NULL;
        }

        // refuse connect tokens expiring at or before min_connect_token_expire_timestamp.

        if ( packet_connect_token_expire_timestamp <= min_connect_token_expire_timestamp )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. connect token predates the server start\n" );
            return NULL;
        }

        uint8_t packet_connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
        netcode_read_bytes(&buffer, packet_connect_token_nonce, sizeof(packet_connect_token_nonce));

        netcode_assert( buffer - start == 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES );

        if ( netcode_decrypt_connect_token_private( buffer, 
                                                    NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                    version_info, 
                                                    protocol_id, 
                                                    packet_connect_token_expire_timestamp, 
                                                    packet_connect_token_nonce, 
                                                    private_key ) != NETCODE_OK )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. connect token failed to decrypt\n" );
            return NULL;
        }

        struct netcode_connection_request_packet_t * packet = (struct netcode_connection_request_packet_t*) 
            allocate_function( allocator_context, sizeof( struct netcode_connection_request_packet_t ) );

        if ( !packet )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. failed to allocate packet\n" );
            return NULL;
        }

        packet->packet_type = NETCODE_CONNECTION_REQUEST_PACKET;
        memcpy( packet->version_info, version_info, NETCODE_VERSION_INFO_BYTES );
        packet->protocol_id = packet_protocol_id;
        packet->connect_token_expire_timestamp = packet_connect_token_expire_timestamp;
        memcpy( packet->connect_token_nonce, packet_connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
        netcode_read_bytes( &buffer, packet->connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
        netcode_read_bytes( &buffer, packet->client_salt, NETCODE_CONNECT_SALT_BYTES );

        netcode_assert( buffer - start == 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES + NETCODE_CONNECT_SALT_BYTES );

        return packet;
    }
    else
    {
        // *** encrypted packets ***

        if ( !read_packet_key )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. no read packet key for this address\n" );
            return NULL;
        }

        if ( buffer_length < 1 + 1 + NETCODE_MAC_BYTES )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. packet is too small to be valid (%d bytes)\n", buffer_length );
            return NULL;
        }

        // extract the packet type and number of sequence bytes from the prefix byte

        int packet_type = prefix_byte & 0xF;

        if ( packet_type >= NETCODE_CONNECTION_NUM_PACKETS )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. packet type %d is invalid\n", packet_type );
            return NULL;
        }

        if ( !allowed_packets[packet_type] )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. packet type %d is not allowed\n", packet_type );
            return NULL;
        }

        int sequence_bytes = prefix_byte >> 4;

        if ( sequence_bytes < 1 || sequence_bytes > 8 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. sequence bytes %d is out of range [1,8]\n", sequence_bytes );
            return NULL;
        }

        int header_nonce_bytes = ( packet_type == NETCODE_CONNECTION_DENIED_PACKET ||
                                   packet_type == NETCODE_CONNECTION_CHALLENGE_PACKET )
                               ? NETCODE_SERVER_NONCE_BYTES : 0;

        if ( buffer_length < 1 + sequence_bytes + header_nonce_bytes + NETCODE_MAC_BYTES )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. buffer is too small for sequence bytes + encryption mac\n" );
            return NULL;
        }

        // read variable length sequence number [1,8]

        int i;
        for ( i = 0; i < sequence_bytes; i++ )
        {
            uint8_t value = netcode_read_uint8( &buffer );
            (*sequence) |= ( uint64_t) ( value ) << ( 8 * i );
        }

        // The maximum value is reserved by replay protection.

        if ( *sequence == UINT64_MAX )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. sequence is invalid\n" );
            return NULL;
        }

        // ignore the packet if it has already been received

        if ( replay_protection && packet_type >= NETCODE_CONNECTION_KEEP_ALIVE_PACKET )
        {
            if ( netcode_replay_protection_already_received( replay_protection, *sequence ) )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored packet. sequence %.16" PRIx64 " already received (replay protection)\n", *sequence );
                return NULL;
            }
        }

        uint8_t header_server_nonce[NETCODE_SERVER_NONCE_BYTES];
        if ( header_nonce_bytes > 0 )
        {
            netcode_read_bytes( &buffer, header_server_nonce, NETCODE_SERVER_NONCE_BYTES );
        }

        // decrypt the per-packet type data

        uint8_t additional_data[NETCODE_VERSION_INFO_BYTES + 8 + 1 + NETCODE_SERVER_NONCE_BYTES];
        int additional_data_bytes;
        {
            uint8_t * p = additional_data;
            netcode_write_bytes( &p, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
            netcode_write_uint64( &p, protocol_id );
            netcode_write_uint8( &p, prefix_byte );
            if ( header_nonce_bytes > 0 )
            {
                netcode_write_bytes( &p, header_server_nonce, NETCODE_SERVER_NONCE_BYTES );
            }
            additional_data_bytes = (int) ( p - additional_data );
        }

        uint8_t nonce[12];
        {
            uint8_t * p = nonce;
            netcode_write_uint32( &p, 0 );
            netcode_write_uint64( &p, *sequence );
        }

        int encrypted_bytes = (int) ( buffer_length - ( buffer - start ) );

        if ( encrypted_bytes < NETCODE_MAC_BYTES )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. encrypted payload is too small\n" );
            return NULL;
        }

        if ( netcode_decrypt_aead( buffer, encrypted_bytes, additional_data, additional_data_bytes, nonce, read_packet_key ) != NETCODE_OK )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. failed to decrypt\n" );
            return NULL;
        }

        int decrypted_bytes = encrypted_bytes - NETCODE_MAC_BYTES;

        // update the latest replay protection sequence #

        if ( replay_protection && packet_type >= NETCODE_CONNECTION_KEEP_ALIVE_PACKET )
        {
            netcode_replay_protection_advance_sequence( replay_protection, *sequence );
        }

        // process the per-packet type data that was just decrypted
        
        switch ( packet_type )
        {
            case NETCODE_CONNECTION_DENIED_PACKET:
            {
                if ( decrypted_bytes != 0 )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection denied packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_denied_packet_t * packet = (struct netcode_connection_denied_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_denied_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection denied packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_DENIED_PACKET;
                memcpy( packet->server_nonce, header_server_nonce, NETCODE_SERVER_NONCE_BYTES );
                
                return packet;
            }
            break;

            case NETCODE_CONNECTION_CHALLENGE_PACKET:
            {
                if ( decrypted_bytes != 8 + NETCODE_CHALLENGE_TOKEN_BYTES )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection challenge packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_challenge_packet_t * packet = (struct netcode_connection_challenge_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_challenge_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection challenge packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_CHALLENGE_PACKET;
                memcpy( packet->server_nonce, header_server_nonce, NETCODE_SERVER_NONCE_BYTES );
                packet->challenge_token_sequence = netcode_read_uint64( &buffer );
                netcode_read_bytes( &buffer, packet->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
                
                return packet;
            }
            break;

            case NETCODE_CONNECTION_RESPONSE_PACKET:
            {
                if ( decrypted_bytes != 8 + NETCODE_CHALLENGE_TOKEN_BYTES )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection response packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_response_packet_t * packet = (struct netcode_connection_response_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_response_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection response packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_RESPONSE_PACKET;
                packet->challenge_token_sequence = netcode_read_uint64( &buffer );
                netcode_read_bytes( &buffer, packet->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
                
                return packet;
            }
            break;

            case NETCODE_CONNECTION_KEEP_ALIVE_PACKET:
            {
                if ( decrypted_bytes != 8 )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection keep alive packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_keep_alive_packet_t * packet = (struct netcode_connection_keep_alive_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_keep_alive_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection keep alive packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
                packet->client_index = netcode_read_uint32( &buffer );
                packet->max_clients = netcode_read_uint32( &buffer );
                
                return packet;
            }
            break;
            
            case NETCODE_CONNECTION_PAYLOAD_PACKET:
            {
                if ( decrypted_bytes < 1 )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection payload packet. payload is too small\n" );
                    return NULL;
                }

                if ( decrypted_bytes > NETCODE_MAX_PAYLOAD_BYTES )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection payload packet. payload is too large\n" );
                    return NULL;
                }

                struct netcode_connection_payload_packet_t * packet = netcode_create_payload_packet( decrypted_bytes, allocator_context, allocate_function );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection payload packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                memcpy( packet->payload_data, buffer, decrypted_bytes );
                
                return packet;
            }
            break;

            case NETCODE_CONNECTION_DISCONNECT_PACKET:
            {
                if ( decrypted_bytes != 0 )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection disconnect packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_disconnect_packet_t * packet = (struct netcode_connection_disconnect_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_disconnect_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection disconnect packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;
                
                return packet;
            }
            break;

            default:
                return NULL;
        }
    }
}

// ----------------------------------------------------------------

struct netcode_connect_token_t
{
    uint8_t version_info[NETCODE_VERSION_INFO_BYTES];
    uint64_t protocol_id;
    uint64_t create_timestamp;
    uint64_t expire_timestamp;
    uint8_t nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    uint8_t private_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    int timeout_seconds;
    int num_server_addresses;
    struct netcode_address_t server_addresses[NETCODE_MAX_SERVERS_PER_CONNECT];
    uint8_t client_to_server_key[NETCODE_KEY_BYTES];
    uint8_t server_to_client_key[NETCODE_KEY_BYTES];
};

void netcode_write_connect_token( struct netcode_connect_token_t * connect_token, uint8_t * buffer, int buffer_length )
{
    netcode_assert( connect_token );
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CONNECT_TOKEN_BYTES );

    uint8_t * start = buffer;

    (void) start;
    (void) buffer_length;

    netcode_write_bytes( &buffer, connect_token->version_info, NETCODE_VERSION_INFO_BYTES );

    netcode_write_uint64( &buffer, connect_token->protocol_id );

    netcode_write_uint64( &buffer, connect_token->create_timestamp );

    netcode_write_uint64( &buffer, connect_token->expire_timestamp );

    netcode_write_bytes( &buffer, connect_token->nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );

    netcode_write_bytes( &buffer, connect_token->private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    int i,j;

    netcode_write_uint32( &buffer, connect_token->timeout_seconds );

    netcode_write_uint32( &buffer, connect_token->num_server_addresses );

    for ( i = 0; i < connect_token->num_server_addresses; i++ )
    {
        if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV4 )
        {
            netcode_write_uint8( &buffer, NETCODE_ADDRESS_IPV4 );
            for ( j = 0; j < 4; j++ )
            {
                netcode_write_uint8( &buffer, connect_token->server_addresses[i].data.ipv4[j] );
            }
            netcode_write_uint16( &buffer, connect_token->server_addresses[i].port );
        }
        else if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV6 )
        {
            netcode_write_uint8( &buffer, NETCODE_ADDRESS_IPV6 );
            for ( j = 0; j < 8; j++ )
            {
                netcode_write_uint16( &buffer, connect_token->server_addresses[i].data.ipv6[j] );
            }
            netcode_write_uint16( &buffer, connect_token->server_addresses[i].port );
        }
        else
        {
            netcode_assert( 0 );
        }
    }

    netcode_write_bytes( &buffer, connect_token->client_to_server_key, NETCODE_KEY_BYTES );

    netcode_write_bytes( &buffer, connect_token->server_to_client_key, NETCODE_KEY_BYTES );

    netcode_assert( buffer - start <= NETCODE_CONNECT_TOKEN_BYTES );

    memset( buffer, 0, NETCODE_CONNECT_TOKEN_BYTES - ( buffer - start ) );
}

int netcode_read_connect_token( uint8_t * buffer, int buffer_length, struct netcode_connect_token_t * connect_token )
{
    netcode_assert( buffer );
    netcode_assert( connect_token );

    if ( buffer_length != NETCODE_CONNECT_TOKEN_BYTES )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: read connect data has bad buffer length (%d)\n", buffer_length );
        return NETCODE_ERROR;
    }

    netcode_read_bytes( &buffer, connect_token->version_info, NETCODE_VERSION_INFO_BYTES );
    if ( connect_token->version_info[0]  != 'N' || 
         connect_token->version_info[1]  != 'E' || 
         connect_token->version_info[2]  != 'T' || 
         connect_token->version_info[3]  != 'C' || 
         connect_token->version_info[4]  != 'O' ||
         connect_token->version_info[5]  != 'D' ||
         connect_token->version_info[6]  != 'E' ||
         connect_token->version_info[7]  != ' ' || 
         connect_token->version_info[8]  != '1' ||
         connect_token->version_info[9]  != '.' ||
         connect_token->version_info[10] != '0' ||
         connect_token->version_info[11] != '3' ||
         connect_token->version_info[12] != '\0' )
    {
        connect_token->version_info[12] = '\0';
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: read connect data has bad version info (got %s, expected %s)\n", connect_token->version_info, NETCODE_VERSION_INFO );
        return NETCODE_ERROR;
    }

    connect_token->protocol_id = netcode_read_uint64( &buffer );

    connect_token->create_timestamp = netcode_read_uint64( &buffer );

    connect_token->expire_timestamp = netcode_read_uint64( &buffer );

    if ( connect_token->create_timestamp > connect_token->expire_timestamp )
        return NETCODE_ERROR;

    netcode_read_bytes( &buffer, connect_token->nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );

    netcode_read_bytes( &buffer, connect_token->private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    connect_token->timeout_seconds = (int) netcode_read_uint32( &buffer );

    connect_token->num_server_addresses = netcode_read_uint32( &buffer );

    if ( connect_token->num_server_addresses <= 0 || connect_token->num_server_addresses > NETCODE_MAX_SERVERS_PER_CONNECT )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: read connect data has bad number of server addresses (%d)\n", connect_token->num_server_addresses );
        return NETCODE_ERROR;
    }

    int i,j;

    for ( i = 0; i < connect_token->num_server_addresses; i++ )
    {
        connect_token->server_addresses[i].type = netcode_read_uint8( &buffer );

        if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV4 )
        {
            for ( j = 0; j < 4; j++ )
            {
                connect_token->server_addresses[i].data.ipv4[j] = netcode_read_uint8( &buffer );
            }
            connect_token->server_addresses[i].port = netcode_read_uint16( &buffer );
        }
        else if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV6 )
        {
            for ( j = 0; j < 8; j++ )
            {
                connect_token->server_addresses[i].data.ipv6[j] = netcode_read_uint16( &buffer );
            }
            connect_token->server_addresses[i].port = netcode_read_uint16( &buffer );
        }
        else
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: read connect data has bad address type (%d)\n", connect_token->server_addresses[i].type );
            return NETCODE_ERROR;
        }
    }

    netcode_read_bytes( &buffer, connect_token->client_to_server_key, NETCODE_KEY_BYTES );

    netcode_read_bytes( &buffer, connect_token->server_to_client_key, NETCODE_KEY_BYTES );
    
    return NETCODE_OK;
}

// ----------------------------------------------------------------

struct netcode_packet_queue_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    int num_packets;
    int start_index;
    void * packet_data[NETCODE_PACKET_QUEUE_SIZE];
    uint64_t packet_sequence[NETCODE_PACKET_QUEUE_SIZE];
};

void netcode_packet_queue_init( struct netcode_packet_queue_t * queue, 
                                void * allocator_context, 
                                void * (*allocate_function)(void*,size_t), 
                                void (*free_function)(void*,void*) )
{
    if ( allocate_function == NULL )
    {
        allocate_function = netcode_default_allocate_function;
    }

    if ( free_function == NULL )
    {
        free_function = netcode_default_free_function;
    }

    netcode_assert( queue );

    queue->allocator_context = allocator_context;
    queue->allocate_function = allocate_function;
    queue->free_function = free_function;
    queue->num_packets = 0;
    queue->start_index = 0;
    memset( queue->packet_data, 0, sizeof( queue->packet_data ) );
    memset( queue->packet_sequence, 0, sizeof( queue->packet_sequence ) );
}

int netcode_packet_queue_push( struct netcode_packet_queue_t * queue, void * packet_data, uint64_t packet_sequence )
{
    netcode_assert( queue );
    netcode_assert( packet_data );
    if ( queue->num_packets == NETCODE_PACKET_QUEUE_SIZE )
    {
        queue->free_function( queue->allocator_context, packet_data );
        return 0;
    }
    int index = ( queue->start_index + queue->num_packets ) % NETCODE_PACKET_QUEUE_SIZE;
    queue->packet_data[index] = packet_data;
    queue->packet_sequence[index] = packet_sequence;
    queue->num_packets++;
    return 1;
}

void * netcode_packet_queue_pop( struct netcode_packet_queue_t * queue, uint64_t * packet_sequence )
{
    if ( queue->num_packets == 0 )
        return NULL;
    void * packet = queue->packet_data[queue->start_index];
    if ( packet_sequence )
        *packet_sequence = queue->packet_sequence[queue->start_index];
    queue->start_index = ( queue->start_index + 1 ) % NETCODE_PACKET_QUEUE_SIZE;
    queue->num_packets--;
    return packet;
}

void netcode_packet_queue_clear( struct netcode_packet_queue_t * queue )
{
    netcode_assert( queue );
    while ( queue->num_packets > 0 )
    {
        queue->free_function( queue->allocator_context, netcode_packet_queue_pop( queue, NULL ) );
    }
    queue->start_index = 0;
    memset( queue->packet_data, 0, sizeof( queue->packet_data ) );
    memset( queue->packet_sequence, 0, sizeof( queue->packet_sequence ) );
}

// ----------------------------------------------------------------

#define NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES ( NETCODE_MAX_CLIENTS * 256 )
#define NETCODE_NETWORK_SIMULATOR_NUM_PENDING_RECEIVE_PACKETS ( NETCODE_MAX_CLIENTS * 64 )
#define NETCODE_NETWORK_SIMULATOR_RNG_SEED 0x9E3779B97F4A7C15ULL

struct netcode_network_simulator_packet_entry_t
{
    struct netcode_address_t from;
    struct netcode_address_t to;
    double delivery_time;
    uint8_t * packet_data;
    int packet_bytes;
};

struct netcode_network_simulator_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    float latency_milliseconds;
    float jitter_milliseconds;
    float packet_loss_percent;
    float duplicate_packet_percent;
    uint64_t rng_state;
    double time;
    int current_index;
    int num_pending_receive_packets;
    struct netcode_network_simulator_packet_entry_t packet_entries[NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES];
    struct netcode_network_simulator_packet_entry_t pending_receive_packets[NETCODE_NETWORK_SIMULATOR_NUM_PENDING_RECEIVE_PACKETS];
};

struct netcode_network_simulator_t * netcode_network_simulator_create( void * allocator_context, 
                                                                       void * (*allocate_function)(void*,size_t), 
                                                                       void (*free_function)(void*,void*) )
{
    if ( allocate_function == NULL )
    {
        allocate_function = netcode_default_allocate_function;
    }

    if ( free_function == NULL )
    {
        free_function = netcode_default_free_function;
    }

    struct netcode_network_simulator_t * network_simulator = (struct netcode_network_simulator_t*) 
        allocate_function( allocator_context, sizeof( struct netcode_network_simulator_t ) );

    netcode_assert( network_simulator );

    memset( network_simulator, 0, sizeof( struct netcode_network_simulator_t ) );

    network_simulator->allocator_context = allocator_context;
    network_simulator->allocate_function = allocate_function;
    network_simulator->free_function = free_function;
    network_simulator->rng_state = NETCODE_NETWORK_SIMULATOR_RNG_SEED;

    return network_simulator;
}

void netcode_network_simulator_reset( struct netcode_network_simulator_t * network_simulator )
{
    netcode_assert( network_simulator );

    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "network simulator reset\n" );

    int i;
    for ( i = 0; i < NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES; i++ )
    {
        network_simulator->free_function( network_simulator->allocator_context, network_simulator->packet_entries[i].packet_data );
        memset( &network_simulator->packet_entries[i], 0, sizeof( struct netcode_network_simulator_packet_entry_t ) );
    }

    for ( i = 0; i < network_simulator->num_pending_receive_packets; i++ )
    {
        network_simulator->free_function( network_simulator->allocator_context, network_simulator->pending_receive_packets[i].packet_data );
        memset( &network_simulator->pending_receive_packets[i], 0, sizeof( struct netcode_network_simulator_packet_entry_t ) );
    }

    network_simulator->current_index = 0;
    network_simulator->num_pending_receive_packets = 0;
    network_simulator->rng_state = NETCODE_NETWORK_SIMULATOR_RNG_SEED;
}

void netcode_network_simulator_destroy( struct netcode_network_simulator_t * network_simulator )
{
    netcode_assert( network_simulator );
    netcode_network_simulator_reset( network_simulator );
    network_simulator->free_function( network_simulator->allocator_context, network_simulator );
}

static uint64_t netcode_network_simulator_random_uint64( struct netcode_network_simulator_t * network_simulator )
{
    // xorshift64*. self-contained and deterministic, unlike rand(): the simulator
    // produces the same loss, jitter and duplication sequence on every run, and
    // shares no state with the application or other simulator instances.

    uint64_t x = network_simulator->rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    network_simulator->rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static float netcode_network_simulator_random_float( struct netcode_network_simulator_t * network_simulator, float a, float b )
{
    netcode_assert( a < b );
    float random = (float) ( netcode_network_simulator_random_uint64( network_simulator ) >> 40 ) / (float) ( 1 << 24 );
    return a + random * ( b - a );
}

void netcode_network_simulator_queue_packet( struct netcode_network_simulator_t * network_simulator, 
                                             struct netcode_address_t * from, 
                                             struct netcode_address_t * to, 
                                             uint8_t * packet_data, 
                                             int packet_bytes, 
                                             float delay )
{
    // allocate before touching the slot. if the allocator fails, drop this packet and leave
    // the entry already queued in that slot alone, rather than copying into null.

    uint8_t * entry_packet_data = (uint8_t*) network_simulator->allocate_function( network_simulator->allocator_context, packet_bytes );

    if ( !entry_packet_data )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: network simulator could not allocate packet data\n" );
        return;
    }

    if ( network_simulator->packet_entries[network_simulator->current_index].packet_data )
    {
        network_simulator->free_function( network_simulator->allocator_context, network_simulator->packet_entries[network_simulator->current_index].packet_data );
        network_simulator->packet_entries[network_simulator->current_index].packet_data = NULL;
    }

    network_simulator->packet_entries[network_simulator->current_index].from = *from;
    network_simulator->packet_entries[network_simulator->current_index].to = *to;
    network_simulator->packet_entries[network_simulator->current_index].packet_data = entry_packet_data;
    memcpy( network_simulator->packet_entries[network_simulator->current_index].packet_data, packet_data, packet_bytes );
    network_simulator->packet_entries[network_simulator->current_index].packet_bytes = packet_bytes;
    network_simulator->packet_entries[network_simulator->current_index].delivery_time = network_simulator->time + delay;
    network_simulator->current_index++;
    network_simulator->current_index %= NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES;
}

void netcode_network_simulator_send_packet( struct netcode_network_simulator_t * network_simulator, 
                                            struct netcode_address_t * from, 
                                            struct netcode_address_t * to, 
                                            uint8_t * packet_data, 
                                            int packet_bytes )
{
    netcode_assert( network_simulator );
    netcode_assert( from );
    netcode_assert( from->type != 0 );
    netcode_assert( to );
    netcode_assert( to->type != 0 );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes > 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    if ( netcode_network_simulator_random_float( network_simulator, 0.0f, 100.0f ) <= network_simulator->packet_loss_percent )
        return;

    float delay = network_simulator->latency_milliseconds / 1000.0f;

    if ( network_simulator->jitter_milliseconds > 0.0 )
        delay += netcode_network_simulator_random_float( network_simulator, -network_simulator->jitter_milliseconds, +network_simulator->jitter_milliseconds ) / 1000.0f;

    netcode_network_simulator_queue_packet( network_simulator, from, to, packet_data, packet_bytes, delay );

    if ( netcode_network_simulator_random_float( network_simulator, 0.0f, 100.0f ) <= network_simulator->duplicate_packet_percent )
    {
        netcode_network_simulator_queue_packet( network_simulator, from, to, packet_data, packet_bytes, delay + netcode_network_simulator_random_float( network_simulator, 0, 1.0 ) );
    }
}

int netcode_network_simulator_receive_packets( struct netcode_network_simulator_t * network_simulator, 
                                               struct netcode_address_t * to, 
                                               int max_packets, 
                                               uint8_t ** packet_data, 
                                               int * packet_bytes, 
                                               struct netcode_address_t * from )
{
    netcode_assert( network_simulator );
    netcode_assert( max_packets >= 0 );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes );
    netcode_assert( from );
    netcode_assert( to );

    int num_packets = 0;

    int i;
    for ( i = 0; i < network_simulator->num_pending_receive_packets; i++ )
    {
        if ( num_packets == max_packets )
            break;

        if ( !network_simulator->pending_receive_packets[i].packet_data )
            continue;

        if ( !netcode_address_equal( &network_simulator->pending_receive_packets[i].to, to ) )
            continue;

        packet_data[num_packets] = network_simulator->pending_receive_packets[i].packet_data;
        packet_bytes[num_packets] = network_simulator->pending_receive_packets[i].packet_bytes;
        from[num_packets] = network_simulator->pending_receive_packets[i].from;

        network_simulator->pending_receive_packets[i].packet_data = NULL;

        num_packets++;
    }

    netcode_assert( num_packets <= max_packets );

    return num_packets;
}

void netcode_network_simulator_update( struct netcode_network_simulator_t * network_simulator, double time )
{   
    netcode_assert( network_simulator );

    network_simulator->time = time;

    // discard any pending receive packets that are still in the buffer

    int i;
    for ( i = 0; i < network_simulator->num_pending_receive_packets; i++ )
    {
        if ( network_simulator->pending_receive_packets[i].packet_data )
        {
            network_simulator->free_function( network_simulator->allocator_context, network_simulator->pending_receive_packets[i].packet_data );
            network_simulator->pending_receive_packets[i].packet_data = NULL;
        }
    }

    network_simulator->num_pending_receive_packets = 0;

    // walk across packet entries and move any that are ready to be received into the pending receive buffer

    for ( i = 0; i < NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES; i++ )
    {
        if ( !network_simulator->packet_entries[i].packet_data )
            continue;

        if ( network_simulator->num_pending_receive_packets == NETCODE_NETWORK_SIMULATOR_NUM_PENDING_RECEIVE_PACKETS )
            break;

        if ( network_simulator->packet_entries[i].packet_data && network_simulator->packet_entries[i].delivery_time <= time )
        {
            network_simulator->pending_receive_packets[network_simulator->num_pending_receive_packets] = network_simulator->packet_entries[i];
            network_simulator->num_pending_receive_packets++;
            network_simulator->packet_entries[i].packet_data = NULL;
        }
    }
}

// ----------------------------------------------------------------

// shared by the client and server send paths: dispatch a written packet to the network
// simulator, the send override, or the socket matching the destination address family

static void netcode_send_packet_to_address( struct netcode_network_simulator_t * network_simulator,
                                            void * override_context,
                                            void (*send_packet_override)(void*,struct netcode_address_t*,NETCODE_CONST uint8_t*,int),
                                            struct netcode_socket_holder_t * socket_holder,
                                            struct netcode_address_t * from,
                                            struct netcode_address_t * to,
                                            uint8_t * packet_data,
                                            int packet_bytes )
{
    if ( network_simulator )
    {
        netcode_network_simulator_send_packet( network_simulator, from, to, packet_data, packet_bytes );
    }
    else if ( send_packet_override )
    {
        send_packet_override( override_context, to, packet_data, packet_bytes );
    }
    else if ( to->type == NETCODE_ADDRESS_IPV4 )
    {
        netcode_socket_send_packet( &socket_holder->ipv4, to, packet_data, packet_bytes );
    }
    else if ( to->type == NETCODE_ADDRESS_IPV6 )
    {
        netcode_socket_send_packet( &socket_holder->ipv6, to, packet_data, packet_bytes );
    }
}

// ----------------------------------------------------------------

NETCODE_CONST char * netcode_client_state_name( int client_state )
{
    switch ( client_state )
    {
        case NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED:                return "connect token expired";
        case NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN:                return "invalid connect token";
        case NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT:                 return "connection timed out";
        case NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT:         return "connection request timed out";
        case NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT:        return "connection response timed out";
        case NETCODE_CLIENT_STATE_CONNECTION_DENIED:                    return "connection denied";
        case NETCODE_CLIENT_STATE_DISCONNECTED:                         return "disconnected";
        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST:           return "sending connection request";
        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE:          return "sending connection response";
        case NETCODE_CLIENT_STATE_CONNECTED:                            return "connected";
        default:
            netcode_assert( 0 );
            return "???";
    }
}

void netcode_default_client_config( struct netcode_client_config_t * config )
{
    netcode_assert( config );
    config->allocator_context = NULL;
    config->allocate_function = netcode_default_allocate_function;
    config->free_function = netcode_default_free_function;
    config->network_simulator = NULL;
    config->callback_context = NULL;
    config->state_change_callback = NULL;
    config->send_loopback_packet_callback = NULL;
    config->override_send_and_receive = 0;
    config->send_packet_override = NULL;
    config->receive_packet_override = NULL;
}

struct netcode_client_t
{
    struct netcode_client_config_t config;
    int state;
    double time;
    double connect_start_time;
    double last_packet_send_time;
    double last_packet_receive_time;
    int should_disconnect;
    int should_disconnect_state;
    uint64_t sequence;
    int client_index;
    int max_clients;
    int server_address_index;
    struct netcode_address_t address;
    struct netcode_address_t server_address;
    struct netcode_connect_token_t connect_token;
    struct netcode_socket_holder_t socket_holder;
    struct netcode_context_t context;
    struct netcode_replay_protection_t replay_protection;
    struct netcode_packet_queue_t packet_receive_queue;
    uint8_t client_salt[NETCODE_CONNECT_SALT_BYTES];
    uint8_t server_nonce[NETCODE_SERVER_NONCE_BYTES];
    int has_server_nonce;
    uint64_t challenge_token_sequence;
    uint8_t challenge_token_data[NETCODE_CHALLENGE_TOKEN_BYTES];
    uint8_t * receive_packet_data[NETCODE_CLIENT_MAX_RECEIVE_PACKETS];
    int receive_packet_bytes[NETCODE_CLIENT_MAX_RECEIVE_PACKETS];
    struct netcode_address_t receive_from[NETCODE_CLIENT_MAX_RECEIVE_PACKETS];
    int loopback;
};

static int client_create_error;

int netcode_client_create_error()
{
    return client_create_error;
}

int netcode_client_socket_create( struct netcode_socket_t * socket,
                                  struct netcode_address_t * address,
                                  int send_buffer_size,
                                  int receive_buffer_size,
                                  NETCODE_CONST struct netcode_client_config_t * config )
{
    netcode_assert( socket );
    netcode_assert( address );
    netcode_assert( config );

    if ( !config->network_simulator )
    {
        if ( !config->override_send_and_receive )
        {
            if ( netcode_socket_create( socket, address, send_buffer_size, receive_buffer_size ) != NETCODE_SOCKET_ERROR_NONE )
            {
                client_create_error = ( address->type == NETCODE_ADDRESS_IPV6 ) ? NETCODE_CLIENT_CREATE_ERROR_CREATE_SOCKET_IPV6_FAILED
                                                                                : NETCODE_CLIENT_CREATE_ERROR_CREATE_SOCKET_IPV4_FAILED;
                return 0;
            }
        }
    }
    else
    {
        if ( address->port == 0 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: must bind to a specific port when using network simulator\n" );
            client_create_error = NETCODE_CLIENT_CREATE_ERROR_SIMULATOR_REQUIRES_PORT;
            return 0;
        }
    }

    return 1;
}

struct netcode_client_t * netcode_client_create_dual( NETCODE_CONST char * address1_string,
                                                          NETCODE_CONST char * address2_string,
                                                          NETCODE_CONST struct netcode_client_config_t * config,
                                                          double time )
{
    netcode_assert( config );
    netcode_assert( netcode.initialized );

    client_create_error = NETCODE_CLIENT_CREATE_ERROR_NONE;

    // tolerate a zeroed config: default the allocator functions so a forgotten
    // netcode_default_client_config is an inconvenience, not a crash

    struct netcode_client_config_t config_copy = *config;
    if ( !config_copy.allocate_function )
        config_copy.allocate_function = netcode_default_allocate_function;
    if ( !config_copy.free_function )
        config_copy.free_function = netcode_default_free_function;
    config = &config_copy;

    // the overrides are called on the update path with no null check. a missing one is a
    // configuration error, refused here rather than dereferenced on the first update.

    if ( config->override_send_and_receive && ( !config->send_packet_override || !config->receive_packet_override ) )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: override_send_and_receive requires both send_packet_override and receive_packet_override\n" );
        client_create_error = NETCODE_CLIENT_CREATE_ERROR_MISSING_OVERRIDE_CALLBACK;
        return NULL;
    }

    struct netcode_address_t address1;
    struct netcode_address_t address2;

    memset( &address1, 0, sizeof( address1 ) );
    memset( &address2, 0, sizeof( address2 ) );

    if ( netcode_parse_address( address1_string, &address1 ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to parse client address\n" );
        client_create_error = NETCODE_CLIENT_CREATE_ERROR_PARSE_ADDRESS_FAILED;
        return NULL;
    }

    if ( address2_string != NULL && netcode_parse_address( address2_string, &address2 ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to parse client address2\n" );
        client_create_error = NETCODE_CLIENT_CREATE_ERROR_PARSE_ADDRESS2_FAILED;
        return NULL;
    }


    struct netcode_socket_t socket_ipv4;
    struct netcode_socket_t socket_ipv6;

    netcode_socket_init( &socket_ipv4 );
    netcode_socket_init( &socket_ipv6 );

    if ( address1.type == NETCODE_ADDRESS_IPV4 || address2.type == NETCODE_ADDRESS_IPV4 )
    {
        if ( !netcode_client_socket_create( &socket_ipv4, address1.type == NETCODE_ADDRESS_IPV4 ? &address1 : &address2, NETCODE_CLIENT_SOCKET_SNDBUF_SIZE, NETCODE_CLIENT_SOCKET_RCVBUF_SIZE, config ) )
        {
            return NULL;
        }
    }

    if ( address1.type == NETCODE_ADDRESS_IPV6 || address2.type == NETCODE_ADDRESS_IPV6 )
    {
        if ( !netcode_client_socket_create( &socket_ipv6, address1.type == NETCODE_ADDRESS_IPV6 ? &address1 : &address2, NETCODE_CLIENT_SOCKET_SNDBUF_SIZE, NETCODE_CLIENT_SOCKET_RCVBUF_SIZE, config ) )
        {
            netcode_socket_destroy( &socket_ipv4 );
            return NULL;
        }
    }

    struct netcode_client_t * client = (struct netcode_client_t*) config->allocate_function( config->allocator_context, sizeof( struct netcode_client_t ) );

    if ( !client )
    {
        netcode_socket_destroy( &socket_ipv4 );
        netcode_socket_destroy( &socket_ipv6 );
        client_create_error = NETCODE_CLIENT_CREATE_ERROR_ALLOCATE_CLIENT_FAILED;
        return NULL;
    }

    struct netcode_address_t socket_address = address1.type == NETCODE_ADDRESS_IPV4 ? socket_ipv4.address : socket_ipv6.address;

    if ( !config->network_simulator )
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "client started on port %d\n", socket_address.port );
    }
    else
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "client started on port %d (network simulator)\n", socket_address.port );
    }

    client->config = *config;
    client->socket_holder.ipv4 = socket_ipv4;
    client->socket_holder.ipv6 = socket_ipv6;
    client->address = config->network_simulator ? address1 : socket_address;
    client->state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->time = time;
    client->connect_start_time = 0.0;
    client->last_packet_send_time = -1000.0;
    client->last_packet_receive_time = -1000.0;
    client->should_disconnect = 0;
    client->should_disconnect_state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->sequence = 0;
    client->client_index = 0;
    client->max_clients = 0;
    client->server_address_index = 0;
    client->has_server_nonce = 0;
    client->challenge_token_sequence = 0;
    client->loopback = 0;
    memset( &client->server_address, 0, sizeof( struct netcode_address_t ) );
    sodium_memzero( &client->connect_token, sizeof( struct netcode_connect_token_t ) );
    sodium_memzero( &client->context, sizeof( struct netcode_context_t ) );
    sodium_memzero( client->client_salt, sizeof( client->client_salt ) );
    memset( client->server_nonce, 0, sizeof( client->server_nonce ) );
    memset( client->challenge_token_data, 0, NETCODE_CHALLENGE_TOKEN_BYTES );

    netcode_packet_queue_init( &client->packet_receive_queue, config->allocator_context, config->allocate_function, config->free_function );

    netcode_replay_protection_reset( &client->replay_protection );

    return client;
}

struct netcode_client_t * netcode_client_create( NETCODE_CONST char * address,
                                                 NETCODE_CONST struct netcode_client_config_t * config,
                                                 double time )
{
    return netcode_client_create_dual( address, NULL, config, time );
}

void netcode_client_destroy( struct netcode_client_t * client )
{
    netcode_assert( client );
    if ( !client->loopback )
        netcode_client_disconnect( client );
    else
        netcode_client_disconnect_loopback( client );
    netcode_socket_destroy( &client->socket_holder.ipv4 );
    netcode_socket_destroy( &client->socket_holder.ipv6 );
    netcode_packet_queue_clear( &client->packet_receive_queue );
    client->config.free_function( client->config.allocator_context, client );
}

void netcode_client_set_state( struct netcode_client_t * client, int client_state )
{
    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client changed state from '%s' to '%s'\n", 
        netcode_client_state_name( client->state ), netcode_client_state_name( client_state ) );

    if ( client->config.state_change_callback )
    {
        client->config.state_change_callback( client->config.callback_context, client->state, client_state );
    }

    client->state = client_state;
}

void netcode_client_reset_before_next_connect( struct netcode_client_t * client )
{
    client->last_packet_send_time = client->time - 1.0f;
    client->last_packet_receive_time = client->time;
    client->should_disconnect = 0;
    client->should_disconnect_state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->has_server_nonce = 0;
    client->challenge_token_sequence = 0;

    netcode_random_bytes( client->client_salt, NETCODE_CONNECT_SALT_BYTES );
    memset( client->server_nonce, 0, NETCODE_SERVER_NONCE_BYTES );
    sodium_memzero( &client->context, sizeof( struct netcode_context_t ) );
    memset( client->challenge_token_data, 0, NETCODE_CHALLENGE_TOKEN_BYTES );

    netcode_replay_protection_reset( &client->replay_protection );
}

void netcode_client_reset_connection_data( struct netcode_client_t * client, int client_state )
{
    netcode_assert( client );

    client->sequence = 0;
    client->loopback = 0;
    client->client_index = 0;
    client->max_clients = 0;
    client->connect_start_time = 0.0;
    client->server_address_index = 0;
    memset( &client->server_address, 0, sizeof( struct netcode_address_t ) );
    sodium_memzero( &client->connect_token, sizeof( struct netcode_connect_token_t ) );
    sodium_memzero( &client->context, sizeof( struct netcode_context_t ) );

    netcode_client_set_state( client, client_state );

    netcode_client_reset_before_next_connect( client );
    sodium_memzero( client->client_salt, sizeof( client->client_salt ) );

    netcode_packet_queue_clear( &client->packet_receive_queue );
}

void netcode_client_disconnect_internal( struct netcode_client_t * client, int destination_state, int send_disconnect_packets );

static int netcode_client_address_family_usable( struct netcode_client_t * client, int address_type )
{
    // a network simulator or send/receive override reaches every family without a socket

    if ( client->config.network_simulator || client->config.override_send_and_receive )
        return 1;

    if ( address_type == NETCODE_ADDRESS_IPV4 )
        return netcode_socket_handle_valid( client->socket_holder.ipv4.handle );

    if ( address_type == NETCODE_ADDRESS_IPV6 )
        return netcode_socket_handle_valid( client->socket_holder.ipv6.handle );

    return 0;
}

void netcode_client_connect( struct netcode_client_t * client, uint8_t * connect_token )
{
    netcode_assert( client );
    netcode_assert( connect_token );

    netcode_client_disconnect( client );

    if ( netcode_read_connect_token( connect_token, NETCODE_CONNECT_TOKEN_BYTES, &client->connect_token ) != NETCODE_OK )
    {
        netcode_client_set_state( client, NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN );
        return;
    }

    int server_address_index = 0;

    while ( server_address_index < client->connect_token.num_server_addresses &&
            !netcode_client_address_family_usable( client, client->connect_token.server_addresses[server_address_index].type ) )
    {
        server_address_index++;
    }

    if ( server_address_index >= client->connect_token.num_server_addresses )
    {
        netcode_client_set_state( client, NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN );
        return;
    }

    client->server_address_index = server_address_index;
    client->server_address = client->connect_token.server_addresses[server_address_index];

    char server_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];

    if ( client->connect_token.num_server_addresses == 1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connecting to server %s\n", 
            netcode_address_to_string( &client->server_address, server_address_string ) );
    }
    else
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connecting to server %s [%d/%d]\n", 
            netcode_address_to_string( &client->server_address, server_address_string ), client->server_address_index + 1, client->connect_token.num_server_addresses );
    }

    netcode_client_reset_before_next_connect( client );

    client->connect_start_time = client->time;

    netcode_client_set_state( client, NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );
}

void netcode_client_process_packet_internal( struct netcode_client_t * client, struct netcode_address_t * from, uint8_t * packet, uint64_t sequence )
{
    netcode_assert( client );
    netcode_assert( packet );

    uint8_t packet_type = ( (uint8_t*) packet ) [0];

    switch ( packet_type )
    {
        case NETCODE_CONNECTION_DENIED_PACKET:
        {
            if ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST &&
                 netcode_address_equal( from, &client->server_address ) )
            {
                client->should_disconnect = 1;
                client->should_disconnect_state = NETCODE_CLIENT_STATE_CONNECTION_DENIED;
                client->last_packet_receive_time = client->time;
            }
            else if ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE &&
                      netcode_address_equal( from, &client->server_address ) )
            {
                struct netcode_connection_denied_packet_t * p = (struct netcode_connection_denied_packet_t*) packet;
                if ( netcode_bytes_equal_8( p->server_nonce, client->server_nonce ) )
                {
                    client->should_disconnect = 1;
                    client->should_disconnect_state = NETCODE_CLIENT_STATE_CONNECTION_DENIED;
                    client->last_packet_receive_time = client->time;
                }
            }
        }
        break;

        case NETCODE_CONNECTION_CHALLENGE_PACKET:
        {
            if ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST && netcode_address_equal( from, &client->server_address ) )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received connection challenge packet from server\n" );

                struct netcode_connection_challenge_packet_t * p = (struct netcode_connection_challenge_packet_t*) packet;
                client->challenge_token_sequence = p->challenge_token_sequence;
                memcpy( client->challenge_token_data, p->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
                client->last_packet_receive_time = client->time;

                netcode_client_set_state( client, NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE );
            }
        }
        break;

        case NETCODE_CONNECTION_KEEP_ALIVE_PACKET:
        {
            if ( netcode_address_equal( from, &client->server_address ) )
            {
                struct netcode_connection_keep_alive_packet_t * p = (struct netcode_connection_keep_alive_packet_t*) packet;

                if ( p->max_clients <= 0 || p->max_clients > NETCODE_MAX_CLIENTS || p->client_index < 0 || p->client_index >= p->max_clients )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection keep alive packet. client index or max clients is out of range\n" );
                    break;
                }

                if ( client->state == NETCODE_CLIENT_STATE_CONNECTED )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received connection keep alive packet from server\n" );

                    client->last_packet_receive_time = client->time;
                }
                else if ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received connection keep alive packet from server\n" );

                    client->last_packet_receive_time = client->time;
                    client->client_index = p->client_index;
                    client->max_clients = p->max_clients;

                    netcode_client_set_state( client, NETCODE_CLIENT_STATE_CONNECTED );

                    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connected to server\n" );
                }
            }
        }
        break;

        case NETCODE_CONNECTION_PAYLOAD_PACKET:
        {
            if ( client->state == NETCODE_CLIENT_STATE_CONNECTED && netcode_address_equal( from, &client->server_address ) )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received connection payload packet from server\n" );

                netcode_packet_queue_push( &client->packet_receive_queue, packet, sequence );

                client->last_packet_receive_time = client->time;

                return;
            }
        }
        break;

        case NETCODE_CONNECTION_DISCONNECT_PACKET:
        {
            if ( client->state == NETCODE_CLIENT_STATE_CONNECTED && netcode_address_equal( from, &client->server_address ) )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received disconnect packet from server\n" );

                client->should_disconnect = 1;
                client->should_disconnect_state = NETCODE_CLIENT_STATE_DISCONNECTED;
                client->last_packet_receive_time = client->time;
            }
        }
        break;

        default:
            break;
    }

    client->config.free_function( client->config.allocator_context, packet );    
}

void netcode_client_process_packet( struct netcode_client_t * client, struct netcode_address_t * from, uint8_t * packet_data, int packet_bytes )
{
    // check the source address first, so a packet from any other address cannot advance replay protection

    if ( !netcode_address_equal( from, &client->server_address ) )
        return;

    if ( client->state <= NETCODE_CLIENT_STATE_DISCONNECTED )
        return;

    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 0, sizeof( allowed_packets ) );

    uint64_t current_timestamp = (uint64_t) netcode_current_timestamp();

    uint64_t sequence;

    if ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST )
    {
        netcode_assert( !client->has_server_nonce );

        if ( packet_bytes < 1 + 1 + NETCODE_SERVER_NONCE_BYTES + NETCODE_MAC_BYTES )
            return;

        uint8_t prefix_byte = packet_data[0];
        int packet_type = prefix_byte & 0xF;
        int sequence_bytes = prefix_byte >> 4;

        if ( packet_type != NETCODE_CONNECTION_DENIED_PACKET &&
             packet_type != NETCODE_CONNECTION_CHALLENGE_PACKET )
        {
            return;
        }

        if ( sequence_bytes < 1 || sequence_bytes > 8 )
            return;

        if ( packet_bytes < 1 + sequence_bytes + NETCODE_SERVER_NONCE_BYTES + NETCODE_MAC_BYTES )
            return;

        allowed_packets[NETCODE_CONNECTION_DENIED_PACKET] = 1;
        allowed_packets[NETCODE_CONNECTION_CHALLENGE_PACKET] = 1;

        uint8_t candidate_server_nonce[NETCODE_SERVER_NONCE_BYTES];
        memcpy( candidate_server_nonce, packet_data + 1 + sequence_bytes, NETCODE_SERVER_NONCE_BYTES );

        uint8_t candidate_read_key[NETCODE_KEY_BYTES];
        netcode_derive_packet_key( candidate_read_key,
                                   client->connect_token.server_to_client_key,
                                   client->client_salt,
                                   candidate_server_nonce );

        void * packet = netcode_read_packet( packet_data,
                                             packet_bytes,
                                             &sequence,
                                             candidate_read_key,
                                             client->connect_token.protocol_id,
                                             current_timestamp,
                                             0,
                                             NULL,
                                             allowed_packets,
                                             &client->replay_protection,
                                             client->config.allocator_context,
                                             client->config.allocate_function );

        if ( !packet )
        {
            sodium_memzero( candidate_read_key, sizeof( candidate_read_key ) );
            return;
        }

        if ( ( (uint8_t*) packet )[0] == NETCODE_CONNECTION_CHALLENGE_PACKET )
        {
            memcpy( client->server_nonce, candidate_server_nonce, NETCODE_SERVER_NONCE_BYTES );
            client->has_server_nonce = 1;
            memcpy( client->context.read_packet_key, candidate_read_key, NETCODE_KEY_BYTES );
            netcode_derive_packet_key( client->context.write_packet_key,
                                       client->connect_token.client_to_server_key,
                                       client->client_salt,
                                       candidate_server_nonce );
        }

        sodium_memzero( candidate_read_key, sizeof( candidate_read_key ) );

        netcode_client_process_packet_internal( client, from, (uint8_t*) packet, sequence );
        return;
    }

    if ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE )
    {
        allowed_packets[NETCODE_CONNECTION_DENIED_PACKET] = 1;
        allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
        allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
        allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;
    }
    else if ( client->state == NETCODE_CLIENT_STATE_CONNECTED )
    {
        allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
        allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
        allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;
    }
    else
    {
        return;
    }

    void * packet = netcode_read_packet( packet_data, 
                                         packet_bytes, 
                                         &sequence, 
                                         client->context.read_packet_key, 
                                         client->connect_token.protocol_id, 
                                         current_timestamp, 
                                         0, 
                                         NULL, 
                                         allowed_packets, 
                                         &client->replay_protection, 
                                         client->config.allocator_context, 
                                         client->config.allocate_function );

    if ( !packet )
        return;
    
    netcode_client_process_packet_internal( client, from, (uint8_t*)packet, sequence );
}

void netcode_client_receive_packets( struct netcode_client_t * client )
{
    netcode_assert( client );
    netcode_assert( !client->loopback );

    if ( !client->config.network_simulator )
    {
        // process packets received from socket

        while ( 1 )
        {
            struct netcode_address_t from;
            uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];
            int packet_bytes = 0;

            if ( client->config.override_send_and_receive )
            {
                packet_bytes = client->config.receive_packet_override( client->config.callback_context, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }
            else if ( client->server_address.type == NETCODE_ADDRESS_IPV4 )
            {
                packet_bytes = netcode_socket_receive_packet( &client->socket_holder.ipv4, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }
            else if ( client->server_address.type == NETCODE_ADDRESS_IPV6 )
            {
                packet_bytes = netcode_socket_receive_packet( &client->socket_holder.ipv6, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }

            if ( packet_bytes == 0 )
                break;

            netcode_client_process_packet( client, &from, packet_data, packet_bytes );
        }
    }
    else
    {
        // process packets received from network simulator

        int num_packets_received = netcode_network_simulator_receive_packets( client->config.network_simulator,
                                                                              &client->address,
                                                                              NETCODE_CLIENT_MAX_RECEIVE_PACKETS,
                                                                              client->receive_packet_data,
                                                                              client->receive_packet_bytes,
                                                                              client->receive_from );

        int i;
        for ( i = 0; i < num_packets_received; i++ )
        {
            netcode_client_process_packet( client, &client->receive_from[i], client->receive_packet_data[i], client->receive_packet_bytes[i] );

            client->config.free_function( client->config.allocator_context, client->receive_packet_data[i] );
        }
    }
}

void netcode_client_send_packet_to_server_internal( struct netcode_client_t * client, void * packet )
{
    netcode_assert( client );
    netcode_assert( !client->loopback );
    
    uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];

    int packet_bytes = netcode_write_packet( packet, 
                                             packet_data, 
                                             NETCODE_MAX_PACKET_BYTES, 
                                             client->sequence++, 
                                             client->context.write_packet_key, 
                                             client->connect_token.protocol_id );

    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    netcode_send_packet_to_address( client->config.network_simulator,
                                    client->config.callback_context,
                                    client->config.override_send_and_receive ? client->config.send_packet_override : NULL,
                                    &client->socket_holder,
                                    &client->address,
                                    &client->server_address,
                                    packet_data,
                                    packet_bytes );

    client->last_packet_send_time = client->time;
}

void netcode_client_send_packets( struct netcode_client_t * client )
{
    netcode_assert( client );
    netcode_assert( !client->loopback );

    switch ( client->state )
    {
        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST:
        {
            if ( client->last_packet_send_time + ( 1.0 / NETCODE_PACKET_SEND_RATE ) >= client->time )
                return;

            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent connection request packet to server\n" );

            struct netcode_connection_request_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_REQUEST_PACKET;
            memcpy( packet.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
            packet.protocol_id = client->connect_token.protocol_id;
            packet.connect_token_expire_timestamp = client->connect_token.expire_timestamp;
            memcpy( packet.connect_token_nonce, client->connect_token.nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
            memcpy( packet.connect_token_data, client->connect_token.private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
            memcpy( packet.client_salt, client->client_salt, NETCODE_CONNECT_SALT_BYTES );

            netcode_client_send_packet_to_server_internal( client, &packet );
        }
        break;

        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE:
        {
            if ( client->last_packet_send_time + ( 1.0 / NETCODE_PACKET_SEND_RATE ) >= client->time )
                return;

            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent connection response packet to server\n" );

            struct netcode_connection_response_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_RESPONSE_PACKET;
            packet.challenge_token_sequence = client->challenge_token_sequence;
            memcpy( packet.challenge_token_data, client->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );

            netcode_client_send_packet_to_server_internal( client, &packet );
        }
        break;

        case NETCODE_CLIENT_STATE_CONNECTED:
        {
            if ( client->last_packet_send_time + ( 1.0 / NETCODE_PACKET_SEND_RATE ) >= client->time )
                return;

            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent connection keep alive packet to server\n" );

            struct netcode_connection_keep_alive_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
            packet.client_index = 0;
            packet.max_clients = 0;

            netcode_client_send_packet_to_server_internal( client, &packet );
        }
        break;
        
        default:
            break;
    }
}

int netcode_client_connect_to_next_server( struct netcode_client_t * client )
{
    netcode_assert( client );

    int server_address_index = client->server_address_index + 1;

    while ( server_address_index < client->connect_token.num_server_addresses &&
            !netcode_client_address_family_usable( client, client->connect_token.server_addresses[server_address_index].type ) )
    {
        server_address_index++;
    }

    if ( server_address_index >= client->connect_token.num_server_addresses )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client has no more servers to connect to\n" );
        return 0;
    }

    client->server_address_index = server_address_index;
    client->server_address = client->connect_token.server_addresses[server_address_index];

    netcode_client_reset_before_next_connect( client );

    char server_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connecting to next server %s [%d/%d]\n", 
        netcode_address_to_string( &client->server_address, server_address_string ), 
        client->server_address_index + 1, 
        client->connect_token.num_server_addresses );

    netcode_client_set_state( client, NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );

    return 1;
}

void netcode_client_update( struct netcode_client_t * client, double time )
{
    netcode_assert( client );

    client->time = time;

    if ( client->loopback )
        return;

    netcode_client_receive_packets( client );

    netcode_client_send_packets( client );

    if ( client->state > NETCODE_CLIENT_STATE_DISCONNECTED && client->state < NETCODE_CLIENT_STATE_CONNECTED )
    {
        uint64_t connect_token_expire_seconds = ( client->connect_token.expire_timestamp - client->connect_token.create_timestamp );            
        if ( client->time - client->connect_start_time >= connect_token_expire_seconds )
        {
            netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connect failed. connect token expired\n" );
            netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED, 0 );
            return;
        }
    }

    if ( client->should_disconnect )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client should disconnect -> %s\n", netcode_client_state_name( client->should_disconnect_state ) );
        if ( client->state != NETCODE_CLIENT_STATE_CONNECTED && netcode_client_connect_to_next_server( client ) )
            return;
        netcode_client_disconnect_internal( client, client->should_disconnect_state, 0 );
        return;
    }

    switch ( client->state )
    {
        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST:
        {
            if ( client->connect_token.timeout_seconds > 0 && client->last_packet_receive_time + client->connect_token.timeout_seconds < time )
            {
                netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connect failed. connection request timed out\n" );
                if ( netcode_client_connect_to_next_server( client ) )
                    return;
                netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT, 0 );
                return;
            }
        }
        break;

        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE:
        {
            if ( client->connect_token.timeout_seconds > 0 && client->last_packet_receive_time + client->connect_token.timeout_seconds < time )
            {
                netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connect failed. connection response timed out\n" );
                if ( netcode_client_connect_to_next_server( client ) )
                    return;
                netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT, 0 );
                return;
            }
        }
        break;

        case NETCODE_CLIENT_STATE_CONNECTED:
        {
            if ( client->connect_token.timeout_seconds > 0 && client->last_packet_receive_time + client->connect_token.timeout_seconds < time )
            {
                netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connection timed out\n" );
                netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT, 0 );
                return;
            }
        }
        break;

        default:
            break;
    }
}

uint64_t netcode_client_next_packet_sequence( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->sequence;  
}

void netcode_client_send_packet( struct netcode_client_t * client, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    netcode_assert( client );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes > 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_SIZE );

    // zero byte payloads are not valid on the wire and would silently vanish at the receiver

    if ( packet_bytes <= 0 || packet_bytes > NETCODE_MAX_PACKET_SIZE )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: payload packet size is out of range (%d)\n", packet_bytes );
        return;
    }

    if ( client->state != NETCODE_CLIENT_STATE_CONNECTED )
        return;

    if ( !client->loopback )
    {
        union { struct netcode_connection_payload_packet_t packet; uint8_t data[NETCODE_MAX_PAYLOAD_BYTES*2]; } buffer;

        struct netcode_connection_payload_packet_t * packet = &buffer.packet;

        packet->packet_type = NETCODE_CONNECTION_PAYLOAD_PACKET;
        packet->payload_bytes = packet_bytes;
        memcpy( packet->payload_data, packet_data, packet_bytes );

        netcode_client_send_packet_to_server_internal( client, packet );
    }
    else
    {
        client->config.send_loopback_packet_callback( client->config.callback_context, 
                                                      client->client_index, 
                                                      packet_data,
                                                      packet_bytes,
                                                      client->sequence++ );
    }
}

uint8_t * netcode_client_receive_packet( struct netcode_client_t * client, int * packet_bytes, uint64_t * packet_sequence )
{
    netcode_assert( client );
    netcode_assert( packet_bytes );

    struct netcode_connection_payload_packet_t * packet = (struct netcode_connection_payload_packet_t*) 
        netcode_packet_queue_pop( &client->packet_receive_queue, packet_sequence );
    
    if ( packet )
    {
        netcode_assert( packet->packet_type == NETCODE_CONNECTION_PAYLOAD_PACKET );
        *packet_bytes = packet->payload_bytes;
        netcode_assert( *packet_bytes >= 0 );
        netcode_assert( *packet_bytes <= NETCODE_MAX_PAYLOAD_BYTES );
        return (uint8_t*) &packet->payload_data;
    }
    else
    {
        return NULL;
    }
}

void netcode_client_free_packet( struct netcode_client_t * client, void * packet )
{
    netcode_assert( client );
    netcode_assert( packet );
    uint8_t * packet_data = (uint8_t*) packet;
    int offset = offsetof( struct netcode_connection_payload_packet_t, payload_data );
    client->config.free_function( client->config.allocator_context, packet_data - offset );
}

void netcode_client_disconnect( struct netcode_client_t * client )
{
    netcode_assert( client );
    netcode_assert( !client->loopback );
    netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_DISCONNECTED, 1 );
}

void netcode_client_disconnect_internal( struct netcode_client_t * client, int destination_state, int send_disconnect_packets )
{
    netcode_assert( !client->loopback );
    netcode_assert( destination_state <= NETCODE_CLIENT_STATE_DISCONNECTED );

    if ( client->state <= NETCODE_CLIENT_STATE_DISCONNECTED || client->state == destination_state )
        return;

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client disconnected\n" );

    if ( !client->loopback && send_disconnect_packets && client->state > NETCODE_CLIENT_STATE_DISCONNECTED && client->has_server_nonce )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent disconnect packets to server\n" );

        int i;
        for ( i = 0; i < NETCODE_NUM_DISCONNECT_PACKETS; i++ )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent disconnect packet %d\n", i );

            struct netcode_connection_disconnect_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;

            netcode_client_send_packet_to_server_internal( client, &packet );
        }
    }

    netcode_client_reset_connection_data( client, destination_state );
}

int netcode_client_state( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->state;
}

int netcode_client_index( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->client_index;
}

int netcode_client_max_clients( struct netcode_client_t * client )
{   
    netcode_assert( client );
    return client->max_clients;
}

void netcode_client_connect_loopback( struct netcode_client_t * client, int client_index, int max_clients )
{
    netcode_assert( client );
    netcode_assert( client->state <= NETCODE_CLIENT_STATE_DISCONNECTED );

    // a loopback client sends only through this callback. without it the first send would
    // call a null pointer, so refuse to enter loopback at all, in every build.

    netcode_assert( client->config.send_loopback_packet_callback );

    if ( !client->config.send_loopback_packet_callback )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: a loopback client requires send_loopback_packet_callback\n" );
        return;
    }

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connected to server via loopback as client %d\n", client_index );
    netcode_client_set_state( client, NETCODE_CLIENT_STATE_CONNECTED );
    client->client_index = client_index;
    client->max_clients = max_clients;
    client->loopback = 1;
}

void netcode_client_disconnect_loopback( struct netcode_client_t * client )
{
    netcode_assert( client );
    netcode_assert( client->loopback );
    netcode_client_reset_connection_data( client, NETCODE_CLIENT_STATE_DISCONNECTED );
}

int netcode_client_loopback( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->loopback;
}

void netcode_client_process_loopback_packet( struct netcode_client_t * client, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    netcode_assert( client );
    netcode_assert( client->loopback );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes > 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_SIZE );

    if ( !client->loopback )
        return;

    if ( packet_bytes <= 0 || packet_bytes > NETCODE_MAX_PACKET_SIZE )
        return;

    struct netcode_connection_payload_packet_t * packet = netcode_create_payload_packet( packet_bytes, client->config.allocator_context, client->config.allocate_function );
    if ( !packet )
        return;
    memcpy( packet->payload_data, packet_data, packet_bytes );
    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client processing loopback packet from server\n" );
    netcode_packet_queue_push( &client->packet_receive_queue, packet, packet_sequence );
}

uint16_t netcode_client_get_port( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->address.type == NETCODE_ADDRESS_IPV4 ? client->socket_holder.ipv4.address.port : client->socket_holder.ipv6.address.port;
}

struct netcode_address_t * netcode_client_server_address( struct netcode_client_t * client )
{
    netcode_assert( client );
    return &client->server_address;
}

// ----------------------------------------------------------------

#define NETCODE_MAX_ENCRYPTION_MAPPINGS ( NETCODE_MAX_CLIENTS * 4 )

struct netcode_encryption_manager_t
{
    int num_encryption_mappings;
    int timeout[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    double expire_time[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    double last_access_time[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    struct netcode_address_t address[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    int client_index[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    int connect_token_entry_index[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    uint8_t connect_token_mac[NETCODE_MAC_BYTES*NETCODE_MAX_ENCRYPTION_MAPPINGS];
    uint8_t client_salt[NETCODE_CONNECT_SALT_BYTES*NETCODE_MAX_ENCRYPTION_MAPPINGS];
    uint8_t server_nonce[NETCODE_SERVER_NONCE_BYTES*NETCODE_MAX_ENCRYPTION_MAPPINGS];
    uint8_t send_key[NETCODE_KEY_BYTES*NETCODE_MAX_ENCRYPTION_MAPPINGS];
    uint8_t receive_key[NETCODE_KEY_BYTES*NETCODE_MAX_ENCRYPTION_MAPPINGS];
};

void netcode_encryption_manager_reset( struct netcode_encryption_manager_t * encryption_manager )
{
    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "reset encryption manager\n" );

    netcode_assert( encryption_manager );

    encryption_manager->num_encryption_mappings = 0;
    
    int i;
    for ( i = 0; i < NETCODE_MAX_ENCRYPTION_MAPPINGS; i++ )
    {
        encryption_manager->client_index[i] = -1;
        encryption_manager->connect_token_entry_index[i] = -1;
        encryption_manager->expire_time[i] = -1.0;
        encryption_manager->last_access_time[i] = -1000.0;
        memset( &encryption_manager->address[i], 0, sizeof( struct netcode_address_t ) );
    }

    memset( encryption_manager->timeout, 0, sizeof( encryption_manager->timeout ) );    
    memset( encryption_manager->connect_token_mac, 0, sizeof( encryption_manager->connect_token_mac ) );
    sodium_memzero( encryption_manager->client_salt, sizeof( encryption_manager->client_salt ) );
    sodium_memzero( encryption_manager->server_nonce, sizeof( encryption_manager->server_nonce ) );
    sodium_memzero( encryption_manager->send_key, sizeof( encryption_manager->send_key ) );
    sodium_memzero( encryption_manager->receive_key, sizeof( encryption_manager->receive_key ) );
}

int netcode_encryption_manager_entry_expired( struct netcode_encryption_manager_t * encryption_manager, int index, double time )
{
    return ( encryption_manager->timeout[index] > 0 && ( encryption_manager->last_access_time[index] + encryption_manager->timeout[index] ) < time ) ||
           ( encryption_manager->expire_time[index] >= 0.0 && encryption_manager->expire_time[index] < time );
}

int netcode_encryption_manager_add_encryption_mapping( struct netcode_encryption_manager_t * encryption_manager, 
                                                       struct netcode_address_t * address, 
                                                       uint8_t * send_key, 
                                                       uint8_t * receive_key, 
                                                       double time, 
                                                       double expire_time,
                                                       int timeout,
                                                       int connect_token_entry_index )
{
    int i;
    for ( i = 0; i < encryption_manager->num_encryption_mappings; i++ )
    {
        if ( netcode_address_equal( &encryption_manager->address[i], address ) && !netcode_encryption_manager_entry_expired( encryption_manager, i, time ) )
        {
            encryption_manager->timeout[i] = timeout;
            encryption_manager->expire_time[i] = expire_time;
            encryption_manager->last_access_time[i] = time;
            encryption_manager->connect_token_entry_index[i] = connect_token_entry_index;
            memset( encryption_manager->connect_token_mac + i * NETCODE_MAC_BYTES, 0, NETCODE_MAC_BYTES );
            sodium_memzero( encryption_manager->client_salt + i * NETCODE_CONNECT_SALT_BYTES, NETCODE_CONNECT_SALT_BYTES );
            sodium_memzero( encryption_manager->server_nonce + i * NETCODE_SERVER_NONCE_BYTES, NETCODE_SERVER_NONCE_BYTES );
            memcpy( encryption_manager->send_key + i * NETCODE_KEY_BYTES, send_key, NETCODE_KEY_BYTES );
            memcpy( encryption_manager->receive_key + i * NETCODE_KEY_BYTES, receive_key, NETCODE_KEY_BYTES );
            return 1;
        }
    }

    for ( i = 0; i < NETCODE_MAX_ENCRYPTION_MAPPINGS; i++ )
    {
        if ( encryption_manager->address[i].type == NETCODE_ADDRESS_NONE || 
            ( netcode_encryption_manager_entry_expired( encryption_manager, i, time ) && encryption_manager->client_index[i] == -1 ) )
        {
            encryption_manager->timeout[i] = timeout;
            encryption_manager->address[i] = *address;
            encryption_manager->expire_time[i] = expire_time;
            encryption_manager->last_access_time[i] = time;
            encryption_manager->connect_token_entry_index[i] = connect_token_entry_index;
            memset( encryption_manager->connect_token_mac + i * NETCODE_MAC_BYTES, 0, NETCODE_MAC_BYTES );
            sodium_memzero( encryption_manager->client_salt + i * NETCODE_CONNECT_SALT_BYTES, NETCODE_CONNECT_SALT_BYTES );
            sodium_memzero( encryption_manager->server_nonce + i * NETCODE_SERVER_NONCE_BYTES, NETCODE_SERVER_NONCE_BYTES );
            memcpy( encryption_manager->send_key + i * NETCODE_KEY_BYTES, send_key, NETCODE_KEY_BYTES );
            memcpy( encryption_manager->receive_key + i * NETCODE_KEY_BYTES, receive_key, NETCODE_KEY_BYTES );
            if ( i + 1 > encryption_manager->num_encryption_mappings )
                encryption_manager->num_encryption_mappings = i + 1;
            return 1;
        }
    }

    return 0;
}

int netcode_encryption_manager_remove_encryption_mapping( struct netcode_encryption_manager_t * encryption_manager, struct netcode_address_t * address, double time )
{
    netcode_assert( encryption_manager );
    netcode_assert( address );

    int i;
    for ( i = 0; i < encryption_manager->num_encryption_mappings; i++ )
    {
        if ( netcode_address_equal( &encryption_manager->address[i], address ) )
        {
            encryption_manager->expire_time[i] = -1.0;
            encryption_manager->last_access_time[i] = -1000.0;
            encryption_manager->connect_token_entry_index[i] = -1;
            memset( encryption_manager->connect_token_mac + i * NETCODE_MAC_BYTES, 0, NETCODE_MAC_BYTES );
            sodium_memzero( encryption_manager->client_salt + i * NETCODE_CONNECT_SALT_BYTES, NETCODE_CONNECT_SALT_BYTES );
            sodium_memzero( encryption_manager->server_nonce + i * NETCODE_SERVER_NONCE_BYTES, NETCODE_SERVER_NONCE_BYTES );
            memset( &encryption_manager->address[i], 0, sizeof( struct netcode_address_t ) );
            sodium_memzero( encryption_manager->send_key + i * NETCODE_KEY_BYTES, NETCODE_KEY_BYTES );
            sodium_memzero( encryption_manager->receive_key + i * NETCODE_KEY_BYTES, NETCODE_KEY_BYTES );

            if ( i + 1 == encryption_manager->num_encryption_mappings )
            {
                int index = i - 1;
                while ( index >= 0 )
                {
                    if ( !netcode_encryption_manager_entry_expired( encryption_manager, index, time ) || encryption_manager->client_index[index] != -1 )
                    {
                        break;
                    }
                    encryption_manager->address[index].type = NETCODE_ADDRESS_NONE;
                    memset( encryption_manager->connect_token_mac + index * NETCODE_MAC_BYTES, 0, NETCODE_MAC_BYTES );
                    sodium_memzero( encryption_manager->client_salt + index * NETCODE_CONNECT_SALT_BYTES, NETCODE_CONNECT_SALT_BYTES );
                    sodium_memzero( encryption_manager->server_nonce + index * NETCODE_SERVER_NONCE_BYTES, NETCODE_SERVER_NONCE_BYTES );
                    sodium_memzero( encryption_manager->send_key + index * NETCODE_KEY_BYTES, NETCODE_KEY_BYTES );
                    sodium_memzero( encryption_manager->receive_key + index * NETCODE_KEY_BYTES, NETCODE_KEY_BYTES );
                    index--;
                }
                encryption_manager->num_encryption_mappings = index + 1;
            }

            return 1;
        }
    }

    return 0;
}

int netcode_encryption_manager_find_encryption_mapping( struct netcode_encryption_manager_t * encryption_manager, struct netcode_address_t * address, double time )
{
    int i;
    for ( i = 0; i < encryption_manager->num_encryption_mappings; i++ )
    {
        if ( netcode_address_equal( &encryption_manager->address[i], address ) && !netcode_encryption_manager_entry_expired( encryption_manager, i, time ) )
        {
            encryption_manager->last_access_time[i] = time;
            return i;
        }
    }
    return -1;
}

int netcode_encryption_manager_touch( struct netcode_encryption_manager_t * encryption_manager, int index, struct netcode_address_t * address, double time )
{
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    if ( !netcode_address_equal( &encryption_manager->address[index], address ) )
        return 0;
    encryption_manager->last_access_time[index] = time;
    return 1;
}

void netcode_encryption_manager_set_expire_time( struct netcode_encryption_manager_t * encryption_manager, int index, double expire_time )
{
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    encryption_manager->expire_time[index] = expire_time;
}


uint8_t * netcode_encryption_manager_get_send_key( struct netcode_encryption_manager_t * encryption_manager, int index )
{
    netcode_assert( encryption_manager );
    if ( index == -1 )
        return NULL;
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    return encryption_manager->send_key + index * NETCODE_KEY_BYTES;
}

uint8_t * netcode_encryption_manager_get_receive_key( struct netcode_encryption_manager_t * encryption_manager, int index )
{
    netcode_assert( encryption_manager );
    if ( index == -1 )
        return NULL;
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    return encryption_manager->receive_key + index * NETCODE_KEY_BYTES;
}

int netcode_encryption_manager_get_timeout( struct netcode_encryption_manager_t * encryption_manager, int index )
{
    netcode_assert( encryption_manager );
    if ( index == -1 )
        return 0;
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    return encryption_manager->timeout[index];
}

int netcode_encryption_manager_get_connect_token_entry_index( struct netcode_encryption_manager_t * encryption_manager, int index )
{
    netcode_assert( encryption_manager );
    if ( index == -1 )
        return -1;
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    return encryption_manager->connect_token_entry_index[index];
}

uint8_t * netcode_encryption_manager_get_server_nonce( struct netcode_encryption_manager_t * encryption_manager, int index )
{
    netcode_assert( encryption_manager );
    if ( index == -1 )
        return NULL;
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    return encryption_manager->server_nonce + index * NETCODE_SERVER_NONCE_BYTES;
}

// ----------------------------------------------------------------

#define NETCODE_MAX_CONNECT_TOKEN_ENTRIES ( NETCODE_MAX_CLIENTS * 8 )

#define NETCODE_CONNECT_TOKEN_ENTRY_FREE        0
#define NETCODE_CONNECT_TOKEN_ENTRY_PENDING     1
#define NETCODE_CONNECT_TOKEN_ENTRY_CONSUMED    2

#define NETCODE_CONNECT_TOKEN_ENTRY_REFUSED     -1
#define NETCODE_CONNECT_TOKEN_HISTORY_FULL      -2

struct netcode_connect_token_entry_t
{
    int state;
    double time;                    // server time the entry was created. never refreshed afterwards
    uint64_t expire_timestamp;      // when the connect token expires, and with it this entry
    uint8_t mac[NETCODE_MAC_BYTES];
    uint8_t client_salt[NETCODE_CONNECT_SALT_BYTES];
    struct netcode_address_t address;
};

void netcode_connect_token_entries_reset( struct netcode_connect_token_entry_t * connect_token_entries )
{
    int i;
    for ( i = 0; i < NETCODE_MAX_CONNECT_TOKEN_ENTRIES; i++ )
    {
        connect_token_entries[i].state = NETCODE_CONNECT_TOKEN_ENTRY_FREE;
        connect_token_entries[i].time = -1000.0;
        connect_token_entries[i].expire_timestamp = 0;
        memset( connect_token_entries[i].mac, 0, NETCODE_MAC_BYTES );
        sodium_memzero( connect_token_entries[i].client_salt, NETCODE_CONNECT_SALT_BYTES );
        memset( &connect_token_entries[i].address, 0, sizeof( struct netcode_address_t ) );
    }
}

/*
    Returns the index of the entry that admits this connection request, or one of
    NETCODE_CONNECT_TOKEN_ENTRY_REFUSED and NETCODE_CONNECT_TOKEN_HISTORY_FULL.

    An entry is created pending the first time a connect token is seen, and becomes consumed
    when the client that presented it is installed in a client slot or aborts the handshake
    with a disconnect packet. A pending entry admits a retransmitted connection request from
    the address and client_salt that created it within timeout_seconds of creation (when
    timeout_seconds is positive), so a handshake that loses a packet still completes without
    reopening the token for another attempt. A consumed entry admits nothing, whatever the
    address, so a connect token is usable for at most one session.

    An entry lives until its connect token expires. A history whose entries all hold unexpired
    connect tokens refuses a new connect token instead of evicting one, because evicting is
    how a flood of connect tokens would reopen a token that has already been used.
*/

int netcode_connect_token_entries_find_or_add( struct netcode_connect_token_entry_t * connect_token_entries, 
                                               struct netcode_address_t * address, 
                                               uint8_t * mac, 
                                               uint8_t * client_salt,
                                               uint64_t expire_timestamp, 
                                               uint64_t current_timestamp, 
                                               double time,
                                               int timeout_seconds )
{
    netcode_assert( connect_token_entries );
    netcode_assert( address );
    netcode_assert( mac );
    netcode_assert( client_salt );

    // find the matching entry for the token mac and the first entry free to take a new token. 
    // constant time worst case. This is intentional!

    int matching_token_index = -1;
    int free_token_index = -1;

    int i;
    for ( i = 0; i < NETCODE_MAX_CONNECT_TOKEN_ENTRIES; i++ )
    {
        if ( connect_token_entries[i].state != NETCODE_CONNECT_TOKEN_ENTRY_FREE && 
             memcmp( mac, connect_token_entries[i].mac, NETCODE_MAC_BYTES ) == 0 )
        {
            matching_token_index = i;
        }

        if ( free_token_index == -1 && 
             ( connect_token_entries[i].state == NETCODE_CONNECT_TOKEN_ENTRY_FREE || 
               connect_token_entries[i].expire_timestamp <= current_timestamp ) )
        {
            free_token_index = i;
        }
    }

    // if no entry is found with the mac, this is a new connect token

    if ( matching_token_index == -1 )
    {
        if ( free_token_index == -1 )
            return NETCODE_CONNECT_TOKEN_HISTORY_FULL;

        connect_token_entries[free_token_index].state = NETCODE_CONNECT_TOKEN_ENTRY_PENDING;
        connect_token_entries[free_token_index].time = time;
        connect_token_entries[free_token_index].expire_timestamp = expire_timestamp;
        connect_token_entries[free_token_index].address = *address;
        memcpy( connect_token_entries[free_token_index].mac, mac, NETCODE_MAC_BYTES );
        memcpy( connect_token_entries[free_token_index].client_salt, client_salt, NETCODE_CONNECT_SALT_BYTES );
        return free_token_index;
    }

    // a pending entry admits the address and client_salt that created it within timeout_seconds
    // of creation, and nothing else. a consumed entry admits nothing. the entry time is set when
    // the entry is created and is never refreshed.

    netcode_assert( matching_token_index >= 0 );
    netcode_assert( matching_token_index < NETCODE_MAX_CONNECT_TOKEN_ENTRIES );

    if ( connect_token_entries[matching_token_index].state == NETCODE_CONNECT_TOKEN_ENTRY_PENDING && 
         netcode_address_equal( &connect_token_entries[matching_token_index].address, address ) &&
         netcode_bytes_equal_8( connect_token_entries[matching_token_index].client_salt, client_salt ) &&
         ( timeout_seconds <= 0 || connect_token_entries[matching_token_index].time + timeout_seconds >= time ) )
    {
        return matching_token_index;
    }

    return NETCODE_CONNECT_TOKEN_ENTRY_REFUSED;
}

void netcode_connect_token_entries_consume( struct netcode_connect_token_entry_t * connect_token_entries, int index )
{
    netcode_assert( connect_token_entries );
    netcode_assert( index >= 0 );
    netcode_assert( index < NETCODE_MAX_CONNECT_TOKEN_ENTRIES );

    connect_token_entries[index].state = NETCODE_CONNECT_TOKEN_ENTRY_CONSUMED;
}

// ----------------------------------------------------------------

#define NETCODE_SERVER_FLAG_IGNORE_CONNECTION_REQUEST_PACKETS       1
#define NETCODE_SERVER_FLAG_IGNORE_CONNECTION_RESPONSE_PACKETS      (1<<1)

void netcode_default_server_config( struct netcode_server_config_t * config )
{
    netcode_assert( config );
    config->max_connect_token_lifetime = NETCODE_DEFAULT_MAX_CONNECT_TOKEN_LIFETIME;
    config->allocator_context = NULL;
    config->allocate_function = netcode_default_allocate_function;
    config->free_function = netcode_default_free_function;
    config->network_simulator = NULL;
    config->callback_context = NULL;
    config->connect_disconnect_callback = NULL;
    config->send_loopback_packet_callback = NULL;
    config->override_send_and_receive = 0;
    config->send_packet_override = NULL;
    config->receive_packet_override = NULL;
}

struct netcode_server_t
{
    struct netcode_server_config_t config;
    struct netcode_socket_holder_t socket_holder;
    struct netcode_address_t address;
    struct netcode_address_t address2;
    uint32_t flags;
    double time;
    int running;
    int max_clients;
    int num_connected_clients;
    uint64_t global_sequence;
    uint64_t challenge_sequence;
    uint64_t min_connect_token_expire_timestamp;
    uint8_t challenge_key[NETCODE_KEY_BYTES];
    int client_connected[NETCODE_MAX_CLIENTS];
    int client_timeout[NETCODE_MAX_CLIENTS];
    int client_loopback[NETCODE_MAX_CLIENTS];
    int client_confirmed[NETCODE_MAX_CLIENTS];
    int client_disconnect_reason[NETCODE_MAX_CLIENTS];
    int client_encryption_index[NETCODE_MAX_CLIENTS];
    uint64_t client_id[NETCODE_MAX_CLIENTS];
    uint64_t client_sequence[NETCODE_MAX_CLIENTS];
    double client_last_packet_send_time[NETCODE_MAX_CLIENTS];
    double client_last_packet_receive_time[NETCODE_MAX_CLIENTS];
    uint8_t client_user_data[NETCODE_MAX_CLIENTS][NETCODE_USER_DATA_BYTES];
    struct netcode_replay_protection_t client_replay_protection[NETCODE_MAX_CLIENTS];
    struct netcode_packet_queue_t client_packet_queue[NETCODE_MAX_CLIENTS];
    struct netcode_address_t client_address[NETCODE_MAX_CLIENTS];
    struct netcode_connect_token_entry_t connect_token_entries[NETCODE_MAX_CONNECT_TOKEN_ENTRIES];
    struct netcode_encryption_manager_t encryption_manager;
    uint8_t * receive_packet_data[NETCODE_SERVER_MAX_RECEIVE_PACKETS];
    int receive_packet_bytes[NETCODE_SERVER_MAX_RECEIVE_PACKETS];
    struct netcode_address_t receive_from[NETCODE_SERVER_MAX_RECEIVE_PACKETS];
};

static int server_create_error;

int netcode_server_create_error()
{
    return server_create_error;
}

int netcode_server_socket_create( struct netcode_socket_t * socket,
                                  struct netcode_address_t * address,
                                  int send_buffer_size,
                                  int receive_buffer_size,
                                  NETCODE_CONST struct netcode_server_config_t * config )
{
    netcode_assert( socket );
    netcode_assert( address );
    netcode_assert( config );

    if ( !config->network_simulator )
    {
        if ( !config->override_send_and_receive )
        {
            int socket_error = netcode_socket_create( socket, address, send_buffer_size, receive_buffer_size );

            if ( socket_error != NETCODE_SOCKET_ERROR_NONE )
            {
                // report bind failures separately: a port already in use is the common
                // operational failure for dedicated servers, and callers want to react
                // to it differently than to a socket that could not be created at all

                if ( socket_error == NETCODE_SOCKET_ERROR_BIND_IPV4_FAILED || socket_error == NETCODE_SOCKET_ERROR_BIND_IPV6_FAILED )
                {
                    server_create_error = ( address->type == NETCODE_ADDRESS_IPV6 ) ? NETCODE_SERVER_CREATE_ERROR_BIND_SOCKET_IPV6_FAILED
                                                                                    : NETCODE_SERVER_CREATE_ERROR_BIND_SOCKET_IPV4_FAILED;
                }
                else
                {
                    server_create_error = ( address->type == NETCODE_ADDRESS_IPV6 ) ? NETCODE_SERVER_CREATE_ERROR_CREATE_SOCKET_IPV6_FAILED
                                                                                    : NETCODE_SERVER_CREATE_ERROR_CREATE_SOCKET_IPV4_FAILED;
                }
                return 0;
            }
        }
    }

    return 1;
}

struct netcode_server_t * netcode_server_create_dual( NETCODE_CONST char * server_address1_string, NETCODE_CONST char * server_address2_string, NETCODE_CONST struct netcode_server_config_t * config, double time )
{
    netcode_assert( config );
    netcode_assert( netcode.initialized );

    server_create_error = NETCODE_SERVER_CREATE_ERROR_NONE;

    // tolerate a zeroed config: default the allocator functions so a forgotten
    // netcode_default_server_config is an inconvenience, not a crash

    struct netcode_server_config_t config_copy = *config;
    if ( !config_copy.allocate_function )
        config_copy.allocate_function = netcode_default_allocate_function;
    if ( !config_copy.free_function )
        config_copy.free_function = netcode_default_free_function;
    if ( config_copy.max_connect_token_lifetime <= 0 )
        config_copy.max_connect_token_lifetime = NETCODE_DEFAULT_MAX_CONNECT_TOKEN_LIFETIME;
    config = &config_copy;

    // the overrides are called on the update path with no null check. a missing one is a
    // configuration error, refused here rather than dereferenced on the first update.

    if ( config->override_send_and_receive && ( !config->send_packet_override || !config->receive_packet_override ) )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: override_send_and_receive requires both send_packet_override and receive_packet_override\n" );
        server_create_error = NETCODE_SERVER_CREATE_ERROR_MISSING_OVERRIDE_CALLBACK;
        return NULL;
    }

    struct netcode_address_t server_address1;
    struct netcode_address_t server_address2;

    memset( &server_address1, 0, sizeof( server_address1 ) );
    memset( &server_address2, 0, sizeof( server_address2 ) );

    if ( netcode_parse_address( server_address1_string, &server_address1 ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to parse server public address\n" );
        server_create_error = NETCODE_SERVER_CREATE_ERROR_PARSE_ADDRESS_FAILED;
        return NULL;
    }

    if ( server_address2_string != NULL && netcode_parse_address( server_address2_string, &server_address2 ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to parse server public address2\n" );
        server_create_error = NETCODE_SERVER_CREATE_ERROR_PARSE_ADDRESS2_FAILED;
        return NULL;
    }

    struct netcode_address_t bind_address_ipv4;
    struct netcode_address_t bind_address_ipv6;

    memset( &bind_address_ipv4, 0, sizeof( bind_address_ipv4 ) );
    memset( &bind_address_ipv6, 0, sizeof( bind_address_ipv6 ) );

    struct netcode_socket_t socket_ipv4;
    struct netcode_socket_t socket_ipv6;

    netcode_socket_init( &socket_ipv4 );
    netcode_socket_init( &socket_ipv6 );

    if ( server_address1.type == NETCODE_ADDRESS_IPV4 || server_address2.type == NETCODE_ADDRESS_IPV4 )
    {
        bind_address_ipv4.type = NETCODE_ADDRESS_IPV4;
        bind_address_ipv4.port = server_address1.type == NETCODE_ADDRESS_IPV4 ? server_address1.port : server_address2.port;

        if ( !netcode_server_socket_create( &socket_ipv4, &bind_address_ipv4, NETCODE_SERVER_SOCKET_SNDBUF_SIZE, NETCODE_SERVER_SOCKET_RCVBUF_SIZE, config ) )
        {
            return NULL;
        }
    }

    if ( server_address1.type == NETCODE_ADDRESS_IPV6 || server_address2.type == NETCODE_ADDRESS_IPV6 )
    {
        bind_address_ipv6.type = NETCODE_ADDRESS_IPV6;
        bind_address_ipv6.port = server_address1.type == NETCODE_ADDRESS_IPV6 ? server_address1.port : server_address2.port;

        if ( !netcode_server_socket_create( &socket_ipv6, &bind_address_ipv6, NETCODE_SERVER_SOCKET_SNDBUF_SIZE, NETCODE_SERVER_SOCKET_RCVBUF_SIZE, config ) )
        {
            netcode_socket_destroy( &socket_ipv4 );
            return NULL;
        }
    }

    struct netcode_server_t * server = (struct netcode_server_t*) config->allocate_function( config->allocator_context, sizeof( struct netcode_server_t ) );
    if ( !server )
    {
        netcode_socket_destroy( &socket_ipv4 );
        netcode_socket_destroy( &socket_ipv6 );
        server_create_error = NETCODE_SERVER_CREATE_ERROR_ALLOCATE_SERVER_FAILED;
        return NULL;
    }

    memset( server, 0, sizeof(struct netcode_server_t) );

    if ( !config->network_simulator )
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "server listening on %s\n", server_address1_string );
    }
    else
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "server listening on %s (network simulator)\n", server_address1_string );
    }

    server->config = *config;
    server->socket_holder.ipv4 = socket_ipv4;
    server->socket_holder.ipv6 = socket_ipv6;
    server->address = server_address1;
    server->address2 = server_address2;
    server->time = time;
    server->global_sequence = 1ULL << 63;

    int i;
    for ( i = 0; i < NETCODE_MAX_CLIENTS; i++ )
    {
        server->client_encryption_index[i] = -1;
    }

    netcode_connect_token_entries_reset( server->connect_token_entries );

    netcode_encryption_manager_reset( &server->encryption_manager );

    for ( i = 0; i < NETCODE_MAX_CLIENTS; i++ )
    {
        netcode_replay_protection_reset( &server->client_replay_protection[i] );
    }

    return server;
}

struct netcode_server_t * netcode_server_create( NETCODE_CONST char * server_address_string, NETCODE_CONST struct netcode_server_config_t * config, double time )
{
    return netcode_server_create_dual( server_address_string, NULL, config, time );
}

void netcode_server_stop( struct netcode_server_t * server );

void netcode_server_destroy( struct netcode_server_t * server )
{
    netcode_assert( server );

    netcode_server_stop( server );

    netcode_socket_destroy( &server->socket_holder.ipv4 );
    netcode_socket_destroy( &server->socket_holder.ipv6 );

    sodium_memzero( server->config.private_key, NETCODE_KEY_BYTES );

    server->config.free_function( server->config.allocator_context, server );
}

void netcode_server_start( struct netcode_server_t * server, int max_clients )
{
    netcode_assert( server );
    netcode_assert( max_clients > 0 );
    netcode_assert( max_clients <= NETCODE_MAX_CLIENTS );

    // the per-client arrays are sized NETCODE_MAX_CLIENTS. an out of range value here
    // must not get through in release builds where asserts compile out

    if ( max_clients <= 0 || max_clients > NETCODE_MAX_CLIENTS )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: max clients must be in [1,%d], got %d\n", NETCODE_MAX_CLIENTS, max_clients );
        return;
    }

    if ( server->running )
    {
        netcode_server_stop( server );
    }

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server started with %d client slots\n", max_clients );

    server->running = 1;
    server->max_clients = max_clients;
    server->num_connected_clients = 0;
    server->challenge_sequence = 0;
    netcode_generate_key( server->challenge_key );

    // a connect token issued before this server started carries keys that already encrypted
    // packets at sequence numbers this run starts again from. the earliest expire timestamp a
    // connect token issued after the start can carry is the start time plus the maximum
    // lifetime the backend issues, so anything at or before that is refused.

    server->min_connect_token_expire_timestamp = (uint64_t) netcode_current_timestamp() + (uint64_t) server->config.max_connect_token_lifetime;

    // global packets (challenge, denied) encrypt with the same per-token server to client
    // keys as per-client packets, whose sequences start at zero, so the global sequence
    // lives in the top half of the sequence space to keep AEAD nonces disjoint under a
    // shared key. netcode_server_stop zeroes it, so it must be re-seeded on every start,
    // not just in netcode_server_create -- otherwise a stopped and restarted server would
    // reuse nonces between global and per-client packets.

    server->global_sequence = 1ULL << 63;

    int i;
    for ( i = 0; i < server->max_clients; i++ )
    {
        netcode_packet_queue_init( &server->client_packet_queue[i], server->config.allocator_context, server->config.allocate_function, server->config.free_function );
    }

    for ( i = 0; i < NETCODE_MAX_CLIENTS; i++ )
    {
        server->client_disconnect_reason[i] = NETCODE_SERVER_CLIENT_DISCONNECT_REASON_NONE;
    }
}

void netcode_server_send_global_packet( struct netcode_server_t * server, void * packet, struct netcode_address_t * to, uint8_t * packet_key )
{
    netcode_assert( server );
    netcode_assert( packet );
    netcode_assert( to );
    netcode_assert( packet_key );

    uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];

    int packet_bytes = netcode_write_packet( packet, packet_data, NETCODE_MAX_PACKET_BYTES, server->global_sequence, packet_key, server->config.protocol_id );

    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    netcode_send_packet_to_address( server->config.network_simulator,
                                    server->config.callback_context,
                                    server->config.override_send_and_receive ? server->config.send_packet_override : NULL,
                                    &server->socket_holder,
                                    &server->address,
                                    to,
                                    packet_data,
                                    packet_bytes );

    server->global_sequence++;
}

void netcode_server_send_client_packet( struct netcode_server_t * server, void * packet, int client_index )
{
    netcode_assert( server );
    netcode_assert( packet );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->client_connected[client_index] );
    netcode_assert( !server->client_loopback[client_index] );

    uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];

    if ( !netcode_encryption_manager_touch( &server->encryption_manager, 
                                            server->client_encryption_index[client_index], 
                                            &server->client_address[client_index], 
                                            server->time ) )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: encryption mapping is out of date for client %d\n", client_index );
        return;
    }

    uint8_t * packet_key = netcode_encryption_manager_get_send_key( &server->encryption_manager, server->client_encryption_index[client_index] );

    int packet_bytes = netcode_write_packet( packet, packet_data, NETCODE_MAX_PACKET_BYTES, server->client_sequence[client_index], packet_key, server->config.protocol_id );

    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    netcode_send_packet_to_address( server->config.network_simulator,
                                    server->config.callback_context,
                                    server->config.override_send_and_receive ? server->config.send_packet_override : NULL,
                                    &server->socket_holder,
                                    &server->address,
                                    &server->client_address[client_index],
                                    packet_data,
                                    packet_bytes );

    server->client_sequence[client_index]++;

    server->client_last_packet_send_time[client_index] = server->time;
}

static void netcode_server_reset_client_slot( struct netcode_server_t * server, int client_index )
{
    netcode_packet_queue_clear( &server->client_packet_queue[client_index] );

    server->client_connected[client_index] = 0;
    server->client_loopback[client_index] = 0;
    server->client_confirmed[client_index] = 0;
    server->client_id[client_index] = 0;
    server->client_sequence[client_index] = 0;
    server->client_last_packet_send_time[client_index] = 0.0;
    server->client_last_packet_receive_time[client_index] = 0.0;
    memset( &server->client_address[client_index], 0, sizeof( struct netcode_address_t ) );
    server->client_encryption_index[client_index] = -1;
    memset( server->client_user_data[client_index], 0, NETCODE_USER_DATA_BYTES );
}

void netcode_server_disconnect_client_internal( struct netcode_server_t * server, int client_index, int send_disconnect_packets, int disconnect_reason )
{
    netcode_assert( server );
    netcode_assert( server->running );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->client_connected[client_index] );
    netcode_assert( !server->client_loopback[client_index] );
    netcode_assert( server->encryption_manager.client_index[server->client_encryption_index[client_index]] == client_index );

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server disconnected client %d\n", client_index );

    // record why before the callback fires, so the reason can be queried from inside the callback

    server->client_disconnect_reason[client_index] = disconnect_reason;

    if ( send_disconnect_packets )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server sent disconnect packets to client %d\n", client_index );

        int i;
        for ( i = 0; i < NETCODE_NUM_DISCONNECT_PACKETS; i++ )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server sent disconnect packet %d\n", i );

            struct netcode_connection_disconnect_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;

            netcode_server_send_client_packet( server, &packet, client_index );
        }
    }

    netcode_replay_protection_reset( &server->client_replay_protection[client_index] );

    server->encryption_manager.client_index[server->client_encryption_index[client_index]] = -1;

    netcode_encryption_manager_remove_encryption_mapping( &server->encryption_manager, &server->client_address[client_index], server->time );

    // mark the slot disconnected and count it out before the callback so a re-entrant
    // disconnect of this slot returns at the connected guard, and a callback that stops
    // the server leaves the count at zero. the rest of the slot stays readable from
    // inside the callback until the reset below.

    server->client_connected[client_index] = 0;
    server->num_connected_clients--;
    netcode_assert( server->num_connected_clients >= 0 );

    if ( server->config.connect_disconnect_callback )
    {
        server->config.connect_disconnect_callback( server->config.callback_context, client_index, 0 );
    }

    netcode_server_reset_client_slot( server, client_index );
}

void netcode_server_disconnect_client( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );

    if ( client_index < 0 || client_index >= server->max_clients )
        return;

    netcode_assert( server->client_loopback[client_index] == 0 );

    if ( !server->client_connected[client_index] )
        return;

    if ( server->client_loopback[client_index] )
        return;

    netcode_server_disconnect_client_internal( server, client_index, 1, NETCODE_SERVER_CLIENT_DISCONNECT_REASON_SERVER_DISCONNECT );
}

void netcode_server_disconnect_all_clients( struct netcode_server_t * server )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    int i;
    for ( i = 0; i < server->max_clients; i++ )
    {
        if ( server->client_connected[i] && !server->client_loopback[i] )
        {
            netcode_server_disconnect_client_internal( server, i, 1, NETCODE_SERVER_CLIENT_DISCONNECT_REASON_SERVER_DISCONNECT );
        }
    }
}

void netcode_server_stop( struct netcode_server_t * server )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    netcode_server_disconnect_all_clients( server );

    // loopback clients are not disconnected above, but they must not survive a server stop

    int i;
    for ( i = 0; i < server->max_clients; i++ )
    {
        if ( server->client_connected[i] && server->client_loopback[i] )
        {
            netcode_server_disconnect_loopback_client( server, i );
        }
    }

    server->running = 0;
    server->max_clients = 0;
    server->num_connected_clients = 0;

    server->global_sequence = 0;
    server->challenge_sequence = 0;
    sodium_memzero( server->challenge_key, NETCODE_KEY_BYTES );

    netcode_connect_token_entries_reset( server->connect_token_entries );

    netcode_encryption_manager_reset( &server->encryption_manager );

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server stopped\n" );
}

int netcode_server_find_client_index_by_id( struct netcode_server_t * server, uint64_t client_id )
{
    netcode_assert( server );

    int i;
    for ( i = 0; i < server->max_clients; i++ )
    {   
        if ( server->client_connected[i] && server->client_id[i] == client_id )
            return i;
    }

    return -1;
}

int netcode_server_find_client_index_by_address( struct netcode_server_t * server, struct netcode_address_t * address )
{
    netcode_assert( server );
    netcode_assert( address );

    int i;
    for ( i = 0; i < server->max_clients; i++ )
    {   
        if ( server->client_connected[i] && netcode_address_equal( &server->client_address[i], address ) )
            return i;
    }

    return -1;
}

void netcode_server_process_connection_request_packet( struct netcode_server_t * server, 
                                                       struct netcode_address_t * from, 
                                                       struct netcode_connection_request_packet_t * packet, 
                                                       uint64_t current_timestamp )
{
    netcode_assert( server );

    struct netcode_connect_token_private_t connect_token_private;
    if ( netcode_read_connect_token_private( packet->connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, &connect_token_private ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. failed to read connect token\n" );
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    int found_server_address = 0;
    int i;
    for ( i = 0; i < connect_token_private.num_server_addresses; i++ )
    {
        if ( netcode_address_equal( &server->address, &connect_token_private.server_addresses[i] ) )
        {
            found_server_address = 1;
        }
        if ( server->address2.type != NETCODE_ADDRESS_NONE && netcode_address_equal( &server->address2, &connect_token_private.server_addresses[i] ) )
        {
            found_server_address = 1;
        }
    }
    if ( !found_server_address )
    {   
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. server address not in connect token whitelist\n" );
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    if ( netcode_server_find_client_index_by_address( server, from ) != -1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. a client with this address is already connected\n" );
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    if ( netcode_server_find_client_index_by_id( server, connect_token_private.client_id ) != -1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. a client with this id is already connected\n" );
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    uint8_t * connect_token_mac = packet->connect_token_data + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES;

    int connect_token_entry_index = netcode_connect_token_entries_find_or_add( server->connect_token_entries, 
                                                                               from, 
                                                                               connect_token_mac, 
                                                                               packet->client_salt,
                                                                               packet->connect_token_expire_timestamp, 
                                                                               current_timestamp, 
                                                                               server->time,
                                                                               connect_token_private.timeout_seconds );

    if ( connect_token_entry_index == NETCODE_CONNECT_TOKEN_HISTORY_FULL )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. connect token history is full\n" );
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    if ( connect_token_entry_index == NETCODE_CONNECT_TOKEN_ENTRY_REFUSED )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. connect token has already been used\n" );
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    if ( server->num_connected_clients == server->max_clients )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server denied connection request. server is full\n" );

        uint8_t denied_server_nonce[NETCODE_SERVER_NONCE_BYTES];
        uint8_t denied_send_key[NETCODE_KEY_BYTES];

        netcode_random_bytes( denied_server_nonce, NETCODE_SERVER_NONCE_BYTES );
        netcode_derive_packet_key( denied_send_key,
                                   connect_token_private.server_to_client_key,
                                   packet->client_salt,
                                   denied_server_nonce );

        struct netcode_connection_denied_packet_t p;
        p.packet_type = NETCODE_CONNECTION_DENIED_PACKET;
        memcpy( p.server_nonce, denied_server_nonce, NETCODE_SERVER_NONCE_BYTES );
        
        netcode_server_send_global_packet( server, &p, from, denied_send_key );

        sodium_memzero( denied_server_nonce, sizeof( denied_server_nonce ) );
        sodium_memzero( denied_send_key, sizeof( denied_send_key ) );
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    uint64_t token_lifetime = ( packet->connect_token_expire_timestamp == UINT64_MAX )
                            ? (uint64_t) server->config.max_connect_token_lifetime
                            : packet->connect_token_expire_timestamp - current_timestamp;

    double expire_time = ( connect_token_private.timeout_seconds >= 0 )
                       ? server->time + connect_token_private.timeout_seconds
                       : server->time + (double) token_lifetime;

    int encryption_index = netcode_encryption_manager_find_encryption_mapping( &server->encryption_manager, from, server->time );

    if ( encryption_index >= 0 &&
         server->encryption_manager.client_index[encryption_index] == -1 &&
         crypto_verify_16( server->encryption_manager.connect_token_mac + encryption_index * NETCODE_MAC_BYTES, connect_token_mac ) == 0 &&
         netcode_bytes_equal_8( server->encryption_manager.client_salt + encryption_index * NETCODE_CONNECT_SALT_BYTES, packet->client_salt ) )
    {
        netcode_encryption_manager_set_expire_time( &server->encryption_manager, encryption_index, expire_time );
    }
    else
    {
        uint8_t server_nonce[NETCODE_SERVER_NONCE_BYTES];
        uint8_t send_key[NETCODE_KEY_BYTES];
        uint8_t receive_key[NETCODE_KEY_BYTES];

        netcode_random_bytes( server_nonce, NETCODE_SERVER_NONCE_BYTES );
        netcode_derive_packet_key( send_key,
                                   connect_token_private.server_to_client_key,
                                   packet->client_salt,
                                   server_nonce );
        netcode_derive_packet_key( receive_key,
                                   connect_token_private.client_to_server_key,
                                   packet->client_salt,
                                   server_nonce );

        if ( !netcode_encryption_manager_add_encryption_mapping( &server->encryption_manager, 
                                                                 from, 
                                                                 send_key, 
                                                                 receive_key, 
                                                                 server->time, 
                                                                 expire_time,
                                                                 connect_token_private.timeout_seconds,
                                                                 connect_token_entry_index ) )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. failed to add encryption mapping\n" );
            sodium_memzero( server_nonce, sizeof( server_nonce ) );
            sodium_memzero( send_key, sizeof( send_key ) );
            sodium_memzero( receive_key, sizeof( receive_key ) );
            sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
            return;
        }

        encryption_index = netcode_encryption_manager_find_encryption_mapping( &server->encryption_manager, from, server->time );
        if ( encryption_index >= 0 )
        {
            memcpy( server->encryption_manager.connect_token_mac + encryption_index * NETCODE_MAC_BYTES, connect_token_mac, NETCODE_MAC_BYTES );
            memcpy( server->encryption_manager.client_salt + encryption_index * NETCODE_CONNECT_SALT_BYTES, packet->client_salt, NETCODE_CONNECT_SALT_BYTES );
            memcpy( server->encryption_manager.server_nonce + encryption_index * NETCODE_SERVER_NONCE_BYTES, server_nonce, NETCODE_SERVER_NONCE_BYTES );
        }

        sodium_memzero( server_nonce, sizeof( server_nonce ) );
        sodium_memzero( send_key, sizeof( send_key ) );
        sodium_memzero( receive_key, sizeof( receive_key ) );
    }

    uint8_t * mapping_send_key = netcode_encryption_manager_get_send_key( &server->encryption_manager, encryption_index );
    uint8_t * mapping_server_nonce = netcode_encryption_manager_get_server_nonce( &server->encryption_manager, encryption_index );
    if ( !mapping_send_key || !mapping_server_nonce )
    {
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    struct netcode_challenge_token_t challenge_token;
    challenge_token.client_id = connect_token_private.client_id;
    memcpy( challenge_token.user_data, connect_token_private.user_data, NETCODE_USER_DATA_BYTES );

    struct netcode_connection_challenge_packet_t challenge_packet;
    challenge_packet.packet_type = NETCODE_CONNECTION_CHALLENGE_PACKET;
    memcpy( challenge_packet.server_nonce, mapping_server_nonce, NETCODE_SERVER_NONCE_BYTES );
    challenge_packet.challenge_token_sequence = server->challenge_sequence;
    netcode_write_challenge_token( &challenge_token, challenge_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
    if ( netcode_encrypt_challenge_token( challenge_packet.challenge_token_data, 
                                          NETCODE_CHALLENGE_TOKEN_BYTES, 
                                          server->challenge_sequence, 
                                          server->challenge_key ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. failed to encrypt challenge token\n" );
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        return;
    }

    server->challenge_sequence++;

    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server sent connection challenge packet\n" );

    netcode_server_send_global_packet( server, &challenge_packet, from, mapping_send_key );

    sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
}

int netcode_server_find_free_client_index( struct netcode_server_t * server )
{
    netcode_assert( server );

    int i;
    for ( i = 0; i < server->max_clients; i++ )
    {
        if ( !server->client_connected[i] )
            return i;
    }

    return -1;
}

void netcode_server_connect_client( struct netcode_server_t * server, 
                                    int client_index, 
                                    struct netcode_address_t * address, 
                                    uint64_t client_id, 
                                    int encryption_index,
                                    int timeout_seconds, 
                                    void * user_data )
{
    netcode_assert( server );
    netcode_assert( server->running );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( address );
    netcode_assert( encryption_index != -1 );
    netcode_assert( user_data );
    netcode_assert( server->encryption_manager.client_index[encryption_index] == -1 );

    server->num_connected_clients++;

    netcode_assert( server->num_connected_clients <= server->max_clients );

    netcode_assert( server->client_connected[client_index] == 0 );

    netcode_encryption_manager_set_expire_time( &server->encryption_manager, encryption_index, -1.0 );
    
    server->encryption_manager.client_index[encryption_index] = client_index;

    server->client_connected[client_index] = 1;
    server->client_timeout[client_index] = timeout_seconds;
    server->client_encryption_index[client_index] = encryption_index;
    server->client_id[client_index] = client_id;
    server->client_sequence[client_index] = 0;
    server->client_address[client_index] = *address;
    server->client_disconnect_reason[client_index] = NETCODE_SERVER_CLIENT_DISCONNECT_REASON_NONE;

    netcode_assert( netcode_server_find_client_index_by_id( server, client_id ) == client_index );
    netcode_assert( netcode_server_find_client_index_by_address( server, address ) == client_index );

    server->client_last_packet_send_time[client_index] = server->time;
    server->client_last_packet_receive_time[client_index] = server->time;
    memcpy( server->client_user_data[client_index], user_data, NETCODE_USER_DATA_BYTES );

    // the connect token that got this client here is spent. the history slot may have been
    // reclaimed after that token expired, so consume it only when the mac still matches.

    int connect_token_entry_index = netcode_encryption_manager_get_connect_token_entry_index( &server->encryption_manager, encryption_index );
    if ( connect_token_entry_index >= 0 &&
         memcmp( server->connect_token_entries[connect_token_entry_index].mac,
                 server->encryption_manager.connect_token_mac + encryption_index * NETCODE_MAC_BYTES,
                 NETCODE_MAC_BYTES ) == 0 )
    {
        netcode_connect_token_entries_consume( server->connect_token_entries, connect_token_entry_index );
    }

    char address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server accepted client %s %.16" PRIx64 " in slot %d\n", 
        netcode_address_to_string( address, address_string ), client_id, client_index );

    struct netcode_connection_keep_alive_packet_t packet;
    packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
    packet.client_index = client_index;
    packet.max_clients = server->max_clients;

    netcode_server_send_client_packet( server, &packet, client_index );

    if ( server->config.connect_disconnect_callback )
    {
        server->config.connect_disconnect_callback( server->config.callback_context, client_index, 1 );
    }
}

void netcode_server_process_connection_response_packet( struct netcode_server_t * server, 
                                                        struct netcode_address_t * from, 
                                                        struct netcode_connection_response_packet_t * packet, 
                                                        int encryption_index )
{
    netcode_assert( server );

    if ( netcode_decrypt_challenge_token( packet->challenge_token_data, 
                                          NETCODE_CHALLENGE_TOKEN_BYTES, 
                                          packet->challenge_token_sequence, 
                                          server->challenge_key ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. failed to decrypt challenge token\n" );
        return;
    }

    struct netcode_challenge_token_t challenge_token;
    if ( netcode_read_challenge_token( packet->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES, &challenge_token ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. failed to read challenge token\n" );
        return;
    }

    uint8_t * packet_send_key = netcode_encryption_manager_get_send_key( &server->encryption_manager, encryption_index );

    if ( !packet_send_key )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. no packet send key\n" );
        return;
    }

    if ( netcode_server_find_client_index_by_address( server, from ) != -1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. a client with this address is already connected\n" );
        return;
    }

    if ( netcode_server_find_client_index_by_id( server, challenge_token.client_id ) != -1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. a client with this id is already connected\n" );
        return;
    }

    if ( server->num_connected_clients == server->max_clients )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server denied connection response. server is full\n" );

        uint8_t * server_nonce = netcode_encryption_manager_get_server_nonce( &server->encryption_manager, encryption_index );
        if ( !server_nonce )
            return;

        struct netcode_connection_denied_packet_t p;
        p.packet_type = NETCODE_CONNECTION_DENIED_PACKET;
        memcpy( p.server_nonce, server_nonce, NETCODE_SERVER_NONCE_BYTES );

        netcode_server_send_global_packet( server, &p, from, packet_send_key );

        return;
    }

    int client_index = netcode_server_find_free_client_index( server );

    netcode_assert( client_index != -1 );

    int timeout_seconds = netcode_encryption_manager_get_timeout( &server->encryption_manager, encryption_index );

    netcode_server_connect_client( server, client_index, from, challenge_token.client_id, encryption_index, timeout_seconds, challenge_token.user_data );
}

void netcode_server_process_packet_internal( struct netcode_server_t * server, 
                                             struct netcode_address_t * from, 
                                             void * packet, 
                                             uint64_t sequence, 
                                             uint64_t current_timestamp, 
                                             int encryption_index, 
                                             int client_index )
{
    netcode_assert( server );
    netcode_assert( packet );

    (void) from;
    (void) sequence;

    uint8_t packet_type = ( (uint8_t*) packet ) [0];

    switch ( packet_type )
    {
        case NETCODE_CONNECTION_REQUEST_PACKET:
        {    
            if ( ( server->flags & NETCODE_SERVER_FLAG_IGNORE_CONNECTION_REQUEST_PACKETS ) == 0 )
            {
                char from_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received connection request from %s\n", netcode_address_to_string( from, from_address_string ) );
                netcode_server_process_connection_request_packet( server, from, (struct netcode_connection_request_packet_t*) packet, current_timestamp );
            }
        }
        break;

        case NETCODE_CONNECTION_RESPONSE_PACKET:
        {    
            if ( ( server->flags & NETCODE_SERVER_FLAG_IGNORE_CONNECTION_RESPONSE_PACKETS ) == 0 )
            {
                char from_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received connection response from %s\n", netcode_address_to_string( from, from_address_string ) );
                netcode_server_process_connection_response_packet( server, from, (struct netcode_connection_response_packet_t*) packet, encryption_index );
            }
        }
        break;

        case NETCODE_CONNECTION_KEEP_ALIVE_PACKET:
        {
            if ( client_index != -1 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received connection keep alive packet from client %d\n", client_index );
                server->client_last_packet_receive_time[client_index] = server->time;
                if ( !server->client_confirmed[client_index] )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server confirmed connection with client %d\n", client_index );
                    server->client_confirmed[client_index] = 1;
                }
            }
        }
        break;

        case NETCODE_CONNECTION_PAYLOAD_PACKET:
        {
            if ( client_index != -1 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received connection payload packet from client %d\n", client_index );
                server->client_last_packet_receive_time[client_index] = server->time;
                if ( !server->client_confirmed[client_index] )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server confirmed connection with client %d\n", client_index );
                    server->client_confirmed[client_index] = 1;
                }
                netcode_packet_queue_push( &server->client_packet_queue[client_index], packet, sequence );
                return;
            }
        }
        break;

        case NETCODE_CONNECTION_DISCONNECT_PACKET:
        {
            if ( client_index != -1 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received disconnect packet from client %d\n", client_index );
                netcode_server_disconnect_client_internal( server, client_index, 0, NETCODE_SERVER_CLIENT_DISCONNECT_REASON_CLIENT_DISCONNECT );
            }
            else if ( encryption_index != -1 )
            {
                char from_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received disconnect packet from %s before connection completed\n", netcode_address_to_string( from, from_address_string ) );
                int connect_token_entry_index = netcode_encryption_manager_get_connect_token_entry_index( &server->encryption_manager, encryption_index );
                if ( connect_token_entry_index >= 0 &&
                     memcmp( server->connect_token_entries[connect_token_entry_index].mac,
                             server->encryption_manager.connect_token_mac + encryption_index * NETCODE_MAC_BYTES,
                             NETCODE_MAC_BYTES ) == 0 )
                {
                    netcode_connect_token_entries_consume( server->connect_token_entries, connect_token_entry_index );
                }
                netcode_encryption_manager_remove_encryption_mapping( &server->encryption_manager, from, server->time );
            }
        }
        break;

        default:
            break;
    }

    if ( packet_type == NETCODE_CONNECTION_REQUEST_PACKET )
    {
        struct netcode_connection_request_packet_t * request_packet = (struct netcode_connection_request_packet_t*) packet;
        sodium_memzero( request_packet->connect_token_data, sizeof( request_packet->connect_token_data ) );
        sodium_memzero( request_packet->client_salt, sizeof( request_packet->client_salt ) );
    }

    server->config.free_function( server->config.allocator_context, packet );
}

void netcode_server_read_and_process_packet( struct netcode_server_t * server,
                                             struct netcode_address_t * from,
                                             uint8_t * packet_data,
                                             int packet_bytes,
                                             uint64_t current_timestamp,
                                             uint8_t * allowed_packets );

void netcode_server_process_packet( struct netcode_server_t * server, struct netcode_address_t * from, uint8_t * packet_data, int packet_bytes )
{
    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 0, sizeof( allowed_packets ) );
    allowed_packets[NETCODE_CONNECTION_REQUEST_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_RESPONSE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;

    uint64_t current_timestamp = (uint64_t) netcode_current_timestamp();

    netcode_server_read_and_process_packet( server, from, packet_data, packet_bytes, current_timestamp, allowed_packets );
}

void netcode_server_read_and_process_packet( struct netcode_server_t * server, 
                                             struct netcode_address_t * from, 
                                             uint8_t * packet_data, 
                                             int packet_bytes, 
                                             uint64_t current_timestamp, 
                                             uint8_t * allowed_packets )
{
    if ( !server->running )
        return;

    if ( packet_bytes <= 1 )
        return;

    uint64_t sequence;

    int encryption_index = -1;
    int client_index = netcode_server_find_client_index_by_address( server, from );
    if ( client_index != -1 )
    {
        netcode_assert( client_index >= 0 );
        netcode_assert( client_index < server->max_clients );
        encryption_index = server->client_encryption_index[client_index];
    }
    else
    {
        encryption_index = netcode_encryption_manager_find_encryption_mapping( &server->encryption_manager, from, server->time );
    }
    
    uint8_t * read_packet_key = netcode_encryption_manager_get_receive_key( &server->encryption_manager, encryption_index );

    if ( !read_packet_key && packet_data[0] != 0 )
    {
        char address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server could not process packet because no encryption mapping exists for %s\n", netcode_address_to_string( from, address_string ) );
        return;
    }

    void * packet = netcode_read_packet( packet_data, 
                                         packet_bytes, 
                                         &sequence, 
                                         read_packet_key, 
                                         server->config.protocol_id, 
                                         current_timestamp, 
                                         server->min_connect_token_expire_timestamp, 
                                         server->config.private_key, 
                                         allowed_packets, 
                                         ( client_index != -1 ) ? &server->client_replay_protection[client_index] : NULL, 
                                         server->config.allocator_context, 
                                         server->config.allocate_function );

    if ( !packet )
        return;

    netcode_server_process_packet_internal( server, from, packet, sequence, current_timestamp, encryption_index, client_index );
}

void netcode_server_receive_packets( struct netcode_server_t * server )
{
    netcode_assert( server );

    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 0, sizeof( allowed_packets ) );
    allowed_packets[NETCODE_CONNECTION_REQUEST_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_RESPONSE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;

    uint64_t current_timestamp = (uint64_t) netcode_current_timestamp();

    if ( !server->config.network_simulator )
    {
        // process packets received from socket

        while ( 1 )
        {
            struct netcode_address_t from;
            memset( &from, 0, sizeof(from) );

            uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];
            
            int packet_bytes = 0;
            
            if ( server->config.override_send_and_receive )
            {
                packet_bytes = server->config.receive_packet_override( server->config.callback_context, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }
            else
            {
                if ( netcode_socket_handle_valid( server->socket_holder.ipv4.handle ) )
                    packet_bytes = netcode_socket_receive_packet( &server->socket_holder.ipv4, &from, packet_data, NETCODE_MAX_PACKET_BYTES );

                if ( packet_bytes == 0 && netcode_socket_handle_valid( server->socket_holder.ipv6.handle ) )
                    packet_bytes = netcode_socket_receive_packet( &server->socket_holder.ipv6, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }

            if ( packet_bytes == 0 )
                break;

            netcode_server_read_and_process_packet( server, &from, packet_data, packet_bytes, current_timestamp, allowed_packets );
        }
    }
    else
    {
        // process packets received from network simulator

        int num_packets_received = netcode_network_simulator_receive_packets( server->config.network_simulator, 
                                                                              &server->address, 
                                                                              NETCODE_SERVER_MAX_RECEIVE_PACKETS, 
                                                                              server->receive_packet_data, 
                                                                              server->receive_packet_bytes, 
                                                                              server->receive_from );

        int i;
        for ( i = 0; i < num_packets_received; i++ )
        {
            netcode_server_read_and_process_packet( server, 
                                                    &server->receive_from[i], 
                                                    server->receive_packet_data[i], 
                                                    server->receive_packet_bytes[i], 
                                                    current_timestamp, 
                                                    allowed_packets );

            server->config.free_function( server->config.allocator_context, server->receive_packet_data[i] );
        }
    }
}

void netcode_server_send_packets( struct netcode_server_t * server )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    int i;
    for ( i = 0; i < server->max_clients; i++ )
    {
        if ( server->client_connected[i] && !server->client_loopback[i] &&
             ( server->client_last_packet_send_time[i] + ( 1.0 / NETCODE_PACKET_SEND_RATE ) <= server->time ) )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server sent connection keep alive packet to client %d\n", i );
            struct netcode_connection_keep_alive_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
            packet.client_index = i;
            packet.max_clients = server->max_clients;
            netcode_server_send_client_packet( server, &packet, i );
        }
    }
}

void netcode_server_check_for_timeouts( struct netcode_server_t * server )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    int i;
    for ( i = 0; i < server->max_clients; i++ )
    {
        if ( !server->client_connected[i] )
            continue;

        if ( server->client_timeout[i] <= 0 )
            continue;

        if ( server->client_loopback[i] )
            continue;

        if ( ( server->time - server->client_last_packet_receive_time[i] ) >= 1.0f )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server has not received a packet from client %d for %.2f seconds\n", i, server->time - server->client_last_packet_receive_time[i] );
        }

        if ( server->client_last_packet_receive_time[i] + server->client_timeout[i] <= server->time )
        {
            netcode_printf( NETCODE_LOG_LEVEL_INFO, "server timed out client %d\n", i );
            netcode_server_disconnect_client_internal( server, i, 0, NETCODE_SERVER_CLIENT_DISCONNECT_REASON_TIMED_OUT );
        }
    }
}

int netcode_server_client_connected( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if ( !server->running )
        return 0;

    if ( client_index < 0 || client_index >= server->max_clients )
        return 0;

    return server->client_connected[client_index];
}

int netcode_server_client_disconnect_reason( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if ( !server->running )
        return NETCODE_SERVER_CLIENT_DISCONNECT_REASON_NONE;

    if ( client_index < 0 || client_index >= server->max_clients )
        return NETCODE_SERVER_CLIENT_DISCONNECT_REASON_NONE;

    return server->client_disconnect_reason[client_index];
}

uint64_t netcode_server_client_id( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if ( !server->running )
        return 0;

    if ( client_index < 0 || client_index >= server->max_clients )
        return 0;

    return server->client_id[client_index];
}

struct netcode_address_t * netcode_server_client_address( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if (!server->running)
        return NULL;

    if (client_index < 0 || client_index >= server->max_clients)
        return NULL;

    return &server->client_address[client_index];
}

uint64_t netcode_server_next_packet_sequence( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    if ( !server->running )
        return 0;
    if ( client_index < 0 || client_index >= server->max_clients )
        return 0;
    if ( !server->client_connected[client_index] )
        return 0;
    return server->client_sequence[client_index];
}

void netcode_server_send_packet( struct netcode_server_t * server, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    netcode_assert( server );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes > 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_SIZE );

    // zero byte payloads are not valid on the wire and would silently vanish at the receiver

    if ( packet_bytes <= 0 || packet_bytes > NETCODE_MAX_PACKET_SIZE )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: payload packet size is out of range (%d)\n", packet_bytes );
        return;
    }

    if ( !server->running )
        return;

    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );

    if ( client_index < 0 || client_index >= server->max_clients )
        return;

    if ( !server->client_connected[client_index] )
        return;

    if ( !server->client_loopback[client_index] )
    {
        union { struct netcode_connection_payload_packet_t packet; uint8_t data[NETCODE_MAX_PAYLOAD_BYTES*2]; } buffer;

        struct netcode_connection_payload_packet_t * packet = &buffer.packet;

        packet->packet_type = NETCODE_CONNECTION_PAYLOAD_PACKET;
        packet->payload_bytes = packet_bytes;
        memcpy( packet->payload_data, packet_data, packet_bytes );

        if ( !server->client_confirmed[client_index] )
        {
            struct netcode_connection_keep_alive_packet_t keep_alive_packet;
            keep_alive_packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
            keep_alive_packet.client_index = client_index;
            keep_alive_packet.max_clients = server->max_clients;
            netcode_server_send_client_packet( server, &keep_alive_packet, client_index );
        }

        netcode_server_send_client_packet( server, packet, client_index );
    }
    else
    {
        netcode_assert( server->config.send_loopback_packet_callback );

        server->config.send_loopback_packet_callback( server->config.callback_context,
                                                      client_index, 
                                                      packet_data, 
                                                      packet_bytes, 
                                                      server->client_sequence[client_index]++ );

        server->client_last_packet_send_time[client_index] = server->time;
    }
}

uint8_t * netcode_server_receive_packet( struct netcode_server_t * server, int client_index, int * packet_bytes, uint64_t * packet_sequence )
{
    netcode_assert( server );
    netcode_assert( packet_bytes );

    netcode_assert( client_index >= 0 );

    if ( !server->running )
        return NULL;

    netcode_assert( client_index < server->max_clients );

    if ( client_index < 0 || client_index >= server->max_clients )
        return NULL;

    if ( !server->client_connected[client_index] )
        return NULL;

    struct netcode_connection_payload_packet_t * packet = (struct netcode_connection_payload_packet_t*)
        netcode_packet_queue_pop( &server->client_packet_queue[client_index], packet_sequence );
    
    if ( packet )
    {
        netcode_assert( packet->packet_type == NETCODE_CONNECTION_PAYLOAD_PACKET );
        *packet_bytes = packet->payload_bytes;
        netcode_assert( *packet_bytes >= 0 );
        netcode_assert( *packet_bytes <= NETCODE_MAX_PAYLOAD_BYTES );
        return (uint8_t*) &packet->payload_data;
    }
    else
    {
        return NULL;
    }
}

void netcode_server_free_packet( struct netcode_server_t * server, void * packet )
{
    netcode_assert( server );
    netcode_assert( packet );
    (void) server;
    int offset = offsetof( struct netcode_connection_payload_packet_t, payload_data );
    server->config.free_function( server->config.allocator_context, ( (uint8_t*) packet ) - offset );
}

int netcode_server_num_connected_clients( struct netcode_server_t * server )
{
    netcode_assert( server );
    return server->num_connected_clients;
}

void * netcode_server_client_user_data( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    if ( !server->running )
        return NULL;
    if ( client_index < 0 || client_index >= server->max_clients )
        return NULL;
    return server->client_user_data[client_index];
}

int netcode_server_running( struct netcode_server_t * server )
{
    netcode_assert( server );
    return server->running;
}

int netcode_server_max_clients( struct netcode_server_t * server )
{
    return server->max_clients;
}

void netcode_server_update( struct netcode_server_t * server, double time )
{
    netcode_assert( server );
    server->time = time;
    netcode_server_receive_packets( server );
    netcode_server_send_packets( server );
    netcode_server_check_for_timeouts( server );
}

void netcode_server_connect_loopback_client( struct netcode_server_t * server, int client_index, uint64_t client_id, NETCODE_CONST uint8_t * user_data )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->running );

    // the server sends to a loopback client only through this callback. without it the
    // first send would call a null pointer, so refuse the slot at all, in every build.

    netcode_assert( server->config.send_loopback_packet_callback );

    if ( !server->config.send_loopback_packet_callback )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: a loopback client requires send_loopback_packet_callback\n" );
        return;
    }

    if ( !server->running )
        return;

    if ( client_index < 0 || client_index >= server->max_clients )
        return;

    netcode_assert( !server->client_connected[client_index] );

    if ( server->client_connected[client_index] )
        return;

    server->num_connected_clients++;

    netcode_assert( server->num_connected_clients <= server->max_clients );

    server->client_loopback[client_index] = 1;
    server->client_connected[client_index] = 1;
    server->client_confirmed[client_index] = 1;
    server->client_encryption_index[client_index] = -1;
    server->client_id[client_index] = client_id;
    server->client_sequence[client_index] = 0;
    server->client_disconnect_reason[client_index] = NETCODE_SERVER_CLIENT_DISCONNECT_REASON_NONE;
    memset( &server->client_address[client_index], 0, sizeof( struct netcode_address_t ) );
    server->client_last_packet_send_time[client_index] = server->time;
    server->client_last_packet_receive_time[client_index] = server->time;

    if ( user_data )
    {
        memcpy( server->client_user_data[client_index], user_data, NETCODE_USER_DATA_BYTES );
    }
    else
    {
        memset( server->client_user_data[client_index], 0, NETCODE_USER_DATA_BYTES );
    }

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server connected loopback client %.16" PRIx64 " in slot %d\n", client_id, client_index );

    if ( server->config.connect_disconnect_callback )
    {
        server->config.connect_disconnect_callback( server->config.callback_context, client_index, 1 );
    }
}

void netcode_server_disconnect_loopback_client( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->running );

    if ( !server->running )
        return;

    if ( client_index < 0 || client_index >= server->max_clients )
        return;

    netcode_assert( server->client_connected[client_index] );
    netcode_assert( server->client_loopback[client_index] );

    if ( !server->client_connected[client_index] || !server->client_loopback[client_index] )
        return;

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server disconnected loopback client %d\n", client_index );

    server->client_disconnect_reason[client_index] = NETCODE_SERVER_CLIENT_DISCONNECT_REASON_SERVER_DISCONNECT;

    // match the remote disconnect path: mark the slot disconnected so a re-entrant
    // disconnect is a no-op, keeping the rest of the slot readable in the callback.

    server->client_connected[client_index] = 0;
    server->num_connected_clients--;
    netcode_assert( server->num_connected_clients >= 0 );

    if ( server->config.connect_disconnect_callback )
    {
        server->config.connect_disconnect_callback( server->config.callback_context, client_index, 0 );
    }

    netcode_server_reset_client_slot( server, client_index );
}

int netcode_server_client_loopback( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );
    netcode_assert( server->running );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    if ( !server->running )
        return 0;
    if ( client_index < 0 || client_index >= server->max_clients )
        return 0;
    return server->client_loopback[client_index];
}

void netcode_server_process_loopback_packet( struct netcode_server_t * server, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes > 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_SIZE );
    netcode_assert( server->running );

    if ( !server->running )
        return;

    if ( client_index < 0 || client_index >= server->max_clients )
        return;

    netcode_assert( server->client_connected[client_index] );
    netcode_assert( server->client_loopback[client_index] );

    if ( !server->client_connected[client_index] || !server->client_loopback[client_index] )
        return;

    if ( packet_bytes <= 0 || packet_bytes > NETCODE_MAX_PACKET_SIZE )
        return;

    struct netcode_connection_payload_packet_t * packet = netcode_create_payload_packet( packet_bytes, server->config.allocator_context, server->config.allocate_function );
    if ( !packet )
        return;

    memcpy( packet->payload_data, packet_data, packet_bytes );

    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server processing loopback packet from client %d\n", client_index );

    server->client_last_packet_receive_time[client_index] = server->time;

    netcode_packet_queue_push( &server->client_packet_queue[client_index], packet, packet_sequence );
}

uint16_t netcode_server_get_port( struct netcode_server_t * server )
{
    netcode_assert( server );
    return server->address.type == NETCODE_ADDRESS_IPV4 ? server->socket_holder.ipv4.address.port : server->socket_holder.ipv6.address.port;
}

// ----------------------------------------------------------------

int netcode_generate_connect_token( int num_server_addresses, 
                                    NETCODE_CONST char ** public_server_addresses, 
                                    NETCODE_CONST char ** internal_server_addresses, 
                                    int expire_seconds, 
                                    int timeout_seconds,
                                    uint64_t client_id, 
                                    uint64_t protocol_id, 
                                    NETCODE_CONST uint8_t * private_key, 
                                    uint8_t * user_data, 
                                    uint8_t * output_buffer )
{
    netcode_assert( num_server_addresses > 0 );
    netcode_assert( num_server_addresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
    netcode_assert( public_server_addresses );
    netcode_assert( internal_server_addresses );

    // the parsed address arrays below are sized NETCODE_MAX_SERVERS_PER_CONNECT. an out of
    // range value here must not get through in release builds where asserts compile out.
    // every other public entry point already does this; this one was missed

    if ( num_server_addresses <= 0 || num_server_addresses > NETCODE_MAX_SERVERS_PER_CONNECT )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: number of server addresses must be in [1,%d], got %d\n", NETCODE_MAX_SERVERS_PER_CONNECT, num_server_addresses );
        return NETCODE_ERROR;
    }
    netcode_assert( private_key );
    netcode_assert( user_data );
    netcode_assert( output_buffer );

    // parse public server addresses

    struct netcode_address_t parsed_public_server_addresses[NETCODE_MAX_SERVERS_PER_CONNECT];
    int i;
    for ( i = 0; i < num_server_addresses; i++ )
    {
        if ( netcode_parse_address( public_server_addresses[i], &parsed_public_server_addresses[i] ) != NETCODE_OK )
        {
            return NETCODE_ERROR;
        }
    }

    // parse internal server addresses

     struct netcode_address_t parsed_internal_server_addresses[NETCODE_MAX_SERVERS_PER_CONNECT];
    for ( i = 0; i < num_server_addresses; i++ )
    {
        if ( netcode_parse_address( internal_server_addresses[i], &parsed_internal_server_addresses[i] ) != NETCODE_OK )
        {
            return NETCODE_ERROR;
        }
    }

    // generate a connect token

    uint8_t nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    netcode_generate_nonce(nonce);

    struct netcode_connect_token_private_t connect_token_private;
    netcode_generate_connect_token_private( &connect_token_private, client_id, timeout_seconds, num_server_addresses, parsed_internal_server_addresses, user_data );

    // write it to a buffer

    uint8_t connect_token_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    netcode_write_connect_token_private( &connect_token_private, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // encrypt the buffer

    uint64_t create_timestamp = netcode_current_timestamp();
    uint64_t expire_timestamp = ( expire_seconds >= 0 ) ? ( create_timestamp + expire_seconds ) : 0xFFFFFFFFFFFFFFFFULL;
    if ( netcode_encrypt_connect_token_private( connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, NETCODE_VERSION_INFO, protocol_id, expire_timestamp, nonce, private_key ) != NETCODE_OK )
    {
        sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
        sodium_memzero( connect_token_data, sizeof( connect_token_data ) );
        return NETCODE_ERROR;
    }

    // wrap a connect token around the private connect token data

    struct netcode_connect_token_t connect_token;
    memcpy( connect_token.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
    connect_token.protocol_id = protocol_id;
    connect_token.create_timestamp = create_timestamp;
    connect_token.expire_timestamp = expire_timestamp;
    memcpy( connect_token.nonce, nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
    memcpy( connect_token.private_data, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    connect_token.num_server_addresses = num_server_addresses;
    for ( i = 0; i < num_server_addresses; i++ )
        connect_token.server_addresses[i] = parsed_public_server_addresses[i];
    memcpy( connect_token.client_to_server_key, connect_token_private.client_to_server_key, NETCODE_KEY_BYTES );
    memcpy( connect_token.server_to_client_key, connect_token_private.server_to_client_key, NETCODE_KEY_BYTES );
    connect_token.timeout_seconds = timeout_seconds;

    // write the connect token to the output buffer

    netcode_write_connect_token( &connect_token, output_buffer, NETCODE_CONNECT_TOKEN_BYTES );

    sodium_memzero( &connect_token_private, sizeof( connect_token_private ) );
    sodium_memzero( &connect_token, sizeof( connect_token ) );
    sodium_memzero( connect_token_data, sizeof( connect_token_data ) );

    return NETCODE_OK;
}

// ---------------------------------------------------------------

#if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC

// MacOS

#include <unistd.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

void netcode_sleep( double time )
{
#if NETCODE_ENABLE_TESTS
    netcode_test_real_sleep_count++;
#endif // #if NETCODE_ENABLE_TESTS
    struct timespec ts;
    ts.tv_sec = (time_t) time;
    ts.tv_nsec = (long) ((time - (double) ( ts.tv_sec )) * 1000000000.0);
    nanosleep( &ts, NULL );
}

static uint64_t start = 0;
static mach_timebase_info_data_t timebase_info;

double netcode_time()
{
    if ( start == 0 )
    {
        mach_timebase_info( &timebase_info );
        start = mach_absolute_time();
        return 0.0;
    }
    uint64_t current = mach_absolute_time();
    return ( (double) ( current - start ) ) * ( (double) timebase_info.numer ) / ( (double) timebase_info.denom ) / 1000000000.0;
}

#elif NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX

// linux and other unix systems (openbsd, freebsd etc.)

#include <unistd.h>

// linux has CLOCK_MONOTONIC_RAW, but other unix systems only have CLOCK_MONOTONIC

#ifdef CLOCK_MONOTONIC_RAW
#define NETCODE_MONOTONIC_CLOCK CLOCK_MONOTONIC_RAW
#else
#define NETCODE_MONOTONIC_CLOCK CLOCK_MONOTONIC
#endif

void netcode_sleep( double time )
{
#if NETCODE_ENABLE_TESTS
    netcode_test_real_sleep_count++;
#endif // #if NETCODE_ENABLE_TESTS
    struct timespec ts;
    ts.tv_sec = (time_t) time;
    ts.tv_nsec = (long) ((time - (double) ( ts.tv_sec )) * 1000000000.0);
    nanosleep( &ts, NULL );
}

double netcode_time()
{
    static double start = -1;
    if ( start == -1 )
    {
        struct timespec ts;
        clock_gettime( NETCODE_MONOTONIC_CLOCK, &ts );
        start = ts.tv_sec + ( (double) ( ts.tv_nsec ) ) / 1000000000.0;
        return 0.0;
    }
    struct timespec ts;
    clock_gettime( NETCODE_MONOTONIC_CLOCK, &ts );
    double current = ts.tv_sec + ( (double) ( ts.tv_nsec ) ) / 1000000000.0;
    return current - start;
}

#elif NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

// windows

#ifndef NOMINMAX
#define NOMINMAX
#endif // #ifndef NOMINMAX
#include <windows.h>

void netcode_sleep( double time )
{
#if NETCODE_ENABLE_TESTS
    netcode_test_real_sleep_count++;
#endif // #if NETCODE_ENABLE_TESTS
    int milliseconds = (int) ( time * 1000 );
    Sleep( milliseconds );
}

static int timer_initialized = 0;
static LARGE_INTEGER timer_frequency;
static LARGE_INTEGER timer_start;

double netcode_time()
{
    if ( !timer_initialized )
    {
        QueryPerformanceFrequency( &timer_frequency );
        QueryPerformanceCounter( &timer_start );
        timer_initialized = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter( &now );
    return ( (double) ( now.QuadPart - timer_start.QuadPart ) ) / ( (double) ( timer_frequency.QuadPart ) );
}

#else

#error unsupported platform!

#endif

// ---------------------------------------------------------------

#if NETCODE_ENABLE_TESTS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

static void check_handler( NETCODE_CONST char * condition, 
                           NETCODE_CONST char * function,
                           NETCODE_CONST char * file,
                           int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifdef NETCODE_DEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define check( condition )                                                                                      \
do                                                                                                              \
{                                                                                                               \
    if ( !(condition) )                                                                                         \
    {                                                                                                           \
        check_handler( #condition, (NETCODE_CONST char*) __FUNCTION__, (char*) __FILE__, __LINE__ );            \
    }                                                                                                           \
} while(0)

static void test_crypto_aead_vectors()
{
    // Known-answer test for the two AEAD primitives netcode relies on, exercising
    // whichever implementation the running CPU selects (reference / SSSE3 / AVX2 for
    // ChaCha20, donna / SSE2 for Poly1305). Expected ciphertext was generated from
    // libsodium reference output and is unchanged across 1.0.20 -> 1.0.22. Do not
    // edit these arrays by hand: a golden failure here means the vendored crypto no
    // longer agrees with upstream, which would break every other netcode
    // implementation on the wire.
    //
    // Declared static on purpose. yojimbo vendors this file and compiles it with
    // NETCODE_ENABLE_TESTS=1 while defining its own test_crypto_aead_vectors in
    // test.cpp; internal linkage keeps the two from colliding.

    static const uint8_t kat_key[32] = {
        0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,
        0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
        0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
    };
    static const uint8_t kat_ad[12] = {
        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,
    };
    static const uint8_t kat_msg[58] = {
        0x79,0x6f,0x6a,0x69,0x6d,0x62,0x6f,0x20,0x76,0x65,0x6e,0x64,0x6f,0x72,0x65,0x64,0x20,0x6c,0x69,0x62,
        0x73,0x6f,0x64,0x69,0x75,0x6d,0x20,0x41,0x45,0x41,0x44,0x20,0x6b,0x6e,0x6f,0x77,0x6e,0x2d,0x61,0x6e,
        0x73,0x77,0x65,0x72,0x20,0x74,0x65,0x73,0x74,0x20,0x76,0x65,0x63,0x74,0x6f,0x72,0x21,0x21,
    };
    static const uint8_t kat_npub_ietf[12] = {
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,
    };
    static const uint8_t kat_ct_ietf[74] = {
        0xd5,0xae,0xb1,0x85,0x15,0x8b,0x07,0xb3,0x01,0x15,0xf0,0x59,
        0xb4,0x4e,0x9d,0x45,0x91,0x58,0xab,0xff,0xaf,0xbd,0x81,0x4f,
        0xbf,0x52,0xc2,0x4c,0xa1,0x5e,0x60,0x5f,0x58,0x63,0x31,0x96,
        0xda,0x90,0x07,0x63,0xb9,0x0c,0x21,0x46,0xf2,0xe4,0x65,0x96,
        0x7a,0x81,0x7f,0xa2,0x5d,0xd1,0x79,0xf6,0x9b,0x18,0x5d,0xe0,
        0xb6,0x57,0x93,0xbe,0x8c,0xb5,0xa9,0x75,0x98,0xa4,0x6f,0xd5,
        0xbe,0x9d,
    };
    static const uint8_t kat_npub_xchacha[24] = {
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,
        0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    };
    static const uint8_t kat_ct_xchacha[74] = {
        0x2b,0x24,0x83,0x2a,0x6c,0x9e,0x21,0x02,0x2a,0x14,0x32,0x56,
        0x4b,0x27,0x37,0x92,0x24,0x40,0xa9,0x92,0xd3,0x53,0xa7,0xa5,
        0x64,0xd3,0x8e,0x0c,0x75,0x79,0x75,0x3f,0xca,0x82,0xfa,0x85,
        0xf0,0xa6,0xac,0x08,0x9a,0x25,0xf1,0x8f,0x42,0x20,0x70,0x8e,
        0x38,0x25,0xd1,0x08,0x45,0x81,0x75,0x18,0xe4,0xd1,0x88,0xbd,
        0x92,0xfa,0x84,0xdc,0xd6,0xa3,0x9a,0x67,0x52,0x91,0x62,0xf4,
        0x86,0x7b,
    };

    uint8_t c[128];
    uint8_t m[128];
    unsigned long long clen = 0;
    unsigned long long mlen = 0;

    // ChaCha20-Poly1305 (IETF) -- the construction netcode uses on the wire

    check( crypto_aead_chacha20poly1305_ietf_encrypt( c, &clen, kat_msg, sizeof( kat_msg ), kat_ad, sizeof( kat_ad ), NULL, kat_npub_ietf, kat_key ) == 0 );
    check( clen == sizeof( kat_ct_ietf ) );
    check( memcmp( c, kat_ct_ietf, (size_t) clen ) == 0 );
    check( crypto_aead_chacha20poly1305_ietf_decrypt( m, &mlen, NULL, c, clen, kat_ad, sizeof( kat_ad ), kat_npub_ietf, kat_key ) == 0 );
    check( mlen == sizeof( kat_msg ) );
    check( memcmp( m, kat_msg, (size_t) mlen ) == 0 );

    // a tampered tag must be rejected. this is the path ACQUIRE_FENCE guards: the
    // plaintext must not be produced before authentication has completed.

    c[0] ^= 0x01;
    check( crypto_aead_chacha20poly1305_ietf_decrypt( m, &mlen, NULL, c, clen, kat_ad, sizeof( kat_ad ), kat_npub_ietf, kat_key ) != 0 );

    // XChaCha20-Poly1305

    check( crypto_aead_xchacha20poly1305_ietf_encrypt( c, &clen, kat_msg, sizeof( kat_msg ), kat_ad, sizeof( kat_ad ), NULL, kat_npub_xchacha, kat_key ) == 0 );
    check( clen == sizeof( kat_ct_xchacha ) );
    check( memcmp( c, kat_ct_xchacha, (size_t) clen ) == 0 );
    check( crypto_aead_xchacha20poly1305_ietf_decrypt( m, &mlen, NULL, c, clen, kat_ad, sizeof( kat_ad ), kat_npub_xchacha, kat_key ) == 0 );
    check( mlen == sizeof( kat_msg ) );
    check( memcmp( m, kat_msg, (size_t) mlen ) == 0 );
    c[0] ^= 0x01;
    check( crypto_aead_xchacha20poly1305_ietf_decrypt( m, &mlen, NULL, c, clen, kat_ad, sizeof( kat_ad ), kat_npub_xchacha, kat_key ) != 0 );

    // the constant-time comparison that checks the Poly1305 tag. libsodium 1.0.21
    // hardened crypto_verify_n against the compiler optimising it into something
    // branchy; these assertions pin its contract.

    uint8_t a[64], b[64];
    int i;
    for ( i = 0; i < 64; i++ ) { a[i] = (uint8_t) i; b[i] = (uint8_t) i; }
    check( crypto_verify_16( a, b ) == 0 );
    check( crypto_verify_32( a, b ) == 0 );
    check( crypto_verify_64( a, b ) == 0 );
    for ( i = 0; i < 8; i++ )
    {
        b[0] = (uint8_t) ( a[0] ^ ( 1u << i ) );
        check( crypto_verify_16( a, b ) == -1 );
        check( crypto_verify_32( a, b ) == -1 );
        check( crypto_verify_64( a, b ) == -1 );
    }
    b[0] = a[0];
    b[63] = (uint8_t) ( a[63] ^ 0x80 );
    check( crypto_verify_64( a, b ) == -1 );
}

static void test_kat_fill_pattern( uint8_t * data, int bytes, uint8_t multiplier, uint8_t offset )
{
    int i;
    for ( i = 0; i < bytes; ++i )
    {
        data[i] = (uint8_t) ( i * multiplier + offset );
    }
}

static void test_derive_packet_keys_kat()
{
    // Known-answer vectors for NETCODE 1.03 per-connection HChaCha20 key derivation:
    //   derived_key = crypto_core_hchacha20( client_salt[8] || server_nonce[8], token_key[32], NULL )
    // Inputs:
    //   c2s_key      (32B, i*3 + 1):   0104070a0d101316191c1f2225282b2e3134373a3d404346494c4f5255585b5e
    //   s2c_key      (32B, i*5 + 2):   02070c11161b20252a2f34393e43484d52575c61666b70757a7f84898e93989d
    //   client_salt  (8B,  i*19 + 21): 15283b4e6174879a
    //   server_nonce (8B,  i*23 + 29): 1d344b627990a7be
    // Outputs:
    //   derived_c2s_key: a5225053ddedabd2fdf737924c4e7cda5c2f334fe78957b499adf7e0ad1471df
    //   derived_s2c_key: 8be2caf26f26a9bc1710172def250ad1b5bcf543138d7534994ae65ddc25a6fb

    static const uint8_t kat_derived_c2s_key[NETCODE_KEY_BYTES] = {
        0xa5,0x22,0x50,0x53,0xdd,0xed,0xab,0xd2,0xfd,0xf7,0x37,0x92,0x4c,0x4e,0x7c,0xda,
        0x5c,0x2f,0x33,0x4f,0xe7,0x89,0x57,0xb4,0x99,0xad,0xf7,0xe0,0xad,0x14,0x71,0xdf,
    };
    static const uint8_t kat_derived_s2c_key[NETCODE_KEY_BYTES] = {
        0x8b,0xe2,0xca,0xf2,0x6f,0x26,0xa9,0xbc,0x17,0x10,0x17,0x2d,0xef,0x25,0x0a,0xd1,
        0xb5,0xbc,0xf5,0x43,0x13,0x8d,0x75,0x34,0x99,0x4a,0xe6,0x5d,0xdc,0x25,0xa6,0xfb,
    };

    uint8_t c2s_key[NETCODE_KEY_BYTES];
    uint8_t s2c_key[NETCODE_KEY_BYTES];
    uint8_t client_salt[NETCODE_CONNECT_SALT_BYTES];
    uint8_t server_nonce[NETCODE_SERVER_NONCE_BYTES];
    uint8_t derived_c2s_key[NETCODE_KEY_BYTES];
    uint8_t derived_s2c_key[NETCODE_KEY_BYTES];
    uint8_t altered_key[NETCODE_KEY_BYTES];

    test_kat_fill_pattern( c2s_key, sizeof( c2s_key ), 3, 1 );
    test_kat_fill_pattern( s2c_key, sizeof( s2c_key ), 5, 2 );
    test_kat_fill_pattern( client_salt, sizeof( client_salt ), 19, 21 );
    test_kat_fill_pattern( server_nonce, sizeof( server_nonce ), 23, 29 );

    netcode_derive_packet_key( derived_c2s_key, c2s_key, client_salt, server_nonce );
    netcode_derive_packet_key( derived_s2c_key, s2c_key, client_salt, server_nonce );

    check( memcmp( derived_c2s_key, kat_derived_c2s_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( derived_s2c_key, kat_derived_s2c_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( derived_c2s_key, c2s_key, NETCODE_KEY_BYTES ) != 0 );
    check( memcmp( derived_s2c_key, s2c_key, NETCODE_KEY_BYTES ) != 0 );
    check( memcmp( derived_c2s_key, derived_s2c_key, NETCODE_KEY_BYTES ) != 0 );

    client_salt[0] ^= 0x01;
    netcode_derive_packet_key( altered_key, c2s_key, client_salt, server_nonce );
    check( memcmp( altered_key, derived_c2s_key, NETCODE_KEY_BYTES ) != 0 );
    client_salt[0] ^= 0x01;

    server_nonce[0] ^= 0x01;
    netcode_derive_packet_key( altered_key, s2c_key, client_salt, server_nonce );
    check( memcmp( altered_key, derived_s2c_key, NETCODE_KEY_BYTES ) != 0 );
    server_nonce[0] ^= 0x01;
}

static void test_wire_kat_vectors()
{
    // Known-answer wire vectors for netcode 1.03 encrypted packets, encrypted private
    // connect token, and encrypted challenge token constructions.
    static const uint8_t kat_packet_keep_alive[29] = {
        0x44,0x77,0x66,0x55,0x44,0x8e,0x81,0x04,0xbd,0xf6,0xca,0x06,0xd9,0xa8,0x32,0x8e,
        0x8c,0x6b,0x42,0xbb,0x0a,0x7e,0x84,0xd1,0xab,0x70,0xf5,0xaa,0x31,
    };
    static const uint8_t kat_packet_denied[33] = {
        0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x1d,0x34,0x4b,0x62,0x79,0x90,0xa7,
        0xbe,0xea,0xa7,0xd8,0x85,0x75,0x91,0xae,0x63,0x25,0xd0,0xed,0x98,0xfd,0x6a,0x36,
        0xe5,
    };
    static const uint8_t kat_packet_challenge[341] = {
        0x82,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x1d,0x34,0x4b,0x62,0x79,0x90,0xa7,
        0xbe,0xf9,0x17,0xe3,0xd7,0xd3,0x9f,0x8c,0x91,0x85,0x5b,0xe7,0x2a,0xa4,0x34,0x42,
        0x8d,0x7b,0xc6,0x42,0x6b,0xe5,0x1c,0x90,0x06,0x69,0xb8,0x5e,0x1f,0x0c,0x60,0xf0,
        0x6d,0x74,0xc8,0x0f,0x93,0x01,0x13,0x40,0xb5,0x7d,0x46,0x3e,0xa4,0x7f,0x72,0x57,
        0x20,0xb1,0x5d,0x74,0x22,0x07,0xa5,0xd9,0xfe,0xe5,0x79,0x11,0x7b,0x0c,0x50,0xf3,
        0xd6,0x86,0xf6,0x01,0x82,0xec,0x61,0x71,0x43,0x1d,0x65,0x98,0x8a,0xbc,0xe1,0xfb,
        0x8f,0x92,0x44,0x79,0x9b,0xf9,0xfe,0xe8,0x6f,0xd3,0x87,0x1d,0xd4,0x39,0xa2,0x83,
        0x28,0x16,0x98,0xc8,0xe5,0x8a,0xf7,0xc2,0x62,0xdc,0x75,0x31,0x38,0x2c,0xef,0xd1,
        0xa7,0xb7,0x54,0x0e,0xbd,0x04,0x2e,0x53,0xe6,0xe0,0x47,0x07,0x68,0xb9,0x8d,0x59,
        0x44,0x3c,0x27,0x08,0x1e,0xaa,0x51,0x08,0x0c,0x52,0xa6,0xdf,0x14,0x6e,0xa8,0x7a,
        0x4d,0xe9,0x99,0x84,0xa8,0xfe,0x49,0x41,0x01,0x46,0x89,0xee,0xbb,0xfe,0xa0,0xb6,
        0x06,0x8e,0xc1,0x29,0x73,0x34,0xa1,0xe0,0x2a,0x6e,0x0f,0xee,0xe9,0x3b,0xd1,0x71,
        0x97,0x8d,0x36,0x6d,0x2d,0x0c,0x1f,0x38,0x3e,0xd6,0xd6,0x62,0x84,0x2b,0x3f,0x8a,
        0x2e,0x5e,0xec,0x1b,0xa8,0xac,0xea,0x0c,0x7c,0x14,0xa6,0x3a,0x8c,0x8f,0x81,0x90,
        0xf1,0x33,0xf1,0x8e,0x8f,0xff,0x17,0x40,0x07,0x57,0xc8,0xba,0x69,0xda,0x47,0x93,
        0xf6,0xfd,0x52,0x71,0x9d,0x76,0x30,0x56,0xb1,0x2c,0xba,0x04,0x42,0x07,0x7e,0xe0,
        0x35,0xb2,0xa3,0x9e,0x49,0x61,0xcc,0xfd,0x4e,0x26,0xc6,0x19,0xaa,0x63,0x92,0x40,
        0x40,0x8f,0x3e,0x38,0xd7,0xea,0xd8,0x84,0x04,0xe6,0x9c,0x49,0xb1,0xe9,0x99,0x78,
        0xc6,0xf2,0xda,0xa8,0xd4,0x31,0xf3,0x5b,0x26,0xca,0xe7,0xc6,0xf5,0xa9,0x9c,0x64,
        0x11,0x7c,0xb0,0x92,0xcc,0xac,0x74,0xc0,0xf3,0xbb,0xa7,0xe2,0xec,0x2c,0x1c,0x2c,
        0x6f,0x0b,0xa3,0xf6,0x76,0x24,0xf9,0xd4,0x9d,0xd3,0x12,0x4c,0xfa,0xf7,0xae,0xf6,
        0xfd,0x46,0xdf,0x8c,0xa9,
    };
    static const uint8_t kat_connect_token_private[1024] = {
        0x9b,0xa9,0xab,0x36,0x41,0xa1,0x1f,0x10,0x02,0x39,0x1e,0x02,0xd1,0x09,0xd6,0xd5,
        0xdb,0xc7,0x90,0xeb,0x0d,0x91,0x01,0x43,0xd2,0x4e,0x31,0x3f,0xa7,0xa8,0x2d,0x80,
        0x87,0x5f,0x0a,0xb2,0x5d,0x06,0xd8,0xda,0x13,0x9b,0xa5,0x4d,0x56,0x62,0x00,0xcd,
        0xb2,0xb1,0x99,0xd4,0x26,0xb6,0xd5,0x9e,0x8b,0x68,0x47,0xdc,0xca,0x9f,0x74,0x3d,
        0xf7,0x43,0x17,0xf2,0x23,0x15,0xe6,0x67,0xcc,0xc3,0xff,0xbd,0x57,0x76,0x32,0x5b,
        0xa0,0xde,0x46,0x16,0x32,0x04,0x53,0xb9,0xbd,0x88,0xbd,0x0d,0xd7,0x80,0x0b,0x75,
        0x7c,0xf2,0x76,0xfb,0xb6,0x3f,0xd3,0xe2,0x03,0xcd,0x9e,0xf2,0x3e,0xca,0x9f,0x21,
        0xc7,0xfb,0x54,0x9c,0x7a,0x4f,0x09,0x2a,0xda,0xea,0xbe,0xa6,0x66,0x0c,0x82,0x3d,
        0xa4,0xc8,0x22,0xc3,0x96,0xac,0x31,0x78,0x25,0x84,0x78,0xde,0x37,0x34,0x1b,0x3b,
        0xb2,0xd3,0x95,0x17,0xbe,0x9d,0xa5,0x24,0xd5,0xe7,0x5a,0x1e,0x08,0x00,0xd2,0xa7,
        0x1d,0x39,0xbe,0x41,0x3e,0x52,0xf9,0x8f,0x88,0xac,0x96,0x52,0xa3,0x1d,0xa0,0x11,
        0xa3,0x0d,0x0d,0xf3,0x39,0x1a,0xa9,0x49,0xae,0x1a,0x7b,0x9f,0xd2,0xbf,0x18,0xde,
        0x92,0x34,0x78,0x40,0x37,0x7c,0xa9,0x2c,0x19,0x37,0xfd,0x63,0x1e,0x38,0xdf,0xda,
        0xa1,0xdc,0x9b,0x7d,0x82,0xf2,0xb4,0xde,0x1a,0x4b,0xda,0x04,0x39,0x25,0x10,0x4a,
        0xbe,0x46,0x2e,0x49,0x33,0x32,0x63,0x13,0x4b,0xc0,0x5e,0xe3,0xb4,0x49,0x57,0xfc,
        0xec,0xf7,0xca,0xf9,0x19,0x3d,0x50,0x7d,0x38,0x2e,0x29,0xcc,0x07,0x18,0xfd,0x94,
        0xd7,0x48,0x18,0x15,0x3e,0xa5,0x04,0xf9,0xdf,0x47,0xea,0x4e,0xba,0x7e,0x88,0xbe,
        0xd8,0x85,0x72,0x3d,0xcb,0x62,0x7b,0xc4,0x42,0x10,0x48,0x4d,0x35,0xdd,0xc1,0x28,
        0x15,0xab,0xfa,0xb0,0x67,0x69,0x95,0xaf,0x0a,0x9c,0xec,0xb0,0x6a,0xb6,0x87,0xe5,
        0x7d,0x58,0xbb,0xa7,0xe5,0x74,0xfc,0x34,0x3e,0x2f,0xa5,0xa1,0x35,0x99,0x63,0xff,
        0xa1,0xc1,0x3a,0x23,0x3d,0xd5,0x6f,0xd3,0x3f,0xc0,0x4c,0x3f,0x01,0x6b,0xc4,0x52,
        0xf5,0xd0,0xcd,0x46,0xbf,0x00,0xfc,0x4c,0xbd,0xed,0x20,0xf2,0xb3,0xf8,0xcc,0xfa,
        0x75,0x95,0x63,0xc5,0xb7,0x54,0x38,0x3d,0x42,0xb5,0xd6,0xc5,0xe8,0x4a,0x8b,0x27,
        0x16,0xeb,0x91,0x48,0x26,0x79,0xe3,0x45,0xb8,0xc7,0x64,0xfb,0xda,0x4b,0x2f,0x98,
        0x0b,0x3e,0x83,0xf4,0xa7,0x95,0xfd,0x24,0xeb,0x2a,0x5e,0x0c,0x64,0xb3,0xf2,0x21,
        0xf7,0xa9,0xc8,0x32,0x25,0xb5,0x03,0xcf,0x11,0xf0,0xda,0x28,0x0c,0x91,0xb2,0x74,
        0x17,0xc6,0x98,0xa0,0xec,0x46,0x20,0x09,0x50,0x6a,0x97,0xb8,0x50,0xdc,0x5d,0x18,
        0x50,0x45,0xdf,0xff,0xbc,0x6e,0x3e,0xa4,0xfd,0x17,0xb7,0x62,0x43,0xd2,0xc6,0xa5,
        0x63,0xe9,0x50,0x3b,0xef,0xe4,0xc5,0x1f,0x22,0xf2,0x73,0x85,0x6a,0x84,0x95,0x28,
        0x46,0x10,0x04,0xa1,0x86,0xfe,0x67,0x98,0xaa,0xaf,0x1e,0x9b,0xe8,0x1d,0x09,0x2e,
        0xce,0xe2,0x68,0xf1,0x7b,0xdb,0xac,0xb4,0xdc,0x5c,0xad,0x7f,0x9c,0xd1,0x8c,0x51,
        0x1c,0x12,0x0f,0x96,0xb9,0x44,0x51,0x45,0x07,0x7d,0xce,0x0f,0xe1,0xf2,0x6c,0xd9,
        0x4f,0x6c,0x58,0xae,0xe0,0x73,0x35,0xa2,0xf4,0x7b,0x60,0xb9,0xca,0x9c,0xca,0x25,
        0x16,0x58,0x0c,0x2c,0x8d,0xb8,0x10,0x56,0x6c,0x95,0x48,0x7d,0xe4,0x98,0x56,0x2b,
        0xf6,0x00,0xc0,0xcf,0x27,0x53,0xfc,0xf6,0x44,0x35,0xbf,0xc5,0x2d,0x87,0xdd,0x6e,
        0xef,0x11,0x13,0x81,0x9e,0x86,0x1f,0xaa,0x1b,0xba,0xd3,0xa3,0xa3,0x72,0x49,0x02,
        0x20,0xd6,0x22,0x8c,0x04,0xba,0xa3,0x3e,0x1d,0x25,0x2d,0xed,0x52,0x24,0x04,0x5c,
        0x40,0x87,0x72,0xdc,0x2d,0x18,0x27,0xc6,0x84,0x62,0xc9,0x51,0x7c,0x93,0x06,0x10,
        0x09,0x3f,0xe3,0x8d,0xc9,0xdb,0x51,0xe0,0x95,0x72,0x4b,0xfb,0xaa,0xbf,0x4f,0x2b,
        0x73,0x07,0x3b,0x3b,0xde,0xf8,0xd6,0x35,0x9a,0xb8,0xb6,0x13,0x4c,0x2f,0xcf,0xc7,
        0x94,0x18,0x3d,0xcf,0x3e,0xf3,0xdb,0x7e,0x12,0x07,0x54,0x4f,0xca,0xfc,0x1f,0x61,
        0x2a,0x84,0x4e,0x52,0xc6,0xaa,0xab,0x55,0x92,0x5f,0x6e,0x3b,0x1f,0xca,0x62,0xdd,
        0xbb,0x75,0x73,0xf3,0xbe,0xba,0x8c,0x65,0xcb,0x3e,0xd8,0x15,0xee,0xd2,0xc2,0x73,
        0xdf,0x4c,0x70,0x70,0x83,0xe1,0x07,0xd5,0x1a,0xe0,0x34,0xb4,0x09,0x09,0x32,0xa2,
        0x12,0xf0,0x6f,0x7c,0xa1,0x6b,0x32,0xfb,0xfb,0xde,0x90,0x1a,0x75,0x0c,0x63,0xa7,
        0xeb,0x47,0x08,0x46,0x43,0x7a,0x43,0x8b,0x71,0x1e,0x55,0xcc,0xaa,0x15,0xa1,0xb3,
        0x8c,0x3c,0x02,0x32,0xd6,0x5f,0x49,0xb8,0x14,0x5d,0xed,0x5e,0xc0,0x2b,0xcc,0xb7,
        0xec,0xe7,0xe6,0xe8,0x6d,0x24,0x40,0xbd,0xa6,0x81,0x98,0x9c,0xbe,0x2e,0x58,0x81,
        0x4a,0x0b,0x5e,0xc2,0xdc,0x96,0x3c,0x30,0x56,0xe0,0x61,0x51,0x74,0x2e,0x2b,0x3d,
        0xe5,0xba,0xa9,0x55,0x10,0xb7,0xa2,0xe9,0xa1,0x21,0xc9,0x71,0xab,0x96,0xe8,0x9d,
        0xf0,0xac,0x3f,0x1d,0xc6,0x18,0xf5,0x62,0x9f,0xc6,0x45,0x4c,0xf9,0x26,0xac,0x0a,
        0x8f,0x6d,0xf2,0xa1,0xa0,0x54,0x78,0x80,0x40,0x32,0x36,0x28,0x8e,0x32,0x58,0x80,
        0x71,0x8b,0xe9,0xcf,0x53,0x90,0xef,0x11,0x35,0xc0,0xd7,0x01,0x79,0x38,0x6e,0xc6,
        0xba,0x94,0xa2,0x43,0x7b,0x52,0x13,0x60,0x6f,0xc0,0xd9,0x5f,0x13,0x2d,0x54,0xd3,
        0x32,0x62,0xd1,0x4d,0x19,0x7b,0x3f,0xf0,0x20,0xd5,0x14,0x57,0x09,0x81,0x59,0xe9,
        0xa9,0x51,0x4f,0x0f,0x81,0x3f,0x30,0x55,0xbd,0xa7,0x0d,0x90,0x24,0xc8,0xeb,0xc6,
        0xbf,0x1f,0x4b,0xde,0x68,0x5f,0x84,0x19,0x97,0xaa,0xb7,0x32,0x7f,0x5f,0x77,0x11,
        0x15,0xbf,0x4c,0x91,0xb9,0xa4,0xdd,0x7f,0xb2,0xac,0x3f,0x67,0x7c,0xf4,0x43,0x45,
        0x0c,0x84,0x13,0xb2,0x6d,0xd2,0xce,0x1d,0x4c,0xd5,0xf3,0x41,0x6c,0xc0,0xe5,0x19,
        0xe4,0x87,0xc9,0xec,0x3b,0x12,0xdb,0xf7,0x97,0x61,0x4b,0xa9,0x14,0xe0,0xf1,0x07,
        0x4a,0x5c,0x97,0x95,0x85,0x39,0x99,0x54,0x72,0xc4,0xca,0xd8,0x55,0x58,0x1f,0x47,
        0x2a,0x1f,0xb9,0x46,0xcc,0x10,0xd1,0x3e,0xa4,0xd9,0xaf,0x94,0xde,0x5d,0x66,0xbb,
        0xe3,0x92,0x4b,0x12,0xdc,0x25,0x30,0xa4,0x5c,0x12,0x48,0x7f,0xfb,0xd2,0x4d,0xfe,
        0x4d,0x14,0x07,0x22,0xfa,0xb8,0xda,0xb8,0x51,0x32,0x0e,0x57,0x54,0x15,0x89,0x28,
    };
    static const uint8_t kat_challenge_token[300] = {
        0x90,0xdd,0x66,0x4e,0x43,0x0a,0xaf,0x1f,0x6e,0x69,0x65,0xda,0x41,0x17,0xde,0xeb,
        0x38,0x47,0x06,0xe5,0x5e,0x84,0xb2,0xb5,0x0c,0x33,0xb9,0x83,0x45,0xbb,0xdc,0x8c,
        0x84,0xbb,0x85,0x25,0x9d,0xf0,0x2c,0x82,0x1e,0x20,0xa1,0x4d,0xeb,0x55,0x6c,0xdb,
        0xc0,0xfd,0x3c,0x52,0x69,0xff,0xad,0xfc,0xa3,0xef,0x2a,0x32,0x9f,0x70,0xe3,0xcb,
        0x65,0x2c,0xcb,0xf3,0x16,0x7f,0xd2,0x88,0x48,0x7c,0x4e,0x8f,0xbb,0xa2,0x2e,0x64,
        0x94,0xc2,0x15,0xa3,0xc1,0xdf,0x36,0x3e,0x66,0x74,0x0e,0x62,0x39,0xe7,0xfd,0x8e,
        0x99,0xa4,0x20,0x18,0xb3,0x7d,0x61,0x8d,0xf6,0x9a,0x6d,0x7f,0xaf,0xcb,0x11,0x51,
        0xee,0xf7,0x13,0x90,0x1f,0xda,0xa4,0x25,0x09,0x04,0x50,0x37,0xc3,0xf0,0x92,0x8b,
        0x55,0x47,0x19,0x71,0xdd,0xb8,0x65,0xf0,0x5a,0xed,0x73,0xac,0x60,0x77,0xfa,0x12,
        0x6e,0x33,0xb5,0x70,0xce,0x3b,0x0b,0x10,0xa3,0x0b,0x36,0xc5,0xf2,0x83,0x55,0xd8,
        0xa2,0x03,0x97,0x02,0x05,0x1b,0xcf,0x91,0xd6,0xdb,0x76,0xf3,0xb3,0x29,0x52,0x1a,
        0xc7,0x52,0xc7,0xa8,0x91,0x6e,0x3d,0x69,0x81,0xc1,0xa6,0x22,0xa7,0x85,0xea,0xfc,
        0x5b,0x08,0x83,0x8e,0xc5,0x4e,0x31,0xc1,0xa4,0x5b,0x49,0x4f,0x20,0x86,0x07,0x58,
        0x45,0xbf,0xde,0x03,0xed,0x5f,0xe2,0xfa,0xfc,0xa1,0xbf,0xe8,0x00,0x66,0x27,0xb7,
        0x57,0x3a,0xec,0xdf,0xc9,0x7d,0x1b,0x06,0x01,0xc4,0xb6,0x02,0x2f,0x8f,0xa6,0x24,
        0x70,0x41,0x4a,0x92,0x63,0x01,0x08,0xa6,0x68,0xb2,0xbf,0x57,0x47,0x8d,0x6a,0x3a,
        0xe9,0xc6,0xbc,0xb0,0x87,0x08,0x16,0x9b,0x2e,0x02,0xba,0x68,0x1f,0xe6,0x2f,0x23,
        0x58,0xa7,0xd8,0x3f,0xf2,0x67,0x03,0xd2,0xf5,0xd1,0x0f,0x6e,0x66,0x2c,0x66,0x8d,
        0x54,0x1c,0x0c,0xcf,0xeb,0x94,0x99,0xb5,0x0f,0xa5,0x06,0x96,
    };

    const uint64_t protocol_id = 0x1122334455667788ULL;
    const uint64_t client_id = 0x0102030405060708ULL;
    const int timeout_seconds = 15;
    const uint64_t expire_timestamp = 0xAABBCCDD11223344ULL;
    const uint64_t packet_sequence = 0x44556677ULL;
    const uint64_t challenge_sequence = 0xFFEEDDCCBBAA9988ULL;

    uint8_t packet_key[NETCODE_KEY_BYTES];
    uint8_t private_key[NETCODE_KEY_BYTES];
    uint8_t challenge_key[NETCODE_KEY_BYTES];
    uint8_t c2s_key[NETCODE_KEY_BYTES];
    uint8_t s2c_key[NETCODE_KEY_BYTES];
    uint8_t connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];

    test_kat_fill_pattern( packet_key, sizeof( packet_key ), 7, 1 );
    test_kat_fill_pattern( private_key, sizeof( private_key ), 11, 3 );
    test_kat_fill_pattern( challenge_key, sizeof( challenge_key ), 13, 5 );
    test_kat_fill_pattern( c2s_key, sizeof( c2s_key ), 3, 1 );
    test_kat_fill_pattern( s2c_key, sizeof( s2c_key ), 5, 2 );
    test_kat_fill_pattern( connect_token_nonce, sizeof( connect_token_nonce ), 17, 9 );

    // 1. Encrypted wire packet known-answer vector (encrypt, decrypt, and tamper rejection)
    {
        struct netcode_connection_keep_alive_packet_t in_pkt;
        in_pkt.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
        in_pkt.client_index = 10;
        in_pkt.max_clients = 16;

        uint8_t pkt_buf[NETCODE_MAX_PACKET_BYTES];
        int bytes_written = netcode_write_packet( &in_pkt, pkt_buf, sizeof( pkt_buf ), packet_sequence, packet_key, protocol_id );
        check( bytes_written == (int) sizeof( kat_packet_keep_alive ) );
        check( memcmp( pkt_buf, kat_packet_keep_alive, sizeof( kat_packet_keep_alive ) ) == 0 );

        uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
        memset( allowed_packets, 1, sizeof( allowed_packets ) );

        uint8_t pkt_work[sizeof( kat_packet_keep_alive )];
        memcpy( pkt_work, kat_packet_keep_alive, sizeof( kat_packet_keep_alive ) );
        uint64_t read_sequence = 0;
        struct netcode_connection_keep_alive_packet_t * out_pkt = (struct netcode_connection_keep_alive_packet_t*)
            netcode_read_packet( pkt_work, sizeof( pkt_work ), &read_sequence, packet_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL );
        check( out_pkt != NULL );
        check( read_sequence == packet_sequence );
        check( out_pkt->packet_type == NETCODE_CONNECTION_KEEP_ALIVE_PACKET );
        check( out_pkt->client_index == 10 );
        check( out_pkt->max_clients == 16 );
        free( out_pkt );

        // AD protocol_id flip
        memcpy( pkt_work, kat_packet_keep_alive, sizeof( kat_packet_keep_alive ) );
        check( netcode_read_packet( pkt_work, sizeof( pkt_work ), &read_sequence, packet_key, protocol_id ^ 1ULL, 0, 0, NULL, allowed_packets, NULL, NULL, NULL ) == NULL );

        // AD prefix byte flip
        memcpy( pkt_work, kat_packet_keep_alive, sizeof( kat_packet_keep_alive ) );
        pkt_work[0] ^= 0x01;
        check( netcode_read_packet( pkt_work, sizeof( pkt_work ), &read_sequence, packet_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL ) == NULL );

        // Nonce / sequence byte flip
        memcpy( pkt_work, kat_packet_keep_alive, sizeof( kat_packet_keep_alive ) );
        pkt_work[1] ^= 0x01;
        check( netcode_read_packet( pkt_work, sizeof( pkt_work ), &read_sequence, packet_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL ) == NULL );

        // Ciphertext byte flip
        memcpy( pkt_work, kat_packet_keep_alive, sizeof( kat_packet_keep_alive ) );
        pkt_work[5] ^= 0x01;
        check( netcode_read_packet( pkt_work, sizeof( pkt_work ), &read_sequence, packet_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL ) == NULL );

        // MAC tag byte flip
        memcpy( pkt_work, kat_packet_keep_alive, sizeof( kat_packet_keep_alive ) );
        pkt_work[sizeof( pkt_work ) - 1] ^= 0x01;
        check( netcode_read_packet( pkt_work, sizeof( pkt_work ), &read_sequence, packet_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL ) == NULL );
    }

    // 2. Encrypted private connect token known-answer vector (encrypt, decrypt, and tamper rejection)
    {
        struct netcode_connect_token_private_t in_priv;
        memset( &in_priv, 0, sizeof( in_priv ) );
        in_priv.client_id = client_id;
        in_priv.timeout_seconds = timeout_seconds;
        in_priv.num_server_addresses = 3;
        check( netcode_parse_address( "127.0.0.1:40000", &in_priv.server_addresses[0] ) == NETCODE_OK );
        check( netcode_parse_address( "[fe80::202:b3ff:fe1e:8329]:50000", &in_priv.server_addresses[1] ) == NETCODE_OK );
        check( netcode_parse_address( "10.24.93.7:12345", &in_priv.server_addresses[2] ) == NETCODE_OK );
        memcpy( in_priv.client_to_server_key, c2s_key, NETCODE_KEY_BYTES );
        memcpy( in_priv.server_to_client_key, s2c_key, NETCODE_KEY_BYTES );
        test_kat_fill_pattern( in_priv.user_data, NETCODE_USER_DATA_BYTES, 7, 13 );

        uint8_t priv_plain[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
        uint8_t priv_buf[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
        netcode_write_connect_token_private( &in_priv, priv_plain, sizeof( priv_plain ) );
        memcpy( priv_buf, priv_plain, sizeof( priv_buf ) );

        check( netcode_encrypt_connect_token_private( priv_buf, sizeof( priv_buf ), NETCODE_VERSION_INFO, protocol_id, expire_timestamp, connect_token_nonce, private_key ) == NETCODE_OK );
        check( memcmp( priv_buf, kat_connect_token_private, sizeof( kat_connect_token_private ) ) == 0 );

        uint8_t priv_work[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
        memcpy( priv_work, kat_connect_token_private, sizeof( priv_work ) );
        check( netcode_decrypt_connect_token_private( priv_work, sizeof( priv_work ), NETCODE_VERSION_INFO, protocol_id, expire_timestamp, connect_token_nonce, private_key ) == NETCODE_OK );
        check( memcmp( priv_work, priv_plain, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES ) == 0 );

        struct netcode_connect_token_private_t out_priv;
        memset( &out_priv, 0, sizeof( out_priv ) );
        check( netcode_read_connect_token_private( priv_work, sizeof( priv_work ), &out_priv ) == NETCODE_OK );
        check( out_priv.client_id == in_priv.client_id );
        check( out_priv.timeout_seconds == in_priv.timeout_seconds );
        check( out_priv.num_server_addresses == in_priv.num_server_addresses );
        check( netcode_address_equal( &out_priv.server_addresses[0], &in_priv.server_addresses[0] ) );
        check( netcode_address_equal( &out_priv.server_addresses[1], &in_priv.server_addresses[1] ) );
        check( netcode_address_equal( &out_priv.server_addresses[2], &in_priv.server_addresses[2] ) );
        check( memcmp( out_priv.client_to_server_key, in_priv.client_to_server_key, NETCODE_KEY_BYTES ) == 0 );
        check( memcmp( out_priv.server_to_client_key, in_priv.server_to_client_key, NETCODE_KEY_BYTES ) == 0 );
        check( memcmp( out_priv.user_data, in_priv.user_data, NETCODE_USER_DATA_BYTES ) == 0 );

        // AD version_info flip
        uint8_t bad_version[NETCODE_VERSION_INFO_BYTES];
        memcpy( bad_version, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
        bad_version[0] ^= 0x01;
        memcpy( priv_work, kat_connect_token_private, sizeof( priv_work ) );
        check( netcode_decrypt_connect_token_private( priv_work, sizeof( priv_work ), bad_version, protocol_id, expire_timestamp, connect_token_nonce, private_key ) != NETCODE_OK );

        // AD protocol_id flip
        memcpy( priv_work, kat_connect_token_private, sizeof( priv_work ) );
        check( netcode_decrypt_connect_token_private( priv_work, sizeof( priv_work ), NETCODE_VERSION_INFO, protocol_id ^ 1ULL, expire_timestamp, connect_token_nonce, private_key ) != NETCODE_OK );

        // AD expire_timestamp flip
        memcpy( priv_work, kat_connect_token_private, sizeof( priv_work ) );
        check( netcode_decrypt_connect_token_private( priv_work, sizeof( priv_work ), NETCODE_VERSION_INFO, protocol_id, expire_timestamp ^ 1ULL, connect_token_nonce, private_key ) != NETCODE_OK );

        // Nonce flip
        uint8_t bad_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
        memcpy( bad_nonce, connect_token_nonce, sizeof( bad_nonce ) );
        bad_nonce[0] ^= 0x01;
        memcpy( priv_work, kat_connect_token_private, sizeof( priv_work ) );
        check( netcode_decrypt_connect_token_private( priv_work, sizeof( priv_work ), NETCODE_VERSION_INFO, protocol_id, expire_timestamp, bad_nonce, private_key ) != NETCODE_OK );

        // Ciphertext flip
        memcpy( priv_work, kat_connect_token_private, sizeof( priv_work ) );
        priv_work[0] ^= 0x01;
        check( netcode_decrypt_connect_token_private( priv_work, sizeof( priv_work ), NETCODE_VERSION_INFO, protocol_id, expire_timestamp, connect_token_nonce, private_key ) != NETCODE_OK );

        // MAC tag flip
        memcpy( priv_work, kat_connect_token_private, sizeof( priv_work ) );
        priv_work[sizeof( priv_work ) - 1] ^= 0x01;
        check( netcode_decrypt_connect_token_private( priv_work, sizeof( priv_work ), NETCODE_VERSION_INFO, protocol_id, expire_timestamp, connect_token_nonce, private_key ) != NETCODE_OK );
    }

    // 3. Encrypted challenge token known-answer vector (encrypt, decrypt, and tamper rejection)
    {
        struct netcode_challenge_token_t in_chal;
        memset( &in_chal, 0, sizeof( in_chal ) );
        in_chal.client_id = client_id;
        test_kat_fill_pattern( in_chal.user_data, NETCODE_USER_DATA_BYTES, 9, 17 );

        uint8_t chal_plain[NETCODE_CHALLENGE_TOKEN_BYTES];
        uint8_t chal_buf[NETCODE_CHALLENGE_TOKEN_BYTES];
        netcode_write_challenge_token( &in_chal, chal_plain, sizeof( chal_plain ) );
        memcpy( chal_buf, chal_plain, sizeof( chal_buf ) );

        check( netcode_encrypt_challenge_token( chal_buf, sizeof( chal_buf ), challenge_sequence, challenge_key ) == NETCODE_OK );
        check( memcmp( chal_buf, kat_challenge_token, sizeof( kat_challenge_token ) ) == 0 );

        uint8_t chal_work[NETCODE_CHALLENGE_TOKEN_BYTES];
        memcpy( chal_work, kat_challenge_token, sizeof( chal_work ) );
        check( netcode_decrypt_challenge_token( chal_work, sizeof( chal_work ), challenge_sequence, challenge_key ) == NETCODE_OK );
        check( memcmp( chal_work, chal_plain, NETCODE_CHALLENGE_TOKEN_BYTES - NETCODE_MAC_BYTES ) == 0 );

        struct netcode_challenge_token_t out_chal;
        memset( &out_chal, 0, sizeof( out_chal ) );
        check( netcode_read_challenge_token( chal_work, sizeof( chal_work ), &out_chal ) == NETCODE_OK );
        check( out_chal.client_id == in_chal.client_id );
        check( memcmp( out_chal.user_data, in_chal.user_data, NETCODE_USER_DATA_BYTES ) == 0 );

        // Nonce / sequence flip
        memcpy( chal_work, kat_challenge_token, sizeof( chal_work ) );
        check( netcode_decrypt_challenge_token( chal_work, sizeof( chal_work ), challenge_sequence ^ 1ULL, challenge_key ) != NETCODE_OK );

        // Ciphertext flip
        memcpy( chal_work, kat_challenge_token, sizeof( chal_work ) );
        chal_work[0] ^= 0x01;
        check( netcode_decrypt_challenge_token( chal_work, sizeof( chal_work ), challenge_sequence, challenge_key ) != NETCODE_OK );

        // MAC tag flip
        memcpy( chal_work, kat_challenge_token, sizeof( chal_work ) );
        chal_work[sizeof( chal_work ) - 1] ^= 0x01;
        check( netcode_decrypt_challenge_token( chal_work, sizeof( chal_work ), challenge_sequence, challenge_key ) != NETCODE_OK );

        // Non-empty AD rejection (challenge token uses empty AD)
        uint8_t non_empty_ad[1] = { 0x01 };
        uint8_t chal_nonce[12];
        memset( chal_nonce, 0, 4 );
        uint8_t * np = chal_nonce + 4;
        netcode_write_uint64( &np, challenge_sequence );
        unsigned long long dec_len = 0;
        memcpy( chal_work, kat_challenge_token, sizeof( chal_work ) );
        check( crypto_aead_chacha20poly1305_ietf_decrypt( chal_work, &dec_len, NULL, chal_work, NETCODE_CHALLENGE_TOKEN_BYTES, non_empty_ad, sizeof( non_empty_ad ), chal_nonce, challenge_key ) != 0 );
    }

    // 4. Encrypted connection denied and connection challenge packets with server_nonce header + 30-byte AAD
    {
        uint8_t client_salt[NETCODE_CONNECT_SALT_BYTES];
        uint8_t server_nonce[NETCODE_SERVER_NONCE_BYTES];
        uint8_t derived_s2c_key[NETCODE_KEY_BYTES];
        uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];

        test_kat_fill_pattern( client_salt, sizeof( client_salt ), 19, 21 );
        test_kat_fill_pattern( server_nonce, sizeof( server_nonce ), 23, 29 );
        netcode_derive_packet_key( derived_s2c_key, s2c_key, client_salt, server_nonce );
        memset( allowed_packets, 1, sizeof( allowed_packets ) );

        struct netcode_connection_denied_packet_t in_denied;
        memset( &in_denied, 0, sizeof( in_denied ) );
        in_denied.packet_type = NETCODE_CONNECTION_DENIED_PACKET;
        memcpy( in_denied.server_nonce, server_nonce, NETCODE_SERVER_NONCE_BYTES );

        uint8_t denied_buf[NETCODE_MAX_PACKET_BYTES];
        int denied_bytes = netcode_write_packet( &in_denied, denied_buf, sizeof( denied_buf ), ( 1ULL << 63 ), derived_s2c_key, protocol_id );
        check( denied_bytes == (int) sizeof( kat_packet_denied ) );
        check( memcmp( denied_buf, kat_packet_denied, sizeof( kat_packet_denied ) ) == 0 );

        uint8_t denied_work[sizeof( kat_packet_denied )];
        memcpy( denied_work, kat_packet_denied, sizeof( kat_packet_denied ) );
        uint64_t read_sequence = 0;
        struct netcode_connection_denied_packet_t * out_denied = (struct netcode_connection_denied_packet_t*)
            netcode_read_packet( denied_work, sizeof( denied_work ), &read_sequence, derived_s2c_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL );
        check( out_denied != NULL );
        check( read_sequence == ( 1ULL << 63 ) );
        check( out_denied->packet_type == NETCODE_CONNECTION_DENIED_PACKET );
        check( memcmp( out_denied->server_nonce, server_nonce, NETCODE_SERVER_NONCE_BYTES ) == 0 );
        free( out_denied );

        // Cleartext server_nonce header flip is rejected by 30-byte AAD
        memcpy( denied_work, kat_packet_denied, sizeof( kat_packet_denied ) );
        denied_work[9] ^= 0x01;
        check( netcode_read_packet( denied_work, sizeof( denied_work ), &read_sequence, derived_s2c_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL ) == NULL );

        struct netcode_connection_challenge_packet_t in_cpkt;
        memset( &in_cpkt, 0, sizeof( in_cpkt ) );
        in_cpkt.packet_type = NETCODE_CONNECTION_CHALLENGE_PACKET;
        memcpy( in_cpkt.server_nonce, server_nonce, NETCODE_SERVER_NONCE_BYTES );
        in_cpkt.challenge_token_sequence = challenge_sequence;
        memcpy( in_cpkt.challenge_token_data, kat_challenge_token, NETCODE_CHALLENGE_TOKEN_BYTES );

        uint8_t cpkt_buf[NETCODE_MAX_PACKET_BYTES];
        int cpkt_bytes = netcode_write_packet( &in_cpkt, cpkt_buf, sizeof( cpkt_buf ), ( 1ULL << 63 ) + 1, derived_s2c_key, protocol_id );
        check( cpkt_bytes == (int) sizeof( kat_packet_challenge ) );
        check( memcmp( cpkt_buf, kat_packet_challenge, sizeof( kat_packet_challenge ) ) == 0 );

        uint8_t cpkt_work[sizeof( kat_packet_challenge )];
        memcpy( cpkt_work, kat_packet_challenge, sizeof( kat_packet_challenge ) );
        struct netcode_connection_challenge_packet_t * out_cpkt = (struct netcode_connection_challenge_packet_t*)
            netcode_read_packet( cpkt_work, sizeof( cpkt_work ), &read_sequence, derived_s2c_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL );
        check( out_cpkt != NULL );
        check( read_sequence == ( 1ULL << 63 ) + 1 );
        check( out_cpkt->packet_type == NETCODE_CONNECTION_CHALLENGE_PACKET );
        check( memcmp( out_cpkt->server_nonce, server_nonce, NETCODE_SERVER_NONCE_BYTES ) == 0 );
        check( out_cpkt->challenge_token_sequence == challenge_sequence );
        check( memcmp( out_cpkt->challenge_token_data, kat_challenge_token, NETCODE_CHALLENGE_TOKEN_BYTES ) == 0 );
        free( out_cpkt );

        // Cleartext server_nonce header flip is rejected by 30-byte AAD
        memcpy( cpkt_work, kat_packet_challenge, sizeof( kat_packet_challenge ) );
        cpkt_work[9] ^= 0x01;
        check( netcode_read_packet( cpkt_work, sizeof( cpkt_work ), &read_sequence, derived_s2c_key, protocol_id, 0, 0, NULL, allowed_packets, NULL, NULL, NULL ) == NULL );
    }
}


static void test_queue()
{
    struct netcode_packet_queue_t queue;

    netcode_packet_queue_init( &queue, NULL, NULL, NULL );

    check( queue.num_packets == 0 );
    check( queue.start_index == 0 );

    // attempting to pop a packet off an empty queue should return NULL

    check( netcode_packet_queue_pop( &queue, NULL ) == NULL );

    // add some packets to the queue and make sure they pop off in the correct order
    {
        #define NUM_PACKETS 100

        void * packets[NUM_PACKETS];

        int i;
        for ( i = 0; i < NUM_PACKETS; i++ )
        {
            packets[i] = malloc( (i+1) * 256 );
            check( netcode_packet_queue_push( &queue, packets[i], (uint64_t) i ) == 1 );
        }

        check( queue.num_packets == NUM_PACKETS );

        for ( i = 0; i < NUM_PACKETS; i++ )
        {
            uint64_t sequence = 0;
            void * packet = netcode_packet_queue_pop( &queue, &sequence );
            check( sequence == (uint64_t) i ) ;
            check( packet == packets[i] );
            free( packet );
        }
    }

    // after all entries are popped off, the queue is empty, so calls to pop should return NULL

    check( queue.num_packets == 0 );

    check( netcode_packet_queue_pop( &queue, NULL ) == NULL );

    // test that the packet queue can be filled to max capacity

    void * packets[NETCODE_PACKET_QUEUE_SIZE];

    int i;
    for ( i = 0; i < NETCODE_PACKET_QUEUE_SIZE; i++ )
    {
        packets[i] = malloc( i * 256 );
        check( netcode_packet_queue_push( &queue, packets[i], (uint64_t) i ) == 1 );
    }

    check( queue.num_packets == NETCODE_PACKET_QUEUE_SIZE );

    // when the queue is full, attempting to push a packet should fail and return 0

    check( netcode_packet_queue_push( &queue, malloc( 100 ), 0 ) == 0 );

    // make sure all packets pop off in the correct order

    for ( i = 0; i < NETCODE_PACKET_QUEUE_SIZE; i++ )
    {
        uint64_t sequence = 0;
        void * packet = netcode_packet_queue_pop( &queue, &sequence );
        check( sequence == (uint64_t) i );
        check( packet == packets[i] );
        free( packet );
    }

    // add some packets again

    for ( i = 0; i < NETCODE_PACKET_QUEUE_SIZE; i++ )
    {
        packets[i] = malloc( i * 256 );
        check( netcode_packet_queue_push( &queue, packets[i], (uint64_t) i ) == 1 );
    }

    // clear the queue and make sure that all packets are freed

    netcode_packet_queue_clear( &queue );

    check( queue.start_index == 0 );
    check( queue.num_packets == 0 );
    for ( i = 0; i < NETCODE_PACKET_QUEUE_SIZE; i++ )
        check( queue.packet_data[i] == NULL );
}

static void test_endian()
{
    uint32_t value = 0x11223344;

    char * bytes = (char*) &value;

#if NETCODE_LITTLE_ENDIAN

    check( bytes[0] == 0x44 );
    check( bytes[1] == 0x33 );
    check( bytes[2] == 0x22 );
    check( bytes[3] == 0x11 );

#else // #if NETCODE_LITTLE_ENDIAN

    check( bytes[3] == 0x44 );
    check( bytes[2] == 0x33 );
    check( bytes[1] == 0x22 );
    check( bytes[0] == 0x11 );

#endif // #if NETCODE_LITTLE_ENDIAN
}

static void test_sequence()
{
    check( netcode_sequence_number_bytes_required( 0 ) == 1 );
    check( netcode_sequence_number_bytes_required( 0x11 ) == 1 );
    check( netcode_sequence_number_bytes_required( 0x1122 ) == 2 );
    check( netcode_sequence_number_bytes_required( 0x112233 ) == 3 );
    check( netcode_sequence_number_bytes_required( 0x11223344 ) == 4 );
    check( netcode_sequence_number_bytes_required( 0x1122334455 ) == 5 );
    check( netcode_sequence_number_bytes_required( 0x112233445566 ) == 6 );
    check( netcode_sequence_number_bytes_required( 0x11223344556677 ) == 7 );
    check( netcode_sequence_number_bytes_required( 0x1122334455667788 ) == 8 );
}

struct test_socket_buffer_os_t
{
    int limit;          // the largest size this fake OS accepts (a BSD rejects anything above it)
    int calls;
    int smallest_tried;
};

static int test_socket_buffer_os_try( void * context, int size )
{
    struct test_socket_buffer_os_t * os = (struct test_socket_buffer_os_t*) context;
    os->calls++;
    if ( os->smallest_tried == 0 || size < os->smallest_tried )
        os->smallest_tried = size;
    return size <= os->limit;
}

static void test_socket_handle_signed()
{
#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    check( sizeof( netcode_socket_handle_t ) >= sizeof( void * ) );
#else // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    netcode_socket_handle_t handle = (netcode_socket_handle_t) -1;
    check( handle < 0 );
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
}

static void test_socket_buffer_size_back_off()
{
    const int requested = 4 * 1024 * 1024;

    // an OS that accepts the request (or clamps it, as linux and windows do) takes it on the first try

    {
        struct test_socket_buffer_os_t os = { requested, 0, 0 };
        check( netcode_socket_buffer_size_back_off( requested, test_socket_buffer_os_try, &os ) == requested );
        check( os.calls == 1 );
    }

    // a stock NetBSD (kern.sbmax 256 KiB, about 227 KiB usable) refuses 4M, 2M, 1M, 512K and 256K, and takes 128K

    {
        struct test_socket_buffer_os_t os = { 227 * 1024, 0, 0 };
        check( netcode_socket_buffer_size_back_off( requested, test_socket_buffer_os_try, &os ) == 128 * 1024 );
        check( os.calls == 6 );
    }

    // an OS that refuses every request down to the minimum keeps its default: not an error, and never a request below the minimum

    {
        struct test_socket_buffer_os_t os = { 16 * 1024, 0, 0 };
        check( netcode_socket_buffer_size_back_off( requested, test_socket_buffer_os_try, &os ) == 0 );
        check( os.smallest_tried == NETCODE_SOCKET_BUFFER_SIZE_MINIMUM );
        check( os.calls == 7 );
    }

    // a request already below the minimum is not made at all

    {
        struct test_socket_buffer_os_t os = { requested, 0, 0 };
        check( netcode_socket_buffer_size_back_off( 32 * 1024, test_socket_buffer_os_try, &os ) == 0 );
        check( os.calls == 0 );
    }
}

static void test_socket_handle()
{
    // a fresh socket holds no handle, and destroying it touches nothing

    struct netcode_socket_t socket;
    netcode_socket_init( &socket );
    check( !netcode_socket_handle_valid( socket.handle ) );
    netcode_socket_destroy( &socket );
    check( !netcode_socket_handle_valid( socket.handle ) );

    // descriptor 0 is a valid socket handle on posix

#if NETCODE_PLATFORM != NETCODE_PLATFORM_WINDOWS
    check( netcode_socket_handle_valid( 0 ) );
#endif // #if NETCODE_PLATFORM != NETCODE_PLATFORM_WINDOWS
}

static void test_address()
{
    {
        struct netcode_address_t address;
        check( netcode_parse_address( "", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[]", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[]:", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( ":", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "12", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "123", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1234", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1234.0.12313.0000", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1234.0.12313.0000.0.0.0.0.0", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1312313:123131:1312313:123131:1312313:123131:1312313:123131:1312313:123131:1312313:123131", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( ".", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "..", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "...", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "....", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( ".....", &address ) == NETCODE_ERROR );
    }

    // ports must be all digits in [0,65535]. out of range and non-numeric ports must not silently truncate

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "127.0.0.1:65535", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 65535 );
        check( netcode_parse_address( "[::1]:65535", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 65535 );
        check( netcode_parse_address( "127.0.0.1:65536", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "127.0.0.1:99999", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "127.0.0.1:", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "127.0.0.1:40k", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[::1]:65536", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[::1]:", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[::1]:40k", &address ) == NETCODE_ERROR );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "107.77.207.77", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 0 );
        check( address.data.ipv4[0] == 107 );
        check( address.data.ipv4[1] == 77 );
        check( address.data.ipv4[2] == 207 );
        check( address.data.ipv4[3] == 77 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "127.0.0.1", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 0 );
        check( address.data.ipv4[0] == 127 );
        check( address.data.ipv4[1] == 0 );
        check( address.data.ipv4[2] == 0 );
        check( address.data.ipv4[3] == 1 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "107.77.207.77:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 40000 );
        check( address.data.ipv4[0] == 107 );
        check( address.data.ipv4[1] == 77 );
        check( address.data.ipv4[2] == 207 );
        check( address.data.ipv4[3] == 77 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "127.0.0.1:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 40000 );
        check( address.data.ipv4[0] == 127 );
        check( address.data.ipv4[1] == 0 );
        check( address.data.ipv4[2] == 0 );
        check( address.data.ipv4[3] == 1 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "fe80::202:b3ff:fe1e:8329", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0xfe80 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0202 );
        check( address.data.ipv6[5] == 0xb3ff );
        check( address.data.ipv6[6] == 0xfe1e );
        check( address.data.ipv6[7] == 0x8329 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "::", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0000 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "::1", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0001 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "::0", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0000 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[::1]", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0001 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[::0]", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0000 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[fe80::1]", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0xfe80 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0001 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[fe80::202:b3ff:fe1e:8329]:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 40000 );
        check( address.data.ipv6[0] == 0xfe80 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0202 );
        check( address.data.ipv6[5] == 0xb3ff );
        check( address.data.ipv6[6] == 0xfe1e );
        check( address.data.ipv6[7] == 0x8329 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[::]:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 40000 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0000 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[::1]:5", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 5 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0001 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[fe80::1]:5", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 5 );
        check( address.data.ipv6[0] == 0xfe80 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0001 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[::1]:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 40000 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0001 );
    }
}

#define TEST_PROTOCOL_ID            0x1122334455667788ULL
#define TEST_CLIENT_ID              0x1ULL
#define TEST_SERVER_PORT            40000
// the lifetime the test backend issues, kept within the server's default configured maximum
#define TEST_CONNECT_TOKEN_EXPIRY   NETCODE_DEFAULT_MAX_CONNECT_TOKEN_LIFETIME
#define TEST_TIMEOUT_SECONDS        15

// start a server for a test, then move time on to the next second. the server refuses connect
// tokens issued in the second it started, so a test issues its tokens one second after its
// server starts. the unit tests move their fake clock on; the functional tests run on the wall
// clock and wait for it.

static void test_server_start( struct netcode_server_t * server, int max_clients )
{
    netcode_server_start( server, max_clients );

    if ( netcode_test_fake_time_enabled )
    {
        netcode_test_fake_time_value++;
    }
    else
    {
        uint64_t start_second = netcode_current_timestamp();
        while ( netcode_current_timestamp() == start_second )
        {
            netcode_sleep( 0.01 );
        }
    }
}

static int test_oor_asserts_fired = 0;

static void test_oor_assert_handler( NETCODE_CONST char * condition, NETCODE_CONST char * function, NETCODE_CONST char * file, int line )
{
    (void) condition; (void) function; (void) file; (void) line;
    test_oor_asserts_fired++;
    // deliberately RETURNS. netcode.h documents that a custom handler may do this and
    // execution continues past the failed assert -- which is the only way to reach the
    // release-build code path from a test binary that has asserts compiled in.
}

static void test_connect_token_expire_timestamp()
{
    uint8_t private_key[NETCODE_KEY_BYTES];
    netcode_generate_key( private_key );

    NETCODE_CONST char * server_address = "127.0.0.1:40000";

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    const int expire_seconds[] = { -1, -30 };

    int i;
    for ( i = 0; i < (int) ( sizeof( expire_seconds ) / sizeof( expire_seconds[0] ) ); i++ )
    {
        uint8_t connect_token_data[NETCODE_CONNECT_TOKEN_BYTES];
        check( netcode_generate_connect_token( 1, &server_address, &server_address, expire_seconds[i], TEST_TIMEOUT_SECONDS, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token_data ) == NETCODE_OK );

        struct netcode_connect_token_t connect_token;
        check( netcode_read_connect_token( connect_token_data, NETCODE_CONNECT_TOKEN_BYTES, &connect_token ) == NETCODE_OK );
        check( connect_token.expire_timestamp == UINT64_MAX );
    }
}

static void test_generate_connect_token_out_of_range()
{
    // netcode_generate_connect_token parses into arrays sized NETCODE_MAX_SERVERS_PER_CONNECT.
    // The bounds used to be assert-only, so -DNDEBUG release builds -- which is what ships,
    // and what Debian packages -- wrote past a stack array. Every other public entry point
    // already paired its asserts with a runtime check; this one was missed.
    //
    // The asserts fire first by design, so this installs a handler that returns in order to
    // reach the runtime check underneath them. Without the fix, execution continues into the
    // parse loop and writes out of bounds instead of returning NETCODE_ERROR.

    uint8_t private_key[NETCODE_KEY_BYTES];
    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    memset( private_key, 0, sizeof( private_key ) );
    memset( user_data, 0, sizeof( user_data ) );

    NETCODE_CONST char * server_address = "127.0.0.1:40000";

    test_oor_asserts_fired = 0;
    netcode_set_assert_function( &test_oor_assert_handler );

    check( netcode_generate_connect_token( 0, &server_address, &server_address, 30, 5, 1000ULL, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) == NETCODE_ERROR );
    check( netcode_generate_connect_token( -1, &server_address, &server_address, 30, 5, 1000ULL, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) == NETCODE_ERROR );
    check( netcode_generate_connect_token( NETCODE_MAX_SERVERS_PER_CONNECT + 1, &server_address, &server_address, 30, 5, 1000ULL, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) == NETCODE_ERROR );

    // In a DEBUG build the asserts must still have fired -- they are the caller's debug aid
    // and this test must not silently prove they were removed. In a RELEASE build they are
    // compiled to ((void)0) by design, so requiring them there would fail the very
    // configuration this fix exists for. That asymmetry IS the point of the fix.

#ifndef NDEBUG
    check( test_oor_asserts_fired > 0 );
#endif // #ifndef NDEBUG

    // restore the DEFAULT handler, not NULL: netcode_assert calls the pointer with no null
    // guard, so NULL would turn the next failing assert anywhere in the suite into a crash

    netcode_set_assert_function( &netcode_default_assert_handler );

    // and an in-range call still succeeds, so the guard cannot pass by rejecting everything

    check( netcode_generate_connect_token( 1, &server_address, &server_address, 30, 5, 1000ULL, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) == NETCODE_OK );
}

static void test_connect_token()
{
    // generate a connect token

    struct netcode_address_t server_address;
    server_address.type = NETCODE_ADDRESS_IPV4;
    server_address.data.ipv4[0] = 127;
    server_address.data.ipv4[1] = 0;
    server_address.data.ipv4[2] = 0;
    server_address.data.ipv4[3] = 1;
    server_address.port = TEST_SERVER_PORT;

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    struct netcode_connect_token_private_t input_token;

    netcode_generate_connect_token_private( &input_token, TEST_CLIENT_ID, TEST_TIMEOUT_SECONDS, 1, &server_address, user_data );

    check( input_token.client_id == TEST_CLIENT_ID );
    check( input_token.num_server_addresses == 1 );
    check( memcmp( input_token.user_data, user_data, NETCODE_USER_DATA_BYTES ) == 0 );
    check( netcode_address_equal( &input_token.server_addresses[0], &server_address ) );

    // write it to a buffer

    uint8_t buffer[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];

    netcode_write_connect_token_private( &input_token, buffer, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // encrypt the buffer

    uint64_t expire_timestamp = netcode_current_timestamp() + 30;
    uint8_t nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    netcode_generate_nonce(nonce);
    uint8_t key[NETCODE_KEY_BYTES];
    netcode_generate_key( key );    

    check( netcode_encrypt_connect_token_private( buffer, 
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                  NETCODE_VERSION_INFO, 
                                                  TEST_PROTOCOL_ID, 
                                                  expire_timestamp, 
                                                  nonce, 
                                                  key ) == NETCODE_OK );

    // decrypt the buffer

    check( netcode_decrypt_connect_token_private( buffer, 
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                  NETCODE_VERSION_INFO, 
                                                  TEST_PROTOCOL_ID, 
                                                  expire_timestamp, 
                                                  nonce, 
                                                  key ) == NETCODE_OK );

    // read the connect token back in

    struct netcode_connect_token_private_t output_token;

    check( netcode_read_connect_token_private( buffer, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, &output_token ) == NETCODE_OK );

    // make sure that everything matches the original connect token

    check( output_token.client_id == input_token.client_id );
    check( output_token.timeout_seconds == input_token.timeout_seconds );
    check( output_token.num_server_addresses == input_token.num_server_addresses );
    check( netcode_address_equal( &output_token.server_addresses[0], &input_token.server_addresses[0] ) );
    check( memcmp( output_token.client_to_server_key, input_token.client_to_server_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( output_token.server_to_client_key, input_token.server_to_client_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( output_token.user_data, input_token.user_data, NETCODE_USER_DATA_BYTES ) == 0 );
}

static void test_challenge_token()
{
    // additional data is NULL, 0. CMakeLists.txt keeps sodium's UBSan exemption as alignment only, so this path guards #186.

    struct netcode_challenge_token_t input_token;

    input_token.client_id = TEST_CLIENT_ID;
    netcode_random_bytes( input_token.user_data, NETCODE_USER_DATA_BYTES );

    // write it to a buffer

    uint8_t buffer[NETCODE_CHALLENGE_TOKEN_BYTES];

    netcode_write_challenge_token( &input_token, buffer, NETCODE_CHALLENGE_TOKEN_BYTES );

    // encrypt the buffer

    uint64_t sequence = 1000;
    uint8_t key[NETCODE_KEY_BYTES]; 
    netcode_generate_key( key );    

    check( netcode_encrypt_challenge_token( buffer, NETCODE_CHALLENGE_TOKEN_BYTES, sequence, key ) == NETCODE_OK );

    // decrypt the buffer

    check( netcode_decrypt_challenge_token( buffer, NETCODE_CHALLENGE_TOKEN_BYTES, sequence, key ) == NETCODE_OK );

    // read the challenge token back in

    struct netcode_challenge_token_t output_token;

    check( netcode_read_challenge_token( buffer, NETCODE_CHALLENGE_TOKEN_BYTES, &output_token ) == NETCODE_OK );

    // make sure that everything matches the original challenge token

    check( output_token.client_id == input_token.client_id );
    check( memcmp( output_token.user_data, input_token.user_data, NETCODE_USER_DATA_BYTES ) == 0 );
}

static void test_connection_request_packet()
{
    // generate a connect token

    struct netcode_address_t server_address;
    server_address.type = NETCODE_ADDRESS_IPV4;
    server_address.data.ipv4[0] = 127;
    server_address.data.ipv4[1] = 0;
    server_address.data.ipv4[2] = 0;
    server_address.data.ipv4[3] = 1;
    server_address.port = TEST_SERVER_PORT;

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    struct netcode_connect_token_private_t input_token;

    netcode_generate_connect_token_private( &input_token, TEST_CLIENT_ID, TEST_TIMEOUT_SECONDS, 1, &server_address, user_data );

    check( input_token.client_id == TEST_CLIENT_ID );
    check( input_token.num_server_addresses == 1 );
    check( memcmp( input_token.user_data, user_data, NETCODE_USER_DATA_BYTES ) == 0 );
    check( netcode_address_equal( &input_token.server_addresses[0], &server_address ) );

    // write the conect token to a buffer (non-encrypted)

    uint8_t connect_token_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];

    netcode_write_connect_token_private( &input_token, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // copy to a second buffer then encrypt it in place (we need the unencrypted token for verification later on)

    uint8_t encrypted_connect_token_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];

    memcpy( encrypted_connect_token_data, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    uint64_t connect_token_expire_timestamp = netcode_current_timestamp() + 30;
    uint8_t connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    netcode_generate_nonce(connect_token_nonce);
    uint8_t connect_token_key[NETCODE_KEY_BYTES];
    netcode_generate_key( connect_token_key );

    check( netcode_encrypt_connect_token_private( encrypted_connect_token_data, 
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                  NETCODE_VERSION_INFO, 
                                                  TEST_PROTOCOL_ID, 
                                                  connect_token_expire_timestamp, 
                                                  connect_token_nonce, 
                                                  connect_token_key ) == NETCODE_OK );

    // setup a connection request packet wrapping the encrypted connect token

    struct netcode_connection_request_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_REQUEST_PACKET;
    memcpy( input_packet.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
    input_packet.protocol_id = TEST_PROTOCOL_ID;
    input_packet.connect_token_expire_timestamp = connect_token_expire_timestamp;
    memcpy( input_packet.connect_token_nonce, connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
    memcpy( input_packet.connect_token_data, encrypted_connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    netcode_random_bytes( input_packet.client_salt, NETCODE_CONNECT_SALT_BYTES );

    // write the connection request packet to a buffer

    uint8_t buffer[2048];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written == 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES + NETCODE_CONNECT_SALT_BYTES );

    // read the connection request packet back in from the buffer (the connect token data is decrypted as part of the read packet validation)

    uint64_t sequence = 1000;

    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 1, sizeof( allowed_packets ) );

    uint8_t read_buffer[2048];
    memcpy( read_buffer, buffer, (size_t) bytes_written );

    struct netcode_connection_request_packet_t * output_packet = (struct netcode_connection_request_packet_t*) 
        netcode_read_packet( read_buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, connect_token_key, allowed_packets, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_REQUEST_PACKET );
    check( memcmp( output_packet->version_info, input_packet.version_info, NETCODE_VERSION_INFO_BYTES ) == 0 );
    check( output_packet->protocol_id == input_packet.protocol_id );
    check( output_packet->connect_token_expire_timestamp == input_packet.connect_token_expire_timestamp );
    check( memcmp( output_packet->connect_token_nonce, input_packet.connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES ) == 0 );
    check( memcmp( output_packet->connect_token_data, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES ) == 0 );
    check( memcmp( output_packet->client_salt, input_packet.client_salt, NETCODE_CONNECT_SALT_BYTES ) == 0 );

    free( output_packet );

    // a 1078-byte NETCODE 1.02 request or wrong version string must be rejected

    memcpy( read_buffer, buffer, (size_t) bytes_written );
    check( netcode_read_packet( read_buffer, bytes_written - NETCODE_CONNECT_SALT_BYTES, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, connect_token_key, allowed_packets, NULL, NULL, NULL ) == NULL );

    memcpy( read_buffer, buffer, (size_t) bytes_written );
    read_buffer[1 + 10] = '2';
    check( netcode_read_packet( read_buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, connect_token_key, allowed_packets, NULL, NULL, NULL ) == NULL );
}

void test_connection_denied_packet()
{
    // setup a connection denied packet

    struct netcode_connection_denied_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_DENIED_PACKET;
    netcode_random_bytes( input_packet.server_nonce, NETCODE_SERVER_NONCE_BYTES );

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written == 1 + 2 + NETCODE_SERVER_NONCE_BYTES + NETCODE_MAC_BYTES );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    uint8_t read_buffer[NETCODE_MAX_PACKET_BYTES];
    memcpy( read_buffer, buffer, (size_t) bytes_written );

    struct netcode_connection_denied_packet_t * output_packet = (struct netcode_connection_denied_packet_t*) 
        netcode_read_packet( read_buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_DENIED_PACKET );
    check( memcmp( output_packet->server_nonce, input_packet.server_nonce, NETCODE_SERVER_NONCE_BYTES ) == 0 );

    free( output_packet );

    // tampering with server_nonce or truncating below header_nonce_bytes + MAC is rejected

    memcpy( read_buffer, buffer, (size_t) bytes_written );
    read_buffer[1 + ( read_buffer[0] >> 4 )] ^= 0x01;
    check( netcode_read_packet( read_buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL ) == NULL );

    memcpy( read_buffer, buffer, (size_t) bytes_written );
    check( netcode_read_packet( read_buffer, bytes_written - 1, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL ) == NULL );
}

void test_connection_challenge_packet()
{
    // setup a connection challenge packet

    struct netcode_connection_challenge_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_CHALLENGE_PACKET;
    netcode_random_bytes( input_packet.server_nonce, NETCODE_SERVER_NONCE_BYTES );
    input_packet.challenge_token_sequence = 0;
    netcode_random_bytes( input_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written == 1 + 2 + NETCODE_SERVER_NONCE_BYTES + 8 + NETCODE_CHALLENGE_TOKEN_BYTES + NETCODE_MAC_BYTES );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    uint8_t read_buffer[NETCODE_MAX_PACKET_BYTES];
    memcpy( read_buffer, buffer, (size_t) bytes_written );

    struct netcode_connection_challenge_packet_t * output_packet = (struct netcode_connection_challenge_packet_t*) 
        netcode_read_packet( read_buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_CHALLENGE_PACKET );
    check( memcmp( output_packet->server_nonce, input_packet.server_nonce, NETCODE_SERVER_NONCE_BYTES ) == 0 );
    check( output_packet->challenge_token_sequence == input_packet.challenge_token_sequence );
    check( memcmp( output_packet->challenge_token_data, input_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES ) == 0 );

    free( output_packet );

    // tampering with server_nonce in the cleartext header is rejected by AEAD

    memcpy( read_buffer, buffer, (size_t) bytes_written );
    read_buffer[1 + ( read_buffer[0] >> 4 )] ^= 0x01;
    check( netcode_read_packet( read_buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL ) == NULL );
}

void test_connection_response_packet()
{
    // setup a connection response packet

    struct netcode_connection_response_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_RESPONSE_PACKET;
    input_packet.challenge_token_sequence = 0;
    netcode_random_bytes( input_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );
    
    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_response_packet_t * output_packet = (struct netcode_connection_response_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_RESPONSE_PACKET );
    check( output_packet->challenge_token_sequence == input_packet.challenge_token_sequence );
    check( memcmp( output_packet->challenge_token_data, input_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES ) == 0 );

    free( output_packet );
}

void test_connection_keep_alive_packet()
{
    // setup a connection keep alive packet

    struct netcode_connection_keep_alive_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
    input_packet.client_index = 10;
    input_packet.max_clients = 16;

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );
    
    struct netcode_connection_keep_alive_packet_t * output_packet = (struct netcode_connection_keep_alive_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_KEEP_ALIVE_PACKET );
    check( output_packet->client_index == input_packet.client_index );
    check( output_packet->max_clients == input_packet.max_clients );

    free( output_packet );
}

void test_connection_payload_packet()
{
    // setup a connection payload packet

    struct netcode_connection_payload_packet_t * input_packet = netcode_create_payload_packet( NETCODE_MAX_PAYLOAD_BYTES, NULL, NULL );

    check( input_packet->packet_type == NETCODE_CONNECTION_PAYLOAD_PACKET );
    check( input_packet->payload_bytes == NETCODE_MAX_PAYLOAD_BYTES );

    netcode_random_bytes( input_packet->payload_data, NETCODE_MAX_PAYLOAD_BYTES );
    
    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_payload_packet_t * output_packet = (struct netcode_connection_payload_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_PAYLOAD_PACKET );
    check( output_packet->payload_bytes == input_packet->payload_bytes );
    check( memcmp( output_packet->payload_data, input_packet->payload_data, NETCODE_MAX_PAYLOAD_BYTES ) == 0 );

    free( input_packet );
    free( output_packet );
}

void test_connection_disconnect_packet()
{
    // setup a connection disconnect packet

    struct netcode_connection_disconnect_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_disconnect_packet_t * output_packet = (struct netcode_connection_disconnect_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, netcode_current_timestamp(), 0, NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_DISCONNECT_PACKET );

    free( output_packet );
}

void test_connect_token_public()
{
    // generate a private connect token

    struct netcode_address_t server_address;
    server_address.type = NETCODE_ADDRESS_IPV4;
    server_address.data.ipv4[0] = 127;
    server_address.data.ipv4[1] = 0;
    server_address.data.ipv4[2] = 0;
    server_address.data.ipv4[3] = 1;
    server_address.port = TEST_SERVER_PORT;

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    struct netcode_connect_token_private_t connect_token_private;

    netcode_generate_connect_token_private( &connect_token_private, TEST_CLIENT_ID, TEST_TIMEOUT_SECONDS, 1, &server_address, user_data );

    check( connect_token_private.client_id == TEST_CLIENT_ID );
    check( connect_token_private.num_server_addresses == 1 );
    check( memcmp( connect_token_private.user_data, user_data, NETCODE_USER_DATA_BYTES ) == 0 );
    check( netcode_address_equal( &connect_token_private.server_addresses[0], &server_address ) );

    // write it to a buffer

    uint8_t connect_token_private_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    netcode_write_connect_token_private( &connect_token_private, connect_token_private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // encrypt the buffer

    uint64_t create_timestamp = netcode_current_timestamp();
    uint64_t expire_timestamp = create_timestamp + 30;
    uint8_t connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    netcode_generate_nonce( connect_token_nonce );    
    uint8_t key[NETCODE_KEY_BYTES];
    netcode_generate_key( key );    
    check( netcode_encrypt_connect_token_private( connect_token_private_data, 
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                  NETCODE_VERSION_INFO, 
                                                  TEST_PROTOCOL_ID, 
                                                  expire_timestamp, 
                                                  connect_token_nonce, 
                                                  key ) == 1 );

    // wrap a public connect token around the private connect token data

    struct netcode_connect_token_t input_connect_token;
    memset( &input_connect_token, 0, sizeof( struct netcode_connect_token_t ) );
    memcpy( input_connect_token.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
    input_connect_token.protocol_id = TEST_PROTOCOL_ID;
    input_connect_token.create_timestamp = create_timestamp;
    input_connect_token.expire_timestamp = expire_timestamp;
    memcpy( input_connect_token.nonce, connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
    memcpy( input_connect_token.private_data, connect_token_private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    input_connect_token.num_server_addresses = 1;
    input_connect_token.server_addresses[0] = server_address;
    memcpy( input_connect_token.client_to_server_key, connect_token_private.client_to_server_key, NETCODE_KEY_BYTES );
    memcpy( input_connect_token.server_to_client_key, connect_token_private.server_to_client_key, NETCODE_KEY_BYTES );
    input_connect_token.timeout_seconds = (int) TEST_TIMEOUT_SECONDS;

    // write the connect token to a buffer

    uint8_t buffer[NETCODE_CONNECT_TOKEN_BYTES];
    netcode_write_connect_token( &input_connect_token, buffer, NETCODE_CONNECT_TOKEN_BYTES );

    // read the buffer back in

    struct netcode_connect_token_t output_connect_token;
    memset( &output_connect_token, 0, sizeof( struct netcode_connect_token_t ) );
    check( netcode_read_connect_token( buffer, NETCODE_CONNECT_TOKEN_BYTES, &output_connect_token ) == 1 );

    // make sure the public connect token matches what was written

    check( memcmp( output_connect_token.version_info, input_connect_token.version_info, NETCODE_VERSION_INFO_BYTES ) == 0 );
    check( output_connect_token.protocol_id == input_connect_token.protocol_id );
    check( output_connect_token.create_timestamp == input_connect_token.create_timestamp );
    check( output_connect_token.expire_timestamp == input_connect_token.expire_timestamp );
    check( memcmp( output_connect_token.nonce, input_connect_token.nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES ) == 0 );
    check( memcmp( output_connect_token.private_data, input_connect_token.private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES ) == 0 );
    check( output_connect_token.num_server_addresses == input_connect_token.num_server_addresses );
    check( netcode_address_equal( &output_connect_token.server_addresses[0], &input_connect_token.server_addresses[0] ) );
    check( memcmp( output_connect_token.client_to_server_key, input_connect_token.client_to_server_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( output_connect_token.server_to_client_key, input_connect_token.server_to_client_key, NETCODE_KEY_BYTES ) == 0 );
    check( output_connect_token.timeout_seconds == input_connect_token.timeout_seconds );
}

void test_encryption_manager()
{
    struct netcode_encryption_manager_t encryption_manager;

    netcode_encryption_manager_reset( &encryption_manager );

    double time = 100.0;

    // generate some test encryption mappings

    struct encryption_mapping_t
    {
        struct netcode_address_t address;
        uint8_t send_key[NETCODE_KEY_BYTES];
        uint8_t receive_key[NETCODE_KEY_BYTES];
    };

    #define NUM_ENCRYPTION_MAPPINGS 5

    struct encryption_mapping_t encryption_mapping[NUM_ENCRYPTION_MAPPINGS];
    memset( encryption_mapping, 0, sizeof( encryption_mapping ) );
    int i;
    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; i++ )
    {
        encryption_mapping[i].address.type = NETCODE_ADDRESS_IPV6;
        encryption_mapping[i].address.data.ipv6[7] = 1;
        encryption_mapping[i].address.port = ( uint16_t) ( 20000 + i );
        netcode_generate_key( encryption_mapping[i].send_key );
        netcode_generate_key( encryption_mapping[i].receive_key );
    }

    // add the encryption mappings to the manager and make sure they can be looked up by address

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; i++ )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        check( encryption_index == -1 );

        check( netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index ) == NULL );
        check( netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index ) == NULL );

        check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                                  &encryption_mapping[i].address, 
                                                                  encryption_mapping[i].send_key, 
                                                                  encryption_mapping[i].receive_key, 
                                                                  time, 
                                                                  -1.0,
                                                                  TEST_TIMEOUT_SECONDS,
                                                                  -1 ) );

        encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( send_key );
        check( receive_key );

        check( memcmp( send_key, encryption_mapping[i].send_key, NETCODE_KEY_BYTES ) == 0 );
        check( memcmp( receive_key, encryption_mapping[i].receive_key, NETCODE_KEY_BYTES ) == 0 );
    }

    // removing an encryption mapping that doesn't exist should return 0
    {
        struct netcode_address_t address;
        address.type = NETCODE_ADDRESS_IPV6;
        address.data.ipv6[7] = 1;
        address.port = 50000;

        check( netcode_encryption_manager_remove_encryption_mapping( &encryption_manager, &address, time ) == 0 );
    }

    // remove the first and last encryption mappings

    check( netcode_encryption_manager_remove_encryption_mapping( &encryption_manager, &encryption_mapping[0].address, time ) == 1 );

    check( netcode_encryption_manager_remove_encryption_mapping( &encryption_manager, &encryption_mapping[NUM_ENCRYPTION_MAPPINGS-1].address, time ) == 1 );

    // make sure the encryption mappings that were removed can no longer be looked up by address

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; i++ )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        if ( i != 0 && i != NUM_ENCRYPTION_MAPPINGS - 1 )
        {
            check( send_key );
            check( receive_key );

            check( memcmp( send_key, encryption_mapping[i].send_key, NETCODE_KEY_BYTES ) == 0 );
            check( memcmp( receive_key, encryption_mapping[i].receive_key, NETCODE_KEY_BYTES ) == 0 );
        }
        else
        {
            check( !send_key );
            check( !receive_key );
        }
    }

    // add the encryption mappings back in
    
    check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                              &encryption_mapping[0].address, 
                                                              encryption_mapping[0].send_key, 
                                                              encryption_mapping[0].receive_key, 
                                                              time, 
                                                              -1.0,
                                                              TEST_TIMEOUT_SECONDS,
                                                              -1 ) );
    
    check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                              &encryption_mapping[NUM_ENCRYPTION_MAPPINGS-1].address, 
                                                              encryption_mapping[NUM_ENCRYPTION_MAPPINGS-1].send_key, 
                                                              encryption_mapping[NUM_ENCRYPTION_MAPPINGS-1].receive_key, 
                                                              time, 
                                                              -1.0,
                                                              TEST_TIMEOUT_SECONDS,
                                                              -1 ) );

    // all encryption mappings should be able to be looked up by address again

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; i++ )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( send_key );
        check( receive_key );

        check( memcmp( send_key, encryption_mapping[i].send_key, NETCODE_KEY_BYTES ) == 0 );
        check( memcmp( receive_key, encryption_mapping[i].receive_key, NETCODE_KEY_BYTES ) == 0 );
    }

    // check that encryption mappings time out properly

    time += TEST_TIMEOUT_SECONDS * 2;

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; i++ )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( !send_key );
        check( !receive_key );
    }

    // add the same encryption mappings after timeout

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; i++ )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        check( encryption_index == -1 );

        check( netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index ) == NULL );
        check( netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index ) == NULL );

        check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                                  &encryption_mapping[i].address, 
                                                                  encryption_mapping[i].send_key, 
                                                                  encryption_mapping[i].receive_key, 
                                                                  time, 
                                                                  -1.0,
                                                                  TEST_TIMEOUT_SECONDS,
                                                                  -1 ) );

        encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( send_key );
        check( receive_key );

        check( memcmp( send_key, encryption_mapping[i].send_key, NETCODE_KEY_BYTES ) == 0 );
        check( memcmp( receive_key, encryption_mapping[i].receive_key, NETCODE_KEY_BYTES ) == 0 );
    }

    // reset the encryption mapping and verify that all encryption mappings have been removed

    netcode_encryption_manager_reset( &encryption_manager );

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; i++ )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( !send_key );
        check( !receive_key );
    }

    // test the expire time for encryption mapping works as expected

    check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                              &encryption_mapping[0].address, 
                                                              encryption_mapping[0].send_key, 
                                                              encryption_mapping[0].receive_key, 
                                                              time, 
                                                              time + 1.0,
                                                              TEST_TIMEOUT_SECONDS,
                                                              -1 ) );

    int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[0].address, time );

    check( encryption_index != -1 );

    check( netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[0].address, time + 1.1f ) == -1 );

    netcode_encryption_manager_set_expire_time( &encryption_manager, encryption_index, -1.0 );

    check( netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[0].address, time ) == encryption_index );
}

static int test_key_is_zero( const uint8_t * key )
{
    int i;
    for ( i = 0; i < NETCODE_KEY_BYTES; i++ )
    {
        if ( key[i] != 0 )
            return 0;
    }
    return 1;
}

void test_encryption_manager_zeroes_keys()
{
    struct netcode_encryption_manager_t encryption_manager;

    netcode_encryption_manager_reset( &encryption_manager );

    double time = 1000.0;

    // slot 0 is owned by a connected client, slot 1 is free, slot 2 is removed below

    struct netcode_address_t address[3];
    uint8_t send_key[NETCODE_KEY_BYTES];
    uint8_t receive_key[NETCODE_KEY_BYTES];
    memset( address, 0, sizeof( address ) );
    memset( send_key, 0xAB, sizeof( send_key ) );
    memset( receive_key, 0xCD, sizeof( receive_key ) );

    int i;
    for ( i = 0; i < 3; i++ )
    {
        address[i].type = NETCODE_ADDRESS_IPV4;
        address[i].data.ipv4[0] = 127;
        address[i].data.ipv4[3] = 1;
        address[i].port = (uint16_t) ( 30000 + i );
        check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, &address[i], send_key, receive_key, time, -1.0, TEST_TIMEOUT_SECONDS, -1 ) );
    }

    encryption_manager.client_index[0] = 0;

    // after the timeout, removing the last mapping trims the expired free slot behind it

    time += TEST_TIMEOUT_SECONDS + 1.0;

    check( netcode_encryption_manager_remove_encryption_mapping( &encryption_manager, &address[2], time ) == 1 );
    check( encryption_manager.num_encryption_mappings == 1 );

    for ( i = 1; i < 3; i++ )
    {
        check( test_key_is_zero( encryption_manager.send_key + i * NETCODE_KEY_BYTES ) );
        check( test_key_is_zero( encryption_manager.receive_key + i * NETCODE_KEY_BYTES ) );
    }

    // the connected client's keys survive the timeout

    check( memcmp( encryption_manager.send_key, send_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( encryption_manager.receive_key, receive_key, NETCODE_KEY_BYTES ) == 0 );

    // reset zeroes every key

    netcode_encryption_manager_reset( &encryption_manager );

    for ( i = 0; i < NETCODE_MAX_ENCRYPTION_MAPPINGS; i++ )
    {
        check( test_key_is_zero( encryption_manager.send_key + i * NETCODE_KEY_BYTES ) );
        check( test_key_is_zero( encryption_manager.receive_key + i * NETCODE_KEY_BYTES ) );
    }
}

static void test_expired_encryption_mapping_keys_are_zero( double time )
{
    struct netcode_encryption_manager_t encryption_manager;
    struct netcode_address_t expired_address;
    struct netcode_address_t tail_address;
    uint8_t expired_send[NETCODE_KEY_BYTES];
    uint8_t expired_receive[NETCODE_KEY_BYTES];
    uint8_t tail_send[NETCODE_KEY_BYTES];
    uint8_t tail_receive[NETCODE_KEY_BYTES];
    int i;

    netcode_encryption_manager_reset( &encryption_manager );

    memset( &expired_address, 0, sizeof( expired_address ) );
    memset( &tail_address, 0, sizeof( tail_address ) );
    expired_address.type = NETCODE_ADDRESS_IPV6;
    expired_address.data.ipv6[7] = 1;
    expired_address.port = 21000;
    tail_address.type = NETCODE_ADDRESS_IPV6;
    tail_address.data.ipv6[7] = 1;
    tail_address.port = 21001;

    for ( i = 0; i < NETCODE_KEY_BYTES; i++ )
    {
        expired_send[i] = (uint8_t) ( 0xA0 + ( i & 0x0F ) );
        expired_receive[i] = (uint8_t) ( 0x50 + ( i & 0x0F ) );
        tail_send[i] = (uint8_t) ( 0x10 + ( i & 0x0F ) );
        tail_receive[i] = (uint8_t) ( 0xC0 + ( i & 0x0F ) );
    }

    check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager,
                                                              &expired_address,
                                                              expired_send,
                                                              expired_receive,
                                                              time,
                                                              time + 1.0,
                                                              0,
                                                              -1 ) );
    check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager,
                                                              &tail_address,
                                                              tail_send,
                                                              tail_receive,
                                                              time,
                                                              -1.0,
                                                              0,
                                                              -1 ) );

    check( netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &expired_address, time ) == 0 );
    check( memcmp( encryption_manager.send_key, expired_send, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( encryption_manager.receive_key, expired_receive, NETCODE_KEY_BYTES ) == 0 );

    // past expire_time the mapping is no longer found; retiring the tail drops it
    check( netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &expired_address, time + 2.0 ) == -1 );
    check( netcode_encryption_manager_remove_encryption_mapping( &encryption_manager, &tail_address, time + 2.0 ) == 1 );

    for ( i = 0; i < NETCODE_KEY_BYTES; i++ )
    {
        check( encryption_manager.send_key[i] == 0 );
        check( encryption_manager.receive_key[i] == 0 );
        check( encryption_manager.send_key[NETCODE_KEY_BYTES + i] == 0 );
        check( encryption_manager.receive_key[NETCODE_KEY_BYTES + i] == 0 );
    }
}

void test_replay_protection()
{
    struct netcode_replay_protection_t replay_protection;

    int i;
    for ( i = 0; i < 2; i++ )
    {
        netcode_replay_protection_reset( &replay_protection );

        check( replay_protection.most_recent_sequence == 0 );

        // the first time we receive packets, they should not be already received

        #define MAX_SEQUENCE ( NETCODE_REPLAY_PROTECTION_BUFFER_SIZE * 4 )

        uint64_t sequence;
        for ( sequence = 0; sequence < MAX_SEQUENCE; ++sequence )
        {
            check( netcode_replay_protection_already_received( &replay_protection, sequence ) == 0 );
            netcode_replay_protection_advance_sequence( &replay_protection, sequence );
        }

        // old packets outside buffer should be considered already received

        check( netcode_replay_protection_already_received( &replay_protection, 0 ) == 1 );

        // packets received a second time should be flagged already received

        for ( sequence = MAX_SEQUENCE - 10; sequence < MAX_SEQUENCE; ++sequence )
        {
            check( netcode_replay_protection_already_received( &replay_protection, sequence ) == 1 );
        }

        // jumping ahead to a much higher sequence should be considered not already received

        check( netcode_replay_protection_already_received( &replay_protection, MAX_SEQUENCE + NETCODE_REPLAY_PROTECTION_BUFFER_SIZE ) == 0 );

        // old packets should be considered already received

        for ( sequence = 0; sequence < MAX_SEQUENCE; ++sequence )
        {
            check( netcode_replay_protection_already_received( &replay_protection, sequence ) == 1 );
        }
    }

    // sequence numbers near UINT64_MAX must not be falsely rejected as replays.
    // "sequence + buffer size" overflowed in the already received check and treated
    // the top of the sequence space as ancient packets. found by fuzz_read_packet.

    netcode_replay_protection_reset( &replay_protection );

    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX - NETCODE_REPLAY_PROTECTION_BUFFER_SIZE ) == 0 );
    netcode_replay_protection_advance_sequence( &replay_protection, UINT64_MAX - NETCODE_REPLAY_PROTECTION_BUFFER_SIZE );

    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX - 1 ) == 0 );
    netcode_replay_protection_advance_sequence( &replay_protection, UINT64_MAX - 1 );

    // and a replayed packet up there is still caught

    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX - 1 ) == 1 );

    // while packets that fell out of the window are rejected as before

    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX - 1 - NETCODE_REPLAY_PROTECTION_BUFFER_SIZE ) == 1 );

    // The reserved sequence cannot be accepted or move the replay window.

    netcode_replay_protection_reset( &replay_protection );
    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX - 1 ) == 0 );
    netcode_replay_protection_advance_sequence( &replay_protection, UINT64_MAX - 1 );
    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX ) == 1 );
    netcode_replay_protection_advance_sequence( &replay_protection, UINT64_MAX );
    check( replay_protection.most_recent_sequence == UINT64_MAX - 1 );
    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX ) == 1 );
    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX - 2 ) == 0 );

    // Refusing the reserved value leaves an ordinary receive window usable.

    netcode_replay_protection_reset( &replay_protection );
    check( netcode_replay_protection_already_received( &replay_protection, 10 ) == 0 );
    netcode_replay_protection_advance_sequence( &replay_protection, 10 );
    check( netcode_replay_protection_already_received( &replay_protection, UINT64_MAX ) == 1 );
    netcode_replay_protection_advance_sequence( &replay_protection, UINT64_MAX );
    check( replay_protection.most_recent_sequence == 10 );
    check( netcode_replay_protection_already_received( &replay_protection, 11 ) == 0 );
    netcode_replay_protection_advance_sequence( &replay_protection, 11 );
    check( replay_protection.most_recent_sequence == 11 );
}

void test_read_packet_replay_protection()
{
    // encrypted packets read through netcode_read_packet with a replay protection window:
    // an identical datagram is accepted once, and a datagram that fails to decrypt leaves
    // the window unchanged, so the authentic packet with the same sequence is still accepted.

    uint8_t packet_key[NETCODE_KEY_BYTES];
    int i;
    for ( i = 0; i < NETCODE_KEY_BYTES; i++ )
    {
        packet_key[i] = (uint8_t) ( i + 1 );
    }

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_keep_alive_packet_t input_packet;
    input_packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
    input_packet.client_index = 3;
    input_packet.max_clients = 16;

    const uint64_t packet_sequence = 1000;

    uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];
    int packet_bytes = netcode_write_packet( &input_packet, packet_data, sizeof( packet_data ), packet_sequence, packet_key, TEST_PROTOCOL_ID );
    check( packet_bytes > 1 + NETCODE_MAC_BYTES );

    struct netcode_replay_protection_t replay_protection;
    uint64_t sequence;
    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    // the same encrypted datagram twice in one session: the second is rejected

    {
        netcode_replay_protection_reset( &replay_protection );

        memcpy( buffer, packet_data, (size_t) packet_bytes );
        struct netcode_connection_keep_alive_packet_t * output_packet = (struct netcode_connection_keep_alive_packet_t*)
            netcode_read_packet( buffer, packet_bytes, &sequence, packet_key, TEST_PROTOCOL_ID, 1000, 0, NULL, allowed_packet_types, &replay_protection, NULL, NULL );
        check( output_packet );
        check( sequence == packet_sequence );
        check( output_packet->packet_type == NETCODE_CONNECTION_KEEP_ALIVE_PACKET );
        check( output_packet->client_index == 3 );
        check( output_packet->max_clients == 16 );
        free( output_packet );

        memcpy( buffer, packet_data, (size_t) packet_bytes );
        check( netcode_read_packet( buffer, packet_bytes, &sequence, packet_key, TEST_PROTOCOL_ID, 1000, 0, NULL, allowed_packet_types, &replay_protection, NULL, NULL ) == NULL );
    }

    // a datagram that fails to decrypt does not move the window: the authentic packet at the same sequence is still accepted

    {
        netcode_replay_protection_reset( &replay_protection );

        const uint64_t most_recent_sequence = replay_protection.most_recent_sequence;
        check( packet_sequence > most_recent_sequence );

        memcpy( buffer, packet_data, (size_t) packet_bytes );
        buffer[packet_bytes-1] ^= 0x01;
        check( netcode_read_packet( buffer, packet_bytes, &sequence, packet_key, TEST_PROTOCOL_ID, 1000, 0, NULL, allowed_packet_types, &replay_protection, NULL, NULL ) == NULL );
        check( sequence == packet_sequence );
        check( replay_protection.most_recent_sequence == most_recent_sequence );
        check( netcode_replay_protection_already_received( &replay_protection, packet_sequence ) == 0 );

        memcpy( buffer, packet_data, (size_t) packet_bytes );
        void * output_packet = netcode_read_packet( buffer, packet_bytes, &sequence, packet_key, TEST_PROTOCOL_ID, 1000, 0, NULL, allowed_packet_types, &replay_protection, NULL, NULL );
        check( output_packet );
        check( sequence == packet_sequence );
        check( replay_protection.most_recent_sequence == packet_sequence );
        free( output_packet );
    }

    // The packet reader refuses the reserved sequence even without a replay window.

    {
        struct netcode_connection_denied_packet_t denied_packet;
        memset( &denied_packet, 0, sizeof( denied_packet ) );
        denied_packet.packet_type = NETCODE_CONNECTION_DENIED_PACKET;
        netcode_random_bytes( denied_packet.server_nonce, NETCODE_SERVER_NONCE_BYTES );
        packet_bytes = netcode_write_packet( &denied_packet, packet_data, sizeof( packet_data ), UINT64_MAX, packet_key, TEST_PROTOCOL_ID );
        check( packet_bytes > 1 + NETCODE_SERVER_NONCE_BYTES + NETCODE_MAC_BYTES );
        memcpy( buffer, packet_data, (size_t) packet_bytes );
        check( netcode_read_packet( buffer, packet_bytes, &sequence, packet_key, TEST_PROTOCOL_ID, 1000, 0, NULL, allowed_packet_types, NULL, NULL, NULL ) == NULL );

        packet_bytes = netcode_write_packet( &denied_packet, packet_data, sizeof( packet_data ), UINT64_MAX - 1, packet_key, TEST_PROTOCOL_ID );
        check( packet_bytes > 1 + NETCODE_SERVER_NONCE_BYTES + NETCODE_MAC_BYTES );
        memcpy( buffer, packet_data, (size_t) packet_bytes );
        void * output_packet = netcode_read_packet( buffer, packet_bytes, &sequence, packet_key, TEST_PROTOCOL_ID, 1000, 0, NULL, allowed_packet_types, NULL, NULL, NULL );
        check( output_packet );
        check( sequence == UINT64_MAX - 1 );
        free( output_packet );
    }
}

static int num_ignored_asserts = 0;

static void test_override_send_packet( void * context, struct netcode_address_t * to, NETCODE_CONST uint8_t * packet_data, int packet_bytes );
static int test_override_receive_packet( void * context, struct netcode_address_t * from, uint8_t * packet_data, int max_packet_bytes );

static void test_runtime_guards_assert_handler( NETCODE_CONST char * condition, NETCODE_CONST char * function, NETCODE_CONST char * file, int line )
{
    (void) condition;
    (void) function;
    (void) file;
    (void) line;
    num_ignored_asserts++;
}

void test_runtime_guards()
{
    // out of range arguments to public entry points must not crash or corrupt memory in
    // release builds, where asserts compile out. install an assert handler that continues
    // instead of aborting so this test also runs in debug builds.

    netcode_set_assert_function( test_runtime_guards_assert_handler );

    // no private key needed: nothing in this test decrypts anything

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.override_send_and_receive = 1;
    server_config.send_packet_override = test_override_send_packet;
    server_config.receive_packet_override = test_override_receive_packet;

    struct netcode_server_t * server = netcode_server_create( "127.0.0.1:40000", &server_config, 0.0 );

    check( server );

    // starting with an out of range number of clients must not start the server

    netcode_server_start( server, 0 );
    check( !netcode_server_running( server ) );

    netcode_server_start( server, -1 );
    check( !netcode_server_running( server ) );

    netcode_server_start( server, NETCODE_MAX_CLIENTS + 1 );
    check( !netcode_server_running( server ) );

    test_server_start( server, 1 );
    check( netcode_server_running( server ) );
    check( netcode_server_max_clients( server ) == 1 );

    // out of range client indices must return cleanly. max clients is 1, so 1 is out of range

    check( netcode_server_client_user_data( server, -1 ) == NULL );
    check( netcode_server_client_user_data( server, 1 ) == NULL );
    check( netcode_server_client_user_data( server, NETCODE_MAX_CLIENTS ) == NULL );

    check( netcode_server_next_packet_sequence( server, -1 ) == 0 );
    check( netcode_server_next_packet_sequence( server, 1 ) == 0 );

    check( netcode_server_client_loopback( server, -1 ) == 0 );
    check( netcode_server_client_loopback( server, 1 ) == 0 );

    int packet_bytes = 0;
    uint64_t packet_sequence = 0;
    check( netcode_server_receive_packet( server, -1, &packet_bytes, &packet_sequence ) == NULL );
    check( netcode_server_receive_packet( server, 1, &packet_bytes, &packet_sequence ) == NULL );

    uint8_t payload[NETCODE_MAX_PACKET_SIZE];
    memset( payload, 0, sizeof( payload ) );

    netcode_server_send_packet( server, -1, payload, NETCODE_MAX_PACKET_SIZE );
    netcode_server_send_packet( server, 1, payload, NETCODE_MAX_PACKET_SIZE );

    netcode_server_disconnect_client( server, -1 );
    netcode_server_disconnect_client( server, 1 );

    netcode_server_connect_loopback_client( server, -1, 1, NULL );
    netcode_server_connect_loopback_client( server, 1, 1, NULL );

    netcode_server_disconnect_loopback_client( server, -1 );
    netcode_server_disconnect_loopback_client( server, 1 );

    netcode_server_process_loopback_packet( server, -1, payload, NETCODE_MAX_PACKET_SIZE, 0 );
    netcode_server_process_loopback_packet( server, 1, payload, NETCODE_MAX_PACKET_SIZE, 0 );

    // none of the above may have connected anybody or torn anything down

    check( netcode_server_running( server ) );
    check( netcode_server_num_connected_clients( server ) == 0 );

    netcode_server_destroy( server );

    netcode_set_assert_function( netcode_default_assert_handler );
}

void test_init_and_defaults()
{
    // netcode_init is reference counted, so multiple subsystems in the same
    // application can call netcode_init and netcode_term independently.
    // the test runner has already called netcode_init once.

    check( netcode.initialized == 1 );
    check( netcode_init() == NETCODE_OK );
    check( netcode.initialized == 2 );
    netcode_term();
    check( netcode.initialized == 1 );

    // a zeroed config must give working defaults instead of crashing on a NULL allocator

    {
        struct netcode_client_config_t client_config;
        memset( &client_config, 0, sizeof( client_config ) );

        client_config.override_send_and_receive = 1;
        client_config.send_packet_override = test_override_send_packet;
        client_config.receive_packet_override = test_override_receive_packet;

        struct netcode_client_t * client = netcode_client_create( "127.0.0.1:50000", &client_config, 0.0 );

        check( client );

        netcode_client_destroy( client );
    }

    {
        struct netcode_server_config_t server_config;
        memset( &server_config, 0, sizeof( server_config ) );

        server_config.override_send_and_receive = 1;
        server_config.send_packet_override = test_override_send_packet;
        server_config.receive_packet_override = test_override_receive_packet;

        struct netcode_server_t * server = netcode_server_create( "127.0.0.1:40000", &server_config, 0.0 );

        check( server );

        netcode_server_destroy( server );
    }
}

static void * test_failing_allocate_function( void * context, size_t bytes )
{
    (void) context;
    (void) bytes;
    return NULL;
}

static int test_simulator_allocations_fail = 0;

static void * test_toggle_allocate_function( void * context, size_t bytes )
{
    (void) context;
    if ( test_simulator_allocations_fail )
        return NULL;
    return malloc( bytes );
}

static void test_toggle_free_function( void * context, void * pointer )
{
    (void) context;
    free( pointer );
}

static void test_override_send_packet( void * context, struct netcode_address_t * to, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) to;
    (void) packet_data;
    (void) packet_bytes;
}

static int test_override_receive_packet( void * context, struct netcode_address_t * from, uint8_t * packet_data, int max_packet_bytes )
{
    (void) context;
    (void) from;
    (void) packet_data;
    (void) max_packet_bytes;
    return 0;
}

void test_client_keep_alive_packet_slot_validation()
{
    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.override_send_and_receive = 1;
    client_config.send_packet_override = test_override_send_packet;
    client_config.receive_packet_override = test_override_receive_packet;

    struct netcode_client_t * client = netcode_client_create( "127.0.0.1:50000", &client_config, 0.0 );

    check( client );

    uint8_t packet_key[NETCODE_KEY_BYTES];
    netcode_generate_key( packet_key );

    memcpy( client->context.read_packet_key, packet_key, NETCODE_KEY_BYTES );
    netcode_generate_key( client->context.write_packet_key );
    client->has_server_nonce = 1;
    client->connect_token.protocol_id = TEST_PROTOCOL_ID;

    check( netcode_parse_address( "127.0.0.1:40000", &client->server_address ) == NETCODE_OK );

    struct netcode_address_t from = client->server_address;

    const int expected_client_index = 3;
    const int expected_max_clients = 7;

    client->client_index = expected_client_index;
    client->max_clients = expected_max_clients;

    netcode_client_set_state( client, NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE );

    struct
    {
        int client_index;
        int max_clients;
    } cases[] =
    {
        { -1, NETCODE_MAX_CLIENTS },
        { 0, 0 },
        { 0, NETCODE_MAX_CLIENTS + 1 },
        { 5, 5 },
        { 6, 5 }
    };

    const int num_cases = (int) ( sizeof( cases ) / sizeof( cases[0] ) );

    int i;

    for ( i = 0; i < num_cases; i++ )
    {
        struct netcode_connection_keep_alive_packet_t packet;

        packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
        packet.client_index = cases[i].client_index;
        packet.max_clients = cases[i].max_clients;

        uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

        int bytes_written = netcode_write_packet( &packet, buffer, sizeof( buffer ), (uint64_t) ( i + 1 ), packet_key, TEST_PROTOCOL_ID );

        check( bytes_written > 0 );

        netcode_client_process_packet( client, &from, buffer, bytes_written );

        check( client->client_index == expected_client_index );
        check( client->max_clients == expected_max_clients );
        check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE );
    }

    netcode_client_destroy( client );
}

void test_client_packet_from_other_address_leaves_replay_protection()
{
    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.override_send_and_receive = 1;
    client_config.send_packet_override = test_override_send_packet;
    client_config.receive_packet_override = test_override_receive_packet;

    struct netcode_client_t * client = netcode_client_create( "127.0.0.1:50000", &client_config, 0.0 );

    check( client );

    uint8_t packet_key[NETCODE_KEY_BYTES];
    netcode_generate_key( packet_key );

    memcpy( client->context.read_packet_key, packet_key, NETCODE_KEY_BYTES );
    client->connect_token.protocol_id = TEST_PROTOCOL_ID;

    // the connected client sends disconnect packets when destroyed, so give it its own write key

    netcode_generate_key( client->context.write_packet_key );
    client->has_server_nonce = 1;

    check( netcode_parse_address( "127.0.0.1:40000", &client->server_address ) == NETCODE_OK );

    struct netcode_address_t other_address;
    check( netcode_parse_address( "127.0.0.1:40001", &other_address ) == NETCODE_OK );

    client->client_index = 0;
    client->max_clients = 1;

    netcode_client_set_state( client, NETCODE_CLIENT_STATE_CONNECTED );

    uint64_t most_recent_sequence = client->replay_protection.most_recent_sequence;

    struct netcode_connection_keep_alive_packet_t packet;
    packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
    packet.client_index = 0;
    packet.max_clients = 1;

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    // a packet that authenticates under the client's key, but comes from another address,
    // must not move the client's replay protection

    int bytes_written = netcode_write_packet( &packet, buffer, sizeof( buffer ), 1000000, packet_key, TEST_PROTOCOL_ID );
    check( bytes_written > 0 );

    netcode_client_process_packet( client, &other_address, buffer, bytes_written );

    check( client->replay_protection.most_recent_sequence == most_recent_sequence );

    // and the client still accepts the next packet from its server

    client->time = 10.0;

    bytes_written = netcode_write_packet( &packet, buffer, sizeof( buffer ), 1, packet_key, TEST_PROTOCOL_ID );
    check( bytes_written > 0 );

    netcode_client_process_packet( client, &client->server_address, buffer, bytes_written );

    check( client->last_packet_receive_time == 10.0 );

    netcode_client_destroy( client );
}

// the real-socket tests bind port 0 wherever any port will do and read the port back. a server
// a client connects to needs its address in the connect token before it starts, so it takes a
// port that is free on both ipv4 and ipv6 loopback: bind port 0 on ipv4, bind the same port on
// ipv6, then release both. no test binds a fixed port that another process may hold.

static uint16_t test_free_port()
{
    int attempt;
    for ( attempt = 0; attempt < 100; attempt++ )
    {
        struct netcode_address_t address;
        check( netcode_parse_address( "127.0.0.1:0", &address ) == NETCODE_OK );

        struct netcode_socket_t ipv4;
        if ( netcode_socket_create( &ipv4, &address, NETCODE_CLIENT_SOCKET_SNDBUF_SIZE, NETCODE_CLIENT_SOCKET_RCVBUF_SIZE ) != NETCODE_SOCKET_ERROR_NONE )
            continue;

        uint16_t port = ipv4.address.port;

        check( netcode_parse_address( "[::1]:0", &address ) == NETCODE_OK );
        address.port = port;

        struct netcode_socket_t ipv6;
        int result = netcode_socket_create( &ipv6, &address, NETCODE_CLIENT_SOCKET_SNDBUF_SIZE, NETCODE_CLIENT_SOCKET_RCVBUF_SIZE );

        netcode_socket_destroy( &ipv4 );

        if ( result == NETCODE_SOCKET_ERROR_NONE )
        {
            netcode_socket_destroy( &ipv6 );
            return port;
        }
    }

    check( 0 );
    return 0;
}

static void test_address_string( char * buffer, int buffer_size, NETCODE_CONST char * host, uint16_t port )
{
    snprintf( buffer, (size_t) buffer_size, "%s:%d", host, (int) port );
}

void test_client_create_error()
{
    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );

    // successful create leaves the create error as NONE

    {
        struct netcode_client_t * client = netcode_client_create( "0.0.0.0:0", &client_config, 0.0 );

        check( client );
        check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_NONE );

        netcode_client_destroy( client );
    }

    // bad first address

    check( netcode_client_create( "not an address", &client_config, 0.0 ) == NULL );
    check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_PARSE_ADDRESS_FAILED );

    // bad second address

    check( netcode_client_create_dual( "0.0.0.0:0", "not an address", &client_config, 0.0 ) == NULL );
    check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_PARSE_ADDRESS2_FAILED );

    // the network simulator requires binding to a specific port

    {
        struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

        struct netcode_client_config_t simulator_config;
        netcode_default_client_config( &simulator_config );
        simulator_config.network_simulator = network_simulator;

        check( netcode_client_create( "0.0.0.0", &simulator_config, 0.0 ) == NULL );
        check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_SIMULATOR_REQUIRES_PORT );

        netcode_network_simulator_destroy( network_simulator );
    }

    // binding a second client to a port already in use fails at socket creation (ipv4). the
    // first client takes any free port, and the second asks for the same one

    {
        struct netcode_client_t * first_client = netcode_client_create( "127.0.0.1:0", &client_config, 0.0 );

        check( first_client );

        char busy_address[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        test_address_string( busy_address, (int) sizeof( busy_address ), "127.0.0.1", netcode_client_get_port( first_client ) );

        check( netcode_client_create( busy_address, &client_config, 0.0 ) == NULL );
        check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_CREATE_SOCKET_IPV4_FAILED );

        netcode_client_destroy( first_client );
    }

    // and the same over ipv6 reports the ipv6 error

    {
        struct netcode_client_t * first_client = netcode_client_create( "[::1]:0", &client_config, 0.0 );

        check( first_client );

        char busy_address[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        test_address_string( busy_address, (int) sizeof( busy_address ), "[::1]", netcode_client_get_port( first_client ) );

        check( netcode_client_create( busy_address, &client_config, 0.0 ) == NULL );
        check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_CREATE_SOCKET_IPV6_FAILED );

        netcode_client_destroy( first_client );
    }

    // client struct allocation failure

    {
        struct netcode_client_config_t failing_config;
        netcode_default_client_config( &failing_config );
        failing_config.allocate_function = test_failing_allocate_function;

        check( netcode_client_create( "0.0.0.0:0", &failing_config, 0.0 ) == NULL );
        check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_ALLOCATE_CLIENT_FAILED );
    }

    // override_send_and_receive with either override callback missing is refused at create time,
    // rather than calling a null pointer on the first update

    {
        struct netcode_client_config_t override_config;
        netcode_default_client_config( &override_config );
        override_config.override_send_and_receive = 1;
        override_config.send_packet_override = test_override_send_packet;

        check( netcode_client_create( "0.0.0.0:0", &override_config, 0.0 ) == NULL );
        check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_MISSING_OVERRIDE_CALLBACK );

        override_config.send_packet_override = NULL;
        override_config.receive_packet_override = test_override_receive_packet;

        check( netcode_client_create( "0.0.0.0:0", &override_config, 0.0 ) == NULL );
        check( netcode_client_create_error() == NETCODE_CLIENT_CREATE_ERROR_MISSING_OVERRIDE_CALLBACK );
    }
}

void test_server_create_error()
{
    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );

    // successful create leaves the create error as NONE

    {
        struct netcode_server_t * server = netcode_server_create( "127.0.0.1:0", &server_config, 0.0 );

        check( server );
        check( netcode_server_create_error() == NETCODE_SERVER_CREATE_ERROR_NONE );

        netcode_server_destroy( server );
    }

    // bad first address

    check( netcode_server_create( "not an address", &server_config, 0.0 ) == NULL );
    check( netcode_server_create_error() == NETCODE_SERVER_CREATE_ERROR_PARSE_ADDRESS_FAILED );

    // bad second address

    check( netcode_server_create_dual( "127.0.0.1:0", "not an address", &server_config, 0.0 ) == NULL );
    check( netcode_server_create_error() == NETCODE_SERVER_CREATE_ERROR_PARSE_ADDRESS2_FAILED );

    // a port already in use is reported as a bind failure, distinct from other socket errors (ipv4).
    // the first server takes any free port, and the second asks for the same one

    {
        struct netcode_server_t * first_server = netcode_server_create( "127.0.0.1:0", &server_config, 0.0 );

        check( first_server );

        char busy_address[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        test_address_string( busy_address, (int) sizeof( busy_address ), "127.0.0.1", netcode_server_get_port( first_server ) );

        check( netcode_server_create( busy_address, &server_config, 0.0 ) == NULL );
        check( netcode_server_create_error() == NETCODE_SERVER_CREATE_ERROR_BIND_SOCKET_IPV4_FAILED );

        netcode_server_destroy( first_server );
    }

    // and the same over ipv6 reports the ipv6 bind error

    {
        struct netcode_server_t * first_server = netcode_server_create( "[::1]:0", &server_config, 0.0 );

        check( first_server );

        char busy_address[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        test_address_string( busy_address, (int) sizeof( busy_address ), "[::1]", netcode_server_get_port( first_server ) );

        check( netcode_server_create( busy_address, &server_config, 0.0 ) == NULL );
        check( netcode_server_create_error() == NETCODE_SERVER_CREATE_ERROR_BIND_SOCKET_IPV6_FAILED );

        netcode_server_destroy( first_server );
    }

    // server struct allocation failure

    {
        struct netcode_server_config_t failing_config;
        netcode_default_server_config( &failing_config );
        failing_config.allocate_function = test_failing_allocate_function;

        check( netcode_server_create( "127.0.0.1:0", &failing_config, 0.0 ) == NULL );
        check( netcode_server_create_error() == NETCODE_SERVER_CREATE_ERROR_ALLOCATE_SERVER_FAILED );
    }

    // override_send_and_receive with either override callback missing is refused at create time,
    // rather than calling a null pointer on the first update

    {
        struct netcode_server_config_t override_config;
        netcode_default_server_config( &override_config );
        override_config.override_send_and_receive = 1;
        override_config.send_packet_override = test_override_send_packet;

        check( netcode_server_create( "127.0.0.1:0", &override_config, 0.0 ) == NULL );
        check( netcode_server_create_error() == NETCODE_SERVER_CREATE_ERROR_MISSING_OVERRIDE_CALLBACK );

        override_config.send_packet_override = NULL;
        override_config.receive_packet_override = test_override_receive_packet;

        check( netcode_server_create( "127.0.0.1:0", &override_config, 0.0 ) == NULL );
        check( netcode_server_create_error() == NETCODE_SERVER_CREATE_ERROR_MISSING_OVERRIDE_CALLBACK );
    }
}

void test_network_simulator_allocation_failure()
{
    // a failed allocation in the simulator must drop the packet, not memcpy into null

    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, test_toggle_allocate_function, test_toggle_free_function );

    check( network_simulator );

    struct netcode_address_t from;
    struct netcode_address_t to;

    check( netcode_parse_address( "127.0.0.1:50000", &from ) == NETCODE_OK );
    check( netcode_parse_address( "127.0.0.1:50001", &to ) == NETCODE_OK );

    uint8_t packet_data[256];
    int i;
    for ( i = 0; i < 256; i++ )
    {
        packet_data[i] = (uint8_t) i;
    }

    test_simulator_allocations_fail = 1;

    netcode_network_simulator_send_packet( network_simulator, &from, &to, packet_data, sizeof( packet_data ) );

    test_simulator_allocations_fail = 0;

    netcode_network_simulator_update( network_simulator, 0.0 );

    uint8_t * receive_packet_data[16];
    int receive_packet_bytes[16];
    struct netcode_address_t receive_from[16];

    check( netcode_network_simulator_receive_packets( network_simulator, &to, 16, receive_packet_data, receive_packet_bytes, receive_from ) == 0 );

    // and the simulator still works once the allocator recovers

    netcode_network_simulator_send_packet( network_simulator, &from, &to, packet_data, sizeof( packet_data ) );

    netcode_network_simulator_update( network_simulator, 0.0 );

    check( netcode_network_simulator_receive_packets( network_simulator, &to, 16, receive_packet_data, receive_packet_bytes, receive_from ) == 1 );

    // a received packet buffer belongs to the caller, so free it before the simulator goes

    test_toggle_free_function( NULL, receive_packet_data[0] );

    netcode_network_simulator_destroy( network_simulator );
}

void test_network_simulator_determinism()
{
    // the network simulator has its own seeded rng, so two simulators given
    // identical inputs must drop, delay and duplicate identically

    #define DETERMINISM_NUM_PACKETS 100
    #define DETERMINISM_MAX_RECEIVE 256

    struct netcode_network_simulator_t * simulator_a = netcode_network_simulator_create( NULL, NULL, NULL );
    struct netcode_network_simulator_t * simulator_b = netcode_network_simulator_create( NULL, NULL, NULL );

    check( simulator_a );
    check( simulator_b );

    simulator_a->latency_milliseconds = 100.0f;
    simulator_a->jitter_milliseconds = 50.0f;
    simulator_a->packet_loss_percent = 25.0f;
    simulator_a->duplicate_packet_percent = 25.0f;

    simulator_b->latency_milliseconds = 100.0f;
    simulator_b->jitter_milliseconds = 50.0f;
    simulator_b->packet_loss_percent = 25.0f;
    simulator_b->duplicate_packet_percent = 25.0f;

    struct netcode_address_t from;
    struct netcode_address_t to;
    check( netcode_parse_address( "127.0.0.1:40000", &from ) == NETCODE_OK );
    check( netcode_parse_address( "127.0.0.1:50000", &to ) == NETCODE_OK );

    int i, j;
    uint8_t packet_data[256];
    for ( i = 0; i < DETERMINISM_NUM_PACKETS; i++ )
    {
        for ( j = 0; j < (int) sizeof( packet_data ); j++ )
        {
            packet_data[j] = (uint8_t) ( i + j );
        }
        netcode_network_simulator_send_packet( simulator_a, &from, &to, packet_data, sizeof( packet_data ) );
        netcode_network_simulator_send_packet( simulator_b, &from, &to, packet_data, sizeof( packet_data ) );
    }

    int total_received = 0;

    double time;
    for ( time = 0.0; time < 2.0; time += 0.01 )
    {
        netcode_network_simulator_update( simulator_a, time );
        netcode_network_simulator_update( simulator_b, time );

        uint8_t * packet_data_a[DETERMINISM_MAX_RECEIVE];
        uint8_t * packet_data_b[DETERMINISM_MAX_RECEIVE];
        int packet_bytes_a[DETERMINISM_MAX_RECEIVE];
        int packet_bytes_b[DETERMINISM_MAX_RECEIVE];
        struct netcode_address_t from_a[DETERMINISM_MAX_RECEIVE];
        struct netcode_address_t from_b[DETERMINISM_MAX_RECEIVE];

        int num_packets_a = netcode_network_simulator_receive_packets( simulator_a, &to, DETERMINISM_MAX_RECEIVE, packet_data_a, packet_bytes_a, from_a );
        int num_packets_b = netcode_network_simulator_receive_packets( simulator_b, &to, DETERMINISM_MAX_RECEIVE, packet_data_b, packet_bytes_b, from_b );

        check( num_packets_a == num_packets_b );

        for ( i = 0; i < num_packets_a; i++ )
        {
            check( packet_bytes_a[i] == packet_bytes_b[i] );
            check( memcmp( packet_data_a[i], packet_data_b[i], packet_bytes_a[i] ) == 0 );
            free( packet_data_a[i] );
            free( packet_data_b[i] );
        }

        total_received += num_packets_a;
    }

    check( total_received > 0 );

    netcode_network_simulator_destroy( simulator_a );
    netcode_network_simulator_destroy( simulator_b );
}

void test_client_create()
{
    {
        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "127.0.0.1:0", &client_config, 0.0 );

        check( client );

        struct netcode_address_t test_address;
        netcode_parse_address( "127.0.0.1:0", &test_address );
        test_address.port = netcode_client_get_port( client );

        check( test_address.port != 0 );
        check( netcode_socket_handle_valid( client->socket_holder.ipv4.handle ) );
        check( !netcode_socket_handle_valid( client->socket_holder.ipv6.handle ) );
        check( netcode_address_equal( &client->address, &test_address ) );

        netcode_client_destroy( client );
    }

    {
        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "[::]:0", &client_config, 0.0 );

        check( client );

        struct netcode_address_t test_address;
        netcode_parse_address( "[::]:0", &test_address );
        test_address.port = netcode_client_get_port( client );

        check( test_address.port != 0 );
        check( !netcode_socket_handle_valid( client->socket_holder.ipv4.handle ) );
        check( netcode_socket_handle_valid( client->socket_holder.ipv6.handle ) );
        check( netcode_address_equal( &client->address, &test_address ) );

        netcode_client_destroy( client );
    }

    {
        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create_dual( "127.0.0.1:0", "[::]:0", &client_config, 0.0 );

        check( client );

        struct netcode_address_t test_address;
        netcode_parse_address( "127.0.0.1:0", &test_address );
        test_address.port = netcode_client_get_port( client );

        check( test_address.port != 0 );
        check( netcode_socket_handle_valid( client->socket_holder.ipv4.handle ) );
        check( netcode_socket_handle_valid( client->socket_holder.ipv6.handle ) );
        check( netcode_address_equal( &client->address, &test_address ) );

        netcode_client_destroy( client );
    }

    {
        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create_dual( "[::]:0", "127.0.0.1:0", &client_config, 0.0 );

        check( client );

        struct netcode_address_t test_address;
        netcode_parse_address( "[::]:0", &test_address );
        test_address.port = netcode_client_get_port( client );

        check( test_address.port != 0 );
        check( netcode_socket_handle_valid( client->socket_holder.ipv4.handle ) );
        check( netcode_socket_handle_valid( client->socket_holder.ipv6.handle ) );
        check( netcode_address_equal( &client->address, &test_address ) );

        netcode_client_destroy( client );
    }
}

void test_server_create()
{
    {
        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create( "127.0.0.1:0", &server_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "127.0.0.1:0", &test_address );

        check( server );
        check( netcode_server_get_port( server ) != 0 );
        check( netcode_socket_handle_valid( server->socket_holder.ipv4.handle ) );
        check( !netcode_socket_handle_valid( server->socket_holder.ipv6.handle ) );
        check( netcode_address_equal( &server->address, &test_address ) );

        netcode_server_destroy( server );
    }

    {
        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create( "[::1]:0", &server_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "[::1]:0", &test_address );

        check( server );
        check( netcode_server_get_port( server ) != 0 );
        check( !netcode_socket_handle_valid( server->socket_holder.ipv4.handle ) );
        check( netcode_socket_handle_valid( server->socket_holder.ipv6.handle ) );
        check( netcode_address_equal( &server->address, &test_address ) );

        netcode_server_destroy( server );
    }

    {
        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create_dual( "127.0.0.1:0", "[::1]:0", &server_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "127.0.0.1:0", &test_address );

        check( server );
        check( netcode_server_get_port( server ) != 0 );
        check( netcode_socket_handle_valid( server->socket_holder.ipv4.handle ) );
        check( netcode_socket_handle_valid( server->socket_holder.ipv6.handle ) );
        check( netcode_address_equal( &server->address, &test_address ) );

        netcode_server_destroy( server );
    }

    {
        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create_dual( "[::1]:0", "127.0.0.1:0", &server_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "[::1]:0", &test_address );

        check( server );
        check( netcode_server_get_port( server ) != 0 );
        check( netcode_socket_handle_valid( server->socket_holder.ipv4.handle ) );
        check( netcode_socket_handle_valid( server->socket_holder.ipv6.handle ) );
        check( netcode_address_equal( &server->address, &test_address ) );

        netcode_server_destroy( server );
    }
}

void test_server_restart_global_sequence()
{
    // global packets (challenge, denied) share per-token server to client keys with
    // per-client packets, so the global sequence must stay in the top half of the
    // sequence space or a stopped and restarted server reuses AEAD nonces. regression
    // test: netcode_server_stop zeroes the global sequence, start must re-seed it.

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.override_send_and_receive = 1;
    server_config.send_packet_override = test_override_send_packet;
    server_config.receive_packet_override = test_override_receive_packet;

    struct netcode_server_t * server = netcode_server_create( "127.0.0.1:40000", &server_config, 0.0 );

    check( server );
    check( server->global_sequence == 1ULL << 63 );

    test_server_start( server, 1 );

    check( server->global_sequence == 1ULL << 63 );

    server->global_sequence += 1000;        // as if the server had sent some global packets

    netcode_server_stop( server );

    test_server_start( server, 1 );

    check( server->global_sequence == 1ULL << 63 );

    netcode_server_destroy( server );
}

static uint8_t private_key[NETCODE_KEY_BYTES] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea,
                                                  0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4,
                                                  0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                                  0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

void test_client_server_connect()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    int server_num_packets_received = 0;
    int client_num_packets_received = 0;

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; i++ )
        packet_data[i] = (uint8_t) i;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            client_num_packets_received++;
            netcode_client_free_packet( client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( client_num_packets_received >= 10 && server_num_packets_received >= 10 )
        {
            if ( netcode_server_client_connected( server, 0 ) )
            {
                netcode_server_disconnect_client( server, 0 );
            }
        }

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( client_num_packets_received >= 10 && server_num_packets_received >= 10 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

// the server hosts are given without a port: the server takes a free port on all of them

void client_server_socket_connect_to( NETCODE_CONST char * client_address, NETCODE_CONST char * client_address2, NETCODE_CONST char * server_host, NETCODE_CONST char * server_host2, NETCODE_CONST char * connect_host )
{
    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    uint16_t server_port = test_free_port();

    char server_address[NETCODE_MAX_ADDRESS_STRING_LENGTH];
    char server_address2[NETCODE_MAX_ADDRESS_STRING_LENGTH];
    char connect_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];

    test_address_string( server_address, (int) sizeof( server_address ), server_host, server_port );
    if ( server_host2 )
    {
        test_address_string( server_address2, (int) sizeof( server_address2 ), server_host2, server_port );
    }
    test_address_string( connect_address_string, (int) sizeof( connect_address_string ), connect_host, server_port );

    NETCODE_CONST char * connect_address = connect_address_string;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );

    struct netcode_client_t * client = netcode_client_create_dual( client_address, client_address2, &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create_dual( server_address, server_host2 ? server_address2 : NULL, &server_config, time );

    check( server );

    test_server_start( server, 1 );

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    check( netcode_generate_connect_token( 1, &connect_address, &connect_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        // this test runs over real sockets while advancing virtual time, so it must yield
        // real time each iteration or the virtual timeouts can expire before the OS delivers
        // a single loopback packet

        netcode_sleep( 0.01 );

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );
}

void client_server_socket_connect( NETCODE_CONST char * client_address, NETCODE_CONST char * client_address2, NETCODE_CONST char * server_host, NETCODE_CONST char * server_host2 )
{
    client_server_socket_connect_to( client_address, client_address2, server_host, server_host2, server_host );
}

void test_client_server_ipv4_socket_connect()
{
    client_server_socket_connect("0.0.0.0:0", NULL    , "127.0.0.1", NULL   );
    client_server_socket_connect("0.0.0.0:0", NULL    , "127.0.0.1", "[::1]");
    client_server_socket_connect("0.0.0.0:0", "[::]:0", "127.0.0.1", NULL   );
    client_server_socket_connect("0.0.0.0:0", "[::]:0", "127.0.0.1", "[::1]");
}

void test_client_server_ipv6_socket_connect()
{
    client_server_socket_connect("[::]:0"   , NULL    , "[::1]", NULL       );
    client_server_socket_connect("[::]:0"   , NULL    , "[::1]", "127.0.0.1");
    client_server_socket_connect("0.0.0.0:0", "[::]:0", "[::1]", NULL       );
    client_server_socket_connect("0.0.0.0:0", "[::]:0", "[::1]", "127.0.0.1");
}

void test_client_server_dual_socket_connect()
{
    // dual stack client connects to dual stack server over ipv4

    client_server_socket_connect("0.0.0.0:0", "[::]:0", "127.0.0.1", "[::1]");

    // dual stack client connects to dual stack server over ipv6

    client_server_socket_connect("0.0.0.0:0", "[::]:0", "[::1]", "127.0.0.1");

    // dual stack client connects to the second address of a dual stack server (ipv6, then ipv4)

    client_server_socket_connect_to("0.0.0.0:0", "[::]:0", "127.0.0.1", "[::1]", "[::1]");

    client_server_socket_connect_to("0.0.0.0:0", "[::]:0", "[::1]", "127.0.0.1", "127.0.0.1");
}

void test_server_update_drains_empty_datagrams()
{
    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );

    struct netcode_client_t * client = netcode_client_create( "127.0.0.1:0", &client_config, time );

    check( client );

    char server_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
    test_address_string( server_address_string, (int) sizeof( server_address_string ), "127.0.0.1", test_free_port() );

    NETCODE_CONST char * server_address = server_address_string;

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( server_address, &server_config, time );

    check( server );

    test_server_start( server, 1 );

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        netcode_sleep( 0.01 );

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );

    netcode_sleep( 0.01 );

    netcode_server_update( server, time );

    while ( 1 )
    {
        int packet_bytes;
        uint64_t packet_sequence;
        void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
        if ( !packet )
            break;
        (void) packet_bytes;
        (void) packet_sequence;
        netcode_server_free_packet( server, packet );
    }

    struct netcode_address_t server_address_value;
    check( netcode_parse_address( server_address, &server_address_value ) == NETCODE_OK );

    struct sockaddr_storage sockaddr_to;
    socklen_t sockaddr_to_length = netcode_address_to_sockaddr( &server_address_value, &sockaddr_to );

    int i;
    for ( i = 0; i < 50; i++ )
    {
        (void) sendto( client->socket_holder.ipv4.handle, "", 0, 0, (struct sockaddr*) &sockaddr_to, sockaddr_to_length );
    }

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; i++ )
        packet_data[i] = (uint8_t) i;

    // the empty datagrams need not all be queued before this update starts, so pump
    // the server until the payload shows up, with a bounded wait so a stuck run fails
    // instead of hanging. real UDP may drop a datagram, so the payload is sent again
    // on each attempt.

    int received = 0;
    int attempts;

    for ( attempts = 0; attempts < 200 && !received; attempts++ )
    {
        netcode_client_send_packet( client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_server_update( server, time );

        while ( 1 )
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            if ( packet_bytes == NETCODE_MAX_PACKET_SIZE && memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 )
                received = 1;
            netcode_server_free_packet( server, packet );
        }

        if ( received )
            break;

        netcode_sleep( 0.01 );

        time += delta_time;
    }

    check( received );

    // a receive loop that stopped at each empty datagram would take one update per empty
    // datagram, about 50, to reach the payload. draining them takes one update; the bound
    // leaves room for real UDP to drop or delay a payload a few times.

    check( attempts < 10 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );
}

void test_client_server_keep_alive()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // connect client to server

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // pump the client and server long enough that they would timeout without keep alive packets

    int num_iterations = (int) ( 1.25f * TEST_TIMEOUT_SECONDS / delta_time ) + 1;

    int i;
    for ( i = 0; i < num_iterations; i++ )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_server_multiple_clients()
{
    #define NUM_START_STOP_ITERATIONS 3

    int max_clients[NUM_START_STOP_ITERATIONS] = { 2, 32, 5 };

    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    int i;
    for ( i = 0; i < NUM_START_STOP_ITERATIONS; i++ )
    {
        // start the server with max # of clients for this iteration

        test_server_start( server, max_clients[i] );

        // create # of client objects for this iteration and connect to server

        struct netcode_client_t ** client = (struct netcode_client_t **) malloc( sizeof( struct netcode_client_t* ) * max_clients[i] );

        check( client );

        int j;
        for ( j = 0; j < max_clients[i]; j++ )
        {
            char client_address[NETCODE_MAX_ADDRESS_STRING_LENGTH];
            snprintf( client_address, sizeof(client_address), "[::]:%d", 50000 + j );

            struct netcode_client_config_t client_config;
            netcode_default_client_config( &client_config );
            client_config.network_simulator = network_simulator;

            client[j] = netcode_client_create( client_address, &client_config, time );

            check( client[j] );

            uint64_t client_id = j;
            netcode_random_bytes( (uint8_t*) &client_id, 8 );

            NETCODE_CONST char * server_address = "[::1]:40000";

            uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

            uint8_t user_data[NETCODE_USER_DATA_BYTES];
            netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

            check( netcode_generate_connect_token( 1, 
                                                   &server_address, 
                                                   &server_address, 
                                                   TEST_CONNECT_TOKEN_EXPIRY, 
                                                   TEST_TIMEOUT_SECONDS,
                                                   client_id, 
                                                   TEST_PROTOCOL_ID, 
                                                   private_key, 
                                                   user_data, 
                                                   connect_token ) );

            netcode_client_connect( client[j], connect_token );
        }

        // make sure all clients can connect

        while ( 1 )
        {
            netcode_network_simulator_update( network_simulator, time );

            for ( j = 0; j < max_clients[i]; j++ )
            {
                netcode_client_update( client[j], time );
            }

            netcode_server_update( server, time );

            int num_connected_clients = 0;

            for ( j = 0; j < max_clients[i]; j++ )
            {
                if ( netcode_client_state( client[j] ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                    break;

                if ( netcode_client_state( client[j] ) == NETCODE_CLIENT_STATE_CONNECTED )
                    num_connected_clients++;
            }

            if ( num_connected_clients == max_clients[i] )
                break;

            time += delta_time;
        }

        check( netcode_server_num_connected_clients( server ) == max_clients[i] );

        for ( j = 0; j < max_clients[i]; j++ )
        {
            check( netcode_client_state( client[j] ) == NETCODE_CLIENT_STATE_CONNECTED );
            check( netcode_server_client_connected( server, j ) == 1 );
        }

        // make sure all clients can exchange packets with the server

        int * server_num_packets_received = (int*) malloc( sizeof(int) * max_clients[i] );
        int * client_num_packets_received = (int*) malloc( sizeof(int) * max_clients[i] );

        memset( server_num_packets_received, 0, sizeof(int) * max_clients[i] );
        memset( client_num_packets_received, 0, sizeof(int) * max_clients[i] );

        uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
        for ( j = 0; j < NETCODE_MAX_PACKET_SIZE; j++ )
            packet_data[j] = (uint8_t) j;

        while ( 1 )
        {
            netcode_network_simulator_update( network_simulator, time );

            for ( j = 0; j < max_clients[i]; j++ )
            {
                netcode_client_update( client[j], time );
            }

            netcode_server_update( server, time );

            for ( j = 0; j < max_clients[i]; j++ )
            {
                netcode_client_send_packet( client[j], packet_data, NETCODE_MAX_PACKET_SIZE );
            }

            for ( j = 0; j < max_clients[i]; j++ )
            {
                netcode_server_send_packet( server, j, packet_data, NETCODE_MAX_PACKET_SIZE );
            }

            for ( j = 0; j < max_clients[i]; j++ )
            {
                while ( 1 )             
                {
                    int packet_bytes;
                    uint64_t packet_sequence;
                    uint8_t * packet = netcode_client_receive_packet( client[j], &packet_bytes, &packet_sequence );
                    if ( !packet )
                        break;
                    (void) packet_sequence;
                    check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
                    check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
                    client_num_packets_received[j]++;
                    netcode_client_free_packet( client[j], packet );
                }
            }

            for ( j = 0; j < max_clients[i]; j++ )
            {
                while ( 1 )             
                {
                    int packet_bytes;
                    uint64_t packet_sequence;
                    void * packet = netcode_server_receive_packet( server, j, &packet_bytes, &packet_sequence );
                    if ( !packet )
                        break;
                    (void) packet_sequence;
                    check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
                    check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
                    server_num_packets_received[j]++;
                    netcode_server_free_packet( server, packet );
                }
            }

            int num_clients_ready = 0;

            for ( j = 0; j < max_clients[i]; j++ )
            {
                if ( client_num_packets_received[j] >= 1 && server_num_packets_received[j] >= 1 )
                {
                    num_clients_ready++;
                }
            }

            if ( num_clients_ready == max_clients[i] )
                break;

            for ( j = 0; j < max_clients[i]; j++ )
            {
                if ( netcode_client_state( client[j] ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                    break;
            }

            time += delta_time;
        }

        int num_clients_ready = 0;

        for ( j = 0; j < max_clients[i]; j++ )
        {
            if ( client_num_packets_received[j] >= 1 && server_num_packets_received[j] >= 1 )
            {
                num_clients_ready++;
            }
        }

        check( num_clients_ready == max_clients[i] );

        free( server_num_packets_received );
        free( client_num_packets_received );
        
        netcode_network_simulator_reset( network_simulator );

        for ( j = 0; j < max_clients[i]; j++ )
        {
            netcode_client_destroy( client[j] );
        }

        free( client );

        netcode_server_stop( server );
    }

    netcode_server_destroy( server );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_server_multiple_servers()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address[] = { "10.10.10.10:1000", "100.100.100.100:50000", "[::1]:40000" };

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 3, server_address, server_address, 3 * TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    int server_num_packets_received = 0;
    int client_num_packets_received = 0;

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; i++ )
        packet_data[i] = (uint8_t) i;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            client_num_packets_received++;
            netcode_client_free_packet( client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( client_num_packets_received >= 10 && server_num_packets_received >= 10 )
        {
            if ( netcode_server_client_connected( server, 0 ) )
            {
                netcode_server_disconnect_client( server, 0 );
            }
        }

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( client_num_packets_received >= 10 && server_num_packets_received >= 10 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connect_token_expired()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, 0, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    netcode_client_update( client, time );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_connect_matching_address_family()
{
    // model a client with one address family and no socket of the other.
    // create runs with send and receive overrides, so it opens no socket;
    // the family that exists is then a handle, and the overrides are cleared
    // before connect. connect does not read or write a socket.

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.override_send_and_receive = 1;
    client_config.send_packet_override = test_override_send_packet;
    client_config.receive_packet_override = test_override_receive_packet;

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    memset( user_data, 0, sizeof( user_data ) );

    struct netcode_client_t * client = netcode_client_create( "127.0.0.1:50000", &client_config, 0.0 );

    check( client );
    check( !netcode_socket_handle_valid( client->socket_holder.ipv4.handle ) );
    check( !netcode_socket_handle_valid( client->socket_holder.ipv6.handle ) );

    client->config.override_send_and_receive = 0;
    client->config.send_packet_override = NULL;
    client->config.receive_packet_override = NULL;
    client->socket_holder.ipv4.handle = 1;
    client->socket_holder.ipv6.handle = NETCODE_SOCKET_HANDLE_INVALID;

    NETCODE_CONST char * other_family = "[::1]:40000";
    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    check( netcode_generate_connect_token( 1, &other_family, &other_family, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN );

    client->state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->socket_holder.ipv4.handle = NETCODE_SOCKET_HANDLE_INVALID;
    netcode_client_destroy( client );

    client = netcode_client_create( "127.0.0.1:50000", &client_config, 0.0 );

    check( client );

    client->config.override_send_and_receive = 0;
    client->config.send_packet_override = NULL;
    client->config.receive_packet_override = NULL;
    client->socket_holder.ipv4.handle = 1;
    client->socket_holder.ipv6.handle = NETCODE_SOCKET_HANDLE_INVALID;

    NETCODE_CONST char * server_addresses[] = { "[::1]:40000", "127.0.0.1:40000" };

    check( netcode_generate_connect_token( 2, server_addresses, server_addresses, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );
    check( client->server_address_index == 1 );
    check( client->server_address.type == NETCODE_ADDRESS_IPV4 );
    check( client->server_address.port == 40000 );

    client->state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->socket_holder.ipv4.handle = NETCODE_SOCKET_HANDLE_INVALID;
    netcode_client_destroy( client );

    client = netcode_client_create( "127.0.0.1:50000", &client_config, 0.0 );

    check( client );

    client->config.override_send_and_receive = 0;
    client->config.send_packet_override = NULL;
    client->config.receive_packet_override = NULL;
    client->socket_holder.ipv4.handle = 1;
    client->socket_holder.ipv6.handle = NETCODE_SOCKET_HANDLE_INVALID;

    NETCODE_CONST char * stopped_addresses[] = { "127.0.0.1:40000", "[::1]:40001" };

    check( netcode_generate_connect_token( 2, stopped_addresses, stopped_addresses, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    check( client->server_address_index == 0 );
    check( client->server_address.type == NETCODE_ADDRESS_IPV4 );
    check( client->server_address.port == 40000 );

    check( netcode_client_connect_to_next_server( client ) == 0 );
    check( client->server_address_index == 0 );
    check( client->server_address.type == NETCODE_ADDRESS_IPV4 );
    check( client->server_address.port == 40000 );
    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );

    client->state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->socket_holder.ipv4.handle = NETCODE_SOCKET_HANDLE_INVALID;
    netcode_client_destroy( client );

    client = netcode_client_create( "127.0.0.1:50000", &client_config, 0.0 );

    check( client );

    client->config.override_send_and_receive = 0;
    client->config.send_packet_override = NULL;
    client->config.receive_packet_override = NULL;
    client->socket_holder.ipv4.handle = 1;
    client->socket_holder.ipv6.handle = NETCODE_SOCKET_HANDLE_INVALID;

    NETCODE_CONST char * later_addresses[] = { "127.0.0.1:40000", "[::1]:40001", "127.0.0.1:40002" };

    check( netcode_generate_connect_token( 3, later_addresses, later_addresses, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    check( client->server_address_index == 0 );
    check( client->server_address.type == NETCODE_ADDRESS_IPV4 );

    check( netcode_client_connect_to_next_server( client ) == 1 );
    check( client->server_address_index == 2 );
    check( client->server_address.type == NETCODE_ADDRESS_IPV4 );
    check( client->server_address.port == 40002 );

    client->state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->socket_holder.ipv4.handle = NETCODE_SOCKET_HANDLE_INVALID;
    netcode_client_destroy( client );

    // ipv6-only client: a token with only an ipv4 address has no family to use

    client = netcode_client_create( "[::1]:50000", &client_config, 0.0 );

    check( client );

    client->config.override_send_and_receive = 0;
    client->config.send_packet_override = NULL;
    client->config.receive_packet_override = NULL;
    client->socket_holder.ipv4.handle = NETCODE_SOCKET_HANDLE_INVALID;
    client->socket_holder.ipv6.handle = 1;

    NETCODE_CONST char * ipv4_only = "127.0.0.1:40000";

    check( netcode_generate_connect_token( 1, &ipv4_only, &ipv4_only, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN );

    client->state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->socket_holder.ipv6.handle = NETCODE_SOCKET_HANDLE_INVALID;
    netcode_client_destroy( client );

    // ipv6-only client: skip an ipv4 address and select the ipv6 one after it

    client = netcode_client_create( "[::1]:50000", &client_config, 0.0 );

    check( client );

    client->config.override_send_and_receive = 0;
    client->config.send_packet_override = NULL;
    client->config.receive_packet_override = NULL;
    client->socket_holder.ipv4.handle = NETCODE_SOCKET_HANDLE_INVALID;
    client->socket_holder.ipv6.handle = 1;

    NETCODE_CONST char * ipv6_after_ipv4_addresses[] = { "127.0.0.1:40000", "[::1]:40001" };

    check( netcode_generate_connect_token( 2, ipv6_after_ipv4_addresses, ipv6_after_ipv4_addresses, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );
    check( client->server_address_index == 1 );
    check( client->server_address.type == NETCODE_ADDRESS_IPV6 );
    check( client->server_address.port == 40001 );

    client->state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->socket_holder.ipv6.handle = NETCODE_SOCKET_HANDLE_INVALID;
    netcode_client_destroy( client );
}

void test_client_error_connect_token_expired_multiple_servers()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    NETCODE_CONST char * server_address[] = { "10.10.10.10:1000", "100.100.100.100:50000" };

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    const int connect_token_expiry = TEST_TIMEOUT_SECONDS + TEST_TIMEOUT_SECONDS / 2;

    check( netcode_generate_connect_token( 2, server_address, server_address, connect_token_expiry, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;

        if ( time > 4.0 * connect_token_expiry )
        {
            check( 0 );
            break;
        }
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_invalid_connect_token()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    netcode_random_bytes( connect_token, NETCODE_CONNECT_TOKEN_BYTES );

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    netcode_client_connect( client, connect_token );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connection_timed_out()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // connect a client to the server

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // now disable updating the server and verify that the client times out

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connection_response_timeout()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    server->flags = NETCODE_SERVER_FLAG_IGNORE_CONNECTION_RESPONSE_PACKETS;

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED  )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connection_request_timeout()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 60.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    server->flags = NETCODE_SERVER_FLAG_IGNORE_CONNECTION_REQUEST_PACKETS;

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED  )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connection_denied()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // now attempt to connect a second client. the connection should be denied.

    struct netcode_client_t * client2 = netcode_client_create( "[::]:50001", &client_config, time );

    check( client2 );

    uint8_t connect_token2[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id2 = 0;
    netcode_random_bytes( (uint8_t*) &client_id2, 8 );

    uint8_t user_data2[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data2, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id2, TEST_PROTOCOL_ID, private_key, user_data2, connect_token2 ) );

    netcode_client_connect( client2, connect_token2 );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_client_update( client2, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client2 ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_state( client2 ) == NETCODE_CLIENT_STATE_CONNECTION_DENIED );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );
    
    netcode_client_destroy( client2 );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_side_disconnect()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // disconnect client side and verify that the server sees that client disconnect cleanly, rather than timing out.

    netcode_client_disconnect( client );

    int i;
    for ( i = 0; i < 10; i++ )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_server_client_connected( server, 0 ) == 0 )
            break;

        time += delta_time;
    }

    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );
    check( netcode_server_client_disconnect_reason( server, 0 ) == NETCODE_SERVER_CLIENT_DISCONNECT_REASON_CLIENT_DISCONNECT );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_server_side_disconnect()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // disconnect server side and verify that the client disconnects cleanly, rather than timing out.

    netcode_server_disconnect_client( server, 0 );

    int i;
    for ( i = 0; i < 10; i++ )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED );
    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );
    check( netcode_server_client_disconnect_reason( server, 0 ) == NETCODE_SERVER_CLIENT_DISCONNECT_REASON_SERVER_DISCONNECT );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_server_side_disconnect_with_more_server_addresses()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    // connect a client with a token naming two servers to the first of them

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address[] = { "[::1]:40000", "[::1]:40001" };

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 2, server_address, server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_client_connected( server, 0 ) == 1 );

    // a connected client that the server disconnects goes to disconnected. it does not move
    // on to the next server address in the connect token.

    netcode_server_disconnect_client( server, 0 );

    int i;
    for ( i = 0; i < 10; i++ )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) != NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_server_client_disconnect_reason()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    // no disconnect has happened yet, so the client slot reason is none

    check( netcode_server_client_disconnect_reason( server, 0 ) == NETCODE_SERVER_CLIENT_DISCONNECT_REASON_NONE );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_disconnect_reason( server, 0 ) == NETCODE_SERVER_CLIENT_DISCONNECT_REASON_NONE );

    // stop updating the client so it goes silent. the server should time it out
    // and record that as the disconnect reason, distinct from a clean disconnect

    int i;
    for ( i = 0; i < 200; i++ )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_server_update( server, time );

        if ( !netcode_server_client_connected( server, 0 ) )
            break;

        time += delta_time;
    }

    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );
    check( netcode_server_client_disconnect_reason( server, 0 ) == NETCODE_SERVER_CLIENT_DISCONNECT_REASON_TIMED_OUT );

    // reconnect. a new client connecting to the slot clears the reason back to none

    netcode_client_disconnect( client );

    // catch the client's internal clock up to the current time before reconnecting, since it
    // was deliberately not updated above. otherwise the first update after connect sees the
    // whole timeout leg as elapsed time and immediately times out the connection request.
    netcode_client_update( client, time );

    netcode_network_simulator_reset( network_simulator );

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_disconnect_reason( server, 0 ) == NETCODE_SERVER_CLIENT_DISCONNECT_REASON_NONE );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

static struct netcode_server_t * disconnect_reentry_server;
static int disconnect_reentry_calls;
static int disconnect_reentry_depth;
static uint64_t disconnect_reentry_client_id;

static void test_server_disconnect_callback_reentry_callback( void * context, int client_index, int connected )
{
    (void) context;

    if ( connected != 0 )
        return;

    disconnect_reentry_calls++;
    disconnect_reentry_depth++;

    // stop once the callback has run again. a depth past one fails the test,
    // and returning here keeps a recursive disconnect from running away.

    if ( disconnect_reentry_depth > 1 )
    {
        printf( "disconnect callback reentered: depth %d calls %d\n", disconnect_reentry_depth, disconnect_reentry_calls );
        fflush( stdout );
        check( disconnect_reentry_depth <= 1 );
        disconnect_reentry_depth--;
        return;
    }

    // the departing client stays readable from inside the callback

    disconnect_reentry_client_id = netcode_server_client_id( disconnect_reentry_server, client_index );

    netcode_server_disconnect_client( disconnect_reentry_server, client_index );

    disconnect_reentry_depth--;
}

void test_server_disconnect_callback_reentry()
{
    // one slot, marked connected, disconnected directly. the callback disconnects
    // that same slot. the callback must run once and the slot must end disconnected.

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.override_send_and_receive = 1;
    server_config.send_packet_override = test_override_send_packet;
    server_config.receive_packet_override = test_override_receive_packet;
    server_config.connect_disconnect_callback = test_server_disconnect_callback_reentry_callback;

    struct netcode_server_t * server = netcode_server_create( "127.0.0.1:0", &server_config, 0.0 );

    check( server );

    netcode_server_start( server, 1 );

    int client_index = 0;

    struct netcode_address_t address;
    check( netcode_parse_address( "127.0.0.1:50000", &address ) == NETCODE_OK );

    uint8_t send_key[NETCODE_KEY_BYTES];
    uint8_t receive_key[NETCODE_KEY_BYTES];
    memset( send_key, 1, sizeof( send_key ) );
    memset( receive_key, 2, sizeof( receive_key ) );

    check( netcode_encryption_manager_add_encryption_mapping( &server->encryption_manager,
                                                              &address,
                                                              send_key,
                                                              receive_key,
                                                              server->time,
                                                              -1.0,
                                                              0,
                                                              -1 ) == 1 );

    server->encryption_manager.client_index[0] = client_index;
    server->client_encryption_index[client_index] = 0;
    server->client_address[client_index] = address;
    server->client_connected[client_index] = 1;
    server->client_loopback[client_index] = 0;
    server->client_id[client_index] = 1;
    server->num_connected_clients = 1;

    disconnect_reentry_server = server;
    disconnect_reentry_calls = 0;
    disconnect_reentry_depth = 0;
    disconnect_reentry_client_id = 0;

    netcode_server_disconnect_client( server, client_index );

    check( disconnect_reentry_calls == 1 );
    check( disconnect_reentry_depth == 0 );
    check( disconnect_reentry_client_id == 1 );
    check( netcode_server_client_connected( server, client_index ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );

    netcode_server_destroy( server );
}

static struct netcode_server_t * disconnect_stop_server;
static int disconnect_stop_calls;

static void test_server_disconnect_callback_stops_server_callback( void * context, int client_index, int connected )
{
    (void) context;
    (void) client_index;

    if ( connected != 0 )
        return;

    disconnect_stop_calls++;

    netcode_server_stop( disconnect_stop_server );
}

void test_server_disconnect_callback_stops_server()
{
    // one slot, marked connected, disconnected directly. the callback stops the server.
    // the stopped server must report no connected clients.

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.override_send_and_receive = 1;
    server_config.send_packet_override = test_override_send_packet;
    server_config.receive_packet_override = test_override_receive_packet;
    server_config.connect_disconnect_callback = test_server_disconnect_callback_stops_server_callback;

    struct netcode_server_t * server = netcode_server_create( "127.0.0.1:0", &server_config, 0.0 );

    check( server );

    netcode_server_start( server, 1 );

    int client_index = 0;

    struct netcode_address_t address;
    check( netcode_parse_address( "127.0.0.1:50000", &address ) == NETCODE_OK );

    // the disconnect sends packets under the send key, so this test uses keys of its own

    uint8_t send_key[NETCODE_KEY_BYTES];
    uint8_t receive_key[NETCODE_KEY_BYTES];
    netcode_generate_key( send_key );
    netcode_generate_key( receive_key );

    check( netcode_encryption_manager_add_encryption_mapping( &server->encryption_manager,
                                                              &address,
                                                              send_key,
                                                              receive_key,
                                                              server->time,
                                                              -1.0,
                                                              0,
                                                              -1 ) == 1 );

    server->encryption_manager.client_index[0] = client_index;
    server->client_encryption_index[client_index] = 0;
    server->client_address[client_index] = address;
    server->client_connected[client_index] = 1;
    server->client_loopback[client_index] = 0;
    server->client_id[client_index] = 1;
    server->num_connected_clients = 1;

    disconnect_stop_server = server;
    disconnect_stop_calls = 0;

    netcode_server_disconnect_client( server, client_index );

    check( disconnect_stop_calls == 1 );
    check( netcode_server_running( server ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );

    netcode_server_destroy( server );
}

void test_client_reconnect()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // disconnect client on the server-side and wait until client sees the disconnect

    netcode_network_simulator_reset( network_simulator );

    netcode_server_disconnect_client( server, 0 );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED );
    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );

    // now reconnect the client and verify they connect

    netcode_network_simulator_reset( network_simulator );

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_connect_token_entries()
{
    struct netcode_connect_token_entry_t connect_token_entries[NETCODE_MAX_CONNECT_TOKEN_ENTRIES];

    netcode_connect_token_entries_reset( connect_token_entries );

    struct netcode_address_t address_a;
    struct netcode_address_t address_b;

    check( netcode_parse_address( "[::1]:50000", &address_a ) == NETCODE_OK );
    check( netcode_parse_address( "[::1]:50001", &address_b ) == NETCODE_OK );

    uint64_t current_timestamp = 1000;
    uint64_t expire_timestamp = current_timestamp + 30;
    int timeout_seconds = 30;

    uint8_t mac[NETCODE_MAC_BYTES];
    memset( mac, 0, NETCODE_MAC_BYTES );

    uint8_t salt_a[NETCODE_CONNECT_SALT_BYTES];
    uint8_t salt_b[NETCODE_CONNECT_SALT_BYTES];
    memset( salt_a, 0xAA, NETCODE_CONNECT_SALT_BYTES );
    memset( salt_b, 0xBB, NETCODE_CONNECT_SALT_BYTES );

    // a connect token the history has not seen creates a pending entry

    mac[0] = 1;

    int index = netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 100.0, timeout_seconds );

    check( index >= 0 );
    check( connect_token_entries[index].state == NETCODE_CONNECT_TOKEN_ENTRY_PENDING );
    check( connect_token_entries[index].time == 100.0 );
    check( memcmp( connect_token_entries[index].client_salt, salt_a, NETCODE_CONNECT_SALT_BYTES ) == 0 );

    // a pending entry admits a retransmitted connection request from the same address with the
    // same client salt within timeout_seconds, and the entry time is not refreshed

    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 130.0, timeout_seconds ) == index );
    check( connect_token_entries[index].time == 100.0 );

    // a pending entry refuses a different client salt without mutating the entry

    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_b, expire_timestamp, current_timestamp, 110.0, timeout_seconds ) == NETCODE_CONNECT_TOKEN_ENTRY_REFUSED );
    check( memcmp( connect_token_entries[index].client_salt, salt_a, NETCODE_CONNECT_SALT_BYTES ) == 0 );
    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 120.0, timeout_seconds ) == index );

    // a pending entry refuses every other address

    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_b, mac, salt_a, expire_timestamp, current_timestamp, 120.0, timeout_seconds ) == NETCODE_CONNECT_TOKEN_ENTRY_REFUSED );

    // once timeout_seconds have elapsed since creation, a pending entry admits nothing
    // (unless timeout_seconds <= 0, which disables the timeout)

    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 130.1, timeout_seconds ) == NETCODE_CONNECT_TOKEN_ENTRY_REFUSED );
    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 200.0, -1 ) == index );

    // a consumed entry admits nothing, including the address and salt that used the connect token

    netcode_connect_token_entries_consume( connect_token_entries, index );

    check( connect_token_entries[index].state == NETCODE_CONNECT_TOKEN_ENTRY_CONSUMED );
    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 110.0, timeout_seconds ) == NETCODE_CONNECT_TOKEN_ENTRY_REFUSED );
    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_b, mac, salt_a, expire_timestamp, current_timestamp, 110.0, timeout_seconds ) == NETCODE_CONNECT_TOKEN_ENTRY_REFUSED );

    // a history whose entries all hold unexpired connect tokens refuses a new connect token
    // instead of evicting one

    int i;
    for ( i = 1; i < NETCODE_MAX_CONNECT_TOKEN_ENTRIES; i++ )
    {
        memset( mac, 0, NETCODE_MAC_BYTES );
        mac[0] = (uint8_t) ( i + 1 );
        mac[1] = (uint8_t) ( ( i + 1 ) >> 8 );
        check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 400.0, timeout_seconds ) >= 0 );
    }

    memset( mac, 0, NETCODE_MAC_BYTES );
    mac[0] = 0xFF;
    mac[1] = 0xFF;

    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 500.0, timeout_seconds ) == NETCODE_CONNECT_TOKEN_HISTORY_FULL );

    // the consumed entry is still refusing its connect token, and was not evicted by the flood

    memset( mac, 0, NETCODE_MAC_BYTES );
    mac[0] = 1;

    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, current_timestamp, 500.0, timeout_seconds ) == NETCODE_CONNECT_TOKEN_ENTRY_REFUSED );

    // entries live until their connect token expires. once they have, the history takes new connect tokens again

    memset( mac, 0, NETCODE_MAC_BYTES );
    mac[0] = 0xFF;
    mac[1] = 0xFF;

    check( netcode_connect_token_entries_find_or_add( connect_token_entries, &address_a, mac, salt_a, expire_timestamp, expire_timestamp, 600.0, timeout_seconds ) >= 0 );
}

struct test_reclaim_capture_t
{
    int packet_bytes;
    struct netcode_address_t to;
    uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];
};

static void test_reclaim_send_packet( void * context, struct netcode_address_t * to, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    struct test_reclaim_capture_t * capture = (struct test_reclaim_capture_t*) context;

    check( packet_bytes > 0 );
    check( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    capture->packet_bytes = packet_bytes;
    capture->to = *to;
    memcpy( capture->packet_data, packet_data, packet_bytes );
}

static int test_reclaim_receive_packet( void * context, struct netcode_address_t * from, uint8_t * packet_data, int max_packet_bytes )
{
    (void) context;
    (void) from;
    (void) packet_data;
    (void) max_packet_bytes;
    return 0;
}

static int test_reclaim_write_request( uint8_t * buffer,
                                       int buffer_size,
                                       struct netcode_address_t * server_address,
                                       uint64_t client_id,
                                       int timeout_seconds,
                                       uint64_t expire_timestamp,
                                       uint8_t * client_to_server_key,
                                       uint8_t * server_to_client_key,
                                       uint8_t * client_salt_out )
{
    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    uint8_t private_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    uint8_t nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    struct netcode_connect_token_private_t token;
    struct netcode_connection_request_packet_t packet;

    memset( user_data, 0, sizeof( user_data ) );

    netcode_generate_connect_token_private( &token, client_id, timeout_seconds, 1, server_address, user_data );

    if ( client_to_server_key )
        memcpy( client_to_server_key, token.client_to_server_key, NETCODE_KEY_BYTES );
    if ( server_to_client_key )
        memcpy( server_to_client_key, token.server_to_client_key, NETCODE_KEY_BYTES );

    netcode_write_connect_token_private( &token, private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    netcode_generate_nonce( nonce );

    check( netcode_encrypt_connect_token_private( private_data,
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES,
                                                  NETCODE_VERSION_INFO,
                                                  TEST_PROTOCOL_ID,
                                                  expire_timestamp,
                                                  nonce,
                                                  private_key ) == NETCODE_OK );

    packet.packet_type = NETCODE_CONNECTION_REQUEST_PACKET;
    memcpy( packet.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
    packet.protocol_id = TEST_PROTOCOL_ID;
    packet.connect_token_expire_timestamp = expire_timestamp;
    memcpy( packet.connect_token_nonce, nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
    memcpy( packet.connect_token_data, private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    netcode_random_bytes( packet.client_salt, NETCODE_CONNECT_SALT_BYTES );

    if ( client_salt_out )
        memcpy( client_salt_out, packet.client_salt, NETCODE_CONNECT_SALT_BYTES );

    return netcode_write_packet( &packet, buffer, buffer_size, 0, private_key, TEST_PROTOCOL_ID );
}

static void test_reclaim_allowed_packets( uint8_t * allowed_packets )
{
    memset( allowed_packets, 0, NETCODE_CONNECTION_NUM_PACKETS );
    allowed_packets[NETCODE_CONNECTION_REQUEST_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_RESPONSE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;
}

static int test_reclaim_history_slot( struct netcode_server_t * server )
{
    int slot = -1;
    int i;

    for ( i = 0; i < NETCODE_MAX_CONNECT_TOKEN_ENTRIES; i++ )
    {
        if ( server->connect_token_entries[i].state == NETCODE_CONNECT_TOKEN_ENTRY_FREE )
            continue;

        check( slot == -1 );
        slot = i;
    }

    return slot;
}

void test_reclaimed_connect_token_is_not_consumed()
{
    // T1's connect token expires on the timestamp passed into the server, while its encryption
    // mapping is still live on server time. T2 reclaims that history slot. Completing T1 must
    // not mark T2 consumed, and T2's retried connection request must still be accepted.

    struct test_reclaim_capture_t capture;
    struct netcode_server_config_t server_config;
    struct netcode_server_t * server;
    struct netcode_address_t server_address;
    struct netcode_address_t from_t1;
    struct netcode_address_t from_t2;
    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    uint8_t request[NETCODE_MAX_PACKET_BYTES];
    uint8_t t2_request[NETCODE_MAX_PACKET_BYTES];
    uint8_t packet_scratch[NETCODE_MAX_PACKET_BYTES];
    uint8_t t1_challenge[NETCODE_MAX_PACKET_BYTES];
    uint8_t t1_client_to_server_key[NETCODE_KEY_BYTES];
    uint8_t t1_server_to_client_key[NETCODE_KEY_BYTES];
    uint8_t t1_client_salt[NETCODE_CONNECT_SALT_BYTES];
    uint8_t t1_derived_c2s_key[NETCODE_KEY_BYTES];
    uint8_t t1_derived_s2c_key[NETCODE_KEY_BYTES];
    uint8_t t1_mac[NETCODE_MAC_BYTES];
    uint8_t t2_mac[NETCODE_MAC_BYTES];
    uint8_t zero_mac[NETCODE_MAC_BYTES];
    uint8_t response_packet[NETCODE_MAX_PACKET_BYTES];
    uint8_t challenge_allowed[NETCODE_CONNECTION_NUM_PACKETS];
    uint64_t now;
    uint64_t t1_expire;
    uint64_t t_after;
    uint64_t sequence;
    int request_bytes;
    int t2_request_bytes;
    int t1_challenge_bytes;
    int response_bytes;
    int slot;
    int encryption_index;
    struct netcode_connection_challenge_packet_t * challenge;
    struct netcode_connection_response_packet_t response;

    memset( &capture, 0, sizeof( capture ) );
    memset( zero_mac, 0, sizeof( zero_mac ) );

    check( netcode_parse_address( "127.0.0.1:40000", &server_address ) == NETCODE_OK );
    check( netcode_parse_address( "127.0.0.1:21001", &from_t1 ) == NETCODE_OK );
    check( netcode_parse_address( "127.0.0.1:21002", &from_t2 ) == NETCODE_OK );

    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.max_connect_token_lifetime = 5;
    server_config.override_send_and_receive = 1;
    server_config.callback_context = &capture;
    server_config.send_packet_override = test_reclaim_send_packet;
    server_config.receive_packet_override = test_reclaim_receive_packet;
    memcpy( server_config.private_key, private_key, NETCODE_KEY_BYTES );

    server = netcode_server_create( "127.0.0.1:40000", &server_config, 0.0 );
    check( server );

    netcode_server_start( server, 2 );

    now = (uint64_t) netcode_current_timestamp() + 1;
    t1_expire = now + (uint64_t) server->config.max_connect_token_lifetime;
    t_after = t1_expire;

    test_reclaim_allowed_packets( allowed_packets );

    request_bytes = test_reclaim_write_request( request, (int) sizeof( request ), &server_address, TEST_CLIENT_ID, TEST_TIMEOUT_SECONDS, t1_expire, t1_client_to_server_key, t1_server_to_client_key, t1_client_salt );
    check( request_bytes > 0 );

    netcode_server_read_and_process_packet( server, &from_t1, request, request_bytes, now, allowed_packets );

    check( capture.packet_bytes > 0 );
    check( netcode_address_equal( &capture.to, &from_t1 ) );
    check( ( capture.packet_data[0] & 0xF ) == NETCODE_CONNECTION_CHALLENGE_PACKET );

    t1_challenge_bytes = capture.packet_bytes;
    memcpy( t1_challenge, capture.packet_data, t1_challenge_bytes );

    slot = test_reclaim_history_slot( server );
    check( slot >= 0 );
    check( server->connect_token_entries[slot].state == NETCODE_CONNECT_TOKEN_ENTRY_PENDING );
    memcpy( t1_mac, server->connect_token_entries[slot].mac, NETCODE_MAC_BYTES );
    check( memcmp( t1_mac, zero_mac, NETCODE_MAC_BYTES ) != 0 );

    encryption_index = netcode_encryption_manager_find_encryption_mapping( &server->encryption_manager, &from_t1, server->time );
    check( encryption_index >= 0 );
    check( netcode_encryption_manager_get_connect_token_entry_index( &server->encryption_manager, encryption_index ) == slot );

    t2_request_bytes = test_reclaim_write_request( t2_request, (int) sizeof( t2_request ), &server_address, TEST_CLIENT_ID + 1, TEST_TIMEOUT_SECONDS, t_after + 30, NULL, NULL, NULL );
    check( t2_request_bytes > 0 );

    memset( &capture, 0, sizeof( capture ) );

    memcpy( packet_scratch, t2_request, t2_request_bytes );
    netcode_server_read_and_process_packet( server, &from_t2, packet_scratch, t2_request_bytes, t_after, allowed_packets );

    check( capture.packet_bytes > 0 );
    check( netcode_address_equal( &capture.to, &from_t2 ) );
    check( ( capture.packet_data[0] & 0xF ) == NETCODE_CONNECTION_CHALLENGE_PACKET );
    check( test_reclaim_history_slot( server ) == slot );
    check( server->connect_token_entries[slot].state == NETCODE_CONNECT_TOKEN_ENTRY_PENDING );
    memcpy( t2_mac, server->connect_token_entries[slot].mac, NETCODE_MAC_BYTES );
    check( memcmp( t2_mac, zero_mac, NETCODE_MAC_BYTES ) != 0 );
    check( memcmp( t1_mac, t2_mac, NETCODE_MAC_BYTES ) != 0 );

    memset( challenge_allowed, 0, sizeof( challenge_allowed ) );
    challenge_allowed[NETCODE_CONNECTION_CHALLENGE_PACKET] = 1;
    sequence = 0;

    check( t1_challenge_bytes >= 1 + ( t1_challenge[0] >> 4 ) + NETCODE_SERVER_NONCE_BYTES + NETCODE_MAC_BYTES );
    netcode_derive_packet_key( t1_derived_s2c_key, t1_server_to_client_key, t1_client_salt, t1_challenge + 1 + ( t1_challenge[0] >> 4 ) );
    netcode_derive_packet_key( t1_derived_c2s_key, t1_client_to_server_key, t1_client_salt, t1_challenge + 1 + ( t1_challenge[0] >> 4 ) );

    challenge = (struct netcode_connection_challenge_packet_t*) netcode_read_packet( t1_challenge,
                                                                                     t1_challenge_bytes,
                                                                                     &sequence,
                                                                                     t1_derived_s2c_key,
                                                                                     TEST_PROTOCOL_ID,
                                                                                     t_after,
                                                                                     0,
                                                                                     NULL,
                                                                                     challenge_allowed,
                                                                                     NULL,
                                                                                     NULL,
                                                                                     NULL );
    check( challenge );

    response.packet_type = NETCODE_CONNECTION_RESPONSE_PACKET;
    response.challenge_token_sequence = challenge->challenge_token_sequence;
    memcpy( response.challenge_token_data, challenge->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
    free( challenge );

    response_bytes = netcode_write_packet( &response, response_packet, (int) sizeof( response_packet ), 0, t1_derived_c2s_key, TEST_PROTOCOL_ID );
    check( response_bytes > 0 );

    netcode_server_read_and_process_packet( server, &from_t1, response_packet, response_bytes, t_after, allowed_packets );

    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );
    check( server->client_id[0] == TEST_CLIENT_ID );

    memset( &capture, 0, sizeof( capture ) );

    memcpy( packet_scratch, t2_request, t2_request_bytes );
    netcode_server_read_and_process_packet( server, &from_t2, packet_scratch, t2_request_bytes, t_after, allowed_packets );

    check( capture.packet_bytes > 0 );
    check( netcode_address_equal( &capture.to, &from_t2 ) );
    check( ( capture.packet_data[0] & 0xF ) == NETCODE_CONNECTION_CHALLENGE_PACKET );
    check( server->connect_token_entries[slot].state == NETCODE_CONNECT_TOKEN_ENTRY_PENDING );
    check( memcmp( server->connect_token_entries[slot].mac, t2_mac, NETCODE_MAC_BYTES ) == 0 );

    netcode_server_destroy( server );
}

void test_connection_request_mapping_expires_with_connect_token()
{
    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.override_send_and_receive = 1;
    server_config.send_packet_override = test_override_send_packet;
    server_config.receive_packet_override = test_override_receive_packet;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    double time = 100.0;

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    uint8_t connect_token_data[NETCODE_CONNECT_TOKEN_BYTES];
    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, -1, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token_data ) );

    struct netcode_connect_token_t connect_token;
    check( netcode_read_connect_token( connect_token_data, NETCODE_CONNECT_TOKEN_BYTES, &connect_token ) == NETCODE_OK );

    struct netcode_address_t from;
    check( netcode_parse_address( "[::1]:50000", &from ) == NETCODE_OK );

    struct netcode_connection_request_packet_t packet;
    memset( &packet, 0, sizeof( packet ) );
    packet.packet_type = NETCODE_CONNECTION_REQUEST_PACKET;
    memcpy( packet.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
    packet.protocol_id = TEST_PROTOCOL_ID;
    packet.connect_token_expire_timestamp = connect_token.expire_timestamp;
    memcpy( packet.connect_token_nonce, connect_token.nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
    memcpy( packet.connect_token_data, connect_token.private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    check( netcode_decrypt_connect_token_private( packet.connect_token_data,
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES,
                                                  NETCODE_VERSION_INFO,
                                                  TEST_PROTOCOL_ID,
                                                  packet.connect_token_expire_timestamp,
                                                  packet.connect_token_nonce,
                                                  private_key ) == NETCODE_OK );

    // the request arrives with two seconds left on its connect token

    uint64_t current_timestamp = connect_token.expire_timestamp - 2;

    netcode_server_process_connection_request_packet( server, &from, &packet, current_timestamp );

    check( netcode_encryption_manager_find_encryption_mapping( &server->encryption_manager, &from, time ) != -1 );

    // the handshake is never completed, so the pending mapping must not outlive the connect token

    double after_token_expires = time + 3.0;

    check( netcode_encryption_manager_find_encryption_mapping( &server->encryption_manager, &from, after_token_expires ) == -1 );

    netcode_server_destroy( server );
}

/*
    A client and a server wired directly to each other through the send and receive overrides.
    Packets are handed straight to the other side, so the handshake is exercised with no sockets,
    no network simulator and no randomness at all. The wire can drop the first packets the server
    sends, which is what makes the client retransmit its connection request, and it keeps a copy
    of the first payload packet the client sends so it can be replayed later.
*/

struct test_wire_t
{
    struct netcode_client_t * client;
    struct netcode_server_t * server;
    struct netcode_address_t client_address;
    struct netcode_address_t server_address;
    int shutting_down;
    int drop_server_packets;
    int drop_client_request_packets;
    int drop_client_response_packets;
    int drop_client_disconnect_packets;
    int num_connection_requests;
    int num_connection_responses;
    uint8_t challenge_packet[NETCODE_MAX_PACKET_BYTES];
    int challenge_packet_bytes;
    uint8_t payload_packet[NETCODE_MAX_PACKET_BYTES];
    int payload_packet_bytes;
};

static struct test_wire_t test_wire;

static void test_wire_client_send_packet( void * context, struct netcode_address_t * to, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) to;

    // the wire is down once either end is being destroyed. the disconnect packets they send
    // on the way out have nowhere to go, exactly as an application shutting down would find

    if ( test_wire.shutting_down )
        return;

    if ( packet_data[0] == NETCODE_CONNECTION_REQUEST_PACKET )
    {
        test_wire.num_connection_requests++;
        if ( test_wire.drop_client_request_packets )
            return;
    }

    if ( ( packet_data[0] & 0xF ) == NETCODE_CONNECTION_RESPONSE_PACKET )
    {
        test_wire.num_connection_responses++;
        if ( test_wire.drop_client_response_packets )
            return;
    }

    if ( ( packet_data[0] & 0xF ) == NETCODE_CONNECTION_DISCONNECT_PACKET )
    {
        if ( test_wire.drop_client_disconnect_packets )
            return;
    }

    if ( ( packet_data[0] & 0xF ) == NETCODE_CONNECTION_PAYLOAD_PACKET && test_wire.payload_packet_bytes == 0 )
    {
        memcpy( test_wire.payload_packet, packet_data, packet_bytes );
        test_wire.payload_packet_bytes = packet_bytes;
    }

    netcode_server_process_packet( test_wire.server, &test_wire.client_address, (uint8_t*) packet_data, packet_bytes );
}

static void test_wire_server_send_packet( void * context, struct netcode_address_t * to, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) to;

    if ( test_wire.shutting_down )
        return;

    if ( ( packet_data[0] & 0xF ) == NETCODE_CONNECTION_CHALLENGE_PACKET && test_wire.challenge_packet_bytes == 0 )
    {
        memcpy( test_wire.challenge_packet, packet_data, packet_bytes );
        test_wire.challenge_packet_bytes = packet_bytes;
    }

    if ( test_wire.drop_server_packets > 0 )
    {
        test_wire.drop_server_packets--;
        return;
    }

    netcode_client_process_packet( test_wire.client, &test_wire.server_address, (uint8_t*) packet_data, packet_bytes );
}

static int test_wire_receive_packet( void * context, struct netcode_address_t * from, uint8_t * packet_data, int max_packet_bytes )
{
    (void) context;
    (void) from;
    (void) packet_data;
    (void) max_packet_bytes;
    return 0;
}

static void test_wire_create( int drop_server_packets )
{
    memset( &test_wire, 0, sizeof( test_wire ) );

    test_wire.drop_server_packets = drop_server_packets;

    check( netcode_parse_address( "[::1]:50000", &test_wire.client_address ) == NETCODE_OK );
    check( netcode_parse_address( "[::1]:40000", &test_wire.server_address ) == NETCODE_OK );

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.override_send_and_receive = 1;
    client_config.send_packet_override = test_wire_client_send_packet;
    client_config.receive_packet_override = test_wire_receive_packet;

    test_wire.client = netcode_client_create( "[::1]:50000", &client_config, 0.0 );

    check( test_wire.client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.override_send_and_receive = 1;
    server_config.send_packet_override = test_wire_server_send_packet;
    server_config.receive_packet_override = test_wire_receive_packet;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    test_wire.server = netcode_server_create( "[::1]:40000", &server_config, 0.0 );

    check( test_wire.server );

    test_server_start( test_wire.server, 1 );
}

static void test_wire_destroy()
{
    test_wire.shutting_down = 1;
    netcode_server_destroy( test_wire.server );
    netcode_client_destroy( test_wire.client );
    memset( &test_wire, 0, sizeof( test_wire ) );
}

static void test_wire_connect_client( uint8_t * connect_token, double * time, double delta_time )
{
    netcode_client_connect( test_wire.client, connect_token );

    while ( 1 )
    {
        netcode_client_update( test_wire.client, *time );

        netcode_server_update( test_wire.server, *time );

        if ( netcode_client_state( test_wire.client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        *time += delta_time;
    }
}

static void test_wire_generate_connect_token( uint8_t * connect_token, uint64_t client_id, int expire_seconds )
{
    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    check( netcode_generate_connect_token( 1, &server_address, &server_address, expire_seconds, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );
}

void test_client_server_connection_request_retransmission()
{
    // the first three packets the server sends are dropped, so the client retransmits its
    // connection request into a handshake the server already has a pending history entry for

    test_wire_create( 3 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_client_connected( test_wire.server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( test_wire.server ) == 1 );
    check( test_wire.num_connection_requests >= 4 );

    test_wire_destroy();
}

void test_client_server_replay_across_sessions()
{
    test_wire_create( 0 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // connect a first session and keep a copy of a payload packet the client sends in it

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTED );

    uint8_t payload[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; i++ )
    {
        payload[i] = (uint8_t) i;
    }

    netcode_client_send_packet( test_wire.client, payload, NETCODE_MAX_PACKET_SIZE );

    check( test_wire.payload_packet_bytes > 0 );

    // disconnect, then connect a second session with a new connect token

    netcode_server_disconnect_client( test_wire.server, 0 );

    while ( netcode_client_state( test_wire.client ) > NETCODE_CLIENT_STATE_DISCONNECTED )
    {
        netcode_client_update( test_wire.client, time );
        netcode_server_update( test_wire.server, time );
        time += delta_time;
    }

    uint8_t second_connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( second_connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    test_wire_connect_client( second_connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTED );

    // drain anything the second session has delivered so far

    while ( 1 )
    {
        int packet_bytes;
        uint64_t packet_sequence;
        void * packet = netcode_server_receive_packet( test_wire.server, 0, &packet_bytes, &packet_sequence );
        if ( !packet )
            break;
        netcode_server_free_packet( test_wire.server, packet );
    }

    // the datagram from the first session is refused by the second

    netcode_server_process_packet( test_wire.server, &test_wire.client_address, test_wire.payload_packet, test_wire.payload_packet_bytes );

    int packet_bytes;
    uint64_t packet_sequence;

    check( netcode_server_receive_packet( test_wire.server, 0, &packet_bytes, &packet_sequence ) == NULL );

    test_wire_destroy();
}

void test_client_reconnect_with_used_connect_token()
{
    test_wire_create( 0 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_num_connected_clients( test_wire.server ) == 1 );

    // disconnect the client server side and wait until the client sees it

    netcode_server_disconnect_client( test_wire.server, 0 );

    while ( netcode_client_state( test_wire.client ) > NETCODE_CLIENT_STATE_DISCONNECTED )
    {
        netcode_client_update( test_wire.client, time );
        netcode_server_update( test_wire.server, time );
        time += delta_time;
    }

    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );

    // the connect token is spent. presenting it again, from the same address that used it,
    // connects nothing: the client runs out of connection request retries instead

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );

    test_wire_destroy();
}

void test_client_error_connect_token_predates_server_start()
{
    test_wire_create( 0 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // a connect token with a shorter lifetime than the server's configured maximum expires
    // earlier than any connect token the backend could have issued after the server started,
    // which is exactly the shape of a connect token issued before it started

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, NETCODE_DEFAULT_MAX_CONNECT_TOKEN_LIFETIME - 10 );

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );

    // a connect token that expires after the server start floor connects

    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_num_connected_clients( test_wire.server ) == 1 );

    test_wire_destroy();
}

void test_client_error_connect_token_at_server_start_floor()
{
    test_wire_create( 0 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // restart the server, and issue a full-lifetime token in the second it started. the token
    // expires exactly at the floor, and is refused

    netcode_server_stop( test_wire.server );
    netcode_server_start( test_wire.server, 1 );

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    struct netcode_connect_token_t decoded_token;
    check( netcode_read_connect_token( connect_token, NETCODE_CONNECT_TOKEN_BYTES, &decoded_token ) == NETCODE_OK );
    check( decoded_token.expire_timestamp == test_wire.server->min_connect_token_expire_timestamp );

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );

    // a full-lifetime token issued in the next second expires one second above the floor, and
    // is accepted

    netcode_test_fake_time_value++;

    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_num_connected_clients( test_wire.server ) == 1 );

    test_wire_destroy();
}

void test_client_reconnect_after_unfinished_handshake()
{
    test_wire_create( 0 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    // attempt 1: drop every connection response until the client times out

    test_wire.drop_client_response_packets = 1;

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT );
    check( test_wire.num_connection_responses >= 2 );
    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );

    // attempt 2: retry with the same connect token after the unfinished handshake timed out

    test_wire.drop_client_response_packets = 0;

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );
#if NETCODE_ENABLE_NONCE_AUDIT
    check( netcode_nonce_audit_repeats() == 0 );
#endif // #if NETCODE_ENABLE_NONCE_AUDIT

    test_wire_destroy();
}

void test_client_reconnect_after_aborted_handshake()
{
    test_wire_create( 0 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    // attempt 1: drop connection responses, then explicitly abort before timeout

    test_wire.drop_client_response_packets = 1;

    netcode_client_connect( test_wire.client, connect_token );

    while ( test_wire.num_connection_responses < 2 )
    {
        netcode_client_update( test_wire.client, time );
        netcode_server_update( test_wire.server, time );
        time += delta_time;
        check( time < 2.0 );
    }

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE );

    netcode_client_disconnect( test_wire.client );
    netcode_server_update( test_wire.server, time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_DISCONNECTED );
    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );
    check( netcode_encryption_manager_find_encryption_mapping( &test_wire.server->encryption_manager, &test_wire.client_address, time ) == -1 );

    // attempt 2: immediate retry with the same connect token is refused

    test_wire.drop_client_response_packets = 0;

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );
#if NETCODE_ENABLE_NONCE_AUDIT
    check( netcode_nonce_audit_repeats() == 0 );
#endif // #if NETCODE_ENABLE_NONCE_AUDIT

    test_wire_destroy();
}

void test_client_reconnect_sub_timeout_packet_loss()
{
    test_wire_create( 0 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    // attempt 1: drop both connection responses and disconnect packets so the server's
    // pending entry and half-open mapping remain live and sub-timeout

    test_wire.drop_client_response_packets = 1;
    test_wire.drop_client_disconnect_packets = 1;

    netcode_client_connect( test_wire.client, connect_token );

    while ( test_wire.num_connection_responses < 2 )
    {
        netcode_client_update( test_wire.client, time );
        netcode_server_update( test_wire.server, time );
        time += delta_time;
        check( time < 2.0 );
    }

    netcode_client_disconnect( test_wire.client );
    netcode_server_update( test_wire.server, time );

    check( netcode_encryption_manager_find_encryption_mapping( &test_wire.server->encryption_manager, &test_wire.client_address, time ) != -1 );

    // attempt 2: sub-timeout reconnect with the same connect token carries a fresh client_salt,
    // so the pending entry refuses it and zero (key, nonce) pairs repeat

    test_wire.drop_client_response_packets = 0;
    test_wire.drop_client_disconnect_packets = 0;

    test_wire_connect_client( connect_token, &time, delta_time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
    check( netcode_server_num_connected_clients( test_wire.server ) == 0 );
#if NETCODE_ENABLE_NONCE_AUDIT
    check( netcode_nonce_audit_repeats() == 0 );
#endif // #if NETCODE_ENABLE_NONCE_AUDIT

    test_wire_destroy();
}

void test_client_replayed_challenge_on_reused_token()
{
    test_wire_create( 0 );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    test_wire_generate_connect_token( connect_token, TEST_CLIENT_ID, TEST_CONNECT_TOKEN_EXPIRY );

    test_wire.drop_client_response_packets = 1;
    test_wire.drop_client_disconnect_packets = 1;

    netcode_client_connect( test_wire.client, connect_token );

    while ( test_wire.num_connection_responses < 2 )
    {
        netcode_client_update( test_wire.client, time );
        netcode_server_update( test_wire.server, time );
        time += delta_time;
        check( time < 2.0 );
    }

    check( test_wire.challenge_packet_bytes > 0 );

    netcode_client_disconnect( test_wire.client );

    // attempt 2: hold the client in SENDING_CONNECTION_REQUEST by dropping outgoing requests,
    // then replay attempt 1's challenge packet. AEAD fails under the fresh client_salt.

    int responses_before_replay = test_wire.num_connection_responses;

    test_wire.drop_client_request_packets = 1;
    test_wire.drop_client_response_packets = 0;
    test_wire.drop_client_disconnect_packets = 0;

    netcode_client_connect( test_wire.client, connect_token );
    netcode_client_update( test_wire.client, time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );
    check( test_wire.client->has_server_nonce == 0 );

    uint8_t replayed_challenge[NETCODE_MAX_PACKET_BYTES];
    memcpy( replayed_challenge, test_wire.challenge_packet, (size_t) test_wire.challenge_packet_bytes );
    netcode_client_process_packet( test_wire.client, &test_wire.server_address, replayed_challenge, test_wire.challenge_packet_bytes );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );
    check( test_wire.client->has_server_nonce == 0 );

    time += delta_time;
    netcode_client_update( test_wire.client, time );

    check( netcode_client_state( test_wire.client ) == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );
    check( test_wire.num_connection_responses == responses_before_replay );
#if NETCODE_ENABLE_NONCE_AUDIT
    check( netcode_nonce_audit_repeats() == 0 );
#endif // #if NETCODE_ENABLE_NONCE_AUDIT

    test_wire_destroy();
}

struct test_multi_server_wire_t
{
    struct netcode_client_t * client;
    struct netcode_server_t * server1;
    struct netcode_server_t * server2;
    struct netcode_address_t client_address;
    struct netcode_address_t server1_address;
    struct netcode_address_t server2_address;
    int shutting_down;
    int num_denied_from_server1;
    int num_challenges_from_server2;
};

static void test_noop_loopback_packet_callback( void * context, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    (void) context;
    (void) client_index;
    (void) packet_data;
    (void) packet_bytes;
    (void) packet_sequence;
}

static void test_multi_server_client_send_packet( void * context, struct netcode_address_t * to, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    struct test_multi_server_wire_t * wire = (struct test_multi_server_wire_t*) context;
    if ( wire->shutting_down )
        return;

    if ( netcode_address_equal( to, &wire->server1_address ) )
    {
        netcode_server_process_packet( wire->server1, &wire->client_address, (uint8_t*) packet_data, packet_bytes );
    }
    else if ( netcode_address_equal( to, &wire->server2_address ) )
    {
        netcode_server_process_packet( wire->server2, &wire->client_address, (uint8_t*) packet_data, packet_bytes );
    }
}

static void test_multi_server1_send_packet( void * context, struct netcode_address_t * to, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    (void) to;
    struct test_multi_server_wire_t * wire = (struct test_multi_server_wire_t*) context;
    if ( wire->shutting_down )
        return;

    if ( ( packet_data[0] & 0xF ) == NETCODE_CONNECTION_DENIED_PACKET )
        wire->num_denied_from_server1++;

    netcode_client_process_packet( wire->client, &wire->server1_address, (uint8_t*) packet_data, packet_bytes );
}

static void test_multi_server2_send_packet( void * context, struct netcode_address_t * to, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    (void) to;
    struct test_multi_server_wire_t * wire = (struct test_multi_server_wire_t*) context;
    if ( wire->shutting_down )
        return;

    if ( ( packet_data[0] & 0xF ) == NETCODE_CONNECTION_CHALLENGE_PACKET )
        wire->num_challenges_from_server2++;

    netcode_client_process_packet( wire->client, &wire->server2_address, (uint8_t*) packet_data, packet_bytes );
}

void test_multiple_servers_nonce_disjoint()
{
    struct test_multi_server_wire_t wire;
    memset( &wire, 0, sizeof( wire ) );

    check( netcode_parse_address( "[::1]:50000", &wire.client_address ) == NETCODE_OK );
    check( netcode_parse_address( "[::1]:40000", &wire.server1_address ) == NETCODE_OK );
    check( netcode_parse_address( "[::1]:40001", &wire.server2_address ) == NETCODE_OK );

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.callback_context = &wire;
    client_config.override_send_and_receive = 1;
    client_config.send_packet_override = test_multi_server_client_send_packet;
    client_config.receive_packet_override = test_wire_receive_packet;

    wire.client = netcode_client_create( "[::1]:50000", &client_config, 0.0 );
    check( wire.client );

    struct netcode_server_config_t server1_config;
    netcode_default_server_config( &server1_config );
    server1_config.protocol_id = TEST_PROTOCOL_ID;
    server1_config.callback_context = &wire;
    server1_config.send_loopback_packet_callback = test_noop_loopback_packet_callback;
    server1_config.override_send_and_receive = 1;
    server1_config.send_packet_override = test_multi_server1_send_packet;
    server1_config.receive_packet_override = test_wire_receive_packet;
    memcpy( &server1_config.private_key, private_key, NETCODE_KEY_BYTES );

    wire.server1 = netcode_server_create( "[::1]:40000", &server1_config, 0.0 );
    check( wire.server1 );

    struct netcode_server_config_t server2_config;
    netcode_default_server_config( &server2_config );
    server2_config.protocol_id = TEST_PROTOCOL_ID;
    server2_config.callback_context = &wire;
    server2_config.override_send_and_receive = 1;
    server2_config.send_packet_override = test_multi_server2_send_packet;
    server2_config.receive_packet_override = test_wire_receive_packet;
    memcpy( &server2_config.private_key, private_key, NETCODE_KEY_BYTES );

    wire.server2 = netcode_server_create( "[::1]:40001", &server2_config, 0.0 );
    check( wire.server2 );

    test_server_start( wire.server1, 1 );
    test_server_start( wire.server2, 1 );

    // fill server1's single slot via loopback so server1->global_sequence stays at 1ULL << 63

    netcode_server_connect_loopback_client( wire.server1, 0, 100ULL, NULL );
    check( netcode_server_num_connected_clients( wire.server1 ) == 1 );
    check( wire.server1->global_sequence == ( 1ULL << 63 ) );
    check( wire.server2->global_sequence == ( 1ULL << 63 ) );

    NETCODE_CONST char * server_addresses[] = { "[::1]:40000", "[::1]:40001" };
    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    check( netcode_generate_connect_token( 2, server_addresses, server_addresses, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, TEST_CLIENT_ID, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) == NETCODE_OK );

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    netcode_client_connect( wire.client, connect_token );

    while ( 1 )
    {
        netcode_client_update( wire.client, time );
        netcode_server_update( wire.server1, time );
        netcode_server_update( wire.server2, time );

        if ( netcode_client_state( wire.client ) <= NETCODE_CLIENT_STATE_DISCONNECTED ||
             netcode_client_state( wire.client ) == NETCODE_CLIENT_STATE_CONNECTED )
        {
            break;
        }

        time += delta_time;
        check( time < 10.0 );
    }

    // server1 emitted CONNECTION_DENIED at global_sequence 2^63 and server2 emitted
    // CONNECTION_CHALLENGE at global_sequence 2^63 for the same connect token, under
    // distinct HChaCha20-derived per-connection keys

    check( wire.num_denied_from_server1 >= 1 );
    check( wire.num_challenges_from_server2 >= 1 );
    check( wire.server1->global_sequence == ( 1ULL << 63 ) + 1 );
    check( wire.server2->global_sequence > ( 1ULL << 63 ) );
    check( netcode_client_state( wire.client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_server_num_connected_clients( wire.server2 ) == 1 );
#if NETCODE_ENABLE_NONCE_AUDIT
    check( netcode_nonce_audit_repeats() == 0 );
#endif // #if NETCODE_ENABLE_NONCE_AUDIT

    wire.shutting_down = 1;
    netcode_server_destroy( wire.server1 );
    netcode_server_destroy( wire.server2 );
    netcode_client_destroy( wire.client );
}

#if NETCODE_ENABLE_NONCE_AUDIT

void test_nonce_audit()
{
    // every packet every test above encrypted was recorded by key and nonce. a repeat is a
    // connect token encrypting two sessions, which is what the connect token lifecycle prevents

    printf( "\n    %d key and nonce pairs recorded\n    ", netcode_nonce_audit_num_pairs() );

    check( netcode_nonce_audit_num_pairs() > 0 );
    check( netcode_nonce_audit_overflow() == 0 );
    check( netcode_nonce_audit_repeats() == 0 );
}

#endif // #if NETCODE_ENABLE_NONCE_AUDIT

struct test_loopback_context_t
{
    struct netcode_client_t * client;
    struct netcode_server_t * server;
    int num_loopback_packets_sent_to_client;
    int num_loopback_packets_sent_to_server;
};

void client_send_loopback_packet_callback( void * _context, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    (void) packet_sequence;
    check( _context );
    check( client_index == 0 );
    check( packet_data );
    check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
    int i;
    for ( i = 0; i < packet_bytes; i++ )
    {
        check( packet_data[i] == (uint8_t) i );
    }
    struct test_loopback_context_t * context = (struct test_loopback_context_t*) _context;
    context->num_loopback_packets_sent_to_server++;
    netcode_server_process_loopback_packet( context->server, client_index, packet_data, packet_bytes, packet_sequence );
}

void server_send_loopback_packet_callback( void * _context, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    (void) packet_sequence;
    check( _context );
    check( client_index == 0 );
    check( packet_data );
    check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
    int i;
    for ( i = 0; i < packet_bytes; i++ )
    {
        check( packet_data[i] == (uint8_t) i );
    }
    struct test_loopback_context_t * context = (struct test_loopback_context_t*) _context;
    context->num_loopback_packets_sent_to_client++;
    netcode_client_process_loopback_packet( context->client, packet_data, packet_bytes, packet_sequence );
}

void test_disable_timeout()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    test_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, -1, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    int server_num_packets_received = 0;
    int client_num_packets_received = 0;

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; i++ )
        packet_data[i] = (uint8_t) i;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            client_num_packets_received++;
            netcode_client_free_packet( client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( client_num_packets_received >= 10 && server_num_packets_received >= 10 )
        {
            if ( netcode_server_client_connected( server, 0 ) )
            {
                netcode_server_disconnect_client( server, 0 );
            }
        }

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += 1000.0f;        // normally this would timeout the client
    }

    check( client_num_packets_received >= 10 && server_num_packets_received >= 10 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_loopback()
{
    struct test_loopback_context_t context;
    memset( &context, 0, sizeof( context ) );

    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // start the server

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    server_config.callback_context = &context;
    server_config.send_loopback_packet_callback = server_send_loopback_packet_callback;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    int max_clients = 2;

    test_server_start( server, max_clients );

    context.server = server;

    // connect a loopback client in slot 0

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.callback_context = &context;
    client_config.send_loopback_packet_callback = client_send_loopback_packet_callback;
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * loopback_client = netcode_client_create( "[::]:50000", &client_config, time );
    check( loopback_client );
    netcode_client_connect_loopback( loopback_client, 0, max_clients );
    context.client = loopback_client;

    check( netcode_client_index( loopback_client ) == 0 );
    check( netcode_client_loopback( loopback_client ) == 1 );
    check( netcode_client_max_clients( loopback_client ) == max_clients );
    check( netcode_client_state( loopback_client ) == NETCODE_CLIENT_STATE_CONNECTED );

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );
    netcode_server_connect_loopback_client( server, 0, client_id, NULL );

    check( netcode_server_client_loopback( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // connect a regular client in the other slot

    struct netcode_client_t * regular_client = netcode_client_create( "[::]:50001", &client_config, time );

    check( regular_client );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( regular_client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( regular_client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( regular_client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( regular_client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( regular_client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( regular_client ) == 1 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 1 ) == 1 );
    check( netcode_server_client_loopback( server, 0 ) == 1 );
    check( netcode_server_client_loopback( server, 1 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 2 );

    // test that we can exchange packets for the regular client and the loopback client

    int loopback_client_num_packets_received = 0;
    int loopback_server_num_packets_received = 0;
    int regular_server_num_packets_received = 0;
    int regular_client_num_packets_received = 0;

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; i++ )
        packet_data[i] = (uint8_t) i;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( regular_client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( loopback_client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_client_send_packet( regular_client, packet_data, NETCODE_MAX_PACKET_SIZE );
        
        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );
        
        netcode_server_send_packet( server, 1, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( loopback_client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            loopback_client_num_packets_received++;
            netcode_client_free_packet( loopback_client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( regular_client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            regular_client_num_packets_received++;
            netcode_client_free_packet( regular_client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            loopback_server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 1, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            regular_server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( loopback_client_num_packets_received >= 10 && loopback_server_num_packets_received >= 10 &&
             regular_client_num_packets_received >= 10 && regular_server_num_packets_received >= 10 )
            break;

        if ( netcode_client_state( regular_client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( loopback_client_num_packets_received >= 10 );
    check( loopback_server_num_packets_received >= 10 );
    check( regular_client_num_packets_received >= 10 );
    check( regular_server_num_packets_received >= 10 );
    check( context.num_loopback_packets_sent_to_client >= 10 );
    check( context.num_loopback_packets_sent_to_server >= 10 );

    // verify that we can disconnect the loopback client

    check( netcode_server_client_loopback( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 2 );

    netcode_server_disconnect_loopback_client( server, 0 );    

    check( netcode_server_client_loopback( server, 0 ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_client_disconnect_loopback( loopback_client );

    check( netcode_client_state( loopback_client ) == NETCODE_CLIENT_STATE_DISCONNECTED );

    // verify that we can reconnect the loopback client

    netcode_random_bytes( (uint8_t*) &client_id, 8 );
    netcode_server_connect_loopback_client( server, 0, client_id, NULL );

    check( netcode_server_client_loopback( server, 0 ) == 1 );
    check( netcode_server_client_loopback( server, 1 ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 1 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 2 );

    netcode_client_connect_loopback( loopback_client, 0, max_clients );
    
    check( netcode_client_index( loopback_client ) == 0 );
    check( netcode_client_loopback( loopback_client ) == 1 );
    check( netcode_client_max_clients( loopback_client ) == max_clients );
    check( netcode_client_state( loopback_client ) == NETCODE_CLIENT_STATE_CONNECTED );

    // verify that we can exchange packets for both regular and loopback client post reconnect

    loopback_server_num_packets_received = 0;
    loopback_client_num_packets_received = 0;
    regular_server_num_packets_received = 0;
    regular_client_num_packets_received = 0;
    context.num_loopback_packets_sent_to_client = 0;
    context.num_loopback_packets_sent_to_server = 0;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( regular_client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( loopback_client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_client_send_packet( regular_client, packet_data, NETCODE_MAX_PACKET_SIZE );
        
        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );
        
        netcode_server_send_packet( server, 1, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( loopback_client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            loopback_client_num_packets_received++;
            netcode_client_free_packet( loopback_client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( regular_client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            regular_client_num_packets_received++;
            netcode_client_free_packet( regular_client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            loopback_server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 1, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            check( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );
            regular_server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( loopback_client_num_packets_received >= 10 && loopback_server_num_packets_received >= 10 &&
             regular_client_num_packets_received >= 10 && regular_server_num_packets_received >= 10 )
            break;

        if ( netcode_client_state( regular_client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( loopback_client_num_packets_received >= 10 );
    check( loopback_server_num_packets_received >= 10 );
    check( regular_client_num_packets_received >= 10 );
    check( regular_server_num_packets_received >= 10 );
    check( context.num_loopback_packets_sent_to_client >= 10 );
    check( context.num_loopback_packets_sent_to_server >= 10 );

    // verify the regular client times out but loopback client doesn't

    time += 100000.0;

    netcode_server_update( server, time );

    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 1 ) == 0 );

    netcode_client_update( loopback_client, time );

    check( netcode_client_state( loopback_client ) == NETCODE_CLIENT_STATE_CONNECTED );

    // verify that disconnect all clients leaves loopback clients alone

    netcode_server_disconnect_all_clients( server );

    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 1 ) == 0 );
    check( netcode_server_client_loopback( server, 0 ) == 1 );

    // clean up

    netcode_client_destroy( regular_client );

    netcode_client_destroy( loopback_client );

    netcode_server_destroy( server );

    netcode_network_simulator_destroy( network_simulator );
}

#if NETCODE_PACKET_TAGGING

#if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX
static void test_packet_tagging_socket_option( struct netcode_socket_t * socket )
{
    // Functional check: read the applied option from the real socket.
    int tos = 0;
    socklen_t option_bytes = sizeof(tos);
    int level = socket->address.type == NETCODE_ADDRESS_IPV6 ? IPPROTO_IPV6 : IPPROTO_IP;
    int option = socket->address.type == NETCODE_ADDRESS_IPV6 ? IPV6_TCLASS : IP_TOS;
    check( getsockopt( socket->handle, level, option, (char*) &tos, &option_bytes ) == 0 );
    check( option_bytes == sizeof(tos) );
    check( tos == 46 );
}
#endif

void test_packet_tagging()
{
    // IMPORTANT: Packet tagging is off by default because it doesn't play well with some older home routers
    // See https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/networking/overviews/qos-packet-tagging
    // However, I really recommend providing players with a way to turn it on, since it can significantly reduce
    // jitter playing over Wi-Fi.

    netcode_enable_packet_tagging();

    {
        char server_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        test_address_string( server_address_string, (int) sizeof( server_address_string ), "127.0.0.1", test_free_port() );

        NETCODE_CONST char * server_address = server_address_string;

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create( server_address, &server_config, 0.0 );

        check( server );

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "127.0.0.1:0", &client_config, 0.0 );

        check( client );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX
        test_packet_tagging_socket_option( &server->socket_holder.ipv4 );
        test_packet_tagging_socket_option( &client->socket_holder.ipv4 );
#endif

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        netcode_client_destroy( client );

        netcode_server_destroy( server );
    }

    {
        char server_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        test_address_string( server_address_string, (int) sizeof( server_address_string ), "[::1]", test_free_port() );

        NETCODE_CONST char * server_address = server_address_string;

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create( server_address, &server_config, 0.0 );

        check( server );

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "[::1]:0", &client_config, 0.0 );

        check( client );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX
        test_packet_tagging_socket_option( &server->socket_holder.ipv6 );
        test_packet_tagging_socket_option( &client->socket_holder.ipv6 );
#endif

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        netcode_client_destroy( client );

        netcode_server_destroy( server );
    }
}

#endif // #if NETCODE_PACKET_TAGGING

struct test_loopback_state_change_context_t
{
    int num_state_changes;
    int from_state;
    int to_state;
};

void test_loopback_state_change_callback( void * _context, int from_state, int to_state )
{
    struct test_loopback_state_change_context_t * context = (struct test_loopback_state_change_context_t*) _context;
    context->num_state_changes++;
    context->from_state = from_state;
    context->to_state = to_state;
}

void test_loopback_noop_packet_callback( void * _context, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    (void) _context;
    (void) client_index;
    (void) packet_data;
    (void) packet_bytes;
    (void) packet_sequence;
}

void test_loopback_state_change()
{
    struct test_loopback_state_change_context_t context;
    memset( &context, 0, sizeof( context ) );

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.callback_context = &context;
    client_config.state_change_callback = test_loopback_state_change_callback;
    client_config.send_loopback_packet_callback = test_loopback_noop_packet_callback;
    client_config.override_send_and_receive = 1;
    client_config.send_packet_override = test_override_send_packet;
    client_config.receive_packet_override = test_override_receive_packet;

    struct netcode_client_t * client = netcode_client_create( "0.0.0.0:50000", &client_config, 0.0 );

    check( client );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED );
    check( context.num_state_changes == 0 );

    netcode_client_connect_loopback( client, 0, 1 );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( context.num_state_changes == 1 );
    check( context.from_state == NETCODE_CLIENT_STATE_DISCONNECTED );
    check( context.to_state == NETCODE_CLIENT_STATE_CONNECTED );

    netcode_client_disconnect_loopback( client );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED );
    check( context.num_state_changes == 2 );
    check( context.from_state == NETCODE_CLIENT_STATE_CONNECTED );
    check( context.to_state == NETCODE_CLIENT_STATE_DISCONNECTED );

    netcode_client_destroy( client );
}

void test_loopback_callback_required()
{
    // entering loopback with send_loopback_packet_callback unset would call a null pointer
    // on the next send. both sides must refuse to enter loopback instead. the guard asserts
    // as well as returning, so install the handler that continues to run this in debug.

    netcode_set_assert_function( test_runtime_guards_assert_handler );

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.override_send_and_receive = 1;
    client_config.send_packet_override = test_override_send_packet;
    client_config.receive_packet_override = test_override_receive_packet;

    struct netcode_client_t * client = netcode_client_create( "0.0.0.0:50000", &client_config, 0.0 );

    check( client );

    netcode_client_connect_loopback( client, 0, 1 );

    check( netcode_client_loopback( client ) == 0 );
    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED );

    uint8_t payload[NETCODE_MAX_PACKET_SIZE];
    memset( payload, 0, sizeof( payload ) );

    netcode_client_send_packet( client, payload, NETCODE_MAX_PACKET_SIZE );

    netcode_client_destroy( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.override_send_and_receive = 1;
    server_config.send_packet_override = test_override_send_packet;
    server_config.receive_packet_override = test_override_receive_packet;

    struct netcode_server_t * server = netcode_server_create( "127.0.0.1:40000", &server_config, 0.0 );

    check( server );

    test_server_start( server, 1 );

    netcode_server_connect_loopback_client( server, 0, 1, NULL );

    check( netcode_server_client_loopback( server, 0 ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );

    netcode_server_send_packet( server, 0, payload, NETCODE_MAX_PACKET_SIZE );

    netcode_server_destroy( server );

    netcode_set_assert_function( netcode_default_assert_handler );
}

#define RUN_TEST( test_function )                                           \
    do                                                                      \
    {                                                                       \
        printf( #test_function "\n" );                                      \
        fflush( stdout );                                                   \
        test_function();                                                    \
    }                                                                       \
    while (0)

static int netcode_test_udp_socket_available()
{
    // tests that bind real UDP sockets cannot run in a sandbox that refuses
    // them. probe once per test with an ephemeral loopback port so those
    // tests skip rather than fail where the platform offers no UDP at all.

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    SOCKET socket_handle = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( socket_handle == INVALID_SOCKET )
        return 0;
#else
    int socket_handle = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( socket_handle < 0 )
        return 0;
#endif

    struct sockaddr_in address;
    memset( &address, 0, sizeof( address ) );
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    address.sin_port = 0;

    int result = bind( socket_handle, (struct sockaddr*) &address, sizeof( address ) );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    closesocket( socket_handle );
#else
    close( socket_handle );
#endif

    return result == 0;
}

#define RUN_TEST_UDP( test_function )                                       \
    do                                                                      \
    {                                                                       \
        if ( netcode_test_udp_socket_available() )                          \
        {                                                                   \
            printf( #test_function "\n" );                                  \
            fflush( stdout );                                               \
            test_function();                                                \
        }                                                                   \
        else                                                                \
        {                                                                   \
            printf( "skip " #test_function " (UDP sockets unavailable)\n" );\
            fflush( stdout );                                               \
        }                                                                   \
    }                                                                       \
    while (0)

void test_unit_tier_is_hermetic()
{
    check( netcode_test_real_socket_count == 0 );
    check( netcode_test_real_sleep_count == 0 );
    check( netcode_test_real_time_count == 0 );
}

void netcode_test_functional()
{
    RUN_TEST_UDP( test_client_create_error );
    RUN_TEST_UDP( test_server_create_error );
    RUN_TEST_UDP( test_client_create );
    RUN_TEST_UDP( test_server_create );
    RUN_TEST_UDP( test_client_server_ipv4_socket_connect );
    RUN_TEST_UDP( test_client_server_ipv6_socket_connect );
    RUN_TEST_UDP( test_client_server_dual_socket_connect );
    RUN_TEST_UDP( test_server_update_drains_empty_datagrams );
#if NETCODE_PACKET_TAGGING
    RUN_TEST_UDP( test_packet_tagging );
#endif // #if NETCODE_PACKET_TAGGING
}

void netcode_test()
{
    if ( getenv( "NETCODE_TEST_FUNCTIONAL" ) )
    {
        netcode_test_functional();
        return;
    }

    netcode_test_real_socket_count = 0;
    netcode_test_real_sleep_count = 0;
    netcode_test_real_time_count = 0;
    netcode_test_fake_time_value = 1700000000ULL;
    netcode_test_fake_time_enabled = 1;

    //while ( 1 )
    {
        RUN_TEST( test_crypto_aead_vectors );
        RUN_TEST( test_derive_packet_keys_kat );
        RUN_TEST( test_wire_kat_vectors );
        RUN_TEST( test_queue );
        RUN_TEST( test_endian );
        RUN_TEST( test_address );
        RUN_TEST( test_sequence );
        RUN_TEST( test_socket_handle_signed );
        RUN_TEST( test_socket_buffer_size_back_off );
        RUN_TEST( test_socket_handle );
        RUN_TEST( test_connect_token );
        RUN_TEST( test_connect_token_expire_timestamp );
        RUN_TEST( test_generate_connect_token_out_of_range );
        RUN_TEST( test_challenge_token );
        RUN_TEST( test_connection_request_packet );
        RUN_TEST( test_connection_denied_packet );
        RUN_TEST( test_connection_challenge_packet );
        RUN_TEST( test_connection_response_packet );
        RUN_TEST( test_connection_keep_alive_packet );
        RUN_TEST( test_client_keep_alive_packet_slot_validation );
        RUN_TEST( test_client_packet_from_other_address_leaves_replay_protection );
        RUN_TEST( test_connection_payload_packet );
        RUN_TEST( test_connection_disconnect_packet );
        RUN_TEST( test_connect_token_public );
        RUN_TEST( test_encryption_manager );
        RUN_TEST( test_encryption_manager_zeroes_keys );
        printf( "test_expired_encryption_mapping_keys_are_zero\n" );
        test_expired_encryption_mapping_keys_are_zero( 1000.0 );
        RUN_TEST( test_replay_protection );
        RUN_TEST( test_read_packet_replay_protection );
        RUN_TEST( test_runtime_guards );
        RUN_TEST( test_init_and_defaults );
        RUN_TEST( test_network_simulator_determinism );
        RUN_TEST( test_network_simulator_allocation_failure );
        RUN_TEST( test_server_restart_global_sequence );
        RUN_TEST( test_client_server_connect );
        RUN_TEST( test_client_server_keep_alive );
        RUN_TEST( test_client_server_multiple_clients );
        RUN_TEST( test_client_server_multiple_servers );
        RUN_TEST( test_client_error_connect_token_expired );
        RUN_TEST( test_client_connect_matching_address_family );
        RUN_TEST( test_client_error_connect_token_expired_multiple_servers );
        RUN_TEST( test_client_error_invalid_connect_token );
        RUN_TEST( test_client_error_connection_timed_out );
        RUN_TEST( test_client_error_connection_response_timeout );
        RUN_TEST( test_client_error_connection_request_timeout );
        RUN_TEST( test_client_error_connection_denied );
        RUN_TEST( test_client_side_disconnect );
        RUN_TEST( test_server_side_disconnect );
        RUN_TEST( test_server_side_disconnect_with_more_server_addresses );
        RUN_TEST( test_server_client_disconnect_reason );
        RUN_TEST( test_server_disconnect_callback_reentry );
        RUN_TEST( test_server_disconnect_callback_stops_server );
        RUN_TEST( test_client_reconnect );
        RUN_TEST( test_connect_token_entries );
        RUN_TEST( test_reclaimed_connect_token_is_not_consumed );
        RUN_TEST( test_connection_request_mapping_expires_with_connect_token );
        RUN_TEST( test_client_server_connection_request_retransmission );
        RUN_TEST( test_client_server_replay_across_sessions );
        RUN_TEST( test_client_reconnect_with_used_connect_token );
        RUN_TEST( test_client_error_connect_token_predates_server_start );
        RUN_TEST( test_client_error_connect_token_at_server_start_floor );
        RUN_TEST( test_client_reconnect_after_unfinished_handshake );
        RUN_TEST( test_client_reconnect_after_aborted_handshake );
        RUN_TEST( test_client_reconnect_sub_timeout_packet_loss );
        RUN_TEST( test_client_replayed_challenge_on_reused_token );
        RUN_TEST( test_multiple_servers_nonce_disjoint );
        RUN_TEST( test_disable_timeout );
        RUN_TEST( test_loopback );
        RUN_TEST( test_loopback_state_change );
        RUN_TEST( test_loopback_callback_required );
#if NETCODE_ENABLE_NONCE_AUDIT
        RUN_TEST( test_nonce_audit );
#endif // #if NETCODE_ENABLE_NONCE_AUDIT
        RUN_TEST( test_unit_tier_is_hermetic );
    }

    netcode_test_fake_time_enabled = 0;
}

#endif // #if NETCODE_ENABLE_TESTS
