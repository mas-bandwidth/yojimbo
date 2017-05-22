/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.

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

#define SERVER 1
#define CLIENT 1
#define LOGGING 0

#include "shared.h"

#define SOAK 0

#if SOAK
#include <signal.h>
static volatile int quit = 0;
void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}
#endif // #if SOAK

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

void test_endian()
{
    uint32_t value = 0x11223344;

    const char * bytes = (const char*) &value;

#if YOJIMBO_LITTLE_ENDIAN

    check( bytes[0] == 0x44 );
    check( bytes[1] == 0x33 );
    check( bytes[2] == 0x22 );
    check( bytes[3] == 0x11 );

#else // #if YOJIMBO_LITTLE_ENDIAN

    check( bytes[3] == 0x44 );
    check( bytes[2] == 0x33 );
    check( bytes[1] == 0x22 );
    check( bytes[0] == 0x11 );

#endif // #if YOJIMBO_LITTLE_ENDIAN
}

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

void test_base64()
{
    const int BufferSize = 256;

    char input[BufferSize];
    char encoded[BufferSize*2];
    char decoded[BufferSize];

    strcpy( input, "[2001:4860:4860::8888]:50000" );

    const int encoded_bytes = base64_encode_string( input, encoded, sizeof( encoded ) );
 
    check( encoded_bytes == (int) strlen( encoded ) + 1 );

    char encoded_expected[] = "WzIwMDE6NDg2MDo0ODYwOjo4ODg4XTo1MDAwMAA=";

    check( strcmp( encoded, encoded_expected ) == 0 );

    const int decoded_bytes = base64_decode_string( encoded, decoded, sizeof( decoded ) );

    check( decoded_bytes == (int) strlen( decoded ) + 1 );

    check( strcmp( input, decoded ) == 0 );

    uint8_t key[KeyBytes];
    random_bytes( key, KeyBytes );

    char base64_key[KeyBytes*2];
    base64_encode_data( key, KeyBytes, base64_key, (int) sizeof( base64_key ) );

    uint8_t decoded_key[KeyBytes];
    base64_decode_data( base64_key, decoded_key, KeyBytes );

    check( memcmp( key, decoded_key, KeyBytes ) == 0 );
}

void test_bitpacker()
{
    const int BufferSize = 256;

    uint8_t buffer[BufferSize];

    BitWriter writer( buffer, BufferSize );

    check( writer.GetData() == buffer );
    check( writer.GetBitsWritten() == 0 );
    check( writer.GetBytesWritten() == 0 );
    check( writer.GetBitsAvailable() == BufferSize * 8 );

    writer.WriteBits( 0, 1 );
    writer.WriteBits( 1, 1 );
    writer.WriteBits( 10, 8 );
    writer.WriteBits( 255, 8 );
    writer.WriteBits( 1000, 10 );
    writer.WriteBits( 50000, 16 );
    writer.WriteBits( 9999999, 32 );
    writer.FlushBits();

    const int bitsWritten = 1 + 1 + 8 + 8 + 10 + 16 + 32;

    check( writer.GetBytesWritten() == 10 );
    check( writer.GetBitsWritten() == bitsWritten );
    check( writer.GetBitsAvailable() == BufferSize * 8 - bitsWritten );

    const int bytesWritten = writer.GetBytesWritten();

    check( bytesWritten == 10 );

    memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );

    BitReader reader( buffer, bytesWritten );

    check( reader.GetBitsRead() == 0 );
    check( reader.GetBitsRemaining() == bytesWritten * 8 );

    uint32_t a = reader.ReadBits( 1 );
    uint32_t b = reader.ReadBits( 1 );
    uint32_t c = reader.ReadBits( 8 );
    uint32_t d = reader.ReadBits( 8 );
    uint32_t e = reader.ReadBits( 10 );
    uint32_t f = reader.ReadBits( 16 );
    uint32_t g = reader.ReadBits( 32 );

    check( a == 0 );
    check( b == 1 );
    check( c == 10 );
    check( d == 255 );
    check( e == 1000 );
    check( f == 50000 );
    check( g == 9999999 );

    check( reader.GetBitsRead() == bitsWritten );
    check( reader.GetBitsRemaining() == bytesWritten * 8 - bitsWritten );
}

const int MaxItems = 11;

struct TestData
{
    TestData()
    {
        memset( this, 0, sizeof( TestData ) );
    }

    int a,b,c;
    uint32_t d : 8;
    uint32_t e : 8;
    uint32_t f : 8;
    bool g;
    int numItems;
    int items[MaxItems];
    float float_value;
    double double_value;
    uint64_t uint64_value;
    uint8_t bytes[17];
    char string[MaxAddressLength];
};

struct TestContext
{
    int min;
    int max;
};

struct TestObject : public Serializable
{
    TestData data;

    void Init()
    {
        data.a = 1;
        data.b = -2;
        data.c = 150;
        data.d = 55;
        data.e = 255;
        data.f = 127;
        data.g = true;

        data.numItems = MaxItems / 2;
        for ( int i = 0; i < data.numItems; ++i )
            data.items[i] = i + 10;     

        data.float_value = 3.1415926f;
        data.double_value = 1 / 3.0;   
        data.uint64_value = 0x1234567898765432L;

        for ( int i = 0; i < (int) sizeof( data.bytes ); ++i )
            data.bytes[i] = rand() % 255;

        strcpy( data.string, "hello world!" );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        const TestContext & context = *(const TestContext*) stream.GetContext();

        serialize_int( stream, data.a, context.min, context.max );
        serialize_int( stream, data.b, context.min, context.max );

        serialize_int( stream, data.c, -100, 10000 );

        serialize_bits( stream, data.d, 6 );
        serialize_bits( stream, data.e, 8 );
        serialize_bits( stream, data.f, 7 );

        serialize_align( stream );

        serialize_bool( stream, data.g );

        serialize_check( stream );

        serialize_int( stream, data.numItems, 0, MaxItems - 1 );
        for ( int i = 0; i < data.numItems; ++i )
            serialize_bits( stream, data.items[i], 8 );

        serialize_float( stream, data.float_value );

        serialize_double( stream, data.double_value );

        serialize_uint64( stream, data.uint64_value );

        serialize_bytes( stream, data.bytes, sizeof( data.bytes ) );

        serialize_string( stream, data.string, sizeof( data.string ) );

        serialize_check( stream );

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();

    bool operator == ( const TestObject & other ) const
    {
        return memcmp( &data, &other.data, sizeof( TestData ) ) == 0;
    }

    bool operator != ( const TestObject & other ) const
    {
        return ! ( *this == other );
    }
};

void test_stream()
{
    const int BufferSize = 1024;

    uint8_t buffer[BufferSize];

    TestContext context;
    context.min = -10;
    context.max = +10;

    WriteStream writeStream( buffer, BufferSize );

    TestObject writeObject;
    writeObject.Init();
    writeStream.SetContext( &context );
    writeObject.Serialize( writeStream );
    writeStream.Flush();

    const int bytesWritten = writeStream.GetBytesProcessed();

    memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );

    TestObject readObject;
    ReadStream readStream( buffer, bytesWritten );
    readStream.SetContext( &context );
    readObject.Serialize( readStream );

    check( readObject == writeObject );
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
            check( allocator.GetErrorLevel() == ALLOCATOR_ERROR_FAILED_TO_ALLOCATE );
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

#if 0 // todo

void PumpClientServerUpdate( double & time, Client ** client, int numClients, Server ** server, int numServers, Transport ** transport, int numTransports, float deltaTime = 0.1f )
{
    for ( int i = 0; i < numClients; ++i )
        client[i]->SendPackets();

    for ( int i = 0; i < numServers; ++i )
        server[i]->SendPackets();

    for ( int i = 0; i < numTransports; ++i )
        transport[i]->WritePackets();

    for ( int i = 0; i < numTransports; ++i )
        transport[i]->ReadPackets();

    for ( int i = 0; i < numClients; ++i )
        client[i]->ReceivePackets();

    for ( int i = 0; i < numServers; ++i )
        server[i]->ReceivePackets();

    for ( int i = 0; i < numClients; ++i )
        client[i]->CheckForTimeOut();

    for ( int i = 0; i < numServers; ++i )
        server[i]->CheckForTimeOut();

    time += deltaTime;

    for ( int i = 0; i < numClients; ++i )
        client[i]->AdvanceTime( time );

    for ( int i = 0; i < numServers; ++i )
        server[i]->AdvanceTime( time );

    for ( int i = 0; i < numTransports; ++i )
        transport[i]->AdvanceTime( time );
}

enum ConnectFlags
{
    CONNECT_FLAG_TOKEN_EXPIRED = 1,
    CONNECT_FLAG_TOKEN_WHITELIST = 1<<1,
};

void ConnectClient( Client & client, uint64_t clientId, const Address serverAddresses[], int numServerAddresses, uint32_t connectFlags = 0 )
{
    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddressesToken;
    Address serverAddressesToken[MaxServersPerConnect];

    memset( connectTokenNonce, 0, NonceBytes );

    uint64_t connectTokenExpireTimestamp;

    int timestampOffset = ( connectFlags & CONNECT_FLAG_TOKEN_EXPIRED ) ? -100 : 0;

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddressesToken, serverAddressesToken, timestampOffset, ( connectFlags & CONNECT_FLAG_TOKEN_WHITELIST ) ? 1000 : -1 ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( clientId, serverAddresses, numServerAddresses, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );
}

void ConnectClient( Client & client, uint64_t clientId, const Address & serverAddress, uint32_t connectFlags = 0 )
{
    ConnectClient( client, clientId, &serverAddress, 1, connectFlags );
}

void test_client_server_connect()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_reconnect()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect the client

    client.Disconnect();

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    // verify the client is able to reconnect to the same server with a new connect token

    clientTransport.Reset();
    serverTransport.Reset();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_keep_alive()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now pump updates for long enough that timeout should kick in, this tests that keep alive is functional

    for ( int i = 0; i < 200; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2, 0.1f );

        if ( !client.IsConnected() || server.GetNumConnectedClients() != 1 )
            break;
    }

    check( client.IsConnected() && server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_client_side_disconnect()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to server

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect client side

    client.Disconnect();

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_server_side_disconnect()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to server

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect server side

    server.DisconnectAllClients();

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_connection_request_timeout()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );

    // connect client to a server that does not exist. verify client connect fails

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { NULL };
        Transport * transports[] = { &clientTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 0, transports, 1 );

        if ( client.ConnectionFailed() )
            break;
    }

    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );
}

void test_client_server_connection_response_timeout()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    server.SetFlags( SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES );

    // connect client to server with ignore challenge response flag set, verify client connect times out on challenge response

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
            break;
    }

    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT );

    client.Disconnect();

    server.Stop();
}

void test_client_server_client_side_timeout()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to server

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now pump only the server. verify that the client times out on the server

    while ( true )
    {
        Client * clients[] = { NULL };
        Server * servers[] = { &server };
        Transport * transports[] = { &serverTransport };

        PumpClientServerUpdate( time, clients, 0, servers, 1, transports, 1 );

        if ( server.GetNumConnectedClients() == 0 )
            break;
    }

    check( server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_server_side_timeout()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to server

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now pump only the client. verify that the client times out

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { NULL };
        Transport * transports[] = { &clientTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 0, transports, 1 );

        if ( !client.IsConnected() )
            break;
    }

    check( !client.IsConnected() );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_TIMEOUT );

    client.Disconnect();

    server.Stop();
}

void CreateClientTransports( int numClients, LocalTransport ** transports, NetworkSimulator & networkSimulator, double time )
{
    for ( int i = 0; i < numClients; ++i )
    {
        Address clientAddress( "::1", ClientPort + i );

        transports[i] = YOJIMBO_NEW( GetDefaultAllocator(), LocalTransport, GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );

        transports[i]->SetNetworkConditions( 250, 250, 5, 10 );
    }
}

void DestroyTransports( int numClients, LocalTransport ** transports )
{
    for ( int i = 0; i < numClients; ++i )
    {
        YOJIMBO_DELETE( GetDefaultAllocator(), LocalTransport, transports[i] );    
    }
}

void CreateClients( int numClients, GameClient ** clients, LocalTransport ** clientTransports, const ClientServerConfig & config, double time )
{
    for ( int i = 0; i < numClients; ++i )
    {
        clients[i] = YOJIMBO_NEW( GetDefaultAllocator(), GameClient, GetDefaultAllocator(), *clientTransports[i], config, time );
    }
}

void ConnectClients( int numClients, GameClient ** clients, const Address & serverAddress )
{
    for ( int i = 0; i < numClients; ++i )
    {
        ConnectClient( *clients[i], 1 + i, serverAddress );
    }
}

void DestroyClients( int numClients, GameClient ** clients )
{
    for ( int i = 0; i < numClients; ++i )
    {
        clients[i]->Disconnect();

        YOJIMBO_DELETE( GetDefaultAllocator(), GameClient, clients[i] );
    }
}

bool AllClientsConnected( int numClients, GameServer & server, GameClient ** clients )
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

bool AnyClientDisconnected( int numClients, GameClient ** clients )
{
    for ( int i = 0; i < numClients; ++i )
    {
        if ( clients[i]->IsDisconnected() )
            return true;
    }

    return false;
}

void test_client_server_server_is_full()
{
    GenerateKey( private_key );

    const int NumClients = 4;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start( NumClients );

    // connect maximum number of clients to server so it's full

    LocalTransport * clientTransports[NumClients+1];
    CreateClientTransports( NumClients + 1, clientTransports, networkSimulator, time );

    GameClient * clients[NumClients+1];
    CreateClients( NumClients + 1, clients, clientTransports, clientServerConfig, time );

    ConnectClients( NumClients, clients, serverAddress );

    while ( true )
    {
        Server * servers[] = { &server };
        Transport * transports[NumClients+1];
        transports[0] = &serverTransport;
        for ( int i = 0; i < NumClients; ++i )
            transports[1+i] = clientTransports[i];

        PumpClientServerUpdate( time, (Client**) clients, NumClients, servers, 1, transports, 1 + NumClients );

        if ( AllClientsConnected( NumClients, server, clients ) )
            break;
    }

    check( AllClientsConnected( NumClients, server, clients ) );

    // now try to connect one more client and verify it's denied with 'server full'

    ConnectClient( *clients[NumClients], 1 + NumClients, serverAddress );

    while ( true )
    {
        Server * servers[] = { &server };
        Transport * transports[NumClients+2];
        transports[0] = &serverTransport;
        for ( int i = 0; i < NumClients + 1; ++i )
            transports[1+i] = clientTransports[i];

        PumpClientServerUpdate( time, (Client**) clients, NumClients + 1, servers, 1, transports, 2 + NumClients );

        check( AllClientsConnected( NumClients, server, clients ) );

        if ( clients[NumClients]->ConnectionFailed() )
            break;
    }

    check( AllClientsConnected( NumClients, server, clients ) );
    check( clients[NumClients]->ConnectionFailed() );
    check( clients[NumClients]->GetClientState() == CLIENT_STATE_CONNECTION_DENIED );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_DENIED_SERVER_IS_FULL ) != 0 );

    DestroyClients( NumClients + 1, clients );

    DestroyTransports( NumClients + 1, clientTransports );

    server.Stop();
}

void test_client_server_connect_token_reuse()
{
    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnect];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    uint64_t connectTokenExpireTimestamp;

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    clientTransport.SetNetworkConditions( 250, 250, 5, 10 );
    serverTransport.SetNetworkConditions( 250, 250, 5, 10 );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect the client, otherwise the server complains about the client id already being connected instead

    client.Disconnect();

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    // now try to connect a second client (different address) using the same token. this connect should be ignored

    Address clientAddress2( "::1", ClientPort + 1 );

    LocalTransport clientTransport2( GetDefaultAllocator(), networkSimulator, clientAddress2, ProtocolId, time );
    
    GameClient client2( GetDefaultAllocator(), clientTransport2, clientServerConfig, time );

    client2.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        Client * clients[] = { &client, &client2 };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &clientTransport2, &serverTransport };

        PumpClientServerUpdate( time, clients, 2, servers, 1, transports, 3 );

        if ( client2.ConnectionFailed() )
            break;
    }

    check( client2.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );
    check( server.GetNumConnectedClients() == 0 );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED ) != 0 );

    client2.Disconnect();

    server.Stop();
}

void test_client_server_connect_token_expired()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress, CONNECT_FLAG_TOKEN_EXPIRED );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
            break;
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );

    client.Disconnect();

    server.Stop();
}

void test_client_server_connect_token_whitelist()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress, CONNECT_FLAG_TOKEN_WHITELIST );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
            break;
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_SERVER_ADDRESS_NOT_IN_WHITELIST ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );

    server.Stop();
}

void test_client_server_connect_token_invalid()
{
    const uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    RandomBytes( connectTokenData, ConnectTokenBytes );
    RandomBytes( connectTokenNonce, NonceBytes );

    GenerateKey( clientToServerKey );
    GenerateKey( serverToClientKey );

    GenerateKey( private_key );

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    clientTransport.SetNetworkConditions( 250, 250, 5, 10 );
    serverTransport.SetNetworkConditions( 250, 250, 5, 10 );

    const int NumIterations = 1000;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    uint64_t connectTokenExpireTimestamp = ::time( NULL ) + 100;

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_DECRYPT_CONNECT_TOKEN ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );

    server.Stop();
}

void test_client_server_connect_address_already_connected()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now try to connect a second client with the same address, but a different connect token and client id. this connect should be ignored

    LocalTransport clientTransport2( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    
    GameClient client2( GetDefaultAllocator(), clientTransport2, clientServerConfig, time );

    ConnectClient( client2, clientId + 1, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client, &client2 };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &clientTransport2, &serverTransport };

        PumpClientServerUpdate( time, clients, 2, servers, 1, transports, 3 );

        if ( client2.ConnectionFailed() )
            break;
    }

    check( client.GetClientState() == CLIENT_STATE_CONNECTED );
    check( client2.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_ADDRESS_ALREADY_CONNECTED ) > 0 );

    client.Disconnect();
    client2.Disconnect();

    server.Stop();
}

void test_client_server_connect_client_id_already_connected()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now try to connect a second client with the same address, but a different connect token and client id. this connect should be ignored

    Address clientAddress2( "::1", ClientPort + 1 );

    LocalTransport clientTransport2( GetDefaultAllocator(), networkSimulator, clientAddress2, ProtocolId, time );
    
    GameClient client2( GetDefaultAllocator(), clientTransport2, clientServerConfig, time );

    ConnectClient( client2, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client, &client2 };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &clientTransport2, &serverTransport };

        PumpClientServerUpdate( time, clients, 2, servers, 1, transports, 3 );

        if ( client2.ConnectionFailed() )
            break;
    }

    check( client.GetClientState() == CLIENT_STATE_CONNECTED );
    check( client2.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED ) > 0 );

    client.Disconnect();
    client2.Disconnect();

    server.Stop();
}

void test_client_server_connect_multiple_servers()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    const int NumServerAddresses = 4;
    Address serverAddresses[NumServerAddresses];
    for ( int i = 0; i < NumServerAddresses; ++i )
    {
        // setup server addresses such that only the last address is valid
        serverAddresses[i] = Address( "::1", ServerPort + NumServerAddresses - 1 - i );
    }

    ConnectClient( client, clientId, serverAddresses, NumServerAddresses );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_user_packets()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now that the client and server are connected lets exchange game packets and count them getting through

    const int clientIndex = server.FindClientIndex( clientAddress );

    check( clientIndex != -1 );

    const int NumUserPackets = 32;

    while ( true )
    {
        client.SendUserPacketToServer();
        server.SendUserPacketToClient( clientIndex );

        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.GetNumUserPacketsReceived() >= NumUserPackets && server.GetNumUserPacketsReceived( clientIndex ) >= NumUserPackets )
            break;
    }

    check( client.GetNumUserPacketsReceived() >= NumUserPackets && server.GetNumUserPacketsReceived( clientIndex ) >= NumUserPackets );

    client.Disconnect();

    server.Stop();
}

#if !YOJIMBO_SECURE_MODE

void test_client_server_insecure_connect()
{
    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    clientTransport.SetNetworkConditions( 250, 250, 5, 10 );
    serverTransport.SetNetworkConditions( 250, 250, 5, 10 );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );

    clientTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );
    serverTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );

    server.SetFlags( SERVER_FLAG_ALLOW_INSECURE_CONNECT );

    server.Start();

    client.InsecureConnect( clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_insecure_connect_multiple_servers()
{
    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    clientTransport.SetNetworkConditions( 250, 250, 5, 10 );
    serverTransport.SetNetworkConditions( 250, 250, 5, 10 );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );

    clientTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );
    serverTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );

    server.SetFlags( SERVER_FLAG_ALLOW_INSECURE_CONNECT );

    server.Start();

    const int NumServerAddresses = 4;
    Address serverAddresses[NumServerAddresses];
    for ( int i = 0; i < NumServerAddresses; ++i )
    {
        // setup server addresses such that only the last address is valid
        serverAddresses[i] = Address( "::1", ServerPort + NumServerAddresses - 1 - i );
    }

    client.InsecureConnect( clientId, serverAddresses, NumServerAddresses );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_insecure_connect_timeout()
{
    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );

    ClientServerConfig config;
    config.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, config, time );

    clientTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );

    client.InsecureConnect( clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { NULL };
        Transport * transports[] = { &clientTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 0, transports, 1 );

        if ( !client.IsConnecting() && client.ConnectionFailed() )
            break;
    }

    check( !client.IsConnecting() );
    check( !client.IsConnected() );
    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_INSECURE_CONNECT_TIMEOUT );
}

void test_client_server_insecure_secure_insecure_secure()
{
    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    uint64_t clientId = 1;

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    clientTransport.SetNetworkConditions( 250, 250, 5, 10 );
    serverTransport.SetNetworkConditions( 250, 250, 5, 10 );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );

    clientTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );
    serverTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );

    server.SetFlags( SERVER_FLAG_ALLOW_INSECURE_CONNECT );

    server.Start();

    for ( int i = 0; i < 4; ++i )
    {
        check( !client.IsConnected() );
        check( !server.IsClientConnected( 0 ) );
        check( server.GetNumConnectedClients() == 0 );

        if ( ( i % 2 ) == 0 )
        {
            client.InsecureConnect( clientId, serverAddress );
        }
        else
        {
            ConnectClient( client, clientId, serverAddress );
        }
        
        while ( true )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };
            Transport * transports[] = { &clientTransport, &serverTransport };

            PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

            if ( client.ConnectionFailed() )
            {
                printf( "error: client connect failed!\n" );
                exit( 1 );
            }

            if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
                break;
        }

        check( !client.IsConnecting() );
        check( client.IsConnected() );
        check( server.GetNumConnectedClients() == 1 );

        client.Disconnect();

        for ( int j = 0; j < 100; ++j )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };
            Transport * transports[] = { &clientTransport, &serverTransport };

            PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

            if ( server.GetNumConnectedClients() == 0 )
                break;
        }

        check( server.GetNumConnectedClients() == 0 );

        clientTransport.Reset();
        serverTransport.Reset();
    }

    server.Stop();
}

#endif // #if !YOJIMBO_SECURE_MODE

#endif

#if 0 // todo

void PumpConnectionUpdate( double & time, Connection & sender, Connection & receiver, Transport & senderTransport, Transport & receiverTransport, float deltaTime = 0.1f )
{
    Packet * senderPacket = sender.GeneratePacket();
    Packet * receiverPacket = receiver.GeneratePacket();

    check( senderPacket );
    check( receiverPacket );

    senderTransport.SendPacket( receiverTransport.GetAddress(), senderPacket, 0, false );
    receiverTransport.SendPacket( senderTransport.GetAddress(), receiverPacket, 0, false );

    senderTransport.WritePackets();
    receiverTransport.WritePackets();

    senderTransport.ReadPackets();
    receiverTransport.ReadPackets();

    while ( true )
    {
        Address from;
        Packet * packet = senderTransport.ReceivePacket( from, NULL );
        if ( !packet )
            break;

        if ( from == receiverTransport.GetAddress() && packet->GetType() == TEST_PACKET_CONNECTION )
            sender.ProcessPacket( (ConnectionPacket*) packet );

        packet->Destroy();
    }

    while ( true )
    {
        Address from;
        Packet * packet = receiverTransport.ReceivePacket( from, NULL );
        if ( !packet )
            break;

        if ( from == senderTransport.GetAddress() && packet->GetType() == TEST_PACKET_CONNECTION )
        {
            receiver.ProcessPacket( (ConnectionPacket*) packet );
        }

        packet->Destroy();
    }

    time += deltaTime;

    sender.AdvanceTime( time );
    receiver.AdvanceTime( time );

    senderTransport.AdvanceTime( time );
    receiverTransport.AdvanceTime( time );
}

void test_connection_reliable_ordered_messages()
{
    TestPacketFactory packetFactory;

    TestMessageFactory messageFactory;

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );
    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext connectionContext;
    connectionContext.messageFactory = &messageFactory;
    connectionContext.connectionConfig = &connectionConfig;

    const int NumMessagesSent = 64;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestMessage * message = (TestMessage*) messageFactory.Create( TEST_MESSAGE );
        check( message );
        message->sequence = i;
        sender.SendMsg( message );
    }

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 1000 );
    networkSimulator.SetDuplicate( 50 );
    networkSimulator.SetPacketLoss( 50 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    double time = 100.0;

    TransportContext transportContext( GetDefaultAllocator(), packetFactory );
    transportContext.connectionContext = &connectionContext;

    LocalTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId, time );
    LocalTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId, time );

    senderTransport.SetContext( transportContext );
    receiverTransport.SetContext( transportContext );

    int numMessagesReceived = 0;

    const int NumIterations = 1000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( time, sender, receiver, senderTransport, receiverTransport );

        while ( true )
        {
            Message * message = receiver.ReceiveMsg();

            if ( !message )
                break;

            check( message->GetId() == (int) numMessagesReceived );
            check( message->GetType() == TEST_MESSAGE );

            TestMessage * testMessage = (TestMessage*) message;

            check( testMessage->sequence == numMessagesReceived );

            ++numMessagesReceived;

            messageFactory.Release( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_blocks()
{
    TestPacketFactory packetFactory;

    TestMessageFactory messageFactory;

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );
    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext connectionContext;
    connectionContext.messageFactory = &messageFactory;
    connectionContext.connectionConfig = &connectionConfig;

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestBlockMessage * message = (TestBlockMessage*) messageFactory.Create( TEST_BLOCK_MESSAGE );
        check( message );
        message->sequence = i;
        const int blockSize = 1 + ( ( i * 901 ) % 3333 );
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        for ( int j = 0; j < blockSize; ++j )
            blockData[j] = i + j;
        message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMsg( message );
    }

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 1000 );
    networkSimulator.SetDuplicate( 50 );
    networkSimulator.SetPacketLoss( 50 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    double time = 100.0;

    TransportContext transportContext( GetDefaultAllocator(), packetFactory );
    transportContext.connectionContext = &connectionContext;

    LocalTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId, time );
    LocalTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId, time );

    senderTransport.SetContext( transportContext );
    receiverTransport.SetContext( transportContext );
    
    const int NumIterations = 10000;

    int numMessagesReceived = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( time, sender, receiver, senderTransport, receiverTransport );

        while ( true )
        {
            Message * message = receiver.ReceiveMsg();

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

            messageFactory.Release( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_messages_and_blocks()
{
    TestPacketFactory packetFactory;

    TestMessageFactory messageFactory;

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext connectionContext;
    connectionContext.messageFactory = &messageFactory;
    connectionContext.connectionConfig = &connectionConfig;

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        if ( rand() % 2 )
        {
            TestMessage * message = (TestMessage*) messageFactory.Create( TEST_MESSAGE );
            check( message );
            message->sequence = i;
            sender.SendMsg( message );
        }
        else
        {
            TestBlockMessage * message = (TestBlockMessage*) messageFactory.Create( TEST_BLOCK_MESSAGE );
            check( message );
            message->sequence = i;
            const int blockSize = 1 + ( ( i * 901 ) % 3333 );
            uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
            for ( int j = 0; j < blockSize; ++j )
                blockData[j] = i + j;
            message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
            sender.SendMsg( message );
        }
    }

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 1000 );
    networkSimulator.SetDuplicate( 50 );
    networkSimulator.SetPacketLoss( 50 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    double time = 100.0;
    
    TransportContext transportContext( GetDefaultAllocator(), packetFactory );
    transportContext.connectionContext = &connectionContext;

    LocalTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId, time );
    LocalTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId, time );

    senderTransport.SetContext( transportContext );
    receiverTransport.SetContext( transportContext );

    const int NumIterations = 10000;

    int numMessagesReceived = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( time, sender, receiver, senderTransport, receiverTransport );

        while ( true )
        {
            Message * message = receiver.ReceiveMsg();

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

            messageFactory.Release( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_messages_and_blocks_multiple_channels()
{
    const int NumChannels = 2;

    TestPacketFactory packetFactory;

    TestMessageFactory messageFactory;

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;
    connectionConfig.numChannels = NumChannels;
    connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    connectionConfig.channel[0].maxMessagesPerPacket = 8;
    connectionConfig.channel[1].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    connectionConfig.channel[1].maxMessagesPerPacket = 8;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext connectionContext;
    connectionContext.messageFactory = &messageFactory;
    connectionContext.connectionConfig = &connectionConfig;

    const int NumMessagesSent = 32;

    for ( int channelId = 0; channelId < NumChannels; ++channelId )
    {
        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            if ( rand() % 2 )
            {
                TestMessage * message = (TestMessage*) messageFactory.Create( TEST_MESSAGE );
                check( message );
                message->sequence = i;
                sender.SendMsg( message, channelId );
            }
            else
            {
                TestBlockMessage * message = (TestBlockMessage*) messageFactory.Create( TEST_BLOCK_MESSAGE );
                check( message );
                message->sequence = i;
                const int blockSize = 1 + ( ( i * 901 ) % 3333 );
                uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
                for ( int j = 0; j < blockSize; ++j )
                    blockData[j] = i + j;
                message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
                sender.SendMsg( message, channelId );
            }
        }
    }

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 1000 );
    networkSimulator.SetDuplicate( 50 );
    networkSimulator.SetPacketLoss( 50 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    double time = 100.0;
    
    TransportContext transportContext( GetDefaultAllocator(), packetFactory );
    transportContext.connectionContext = &connectionContext;

    LocalTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId, time );
    LocalTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId, time );

    senderTransport.SetContext( transportContext );
    receiverTransport.SetContext( transportContext );

    const int NumIterations = 10000;

    int numMessagesReceived[NumChannels];
    memset( numMessagesReceived, 0, sizeof( numMessagesReceived ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( time, sender, receiver, senderTransport, receiverTransport );

        for ( int channelId = 0; channelId < NumChannels; ++channelId )
        {
            while ( true )
            {
                Message * message = receiver.ReceiveMsg( channelId );

                if ( !message )
                    break;

                check( message->GetId() == (int) numMessagesReceived[channelId] );

                switch ( message->GetType() )
                {
                    case TEST_MESSAGE:
                    {
                        TestMessage * testMessage = (TestMessage*) message;

                        check( testMessage->sequence == uint16_t( numMessagesReceived[channelId] ) );

                        ++numMessagesReceived[channelId];
                    }
                    break;

                    case TEST_BLOCK_MESSAGE:
                    {
                        TestBlockMessage * blockMessage = (TestBlockMessage*) message;

                        check( blockMessage->sequence == uint16_t( numMessagesReceived[channelId] ) );

                        const int blockSize = blockMessage->GetBlockSize();

                        check( blockSize == 1 + ( ( numMessagesReceived[channelId] * 901 ) % 3333 ) );
            
                        const uint8_t * blockData = blockMessage->GetBlockData();

                        check( blockData );

                        for ( int j = 0; j < blockSize; ++j )
                        {
                            check( blockData[j] == uint8_t( numMessagesReceived[channelId] + j ) );
                        }

                        ++numMessagesReceived[channelId];
                    }
                    break;
                }

                messageFactory.Release( message );
            }
        }

        bool receivedAllMessages = true;

        for ( int channelId = 0; channelId < NumChannels; ++channelId )
        {
            if ( numMessagesReceived[channelId] != NumMessagesSent )
            {
                receivedAllMessages = false;
                break;
            }
        }

        if ( receivedAllMessages )
            break;
    }

    for ( int channelId = 0; channelId < NumChannels; ++channelId )
    {
        check( numMessagesReceived[channelId] == NumMessagesSent );
    }
}

void test_connection_unreliable_unordered_messages()
{
    TestPacketFactory packetFactory;

    TestMessageFactory messageFactory;

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );
    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext connectionContext;
    connectionContext.messageFactory = &messageFactory;
    connectionContext.connectionConfig = &connectionConfig;

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    double time = 100.0;
   
    TransportContext transportContext( GetDefaultAllocator(), packetFactory );
    transportContext.connectionContext = &connectionContext;

    LocalTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId, time );
    LocalTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId, time );

    senderTransport.SetContext( transportContext );
    receiverTransport.SetContext( transportContext );

    const int NumIterations = 256;

    const int NumMessagesSent = 16;

    for ( int j = 0; j < NumMessagesSent; ++j )
    {
        TestMessage * message = (TestMessage*) messageFactory.Create( TEST_MESSAGE );
        check( message );
        message->sequence = j;
        sender.SendMsg( message );
    }

    int numMessagesReceived = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( time, sender, receiver, senderTransport, receiverTransport );

        while ( true )
        {
            Message * message = receiver.ReceiveMsg();

            if ( !message )
                break;

            check( message->GetType() == TEST_MESSAGE );

            TestMessage * testMessage = (TestMessage*) message;

            check( testMessage->sequence == uint16_t( numMessagesReceived ) );

            ++numMessagesReceived;

            messageFactory.Release( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_unreliable_unordered_blocks()
{
    TestPacketFactory packetFactory;

    TestMessageFactory messageFactory;

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;
    connectionConfig.numChannels = 1;
    connectionConfig.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext connectionContext;
    connectionContext.messageFactory = &messageFactory;
    connectionContext.connectionConfig = &connectionConfig;

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    double time = 100.0;
    
    TransportContext transportContext( GetDefaultAllocator(), packetFactory );
    transportContext.connectionContext = &connectionContext;

    LocalTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId, time );
    LocalTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId, time );

    senderTransport.SetContext( transportContext );
    receiverTransport.SetContext( transportContext );

    const int NumIterations = 256;

    const int NumMessagesSent = 8;

    for ( int j = 0; j < NumMessagesSent; ++j )
    {
        TestBlockMessage * message = (TestBlockMessage*) messageFactory.Create( TEST_BLOCK_MESSAGE );
        check( message );
        message->sequence = j;
        const int blockSize = 1 + ( j * 7 );
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), blockSize );
        for ( int k = 0; k < blockSize; ++k )
            blockData[k] = j + k;
        message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMsg( message );
    }

    int numMessagesReceived = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        PumpConnectionUpdate( time, sender, receiver, senderTransport, receiverTransport );

        while ( true )
        {
            Message * message = receiver.ReceiveMsg();

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

            messageFactory.Release( message );
        }

        if ( numMessagesReceived == NumMessagesSent )
            break;
    }

    check( numMessagesReceived == NumMessagesSent );
}

#endif

#if 0 // todo

void SendClientToServerMessages( Client & client, int numMessagesToSend )
{
    for ( int i = 0; i < numMessagesToSend; ++i )
    {
        if ( rand() % 10 )
        {
            TestMessage * message = (TestMessage*) client.CreateMsg( TEST_MESSAGE );
            check( message );
            message->sequence = i;
            client.SendMsg( message );
        }
        else
        {
            TestBlockMessage * message = (TestBlockMessage*) client.CreateMsg( TEST_BLOCK_MESSAGE );
            check( message );
            message->sequence = i;
            const int blockSize = 1 + ( ( i * 901 ) % 1001 );
            Allocator & allocator = client.GetClientAllocator();
            uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( allocator, blockSize );
            for ( int j = 0; j < blockSize; ++j )
                blockData[j] = i + j;
            message->AttachBlock( allocator, blockData, blockSize );
            client.SendMsg( message );
        }
    }
}

void SendServerToClientMessages( Server & server, int clientIndex, int numMessagesToSend )
{
    for ( int i = 0; i < numMessagesToSend; ++i )
    {
        if ( rand() % 2 )
        {
            TestMessage * message = (TestMessage*) server.CreateMsg( clientIndex, TEST_MESSAGE );
            check( message );
            message->sequence = i;
            server.SendMsg( clientIndex, message );
        }
        else
        {
            TestBlockMessage * message = (TestBlockMessage*) server.CreateMsg( clientIndex, TEST_BLOCK_MESSAGE );
            check( message );
            message->sequence = i;
            const int blockSize = 1 + ( ( i * 901 ) % 1001 );
            Allocator & allocator = server.GetClientAllocator( clientIndex );
            uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( allocator, blockSize );
            for ( int j = 0; j < blockSize; ++j )
                blockData[j] = i + j;
            message->AttachBlock( allocator, blockData, blockSize );
            server.SendMsg( clientIndex, message );
        }
    }
}

void ProcessServerToClientMessages( Client & client, int & numMessagesReceivedFromServer )
{
    while ( true )
    {
        Message * message = client.ReceiveMsg();

        if ( !message )
            break;

        check( message->GetId() == (int) numMessagesReceivedFromServer );
        
        switch ( message->GetType() )
        {
            case TEST_MESSAGE:
            {
                TestMessage * testMessage = (TestMessage*) message;

                check( testMessage->sequence == uint16_t( numMessagesReceivedFromServer ) );

                ++numMessagesReceivedFromServer;
            }
            break;

            case TEST_BLOCK_MESSAGE:
            {
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

        client.ReleaseMsg( message );
    }
}

void ProcessClientToServerMessages( Server & server, int clientIndex, int & numMessagesReceivedFromClient )
{
    while ( true )
    {
        Message * message = server.ReceiveMsg( clientIndex );

        if ( !message )
            break;

        check( message->GetId() == (int) numMessagesReceivedFromClient );

        switch ( message->GetType() )
        {
            case TEST_MESSAGE:
            {
                TestMessage * testMessage = (TestMessage*) message;

                check( testMessage->sequence == uint16_t( numMessagesReceivedFromClient ) );

                ++numMessagesReceivedFromClient;
            }
            break;

            case TEST_BLOCK_MESSAGE:
            {
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

        server.ReleaseMsg( clientIndex, message );
    }
}

void test_client_server_messages()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;
    
    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.connectionConfig.maxPacketSize = 256;
    clientServerConfig.connectionConfig.numChannels = 1;
    clientServerConfig.connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    clientServerConfig.connectionConfig.channel[0].maxBlockSize = 1024;
    clientServerConfig.connectionConfig.channel[0].fragmentSize = 200;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    for ( int iteration = 0; iteration < 2; ++iteration )
    {
        clientTransport.Reset();
        serverTransport.Reset();

        ConnectClient( client, clientId, serverAddress );

        while ( true )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };
            Transport * transports[] = { &clientTransport, &serverTransport };

            PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

            if ( client.ConnectionFailed() )
            {
                printf( "error: client connect failed!\n" );
                exit( 1 );
            }

            if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
                break;
        }

        check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );
        check( client.GetClientIndex() == 0 && server.IsClientConnected(0) );

        const int NumMessagesSent = 64;

        SendClientToServerMessages( client, NumMessagesSent );

        SendServerToClientMessages( server, client.GetClientIndex(), NumMessagesSent );

        int numMessagesReceivedFromClient = 0;
        int numMessagesReceivedFromServer = 0;

        const int NumIterations = 10000;

        for ( int i = 0; i < NumIterations; ++i )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };
            Transport * transports[] = { &clientTransport, &serverTransport };

            PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

            ProcessServerToClientMessages( client, numMessagesReceivedFromServer );

            ProcessClientToServerMessages( server, client.GetClientIndex(), numMessagesReceivedFromClient );

            if ( numMessagesReceivedFromClient == NumMessagesSent && numMessagesReceivedFromServer == NumMessagesSent )
                break;
        }

        check( numMessagesReceivedFromClient == NumMessagesSent );
        check( numMessagesReceivedFromServer == NumMessagesSent );

        client.Disconnect();

        for ( int i = 0; i < NumIterations; ++i )
        {
            Client * clients[] = { &client };
            Server * servers[] = { &server };
            Transport * transports[] = { &clientTransport, &serverTransport };

            PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

            if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
                break;
        }

        check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );
    }

    server.Stop();
}

void test_client_server_start_stop_restart()
{
    GenerateKey( private_key );

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableMessages = false;

    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    serverTransport.SetNetworkConditions( 250, 250, 5, 10 );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );

    int numClients[] = { 2, 5, 1 };

    const int NumIterations = sizeof( numClients ) / sizeof( int );

    for ( int iteration = 0; iteration < NumIterations; ++iteration )
    {
        server.Start( numClients[iteration] );

        LocalTransport * clientTransports[MaxClients];
        CreateClientTransports( numClients[iteration], clientTransports, networkSimulator, time );

        GameClient * clients[MaxClients];
        CreateClients( numClients[iteration], clients, clientTransports, clientServerConfig, time );

        ConnectClients( numClients[iteration], clients, serverAddress );

        while ( true )
        {
            Server * servers[] = { &server };
            Transport * transports[MaxClients+1];
            transports[0] = &serverTransport;
            for ( int i = 0; i < numClients[iteration]; ++i )
                transports[1+i] = clientTransports[i];

            PumpClientServerUpdate( time, (Client**) clients, numClients[iteration], servers, 1, transports, 1 + numClients[iteration] );

            if ( AnyClientDisconnected( numClients[iteration], clients ) )
                break;

            if ( AllClientsConnected( numClients[iteration], server, clients ) )
                break;
        }

        check( AllClientsConnected( numClients[iteration], server, clients ) );

        DestroyClients( numClients[iteration], clients );

        DestroyTransports( numClients[iteration], clientTransports );

        server.Stop();

        serverTransport.Reset();
    }
}

void test_client_server_message_failed_to_serialize_reliable_ordered()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    clientTransport.SetNetworkConditions( 250, 250, 5, 10 );
    serverTransport.SetNetworkConditions( 250, 250, 5, 10 );

    ClientServerConfig clientServerConfig;
    clientServerConfig.connectionConfig.maxPacketSize = 256;
    clientServerConfig.connectionConfig.numChannels = 1;
    clientServerConfig.connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    clientServerConfig.connectionConfig.channel[0].maxBlockSize = 1024;
    clientServerConfig.connectionConfig.channel[0].fragmentSize = 200;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // send a message from client to server that fails to serialize on read, this should disconnect the client from the server

    Message * message = client.CreateMsg( TEST_SERIALIZE_FAIL_ON_READ_MESSAGE );
    check( message );
    client.SendMsg( message );

    for ( int i = 0; i < 256; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_message_failed_to_serialize_unreliable_unordered()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );
    
    double time = 100.0;

    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.connectionConfig.maxPacketSize = 256;
    clientServerConfig.connectionConfig.numChannels = 1;
    clientServerConfig.connectionConfig.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    clientServerConfig.connectionConfig.channel[0].maxBlockSize = 1024;
    clientServerConfig.connectionConfig.channel[0].fragmentSize = 200;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // send a message from client to server that fails to serialize on read, this should disconnect the client from the server

    Message * message = client.CreateMsg( TEST_SERIALIZE_FAIL_ON_READ_MESSAGE );
    check( message );
    client.SendMsg( message );

    for ( int i = 0; i < 256; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_message_exhaust_stream_allocator()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;
    
    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    ClientServerConfig clientServerConfig;
    clientServerConfig.connectionConfig.maxPacketSize = 256;
    clientServerConfig.connectionConfig.numChannels = 1;
    clientServerConfig.connectionConfig.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    clientServerConfig.connectionConfig.channel[0].maxBlockSize = 1024;
    clientServerConfig.connectionConfig.channel[0].fragmentSize = 200;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // send a message from client to server that exhausts the stream allocator on read, this should disconnect the client from the server

    Message * message = client.CreateMsg( TEST_EXHAUST_STREAM_ALLOCATOR_ON_READ_MESSAGE );
    check( message );
    client.SendMsg( message );

    for ( int i = 0; i < 256; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_message_receive_queue_full()
{
    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    double time = 100.0;
    
    LocalTransport clientTransport( GetDefaultAllocator(), networkSimulator, clientAddress, ProtocolId, time );
    LocalTransport serverTransport( GetDefaultAllocator(), networkSimulator, serverAddress, ProtocolId, time );

    clientTransport.SetNetworkConditions( 250, 250, 5, 10 );
    serverTransport.SetNetworkConditions( 250, 250, 5, 10 );

    ClientServerConfig clientServerConfig;
    clientServerConfig.connectionConfig.maxPacketSize = 256;
    clientServerConfig.connectionConfig.numChannels = 1;
    clientServerConfig.connectionConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    clientServerConfig.connectionConfig.channel[0].fragmentSize = 200;
    clientServerConfig.connectionConfig.channel[0].sendQueueSize = 1024;
    clientServerConfig.connectionConfig.channel[0].receiveQueueSize = 16;         // note: tiny receive queue

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig, time );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig, time );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    ConnectClient( client, clientId, serverAddress );

    while ( true )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    const int NumMessagesSent = 64;

    SendClientToServerMessages( client, NumMessagesSent );

    SendServerToClientMessages( server, client.GetClientIndex(), NumMessagesSent );

    int numMessagesReceivedFromClient = 0;
    int numMessagesReceivedFromServer = 0;

    const int NumIterations = 10000;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Client * clients[] = { &client };
        Server * servers[] = { &server };
        Transport * transports[] = { &clientTransport, &serverTransport };

        PumpClientServerUpdate( time, clients, 1, servers, 1, transports, 2 );

        ProcessServerToClientMessages( client, numMessagesReceivedFromServer );

        ProcessClientToServerMessages( server, client.GetClientIndex(), numMessagesReceivedFromClient );

        if ( numMessagesReceivedFromClient == NumMessagesSent && numMessagesReceivedFromServer == NumMessagesSent )
            break;
    }

    check( numMessagesReceivedFromClient == NumMessagesSent );
    check( numMessagesReceivedFromServer == NumMessagesSent );

    client.Disconnect();

    server.Stop();
}

#endif

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

int main()
{
    srand( time( NULL ) );

    printf( "\n" );
 
#if SOAK
    signal( SIGINT, interrupt_handler );    
    int iter = 0;
    while ( true )
#endif // #if SOAK
    {
        {
            printf( "[netcode.io]\n\n" );

            check( InitializeYojimbo() );
            
            netcode_test();

            ShutdownYojimbo();
        }

        {
            printf( "\n[reliable.io]\n\n" );

            check( InitializeYojimbo() );
            
            reliable_test();

            ShutdownYojimbo();
        }

        printf( "\n[yojimbo]\n\n" );

        RUN_TEST( test_endian );
        RUN_TEST( test_queue );
        RUN_TEST( test_base64 );
        RUN_TEST( test_bitpacker );
        RUN_TEST( test_stream );
        RUN_TEST( test_address );
        RUN_TEST( test_bit_array );
        RUN_TEST( test_sequence_buffer );
        RUN_TEST( test_allocator_tlsf );

        /*
        RUN_TEST( test_connection_reliable_ordered_messages );
        RUN_TEST( test_connection_reliable_ordered_blocks );
        RUN_TEST( test_connection_reliable_ordered_messages_and_blocks );
        RUN_TEST( test_connection_reliable_ordered_messages_and_blocks_multiple_channels );
        RUN_TEST( test_connection_unreliable_unordered_messages );
        RUN_TEST( test_connection_unreliable_unordered_blocks );

        RUN_TEST( test_client_server_messages );
        RUN_TEST( test_client_server_start_stop_restart );
        RUN_TEST( test_client_server_message_failed_to_serialize_reliable_ordered );
        RUN_TEST( test_client_server_message_failed_to_serialize_unreliable_unordered );
        RUN_TEST( test_client_server_message_exhaust_stream_allocator );
        RUN_TEST( test_client_server_message_receive_queue_full );
        */

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
        printf( "\ntest stopped\n" );
    else
#else // #if SOAK
        printf( "\n*** ALL TESTS PASS ***\n\n" );
#endif // #if SOAK

    return 0;
}
