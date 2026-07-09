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

#include "yojimbo.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "shared.h"
#include "serialize.h"
#include <sodium.h>

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
        TestSequenceData * entry = sequence_buffer.Insert( i, true );
        check( entry );
        entry->sequence = i;
        check( sequence_buffer.GetSequence() == i + 1 );
    }

    sequence_buffer.Reset();

    check( sequence_buffer.GetSequence() == 0 );

    for ( int i = 0; i < Size; ++i )
        check( sequence_buffer.Find(i) == NULL );
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
            receiver.ProcessPacket( NULL, senderSequence, packetData, packetBytes );
            sender.ProcessAcks( &senderSequence, 1 );
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
template <typename Stream> static bool SerializeRawBlockFragmentPacket( Stream & stream, int maxFragmentsPerBlock, int blockFragmentSize, int numFragments, int fragmentId, int fragmentSize )
{
    int numChannelEntries = 1;
    serialize_int( stream, numChannelEntries, 0, 1 );   // numChannels == 1, so channelIndex is not serialized

    bool blockMessage = true;
    serialize_bool( stream, blockMessage );

    uint32_t messageId = 0;
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

    client.Connect( 1, connectToken );          // must not crash - CreateClient failed
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
    Address clientAddress( "0.0.0.0", ClientPort );
    Client client( GetDefaultAllocator(), clientAddress, config, adapter, 100.0 );

    check( !client.IsLoopback() );              // must not crash; not connected -> not a loopback client
    check( client.GetClientIndex() == -1 );
    check( !client.IsConnected() );
}

void test_client_server_messages()
{
    const uint64_t clientId = 1;

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

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

    server.Start( MaxClients );

    server.SetLatency( 250 );
    server.SetJitter( 100 );
    server.SetPacketLoss( 25 );
    server.SetDuplicates( 25 );

    for ( int iteration = 0; iteration < 2; ++iteration )
    {
        // connect and wait until connection completes

        client.InsecureConnect( privateKey, clientId, serverAddress );

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
        clients[i]->InsecureConnect( privateKey, i + 1, serverAddress );
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
    Address serverAddress( "127.0.0.1", ServerPort );

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

        server.Start( numClients[iteration] );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

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

    server.Start( MaxClients );

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

    double time = 100.0;

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    server.Start( MaxClients );

    // all client slots are cleared to none at server start

    for ( int i = 0; i < MaxClients; ++i )
    {
        check( server.GetClientDisconnectReason( i ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE );
    }

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    const int NumIterations = 10000;

    // connect a client. while it is connected the reason stays none

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    // clean client-side disconnect is recorded as a transport-level disconnect

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

    // restarting the server clears all slots back to none

    server.Stop();

    server.Start( MaxClients );

    for ( int i = 0; i < MaxClients; ++i )
    {
        check( server.GetClientDisconnectReason( i ) == YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE );
    }

    server.Stop();
}

void test_server_client_disconnect_reason_failed_to_serialize()
{
    const uint64_t clientId = 1;

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

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

    server.Start( MaxClients );

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

    double time = 100.0;

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    server.Start( MaxClients );

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    // no disconnect has happened yet

    check( client.GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE );

    const int NumIterations = 10000;

    // connect. while connected the reason stays none

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

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

    server.Start( MaxClients );

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

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

    server.Start( MaxClients );

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

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

    server.Start( MaxClients );

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

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

    server.Start( MaxClients );

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

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

    server.Start( MaxClients );

    client.InsecureConnect( privateKey, clientId, serverAddress );

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

    Address clientAddress( "0.0.0.0", ClientPort );
    Address serverAddress( "127.0.0.1", ServerPort );

    double time = 100.0;

    ClientServerConfig config;
    config.networkSimulator = true;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    Client client( GetDefaultAllocator(), clientAddress, config, adapter, time );

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    server.Start( MaxClients );

    server.SetLatency( 500 );
    server.SetJitter( 100 );
    server.SetPacketLoss( 5 );
    server.SetDuplicates( 5 );

    for ( int iteration = 0; iteration < 2; ++iteration )
    {
        client.InsecureConnect( privateKey, clientId, serverAddress );

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

void test_crypto_aead_vectors()
{
    // Known-answer test for the two AEAD primitives netcode relies on. On Windows
    // this exercises the vendored libsodium in sodium/ (including its SSE2/SSSE3/AVX2
    // paths); on other platforms the system libsodium. Expected ciphertext was
    // generated from libsodium 1.0.20 reference output. Do not edit the arrays by hand.
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

extern "C" void netcode_test();

extern "C" void reliable_test();

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

int main( int argc, char ** argv )
{
    // The tests are randomized (packet loss, ordering, ids). Seed rand() from the clock by
    // default, but let a seed be passed on the command line so a flaky failure can be
    // reproduced exactly: rerun `./bin/test <seed>` with the value printed below. Print and
    // flush the seed first so it survives even if a later test aborts.
    unsigned int seed = ( argc > 1 ) ? (unsigned int) strtoul( argv[1], NULL, 0 )
                                     : (unsigned int) time( NULL );

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

        {
            printf( "\n[netcode]\n\n" );

            check( InitializeYojimbo() );

            netcode_test();

            ShutdownYojimbo();
        }

        {
            printf( "\n[reliable]\n\n" );

            check( InitializeYojimbo() );

            reliable_test();

            ShutdownYojimbo();
        }

        printf( "\n[yojimbo]\n\n" );

        RUN_TEST( test_crypto_aead_vectors );
        RUN_TEST( test_queue );
        RUN_TEST( test_address );
        RUN_TEST( test_network_simulator_drains_all_slots );
        RUN_TEST( test_bit_array );
        RUN_TEST( test_sequence_buffer );
        RUN_TEST( test_allocator_tlsf );

        RUN_TEST( test_connection_reliable_ordered_messages );
        RUN_TEST( test_connection_reliable_ordered_blocks );
        RUN_TEST( test_connection_reliable_ordered_blocks_max_size );
        RUN_TEST( test_connection_reliable_ordered_messages_and_blocks );
        RUN_TEST( test_connection_reliable_ordered_messages_and_blocks_multiple_channels );
        RUN_TEST( test_connection_unreliable_unordered_messages );
        RUN_TEST( test_connection_unreliable_unordered_blocks );
        RUN_TEST( test_connection_reject_empty_packet );
        RUN_TEST( test_connection_unreliable_rejects_block_fragment );
        RUN_TEST( test_connection_reliable_block_fragment_on_disabled_blocks );
        RUN_TEST( test_connection_reliable_block_fragment_overflow );
        RUN_TEST( test_connection_reliable_over_budget_packet );
        RUN_TEST( test_connection_reliable_message_alloc_failure );
        RUN_TEST( test_connection_unreliable_message_alloc_failure );
        RUN_TEST( test_connection_generate_packet_channel_data_alloc_failure );
        RUN_TEST( test_message_factory_create_message_alloc_failure );
        RUN_TEST( test_connection_process_packet_channel_data_alloc_failure );

        RUN_TEST( test_client_connect_socket_failure_no_crash );
        RUN_TEST( test_client_is_loopback_when_disconnected );
        RUN_TEST( test_client_server_messages );
        RUN_TEST( test_client_server_start_stop_restart );
        RUN_TEST( test_client_server_message_failed_to_serialize_reliable_ordered );
        RUN_TEST( test_server_client_disconnect_reason );
        RUN_TEST( test_server_client_disconnect_reason_failed_to_serialize );
        RUN_TEST( test_client_disconnect_reason );
        RUN_TEST( test_client_disconnect_reason_failed_to_serialize );
        RUN_TEST( test_client_server_message_failed_to_serialize_unreliable_unordered );
        RUN_TEST( test_client_server_message_exhaust_stream_allocator );
        RUN_TEST( test_client_server_message_receive_queue_overflow );
        RUN_TEST( test_reliable_outbound_sequence_outdated );

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
