/*
    Yojimbo Unit Tests.

    Copyright © 2016 - 2026, Más Bandwidth LLC.

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

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
// <windows.h> maps SendMessage to SendMessageA/W. yojimbo.h removes the macro only after
// Connection is declared, so remove it here before any yojimbo header is parsed.
#ifdef SendMessage
#undef SendMessage
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#endif

#include "yojimbo.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_MAC || YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX
#include <signal.h>
#include <unistd.h>
#endif

#include "shared.h"
#include <typeinfo>
#include "serialize.h"
#ifndef YOJIMBO_SYSTEM_DEPS
#include <sodium.h>
#endif // #ifndef YOJIMBO_SYSTEM_DEPS

// For netcode_generate_connect_token: one test mints a single connect token and keeps it,
// which is what a matchmaker does and what Client::InsecureConnect deliberately will not do.
// Present in both configurations, unlike sodium.h.
#include "netcode.h"

#if !defined(_WIN32)
#include <dlfcn.h>

// The network simulator makes its loss and duplicate decisions with rand(). A test that needs
// the exact minimum draw can force rand() to zero here; every other call is forwarded to the
// real implementation so the rest of the suite is unaffected.
static bool g_force_zero_random = false;

extern "C" int rand( void )
{
    if ( g_force_zero_random )
        return 0;

    typedef int ( * RandFunction )( void );
    static RandFunction real_rand = (RandFunction) dlsym( RTLD_NEXT, "rand" );
    return real_rand ? real_rand() : 0;
}
#endif // #if !defined(_WIN32)

// Make the requested rand() draw return zero: force every draw on POSIX, or replay a suitable
// seed on Windows, where the CRT's small rand() range makes the search cheap.
static void ForceRandomMinimum( int callIndex )
{
#if defined(_WIN32)
    for ( unsigned int seed = 0; ; ++seed )
    {
        srand( seed );
        bool zero = false;
        for ( int i = 0; i <= callIndex; ++i )
        {
            zero = ( rand() == 0 );
            if ( i == callIndex )
                break;
        }
        if ( zero )
        {
            srand( seed );
            return;
        }
    }
#else
    (void) callIndex;
    g_force_zero_random = true;
#endif
}

static void RestoreRandom()
{
#if !defined(_WIN32)
    g_force_zero_random = false;
#endif
}

using namespace yojimbo;

static void CheckHandler( const char * condition,
                          const char * function,
                          const char * file,
                          int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifndef NDEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define check( condition )                                                     \
do                                                                             \
{                                                                              \
    if ( !(condition) )                                                        \
    {                                                                          \
        CheckHandler( #condition, __FUNCTION__, __FILE__, __LINE__ );          \
    }                                                                          \
} while(0)

// The unit tests must not bind the fixed ports the example programs use, otherwise two test
// runs on the same machine collide. Pick an unused loopback port per server, starting from a
// per-process offset so concurrent runs probe different ranges. Clients bind port zero: the
// server learns their address from the first packet, so it need not be known in advance.
static bool IsTestPortAvailable( int port )
{
#if defined(_WIN32)
    SOCKET handle = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( handle == INVALID_SOCKET )
        return false;
    struct sockaddr_in address;
    memset( &address, 0, sizeof( address ) );
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    address.sin_port = htons( (u_short) port );
    const bool available = bind( handle, (struct sockaddr*) &address, sizeof( address ) ) == 0;
    closesocket( handle );
    return available;
#else
    int handle = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( handle < 0 )
        return false;
    struct sockaddr_in address;
    memset( &address, 0, sizeof( address ) );
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    address.sin_port = htons( (uint16_t) port );
    const bool available = bind( handle, (struct sockaddr*) &address, sizeof( address ) ) == 0;
    close( handle );
    return available;
#endif
}

static int GetTestProcessId()
{
#if defined(_WIN32)
    return (int) _getpid();
#else
    return (int) getpid();
#endif
}

static int GetNextTestPort()
{
    static int nextPort = 0;
    if ( nextPort == 0 )
        nextPort = 20000 + ( GetTestProcessId() % 30000 );
    for ( int i = 0; i < 30000; ++i )
    {
        const int port = nextPort;
        if ( ++nextPort > 60000 )
            nextPort = 20000;
        if ( IsTestPortAvailable( port ) )
            return port;
    }
    check( false );
    return 0;
}

static Address TestClientAddress()
{
    return Address( "0.0.0.0", 0 );
}

static Address TestServerAddress()
{
    return Address( "127.0.0.1", GetNextTestPort() );
}

// The vendored netcode self-test section binds the example ports it was written against, so
// two yojimbo_test processes would contend for them even though yojimbo's own socket tests
// pick free ports. Hold an advisory per-user lock across that section only, so concurrent
// test runs finish without colliding while the rest of the suite still runs in parallel.
// This is best-effort: if the lock cannot be created the section runs unlocked rather than
// failing the test run.
class TestSuiteNetworkLock
{
public:
#if defined(_WIN32)
    TestSuiteNetworkLock() : m_handle( INVALID_HANDLE_VALUE )
    {
        char tempPath[MAX_PATH];
        const DWORD length = GetTempPathA( MAX_PATH, tempPath );
        if ( length == 0 || length >= MAX_PATH )
            return;
        char path[MAX_PATH];
        _snprintf_s( path, sizeof( path ), _TRUNCATE, "%syojimbo_netcode_tests.lock", tempPath );
        for ( ;; )
        {
            m_handle = CreateFileA( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL );
            if ( m_handle != INVALID_HANDLE_VALUE )
                break;
            const DWORD error = GetLastError();
            if ( error != ERROR_SHARING_VIOLATION && error != ERROR_ACCESS_DENIED )
                break;
            Sleep( 50 );
        }
    }
#else
    TestSuiteNetworkLock() : m_fd( -1 )
    {
        char path[128];
        snprintf( path, sizeof( path ), "/tmp/yojimbo_netcode_tests_%u.lock", (unsigned int) getuid() );
        m_fd = open( path, O_CREAT | O_RDWR, 0600 );
        if ( m_fd >= 0 )
            flock( m_fd, LOCK_EX );
    }
#endif

    ~TestSuiteNetworkLock()
    {
#if defined(_WIN32)
        if ( m_handle != INVALID_HANDLE_VALUE )
            CloseHandle( m_handle );
#else
        if ( m_fd >= 0 )
        {
            flock( m_fd, LOCK_UN );
            close( m_fd );
        }
#endif
    }

private:
#if defined(_WIN32)
    HANDLE m_handle;
#else
    int m_fd;
#endif
};

// Allocator used by the allocation-failure tests. Delegates to malloc/free, tracks the number of
// outstanding blocks so leaks are observable, and can be "armed" to start returning NULL after a
// given number of successful allocations (to force a failure at a specific point).
class ArmableAllocator : public Allocator
{
public:
    ArmableAllocator() : m_allocsUntilFail( -1 ), m_outstanding( 0 ) {}
    void Arm( int allocsUntilFail ) { m_allocsUntilFail = allocsUntilFail; }
    void Disarm() { m_allocsUntilFail = -1; }
    int GetOutstanding() const { return m_outstanding; }

    void * Allocate( size_t size, const char * file, int line )
    {
        if ( m_allocsUntilFail == 0 )
        {
            SetErrorLevel( ALLOCATOR_ERROR_OUT_OF_MEMORY );
            return NULL;
        }
        if ( m_allocsUntilFail > 0 )
            m_allocsUntilFail--;
        void * p = malloc( size );
        if ( !p )
        {
            SetErrorLevel( ALLOCATOR_ERROR_OUT_OF_MEMORY );
            return NULL;
        }
        m_outstanding++;
        TrackAlloc( p, size, file, line );
        return p;
    }

    void Free( void * p, const char * file, int line )
    {
        if ( !p )
            return;
        m_outstanding--;
        TrackFree( p, file, line );
        free( p );
    }

private:
    int m_allocsUntilFail;
    int m_outstanding;
};

void test_queue()
{
    const int QueueSize = 1024;

    Queue<int> queue( GetDefaultAllocator(), QueueSize );

    check( queue.IsEmpty() );
    check( !queue.IsFull() );
    check( queue.GetNumEntries() == 0 );
    check( queue.GetSize() == QueueSize );

    int NumEntries = 100;

    for ( int i = 0; i < NumEntries; ++i )
        queue.Push( i );

    check( !queue.IsEmpty() );
    check( !queue.IsFull() );
    check( queue.GetNumEntries() == NumEntries );
    check( queue.GetSize() == QueueSize );

    for ( int i = 0; i < NumEntries; ++i )
        check( queue[i] == i );

    for ( int i = 0; i < NumEntries; ++i )
        check( queue.Pop() == i );

    check( queue.IsEmpty() );
    check( !queue.IsFull() );
    check( queue.GetNumEntries() == 0 );
    check( queue.GetSize() == QueueSize );

    for ( int i = 0; i < QueueSize; ++i )
        queue.Push( i );

    check( !queue.IsEmpty() );
    check( queue.IsFull() );
    check( queue.GetNumEntries() == QueueSize );
    check( queue.GetSize() == QueueSize );

    queue.Clear();

    check( queue.IsEmpty() );
    check( !queue.IsFull() );
    check( queue.GetNumEntries() == 0 );
    check( queue.GetSize() == QueueSize );
}

bool parse_address( const char string[] )
{
    Address address( string );
    return address.IsValid();
}

void test_address_classification()
{
    // These predicates had NO coverage at all, which is how they drifted: IsMulticast,
    // IsLinkLocal and IsSiteLocal compared the first 16-bit group for exact equality
    // instead of masking the prefix. Because IsGlobalUnicast is the negation of the other
    // three, a missed classification did not return false -- it returned TRUE, so ff02::1
    // (all nodes) reported as global unicast.

    // multicast is ff00::/8 -- the whole range, not just the group ff00

    check( Address( "ff00::1" ).IsMulticast() );
    check( Address( "ff02::1" ).IsMulticast() );            // all nodes: the common one
    check( Address( "ff05::1:3" ).IsMulticast() );
    check( Address( "ffff::1" ).IsMulticast() );
    check( !Address( "ff02::1" ).IsGlobalUnicast() );
    check( !Address( "ff05::1:3" ).IsGlobalUnicast() );

    // link local is fe80::/10, so fe80 through febf

    check( Address( "fe80::1" ).IsLinkLocal() );
    check( Address( "febf::1" ).IsLinkLocal() );
    check( !Address( "febf::1" ).IsGlobalUnicast() );
    check( !Address( "fec0::1" ).IsLinkLocal() );           // fec0 is site local, not link local

    // site local is fec0::/10, so fec0 through feff

    check( Address( "fec0::1" ).IsSiteLocal() );
    check( Address( "feff::1" ).IsSiteLocal() );
    check( !Address( "fe80::1" ).IsSiteLocal() );

    // and the boundaries below the ranges must NOT be classified

    check( !Address( "fe7f::1" ).IsLinkLocal() );
    check( !Address( "fe7f::1" ).IsSiteLocal() );
    check( Address( "fe7f::1" ).IsGlobalUnicast() );
    check( !Address( "feff::1" ).IsGlobalUnicast() );

    // genuine global unicast still classifies, so the fix cannot pass by returning false

    check( Address( "2001:4860:4860::8888" ).IsGlobalUnicast() );
    check( !Address( "2001:4860:4860::8888" ).IsMulticast() );
    check( !Address( "2001:4860:4860::8888" ).IsLoopback() );

    // loopback is unchanged, and is excluded from global unicast

    check( Address( "::1" ).IsLoopback() );
    check( !Address( "::1" ).IsGlobalUnicast() );
    check( Address( "127.0.0.1" ).IsLoopback() );

    // IPv4 is never any of the IPv6 classifications

    check( !Address( "127.0.0.1" ).IsMulticast() );
    check( !Address( "10.0.0.1" ).IsGlobalUnicast() );
    check( !Address( "224.0.0.1" ).IsMulticast() );
}

// YJ-11: Address::Parse used strtol with no end pointer and treated everything past a closing
// bracket as unread. strtol stops at the first character it cannot use and reports what it read,
// so "127.0.0.1:40k" parsed as port 40, an empty port as 0, and a port with more digits than a
// long saturated instead of failing; "[::1" skipped the missing bracket and "[::1]junk" had the
// junk truncated away. Address::Parse documents that an unparseable address is cleared to
// ADDRESS_NONE, so each of these must be invalid.
void test_address_malformed_port()
{
    // trailing junk after the digits
    check( parse_address( "127.0.0.1:40k" ) == false );
    check( parse_address( "127.0.0.1:40 " ) == false );
    check( parse_address( "127.0.0.1:4.0" ) == false );
    check( parse_address( "[::1]:40k" ) == false );

    // no digits at all
    check( parse_address( "127.0.0.1:" ) == false );
    check( parse_address( "[::1]:" ) == false );

    // a sign or leading whitespace is not a port
    check( parse_address( "127.0.0.1:+40" ) == false );
    check( parse_address( "127.0.0.1:-1" ) == false );
    check( parse_address( "[::1]:+40" ) == false );
    check( parse_address( "[::1]: 40" ) == false );

    // out of range, including more digits than fit in the parser's own integer
    check( parse_address( "127.0.0.1:65536" ) == false );
    check( parse_address( "127.0.0.1:99999" ) == false );
    check( parse_address( "127.0.0.1:99999999999999999999999" ) == false );
    check( parse_address( "[::1]:65536" ) == false );
    check( parse_address( "[::1]:99999999999999999999999" ) == false );

    // bracket syntax is exact
    check( parse_address( "[::1" ) == false );
    check( parse_address( "[::1]junk" ) == false );
    check( parse_address( "[::1]]" ) == false );
    check( parse_address( "[::1]40000" ) == false );

    // at most one port separator
    {
        Address address( "1.2.3.4:5:6" );
        check( address.IsValid() == false );
        check( address.GetPort() == 0 );
    }
    check( parse_address( "127.0.0.1:5:6" ) == false );
    check( parse_address( "127.0.0.1::5" ) == false );
    check( parse_address( "127.0.0.1:5:" ) == false );
    check( parse_address( "1.2.3.4:5:6:7" ) == false );
    check( parse_address( "1.2.3.4:5::" ) == false );
    check( parse_address( "1.2.3.4:5:6 " ) == false );

    // ...and the forms that were always valid still are, with the port they say
    {
        Address address( "1.2.3.4:5" );
        check( address.IsValid() );
        check( address.GetPort() == 5 );
    }
    {
        Address address( "127.0.0.1:65535" );
        check( address.IsValid() );
        check( address.GetPort() == 65535 );
    }
    {
        Address address( "127.0.0.1:0" );
        check( address.IsValid() );
        check( address.GetPort() == 0 );
    }
    {
        Address address( "[::1]:40000" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 40000 );
    }
    {
        Address address( "[::1]" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
    }
}

void test_address()
{
    check( parse_address( "" ) == false );
    check( parse_address( "[" ) == false );
    check( parse_address( "[]" ) == false );
    check( parse_address( "[]:" ) == false );
    check( parse_address( ":" ) == false );
    check( parse_address( "1" ) == false );
    check( parse_address( "12" ) == false );
    check( parse_address( "123" ) == false );
    check( parse_address( "1234" ) == false );
    check( parse_address( "1234.0.12313.0000" ) == false );
    check( parse_address( "1234.0.12313.0000.0.0.0.0.0" ) == false );
    check( parse_address( "1312313:123131:1312313:123131:1312313:123131:1312313:123131:1312313:123131:1312313:123131" ) == false );
    check( parse_address( "." ) == false );
    check( parse_address( ".." ) == false );
    check( parse_address( "..." ) == false );
    check( parse_address( "...." ) == false );
    check( parse_address( "....." ) == false );

    {
        Address address( "107.77.207.77" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 0 );
        check( address.GetAddress4()[0] == 107 );
        check( address.GetAddress4()[1] == 77 );
        check( address.GetAddress4()[2] == 207 );
        check( address.GetAddress4()[3] == 77 );
        check( !address.IsLoopback() );
    }

    {
        Address address( "127.0.0.1" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 0 );
        check( address.GetAddress4()[0] == 127 );
        check( address.GetAddress4()[1] == 0 );
        check( address.GetAddress4()[2] == 0 );
        check( address.GetAddress4()[3] == 1 );
        check( address.IsLoopback() );
    }

    {
        Address address( "107.77.207.77:40000" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 40000 );
        check( address.GetAddress4()[0] == 107 );
        check( address.GetAddress4()[1] == 77 );
        check( address.GetAddress4()[2] == 207 );
        check( address.GetAddress4()[3] == 77 );
        check( !address.IsLoopback() );
    }

    {
        Address address( "127.0.0.1:40000" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 40000 );
        check( address.GetAddress4()[0] == 127 );
        check( address.GetAddress4()[1] == 0 );
        check( address.GetAddress4()[2] == 0 );
        check( address.GetAddress4()[3] == 1 );
        check( address.IsLoopback() );
    }

    {
        Address address( "fe80::202:b3ff:fe1e:8329" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( address.GetAddress6()[0] == 0xfe80 );
        check( address.GetAddress6()[1] == 0x0000 );
        check( address.GetAddress6()[2] == 0x0000 );
        check( address.GetAddress6()[3] == 0x0000 );
        check( address.GetAddress6()[4] == 0x0202 );
        check( address.GetAddress6()[5] == 0xb3ff );
        check( address.GetAddress6()[6] == 0xfe1e );
        check( address.GetAddress6()[7] == 0x8329 );
        check( !address.IsLoopback() );
    }

    {
        Address address( "::" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( address.GetAddress6()[0] == 0x0000 );
        check( address.GetAddress6()[1] == 0x0000 );
        check( address.GetAddress6()[2] == 0x0000 );
        check( address.GetAddress6()[3] == 0x0000 );
        check( address.GetAddress6()[4] == 0x0000 );
        check( address.GetAddress6()[5] == 0x0000 );
        check( address.GetAddress6()[6] == 0x0000 );
        check( address.GetAddress6()[7] == 0x0000 );
        check( !address.IsLoopback() );
    }

    {
        Address address( "::1" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( address.GetAddress6()[0] == 0x0000 );
        check( address.GetAddress6()[1] == 0x0000 );
        check( address.GetAddress6()[2] == 0x0000 );
        check( address.GetAddress6()[3] == 0x0000 );
        check( address.GetAddress6()[4] == 0x0000 );
        check( address.GetAddress6()[5] == 0x0000 );
        check( address.GetAddress6()[6] == 0x0000 );
        check( address.GetAddress6()[7] == 0x0001 );
        check( address.IsLoopback() );
    }

    {
        Address address( "[fe80::202:b3ff:fe1e:8329]:40000" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 40000 );
        check( address.GetAddress6()[0] == 0xfe80 );
        check( address.GetAddress6()[1] == 0x0000 );
        check( address.GetAddress6()[2] == 0x0000 );
        check( address.GetAddress6()[3] == 0x0000 );
        check( address.GetAddress6()[4] == 0x0202 );
        check( address.GetAddress6()[5] == 0xb3ff );
        check( address.GetAddress6()[6] == 0xfe1e );
        check( address.GetAddress6()[7] == 0x8329 );
        check( !address.IsLoopback() );
    }

    {
        Address address( "[::]:40000" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 40000 );
        check( address.GetAddress6()[0] == 0x0000 );
        check( address.GetAddress6()[1] == 0x0000 );
        check( address.GetAddress6()[2] == 0x0000 );
        check( address.GetAddress6()[3] == 0x0000 );
        check( address.GetAddress6()[4] == 0x0000 );
        check( address.GetAddress6()[5] == 0x0000 );
        check( address.GetAddress6()[6] == 0x0000 );
        check( address.GetAddress6()[7] == 0x0000 );
        check( !address.IsLoopback() );
    }

    {
        Address address( "[::1]:40000" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 40000 );
        check( address.GetAddress6()[0] == 0x0000 );
        check( address.GetAddress6()[1] == 0x0000 );
        check( address.GetAddress6()[2] == 0x0000 );
        check( address.GetAddress6()[3] == 0x0000 );
        check( address.GetAddress6()[4] == 0x0000 );
        check( address.GetAddress6()[5] == 0x0000 );
        check( address.GetAddress6()[6] == 0x0000 );
        check( address.GetAddress6()[7] == 0x0001 );
        check( address.IsLoopback() );
    }

    // Regression: bracketed IPv6 with no port. Scanning for ':' from the end used to misparse
    // a colon inside the address as a port separator, leaving the address unparseable.
    {
        Address address( "[::1]" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( address.GetAddress6()[0] == 0x0000 );
        check( address.GetAddress6()[1] == 0x0000 );
        check( address.GetAddress6()[2] == 0x0000 );
        check( address.GetAddress6()[3] == 0x0000 );
        check( address.GetAddress6()[4] == 0x0000 );
        check( address.GetAddress6()[5] == 0x0000 );
        check( address.GetAddress6()[6] == 0x0000 );
        check( address.GetAddress6()[7] == 0x0001 );
        check( address.IsLoopback() );
    }

    {
        Address address( "[::]" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        for ( int i = 0; i < 8; ++i )
            check( address.GetAddress6()[i] == 0x0000 );
        check( !address.IsLoopback() );
    }

    {
        Address address( "[fe80::202:b3ff:fe1e:8329]" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( address.GetAddress6()[0] == 0xfe80 );
        check( address.GetAddress6()[1] == 0x0000 );
        check( address.GetAddress6()[2] == 0x0000 );
        check( address.GetAddress6()[3] == 0x0000 );
        check( address.GetAddress6()[4] == 0x0202 );
        check( address.GetAddress6()[5] == 0xb3ff );
        check( address.GetAddress6()[6] == 0xfe1e );
        check( address.GetAddress6()[7] == 0x8329 );
        check( !address.IsLoopback() );
    }

    char buffer[MaxAddressLength];

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7] );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( address6[i] == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( address6[i] == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( address6[i] == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "::1" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7], 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( address6[i] == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6, 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( address6[i] == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6, 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( address6[i] == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "[::1]:65535" ) == 0 );
    }

    {
        Address address( "fe80::202:b3ff:fe1e:8329" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        Address address( "::1" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "::1" ) == 0 );
    }

    {
        Address address( "[fe80::202:b3ff:fe1e:8329]:65535" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        Address address( "[::1]:65535" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "[::1]:65535" ) == 0 );
    }
}

void test_address_ipv4_octets()
{
    Address address( "1.2.3.4:5000" );
    check( address.IsValid() );
    check( address.GetType() == ADDRESS_IPV4 );
    check( address.GetPort() == 5000 );
    check( address.GetAddress4()[0] == 1 );
    check( address.GetAddress4()[1] == 2 );
    check( address.GetAddress4()[2] == 3 );
    check( address.GetAddress4()[3] == 4 );

    char buffer[MaxAddressLength];
    check( strcmp( address.ToString( buffer, MaxAddressLength ), "1.2.3.4:5000" ) == 0 );
}

void test_network_simulator_drains_all_slots()
{
    // Regression: ReceivePackets used to scan only the first min(numEntries, maxPackets) ring
    // slots. Packets are stored across the whole ring (at m_currentIndex), so when a caller
    // passes maxPackets < numEntries, any packet held in a tail slot was never drained. It must
    // scan the entire ring and only cap how many packets it returns per call.

    const int NumEntries = 4;

    NetworkSimulator sim( GetDefaultAllocator(), NumEntries, 0.0 );

    // Negative loss/duplicate rates make those RNG checks impossible to fire (random_float is
    // always >= 0), so sends are fully deterministic, and they mark the simulator active.
    sim.SetPacketLoss( -1.0f );
    sim.SetDuplicates( -1.0f );

    uint8_t payload[8];
    for ( int i = 0; i < NumEntries; ++i )
    {
        memset( payload, (uint8_t) i, sizeof( payload ) );
        sim.SendPacket( i, payload, sizeof( payload ) );    // fills ring slots 0..NumEntries-1, "to" == slot
    }

    sim.AdvanceTime( 1.0 );     // past every delivery time (latency 0)

    // Drain with maxPackets (2) smaller than the ring (4). Everything must come out across calls,
    // including packets in the tail slots that the old code stranded.
    bool seen[NumEntries] = {};
    int totalReceived = 0;

    for ( int iter = 0; iter < NumEntries + 2; ++iter )
    {
        uint8_t * packetData[2];
        int packetBytes[2];
        int to[2];
        const int n = sim.ReceivePackets( 2, packetData, packetBytes, to );
        check( n <= 2 );        // never returns more than maxPackets
        for ( int i = 0; i < n; ++i )
        {
            check( to[i] >= 0 && to[i] < NumEntries );
            check( !seen[to[i]] );      // no packet delivered twice
            check( packetBytes[i] == (int) sizeof( payload ) );
            seen[to[i]] = true;
            totalReceived++;
            YOJIMBO_FREE( sim.GetAllocator(), packetData[i] );
        }
    }

    check( totalReceived == NumEntries );
    for ( int i = 0; i < NumEntries; ++i )
        check( seen[i] );       // tail slots (index >= maxPackets) were drained too
}

void test_network_simulator_zero_percent_no_loss_or_duplicates()
{
    // At 0% loss and 0% duplicates the simulator must not drop or duplicate a packet even when
    // the random draw lands on its minimum value (0.0f). A <= comparison treats that draw as
    // a hit and silently changes behavior at a zero setting.

    const int NumEntries = 8;

    uint8_t payload[8];
    memset( payload, 0x5A, sizeof( payload ) );

    // Loss is decided before the packet is stored. Disable duplicates so the zero draw reaches
    // only the loss check.
    {
        NetworkSimulator sim( GetDefaultAllocator(), NumEntries, 0.0 );
        sim.SetPacketLoss( 0.0f );
        sim.SetDuplicates( -1.0f );

        ForceRandomMinimum( 0 );

        sim.SendPacket( 0, payload, sizeof( payload ) );
        sim.AdvanceTime( 1.0 );

        uint8_t * packetData[4];
        int packetBytes[4];
        int to[4];
        const int n = sim.ReceivePackets( 4, packetData, packetBytes, to );
        check( n == 1 );        // the packet must not be dropped
        for ( int i = 0; i < n; ++i )
            YOJIMBO_FREE( sim.GetAllocator(), packetData[i] );
    }

    // Duplicates are decided after the packet is stored, on the next draw. Disable loss so the
    // zero draw reaches the duplicate check.
    {
        NetworkSimulator sim( GetDefaultAllocator(), NumEntries, 0.0 );
        sim.SetPacketLoss( -1.0f );
        sim.SetDuplicates( 0.0f );

        ForceRandomMinimum( 1 );

        sim.SendPacket( 0, payload, sizeof( payload ) );
        sim.AdvanceTime( 1.0 );

        uint8_t * packetData[4];
        int packetBytes[4];
        int to[4];
        const int n = sim.ReceivePackets( 4, packetData, packetBytes, to );
        check( n == 1 );        // the packet must not be duplicated
        for ( int i = 0; i < n; ++i )
            YOJIMBO_FREE( sim.GetAllocator(), packetData[i] );
    }

    RestoreRandom();
}

void test_bit_array()
{
    const int Size = 300;

    BitArray bit_array( GetDefaultAllocator(), Size );

    // verify initial conditions

    check( bit_array.GetSize() == Size );

    for ( int i = 0; i < Size; ++i )
    {
        check( bit_array.GetBit(i) == 0 );
    }

    // set every third bit and verify correct bits are set on read

    for ( int i = 0; i < Size; ++i )
    {
        if ( ( i % 3 ) == 0 )
            bit_array.SetBit( i );
    }

    for ( int i = 0; i < Size; ++i )
    {
        if ( ( i % 3 ) == 0 )
        {
            check( bit_array.GetBit( i ) == 1 );
        }
        else
        {
            check( bit_array.GetBit( i ) == 0 );
        }
    }

    // now clear every third bit to zero and verify all bits are zero

    for ( int i = 0; i < Size; ++i )
    {
        if ( ( i % 3 ) == 0 )
            bit_array.ClearBit( i );
    }

    for ( int i = 0; i < Size; ++i )
    {
        check( bit_array.GetBit(i) == 0 );
    }

    // now set some more bits

    for ( int i = 0; i < Size; ++i )
    {
        if ( ( i % 10 ) == 0 )
            bit_array.SetBit( i );
    }

    for ( int i = 0; i < Size; ++i )
    {
        if ( ( i % 10 ) == 0 )
        {
            check( bit_array.GetBit( i ) == 1 );
        }
        else
        {
            check( bit_array.GetBit( i ) == 0 );
        }
    }

    // clear and verify all bits are zero

    bit_array.Clear();

    for ( int i = 0; i < Size; ++i )
    {
        check( bit_array.GetBit(i) == 0 );
    }
}

struct TestSequenceData
{
    TestSequenceData() : sequence(0xFFFF) {}
    explicit TestSequenceData( uint16_t _sequence ) : sequence( _sequence ) {}
    uint16_t sequence;
};

void test_sequence_buffer()
{
    const int Size = 256;

    SequenceBuffer<TestSequenceData> sequence_buffer( GetDefaultAllocator(), Size );

    for ( int i = 0; i < Size; ++i )
        check( sequence_buffer.Find(i) == NULL );

    for ( int i = 0; i <= Size*4; ++i )
    {
        TestSequenceData * entry = sequence_buffer.Insert( i );
        entry->sequence = i;
        check( sequence_buffer.GetSequence() == i + 1 );
    }

    for ( int i = 0; i <= Size; ++i )
    {
        TestSequenceData * entry = sequence_buffer.Insert( i );
        check( !entry );
    }

    int index = Size * 4;
    for ( int i = 0; i < Size; ++i )
    {
        TestSequenceData * entry = sequence_buffer.Find( index );
        check( entry );
        check( entry->sequence == uint32_t( index ) );
        index--;
    }

    for ( int i = 0; i <= Size; ++i )
    {
        const uint16_t sequence = uint16_t( Size * 4 + 1 + i );
        TestSequenceData * entry = sequence_buffer.Insert( sequence, true );
        check( entry );
        entry->sequence = sequence;
        check( sequence_buffer.GetSequence() == sequence + 1 );
    }

    sequence_buffer.Reset();

    check( sequence_buffer.GetSequence() == 0 );

    for ( int i = 0; i < Size; ++i )
        check( sequence_buffer.Find(i) == NULL );
}

void test_sequence_buffer_reinsert()
{
    const int Size = 256;

    SequenceBuffer<TestSequenceData> sequence_buffer( GetDefaultAllocator(), Size );

    for ( int i = 0; i < 3; ++i )
    {
        TestSequenceData * entry = sequence_buffer.Insert( i, true );
        check( entry );
        entry->sequence = i;
    }

    TestSequenceData * entry = sequence_buffer.Insert( 2, true );
    check( entry );
    entry->sequence = 2;

    check( sequence_buffer.Find(0) );
    check( sequence_buffer.Find(0)->sequence == 0 );
    check( sequence_buffer.Find(1) );
    check( sequence_buffer.Find(1)->sequence == 1 );
    check( sequence_buffer.Find(2) );
    check( sequence_buffer.Find(2)->sequence == 2 );

    check( sequence_buffer.GetSequence() == 3 );

    TestSequenceData * next = sequence_buffer.Insert( 3, true );
    check( next );
    next->sequence = 3;

    for ( int i = 0; i < 4; ++i )
    {
        check( sequence_buffer.Find(i) );
        check( sequence_buffer.Find(i)->sequence == i );
    }

    check( sequence_buffer.GetSequence() == 4 );

    TestSequenceData * older = sequence_buffer.Insert( 1, true );
    check( older );
    older->sequence = 1;

    for ( int i = 0; i < 4; ++i )
    {
        check( sequence_buffer.Find(i) );
        check( sequence_buffer.Find(i)->sequence == i );
    }

    check( sequence_buffer.GetSequence() == 4 );
}

template <typename T> class TestSequenceBuffer : public SequenceBuffer<T>
{
public:

    TestSequenceBuffer( Allocator & allocator, int size ) : SequenceBuffer<T>( allocator, size ) {}

    void RemoveEntries( int start_sequence, int finish_sequence )
    {
        SequenceBuffer<T>::RemoveEntries( start_sequence, finish_sequence );
    }
};

void test_sequence_buffer_remove_entries_across_wrap()
{
    const int Size = 4;

    TestSequenceBuffer<TestSequenceData> sequence_buffer( GetDefaultAllocator(), Size );

    const uint16_t sequences[] = { 65534, 65535, 0, 1 };

    for ( int i = 0; i < 4; ++i )
    {
        TestSequenceData * entry = sequence_buffer.Insert( sequences[i], true );
        check( entry );
        entry->sequence = sequences[i];
    }

    for ( int i = 0; i < 4; ++i )
        check( sequence_buffer.Find( sequences[i] ) != NULL );

    sequence_buffer.RemoveEntries( 65535, 0 );

    check( sequence_buffer.Available( 65535 ) );
    check( sequence_buffer.Available( 0 ) );
    check( sequence_buffer.Find( 65535 ) == NULL );
    check( sequence_buffer.Find( 0 ) == NULL );
    check( sequence_buffer.Find( 65534 ) != NULL );
    check( sequence_buffer.Find( 1 ) != NULL );
}

void test_allocator_tlsf()
{
    const int NumBlocks = 256;
    const int BlockSize = 1024;
    const int MemorySize = NumBlocks * BlockSize;

    uint8_t * memory = (uint8_t*) malloc( MemorySize );

    TLSF_Allocator allocator( memory, MemorySize );

    uint8_t * blockData[NumBlocks];
    memset( blockData, 0, sizeof( blockData ) );

    int stopIndex = 0;

    for ( int i = 0; i < NumBlocks; ++i )
    {
        blockData[i] = (uint8_t*) YOJIMBO_ALLOCATE( allocator, BlockSize );

        if ( !blockData[i] )
        {
            check( allocator.GetErrorLevel() == ALLOCATOR_ERROR_OUT_OF_MEMORY );
            allocator.ClearError();
            check( allocator.GetErrorLevel() == ALLOCATOR_ERROR_NONE );
            stopIndex = i;
            break;
        }

        check( blockData[i] );
        check( allocator.GetErrorLevel() == ALLOCATOR_ERROR_NONE );

        memset( blockData[i], i + 10, BlockSize );
    }

    check( stopIndex > NumBlocks / 2 );

    for ( int i = 0; i < NumBlocks - 1; ++i )
    {
        if ( blockData[i] )
        {
            for ( int j = 0; j < BlockSize; ++j )
                check( blockData[i][j] == uint8_t( i + 10 ) );
        }

        YOJIMBO_FREE( allocator, blockData[i] );
    }

    free( memory );
}

void PumpConnectionUpdate( ConnectionConfig & connectionConfig, double & time, Connection & sender, Connection & receiver, uint16_t & senderSequence, uint16_t & receiverSequence, float deltaTime = 0.1f, int packetLossPercent = 90 )
{
    uint8_t * packetData = (uint8_t*) alloca( connectionConfig.maxPacketSize );

    int packetBytes;
    if ( sender.GeneratePacket( NULL, senderSequence, packetData, connectionConfig.maxPacketSize, packetBytes ) )
    {
        if ( yojimbo_random_int( 0, 100 ) >= packetLossPercent )
        {
            if ( receiver.ProcessPacket( NULL, senderSequence, packetData, packetBytes ) )
            {
                sender.ProcessAcks( &senderSequence, 1 );
            }
        }
    }

    if ( receiver.GeneratePacket( NULL, receiverSequence, packetData, connectionConfig.maxPacketSize, packetBytes ) )
    {
        if ( yojimbo_random_int( 0, 100 ) >= packetLossPercent )
        {
            sender.ProcessPacket( NULL, receiverSequence, packetData, packetBytes );
            receiver.ProcessAcks( &receiverSequence, 1 );
        }
    }

    time += deltaTime;

    sender.AdvanceTime( time );
    receiver.AdvanceTime( time );

    senderSequence++;
    receiverSequence++;
}

const int ReliableChannel = 0;

void test_connection_reliable_ordered_messages()
{
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumMessagesSent = 64;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
        check( message );
        message->sequence = i;
        sender.SendMessage( ReliableChannel, message );
    }

    int numMessagesReceived = 0;

    const int NumIterations = 1000;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( ReliableChannel );
            if ( !message )
                break;

            check( message->GetId() == (int) numMessagesReceived );
            check( message->GetType() == TEST_MESSAGE );

            TestMessage * testMessage = (TestMessage*) message;

            check( testMessage->sequence == numMessagesReceived );

            ++numMessagesReceived;

            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_idle_channel_packet_sequence_wrap()
{
    // A reliable channel that has sent and had a message acked must keep working when the
    // connection packet sequence wraps all the way around while the channel is idle.
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
    check( message );
    message->sequence = 0;
    sender.SendMessage( ReliableChannel, message );

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    // Send the message and let the ack come back. The channel is now idle.
    PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

    check( !sender.HasMessagesToSend( ReliableChannel ) );

    Message * received = receiver.ReceiveMessage( ReliableChannel );
    check( received );
    messageFactory.ReleaseMessage( received );

    // Run the connection until the 16-bit packet sequence wraps back around to the sequence
    // the message was sent on, with the channel idle the whole time.
    const int NumPackets = 65536;
    for ( int i = 0; i < NumPackets; ++i )
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

    check( !sender.HasMessagesToSend( ReliableChannel ) );

    // The channel still delivers messages after the wrap.
    TestMessage * message2 = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
    check( message2 );
    message2->sequence = 1;
    sender.SendMessage( ReliableChannel, message2 );

    int numMessagesReceived = 0;

    for ( int i = 0; i < 100 && numMessagesReceived == 0; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

        while ( true )
        {
            Message * messageFromSender = receiver.ReceiveMessage( ReliableChannel );
            if ( !messageFromSender )
                break;
            check( messageFromSender->GetType() == TEST_MESSAGE );
            messageFactory.ReleaseMessage( messageFromSender );
            ++numMessagesReceived;
        }
    }

    check( !sender.HasMessagesToSend( ReliableChannel ) );
    check( numMessagesReceived == 1 );
}

void test_connection_reliable_ordered_blocks()
{
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
        check( message );
        message->sequence = i;
        const int blockSize = 1 + ( ( i * 901 ) % 3333 );
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        for ( int j = 0; j < blockSize; ++j )
            blockData[j] = i + j;
        message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMessage( ReliableChannel, message );
    }

    int numMessagesReceived = 0;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( ReliableChannel );
            if ( !message )
                break;

            check( message->GetId() == (int) numMessagesReceived );

            check( message->GetType() == TEST_BLOCK_MESSAGE );

            TestBlockMessage * blockMessage = (TestBlockMessage*) message;

            check( blockMessage->sequence == uint16_t( numMessagesReceived ) );

            const int blockSize = blockMessage->GetBlockSize();

            check( blockSize == 1 + ( ( numMessagesReceived * 901 ) % 3333 ) );

            const uint8_t * blockData = blockMessage->GetBlockData();

            check( blockData );

            for ( int j = 0; j < blockSize; ++j )
            {
                check( blockData[j] == uint8_t( numMessagesReceived + j ) );
            }

            ++numMessagesReceived;

            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_blocks_max_size()
{
    // Regression test: when maxBlockSize is not a multiple of blockFragmentSize, a
    // full-size block needs ceil(maxBlockSize/blockFragmentSize) fragments. The
    // send-side fragment buffers used to be sized with floor division, so sending a
    // maxBlockSize block overflowed them. Here 1100/500 => floor 2, ceil 3.
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.channel[ReliableChannel].maxBlockSize = 1100;
    connectionConfig.channel[ReliableChannel].blockFragmentSize = 500;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumMessagesSent = 8;
    const int BlockSize = connectionConfig.channel[ReliableChannel].maxBlockSize;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
        check( message );
        message->sequence = i;
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), BlockSize );
        for ( int j = 0; j < BlockSize; ++j )
            blockData[j] = uint8_t( i + j );
        message->AttachBlock( messageFactory.GetAllocator(), blockData, BlockSize );
        sender.SendMessage( ReliableChannel, message );
    }

    int numMessagesReceived = 0;
    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;
    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( ReliableChannel );
            if ( !message )
                break;

            check( message->GetId() == (int) numMessagesReceived );
            check( message->GetType() == TEST_BLOCK_MESSAGE );

            TestBlockMessage * blockMessage = (TestBlockMessage*) message;
            check( blockMessage->sequence == uint16_t( numMessagesReceived ) );

            const int blockSize = blockMessage->GetBlockSize();
            check( blockSize == BlockSize );

            const uint8_t * blockData = blockMessage->GetBlockData();
            check( blockData );
            for ( int j = 0; j < blockSize; ++j )
                check( blockData[j] == uint8_t( numMessagesReceived + j ) );

            ++numMessagesReceived;
            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_blocks_fragment_count()
{
    // Sends a large block whose size is one byte past a multiple of blockFragmentSize,
    // then checks that the whole block and its final byte arrive intact.
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.channel[ReliableChannel].maxBlockSize = 32 * 1024 * 1024;
    connectionConfig.channel[ReliableChannel].blockFragmentSize = 1024;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int BlockSize = 16 * 1024 * 1024 + 1;

    TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
    check( message );
    message->sequence = 0;
    uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), BlockSize );
    check( blockData );
    for ( int i = 0; i < BlockSize; ++i )
        blockData[i] = uint8_t( i * 31 );
    blockData[BlockSize - 1] = 0x5A;
    message->AttachBlock( messageFactory.GetAllocator(), blockData, BlockSize );
    sender.SendMessage( ReliableChannel, message );

    bool received = false;
    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;
    const int NumIterations = 40000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

        while ( true )
        {
            Message * msg = receiver.ReceiveMessage( ReliableChannel );
            if ( !msg )
                break;

            check( msg->GetType() == TEST_BLOCK_MESSAGE );

            TestBlockMessage * blockMessage = (TestBlockMessage*) msg;
            check( blockMessage->GetBlockSize() == BlockSize );

            const uint8_t * receivedData = blockMessage->GetBlockData();
            check( receivedData );
            for ( int j = 0; j < BlockSize - 1; ++j )
                check( receivedData[j] == uint8_t( j * 31 ) );
            check( receivedData[BlockSize - 1] == 0x5A );

            received = true;
            messageFactory.ReleaseMessage( msg );
        }

        if ( received )
            break;
    }

    check( received );
}

void test_connection_reliable_ordered_messages_and_blocks()
{
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        if ( rand() % 2 )
        {
            TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
            check( message );
            message->sequence = i;
            sender.SendMessage( ReliableChannel, message );
        }
        else
        {
            TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
            check( message );
            message->sequence = i;
            const int blockSize = 1 + ( ( i * 901 ) % 3333 );
            uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
            for ( int j = 0; j < blockSize; ++j )
                blockData[j] = i + j;
            message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
            sender.SendMessage( ReliableChannel, message );
        }
    }

    int numMessagesReceived = 0;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( ReliableChannel );
            if ( !message )
                break;

            check( message->GetId() == (int) numMessagesReceived );

            switch ( message->GetType() )
            {
                case TEST_MESSAGE:
                {
                    TestMessage * testMessage = (TestMessage*) message;

                    check( testMessage->sequence == uint16_t( numMessagesReceived ) );

                    ++numMessagesReceived;
                }
                break;

                case TEST_BLOCK_MESSAGE:
                {
                    TestBlockMessage * blockMessage = (TestBlockMessage*) message;

                    check( blockMessage->sequence == uint16_t( numMessagesReceived ) );

                    const int blockSize = blockMessage->GetBlockSize();

                    check( blockSize == 1 + ( ( numMessagesReceived * 901 ) % 3333 ) );

                    const uint8_t * blockData = blockMessage->GetBlockData();

                    check( blockData );

                    for ( int j = 0; j < blockSize; ++j )
                    {
                        check( blockData[j] == uint8_t( numMessagesReceived + j ) );
                    }

                    ++numMessagesReceived;
                }
                break;
            }

            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_messages_and_blocks_multiple_channels()
{
    const int NumChannels = 2;

    yojimbo_assert( NumChannels >= 0 );
    yojimbo_assert( NumChannels <= MaxChannels );

    double time = 100.0;

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = NumChannels;
    connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    connectionConfig.channel[0].maxMessagesPerPacket = 8;
    connectionConfig.channel[1].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    connectionConfig.channel[1].maxMessagesPerPacket = 8;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumMessagesSent = 32;

    for ( int channelIndex = 0; channelIndex < NumChannels; ++channelIndex )
    {
        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            if ( rand() % 2 )
            {
                TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
                check( message );
                message->sequence = i;
                sender.SendMessage( channelIndex, message );
            }
            else
            {
                TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
                check( message );
                message->sequence = i;
                const int blockSize = 1 + ( ( i * 901 ) % 3333 );
                uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
                for ( int j = 0; j < blockSize; ++j )
                    blockData[j] = i + j;
                message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
                sender.SendMessage( channelIndex, message );
            }
        }
    }

    const int NumIterations = 10000;

    int numMessagesReceived[NumChannels];
    memset( numMessagesReceived, 0, sizeof( numMessagesReceived ) );

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence );

        for ( int channelIndex = 0; channelIndex < NumChannels; ++channelIndex )
        {
            while ( true )
            {
                Message * message = receiver.ReceiveMessage( channelIndex );
                if ( !message )
                    break;

                check( message->GetId() == (int) numMessagesReceived[channelIndex] );

                switch ( message->GetType() )
                {
                    case TEST_MESSAGE:
                    {
                        TestMessage * testMessage = (TestMessage*) message;

                        check( testMessage->sequence == uint16_t( numMessagesReceived[channelIndex] ) );

                        ++numMessagesReceived[channelIndex];
                    }
                    break;

                    case TEST_BLOCK_MESSAGE:
                    {
                        TestBlockMessage * blockMessage = (TestBlockMessage*) message;

                        check( blockMessage->sequence == uint16_t( numMessagesReceived[channelIndex] ) );

                        const int blockSize = blockMessage->GetBlockSize();

                        check( blockSize == 1 + ( ( numMessagesReceived[channelIndex] * 901 ) % 3333 ) );

                        const uint8_t * blockData = blockMessage->GetBlockData();

                        check( blockData );

                        for ( int j = 0; j < blockSize; ++j )
                        {
                            check( blockData[j] == uint8_t( numMessagesReceived[channelIndex] + j ) );
                        }

                        ++numMessagesReceived[channelIndex];
                    }
                    break;
                }

                messageFactory.ReleaseMessage( message );
            }
        }

        bool receivedAllMessages = true;

        for ( int channelIndex = 0; channelIndex < NumChannels; ++channelIndex )
        {
            if ( numMessagesReceived[channelIndex] != NumMessagesSent )
            {
                receivedAllMessages = false;
                break;
            }
        }

        if ( receivedAllMessages )
            break;
    }

    for ( int channelIndex = 0; channelIndex < NumChannels; ++channelIndex )
    {
        check( numMessagesReceived[channelIndex] == NumMessagesSent );
    }
}

void test_connection_unreliable_unordered_messages()
{
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumIterations = 256;

    const int NumMessagesSent = 16;

    for ( int j = 0; j < NumMessagesSent; ++j )
    {
        TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
        check( message );
        message->sequence = j;
        sender.SendMessage( 0, message );
    }

    int numMessagesReceived = 0;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( 0 );
            if ( !message )
                break;

            check( message->GetType() == TEST_MESSAGE );

            TestMessage * testMessage = (TestMessage*) message;

            check( testMessage->sequence == uint16_t( numMessagesReceived ) );

            ++numMessagesReceived;

            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_unreliable_unordered_blocks()
{
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumIterations = 256;

    const int NumMessagesSent = 8;

    for ( int j = 0; j < NumMessagesSent; ++j )
    {
        TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
        check( message );
        message->sequence = j;
        const int blockSize = 1 + ( j * 7 );
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        for ( int k = 0; k < blockSize; ++k )
            blockData[k] = j + k;
        message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMessage( 0, message );
    }

    int numMessagesReceived = 0;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( 0 );
            if ( !message )
                break;

            check( message->GetType() == TEST_BLOCK_MESSAGE );

            TestBlockMessage * blockMessage = (TestBlockMessage*) message;

            check( blockMessage->sequence == uint16_t( numMessagesReceived ) );

            const int blockSize = blockMessage->GetBlockSize();

            check( blockSize == 1 + ( numMessagesReceived * 7 ) );

            const uint8_t * blockData = blockMessage->GetBlockData();

            check( blockData );

            for ( int j = 0; j < blockSize; ++j )
            {
                check( blockData[j] == uint8_t( numMessagesReceived + j ) );
            }

            ++numMessagesReceived;

            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reject_empty_packet()
{
    // A packet that is exactly a reliable header reaches Connection::ProcessPacket with
    // zero payload bytes. That must be rejected cleanly, not fed to the bit reader (which
    // used to trip the bufferSize > 0 assert in ReadPacket).

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    Connection connection( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    uint8_t buffer[1] = { 0 };

    check( !connection.ProcessPacket( NULL, 0, buffer, 0 ) );
    check( connection.GetErrorLevel() == CONNECTION_ERROR_READ_PACKET_FAILED );
}

void test_connection_process_packet_exact_allocation()
{
    // Production receive hands ProcessPacket an exact payload allocation.
    // BitReader needs 8 bytes of slack; ProcessPacket copies into packetBytes+8.
    TestMessageFactory messageFactory( GetDefaultAllocator() );
    double time = 100.0;
    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
    check( message );
    message->sequence = 0;
    sender.SendMessage( 0, message );

    uint8_t packet[2048];
    int packetBytes = 0;
    check( sender.GeneratePacket( NULL, 0, packet, (int) sizeof( packet ), packetBytes ) );
    check( packetBytes > 0 );

    uint8_t * exact = (uint8_t *) malloc( (size_t) packetBytes );
    check( exact );
    memcpy( exact, packet, (size_t) packetBytes );
    check( receiver.ProcessPacket( NULL, 0, exact, packetBytes ) );
    free( exact );

    Message * received = receiver.ReceiveMessage( 0 );
    check( received );
    check( received->GetType() == TEST_MESSAGE );
    messageFactory.ReleaseMessage( received );
}

void test_connection_reject_trailing_bytes()
{
    // serialize 2.0: ReadPacket ends the packet with serialize_end, so a valid packet
    // followed by a trailing byte is refused rather than silently accepted.
    TestMessageFactory messageFactory( GetDefaultAllocator() );
    double time = 100.0;
    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
    check( message );
    message->sequence = 0;
    sender.SendMessage( 0, message );

    uint8_t packet[2048];
    int packetBytes = 0;
    check( sender.GeneratePacket( NULL, 0, packet, (int) sizeof( packet ) - 8, packetBytes ) );
    check( packetBytes > 0 );

    packet[packetBytes] = 0;
    check( !receiver.ProcessPacket( NULL, 0, packet, packetBytes + 1 ) );
    check( receiver.GetErrorLevel() == CONNECTION_ERROR_READ_PACKET_FAILED );
    check( receiver.ReceiveMessage( 0 ) == NULL );
}

void test_connection_rejected_packet_is_not_acked()
{
    // A packet the receiver rejects is never added to its received packet set, so no ack
    // comes back for it and the sender must keep the message outstanding for retransmit.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    // Put the receiver into an error state so every subsequent packet is rejected.
    uint8_t buffer[1] = { 0 };
    check( !receiver.ProcessPacket( NULL, 0, buffer, 0 ) );
    check( receiver.GetErrorLevel() == CONNECTION_ERROR_READ_PACKET_FAILED );

    TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
    check( message );
    message->sequence = 0;
    sender.SendMessage( 0, message );

    check( sender.HasMessagesToSend( 0 ) );

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

    check( sender.HasMessagesToSend( 0 ) );
}

void test_connection_failed_message_body_not_partially_applied()
{
    // A packet can carry entries for several channels. If a later entry's message body
    // fails to deserialize, the whole packet is rejected: an earlier channel entry must
    // not enqueue its messages first.
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 2;
    connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    connectionConfig.channel[1].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    TestMessage * goodMessage = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
    check( goodMessage );
    goodMessage->sequence = 0;
    sender.SendMessage( 0, goodMessage );

    Message * failingMessage = messageFactory.CreateMessage( TEST_SERIALIZE_FAIL_ON_READ_MESSAGE );
    check( failingMessage );
    sender.SendMessage( 1, failingMessage );

    uint8_t packet[2048];
    int packetBytes = 0;
    check( sender.GeneratePacket( NULL, 0, packet, (int) sizeof( packet ), packetBytes ) );
    check( packetBytes > 0 );

    check( !receiver.ProcessPacket( NULL, 0, packet, packetBytes ) );

    // Both channel entries travelled in this one packet: channel 1's body failure must
    // reject the packet before channel 0's valid message is applied.
    check( receiver.GetChannelErrorLevel( 1 ) == CHANNEL_ERROR_FAILED_TO_SERIALIZE );
    check( receiver.ReceiveMessage( 0 ) == NULL );
    check( receiver.ReceiveMessage( 1 ) == NULL );
}

void test_connection_failed_unordered_message_body_not_partially_applied()
{
    // The same guarantee holds when the message body that fails to deserialize is carried
    // by an unreliable-unordered channel: the valid entry for the other channel must not
    // be applied from the rejected packet.
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 2;
    connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    connectionConfig.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    TestMessage * goodMessage = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
    check( goodMessage );
    goodMessage->sequence = 0;
    sender.SendMessage( 0, goodMessage );

    Message * failingMessage = messageFactory.CreateMessage( TEST_SERIALIZE_FAIL_ON_READ_MESSAGE );
    check( failingMessage );
    sender.SendMessage( 1, failingMessage );

    uint8_t packet[2048];
    int packetBytes = 0;
    check( sender.GeneratePacket( NULL, 0, packet, (int) sizeof( packet ), packetBytes ) );
    check( packetBytes > 0 );

    check( !receiver.ProcessPacket( NULL, 0, packet, packetBytes ) );

    check( receiver.GetChannelErrorLevel( 1 ) == CHANNEL_ERROR_FAILED_TO_SERIALIZE );
    check( receiver.ReceiveMessage( 0 ) == NULL );
    check( receiver.ReceiveMessage( 1 ) == NULL );
}

void test_connection_unreliable_rejects_block_fragment()
{
    // An unreliable-unordered channel never sends a top-level block fragment (its blocks
    // are serialized inline). A peer that puts a block fragment on that channel index used
    // to be misread through the ChannelPacketData union (a BlockMessage* reinterpreted as a
    // message-pointer array) and crash. It must instead be rejected as a serialize failure.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    // Sender speaks reliable-ordered on channel 0, so it emits a top-level block fragment.
    ConnectionConfig senderConfig;
    senderConfig.numChannels = 1;
    senderConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    // Receiver treats channel 0 as unreliable-unordered.
    ConnectionConfig receiverConfig;
    receiverConfig.numChannels = 1;
    receiverConfig.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    Connection sender( GetDefaultAllocator(), messageFactory, senderConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, receiverConfig, time );

    TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
    check( message );
    message->sequence = 0;
    const int blockSize = 64;
    uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
    for ( int i = 0; i < blockSize; ++i )
        blockData[i] = (uint8_t) i;
    message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
    sender.SendMessage( 0, message );

    // The sender is never acked, so it keeps re-sending the block fragment every tick.
    // Feed each generated packet to the receiver until its unreliable channel rejects the
    // fragment and the connection drops into the channel error state.
    uint8_t * packetData = (uint8_t*) alloca( senderConfig.maxPacketSize );
    uint16_t sequence = 0;
    bool sawChannelError = false;

    for ( int i = 0; i < 64 && !sawChannelError; ++i )
    {
        int packetBytes = 0;
        if ( sender.GeneratePacket( NULL, sequence, packetData, senderConfig.maxPacketSize, packetBytes ) && packetBytes > 0 )
            receiver.ProcessPacket( NULL, sequence, packetData, packetBytes );

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );
        time += 0.1;
        sequence++;

        if ( receiver.GetErrorLevel() == CONNECTION_ERROR_CHANNEL )
            sawChannelError = true;
    }

    check( sawChannelError );
}

void test_connection_reliable_block_fragment_on_disabled_blocks()
{
    // A peer sends a block fragment on a reliable-ordered channel, but the receiver has
    // blocks disabled on that channel. ChannelPacketData::Serialize reads blockMessage=1 then
    // returns early on disableBlocks, before SerializeBlockFragment initializes the block
    // pointers. The packet destructor's Free() must not then dereference an uninitialized
    // block.message (it used to, because Initialize() only zeroed part of the union).

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    // Sender: reliable-ordered with blocks enabled, so it emits a real block fragment.
    ConnectionConfig senderConfig;
    senderConfig.numChannels = 1;
    senderConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    // Receiver: reliable-ordered on the same channel, but with blocks disabled.
    ConnectionConfig receiverConfig;
    receiverConfig.numChannels = 1;
    receiverConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    receiverConfig.channel[0].disableBlocks = true;

    Connection sender( GetDefaultAllocator(), messageFactory, senderConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, receiverConfig, time );

    TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
    check( message );
    message->sequence = 0;
    const int blockSize = 2000;   // > fragment size, but even a single fragment reproduces it
    uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
    for ( int i = 0; i < blockSize; ++i )
        blockData[i] = (uint8_t) i;
    message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
    sender.SendMessage( 0, message );

    // Feed generated block-fragment packets to the receiver. Before the fix this crashed in
    // the packet destructor; after it, the read simply fails and the connection errors.
    uint8_t * packetData = (uint8_t*) alloca( senderConfig.maxPacketSize );
    uint16_t sequence = 0;

    for ( int i = 0; i < 64; ++i )
    {
        int packetBytes = 0;
        if ( sender.GeneratePacket( NULL, sequence, packetData, senderConfig.maxPacketSize, packetBytes ) && packetBytes > 0 )
            receiver.ProcessPacket( NULL, sequence, packetData, packetBytes );

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );
        time += 0.1;
        sequence++;

        if ( receiver.GetErrorLevel() != CONNECTION_ERROR_NONE )
            break;
    }

    // The receiver rejected the disallowed block fragment without crashing.
    check( receiver.GetErrorLevel() != CONNECTION_ERROR_NONE );
}

// Hand-serialize a connection packet carrying a single reliable-ordered channel entry that is
// a block fragment. Matches ConnectionPacket + ChannelPacketData::Serialize + SerializeBlockFragment
// for a numChannels==1 config. Lets tests inject fragment fields a well-behaved sender never would.
template <typename Stream> static bool SerializeRawBlockFragmentPacket( Stream & stream, int maxFragmentsPerBlock, int blockFragmentSize, int numFragments, int fragmentId, int fragmentSize, uint16_t messageId = 0 )
{
    int numChannelEntries = 1;
    serialize_int( stream, numChannelEntries, 0, 1 );   // numChannels == 1, so channelIndex is not serialized

    bool blockMessage = true;
    serialize_bool( stream, blockMessage );

    serialize_bits( stream, messageId, 16 );

    if ( maxFragmentsPerBlock > 1 )
        serialize_int( stream, numFragments, 1, maxFragmentsPerBlock );

    if ( numFragments > 1 )
        serialize_int( stream, fragmentId, 0, numFragments - 1 );

    serialize_int( stream, fragmentSize, 1, blockFragmentSize );

    uint8_t payload[2048];
    yojimbo_assert( fragmentSize <= (int) sizeof( payload ) );
    memset( payload, 0xAB, sizeof( payload ) );
    serialize_bytes( stream, payload, fragmentSize );

    // fragmentId != 0 here, so no message type / block message is serialized.
    return true;
}

// Hand-serialize a connection packet carrying fragment 0 of a reliable-ordered block, including
// the block message type and body. Lets a test choose the block's message id, which a conforming
// sender assigns from its own send sequence.
template <typename Stream> static bool SerializeRawBlockFragment0Packet( Stream & stream, int maxFragmentsPerBlock, int blockFragmentSize, int numFragments, uint16_t messageId, int fragmentSize, int maxMessageType, int messageType, uint16_t sequence )
{
    int numChannelEntries = 1;
    serialize_int( stream, numChannelEntries, 0, 1 );   // numChannels == 1, so channelIndex is not serialized

    bool blockMessage = true;
    serialize_bool( stream, blockMessage );

    serialize_bits( stream, messageId, 16 );

    if ( maxFragmentsPerBlock > 1 )
        serialize_int( stream, numFragments, 1, maxFragmentsPerBlock );

    int fragmentId = 0;
    if ( numFragments > 1 )
        serialize_int( stream, fragmentId, 0, numFragments - 1 );

    serialize_int( stream, fragmentSize, 1, blockFragmentSize );

    uint8_t payload[2048];
    yojimbo_assert( fragmentSize <= (int) sizeof( payload ) );
    memset( payload, 0xAB, sizeof( payload ) );
    serialize_bytes( stream, payload, fragmentSize );

    // fragmentId == 0 here, so the message type and block message body follow.
    serialize_int( stream, messageType, 0, maxMessageType );

    // TestBlockMessage::Serialize writes a 16-bit sequence.
    serialize_bits( stream, sequence, 16 );

    return true;
}

// Hand-serialize a connection packet carrying a single reliable-ordered ordinary message with a
// chosen message id. Uses TestMessage with the given sequence (16-bit sequence plus dummy bits).
template <typename Stream> static bool SerializeRawOrderedMessagePacket( Stream & stream, int maxMessagesPerPacket, int maxMessageType, uint16_t messageId, uint16_t sequence )
{
    int numChannelEntries = 1;
    serialize_int( stream, numChannelEntries, 0, 1 );   // numChannels == 1, so channelIndex is not serialized

    bool blockMessage = false;
    serialize_bool( stream, blockMessage );

    bool hasMessages = true;
    serialize_bool( stream, hasMessages );

    int numMessages = 1;
    serialize_int( stream, numMessages, 1, maxMessagesPerPacket );

    serialize_bits( stream, messageId, 16 );

    int messageType = TEST_MESSAGE;
    serialize_int( stream, messageType, 0, maxMessageType );

    serialize_bits( stream, sequence, 16 );

    const int numBits = GetNumBitsForMessage( sequence );
    const int numWords = numBits / 32;
    uint32_t dummy = 0;
    for ( int i = 0; i < numWords; ++i )
        serialize_bits( stream, dummy, 32 );
    const int numRemainderBits = numBits - numWords * 32;
    if ( numRemainderBits > 0 )
        serialize_bits( stream, dummy, numRemainderBits );

    return true;
}

// Hand-serialize a connection packet carrying a single reliable-ordered channel entry whose
// ordinary (non-fragment) message list names a block message type. A conforming reliable-ordered
// sender only ever carries block messages on the fragment path.
template <typename Stream> static bool SerializeRawBlockMessageListPacket( Stream & stream, int maxMessagesPerPacket, int maxMessageType, uint16_t messageId, uint16_t sequence )
{
    int numChannelEntries = 1;
    serialize_int( stream, numChannelEntries, 0, 1 );   // numChannels == 1, so channelIndex is not serialized

    bool blockMessage = false;
    serialize_bool( stream, blockMessage );

    bool hasMessages = true;
    serialize_bool( stream, hasMessages );

    int numMessages = 1;
    serialize_int( stream, numMessages, 1, maxMessagesPerPacket );

    serialize_bits( stream, messageId, 16 );

    int messageType = TEST_BLOCK_MESSAGE;
    serialize_int( stream, messageType, 0, maxMessageType );

    // TestBlockMessage::Serialize writes a 16-bit sequence and no block payload.
    serialize_bits( stream, sequence, 16 );

    return true;
}

void test_connection_reliable_rejects_block_in_message_list()
{
    // A reliable-ordered block message is carried by the fragment path only. A message list
    // that names a block type is not something a conforming sender produces; the receiver
    // must reject the packet rather than deliver a block message with no attached block.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

    uint8_t buffer[4096];
    memset( buffer, 0, sizeof( buffer ) );
    WriteStream stream( buffer, sizeof( buffer ) );
    check( SerializeRawBlockMessageListPacket( stream,
                                               config.channel[0].maxMessagesPerPacket,
                                               messageFactory.GetNumTypes() - 1,
                                               0,
                                               0 ) );
    stream.Flush();
    const int packetBytes = stream.GetBytesProcessed();
    check( packetBytes > 0 );

    const bool processed = receiver.ProcessPacket( NULL, 0, buffer, packetBytes );
    check( !processed );

    Message * message = receiver.ReceiveMessage( 0 );
    if ( message )
        messageFactory.ReleaseMessage( message );
    check( message == NULL );

    check( receiver.GetErrorLevel() != CONNECTION_ERROR_NONE );
}

struct TestSizedMessage : public Message
{
    int numBytes;
    uint8_t data[8192];

    TestSizedMessage() : numBytes( 0 )
    {
        memset( data, 0, sizeof( data ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, numBytes, 0, (int) sizeof( data ) );
        serialize_bytes( stream, data, numBytes );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

struct TestBodyBlockMessage : public BlockMessage
{
    uint16_t sequence;
    uint8_t body[64];

    TestBodyBlockMessage() : sequence( 0 )
    {
        memset( body, 0, sizeof( body ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bits( stream, sequence, 16 );
        serialize_bytes( stream, body, (int) sizeof( body ) );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

struct TestLargeBodyBlockMessage : public BlockMessage
{
    uint8_t body[8000];

    TestLargeBodyBlockMessage()
    {
        memset( body, 0, sizeof( body ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bytes( stream, body, (int) sizeof( body ) );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

enum TestFragmentFitMessageType
{
    TEST_FRAGMENT_FIT_SIZED_MESSAGE,
    TEST_FRAGMENT_FIT_BODY_BLOCK_MESSAGE,
    TEST_FRAGMENT_FIT_LARGE_BODY_BLOCK_MESSAGE,
    NUM_TEST_FRAGMENT_FIT_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START( TestFragmentFitMessageFactory, NUM_TEST_FRAGMENT_FIT_MESSAGE_TYPES );
YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_FRAGMENT_FIT_SIZED_MESSAGE, TestSizedMessage );
YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_FRAGMENT_FIT_BODY_BLOCK_MESSAGE, TestBodyBlockMessage );
YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_FRAGMENT_FIT_LARGE_BODY_BLOCK_MESSAGE, TestLargeBodyBlockMessage );
YOJIMBO_MESSAGE_FACTORY_FINISH();

void test_connection_reliable_ordered_block_fragment_fit_multiple_channels()
{
    double time = 1000.0;

    ConnectionConfig config;
    config.numChannels = 2;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[1].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    for ( int padBytes = 7090; padBytes <= 7160; ++padBytes )
    {
        TestFragmentFitMessageFactory messageFactory( GetDefaultAllocator() );
        Connection sender( GetDefaultAllocator(), messageFactory, config, time );

        TestSizedMessage * msg = (TestSizedMessage*) messageFactory.CreateMessage( TEST_FRAGMENT_FIT_SIZED_MESSAGE );
        check( msg );
        msg->numBytes = padBytes;
        memset( msg->data, 0x11, (size_t) padBytes );
        sender.SendMessage( 0, msg );

        TestBodyBlockMessage * block = (TestBodyBlockMessage*) messageFactory.CreateMessage( TEST_FRAGMENT_FIT_BODY_BLOCK_MESSAGE );
        check( block );
        block->sequence = 7;
        memset( block->body, 0x33, sizeof( block->body ) );
        const int blockSize = 2000;
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        check( blockData );
        memset( blockData, 0x22, blockSize );
        block->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMessage( 1, block );

        const int guardBytes = 64;
        uint8_t * buffer = (uint8_t*) malloc( (size_t) config.maxPacketSize + guardBytes );
        check( buffer );
        memset( buffer, 0xCD, (size_t) config.maxPacketSize + guardBytes );

        int packetBytes = 0;
        check( sender.GeneratePacket( NULL, 0, buffer, config.maxPacketSize, packetBytes ) );
        check( packetBytes > 0 );
        check( packetBytes <= config.maxPacketSize );
        for ( int g = 0; g < guardBytes; ++g )
            check( buffer[config.maxPacketSize + g] == 0xCD );

        free( buffer );
    }

    {
        TestFragmentFitMessageFactory messageFactory( GetDefaultAllocator() );
        Connection sender( GetDefaultAllocator(), messageFactory, config, time );
        Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

        TestSizedMessage * msg = (TestSizedMessage*) messageFactory.CreateMessage( TEST_FRAGMENT_FIT_SIZED_MESSAGE );
        check( msg );
        msg->numBytes = 7120;
        for ( int i = 0; i < msg->numBytes; ++i )
            msg->data[i] = (uint8_t) ( i & 0xFF );
        sender.SendMessage( 0, msg );

        TestBodyBlockMessage * block = (TestBodyBlockMessage*) messageFactory.CreateMessage( TEST_FRAGMENT_FIT_BODY_BLOCK_MESSAGE );
        check( block );
        block->sequence = 42;
        for ( int i = 0; i < (int) sizeof( block->body ); ++i )
            block->body[i] = (uint8_t) ( 0xA0 + i );
        const int blockSize = 2000;
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        check( blockData );
        for ( int i = 0; i < blockSize; ++i )
            blockData[i] = (uint8_t) ( i * 3 );
        block->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMessage( 1, block );

        uint8_t * packetData = (uint8_t*) malloc( (size_t) config.maxPacketSize );
        check( packetData );
        uint16_t sequence = 0;
        bool gotUnreliable = false;
        bool gotBlock = false;

        for ( int i = 0; i < 4 && ( !gotUnreliable || !gotBlock ); ++i )
        {
            int packetBytes = 0;
            check( sender.GeneratePacket( NULL, sequence, packetData, config.maxPacketSize, packetBytes ) );
            check( packetBytes > 0 );
            check( packetBytes <= config.maxPacketSize );
            check( receiver.ProcessPacket( NULL, sequence, packetData, packetBytes ) );
            sender.ProcessAcks( &sequence, 1 );
            sequence++;

            Message * m0 = receiver.ReceiveMessage( 0 );
            if ( m0 )
            {
                check( m0->GetType() == TEST_FRAGMENT_FIT_SIZED_MESSAGE );
                TestSizedMessage * sm = (TestSizedMessage*) m0;
                check( sm->numBytes == 7120 );
                gotUnreliable = true;
                messageFactory.ReleaseMessage( m0 );
            }

            Message * m1 = receiver.ReceiveMessage( 1 );
            if ( m1 )
            {
                check( m1->GetType() == TEST_FRAGMENT_FIT_BODY_BLOCK_MESSAGE );
                TestBodyBlockMessage * bm = (TestBodyBlockMessage*) m1;
                check( bm->sequence == 42 );
                for ( int j = 0; j < (int) sizeof( bm->body ); ++j )
                    check( bm->body[j] == (uint8_t) ( 0xA0 + j ) );
                check( bm->GetBlockSize() == blockSize );
                for ( int j = 0; j < blockSize; ++j )
                    check( bm->GetBlockData()[j] == (uint8_t) ( j * 3 ) );
                gotBlock = true;
                messageFactory.ReleaseMessage( m1 );
            }
        }

        check( gotUnreliable );
        check( gotBlock );
        free( packetData );
    }

    {
        TestFragmentFitMessageFactory messageFactory( GetDefaultAllocator() );
        Connection sender( GetDefaultAllocator(), messageFactory, config, time );

        TestLargeBodyBlockMessage * largeBlock = (TestLargeBodyBlockMessage*) messageFactory.CreateMessage( TEST_FRAGMENT_FIT_LARGE_BODY_BLOCK_MESSAGE );
        check( largeBlock );
        const int blockSize = 1024;
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        check( blockData );
        memset( blockData, 0x55, blockSize );
        largeBlock->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMessage( 1, largeBlock );
        check( sender.GetChannelErrorLevel( 1 ) == CHANNEL_ERROR_MESSAGE_TOO_LARGE );
    }
}

#ifdef YOJIMBO_DEBUG
#include <setjmp.h>
static jmp_buf g_configValidateJmpBuf;
static int g_configValidateAssertCount = 0;

static void ExpectConfigValidateAssertHandler( const char * /*condition*/, const char * /*function*/, const char * /*file*/, int /*line*/ )
{
    g_configValidateAssertCount++;
    longjmp( g_configValidateJmpBuf, 1 );
}
#endif // #ifdef YOJIMBO_DEBUG

void test_connection_reliable_ordered_block_fragment_header_headroom()
{
    double time = 1000.0;

#ifdef YOJIMBO_DEBUG
    {
        void (*prevAssertHandler)( const char *, const char *, const char *, int ) = yojimbo_assert_function;
        yojimbo_set_assert_function( ExpectConfigValidateAssertHandler );

        for ( int fragmentSize = 1186; fragmentSize <= 1200; ++fragmentSize )
        {
            ConnectionConfig invalidConfig;
            invalidConfig.numChannels = 1;
            invalidConfig.maxPacketSize = 1200;
            invalidConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            invalidConfig.channel[0].maxBlockSize = 4800;
            invalidConfig.channel[0].blockFragmentSize = fragmentSize;

            g_configValidateAssertCount = 0;
            if ( setjmp( g_configValidateJmpBuf ) == 0 )
                invalidConfig.Validate();
            check( g_configValidateAssertCount == 1 );
        }

        {
            ConnectionConfig edgeConfig;
            edgeConfig.numChannels = 1;
            edgeConfig.maxPacketSize = 1200;
            edgeConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            edgeConfig.channel[0].maxBlockSize = 4800;
            edgeConfig.channel[0].blockFragmentSize = 1185;

            g_configValidateAssertCount = 0;
            if ( setjmp( g_configValidateJmpBuf ) == 0 )
                edgeConfig.Validate();
            check( g_configValidateAssertCount == 0 );
        }

        {
            ConnectionConfig unalignedConfig;
            unalignedConfig.numChannels = 1;
            unalignedConfig.maxPacketSize = 1205;
            unalignedConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            unalignedConfig.channel[0].maxBlockSize = 4800;
            unalignedConfig.channel[0].blockFragmentSize = 1186;

            g_configValidateAssertCount = 0;
            if ( setjmp( g_configValidateJmpBuf ) == 0 )
                unalignedConfig.Validate();
            check( g_configValidateAssertCount == 1 );
        }

        yojimbo_set_assert_function( prevAssertHandler );
    }
#endif // #ifdef YOJIMBO_DEBUG

    for ( int fragmentSize = 1186; fragmentSize <= 1200; ++fragmentSize )
    {
        TestMessageFactory messageFactory( GetDefaultAllocator() );
        ConnectionConfig config;
        config.numChannels = 1;
        config.maxPacketSize = 1200;
        config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
        config.channel[0].maxBlockSize = 4800;
        config.channel[0].blockFragmentSize = fragmentSize;

        Connection sender( GetDefaultAllocator(), messageFactory, config, time );
        TestBlockMessage * block = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
        check( block );
        const int blockSize = 4000;
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        check( blockData );
        memset( blockData, 0x22, blockSize );
        block->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMessage( 0, block );
        check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_MESSAGE_TOO_LARGE );

        uint8_t * buffer = (uint8_t*) malloc( (size_t) config.maxPacketSize );
        check( buffer );
        int packetBytes = 0;
        check( sender.GeneratePacket( NULL, 0, buffer, config.maxPacketSize, packetBytes ) );
        check( packetBytes <= config.maxPacketSize );
        free( buffer );
    }

    {
        TestMessageFactory messageFactory( GetDefaultAllocator() );
        ConnectionConfig config;
        config.numChannels = 1;
        config.maxPacketSize = 1200;
        config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
        config.channel[0].maxBlockSize = 4800;
        config.channel[0].blockFragmentSize = 1182;
        config.Validate();

        Connection sender( GetDefaultAllocator(), messageFactory, config, time );
        Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

        TestBlockMessage * block = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
        check( block );
        block->sequence = 99;
        const int blockSize = 4000;
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        check( blockData );
        for ( int i = 0; i < blockSize; ++i )
            blockData[i] = (uint8_t) ( i & 0xFF );
        block->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMessage( 0, block );
        check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );

        uint8_t * buffer = (uint8_t*) malloc( (size_t) config.maxPacketSize );
        check( buffer );
        uint16_t sequence = 0;
        bool received = false;

        for ( int i = 0; i < 8 && !received; ++i )
        {
            int packetBytes = 0;
            check( sender.GeneratePacket( NULL, sequence, buffer, config.maxPacketSize, packetBytes ) );
            check( packetBytes > 0 );
            check( packetBytes <= config.maxPacketSize );
            check( receiver.ProcessPacket( NULL, sequence, buffer, packetBytes ) );
            sender.ProcessAcks( &sequence, 1 );
            sequence++;

            Message * m = receiver.ReceiveMessage( 0 );
            if ( m )
            {
                check( m->GetType() == TEST_BLOCK_MESSAGE );
                TestBlockMessage * bm = (TestBlockMessage*) m;
                check( bm->sequence == 99 );
                check( bm->GetBlockSize() == blockSize );
                for ( int j = 0; j < blockSize; ++j )
                    check( bm->GetBlockData()[j] == (uint8_t) ( j & 0xFF ) );
                received = true;
                messageFactory.ReleaseMessage( m );
            }
        }

        check( received );
        free( buffer );
    }
}

void test_client_server_config_packet_fragment_size_fits_netcode_payload()
{
#ifdef YOJIMBO_DEBUG
    void (*prevAssertHandler)( const char *, const char *, const char *, int ) = yojimbo_assert_function;
    yojimbo_set_assert_function( ExpectConfigValidateAssertHandler );

    // A fragment is sent as fragment header (5 bytes) + packet header (up to 9 bytes) + packetFragmentSize
    // bytes, and must fit within the 1200 byte netcode payload. The largest valid packetFragmentSize is 1186.

    const int invalidFragmentSizes[] = { 1187, 1200, 4096 };
    for ( int i = 0; i < (int) ( sizeof( invalidFragmentSizes ) / sizeof( invalidFragmentSizes[0] ) ); ++i )
    {
        ClientServerConfig invalidConfig;
        invalidConfig.packetFragmentSize = invalidFragmentSizes[i];

        g_configValidateAssertCount = 0;
        if ( setjmp( g_configValidateJmpBuf ) == 0 )
            invalidConfig.Validate();
        check( g_configValidateAssertCount == 1 );
    }

    {
        ClientServerConfig edgeConfig;
        edgeConfig.packetFragmentSize = 1186;

        g_configValidateAssertCount = 0;
        if ( setjmp( g_configValidateJmpBuf ) == 0 )
            edgeConfig.Validate();
        check( g_configValidateAssertCount == 0 );
    }

    yojimbo_set_assert_function( prevAssertHandler );
#endif // #ifdef YOJIMBO_DEBUG
}

void test_connection_reliable_ordered_block_ignores_packet_budget()
{
    double time = 1000.0;

    TestMessageFactory messageFactory( GetDefaultAllocator() );
    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].packetBudget = 512;
    config.Validate();

    Connection sender( GetDefaultAllocator(), messageFactory, config, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

    TestBlockMessage * block = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
    check( block );
    block->sequence = 7;
    const int blockSize = 2000;
    uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
    check( blockData );
    for ( int i = 0; i < blockSize; ++i )
        blockData[i] = (uint8_t) ( i * 5 );
    block->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
    sender.SendMessage( 0, block );
    check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );

    uint8_t * buffer = (uint8_t*) malloc( (size_t) config.maxPacketSize );
    check( buffer );
    uint16_t sequence = 0;
    bool received = false;

    for ( int i = 0; i < 8 && !received; ++i )
    {
        int packetBytes = 0;
        check( sender.GeneratePacket( NULL, sequence, buffer, config.maxPacketSize, packetBytes ) );
        check( packetBytes > 0 );
        check( packetBytes <= config.maxPacketSize );
        check( receiver.ProcessPacket( NULL, sequence, buffer, packetBytes ) );
        sender.ProcessAcks( &sequence, 1 );
        sequence++;

        Message * m = receiver.ReceiveMessage( 0 );
        if ( m )
        {
            check( m->GetType() == TEST_BLOCK_MESSAGE );
            TestBlockMessage * bm = (TestBlockMessage*) m;
            check( bm->sequence == 7 );
            check( bm->GetBlockSize() == blockSize );
            for ( int j = 0; j < blockSize; ++j )
                check( bm->GetBlockData()[j] == (uint8_t) ( j * 5 ) );
            received = true;
            messageFactory.ReleaseMessage( m );
        }
    }

    check( received );
    check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );
    check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );
    free( buffer );
}

template <int Size>
struct TestFixedBodyMessage : public Message
{
    uint8_t body[Size];

    TestFixedBodyMessage()
    {
        memset( body, 0, sizeof( body ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        // Serialized as raw 8-bit fields so the measured body is exactly Size bytes,
        // with no integer prefix or alignment bits of its own.
        for ( int i = 0; i < Size; ++i )
            serialize_bits( stream, body[i], 8 );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

enum TestMessageBudgetType
{
    TEST_MESSAGE_BUDGET_FULL,
    TEST_MESSAGE_BUDGET_FIT,
    NUM_TEST_MESSAGE_BUDGET_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START( TestMessageBudgetFactory, NUM_TEST_MESSAGE_BUDGET_TYPES );
YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_MESSAGE_BUDGET_FULL, TestFixedBodyMessage<70> );
YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_MESSAGE_BUDGET_FIT, TestFixedBodyMessage<50> );
YOJIMBO_MESSAGE_FACTORY_FINISH();

void test_connection_reliable_ordered_message_packet_budget_fit()
{
    double time = 1000.0;

    TestMessageBudgetFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].packetBudget = 70;
    config.Validate();

    {
        Connection sender( GetDefaultAllocator(), messageFactory, config, time );

        TestFixedBodyMessage<70> * message = (TestFixedBodyMessage<70>*) messageFactory.CreateMessage( TEST_MESSAGE_BUDGET_FULL );
        check( message );
        sender.SendMessage( 0, message );
        check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_MESSAGE_TOO_LARGE );
    }

    {
        Connection sender( GetDefaultAllocator(), messageFactory, config, time );
        Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

        TestFixedBodyMessage<50> * message = (TestFixedBodyMessage<50>*) messageFactory.CreateMessage( TEST_MESSAGE_BUDGET_FIT );
        check( message );
        for ( int i = 0; i < 50; ++i )
            message->body[i] = (uint8_t) ( i + 1 );
        sender.SendMessage( 0, message );
        check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );

        uint16_t senderSequence = 0;
        uint16_t receiverSequence = 0;
        bool received = false;

        for ( int i = 0; i < 10 && !received; ++i )
        {
            PumpConnectionUpdate( config, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

            Message * receivedMessage = receiver.ReceiveMessage( 0 );
            if ( receivedMessage )
            {
                check( receivedMessage->GetType() == TEST_MESSAGE_BUDGET_FIT );
                TestFixedBodyMessage<50> * fitMessage = (TestFixedBodyMessage<50>*) receivedMessage;
                for ( int j = 0; j < 50; ++j )
                    check( fitMessage->body[j] == (uint8_t) ( j + 1 ) );
                messageFactory.ReleaseMessage( receivedMessage );
                received = true;
            }
        }

        check( received );
        check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );
        check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );
    }
}

void test_connection_unreliable_unordered_message_packet_budget_fit()
{
    double time = 1000.0;

    TestMessageBudgetFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[0].disableBlocks = true;
    config.channel[0].packetBudget = 70;
    config.Validate();

    {
        Connection sender( GetDefaultAllocator(), messageFactory, config, time );

        TestFixedBodyMessage<70> * message = (TestFixedBodyMessage<70>*) messageFactory.CreateMessage( TEST_MESSAGE_BUDGET_FULL );
        check( message );
        sender.SendMessage( 0, message );
        check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_MESSAGE_TOO_LARGE );
    }

    {
        Connection sender( GetDefaultAllocator(), messageFactory, config, time );
        Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

        TestFixedBodyMessage<50> * message = (TestFixedBodyMessage<50>*) messageFactory.CreateMessage( TEST_MESSAGE_BUDGET_FIT );
        check( message );
        for ( int i = 0; i < 50; ++i )
            message->body[i] = (uint8_t) ( 0xA0 + i );
        sender.SendMessage( 0, message );
        check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );

        uint16_t senderSequence = 0;
        uint16_t receiverSequence = 0;
        bool received = false;

        for ( int i = 0; i < 10 && !received; ++i )
        {
            PumpConnectionUpdate( config, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

            Message * receivedMessage = receiver.ReceiveMessage( 0 );
            if ( receivedMessage )
            {
                check( receivedMessage->GetType() == TEST_MESSAGE_BUDGET_FIT );
                TestFixedBodyMessage<50> * fitMessage = (TestFixedBodyMessage<50>*) receivedMessage;
                for ( int j = 0; j < 50; ++j )
                    check( fitMessage->body[j] == (uint8_t) ( 0xA0 + j ) );
                messageFactory.ReleaseMessage( receivedMessage );
                received = true;
            }
        }

        check( received );
        check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );
        check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );
    }
}

void test_connection_unreliable_send_queue_drains_when_packet_full()
{
    // An unreliable message can never be resent, so when a packet has no room left for it
    // (here channel 0 reliably fills nearly the whole packet every tick) it must be dropped
    // immediately instead of accumulating in the send queue across ticks. This sends more
    // messages than the unreliable channel's send queue can hold, one per tick, and checks
    // that the queue never fills and the channel never reports CHANNEL_ERROR_SEND_QUEUE_FULL.
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig config;
    config.numChannels = 2;
    config.maxPacketSize = 88;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].disableBlocks = true;
    config.channel[0].messageResendTime = 0.0f;   // keep channel 0 saturating the packet every tick
    config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[1].disableBlocks = true;
    config.channel[1].messageSendQueueSize = 8;
    config.channel[1].messageReceiveQueueSize = 8;
    config.Validate();

    Connection sender( GetDefaultAllocator(), messageFactory, config, time );

    // One reliable message with sequence 16 measures 512 body bits, which leaves channel 1
    // less than ConservativeMessageHeaderBits of room once channel 0 has claimed the packet.
    TestMessage * reliableMessage = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
    check( reliableMessage );
    reliableMessage->sequence = 16;
    sender.SendMessage( 0, reliableMessage );
    check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );

    uint8_t packetData[256];
    int packetBytes = 0;

    const int NumMessagesSent = config.channel[1].messageSendQueueSize * 4;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        check( sender.GetChannelErrorLevel( 1 ) == CHANNEL_ERROR_NONE );
        check( sender.CanSendMessage( 1 ) );

        TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
        check( message );
        message->sequence = 0;
        sender.SendMessage( 1, message );

        sender.GeneratePacket( NULL, (uint16_t) i, packetData, config.maxPacketSize, packetBytes );

        time += 0.1;
        sender.AdvanceTime( time );
    }

    check( sender.GetChannelErrorLevel( 1 ) == CHANNEL_ERROR_NONE );
}

void test_channel_config_fragment_counts_without_overflow()
{
    // yojimbo#347: ceil(maxBlockSize / blockFragmentSize) used to add in int
    // and overflow for a legal huge maxBlockSize. The 65535 check then saw
    // garbage. The add is 64-bit; a too-large result stays well-defined and
    // still fails Validate's cap.
    ChannelConfig channel;
    channel.maxBlockSize = 2147483647;
    channel.blockFragmentSize = 1024;
    const int n = channel.GetMaxFragmentsPerBlock();
    check( n == (int) ( ( (int64_t) 2147483647 + 1023 ) / 1024 ) );
    check( n > 65535 );

    // yojimbo#346: fragmentSize on the wire is 16 bits.
    ChannelConfig smallChannel;
    smallChannel.maxBlockSize = 256 * 1024;
    smallChannel.blockFragmentSize = 1024;
    check( smallChannel.GetMaxFragmentsPerBlock() == 256 );
    check( smallChannel.blockFragmentSize <= 65535 );
}

void test_channel_config_max_messages_per_packet_bounds()
{
#ifdef YOJIMBO_DEBUG
    void (*prevAssertHandler)( const char *, const char *, const char *, int ) = yojimbo_assert_function;
    yojimbo_set_assert_function( ExpectConfigValidateAssertHandler );

    // maxMessagesPerPacket is bounded by the 16-bit SentPacketEntry::numMessageIds field.
    const int invalidMaxMessagesPerPacket[] = { 0, -1, 65536, 2147483647 };
    for ( int i = 0; i < (int) ( sizeof( invalidMaxMessagesPerPacket ) / sizeof( invalidMaxMessagesPerPacket[0] ) ); ++i )
    {
        ChannelConfig channel;
        channel.maxMessagesPerPacket = invalidMaxMessagesPerPacket[i];
        g_configValidateAssertCount = 0;
        if ( setjmp( g_configValidateJmpBuf ) == 0 )
            channel.Validate( 0, 8192 );
        check( g_configValidateAssertCount == 1 );
    }

    // maxMessagesPerPacket * sentPacketBufferSize is used as an int index, so it must fit in int.
    {
        ChannelConfig channel;
        channel.maxMessagesPerPacket = 65535;
        channel.sentPacketBufferSize = 65536;
        g_configValidateAssertCount = 0;
        if ( setjmp( g_configValidateJmpBuf ) == 0 )
            channel.Validate( 0, 8192 );
        check( g_configValidateAssertCount == 1 );
    }

    const int validMaxMessagesPerPacket[] = { 1, 65535 };
    for ( int i = 0; i < (int) ( sizeof( validMaxMessagesPerPacket ) / sizeof( validMaxMessagesPerPacket[0] ) ); ++i )
    {
        ChannelConfig channel;
        channel.maxMessagesPerPacket = validMaxMessagesPerPacket[i];
        g_configValidateAssertCount = 0;
        if ( setjmp( g_configValidateJmpBuf ) == 0 )
            channel.Validate( 0, 8192 );
        check( g_configValidateAssertCount == 0 );
    }

    {
        ChannelConfig channel;
        channel.maxMessagesPerPacket = 65535;
        channel.sentPacketBufferSize = 32768;
        g_configValidateAssertCount = 0;
        if ( setjmp( g_configValidateJmpBuf ) == 0 )
            channel.Validate( 0, 8192 );
        check( g_configValidateAssertCount == 0 );
    }

    yojimbo_set_assert_function( prevAssertHandler );
#endif // #ifdef YOJIMBO_DEBUG
}

void test_client_server_config_default_unreliable_channel_validates()
{
#ifdef YOJIMBO_DEBUG
    void (*prevAssertHandler)( const char *, const char *, const char *, int ) = yojimbo_assert_function;
    yojimbo_set_assert_function( ExpectConfigValidateAssertHandler );

    // The configuration USAGE.md shows: one reliable-ordered and one unreliable-unordered
    // channel, every other field left at its default.
    ClientServerConfig config;
    config.numChannels = 2;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    g_configValidateAssertCount = 0;
    if ( setjmp( g_configValidateJmpBuf ) == 0 )
        config.Validate();
    check( g_configValidateAssertCount == 0 );

    yojimbo_set_assert_function( prevAssertHandler );
#endif // #ifdef YOJIMBO_DEBUG
}

void test_connection_unreliable_unordered_block_too_large_for_packet()
{
    // An unreliable-unordered channel sends a block whole, inside one packet. A block within
    // maxBlockSize that does not fit a packet once the headers are added is the caller's
    // mistake: it asserts at send in debug. Release is unchanged: the block is accepted, no
    // channel error is set, and it is never delivered.
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[0].maxBlockSize = config.maxPacketSize;

    Connection sender( GetDefaultAllocator(), messageFactory, config, time );

    const int blockSize = config.maxPacketSize - 12;
    TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( TEST_BLOCK_MESSAGE );
    check( message );
    uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
    check( blockData );
    memset( blockData, 0, blockSize );
    message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );

#ifdef YOJIMBO_DEBUG
    void (*prevAssertHandler)( const char *, const char *, const char *, int ) = yojimbo_assert_function;
    yojimbo_set_assert_function( ExpectConfigValidateAssertHandler );
    g_configValidateAssertCount = 0;
    if ( setjmp( g_configValidateJmpBuf ) == 0 )
        sender.SendMessage( 0, message );
    yojimbo_set_assert_function( prevAssertHandler );
    check( g_configValidateAssertCount == 1 );
    // The assert stopped SendMessage before it took the message, so release it here.
    messageFactory.ReleaseMessage( message );
#else // #ifdef YOJIMBO_DEBUG
    Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

    sender.SendMessage( 0, message );
    check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;
    for ( int i = 0; i < 10; ++i )
    {
        PumpConnectionUpdate( config, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );
        Message * received = receiver.ReceiveMessage( 0 );
        check( !received );
    }

    check( sender.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );
    check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_NONE );
#endif // #ifdef YOJIMBO_DEBUG
}

void test_connection_reliable_block_fragment_overflow()
{
    // Regression test: a peer sends a reliable-ordered block fragment whose final fragment
    // claims a full blockFragmentSize of data, even though maxBlockSize is not a multiple of
    // blockFragmentSize. The receive buffer (blockData) is maxBlockSize bytes, but the final
    // fragment is copied at offset (numFragments-1)*blockFragmentSize, so a full-size copy
    // used to run past the end of the buffer (heap-buffer-overflow, caught by ASan). The
    // fragment must be rejected before the copy. fragmentSize is attacker-controllable in
    // [1,blockFragmentSize], so a legitimate sender never produces this.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].maxBlockSize = 1100;      // deliberately not a multiple of blockFragmentSize
    config.channel[0].blockFragmentSize = 500;  // => GetMaxFragmentsPerBlock() == ceil(1100/500) == 3

    Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

    const int maxFragmentsPerBlock = config.channel[0].GetMaxFragmentsPerBlock();
    const int blockFragmentSize = config.channel[0].blockFragmentSize;

    // messageId=0 (== expected receive sequence), final fragment, full blockFragmentSize payload.
    const int numFragments = maxFragmentsPerBlock;   // 3
    const int fragmentId = numFragments - 1;         // 2 (final fragment)
    const int fragmentSize = blockFragmentSize;      // 500 -> write [1000,1500) into a 1100-byte buffer

    uint8_t buffer[4096];
    memset( buffer, 0, sizeof( buffer ) );
    WriteStream stream( buffer, sizeof( buffer ) );
    check( SerializeRawBlockFragmentPacket( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, fragmentId, fragmentSize ) );
    stream.Flush();
    const int packetBytes = stream.GetBytesProcessed();
    check( packetBytes > 0 );

    // Before the fix this overflowed the receive buffer inside ProcessPacketFragment. After the
    // fix the fragment is rejected: the read fails and the channel drops into an error state.
    const bool processed = receiver.ProcessPacket( NULL, 0, buffer, packetBytes );
    check( !processed );

    receiver.AdvanceTime( time );
    check( receiver.GetErrorLevel() == CONNECTION_ERROR_CHANNEL );
}

void test_connection_reliable_block_fragment_message_id_mismatch()
{
    // A block is in progress for message id 0. An ordinary message with the same id then
    // advances the receive sequence, and a fragment for message id 1 arrives. The receiver
    // must not reuse the in-progress block state (fragment count, received bits, block
    // message) from message id 0; the id mismatch is a desync and must be rejected.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

    const int maxFragmentsPerBlock = config.channel[0].GetMaxFragmentsPerBlock();
    const int blockFragmentSize = config.channel[0].blockFragmentSize;
    const int maxMessagesPerPacket = config.channel[0].maxMessagesPerPacket;
    const int maxMessageType = messageFactory.GetNumTypes() - 1;
    const int numFragments = 2;
    const int fragmentSize = 16;

    uint8_t buffer[4096];

    // fragment 0 of block message id 0: leaves the block in progress (1 of 2 fragments)
    memset( buffer, 0, sizeof( buffer ) );
    {
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragment0Packet( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, 0, blockFragmentSize, maxMessageType, TEST_BLOCK_MESSAGE, 0 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        receiver.ProcessPacket( NULL, 0, buffer, packetBytes );
    }

    // an ordinary message cannot claim the id of the block still being received
    memset( buffer, 0, sizeof( buffer ) );
    {
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawOrderedMessagePacket( stream, maxMessagesPerPacket, maxMessageType, 0, 0 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        check( !receiver.ProcessPacket( NULL, 1, buffer, packetBytes ) );
    }

    check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_DESYNC );

    // a fragment for message id 1 reusing the stale block's fragment count
    memset( buffer, 0, sizeof( buffer ) );
    {
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragmentPacket( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, 1, fragmentSize, 1 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        receiver.ProcessPacket( NULL, 2, buffer, packetBytes );
    }

    receiver.AdvanceTime( time );
    check( receiver.GetErrorLevel() == CONNECTION_ERROR_CHANNEL );
    check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_DESYNC );
}

void test_connection_reliable_block_fragment_different_message_id()
{
    // A block is in progress for message id 0. An ordinary message with a different id moves
    // the receive sequence past the block, and a fragment for that later id arrives. The
    // receiver must not reassemble it into the block still in progress for message id 0.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

    const int maxFragmentsPerBlock = config.channel[0].GetMaxFragmentsPerBlock();
    const int blockFragmentSize = config.channel[0].blockFragmentSize;
    const int maxMessagesPerPacket = config.channel[0].maxMessagesPerPacket;
    const int maxMessageType = messageFactory.GetNumTypes() - 1;
    const int numFragments = 2;
    const int fragmentSize = 16;

    uint8_t buffer[4096];

    // fragment 0 of block message id 0: leaves the block in progress (1 of 2 fragments)
    memset( buffer, 0, sizeof( buffer ) );
    {
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragment0Packet( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, 0, blockFragmentSize, maxMessageType, TEST_BLOCK_MESSAGE, 0 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        check( receiver.ProcessPacket( NULL, 0, buffer, packetBytes ) );
    }

    check( receiver.GetErrorLevel() == CONNECTION_ERROR_NONE );

    // ordinary message id 2 advances the receive sequence to 3 while block id 0 is in progress
    memset( buffer, 0, sizeof( buffer ) );
    {
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawOrderedMessagePacket( stream, maxMessagesPerPacket, maxMessageType, 2, 0 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        check( receiver.ProcessPacket( NULL, 1, buffer, packetBytes ) );
    }

    check( receiver.GetErrorLevel() == CONNECTION_ERROR_NONE );

    // final fragment for message id 3, which must not complete the block held for message id 0
    memset( buffer, 0, sizeof( buffer ) );
    {
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragmentPacket( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, 1, fragmentSize, 3 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        check( !receiver.ProcessPacket( NULL, 2, buffer, packetBytes ) );
    }

    receiver.AdvanceTime( time );
    check( receiver.GetErrorLevel() == CONNECTION_ERROR_CHANNEL );
    check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_DESYNC );
}

void test_connection_reliable_rejects_short_nonfinal_block_fragment()
{
    // Every non-final fragment of a multi-fragment block must be a full blockFragmentSize.
    // A shorter non-final fragment is rejected as a desync and the block is not delivered.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].maxBlockSize = 1100;      // not a multiple of blockFragmentSize
    config.channel[0].blockFragmentSize = 500;  // => GetMaxFragmentsPerBlock() == 3

    Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

    const int maxFragmentsPerBlock = config.channel[0].GetMaxFragmentsPerBlock();
    const int blockFragmentSize = config.channel[0].blockFragmentSize;
    const int maxMessageType = messageFactory.GetNumTypes() - 1;

    const int numFragments = maxFragmentsPerBlock;   // 3
    check( numFragments > 1 );

    uint8_t buffer[4096];

    // Fragment 0, deliberately shorter than a full fragment. The packet must be rejected on
    // arrival, before its data is copied into the block buffer.
    {
        memset( buffer, 0, sizeof( buffer ) );
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragment0Packet( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, 0, blockFragmentSize / 2, maxMessageType, TEST_BLOCK_MESSAGE, 0 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        check( !receiver.ProcessPacket( NULL, 0, buffer, packetBytes ) );
        check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_DESYNC );
    }

    // Feed the rest of the block too; the block must still not be delivered.
    for ( int fragmentId = 1; fragmentId < numFragments; ++fragmentId )
    {
        const int fragmentSize = ( fragmentId == numFragments - 1 ) ? config.channel[0].maxBlockSize - ( numFragments - 1 ) * blockFragmentSize : blockFragmentSize;

        memset( buffer, 0, sizeof( buffer ) );
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragmentPacket( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, fragmentId, fragmentSize ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        receiver.ProcessPacket( NULL, 0, buffer, packetBytes );
    }

    check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_DESYNC );
    check( receiver.ReceiveMessage( 0 ) == NULL );
}

void test_connection_reliable_rejects_short_middle_block_fragment()
{
    // The full-fragment rule applies to every non-final fragment, not only fragment 0. A short
    // fragment in the middle of a block is rejected as a desync and the block is not delivered.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].maxBlockSize = 1100;      // not a multiple of blockFragmentSize
    config.channel[0].blockFragmentSize = 500;  // => GetMaxFragmentsPerBlock() == 3

    Connection receiver( GetDefaultAllocator(), messageFactory, config, time );

    const int maxFragmentsPerBlock = config.channel[0].GetMaxFragmentsPerBlock();
    const int blockFragmentSize = config.channel[0].blockFragmentSize;
    const int maxMessageType = messageFactory.GetNumTypes() - 1;

    const int numFragments = maxFragmentsPerBlock;   // 3
    const int shortFragmentId = 1;                   // non-final and not fragment 0
    check( numFragments > 2 );
    check( shortFragmentId != 0 );
    check( shortFragmentId != numFragments - 1 );

    uint8_t buffer[4096];

    // Fragment 0 arrives full size and carries the block message body.
    {
        memset( buffer, 0, sizeof( buffer ) );
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragment0Packet( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, 0, blockFragmentSize, maxMessageType, TEST_BLOCK_MESSAGE, 0 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        check( receiver.ProcessPacket( NULL, 0, buffer, packetBytes ) );
    }

    // A middle fragment, deliberately shorter than a full fragment, must be rejected on arrival.
    {
        memset( buffer, 0, sizeof( buffer ) );
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragmentPacket( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, shortFragmentId, blockFragmentSize / 2 ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        check( !receiver.ProcessPacket( NULL, 0, buffer, packetBytes ) );
        check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_DESYNC );
    }

    // Feed the remaining fragments too; the block must still not be delivered.
    for ( int fragmentId = shortFragmentId + 1; fragmentId < numFragments; ++fragmentId )
    {
        const int fragmentSize = ( fragmentId == numFragments - 1 ) ? config.channel[0].maxBlockSize - ( numFragments - 1 ) * blockFragmentSize : blockFragmentSize;

        memset( buffer, 0, sizeof( buffer ) );
        WriteStream stream( buffer, sizeof( buffer ) );
        check( SerializeRawBlockFragmentPacket( stream, maxFragmentsPerBlock, blockFragmentSize, numFragments, fragmentId, fragmentSize ) );
        stream.Flush();
        const int packetBytes = stream.GetBytesProcessed();
        check( packetBytes > 0 );
        receiver.ProcessPacket( NULL, 0, buffer, packetBytes );
    }

    check( receiver.GetChannelErrorLevel( 0 ) == CHANNEL_ERROR_DESYNC );
    check( receiver.ReceiveMessage( 0 ) == NULL );
}

void test_connection_reliable_over_budget_packet()
{
    // A peer sends more channel data than the receiver's configured packetBudget. On read this
    // used to trip the YOJIMBO_DEBUG_MESSAGE_BUDGET assert — a remote crash in debug builds
    // reachable from a decrypted-but-attacker-controlled packet. The message count and sizes
    // are already bounded by the serialize_* range checks, so an over-budget read must simply
    // be accepted, not asserted on.

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    // Sender: default (unlimited) packet budget, so it packs messages freely.
    ConnectionConfig senderConfig;
    senderConfig.numChannels = 1;
    senderConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    // Receiver: a small per-channel packet budget.
    ConnectionConfig receiverConfig;
    receiverConfig.numChannels = 1;
    receiverConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    receiverConfig.channel[0].packetBudget = 32;

    Connection sender( GetDefaultAllocator(), messageFactory, senderConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, receiverConfig, time );

    const int NumMessages = 64;
    for ( int i = 0; i < NumMessages; ++i )
    {
        TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
        check( message );
        message->sequence = (uint16_t) i;
        sender.SendMessage( 0, message );
    }

    // The first generated packet carries far more than 32 bytes of channel data. Before the
    // fix, processing it aborted the receiver in the budget assert; after, it reads cleanly.
    uint8_t * packetData = (uint8_t*) alloca( senderConfig.maxPacketSize );
    uint16_t sequence = 0;
    bool processed = false;

    for ( int i = 0; i < 8 && !processed; ++i )
    {
        int packetBytes = 0;
        if ( sender.GeneratePacket( NULL, sequence, packetData, senderConfig.maxPacketSize, packetBytes ) && packetBytes > 0 )
        {
            check( receiver.ProcessPacket( NULL, sequence, packetData, packetBytes ) );
            processed = true;
        }

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );
        time += 0.1;
        sequence++;
    }

    check( processed );
    check( receiver.GetErrorLevel() == CONNECTION_ERROR_NONE );
}

void PumpClientServerUpdate( double & time, Client ** client, int numClients, Server ** server, int numServers, float deltaTime = 0.1f )
{
    for ( int i = 0; i < numClients; ++i )
        client[i]->SendPackets();

    for ( int i = 0; i < numServers; ++i )
        server[i]->SendPackets();

    for ( int i = 0; i < numClients; ++i )
        client[i]->ReceivePackets();

    for ( int i = 0; i < numServers; ++i )
        server[i]->ReceivePackets();

    time += deltaTime;

    for ( int i = 0; i < numClients; ++i )
        client[i]->AdvanceTime( time );

    for ( int i = 0; i < numServers; ++i )
        server[i]->AdvanceTime( time );

    yojimbo_sleep( 0.0f );
}

void SendClientToServerMessages( Client & client, int numMessagesToSend, int channelIndex = ReliableChannel )
{
    for ( int i = 0; i < numMessagesToSend; ++i )
    {
        if ( !client.CanSendMessage( channelIndex ) )
            break;

        if ( rand() % 10 )
        {
            TestMessage * message = (TestMessage*) client.CreateMessage( TEST_MESSAGE );
            check( message );
            message->sequence = i;
            client.SendMessage( channelIndex, message );
        }
        else
        {
            TestBlockMessage * message = (TestBlockMessage*) client.CreateMessage( TEST_BLOCK_MESSAGE );
            check( message );
            message->sequence = i;
            const int blockSize = 1 + ( ( i * 901 ) % 1001 );
            uint8_t * blockData = client.AllocateBlock( blockSize );
            check( blockData );
            for ( int j = 0; j < blockSize; ++j )
                blockData[j] = i + j;
            client.AttachBlockToMessage( message, blockData, blockSize );
            client.SendMessage( channelIndex, message );
        }
    }
}

void SendServerToClientMessages( Server & server, int clientIndex, int numMessagesToSend, int channelIndex = ReliableChannel )
{
    for ( int i = 0; i < numMessagesToSend; ++i )
    {
        if ( !server.CanSendMessage( clientIndex, channelIndex ) )
            break;

        if ( rand() % 10 )
        {
            TestMessage * message = (TestMessage*) server.CreateMessage( clientIndex, TEST_MESSAGE );
            check( message );
            message->sequence = i;
            server.SendMessage( clientIndex, channelIndex, message );
        }
        else
        {
            TestBlockMessage * message = (TestBlockMessage*) server.CreateMessage( clientIndex, TEST_BLOCK_MESSAGE );
            check( message );
            message->sequence = i;
            const int blockSize = 1 + ( ( i * 901 ) % 1001 );
            uint8_t * blockData = server.AllocateBlock( clientIndex, blockSize );
            check( blockData );
            for ( int j = 0; j < blockSize; ++j )
                blockData[j] = i + j;
            server.AttachBlockToMessage( clientIndex, message, blockData, blockSize );
            server.SendMessage( clientIndex, channelIndex, message );
        }
    }
}

void ProcessServerToClientMessages( Client & client, int & numMessagesReceivedFromServer, int channelIndex = ReliableChannel )
{
    while ( true )
    {
        Message * message = client.ReceiveMessage( channelIndex );

        if ( !message )
            break;

        check( message->GetId() == (int) numMessagesReceivedFromServer );

        switch ( message->GetType() )
        {
            case TEST_MESSAGE:
            {
                TestMessage * testMessage = (TestMessage*) message;
                check( !message->IsBlockMessage() );
                check( testMessage->sequence == uint16_t( numMessagesReceivedFromServer ) );
                ++numMessagesReceivedFromServer;
            }
            break;

            case TEST_BLOCK_MESSAGE:
            {
                check( message->IsBlockMessage() );
                TestBlockMessage * blockMessage = (TestBlockMessage*) message;
                check( blockMessage->sequence == uint16_t( numMessagesReceivedFromServer ) );
                const int blockSize = blockMessage->GetBlockSize();
                check( blockSize == 1 + ( ( numMessagesReceivedFromServer * 901 ) % 1001 ) );
                const uint8_t * blockData = blockMessage->GetBlockData();
                check( blockData );
                for ( int j = 0; j < blockSize; ++j )
                {
                    check( blockData[j] == uint8_t( numMessagesReceivedFromServer + j ) );
                }
                ++numMessagesReceivedFromServer;
            }
            break;
        }

        client.ReleaseMessage( message );
    }
}

void ProcessClientToServerMessages( Server & server, int clientIndex, int & numMessagesReceivedFromClient, int channelIndex = ReliableChannel )
{
    while ( true )
    {
        Message * message = server.ReceiveMessage( clientIndex, channelIndex );

        if ( !message )
            break;

        check( message->GetId() == (int) numMessagesReceivedFromClient );

        switch ( message->GetType() )
        {
            case TEST_MESSAGE:
            {
                check( !message->IsBlockMessage() );
                TestMessage * testMessage = (TestMessage*) message;
                check( testMessage->sequence == uint16_t( numMessagesReceivedFromClient ) );
                ++numMessagesReceivedFromClient;
            }
            break;

            case TEST_BLOCK_MESSAGE:
            {
                check( message->IsBlockMessage() );
                TestBlockMessage * blockMessage = (TestBlockMessage*) message;
                check( blockMessage->sequence == uint16_t( numMessagesReceivedFromClient ) );
                const int blockSize = blockMessage->GetBlockSize();
                check( blockSize == 1 + ( ( numMessagesReceivedFromClient * 901 ) % 1001 ) );
                const uint8_t * blockData = blockMessage->GetBlockData();
                check( blockData );
                for ( int j = 0; j < blockSize; ++j )
                {
                    check( blockData[j] == uint8_t( numMessagesReceivedFromClient + j ) );
                }
                ++numMessagesReceivedFromClient;
            }
            break;
        }

        server.ReleaseMessage( clientIndex, message );
    }
}

void test_connection_reliable_message_alloc_failure()
{
    // Regression: on the send path, GetMessagePacketData allocated the message-pointer array
    // without checking for NULL and then wrote through it - a crash on allocator exhaustion.
    // Arm the allocator to fail that first allocation inside GeneratePacket and verify we neither
    // crash nor leak (the queued reliable message is retained and freed at teardown).
    ArmableAllocator allocator;
    {
        TestMessageFactory messageFactory( allocator );

        ConnectionConfig config;
        config.numChannels = 1;
        config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

        {
            Connection connection( allocator, messageFactory, config, 100.0 );

            Message * message = messageFactory.CreateMessage( TEST_MESSAGE );
            check( message );
            connection.SendMessage( 0, message );

            uint8_t packetData[4096];
            int packetBytes = 0;

            allocator.Arm( 0 );     // fail the first allocation inside GeneratePacket (message array)
            connection.GeneratePacket( NULL, 0, packetData, sizeof( packetData ), packetBytes );
            allocator.Disarm();
        }
    }
    check( allocator.GetOutstanding() == 0 );   // no leak across teardown
}

void test_connection_unreliable_message_alloc_failure()
{
    // Regression: the unreliable channel's GetPacketData allocated the message-pointer array
    // without a NULL check and then wrote through it. The messages had already been popped off
    // the send queue, so a failure crashed (NULL deref) and would have leaked the popped
    // messages. Arm the allocator to fail that allocation and verify no crash and no leak.
    ArmableAllocator allocator;
    {
        TestMessageFactory messageFactory( allocator );

        ConnectionConfig config;
        config.numChannels = 1;
        config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

        {
            Connection connection( allocator, messageFactory, config, 100.0 );

            for ( int i = 0; i < 4; ++i )
            {
                Message * message = messageFactory.CreateMessage( TEST_MESSAGE );
                check( message );
                connection.SendMessage( 0, message );
            }

            uint8_t packetData[4096];
            int packetBytes = 0;

            allocator.Arm( 0 );     // fail the message-pointer array allocation in GetPacketData
            connection.GeneratePacket( NULL, 0, packetData, sizeof( packetData ), packetBytes );
            allocator.Disarm();
        }
    }
    check( allocator.GetOutstanding() == 0 );   // popped messages released, nothing leaked
}

void test_connection_generate_packet_channel_data_alloc_failure()
{
    // Regression: when AllocateChannelData failed, GeneratePacket returned without freeing the
    // per-channel packet data GetPacketData had already populated (acquired message references
    // and the allocated message-pointer array), leaking them. Arm the allocator so the message
    // array allocation succeeds but the subsequent channel-data allocation fails, and verify
    // nothing leaks.
    ArmableAllocator allocator;
    {
        TestMessageFactory messageFactory( allocator );

        ConnectionConfig config;
        config.numChannels = 1;
        config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

        {
            Connection connection( allocator, messageFactory, config, 100.0 );

            Message * message = messageFactory.CreateMessage( TEST_MESSAGE );
            check( message );
            connection.SendMessage( 0, message );

            uint8_t packetData[4096];
            int packetBytes = 0;

            allocator.Arm( 1 );     // 1st alloc (message array) succeeds, 2nd (channel data) fails
            const bool result = connection.GeneratePacket( NULL, 0, packetData, sizeof( packetData ), packetBytes );
            allocator.Disarm();
            check( !result );       // the channel-data allocation failure fails the generate
        }
    }
    check( allocator.GetOutstanding() == 0 );   // no leak across teardown
}

#ifdef YOJIMBO_DEBUG
#include <setjmp.h>

static jmp_buf g_messageTypeLimitJmpBuf;
static int g_messageTypeLimitAssertCount = 0;
static int g_messageTypeLimitConstructed = 0;

static void ExpectMessageTypeLimitAssertHandler( const char * /*condition*/, const char * /*function*/, const char * /*file*/, int /*line*/ )
{
    g_messageTypeLimitAssertCount++;
    longjmp( g_messageTypeLimitJmpBuf, 1 );
}
#endif // #ifdef YOJIMBO_DEBUG

void test_message_factory_num_types_within_type_field()
{
#ifdef YOJIMBO_DEBUG
    void (*prevAssertHandler)( const char *, const char *, const char *, int ) = yojimbo_assert_function;
    yojimbo_set_assert_function( ExpectMessageTypeLimitAssertHandler );

    // Message::m_type is uint32_t : 15, so a factory count must stay in 0..32767.
    const int rejected[] = { -1, 32768, 32769 };
    for ( int i = 0; i < (int) ( sizeof( rejected ) / sizeof( rejected[0] ) ); ++i )
    {
        g_messageTypeLimitAssertCount = 0;
        g_messageTypeLimitConstructed = 0;
        if ( setjmp( g_messageTypeLimitJmpBuf ) == 0 )
        {
            MessageFactory factory( GetDefaultAllocator(), rejected[i] );
            g_messageTypeLimitConstructed = 1;
            (void) factory;
        }
        check( g_messageTypeLimitAssertCount == 1 );
        check( g_messageTypeLimitConstructed == 0 );
    }

    const int accepted[] = { 0, 1, 32767 };
    for ( int i = 0; i < (int) ( sizeof( accepted ) / sizeof( accepted[0] ) ); ++i )
    {
        g_messageTypeLimitAssertCount = 0;
        g_messageTypeLimitConstructed = 0;
        if ( setjmp( g_messageTypeLimitJmpBuf ) == 0 )
        {
            MessageFactory factory( GetDefaultAllocator(), accepted[i] );
            check( factory.GetNumTypes() == accepted[i] );
            g_messageTypeLimitConstructed = 1;
        }
        check( g_messageTypeLimitAssertCount == 0 );
        check( g_messageTypeLimitConstructed == 1 );
    }

    {
        g_messageTypeLimitAssertCount = 0;
        g_messageTypeLimitConstructed = 0;
        if ( setjmp( g_messageTypeLimitJmpBuf ) == 0 )
        {
            MessageFactory factory( GetDefaultAllocator(), 1 );
            ConnectionConfig config;
            config.numChannels = 1;
            config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            Connection connection( GetDefaultAllocator(), factory, config, 0.0 );
            g_messageTypeLimitConstructed = 1;
            (void) connection;
        }
        check( g_messageTypeLimitAssertCount == 0 );
        check( g_messageTypeLimitConstructed == 1 );
    }

    yojimbo_set_assert_function( prevAssertHandler );
#else // #ifdef YOJIMBO_DEBUG
    MessageFactory factory( GetDefaultAllocator(), 1 );
    check( factory.GetNumTypes() == 1 );
#endif // #ifdef YOJIMBO_DEBUG
}

void test_message_factory_create_message_alloc_failure()
{
    // Regression: YOJIMBO_NEW used to run the constructor on a NULL pointer when the allocation
    // failed (undefined behavior / crash) before CreateMessage's NULL check. CreateMessage must
    // return NULL cleanly on allocator exhaustion and flag the factory.
    ArmableAllocator allocator;
    {
        TestMessageFactory factory( allocator );
        allocator.Arm( 0 );     // fail the next allocation (the message object)
        Message * message = factory.CreateMessage( TEST_MESSAGE );
        allocator.Disarm();
        check( message == NULL );
        check( factory.GetErrorLevel() == MESSAGE_FACTORY_ERROR_FAILED_TO_ALLOCATE_MESSAGE );
    }
    check( allocator.GetOutstanding() == 0 );
}

void test_connection_process_packet_channel_data_alloc_failure()
{
    // Regression: on the read path, if AllocateChannelData failed, numChannelEntries had already
    // been read from the wire, so the ConnectionPacket destructor iterated a NULL channelEntry
    // array (crash). Feed a valid packet to a receiver whose allocator fails the channel-data
    // allocation, and verify the read fails cleanly without crashing or leaking.
    ArmableAllocator senderAlloc;
    ArmableAllocator receiverAlloc;
    {
        TestMessageFactory senderFactory( senderAlloc );
        TestMessageFactory receiverFactory( receiverAlloc );

        ConnectionConfig config;
        config.numChannels = 1;
        config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

        {
            Connection sender( senderAlloc, senderFactory, config, 100.0 );
            Connection receiver( receiverAlloc, receiverFactory, config, 100.0 );

            Message * message = senderFactory.CreateMessage( TEST_MESSAGE );
            check( message );
            sender.SendMessage( 0, message );

            uint8_t packetData[4096];
            int packetBytes = 0;
            check( sender.GeneratePacket( NULL, 0, packetData, sizeof( packetData ), packetBytes ) );
            check( packetBytes > 0 );

            receiverAlloc.Arm( 0 );     // fail the channel-data allocation in the connection-packet read
            const bool ok = receiver.ProcessPacket( NULL, 0, packetData, packetBytes );
            receiverAlloc.Disarm();
            check( !ok );               // read fails cleanly rather than crashing in the destructor
        }
    }
    check( senderAlloc.GetOutstanding() == 0 );
    check( receiverAlloc.GetOutstanding() == 0 );
}

// ReliableOrderedChannel's public and protected members are part of an installed header, so a
// subclass written against an earlier release must still compile and see the same members.
class ReliableOrderedChannelSubclass : public ReliableOrderedChannel
{
public:
    ReliableOrderedChannelSubclass( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int maxPacketSize, double time )
        : ReliableOrderedChannel( allocator, messageFactory, config, maxPacketSize, 0, time ) {}

    void CheckProtectedInterface( Allocator & allocator )
    {
        SentPacketEntry sentPacket;
        memset( &sentPacket, 0, sizeof( sentPacket ) );
        sentPacket.timeSent = 1.0;
        check( sentPacket.timeSent == 1.0 );

        SendBlockData sendBlock( allocator, 4 );
        check( sendBlock.blockSize == 0 );

        ReceiveBlockData receiveBlock( allocator, 1024, 4 );
        check( receiveBlock.messageType == 0 );
    }
};

void test_reliable_ordered_channel_interface()
{
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig config;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;

    ReliableOrderedChannelSubclass channel( GetDefaultAllocator(), messageFactory, config.channel[0], config.maxPacketSize, 100.0 );

    channel.CheckProtectedInterface( GetDefaultAllocator() );

    // A fragment for a message id the channel is not expecting yet is ignored.
    const uint8_t fragmentData[1] = { 0 };
    channel.ProcessPacketFragment( TEST_BLOCK_MESSAGE, 1, 1, 0, fragmentData, 1, NULL );
    check( channel.GetErrorLevel() == CHANNEL_ERROR_NONE );
}

// YJ-05: the build added -fno-rtti to every C++ target, so libyojimbo emitted "vtable for
// yojimbo::Allocator" with no matching typeinfo. Allocator, MessageFactory, Adapter and the two
// interfaces are all public polymorphic classes consumers derive from, and consumers build with
// the compiler default, which is RTTI on. Any dynamic_cast or typeid on one of them failed to
// link on the missing typeinfo. RTTI is now on in yojimbo's own build too.
//
// This is a link test as much as a behaviour test: the typeinfo for Allocator and
// DefaultAllocator comes out of libyojimbo, where their key functions live.
void test_rtti_across_the_abi_boundary()
{
    ArmableAllocator allocator;

    Allocator & base = allocator;
    check( dynamic_cast<ArmableAllocator*>( &base ) == &allocator );
    check( dynamic_cast<DefaultAllocator*>( &base ) == NULL );
    check( typeid( base ) == typeid( ArmableAllocator ) );

    // DefaultAllocator's key function is in yojimbo_allocator.cpp, so this resolves the library's
    // own typeinfo rather than one the test binary could have emitted for itself.
    Allocator & defaultAllocator = GetDefaultAllocator();
    check( dynamic_cast<DefaultAllocator*>( &defaultAllocator ) != NULL );

    // ...and the same across the message factory hierarchy.
    TestMessageFactory factory( allocator );
    MessageFactory & factoryBase = factory;
    check( dynamic_cast<TestMessageFactory*>( &factoryBase ) == &factory );
}

// YJ-04: Allocator held a std::map and MessageFactory held another, both compiled in only when
// YOJIMBO_DEBUG_MEMORY_LEAKS / YOJIMBO_DEBUG_MESSAGE_LEAKS were on -- which both derive from the
// consumer's NDEBUG. A release consumer deriving from either class saw a different layout than
// the debug library it linked against (Allocator 16 vs 40 bytes, MessageFactory 24 vs 48 on
// arm64), which is silent memory corruption. Both trackers now live behind one always-present
// pointer to state defined inside the library.
//
// test_layout_debug.cpp and test_layout_release.cpp compile the same public headers with
// YOJIMBO_DEBUG and YOJIMBO_RELEASE forced on, so one binary carries both configurations' idea of
// the layout.
extern void yojimbo_test_layout_debug( size_t & allocatorSize, size_t & messageFactorySize );
extern void yojimbo_test_layout_release( size_t & allocatorSize, size_t & messageFactorySize );

void test_public_layout_is_configuration_independent()
{
    size_t debugAllocator = 0, debugMessageFactory = 0;
    size_t releaseAllocator = 0, releaseMessageFactory = 0;

    yojimbo_test_layout_debug( debugAllocator, debugMessageFactory );
    yojimbo_test_layout_release( releaseAllocator, releaseMessageFactory );

    check( debugAllocator == releaseAllocator );
    check( debugMessageFactory == releaseMessageFactory );

    // ...and this build agrees with both of them.
    check( sizeof( Allocator ) == debugAllocator );
    check( sizeof( MessageFactory ) == debugMessageFactory );
}

// YJ-02: ServerInterface::GetClientUserData was declared non-virtual with no definition
// anywhere in the tree, so any call through a ServerInterface reference failed to link. Nothing
// exercised the interfaces through a base reference, so it went unnoticed.
//
// This is a LINK test. A call to a non-virtual interface method needs a definition of that exact
// symbol; a call to a virtual one goes through the vtable and needs none. Calling every declared
// method of both interfaces through a base reference therefore fails to link the test binary if
// any of them ever loses its "virtual" again.
//
// The calls are guarded by a volatile that is always false: the compiler cannot fold the branch
// away, so it emits every call, and nothing here actually runs against a stopped server.
static volatile bool g_callInterfaceMethods = false;

static void CallEveryServerInterfaceMethod( ServerInterface & server )
{
    NetworkInfo info;
    uint8_t userData[256];
    memset( userData, 0, sizeof( userData ) );
    uint8_t packet[16];
    memset( packet, 0, sizeof( packet ) );

    server.SetContext( NULL );
    check( server.Start( 1 ) );
    server.Stop();
    server.DisconnectClient( 0 );
    server.DisconnectAllClients();
    server.SendPackets();
    server.ReceivePackets();
    server.AdvanceTime( 0.0 );
    (void) server.IsRunning();
    (void) server.GetMaxClients();
    (void) server.IsClientConnected( 0 );
    (void) server.GetClientId( 0 );
    (void) server.GetClientUserData( 0 );
    (void) server.GetClientAddress( 0 );
    (void) server.GetNumConnectedClients();
    (void) server.GetTime();
    Message * message = server.CreateMessage( 0, TEST_MESSAGE );
    uint8_t * block = server.AllocateBlock( 0, 16 );
    server.AttachBlockToMessage( 0, message, block, 16 );
    server.FreeBlock( 0, block );
    (void) server.CanSendMessage( 0, 0 );
    server.SendMessage( 0, 0, message );
    (void) server.ReceiveMessage( 0, 0 );
    server.ReleaseMessage( 0, message );
    server.GetNetworkInfo( 0, info );
    server.ConnectLoopbackClient( 0, 1, userData );
    server.DisconnectLoopbackClient( 0 );
    (void) server.IsLoopbackClient( 0 );
    server.ProcessLoopbackPacket( 0, packet, sizeof( packet ), 0 );
}

static void CallEveryClientInterfaceMethod( ClientInterface & client )
{
    NetworkInfo info;
    uint8_t packet[16];
    memset( packet, 0, sizeof( packet ) );

    client.SetContext( NULL );
    client.Disconnect();
    client.SendPackets();
    client.ReceivePackets();
    client.AdvanceTime( 0.0 );
    (void) client.IsConnecting();
    (void) client.IsConnected();
    (void) client.IsDisconnected();
    (void) client.ConnectionFailed();
    (void) client.GetClientState();
    (void) client.GetClientIndex();
    (void) client.GetClientId();
    (void) client.GetTime();
    Message * message = client.CreateMessage( TEST_MESSAGE );
    uint8_t * block = client.AllocateBlock( 16 );
    client.AttachBlockToMessage( message, block, 16 );
    client.FreeBlock( block );
    (void) client.CanSendMessage( 0 );
    client.SendMessage( 0, message );
    (void) client.ReceiveMessage( 0 );
    client.ReleaseMessage( message );
    client.GetNetworkInfo( info );
    (void) client.ConnectLoopback( 0, 1, 1 );
    client.DisconnectLoopback();
    (void) client.IsLoopback();
    client.ProcessLoopbackPacket( packet, sizeof( packet ), 0 );
}

void test_servers_bind_distinct_ports()
{
    // Two servers created from the same test configuration must be able to run at the same
    // time: each binds its own free port, so two test processes can share one host.

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    const Address serverAddress1 = TestServerAddress();
    const Address serverAddress2 = TestServerAddress();

    Server server1( GetDefaultAllocator(), privateKey, serverAddress1, config, adapter, 100.0 );
    Server server2( GetDefaultAllocator(), privateKey, serverAddress2, config, adapter, 100.0 );

    check( server1.Start( 1 ) );
    check( server2.Start( 1 ) );

    check( server1.GetAddress().GetPort() != 0 );
    check( server2.GetAddress().GetPort() != 0 );
    check( server1.GetAddress().GetPort() != server2.GetAddress().GetPort() );

    server2.Stop();
    server1.Stop();
}

void test_interface_methods_link()
{
    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, TestServerAddress(), config, adapter, 100.0 );
    Client client( GetDefaultAllocator(), Address( "0.0.0.0", 0 ), config, adapter, 100.0 );

    if ( g_callInterfaceMethods )
    {
        CallEveryServerInterfaceMethod( server );
        CallEveryClientInterfaceMethod( client );
    }

    // The binary linked, which is the whole point. Prove the base reference really does reach
    // the derived implementation for the method that had no definition at all. That method needs
    // a running server, so start one for the comparison.
    check( server.Start( 1 ) );
    ServerInterface & serverInterface = server;
    check( serverInterface.GetClientUserData( 0 ) == server.GetClientUserData( 0 ) );
    check( serverInterface.GetMaxClients() == 1 );
    server.Stop();

    ClientInterface & clientInterface = client;
    check( clientInterface.GetClientState() == client.GetClientState() );
}

// Adapter whose factory methods can be made to fail, so the startup paths are exercised at the
// two points an ArmableAllocator cannot reach: a user allocator class that fails to construct,
// and a message factory that fails to construct. Both return NULL from the adapter, which the
// library must treat exactly like an allocation failure.
class FailingFactoryAdapter : public TestAdapter
{
public:
    FailingFactoryAdapter() : m_allocatorsUntilFail( -1 ), m_factoriesUntilFail( -1 ) {}

    void FailAllocatorAfter( int n ) { m_allocatorsUntilFail = n; }
    void FailMessageFactoryAfter( int n ) { m_factoriesUntilFail = n; }

    Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes )
    {
        if ( m_allocatorsUntilFail == 0 )
            return NULL;
        if ( m_allocatorsUntilFail > 0 )
            m_allocatorsUntilFail--;
        return TestAdapter::CreateAllocator( allocator, memory, bytes );
    }

    MessageFactory * CreateMessageFactory( Allocator & allocator )
    {
        if ( m_factoriesUntilFail == 0 )
            return NULL;
        if ( m_factoriesUntilFail > 0 )
            m_factoriesUntilFail--;
        return TestAdapter::CreateMessageFactory( allocator );
    }

private:
    int m_allocatorsUntilFail;
    int m_factoriesUntilFail;
};

// The number of startup allocations a sweep is allowed to walk before we call it a runaway. Well
// above the real counts (6 for a two-client server, 3 for a client) so the sweep terminates on a
// bug instead of looping forever.
static const int MaxStartupAllocations = 64;

void test_server_start_alloc_failure()
{
    // YJ-01: BaseServer::Start guarded its allocations with yojimbo_assert, which compiles out
    // under NDEBUG, so a release build bound references to failed allocations and ran on with a
    // half-built server. Fail at allocation N for every N up to the first N that succeeds: each
    // failure must return false, leave the server stopped, and free everything already taken.
    Address serverAddress = TestServerAddress();

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    int n = 0;
    for ( ; n < MaxStartupAllocations; ++n )
    {
        ArmableAllocator allocator;
        {
            Server server( allocator, privateKey, serverAddress, config, adapter, 100.0 );
            allocator.Arm( n );
            const bool started = server.Start( 2 );
            allocator.Disarm();
            if ( started )
            {
                check( server.IsRunning() );
                server.Stop();
                check( !server.IsRunning() );
                break;
            }
            check( !server.IsRunning() );           // no half-built server left behind
            check( server.GetMaxClients() == 0 );
        }
        check( allocator.GetOutstanding() == 0 );   // everything already allocated was unwound
    }
    check( n > 0 );                                 // the sweep really did force failures
    check( n < MaxStartupAllocations );             // ...and a fully armed start eventually succeeds
}

void test_server_start_factory_failure()
{
    // Same contract for the adapter factory results, which no allocator arming can reach: a user
    // allocator or message factory that fails to construct returns NULL and must be handled like
    // an allocation failure rather than dereferenced.
    Address serverAddress = TestServerAddress();

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    // allocator index 0 is the global allocator, 1 and 2 are the two per-client allocators
    for ( int n = 0; n < 3; ++n )
    {
        ArmableAllocator allocator;
        {
            FailingFactoryAdapter failingAdapter;
            failingAdapter.FailAllocatorAfter( n );
            Server server( allocator, privateKey, serverAddress, config, failingAdapter, 100.0 );
            check( !server.Start( 2 ) );
            check( !server.IsRunning() );
        }
        check( allocator.GetOutstanding() == 0 );
    }

    for ( int n = 0; n < 2; ++n )
    {
        ArmableAllocator allocator;
        {
            FailingFactoryAdapter failingAdapter;
            failingAdapter.FailMessageFactoryAfter( n );
            Server server( allocator, privateKey, serverAddress, config, failingAdapter, 100.0 );
            check( !server.Start( 2 ) );
            check( !server.IsRunning() );
        }
        check( allocator.GetOutstanding() == 0 );
    }
}

void test_server_start_per_client_memory_exhaustion()
{
    // A per-client heap smaller than what the configured channels need must make Start fail
    // cleanly: return false, leave the server stopped, and free everything already taken. Both a
    // size that runs out early and one that lets the first channel through before the second
    // fails are covered.
    Address serverAddress( "127.0.0.1", ServerPort );

    const int perClientMemorySizes[] = { 65536, 1048576 };

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    for ( int i = 0; i < (int) ( sizeof( perClientMemorySizes ) / sizeof( perClientMemorySizes[0] ) ); ++i )
    {
        ClientServerConfig config;
        config.serverPerClientMemory = perClientMemorySizes[i];

        check( config.numChannels == 2 );
        check( config.channel[0].type == CHANNEL_TYPE_RELIABLE_ORDERED );
        check( config.channel[1].type == CHANNEL_TYPE_RELIABLE_ORDERED );

        ArmableAllocator allocator;
        {
            Server server( allocator, privateKey, serverAddress, config, adapter, 100.0 );
            check( !server.Start( 1 ) );
            check( !server.IsRunning() );
            check( server.GetMaxClients() == 0 );
        }
        check( allocator.GetOutstanding() == 0 );
    }
}

void test_container_allocation_failure_is_safe()
{
    // The sequence buffer, queue and bit array sit underneath every connection channel. When
    // their backing allocation fails they must still be constructible and destructible: no
    // touch of the missing buffer, and a size that keeps later use from indexing it.
    {
        ArmableAllocator allocator;
        allocator.Arm( 0 );
        SequenceBuffer<int> sequenceBuffer( allocator, 16 );
        check( sequenceBuffer.GetSize() == 0 );
        sequenceBuffer.Reset();
        check( allocator.GetOutstanding() == 0 );
    }

    {
        ArmableAllocator allocator;
        allocator.Arm( 0 );
        Queue<int> queue( allocator, 16 );
        check( queue.GetSize() == 0 );
        check( queue.IsEmpty() );
        check( queue.IsFull() );
        queue.Clear();
        check( allocator.GetOutstanding() == 0 );
    }

    {
        ArmableAllocator allocator;
        allocator.Arm( 0 );
        BitArray bitArray( allocator, 64 );
        bitArray.Clear();
        check( allocator.GetOutstanding() == 0 );
    }
}

void test_connection_unreliable_channel_alloc_failure_is_safe()
{
    // An unreliable-unordered channel whose queue object cannot be allocated must still be
    // resettable and destructible. Arm the allocator so the channel object itself succeeds and
    // the first thing it allocates fails, leaving the channel holding null queues.
    ConnectionConfig config;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    ArmableAllocator allocator;
    TestMessageFactory messageFactory( allocator );

    allocator.Arm( 1 );
    {
        Connection connection( allocator, messageFactory, config, 0.0 );
        check( allocator.GetErrorLevel() == ALLOCATOR_ERROR_OUT_OF_MEMORY );
        connection.Reset();
    }
    check( allocator.GetOutstanding() == 0 );
}

void test_client_connect_per_client_memory_exhaustion()
{
    // The client side of the same contract: too little client memory for the configured
    // channels must fail the connect cleanly, with no crash, no leak, and an out-of-memory
    // disconnect reason.
    Address clientAddress( "0.0.0.0", 0 );
    Address serverAddress( "127.0.0.1", ServerPort );

    const int clientMemorySizes[] = { 65536, 1048576 };

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    for ( int i = 0; i < (int) ( sizeof( clientMemorySizes ) / sizeof( clientMemorySizes[0] ) ); ++i )
    {
        ClientServerConfig config;
        config.clientMemory = clientMemorySizes[i];

        check( config.numChannels == 2 );
        check( config.channel[0].type == CHANNEL_TYPE_RELIABLE_ORDERED );
        check( config.channel[1].type == CHANNEL_TYPE_RELIABLE_ORDERED );

        ArmableAllocator allocator;
        {
            Client client( allocator, clientAddress, config, adapter, 100.0 );
            check( !client.InsecureConnect( privateKey, 1, serverAddress ) );
            check( !client.IsConnecting() );
            check( !client.IsConnected() );
            check( client.ConnectionFailed() );
            check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_OUT_OF_MEMORY );
        }
        check( allocator.GetOutstanding() == 0 );
    }
}

void test_client_connect_alloc_failure()
{
    // YJ-01 on the client side: BaseClient::CreateInternal bound a reference to *m_clientAllocator
    // one line after allocating it, with no check in any build. Same sweep: every failure point
    // returns false, leaves the client disconnected with an out-of-memory reason, and leaks
    // nothing.
    Address clientAddress( "0.0.0.0", 0 );
    Address serverAddress = TestServerAddress();

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    int n = 0;
    for ( ; n < MaxStartupAllocations; ++n )
    {
        ArmableAllocator allocator;
        {
            Client client( allocator, clientAddress, config, adapter, 100.0 );
            allocator.Arm( n );
            const bool connecting = client.InsecureConnect( privateKey, 1, serverAddress );
            allocator.Disarm();
            if ( connecting )
            {
                check( client.IsConnecting() );
                client.Disconnect();
                break;
            }
            check( !client.IsConnecting() );
            check( !client.IsConnected() );
            check( client.ConnectionFailed() );
            check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_OUT_OF_MEMORY );
        }
        check( allocator.GetOutstanding() == 0 );
    }
    check( n > 0 );
    check( n < MaxStartupAllocations );
}

void test_client_connect_factory_failure()
{
    Address clientAddress( "0.0.0.0", 0 );
    Address serverAddress = TestServerAddress();

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    {
        ArmableAllocator allocator;
        {
            FailingFactoryAdapter failingAdapter;
            failingAdapter.FailAllocatorAfter( 0 );
            Client client( allocator, clientAddress, config, failingAdapter, 100.0 );
            check( !client.InsecureConnect( privateKey, 1, serverAddress ) );
            check( client.ConnectionFailed() );
        }
        check( allocator.GetOutstanding() == 0 );
    }

    {
        ArmableAllocator allocator;
        {
            FailingFactoryAdapter failingAdapter;
            failingAdapter.FailMessageFactoryAfter( 0 );
            Client client( allocator, clientAddress, config, failingAdapter, 100.0 );
            check( !client.InsecureConnect( privateKey, 1, serverAddress ) );
            check( client.ConnectionFailed() );
        }
        check( allocator.GetOutstanding() == 0 );
    }
}

void test_client_connect_socket_failure_no_crash()
{
    // Regression: Connect() and ConnectLoopback() called into netcode_client_connect() without
    // checking that CreateClient() succeeded. When the socket can't be created - forced here with
    // an unparseable bind address - m_client is NULL and netcode dereferences it (crash in
    // release; assert trap in debug). Both must bail like InsecureConnect() already does.
    ClientServerConfig config;

    Address invalidAddress;                     // ADDRESS_NONE -> "NONE" -> netcode_client_create fails
    check( !invalidAddress.IsValid() );

    Client client( GetDefaultAllocator(), invalidAddress, config, adapter, 100.0 );

    uint8_t connectToken[ConnectTokenBytes];
    memset( connectToken, 0, sizeof( connectToken ) );

    check( !client.Connect( 1, connectToken ) );    // must not crash - CreateClient failed
    check( !client.IsConnected() );

    client.ConnectLoopback( 0, 1, 1 );          // must not crash either
    check( !client.IsConnected() );

    client.Disconnect();
}

void test_client_is_loopback_when_disconnected()
{
    // Regression: IsLoopback() called netcode_client_loopback(m_client) with no NULL guard, so a
    // never-connected client (m_client == NULL) crashed. Like GetClientIndex(), it must be safe
    // to query in any state and simply report false.
    ClientServerConfig config;
    Address clientAddress = TestClientAddress();
    Client client( GetDefaultAllocator(), clientAddress, config, adapter, 100.0 );

    check( !client.IsLoopback() );              // must not crash; not connected -> not a loopback client
    check( client.GetClientIndex() == -1 );
    check( !client.IsConnected() );
}

void test_client_connect_twice_with_one_connect_token()
{
    // Mirrors netcode's test_client_reconnect_with_used_connect_token through yojimbo's API.
    // A connect token is spent by the connection it admits: once the server has accepted a
    // client holding it, presenting the same token again connects nothing, from the same
    // address, after the client's slot is free again.
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;

    // The second connect attempt has to run all the way out to a connection request timeout,
    // and the connect token's expiry has to stay above that so the failure is the timeout and
    // not an expired token. A short timeout keeps both waits short.
    config.timeout = 2;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    // One connect token, minted once and held, the way a matchmaker hands one to a client.

    char serverAddressString[MaxAddressLength];
    serverAddress.ToString( serverAddressString, MaxAddressLength );
    const char * serverAddressStrings[] = { serverAddressString };

    uint8_t userData[256];
    memset( userData, 0, sizeof( userData ) );

    uint8_t connectToken[ConnectTokenBytes];
    check( netcode_generate_connect_token( 1,
                                           serverAddressStrings,
                                           serverAddressStrings,
                                           InsecureConnectTokenExpirySeconds,
                                           config.timeout,
                                           clientId,
                                           config.protocolId,
                                           privateKey,
                                           userData,
                                           connectToken ) == NETCODE_OK );

    const int NumIterations = 1000;

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    // First use of the token connects.

    check( client.Connect( clientId, connectToken ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );
    check( client.GetClientIndex() == 0 );

    // Free the slot server side and let the client see it, so the second attempt runs against
    // a server with room for it and nothing left over from the first connection.

    server.DisconnectClient( 0 );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() );
    check( server.GetNumConnectedClients() == 0 );

    // Second use of the same token connects nothing. The server ignores the connection
    // requests, so the client runs out of retries and reports a connection request timeout.

    check( client.Connect( clientId, connectToken ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        check( !client.IsConnected() );
        check( server.GetNumConnectedClients() == 0 );
    }

    check( client.ConnectionFailed() );
    check( !client.IsConnected() );
    check( server.GetNumConnectedClients() == 0 );
    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_REQUEST_TIMED_OUT );

    client.Disconnect();
    server.Stop();
}

void test_client_server_messages()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.channel[0].messageSendQueueSize = 32;
    config.channel[0].maxMessagesPerPacket = 8;
    config.channel[0].maxBlockSize = 1024;
    config.channel[0].blockFragmentSize = 200;

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    server.SetLatency( 250 );
    server.SetJitter( 100 );
    server.SetPacketLoss( 25 );
    server.SetDuplicates( 25 );

    for ( int iteration = 0; iteration < 2; ++iteration )
    {
        // connect and wait until connection completes

        check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

        client.SetLatency( 250 );
        client.SetJitter( 100 );
        client.SetPacketLoss( 25 );
        client.SetDuplicates( 25 );

        const int NumIterations = 10000;

        for ( int i = 0; i < NumIterations; ++i )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, clients, 1, servers, 1 );

            if ( client.ConnectionFailed() )
                break;

            if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
                break;
        }

        // verify connection has completed successfully

        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( server.GetNumConnectedClients() == 1 );
        check( client.GetClientIndex() == 0 );
        check( server.IsClientConnected(0) );

        // send a bunch of messages and pump until they are received

        const int NumMessagesSent = config.channel[0].messageSendQueueSize;

        SendClientToServerMessages( client, NumMessagesSent );

        SendServerToClientMessages( server, client.GetClientIndex(), NumMessagesSent );

        int numMessagesReceivedFromClient = 0;
        int numMessagesReceivedFromServer = 0;

        for ( int i = 0; i < NumIterations; ++i )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, clients, 1, servers, 1 );

            if ( !client.IsConnected() )
                break;

            ProcessServerToClientMessages( client, numMessagesReceivedFromServer );

            ProcessClientToServerMessages( server, client.GetClientIndex(), numMessagesReceivedFromClient );

            if ( numMessagesReceivedFromClient == NumMessagesSent && numMessagesReceivedFromServer == NumMessagesSent )
                break;
        }

        check( client.IsConnected() );
        check( server.IsClientConnected( client.GetClientIndex() ) );
        check( numMessagesReceivedFromClient == NumMessagesSent );
        check( numMessagesReceivedFromServer == NumMessagesSent );

        // disconnect and pump until client disconnects

        client.Disconnect();

        for ( int i = 0; i < NumIterations; ++i )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, clients, 1, servers, 1 );

            if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
                break;
        }

        check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );
    }

    server.Stop();
}

class LoopbackTestAdapter : public TestAdapter
{
public:

    Client * client;
    Server * server;

    LoopbackTestAdapter() : client( NULL ), server( NULL ) {}

    void ClientSendLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        server->ProcessLoopbackPacket( clientIndex, packetData, packetBytes, packetSequence );
    }

    void ServerSendLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        (void) clientIndex;
        client->ProcessLoopbackPacket( packetData, packetBytes, packetSequence );
    }
};

void test_server_message_sent_before_client_connects()
{
    // A message the server sends to a slot with no client in it is not delivered to the
    // client that later connects into that slot. Packets travel over loopback, in memory.

    double time = 1000.0;

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    uint8_t userData[256];
    memset( userData, 0, sizeof( userData ) );

    Address serverAddress = TestServerAddress();
    Address clientAddress = TestClientAddress();

    LoopbackTestAdapter loopbackAdapter;

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, loopbackAdapter, time );
    Client client( GetDefaultAllocator(), clientAddress, config, loopbackAdapter, time );

    loopbackAdapter.server = &server;
    loopbackAdapter.client = &client;

    check( server.Start( MaxClients ) );

    check( !server.IsClientConnected( 0 ) );

    TestMessage * before = (TestMessage*) server.CreateMessage( 0, TEST_MESSAGE );
    check( before );
    before->sequence = 1;
    server.SendMessage( 0, 0, before );

    server.ConnectLoopbackClient( 0, 1, userData );
    check( client.ConnectLoopback( 0, 1, MaxClients ) );
    check( server.IsClientConnected( 0 ) );
    check( client.IsConnected() );

    TestMessage * after = (TestMessage*) server.CreateMessage( 0, TEST_MESSAGE );
    check( after );
    after->sequence = 2;
    server.SendMessage( 0, 0, after );

    int numReceived = 0;
    int numReceivedBefore = 0;
    int numReceivedAfter = 0;

    for ( int i = 0; i < 100 && numReceivedAfter == 0; ++i )
    {
        server.SendPackets();
        client.SendPackets();

        server.ReceivePackets();
        client.ReceivePackets();

        time += 0.1;
        server.AdvanceTime( time );
        client.AdvanceTime( time );

        while ( true )
        {
            Message * message = client.ReceiveMessage( 0 );
            if ( !message )
                break;
            check( message->GetType() == TEST_MESSAGE );
            TestMessage * testMessage = (TestMessage*) message;
            if ( testMessage->sequence == 1 )
                numReceivedBefore++;
            if ( testMessage->sequence == 2 )
                numReceivedAfter++;
            numReceived++;
            client.ReleaseMessage( message );
        }
    }

    check( numReceivedBefore == 0 );
    check( numReceivedAfter == 1 );
    check( numReceived == 1 );

    client.DisconnectLoopback();
    server.DisconnectLoopbackClient( 0 );
    server.Stop();
}

class NoSocketTestAdapter : public TestAdapter
{
public:

    bool UseCustomPacketIO() const YOJIMBO_OVERRIDE { return true; }

    void SendPacket( const Address & to, const uint8_t * packetData, int packetBytes ) YOJIMBO_OVERRIDE
    {
        (void) to;
        (void) packetData;
        (void) packetBytes;
    }

    int ReceivePacket( Address & from, uint8_t * packetData, int maxPacketBytes ) YOJIMBO_OVERRIDE
    {
        (void) from;
        (void) packetData;
        (void) maxPacketBytes;
        return 0;
    }
};

void test_client_insecure_connect_token_failure_cleanup()
{
    // A failed insecure connect must leave the client in a clean, destroyable state: nothing
    // allocated by the attempt may survive it, and destroying the client must be safe.
    // The no-socket adapter keeps this to the token path regardless of the host's networking.
    ArmableAllocator allocator;
    {
        Address clientAddress( "0.0.0.0", 0 );
        Address invalidServerAddress;           // ADDRESS_NONE -> token generation fails
        check( !invalidServerAddress.IsValid() );

        ClientServerConfig config;

        uint8_t privateKey[KeyBytes];
        memset( privateKey, 0, KeyBytes );

        NoSocketTestAdapter noSocketAdapter;

        Client client( allocator, clientAddress, config, noSocketAdapter, 100.0 );
        check( !client.InsecureConnect( privateKey, 1, invalidServerAddress ) );
        check( client.ConnectionFailed() );
        check( !client.IsConnected() );
        check( !client.IsConnecting() );
        check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_INVALID_CONNECT_TOKEN );

        // The failed attempt must have released what it allocated before returning, not only
        // when the client is later destroyed.
        check( allocator.GetOutstanding() == 0 );
    }
    check( allocator.GetOutstanding() == 0 );
}

void test_server_message_to_unconnected_slot_not_queued()
{
    // Sending a message to a slot with no client in it leaves nothing queued for that slot.

    double time = 1000.0;

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Address serverAddress = TestServerAddress();

    NoSocketTestAdapter noSocketAdapter;

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, noSocketAdapter, time );

    check( server.Start( MaxClients ) );

    check( !server.IsClientConnected( 0 ) );
    check( !server.HasMessagesToSend( 0, 0 ) );

    TestMessage * message = (TestMessage*) server.CreateMessage( 0, TEST_MESSAGE );
    check( message );
    message->sequence = 1;
    server.SendMessage( 0, 0, message );

    check( !server.HasMessagesToSend( 0, 0 ) );

    // Repeated sends to the same unconnected slot also leave nothing queued.
    for ( int i = 0; i < 16; ++i )
    {
        TestMessage * repeated = (TestMessage*) server.CreateMessage( 0, TEST_MESSAGE );
        check( repeated );
        repeated->sequence = 2 + i;
        server.SendMessage( 0, 0, repeated );

        check( !server.HasMessagesToSend( 0, 0 ) );
    }

    server.Stop();
}

void CreateClients( int numClients, Client ** clients, const Address & address, const ClientServerConfig & config, Adapter & _adapter, double time )
{
    for ( int i = 0; i < numClients; ++i )
    {
        clients[i] = YOJIMBO_NEW( GetDefaultAllocator(), Client, GetDefaultAllocator(), address, config, _adapter, time );
    }
}

void ConnectClients( int numClients, Client ** clients, const uint8_t privateKey[], const Address & serverAddress )
{
    for ( int i = 0; i < numClients; ++i )
    {
        check( clients[i]->InsecureConnect( privateKey, i + 1, serverAddress ) );
        clients[i]->SetLatency( 250 );
        clients[i]->SetJitter( 100 );
        clients[i]->SetPacketLoss( 25 );
        clients[i]->SetDuplicates( 25 );
    }
}

void DestroyClients( int numClients, Client ** clients )
{
    for ( int i = 0; i < numClients; ++i )
    {
        clients[i]->Disconnect();

        YOJIMBO_DELETE( GetDefaultAllocator(), Client, clients[i] );
    }
}

bool AllClientsConnected( int numClients, Server & server, Client ** clients )
{
    if ( server.GetNumConnectedClients() != numClients )
        return false;

    for ( int i = 0; i < numClients; ++i )
    {
        if ( !clients[i]->IsConnected() )
            return false;
    }

    return true;
}

bool AnyClientDisconnected( int numClients, Client ** clients )
{
    for ( int i = 0; i < numClients; ++i )
    {
        if ( clients[i]->IsDisconnected() )
            return true;
    }

    return false;
}

void test_client_server_start_stop_restart()
{
    Address clientAddress( "0.0.0.0", 0 );
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.channel[0].messageSendQueueSize = 32;
    config.channel[0].maxMessagesPerPacket = 8;
    config.channel[0].maxBlockSize = 1024;
    config.channel[0].blockFragmentSize = 200;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    int numClients[] = { 3, 5, 1, 32, 5 };

    const int NumIterations = sizeof( numClients ) / sizeof( int );

    for ( int iteration = 0; iteration < NumIterations; ++iteration )
    {
        numClients[iteration] = numClients[iteration] % MaxClients;
        if ( numClients[iteration] == 0 )
        {
            numClients[iteration] = 1;
        }        

        check( server.Start( numClients[iteration] ) );

        serverAddress = server.GetAddress();

        server.SetLatency( 250 );
        server.SetJitter( 100 );
        server.SetPacketLoss( 25 );
        server.SetDuplicates( 25 );

        Client * clients[MaxClients];

        CreateClients( numClients[iteration], clients, clientAddress, config, adapter, time );

        ConnectClients( numClients[iteration], clients, privateKey, serverAddress );

        while ( true )
        {
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, (Client**) clients, numClients[iteration], servers, 1 );

            if ( AnyClientDisconnected( numClients[iteration], clients ) )
                break;

            if ( AllClientsConnected( numClients[iteration], server, clients ) )
                break;
        }

        check( AllClientsConnected( numClients[iteration], server, clients ) );

        const int NumMessagesSent = config.channel[0].messageSendQueueSize;

        for ( int clientIndex = 0; clientIndex < numClients[iteration]; ++clientIndex )
        {
            SendClientToServerMessages( *clients[clientIndex], NumMessagesSent );
            SendServerToClientMessages( server, clientIndex, NumMessagesSent );
        }

        int numMessagesReceivedFromClient[MaxClients];
        int numMessagesReceivedFromServer[MaxClients];

        memset( numMessagesReceivedFromClient, 0, sizeof( numMessagesReceivedFromClient ) );
        memset( numMessagesReceivedFromServer, 0, sizeof( numMessagesReceivedFromServer ) );

		const int NumInternalIterations = 10000;

        for ( int i = 0; i < NumInternalIterations; ++i )
        {
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, clients, numClients[iteration], servers, 1 );

            bool allMessagesReceived = true;

            for ( int j = 0; j < numClients[iteration]; ++j )
            {
                ProcessServerToClientMessages( *clients[j], numMessagesReceivedFromServer[j] );

                if ( numMessagesReceivedFromServer[j] != NumMessagesSent )
                    allMessagesReceived = false;

                int clientIndex = clients[j]->GetClientIndex();

                ProcessClientToServerMessages( server, clientIndex, numMessagesReceivedFromClient[clientIndex] );

                if ( numMessagesReceivedFromClient[clientIndex] != NumMessagesSent )
                    allMessagesReceived = false;
            }

            if ( allMessagesReceived )
                break;
        }

        for ( int clientIndex = 0; clientIndex < numClients[iteration]; ++clientIndex )
        {
            check( numMessagesReceivedFromClient[clientIndex] == NumMessagesSent );
            check( numMessagesReceivedFromServer[clientIndex] == NumMessagesSent );
        }

        DestroyClients( numClients[iteration], clients );

        server.Stop();
    }
}

void test_client_server_message_failed_to_serialize_reliable_ordered()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.maxPacketSize = 1100;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].maxBlockSize = 1024;
    config.channel[0].blockFragmentSize = 200;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );
    check( client.GetClientIndex() == 0 );
    check( server.IsClientConnected(0) );

    // send a message from client to server that fails to serialize on read, this should disconnect the client from the server

    Message * message = client.CreateMessage( TEST_SERIALIZE_FAIL_ON_READ_MESSAGE );
    check( message );
    client.SendMessage( 0, message );

    for ( int i = 0; i < 256; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_server_client_disconnect_reason()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    // all client slots are cleared to none at server start

    for ( int i = 0; i < MaxClients; ++i )
    {
        check( server.GetClientDisconnectReason( i ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE );
    }

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    const int NumIterations = 10000;

    // connect a client. while it is connected the reason stays none

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( client.IsConnected() );
    check( server.IsClientConnected( 0 ) );
    check( server.GetClientDisconnectReason( 0 ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE );

    // kick the client. the reason is recorded immediately, before the adapter callback fires

    server.DisconnectClient( 0 );

    check( server.GetClientDisconnectReason( 0 ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_KICKED );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() )
            break;
    }

    check( !client.IsConnected() );
    check( server.GetClientDisconnectReason( 0 ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_KICKED );

    // reconnect. a new client connecting to the slot clears the reason back to none

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( client.IsConnected() );
    check( server.IsClientConnected( 0 ) );
    check( server.GetClientDisconnectReason( 0 ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE );

    // clean client-side disconnect is recorded as disconnected, distinct from a timeout

    client.Disconnect();

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( server.GetNumConnectedClients() == 0 )
            break;
    }

    check( server.GetNumConnectedClients() == 0 );
    check( server.GetClientDisconnectReason( 0 ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_DISCONNECTED );

    // reconnect, then let the client go silent. the server times it out and records that
    // as timed out, distinct from a clean disconnect

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( client.IsConnected() );
    check( server.GetClientDisconnectReason( 0 ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, NULL, 0, servers, 1 );

        if ( server.GetNumConnectedClients() == 0 )
            break;
    }

    check( server.GetNumConnectedClients() == 0 );
    check( server.GetClientDisconnectReason( 0 ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_TIMED_OUT );

    client.Disconnect();

    // restarting the server clears all slots back to none

    server.Stop();

    check( server.Start( MaxClients ) );

    for ( int i = 0; i < MaxClients; ++i )
    {
        check( server.GetClientDisconnectReason( i ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE );
    }

    server.Stop();
}

void test_server_client_disconnect_reason_failed_to_serialize()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.maxPacketSize = 1100;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].maxBlockSize = 1024;
    config.channel[0].blockFragmentSize = 200;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );

    // send a message that fails to serialize on read. the server disconnects the client
    // and records the specific channel error as the disconnect reason

    Message * message = client.CreateMessage( TEST_SERIALIZE_FAIL_ON_READ_MESSAGE );
    check( message );
    client.SendMessage( 0, message );

    for ( int i = 0; i < 256; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );
    check( server.GetClientDisconnectReason( 0 ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_FAILED_TO_SERIALIZE );

    client.Disconnect();

    server.Stop();
}

void test_client_disconnect_reason()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    // no disconnect has happened yet

    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE );

    const int NumIterations = 10000;

    // connect. while connected the reason stays none

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( client.IsConnected() );
    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE );

    // deliberate local disconnect is recorded immediately

    client.Disconnect();

    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_DISCONNECTED );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( server.GetNumConnectedClients() == 0 )
            break;
    }

    check( server.GetNumConnectedClients() == 0 );

    // reconnect. a new connect attempt clears the reason back to none

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( client.IsConnected() );

    // when the server kicks us, the client records disconnected by server

    server.DisconnectClient( 0 );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() )
            break;
    }

    check( !client.IsConnected() );
    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_DISCONNECTED_BY_SERVER );

    // connecting to a server that isn't there times out at the connection request stage.
    // the insecure connect token expiry is deliberately much longer than the connection
    // timeout, so this behaves the same way as a secure connect token from a matchmaker.

    server.Stop();

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };

        PumpClientServerUpdate( time, clients, 1, NULL, 0 );

        if ( client.ConnectionFailed() )
            break;
    }

    check( client.ConnectionFailed() );
    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_REQUEST_TIMED_OUT );

    client.Disconnect();
}

void test_client_disconnect_reason_failed_to_serialize()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.maxPacketSize = 1100;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].maxBlockSize = 1024;
    config.channel[0].blockFragmentSize = 200;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );

    // send a message from the server that fails to serialize on read. the client disconnects
    // itself and records the specific channel error as its disconnect reason

    Message * message = server.CreateMessage( 0, TEST_SERIALIZE_FAIL_ON_READ_MESSAGE );
    check( message );
    server.SendMessage( 0, 0, message );

    for ( int i = 0; i < 256; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() )
            break;
    }

    check( !client.IsConnected() );
    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_FAILED_TO_SERIALIZE );

    client.Disconnect();

    server.Stop();
}

void test_client_server_message_failed_to_serialize_unreliable_unordered()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.maxPacketSize = 1100;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[0].maxBlockSize = 1024;
    config.channel[0].blockFragmentSize = 200;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );
    check( client.GetClientIndex() == 0 );
    check( server.IsClientConnected(0) );

    // send a message from client to server that fails to serialize on read, this should disconnect the client from the server

    for ( int i = 0; i < 256; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        Message * message = client.CreateMessage( TEST_SERIALIZE_FAIL_ON_READ_MESSAGE );
        check( message );
        client.SendMessage( 0, message );

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() );
    check( server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_message_exhaust_stream_allocator()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.maxPacketSize = 1100;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].maxBlockSize = 1024;
    config.channel[0].blockFragmentSize = 200;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );
    check( client.GetClientIndex() == 0 );
    check( server.IsClientConnected(0) );

    // send a message from client to server that exhausts the stream allocator on read, this should disconnect the client from the server

    Message * message = client.CreateMessage( TEST_EXHAUST_STREAM_ALLOCATOR_ON_READ_MESSAGE );
    check( message );
    client.SendMessage( 0, message );

    for ( int i = 0; i < 256; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_message_receive_queue_overflow()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.maxPacketSize = 1100;
    config.numChannels = 1;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[0].maxBlockSize = 1024;
    config.channel[0].blockFragmentSize = 200;
    config.channel[0].messageSendQueueSize = 1024;
    config.channel[0].messageReceiveQueueSize = 256;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );
    check( client.GetClientIndex() == 0 );
    check( server.IsClientConnected(0) );

    // send a lot of messages, but don't dequeue them, this tests that the receive queue is able to handle overflow
    // eg. the receiver should detect an error and disconnect the client, because the message is out of bounds.

    const int NumMessagesSent = config.channel[0].messageSendQueueSize;

    SendClientToServerMessages( client, NumMessagesSent );

    for ( int i = 0; i < NumMessagesSent * 4; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };

        PumpClientServerUpdate( time, clients, 1, servers, 1 );
    }

    check( !client.IsConnected() );
    check( server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_reliable_outbound_sequence_outdated()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;
    double deltaTime = 1.0 / 60.0;

    ClientServerConfig config;
    config.numChannels = 2;
    config.timeout = -1;

    yojimbo_assert( config.numChannels <= MaxChannels );

    const int BlockSize = config.channel[0].blockFragmentSize * 2;

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    Client * clients[] = { &client };
    Server * servers[] = { &server };

    const int NumIterations = 50000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( client.ConnectionFailed() )
            break;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );
    check( client.GetClientIndex() == 0 );
    check( server.IsClientConnected(0) );

    int numMessagesSent = 0;

    TestMessage * clientMessage = (TestMessage*) client.CreateMessage( TEST_MESSAGE );
    check( clientMessage );
    client.SendMessage( 0, clientMessage );
    ++numMessagesSent;

    TestBlockMessage * clientBlockMessage = (TestBlockMessage*) client.CreateMessage( TEST_BLOCK_MESSAGE );
    check( clientBlockMessage );
    uint8_t * clientBlockData = client.AllocateBlock( BlockSize );
    memset( clientBlockData, 0, BlockSize );
    client.AttachBlockToMessage( clientBlockMessage, clientBlockData, BlockSize );
    client.SendMessage( 1, clientBlockMessage );
    ++numMessagesSent;

    // Simulate packet sequence being incremented by unreliable messages until it appears outdated.
    for ( int i = 0; i < 32000; ++i ) // Test takes much longer when sending 32768 at once.
    {
        client.SendPackets();
    }
    PumpClientServerUpdate( time, clients, 1, servers, 1, deltaTime );
    for ( int j = 0; j < 768; ++j )
    {
        client.SendPackets();
    }

    TestMessage * clientMessage2 = (TestMessage*) client.CreateMessage( TEST_MESSAGE );
    check( clientMessage2 );
    client.SendMessage( 0, clientMessage2 );
    ++numMessagesSent;

    TestBlockMessage * clientBlockMessage2 = (TestBlockMessage*) client.CreateMessage( TEST_BLOCK_MESSAGE );
    check( clientBlockMessage2 );
    uint8_t * clientBlockData2 = client.AllocateBlock( BlockSize );
    memset( clientBlockData2, 0, BlockSize );
    client.AttachBlockToMessage( clientBlockMessage2, clientBlockData2, BlockSize );
    client.SendMessage( 1, clientBlockMessage2 );
    ++numMessagesSent;

    int numMessagesReceived = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        if ( !client.IsConnected() )
            break;

        PumpClientServerUpdate( time, clients, 1, servers, 1, deltaTime );

        for ( int channelIndex = 0; channelIndex < config.numChannels; ++channelIndex )
        {
            Message * messageFromClient = server.ReceiveMessage( 0, channelIndex );
            if ( messageFromClient )
            {
                server.ReleaseMessage( 0, messageFromClient );
                ++numMessagesReceived;
             }
        }

        if ( numMessagesReceived == numMessagesSent )
            break;
    }

    check( client.IsConnected() );
    check( server.IsClientConnected( client.GetClientIndex() ) );
    check( numMessagesReceived == numMessagesSent );

    client.Disconnect();

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpClientServerUpdate( time, clients, 1, servers, 1 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    server.Stop();
}

// Send and receive more than 65536 reliable-ordered messages over one connection pair so the
// 16-bit message id crosses the 65535 -> 0 wraparound. Every message must still arrive in order.
void test_reliable_ordered_message_id_wraparound()
{
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    // The 16-bit message id wraps from 65535 back to 0 only after more than 65536 messages.
    const int NumMessages = 66000;

    int numMessagesSent = 0;
    int numMessagesReceived = 0;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    const int MaxIterations = 1000000;

    for ( int i = 0; i < MaxIterations && numMessagesReceived < NumMessages; ++i )
    {
        // The send queue only holds messageSendQueueSize entries, so queue what fits and let the
        // receiver drain and ack before adding more. Sending all 66000 at once is not possible.
        while ( numMessagesSent < NumMessages && sender.CanSendMessage( ReliableChannel ) )
        {
            TestMessage * message = (TestMessage*) messageFactory.CreateMessage( TEST_MESSAGE );
            check( message );
            message->sequence = (uint16_t) numMessagesSent;
            sender.SendMessage( ReliableChannel, message );
            ++numMessagesSent;
        }

        // No packet loss here: this test is about the id wrap, not about retransmission.
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( ReliableChannel );
            if ( !message )
                break;

            check( message->GetType() == TEST_MESSAGE );
            check( message->GetId() == (int) (uint16_t) numMessagesReceived );

            TestMessage * testMessage = (TestMessage*) message;
            check( testMessage->sequence == (uint16_t) numMessagesReceived );

            messageFactory.ReleaseMessage( message );

            ++numMessagesReceived;
        }
    }

    check( numMessagesSent == NumMessages );
    check( numMessagesReceived == NumMessages );
    check( sender.GetErrorLevel() == CONNECTION_ERROR_NONE );
    check( receiver.GetErrorLevel() == CONNECTION_ERROR_NONE );
}

void test_single_message_type_reliable()
{
	SingleTestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumMessagesSent = 64;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestMessage * message = (TestMessage*) messageFactory.CreateMessage( SINGLE_TEST_MESSAGE );
        check( message );
        message->sequence = i;
        sender.SendMessage( 0, message );
    }

    int numMessagesReceived = 0;

    const int NumIterations = 1000;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( 0 );
            if ( !message )
                break;

            check( message->GetId() == (int) numMessagesReceived );
            check( message->GetType() == SINGLE_TEST_MESSAGE );

            TestMessage * testMessage = (TestMessage*) message;

            check( testMessage->sequence == numMessagesReceived );

            ++numMessagesReceived;

            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_single_message_type_reliable_blocks()
{
	SingleBlockTestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestBlockMessage * message = (TestBlockMessage*) messageFactory.CreateMessage( SINGLE_BLOCK_TEST_MESSAGE );
        check( message );
        message->sequence = i;
        const int blockSize = 1 + ( ( i * 901 ) % 3333 );
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        for ( int j = 0; j < blockSize; ++j )
            blockData[j] = i + j;
        message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMessage( 0, message );
    }

    int numMessagesReceived = 0;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( 0 );
            if ( !message )
                break;

            check( message->GetId() == (int) numMessagesReceived );

            check( message->GetType() == SINGLE_BLOCK_TEST_MESSAGE );

            TestBlockMessage * blockMessage = (TestBlockMessage*) message;

            check( blockMessage->sequence == uint16_t( numMessagesReceived ) );

            const int blockSize = blockMessage->GetBlockSize();

            check( blockSize == 1 + ( ( numMessagesReceived * 901 ) % 3333 ) );

            const uint8_t * blockData = blockMessage->GetBlockData();

            check( blockData );

            for ( int j = 0; j < blockSize; ++j )
            {
                check( blockData[j] == uint8_t( numMessagesReceived + j ) );
            }

            ++numMessagesReceived;

            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_single_message_type_unreliable()
{
    SingleTestMessageFactory messageFactory( GetDefaultAllocator() );

    double time = 100.0;

    ConnectionConfig connectionConfig;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    Connection sender( GetDefaultAllocator(), messageFactory, connectionConfig, time );
    Connection receiver( GetDefaultAllocator(), messageFactory, connectionConfig, time );

    const int NumIterations = 256;

    const int NumMessagesSent = 16;

    for ( int j = 0; j < NumMessagesSent; ++j )
    {
        TestMessage * message = (TestMessage*) messageFactory.CreateMessage( SINGLE_TEST_MESSAGE );
        check( message );
        message->sequence = j;
        sender.SendMessage( 0, message );
    }

    int numMessagesReceived = 0;

    uint16_t senderSequence = 0;
    uint16_t receiverSequence = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( connectionConfig, time, sender, receiver, senderSequence, receiverSequence, 0.1f, 0 );

        while ( true )
        {
            Message * message = receiver.ReceiveMessage( 0 );
            if ( !message )
                break;

            check( message->GetType() == SINGLE_TEST_MESSAGE );

            TestMessage * testMessage = (TestMessage*) message;

            check( testMessage->sequence == uint16_t( numMessagesReceived ) );

            ++numMessagesReceived;

            messageFactory.ReleaseMessage( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}


void SendClientToServerMessagesSample( Client & client, int numMessagesToSend, int channelIndex = 0 )
{
    for ( int i = 0; i < numMessagesToSend; ++i )
    {
        if ( !client.CanSendMessage( channelIndex ) )
            break;

        TestMessage * message = (TestMessage*) client.CreateMessage( TEST_MESSAGE );
        check( message );
        message->sequence = i;
        client.SendMessage( channelIndex, message );
    }
}

void SendServerToClientMessagesSample( Server & server, int clientIndex, int numMessagesToSend, int channelIndex = 0 )
{
    for ( int i = 0; i < numMessagesToSend; ++i )
    {
        if ( !server.CanSendMessage( clientIndex, channelIndex ) )
            break;

        TestMessage * message = (TestMessage*) server.CreateMessage( clientIndex, TEST_MESSAGE );
        check( message );
        message->sequence = i;
        server.SendMessage( clientIndex, channelIndex, message );
    }
}

void ProcessServerToClientMessagesSample( Client & client, int & numMessagesReceivedFromServer )
{
    while ( true )
    {
        Message * message = client.ReceiveMessage( 0 );

        if ( !message )
            break;

        switch ( message->GetType() )
        {
            case TEST_MESSAGE:
            {
                ++numMessagesReceivedFromServer;
            }
            break;
        }

        client.ReleaseMessage( message );
    }
}

void ProcessClientToServerMessagesSample( Server & server, int clientIndex, int & numMessagesReceivedFromClient )
{
    while ( true )
    {
        Message * message = server.ReceiveMessage( clientIndex, 0 );

        if ( !message )
            break;

        switch ( message->GetType() )
        {
            case TEST_MESSAGE:
            {
                check( !message->IsBlockMessage() );
                ++numMessagesReceivedFromClient;
            }
            break;
        }

        server.ReleaseMessage( clientIndex, message );
    }
}

void test_client_server_messages_network_sim_leak()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.networkSimulator = true;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[0].disableBlocks = true;

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    check( server.Start( MaxClients ) );

    serverAddress = server.GetAddress();

    server.SetLatency( 500 );
    server.SetJitter( 100 );
    server.SetPacketLoss( 5 );
    server.SetDuplicates( 5 );

    for ( int iteration = 0; iteration < 2; ++iteration )
    {
        check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

        client.SetLatency( 500 );
        client.SetJitter( 100 );
        client.SetPacketLoss( 5 );
        client.SetDuplicates( 5 );

        const int NumIterations = 10000;

        for ( int i = 0; i < NumIterations; ++i )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, clients, 1, servers, 1 );

            if ( client.ConnectionFailed() )
                break;

            if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
                break;
        }

        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( server.GetNumConnectedClients() == 1 );
        check( client.GetClientIndex() == 0 );
        check( server.IsClientConnected(0) );

        const int NumMessagesSent = 2000;

        SendClientToServerMessagesSample( client, NumMessagesSent );

        SendServerToClientMessagesSample( server, client.GetClientIndex(), NumMessagesSent );

        int numMessagesReceivedFromClient = 0;
        int numMessagesReceivedFromServer = 0;

        for ( int i = 0; i < 100; ++i )
        {
            if ( !client.IsConnected() )
                break;

            Client * clients[] = { &client };
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, clients, 1, servers, 1 );

            ProcessServerToClientMessagesSample( client, numMessagesReceivedFromServer );
            ProcessClientToServerMessagesSample( server, client.GetClientIndex(), numMessagesReceivedFromClient );
        }

        check( client.IsConnected() );
        check( server.IsClientConnected( client.GetClientIndex() ) );

        client.Disconnect();

        for ( int i = 0; i < NumIterations; ++i )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, clients, 1, servers, 1 );

            if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
                break;
        }

        check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );
    }

    server.Stop();
}

#ifndef YOJIMBO_SYSTEM_DEPS

void test_crypto_aead_vectors()
{
    // Known-answer test for the two AEAD primitives netcode relies on. On Windows
    // this exercises the vendored libsodium in sodium/ (including its SSE2/SSSE3/AVX2
    // paths); on other platforms the system libsodium. Expected ciphertext was
    // generated from libsodium 1.0.20 reference output. Do not edit the arrays by hand.
    // Not built with YOJIMBO_SYSTEM_DEPS: there the system netcode carries its own
    // crypto, and sodium.h is not on the include path.
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

    // ChaCha20-Poly1305 (IETF)
    check( crypto_aead_chacha20poly1305_ietf_encrypt( c, &clen, kat_msg, sizeof( kat_msg ), kat_ad, sizeof( kat_ad ), NULL, kat_npub_ietf, kat_key ) == 0 );
    check( clen == sizeof( kat_ct_ietf ) );
    check( memcmp( c, kat_ct_ietf, (size_t) clen ) == 0 );
    check( crypto_aead_chacha20poly1305_ietf_decrypt( m, &mlen, NULL, c, clen, kat_ad, sizeof( kat_ad ), kat_npub_ietf, kat_key ) == 0 );
    check( mlen == sizeof( kat_msg ) );
    check( memcmp( m, kat_msg, (size_t) mlen ) == 0 );
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
}

#endif // #ifndef YOJIMBO_SYSTEM_DEPS

#define RUN_TEST( test_function )                                           \
    do                                                                      \
    {                                                                       \
        printf( #test_function "\n" );                                      \
        if ( !InitializeYojimbo() )                                         \
        {                                                                   \
            printf( "error: failed to initialize yojimbo\n" );              \
            exit( 1 );                                                      \
        }                                                                   \
        test_function();                                                    \
        ShutdownYojimbo();                                                  \
    }                                                                       \
    while (0)

#ifndef YOJIMBO_SYSTEM_DEPS

// The vendored netcode and reliable test suites are compiled into those libraries with
// NETCODE_ENABLE_TESTS / RELIABLE_ENABLE_TESTS. System-installed libraries (eg. homebrew,
// -DYOJIMBO_SYSTEM_DEPS=ON) are built without them, so these only exist in vendored builds.

extern "C" void netcode_test();

extern "C" void reliable_test();

#endif // #ifndef YOJIMBO_SYSTEM_DEPS

/*
#ifndef SOAK
#define SOAK 1
#endif
*/

#if SOAK
#include <signal.h>
static volatile int quit = 0;
void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}
#endif // #if SOAK


static void PumpClientServer( Client & client, Server & server, double & time )
{
    client.SendPackets();
    server.SendPackets();
    client.ReceivePackets();
    server.ReceivePackets();
    time += 0.1;
    client.AdvanceTime( time );
    server.AdvanceTime( time );
}

void test_server_slot_reused_by_new_client()
{
    const uint64_t clientId = 1;

    Address clientAddress = TestClientAddress();
    Address serverAddress = TestServerAddress();

    double time = 100.0;

    ClientServerConfig config;
    config.networkSimulator = false;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );
    check( server.Start( 1 ) );

    serverAddress = server.GetAddress();

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    check( client.InsecureConnect( privateKey, clientId, serverAddress ) );

    const int NumIterations = 200;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpClientServer( client, server, time );
        if ( client.ConnectionFailed() )
            break;
        if ( !client.IsConnecting() && client.IsConnected() && server.IsClientConnected( 0 ) )
            break;
    }

    check( client.IsConnected() );
    check( server.IsClientConnected( 0 ) );

    const int BlockBytes = 64 * 1024;
    const int MaxBlocks = 512;
    uint8_t * blocks[MaxBlocks];
    int numBlocks = 0;
    for ( ; numBlocks < MaxBlocks; ++numBlocks )
    {
        uint8_t * block = server.AllocateBlock( 0, BlockBytes );
        if ( !block )
            break;
        blocks[numBlocks] = block;
    }
    check( numBlocks > 0 );
    check( numBlocks < MaxBlocks );
    for ( int i = 0; i < numBlocks; ++i )
        server.FreeBlock( 0, blocks[i] );

    bool slotFreed = false;
    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpClientServer( client, server, time );
        if ( !server.IsClientConnected( 0 ) )
        {
            slotFreed = true;
            break;
        }
    }
    check( slotFreed );

    for ( int i = 0; i < NumIterations && client.IsConnected(); ++i )
        PumpClientServer( client, server, time );

    if ( client.IsConnected() )
        client.Disconnect();

    check( client.InsecureConnect( privateKey, clientId + 1, serverAddress ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpClientServer( client, server, time );
        if ( client.ConnectionFailed() )
            break;
        if ( !client.IsConnecting() && client.IsConnected() && server.IsClientConnected( 0 ) )
            break;
    }

    check( client.IsConnected() );
    check( server.IsClientConnected( 0 ) );

    TestMessage * toServer = (TestMessage*) client.CreateMessage( TEST_MESSAGE );
    check( toServer );
    toServer->sequence = 7;
    client.SendMessage( ReliableChannel, toServer );

    TestMessage * toClient = (TestMessage*) server.CreateMessage( 0, TEST_MESSAGE );
    check( toClient );
    toClient->sequence = 11;
    server.SendMessage( 0, ReliableChannel, toClient );

    bool gotFromClient = false;
    bool gotFromServer = false;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpClientServer( client, server, time );

        if ( !client.IsConnected() || !server.IsClientConnected( 0 ) )
            break;

        Message * message = server.ReceiveMessage( 0, ReliableChannel );
        if ( message )
        {
            check( message->GetType() == TEST_MESSAGE );
            check( ((TestMessage*) message)->sequence == 7 );
            server.ReleaseMessage( 0, message );
            gotFromClient = true;
        }

        message = client.ReceiveMessage( ReliableChannel );
        if ( message )
        {
            check( message->GetType() == TEST_MESSAGE );
            check( ((TestMessage*) message)->sequence == 11 );
            client.ReleaseMessage( message );
            gotFromServer = true;
        }

        if ( gotFromClient && gotFromServer )
            break;
    }

    check( client.IsConnected() );
    check( server.IsClientConnected( 0 ) );
    check( gotFromClient );
    check( gotFromServer );

    client.Disconnect();
    server.Stop();
}

static const unsigned int DefaultTestSeed = 0x12345678u;

static unsigned int SelectTestSeed( int argc, char ** argv )
{
    if ( argc > 1 )
        return (unsigned int) strtoul( argv[1], NULL, 0 );

    return DefaultTestSeed;
}

void test_default_seed_is_fixed()
{
    char program[] = "test";
    char seed[] = "7";

    char * no_seed[] = { program, NULL };
    check( SelectTestSeed( 1, no_seed ) == DefaultTestSeed );

    char * explicit_seed[] = { program, seed, NULL };
    check( SelectTestSeed( 2, explicit_seed ) == 7 );
}

void test_fp_contract_off()
{
    // FP-contraction canary. If the compiler contracts a*b + c into a single fused
    // multiply-add (e.g. -ffp-contract=fast / =on, or /fp:fast) the result differs from
    // the two-rounding value yojimbo's wire arithmetic depends on. The inputs are read
    // through volatile so nothing constant-folds.
    //
    // float:  a = b = 0x3F800800 (1 + 2^-12), c = 0xBF801000 (-(1 + 2^-11)).
    //         a*b + c = 0x00000000 with two roundings, 0x33800000 if contracted.
    // double: a = b = 0x3FF0000002000000, c = 0xBFF0000004000000.
    //         a*b + c = 0x0 with two roundings, 0x3C90000000000000 if contracted.

    volatile float af = 0.0f, bf = 0.0f, cf = 0.0f;
    uint32_t u;
    u = 0x3F800800u; memcpy( (void*) &af, &u, sizeof( u ) );
    u = 0x3F800800u; memcpy( (void*) &bf, &u, sizeof( u ) );
    u = 0xBF801000u; memcpy( (void*) &cf, &u, sizeof( u ) );
    float rf = af * bf + cf;
    uint32_t rfu;
    memcpy( &rfu, &rf, sizeof( rfu ) );
    check( rfu == 0x00000000u );    // one rounding (contracted) would give 0x33800000

    volatile double ad = 0.0, bd = 0.0, cd = 0.0;
    uint64_t v;
    v = 0x3FF0000002000000ULL; memcpy( (void*) &ad, &v, sizeof( v ) );
    v = 0x3FF0000002000000ULL; memcpy( (void*) &bd, &v, sizeof( v ) );
    v = 0xBFF0000004000000ULL; memcpy( (void*) &cd, &v, sizeof( v ) );
    double rd = ad * bd + cd;
    uint64_t rdu;
    memcpy( &rdu, &rd, sizeof( rdu ) );
    check( rdu == 0x0000000000000000ULL );    // one rounding would give 0x3C90000000000000
}

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_MAC || YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX

static volatile sig_atomic_t sleep_alarm_fired = 0;

static void sleep_alarm_handler( int )
{
    sleep_alarm_fired = 1;
}

void test_yojimbo_sleep_non_positive_returns_immediately()
{
    struct sigaction previous;
    struct sigaction action;
    memset( &action, 0, sizeof( action ) );
    action.sa_handler = sleep_alarm_handler;
    sigemptyset( &action.sa_mask );
    sigaction( SIGALRM, &action, &previous );

    sleep_alarm_fired = 0;
    alarm( 2 );

    const double start = yojimbo_time();
    yojimbo_sleep( -1.0 );
    const double elapsed = yojimbo_time() - start;

    alarm( 0 );
    sigaction( SIGALRM, &previous, NULL );

    check( !sleep_alarm_fired );
    check( elapsed < 1.0 );
}

void test_yojimbo_sleep_longer_than_one_second()
{
    const double start = yojimbo_time();
    yojimbo_sleep( 1.1 );
    const double elapsed = yojimbo_time() - start;
    check( elapsed >= 1.0 );
}

#endif

int main( int argc, char ** argv )
{
    // The tests are randomized (packet loss, ordering, ids). Use a fixed default seed so a
    // run is reproducible, but let a seed be passed on the command line to explore another
    // run: `./bin/test <seed>`. Print and flush the seed first so it survives even if a
    // later test aborts.
    unsigned int seed = SelectTestSeed( argc, argv );

    // A failed check or assert traps without flushing, so keep stdout unbuffered: the name of
    // the failing test and the failed condition must reach a log that is not a terminal.
    setvbuf( stdout, NULL, _IONBF, 0 );

    printf( "test random seed %u\n", seed );
    fflush( stdout );

    srand( seed );

    printf( "\n" );

#if SOAK
    signal( SIGINT, interrupt_handler );
    int iter = 0;
    while ( true )
#endif // #if SOAK
    {
        {
            printf( "[serialize]\n\n" );

            check( InitializeYojimbo() );

            serialize_test();

            ShutdownYojimbo();
        }

#ifndef YOJIMBO_SYSTEM_DEPS

        {
            printf( "\n[netcode]\n\n" );

            check( InitializeYojimbo() );

            {
                TestSuiteNetworkLock lock;
                netcode_test();
            }

            ShutdownYojimbo();
        }

        {
            printf( "\n[reliable]\n\n" );

            check( InitializeYojimbo() );

            reliable_test();

            ShutdownYojimbo();
        }

#endif // #ifndef YOJIMBO_SYSTEM_DEPS

        printf( "\n[yojimbo]\n\n" );

#ifndef YOJIMBO_SYSTEM_DEPS
        RUN_TEST( test_crypto_aead_vectors );
#endif // #ifndef YOJIMBO_SYSTEM_DEPS
        RUN_TEST( test_fp_contract_off );
        RUN_TEST( test_default_seed_is_fixed );
#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_MAC || YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX
        RUN_TEST( test_yojimbo_sleep_non_positive_returns_immediately );
        RUN_TEST( test_yojimbo_sleep_longer_than_one_second );
#endif
        RUN_TEST( test_servers_bind_distinct_ports );
        RUN_TEST( test_queue );
        RUN_TEST( test_address );
        RUN_TEST( test_address_ipv4_octets );
        RUN_TEST( test_address_malformed_port );
        RUN_TEST( test_address_classification );
        RUN_TEST( test_network_simulator_drains_all_slots );
        RUN_TEST( test_network_simulator_zero_percent_no_loss_or_duplicates );
        RUN_TEST( test_bit_array );
        RUN_TEST( test_sequence_buffer );
        RUN_TEST( test_sequence_buffer_reinsert );
        RUN_TEST( test_sequence_buffer_remove_entries_across_wrap );
        RUN_TEST( test_allocator_tlsf );

        RUN_TEST( test_connection_reliable_ordered_messages );
        RUN_TEST( test_connection_reliable_idle_channel_packet_sequence_wrap );
        RUN_TEST( test_connection_reliable_ordered_blocks );
        RUN_TEST( test_connection_reliable_ordered_blocks_max_size );
        RUN_TEST( test_connection_reliable_ordered_blocks_fragment_count );
        RUN_TEST( test_connection_reliable_ordered_messages_and_blocks );
        RUN_TEST( test_connection_reliable_ordered_messages_and_blocks_multiple_channels );
        RUN_TEST( test_connection_unreliable_unordered_messages );
        RUN_TEST( test_connection_unreliable_unordered_blocks );
        RUN_TEST( test_connection_reject_empty_packet );
        RUN_TEST( test_connection_process_packet_exact_allocation );
        RUN_TEST( test_connection_reject_trailing_bytes );
        RUN_TEST( test_connection_rejected_packet_is_not_acked );
        RUN_TEST( test_connection_failed_message_body_not_partially_applied );
        RUN_TEST( test_connection_failed_unordered_message_body_not_partially_applied );
        RUN_TEST( test_connection_unreliable_rejects_block_fragment );
        RUN_TEST( test_connection_reliable_block_fragment_on_disabled_blocks );
        RUN_TEST( test_connection_reliable_rejects_block_in_message_list );
        RUN_TEST( test_channel_config_fragment_counts_without_overflow );
        RUN_TEST( test_channel_config_max_messages_per_packet_bounds );
        RUN_TEST( test_client_server_config_default_unreliable_channel_validates );
        RUN_TEST( test_connection_unreliable_unordered_block_too_large_for_packet );
        RUN_TEST( test_connection_reliable_ordered_block_fragment_fit_multiple_channels );
        RUN_TEST( test_connection_reliable_ordered_block_fragment_header_headroom );
        RUN_TEST( test_client_server_config_packet_fragment_size_fits_netcode_payload );
        RUN_TEST( test_connection_reliable_ordered_block_ignores_packet_budget );
        RUN_TEST( test_connection_reliable_ordered_message_packet_budget_fit );
        RUN_TEST( test_connection_unreliable_unordered_message_packet_budget_fit );
        RUN_TEST( test_connection_unreliable_send_queue_drains_when_packet_full );
        RUN_TEST( test_connection_reliable_block_fragment_overflow );
        RUN_TEST( test_connection_reliable_block_fragment_message_id_mismatch );
        RUN_TEST( test_connection_reliable_block_fragment_different_message_id );
        RUN_TEST( test_connection_reliable_rejects_short_nonfinal_block_fragment );
        RUN_TEST( test_connection_reliable_rejects_short_middle_block_fragment );
        RUN_TEST( test_connection_reliable_over_budget_packet );
        RUN_TEST( test_connection_reliable_message_alloc_failure );
        RUN_TEST( test_connection_unreliable_message_alloc_failure );
        RUN_TEST( test_connection_generate_packet_channel_data_alloc_failure );
        RUN_TEST( test_message_factory_num_types_within_type_field );
        RUN_TEST( test_message_factory_create_message_alloc_failure );
        RUN_TEST( test_connection_process_packet_channel_data_alloc_failure );

        RUN_TEST( test_reliable_ordered_channel_interface );
        RUN_TEST( test_rtti_across_the_abi_boundary );
        RUN_TEST( test_public_layout_is_configuration_independent );

        RUN_TEST( test_interface_methods_link );
        RUN_TEST( test_server_start_alloc_failure );
        RUN_TEST( test_server_start_factory_failure );
        RUN_TEST( test_server_start_per_client_memory_exhaustion );
        RUN_TEST( test_container_allocation_failure_is_safe );
        RUN_TEST( test_connection_unreliable_channel_alloc_failure_is_safe );
        RUN_TEST( test_client_connect_per_client_memory_exhaustion );
        RUN_TEST( test_client_connect_alloc_failure );
        RUN_TEST( test_client_connect_factory_failure );
        RUN_TEST( test_client_connect_socket_failure_no_crash );
        RUN_TEST( test_client_insecure_connect_token_failure_cleanup );
        RUN_TEST( test_client_is_loopback_when_disconnected );
        RUN_TEST( test_client_connect_twice_with_one_connect_token );
        RUN_TEST( test_client_server_messages );
        RUN_TEST( test_server_message_sent_before_client_connects );
        RUN_TEST( test_server_message_to_unconnected_slot_not_queued );
        RUN_TEST( test_client_server_start_stop_restart );
        RUN_TEST( test_client_server_message_failed_to_serialize_reliable_ordered );
        RUN_TEST( test_server_client_disconnect_reason );
        RUN_TEST( test_server_slot_reused_by_new_client );
        RUN_TEST( test_server_client_disconnect_reason_failed_to_serialize );
        RUN_TEST( test_client_disconnect_reason );
        RUN_TEST( test_client_disconnect_reason_failed_to_serialize );
        RUN_TEST( test_client_server_message_failed_to_serialize_unreliable_unordered );
        RUN_TEST( test_client_server_message_exhaust_stream_allocator );
        RUN_TEST( test_client_server_message_receive_queue_overflow );
        RUN_TEST( test_reliable_outbound_sequence_outdated );
        RUN_TEST( test_reliable_ordered_message_id_wraparound );

        RUN_TEST( test_single_message_type_reliable );
        RUN_TEST( test_single_message_type_reliable_blocks );
        RUN_TEST( test_single_message_type_unreliable );

        RUN_TEST( test_client_server_messages_network_sim_leak );

#if SOAK
        if ( quit )
            break;
        iter++;
        for ( int j = 0; j < iter % 10; ++j )
            printf( "." );
        printf( "\n" );
#endif // #if SOAK
    }

#if SOAK
    if ( quit )
        printf( "\n" );
    else
#else // #if SOAK
        printf( "\n*** ALL TESTS PASS ***\n\n" );
#endif // #if SOAK

    return 0;
}
