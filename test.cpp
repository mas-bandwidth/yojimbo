/*
    Yojimbo Unit Tests.

    Copyright © 2016 - 2019, The Network Protocol Company, Inc.

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

#if YOJIMBO_WITH_MBEDTLS

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

#endif // #if YOJIMBO_WITH_MBEDTLS

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

void test_bits_required()
{
    check( bits_required( 0, 0 ) == 0 );
    check( bits_required( 0, 1 ) == 1 );
    check( bits_required( 0, 2 ) == 2 );
    check( bits_required( 0, 3 ) == 2 );
    check( bits_required( 0, 4 ) == 3 );
    check( bits_required( 0, 5 ) == 3 );
    check( bits_required( 0, 6 ) == 3 );
    check( bits_required( 0, 7 ) == 3 );
    check( bits_required( 0, 8 ) == 4 );
    check( bits_required( 0, 255 ) == 8 );
    check( bits_required( 0, 65535 ) == 16 );
    check( bits_required( 0, 4294967295 ) == 32 );
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
    uint32_t v32;
    uint64_t v64;
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
        data.v32 = UINT32_MAX;
        data.v64 = UINT64_MAX;

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

        serialize_varint32( stream, data.v32 );
        serialize_varint64( stream, data.v64 );

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

    WriteStream writeStream( GetDefaultAllocator(), buffer, BufferSize );

    TestObject writeObject;
    writeObject.Init();
    writeStream.SetContext( &context );
    writeObject.Serialize( writeStream );
    writeStream.Flush();

    const int bytesWritten = writeStream.GetBytesProcessed();

    memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );

    TestObject readObject;
    ReadStream readStream( GetDefaultAllocator(), buffer, bytesWritten );
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
        if ( random_int( 0, 100 ) >= packetLossPercent )
        {
            receiver.ProcessPacket( NULL, senderSequence, packetData, packetBytes );
            sender.ProcessAcks( &senderSequence, 1 );
        }
    }

    if ( receiver.GeneratePacket( NULL, receiverSequence, packetData, connectionConfig.maxPacketSize, packetBytes ) )
    {
        if ( random_int( 0, 100 ) >= packetLossPercent )
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
        sender.SendMessage( 0, message );
    }

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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
        sender.SendMessage( 0, message );
    }

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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
            sender.SendMessage( 0, message );
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
            sender.SendMessage( 0, message );
        }
    }

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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

void SendClientToServerMessages( Client & client, int numMessagesToSend, int channelIndex = 0 )
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

void SendServerToClientMessages( Server & server, int clientIndex, int numMessagesToSend, int channelIndex = 0 )
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

void ProcessServerToClientMessages( Client & client, int & numMessagesReceivedFromServer )
{
    while ( true )
    {
        Message * message = client.ReceiveMessage( 0 );

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

void ProcessClientToServerMessages( Server & server, int clientIndex, int & numMessagesReceivedFromClient )
{
    while ( true )
    {
        Message * message = server.ReceiveMessage( clientIndex, 0 );

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

    client.SetLatency( 250 );
    client.SetJitter( 100 );
    client.SetPacketLoss( 25 );
    client.SetDuplicates( 25 );

    server.SetLatency( 250 );
    server.SetJitter( 100 );
    server.SetPacketLoss( 25 );
    server.SetDuplicates( 25 );

    for ( int iteration = 0; iteration < 2; ++iteration )
    {
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

        const int NumMessagesSent = config.channel[0].messageSendQueueSize;

        SendClientToServerMessages( client, NumMessagesSent );

        SendServerToClientMessages( server, client.GetClientIndex(), NumMessagesSent );

        int numMessagesReceivedFromClient = 0;
        int numMessagesReceivedFromServer = 0;

        for ( int i = 0; i < NumIterations; ++i )
        {
            if ( !client.IsConnected() )
                break;

            Client * clients[] = { &client };
            Server * servers[] = { &server };

            PumpClientServerUpdate( time, clients, 1, servers, 1 );

            ProcessServerToClientMessages( client, numMessagesReceivedFromServer );

            ProcessClientToServerMessages( server, client.GetClientIndex(), numMessagesReceivedFromClient );

            if ( numMessagesReceivedFromClient == NumMessagesSent && numMessagesReceivedFromServer == NumMessagesSent )
                break;
        }

        check( client.IsConnected() );
        check( server.IsClientConnected( client.GetClientIndex() ) );
        check( numMessagesReceivedFromClient == NumMessagesSent );
        check( numMessagesReceivedFromServer == NumMessagesSent );

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
        clients[i]->SetLatency( 250 );
        clients[i]->SetJitter( 100 );
        clients[i]->SetPacketLoss( 25 );
        clients[i]->SetDuplicates( 25 );
    }
}

void ConnectClients( int numClients, Client ** clients, const uint8_t privateKey[], const Address & serverAddress )
{
    for ( int i = 0; i < numClients; ++i )
    {
        clients[i]->InsecureConnect( privateKey, i + 1, serverAddress );
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

    server.Start( MaxClients );

    server.SetLatency( 250 );
    server.SetJitter( 100 );
    server.SetPacketLoss( 25 );
    server.SetDuplicates( 25 );

    int numClients[] = { 3, 5, 1 };

    const int NumIterations = sizeof( numClients ) / sizeof( int );

    for ( int iteration = 0; iteration < NumIterations; ++iteration )
    {
        server.Start( numClients[iteration] );

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

void test_reliable_fragment_overflow_bug()
{
    double time = 100.0;
    
    ClientServerConfig config;
    config.numChannels = 2;
    config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[0].packetBudget = 8000;
    config.channel[1].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[1].packetBudget = -1;
    
    uint8_t privateKey[KeyBytes];
    memset(privateKey, 0, KeyBytes);
    Server server(GetDefaultAllocator(), privateKey, Address("127.0.0.1", ServerPort), config, adapter, time);
    
    server.Start(MaxClients);
    check(server.IsRunning());
    
    uint64_t clientId = 0;
    random_bytes((uint8_t*)&clientId, 8);
    
    Client client(GetDefaultAllocator(), Address("0.0.0.0"), config, adapter, time);
    
    Address serverAddress("127.0.0.1", ServerPort);
    
    client.InsecureConnect(privateKey, clientId, serverAddress);
    
    Client * clients[] = { &client };
    Server * servers[] = { &server };
    
    while (true)
    {
        PumpClientServerUpdate(time, clients, 1, servers, 1);
        
        if (client.ConnectionFailed())
            break;
        
        if (!client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1)
            break;
    }
    
    check(!client.IsConnecting());
    check(client.IsConnected());
    check(server.GetNumConnectedClients() == 1);
    check(client.GetClientIndex() == 0);
    check(server.IsClientConnected(0));
    
    PumpClientServerUpdate(time, clients, 1, servers, 1);
    check(!client.IsDisconnected());
    
    TestBlockMessage *testBlockMessage = (TestBlockMessage *)client.CreateMessage(TEST_BLOCK_MESSAGE);
    uint8_t * blockData = client.AllocateBlock(7169);
    memset( blockData, 0, 7169 );
    client.AttachBlockToMessage(testBlockMessage, blockData, 7169);
    client.SendMessage(0, testBlockMessage);
    
    testBlockMessage = (TestBlockMessage *)client.CreateMessage(TEST_BLOCK_MESSAGE);
    blockData = client.AllocateBlock(1024);
    memset( blockData, 0, 1024 );
    client.AttachBlockToMessage(testBlockMessage, blockData, 1024);
    client.SendMessage(1, testBlockMessage);
    
    PumpClientServerUpdate(time, clients, 1, servers, 1);
    
    PumpClientServerUpdate(time, clients, 1, servers, 1);
    
    PumpClientServerUpdate(time, clients, 1, servers, 1);
    check(!client.IsDisconnected());
    
    Message *message = server.ReceiveMessage(0, 0);
    check(message);
    check(message->GetType() == TEST_BLOCK_MESSAGE);
    server.ReleaseMessage(0, message);
    
    message = server.ReceiveMessage(0, 1);
    check(message);
    check(message->GetType() == TEST_BLOCK_MESSAGE);
    server.ReleaseMessage(0, message);
    
    client.Disconnect();

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

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

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

#ifndef SOAK
#define SOAK 0
#endif

#if SOAK
#include <signal.h>
static volatile int quit = 0;
void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}
#endif // #if SOAK

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
#if YOJIMBO_WITH_MBEDTLS
		RUN_TEST( test_base64 );
#endif // #if YOJIMBO_WITH_MBEDTLS
        RUN_TEST( test_bitpacker );
        RUN_TEST( test_bits_required );
        RUN_TEST( test_stream );
        RUN_TEST( test_address );
        RUN_TEST( test_bit_array );
        RUN_TEST( test_sequence_buffer );
        RUN_TEST( test_allocator_tlsf );

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
        RUN_TEST( test_client_server_message_receive_queue_overflow );
        RUN_TEST( test_reliable_fragment_overflow_bug );
        RUN_TEST( test_single_message_type_reliable );
        RUN_TEST( test_single_message_type_reliable_blocks );
        RUN_TEST( test_single_message_type_unreliable );
        
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
