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

#include "yojimbo.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define SERVER 1
#define CLIENT 1
#define MATCHER 1

#include "shared.h"

static LocalMatcher matcher;

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
    printf( "test_endian\n" );

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
    printf( "test_queue\n" );

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
    printf( "test_base64\n" );

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
    GenerateKey( key );

    char base64_key[KeyBytes*2];
    base64_encode_data( key, KeyBytes, base64_key, (int) sizeof( base64_key ) );

    uint8_t decoded_key[KeyBytes];
    base64_decode_data( base64_key, decoded_key, KeyBytes );

    check( memcmp( key, decoded_key, KeyBytes ) == 0 );
}

void test_bitpacker()
{
    printf( "test_bitpacker\n" );

    const int BufferSize = 256;

    uint8_t buffer[BufferSize];

    BitWriter writer( buffer, BufferSize );

    check( writer.GetData() == buffer );
    check( writer.GetTotalBytes() == BufferSize );
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
    check( writer.GetTotalBytes() == BufferSize );
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

        serialize_check( stream, "test object serialize check" );

        serialize_int( stream, data.numItems, 0, MaxItems - 1 );
        for ( int i = 0; i < data.numItems; ++i )
            serialize_bits( stream, data.items[i], 8 );

        serialize_float( stream, data.float_value );

        serialize_double( stream, data.double_value );

        serialize_uint64( stream, data.uint64_value );

        serialize_bytes( stream, data.bytes, sizeof( data.bytes ) );

        serialize_string( stream, data.string, sizeof( data.string ) );

        serialize_check( stream, "end of test object" );

        return true;
    }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();

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
    printf( "test_stream\n" );

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

struct TestPacketA : public Packet
{
    int a,b,c;

    TestPacketA()
    {
        a = 1;
        b = 2;
        c = 3;        
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -20, 20 );
        serialize_int( stream, c, -30, 30 );
        return true;
    }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestPacketB : public Packet
{
    int x,y;

    TestPacketB()
    {
        x = 0;
        y = 1;
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, x, -5, +5 );
        serialize_int( stream, y, -5, +5 );
        return true;
    }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestPacketC : public Packet
{
    uint8_t data[16];

    TestPacketC()
    {
        for ( int i = 0; i < (int) sizeof( data ); ++i )
            data[i] = (uint8_t) i;
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        for ( int i = 0; i < (int) sizeof( data ); ++i )
            serialize_int( stream, data[i], 0, 255 );
        return true;
    }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum TestPacketTypes
{
    TEST_PACKET_A,
    TEST_PACKET_B,
    TEST_PACKET_C,
    TEST_PACKET_CONNECTION,
    TEST_PACKET_NUM_TYPES
};

YOJIMBO_PACKET_FACTORY_START( TestPacketFactory, PacketFactory, TEST_PACKET_NUM_TYPES );

    YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_A, TestPacketA );
    YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_B, TestPacketB );
    YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_C, TestPacketC );
    YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_CONNECTION, ConnectionPacket );

YOJIMBO_PACKET_FACTORY_FINISH();

void test_packets()
{
    printf( "test_packets\n" );

    TestPacketFactory packetFactory;

    TestPacketA * a = (TestPacketA*) packetFactory.CreatePacket( TEST_PACKET_A );
    TestPacketB * b = (TestPacketB*) packetFactory.CreatePacket( TEST_PACKET_B );
    TestPacketC * c = (TestPacketC*) packetFactory.CreatePacket( TEST_PACKET_C );

    check( a );
    check( b );
    check( c );

    check( a->GetType() == TEST_PACKET_A );
    check( b->GetType() == TEST_PACKET_B );
    check( c->GetType() == TEST_PACKET_C );

    a->Destroy();
    b->Destroy();
    c->Destroy();
}

void test_address_ipv4()
{
    printf( "test_address_ipv4\n" );

    char buffer[MaxAddressLength];

    {
        Address address( 127, 0, 0, 1 );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 0 );
        check( address.GetAddress4() == 0x100007f );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "127.0.0.1" ) == 0 );
    }

    {
        Address address( 127, 0, 0, 1, 1000 );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 1000 );
        check( address.GetAddress4() == 0x100007f );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "127.0.0.1:1000" ) == 0 );
    }

    {
        Address address( "127.0.0.1" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 0 );
        check( address.GetAddress4() == 0x100007f );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "127.0.0.1" ) == 0 );
    }

    {
        Address address( "127.0.0.1:65535" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 65535 );
        check( address.GetAddress4() == 0x100007f );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "127.0.0.1:65535" ) == 0 );
    }

    {
        Address address( "10.24.168.192:3000" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 3000 );
        check( address.GetAddress4() == 0xc0a8180a );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "10.24.168.192:3000" ) == 0 );
    }

    {
        Address address( "255.255.255.255:65535" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 65535 );
        check( address.GetAddress4() == 0xffffffff );
        check( strcmp( address.ToString( buffer, MaxAddressLength ), "255.255.255.255:65535" ) == 0 );
    }
}

inline uint16_t test_htons( uint16_t input )
{
#if YOJIMBO_LITTLE_ENDIAN
    return ( ( input & 0xFF ) << 8 ) | 
           ( ( input & 0xFF00 ) >> 8 );
#else
    return input;
#endif // #if YOJIMBO_LITTLE_ENDIAN
}

void test_address_ipv6()
{
    printf( "test_address_ipv6\n" );

    char buffer[MaxAddressLength];

    // without port numbers

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7] );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "::1" ) == 0 );
    }

    // same addresses but with port numbers

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6[0], address6[1], address6[2], address6[2],
                         address6[4], address6[5], address6[6], address6[7], 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6, 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6, 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, MaxAddressLength ), "[::1]:65535" ) == 0 );
    }

    // parse addresses from strings (no ports)

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

    // parse addresses from strings (with ports)

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

void test_packet_sequence()
{
    printf( "test_packet_sequence\n" );

    {
        uint64_t sequence = 0x00001100223344;

        uint8_t prefix_byte;
        uint8_t sequence_bytes[8];
        int num_sequence_bytes;

        yojimbo::compress_packet_sequence( sequence, prefix_byte, num_sequence_bytes, sequence_bytes );

        check( prefix_byte == ( 1 | (1<<1) | (1<<3) ) );

        check( num_sequence_bytes == 4 );

        check( sequence_bytes[0] == 0x11 );
        check( sequence_bytes[1] == 0x22 );
        check( sequence_bytes[2] == 0x33 );
        check( sequence_bytes[3] == 0x44 );

        int decoded_num_sequence_bytes = yojimbo::get_packet_sequence_bytes( prefix_byte );

        check( decoded_num_sequence_bytes == num_sequence_bytes );

        uint64_t decoded_sequence = yojimbo::decompress_packet_sequence( prefix_byte, sequence_bytes );

        check( decoded_sequence == sequence );
    }
}

#include <sodium.h>

void test_encrypt_and_decrypt()
{
    printf( "test_encrypt_and_decrypt\n" );

    using namespace yojimbo;

    uint8_t packet[1024];
  
    int packet_length = 1;
    memset( packet, 0, sizeof( packet ) );
    packet[0] = 1;  
  
    uint8_t key[KeyBytes];
    uint8_t nonce[NonceBytes];

    memset( key, 1, sizeof( key ) );
    memset( nonce, 1, sizeof( nonce ) );

    uint8_t encrypted_packet[2048];

    int encrypted_length;
    if ( !Encrypt( packet, packet_length, encrypted_packet, encrypted_length, nonce, key ) )
    {
        printf( "error: failed to encrypt\n" );
        exit(1);
    }

    const int expected_encrypted_length = 17;
    const uint8_t expected_encrypted_packet[] = { 0xfa, 0x6c, 0x91, 0xf7, 0xef, 0xdc, 0xed, 0x22, 0x09, 0x23, 0xd5, 0xbf, 0xa1, 0xe9, 0x17, 0x70, 0x14 };
    if ( encrypted_length != expected_encrypted_length || memcmp( expected_encrypted_packet, encrypted_packet, encrypted_length ) != 0 )
    {
        printf( "error: packet encryption failed\n" );
        exit(1);
    }

    uint8_t decrypted_packet[2048];
    int decrypted_length;
    if ( !Decrypt( encrypted_packet, encrypted_length, decrypted_packet, decrypted_length, nonce, key ) )
    {
        printf( "error: failed to decrypt\n" );
        exit(1);
    }

    if ( decrypted_length != packet_length || memcmp( packet, decrypted_packet, packet_length ) != 0 )
    {
        printf( "error: decrypted packet does not match original packet\n" );
        exit(1);
    }
}

void test_encryption_manager()
{
    printf( "test_encryption_manager\n" );

    EncryptionManager encryptionManager;

    struct EncryptionMapping
    {
        Address address;
        uint8_t sendKey[KeyBytes];
        uint8_t receiveKey[KeyBytes];
    };

    const int NumEncryptionMappings = 5;

    EncryptionMapping encryptionMapping[NumEncryptionMappings];

    double time = 0.0;

    for ( int i = 0; i < NumEncryptionMappings; ++i )
    {
        encryptionMapping[i].address = Address( "::1", 20000 + i );
        GenerateKey( encryptionMapping[i].sendKey );
        GenerateKey( encryptionMapping[i].receiveKey );

        check( encryptionManager.GetSendKey( encryptionMapping[i].address, time ) == NULL );
        check( encryptionManager.GetReceiveKey( encryptionMapping[i].address, time ) == NULL );

        check( encryptionManager.AddEncryptionMapping( encryptionMapping[i].address, encryptionMapping[i].sendKey, encryptionMapping[i].receiveKey, time ) );

        const uint8_t * sendKey = encryptionManager.GetSendKey( encryptionMapping[i].address, time );
        const uint8_t * receiveKey = encryptionManager.GetReceiveKey( encryptionMapping[i].address, time );

        check( sendKey );
        check( receiveKey );

        check( memcmp( sendKey, encryptionMapping[i].sendKey, KeyBytes ) == 0 );
        check( memcmp( receiveKey, encryptionMapping[i].receiveKey, KeyBytes ) == 0 );
    }

    check( encryptionManager.RemoveEncryptionMapping( Address( "::1", 50000 ), time ) == false );

    check( encryptionManager.RemoveEncryptionMapping( encryptionMapping[0].address, time ) );
    check( encryptionManager.RemoveEncryptionMapping( encryptionMapping[NumEncryptionMappings-1].address, time ) );

    for ( int i = 0; i < NumEncryptionMappings; ++i )
    {
        const uint8_t * sendKey = encryptionManager.GetSendKey( encryptionMapping[i].address, time );
        const uint8_t * receiveKey = encryptionManager.GetReceiveKey( encryptionMapping[i].address, time );

        if ( i != 0 && i != NumEncryptionMappings -1 )
        {
            check( sendKey );
            check( receiveKey );

            check( memcmp( sendKey, encryptionMapping[i].sendKey, KeyBytes ) == 0 );
            check( memcmp( receiveKey, encryptionMapping[i].receiveKey, KeyBytes ) == 0 );
        }
        else
        {
            check( !sendKey );
            check( !receiveKey );
        }
    }

    check( encryptionManager.AddEncryptionMapping( encryptionMapping[0].address, encryptionMapping[0].sendKey, encryptionMapping[0].receiveKey, time ) );
    check( encryptionManager.AddEncryptionMapping( encryptionMapping[NumEncryptionMappings-1].address, encryptionMapping[NumEncryptionMappings-1].sendKey, encryptionMapping[NumEncryptionMappings-1].receiveKey, time ) );

    for ( int i = 0; i < NumEncryptionMappings; ++i )
    {
        const uint8_t * sendKey = encryptionManager.GetSendKey( encryptionMapping[i].address, time );
        const uint8_t * receiveKey = encryptionManager.GetReceiveKey( encryptionMapping[i].address, time );

        check( sendKey );
        check( receiveKey );

        check( memcmp( sendKey, encryptionMapping[i].sendKey, KeyBytes ) == 0 );
        check( memcmp( receiveKey, encryptionMapping[i].receiveKey, KeyBytes ) == 0 );
    }

    time = DefaultEncryptionMappingTimeout * 2;

    for ( int i = 0; i < NumEncryptionMappings; ++i )
    {
        const uint8_t * sendKey = encryptionManager.GetSendKey( encryptionMapping[i].address, time );
        const uint8_t * receiveKey = encryptionManager.GetReceiveKey( encryptionMapping[i].address, time );

        check( !sendKey );
        check( !receiveKey );
    }

    for ( int i = 0; i < NumEncryptionMappings; ++i )
    {
        encryptionMapping[i].address = Address( "::1", 20000 + i );
        GenerateKey( encryptionMapping[i].sendKey );
        GenerateKey( encryptionMapping[i].receiveKey );

        check( encryptionManager.GetSendKey( encryptionMapping[i].address, time ) == NULL );
        check( encryptionManager.GetReceiveKey( encryptionMapping[i].address, time ) == NULL );

        check( encryptionManager.AddEncryptionMapping( encryptionMapping[i].address, encryptionMapping[i].sendKey, encryptionMapping[i].receiveKey, time ) );

        const uint8_t * sendKey = encryptionManager.GetSendKey( encryptionMapping[i].address, time );
        const uint8_t * receiveKey = encryptionManager.GetReceiveKey( encryptionMapping[i].address, time );

        check( sendKey );
        check( receiveKey );

        check( memcmp( sendKey, encryptionMapping[i].sendKey, KeyBytes ) == 0 );
        check( memcmp( receiveKey, encryptionMapping[i].receiveKey, KeyBytes ) == 0 );
    }
}

void test_client_server_tokens()
{
    printf( "test_client_server_tokens\n" );

    const uint32_t ProtocolId = 0x12398137;
    const int ServerPort = 30000;
    const int ClientPort = 40000;

    uint8_t key[KeyBytes];

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t challengeTokenData[ChallengeTokenBytes];
    
    uint8_t connectTokenNonce[NonceBytes];
    uint8_t challengeTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );
    memset( challengeTokenNonce, 0, NonceBytes );

    GenerateKey( key );

    numServerAddresses = 1;
    serverAddresses[0] = Address( "::1", ServerPort );

    memset( connectTokenNonce, 0, NonceBytes );

    uint64_t connectTokenExpireTimestamp;

    {
        ConnectToken token;
        GenerateConnectToken( token, clientId, numServerAddresses, serverAddresses, ProtocolId, 10 );

        connectTokenExpireTimestamp = token.expireTimestamp;

        char json[2048];

        check( WriteConnectTokenToJSON( token, json, sizeof( json ) ) );

        check( strlen( json ) > 0 );

        ConnectToken readToken;
        check( ReadConnectTokenFromJSON( json, readToken ) );
        check( token == readToken );

        memcpy( clientToServerKey, token.clientToServerKey, KeyBytes );
        memcpy( serverToClientKey, token.serverToClientKey, KeyBytes );

        if ( !EncryptConnectToken( token, connectTokenData, connectTokenNonce, key ) )
        {
            printf( "error: failed to encrypt connect token\n" );
            exit( 1 );
        }
    }

    ConnectToken connectToken;
    if ( !DecryptConnectToken( connectTokenData, connectToken, connectTokenNonce, key, connectTokenExpireTimestamp ) )
    {
        printf( "error: failed to decrypt connect token\n" );
        exit( 1 );
    }

    check( connectToken.clientId == clientId );
    check( connectToken.numServerAddresses == 1 );
    check( connectToken.serverAddresses[0] == Address( "::1", ServerPort ) );
    check( memcmp( connectToken.clientToServerKey, clientToServerKey, KeyBytes ) == 0 );
    check( memcmp( connectToken.serverToClientKey, serverToClientKey, KeyBytes ) == 0 );

    Address clientAddress( "::1", ClientPort );

    ChallengeToken challengeToken;
    if ( !GenerateChallengeToken( connectToken, connectTokenData, challengeToken ) )
    {
        printf( "error: failed to generate challenge token\n" );
        exit( 1 );
    }

    if ( !EncryptChallengeToken( challengeToken, challengeTokenData, NULL, 0, challengeTokenNonce, key ) )
    {
        printf( "error: failed to encrypt challenge token\n" );
        exit( 1 );
    }

    ChallengeToken decryptedChallengeToken;
    if ( !DecryptChallengeToken( challengeTokenData, decryptedChallengeToken, NULL, 0, challengeTokenNonce, key ) )
    {
        printf( "error: failed to decrypt challenge token\n" );
        exit( 1 );
    }

    check( challengeToken.clientId == clientId );
    check( memcmp( challengeToken.connectTokenMac, connectTokenData, MacBytes ) == 0 );
    check( memcmp( challengeToken.clientToServerKey, clientToServerKey, KeyBytes ) == 0 );
    check( memcmp( challengeToken.serverToClientKey, serverToClientKey, KeyBytes ) == 0 );
}

struct TestMessage : public Message
{
    uint16_t sequence;

    TestMessage()
    {
        sequence = 0;
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );

        int numBits = GetNumBitsForMessage( sequence );
        int numWords = numBits / 32;
        uint32_t dummy = 0;
        for ( int i = 0; i < numWords; ++i )
            serialize_bits( stream, dummy, 32 );
        int numRemainderBits = numBits - numWords * 32;
        if ( numRemainderBits > 0 )
            serialize_bits( stream, dummy, numRemainderBits );

        return true;
    }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestBlockMessage : public BlockMessage
{
    uint16_t sequence;

    TestBlockMessage()
    {
        sequence = 0;
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );
        return true;
    }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum MessageType
{
    TEST_MESSAGE,
    TEST_BLOCK_MESSAGE,
    TEST_NUM_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START( TestMessageFactory, MessageFactory, TEST_NUM_MESSAGE_TYPES );
    YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_MESSAGE, TestMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_BLOCK_MESSAGE, TestBlockMessage );
YOJIMBO_MESSAGE_FACTORY_FINISH();

#if 0

// todo: integrate this functionality into the shared.h GameServer and GameClient

class GameServer : public Server
{
    uint32_t m_gamePacketSequence;
    uint64_t m_numGamePacketsReceived[MaxClients];

    void Initialize()
    {
        SetPrivateKey( private_key );
        m_gamePacketSequence = 0;
        memset( m_numGamePacketsReceived, 0, sizeof( m_numGamePacketsReceived ) );
    }

public:

    explicit GameServer( Allocator & allocator, Transport & transport, const ClientServerConfig & config ) : Server( allocator, transport, config )
    {
        Initialize();
    }

    MessageFactory * CreateMessageFactory( Allocator & allocator, ServerResourceType /*type*/, int /*clientIndex*/ )
    {
        return YOJIMBO_NEW( allocator, TestMessageFactory, allocator );
    }

    PacketFactory * CreatePacketFactory( Allocator & allocator, ServerResourceType /*type*/, int /*clientIndex*/ )
    {
        return YOJIMBO_NEW( allocator, GamePacketFactory, allocator );
    }

    void OnClientConnect( int clientIndex )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < GetMaxClients() );
        m_numGamePacketsReceived[clientIndex] = 0;

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            GetClientAddress( clientIndex ).ToString( addressString, MaxAddressLength );
            printf( "client %d connected (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
        }
    }

    void OnClientDisconnect( int clientIndex )
    {
        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            GetClientAddress( clientIndex ).ToString( addressString, MaxAddressLength );
            printf( "client %d disconnected (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
        }
    }

    void OnClientTimedOut( int clientIndex )
    {
        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            GetClientAddress( clientIndex ).ToString( addressString, MaxAddressLength );
            printf( "client %d timed out (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
        }
    }

    void OnPacketSent( int packetType, const Address & to, bool immediate )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:        packetTypeString = "connection denied";     break;
            case CLIENT_SERVER_PACKET_CHALLENGE:                packetTypeString = "challenge";             break;
            case CLIENT_SERVER_PACKET_HEARTBEAT:                packetTypeString = "heartbeat";             break;
            case CLIENT_SERVER_PACKET_DISCONNECT:               packetTypeString = "disconnect";            break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            to.ToString( addressString, MaxAddressLength );
            printf( "server sent %s packet to %s%s\n", packetTypeString, addressString, immediate ? " (immediate)" : "" );
        }
    }

    void OnPacketReceived( int packetType, const Address & from, uint64_t /*sequence*/ )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:         packetTypeString = "connection request";        break;
            case CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:         packetTypeString = "challenge response";        break;
            case CLIENT_SERVER_PACKET_HEARTBEAT:                  packetTypeString = "heartbeat";                 break;  
            case CLIENT_SERVER_PACKET_DISCONNECT:                 packetTypeString = "disconnect";                break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            from.ToString( addressString, MaxAddressLength );
            printf( "server received '%s' packet from %s\n", packetTypeString, addressString );
        }
    }

    uint64_t GetNumGamePacketsReceived( int clientIndex ) const
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < GetMaxClients() );
        return m_numGamePacketsReceived[clientIndex];
    }

    void SendGamePacketToClient( int clientIndex )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < GetMaxClients() );
        assert( IsClientConnected( clientIndex ) );
        GamePacket * packet = (GamePacket*) GetTransport()->CreatePacket( GAME_PACKET );
        assert( packet );
        packet->Initialize( ++m_gamePacketSequence );
        SendPacketToConnectedClient( clientIndex, packet );
    }

    bool ProcessGamePacket( int clientIndex, Packet * packet, uint64_t /*sequence*/ )
    {
        if ( packet->GetType() == GAME_PACKET )
        {
            m_numGamePacketsReceived[clientIndex]++;
            return true;
        }

        return false;
    }
};

class GameClient : public Client
{
    uint64_t m_numGamePacketsReceived;
    uint32_t m_gamePacketSequence;

    void Initialize()
    {
        m_numGamePacketsReceived = 0;
        m_gamePacketSequence = 0;
    }

public:

    explicit GameClient( Allocator & allocator, Transport & transport, const ClientServerConfig & config ) : Client( allocator, transport, config )
    {
        Initialize();
    }

    void SendGamePacketToServer()
    {
        GamePacket * packet = (GamePacket*) m_transport->CreatePacket( GAME_PACKET );
        assert( packet );
        packet->Initialize( ++m_gamePacketSequence );
        SendPacketToServer( packet );
    }

    MessageFactory * CreateMessageFactory( Allocator & allocator )
    {
        return YOJIMBO_NEW( allocator, TestMessageFactory, allocator );
    }

    void OnConnect( const Address & address )
    {
        m_numGamePacketsReceived = 0;

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            address.ToString( addressString, MaxAddressLength );
            printf( "client connecting to %s\n", addressString );
        }
    }

    void OnClientStateChange( int previousState, int currentState )
    {
        if ( verbose_logging )
        {
            assert( previousState != currentState );
            const char * previousStateString = GetClientStateName( previousState );
            const char * currentStateString = GetClientStateName( currentState );
            printf( "client changed state from '%s' to '%s'\n", previousStateString, currentStateString );

            if ( currentState == CLIENT_STATE_CONNECTED )
            {
                printf( "client connected as client %d\n", GetClientIndex() );
            }
        }
    }

    void OnDisconnect()
    {
        if ( verbose_logging )
        {
            printf( "client disconnected\n" );
        }
    }

    PacketFactory * CreatePacketFactory( Allocator & allocator )
    {
        return YOJIMBO_NEW( allocator, GamePacketFactory, allocator );
    }

    void OnPacketSent( int packetType, const Address & to, bool immediate )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:         packetTypeString = "connection request";        break;
            case CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:         packetTypeString = "challenge response";        break;
            case CLIENT_SERVER_PACKET_HEARTBEAT:                  packetTypeString = "heartbeat";                 break;  
            case CLIENT_SERVER_PACKET_DISCONNECT:                 packetTypeString = "disconnect";                break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            to.ToString( addressString, MaxAddressLength );
            printf( "client sent %s packet to %s%s\n", packetTypeString, addressString, immediate ? " (immediate)" : "" );
        }
    }

    void OnPacketReceived( int packetType, const Address & from, uint64_t /*sequence*/ )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:          packetTypeString = "connection denied";     break;
            case CLIENT_SERVER_PACKET_CHALLENGE:                  packetTypeString = "challenge";             break;
            case CLIENT_SERVER_PACKET_HEARTBEAT:                  packetTypeString = "heartbeat";             break;
            case CLIENT_SERVER_PACKET_DISCONNECT:                 packetTypeString = "disconnect";            break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            from.ToString( addressString, MaxAddressLength );
            printf( "client received %s packet from %s\n", packetTypeString, addressString );
        }
    }

    uint64_t GetNumGamePacketsReceived() const
    {
        return m_numGamePacketsReceived;
    }

    bool ProcessGamePacket( Packet * packet, uint64_t /*sequence*/ )
    {
        if ( packet->GetType() == GAME_PACKET )
        {
            m_numGamePacketsReceived++;
            return true;
        }

        return false;
    }
};

#endif

class TestNetworkSimulator : public NetworkSimulator
{
public:

    TestNetworkSimulator() : NetworkSimulator( GetDefaultAllocator() )
    {
        SetJitter( 250 );
        SetLatency( 256 );
        SetDuplicates( 5 );
        SetPacketLoss( 10 );
    }   
};

class TestNetworkTransport : public SimulatorTransport
{   
public:

    TestNetworkTransport( NetworkSimulator & networkSimulator, const Address & address ) 
        : SimulatorTransport( GetDefaultAllocator(), networkSimulator, address, ProtocolId ) {}

    ~TestNetworkTransport()
    {
        ClearSendQueue();
        ClearReceiveQueue();
    }
};

void test_unencrypted_packets()
{
    printf( "test_unencrypted_packets\n" );

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    clientTransport.SetPacketFactory( packetFactory );
    serverTransport.SetPacketFactory( packetFactory );

    // make sure that encrypted packet types will *not* be received if they are sent as unencrypted

    clientTransport.EnablePacketEncryption();
    serverTransport.EnablePacketEncryption();

    check( clientTransport.IsEncryptedPacketType( CLIENT_SERVER_PACKET_HEARTBEAT ) );
    check( serverTransport.IsEncryptedPacketType( CLIENT_SERVER_PACKET_HEARTBEAT ) );

    clientTransport.DisableEncryptionForPacketType( CLIENT_SERVER_PACKET_HEARTBEAT );

    check( !clientTransport.IsEncryptedPacketType( CLIENT_SERVER_PACKET_HEARTBEAT ) );

    const int NumIterations = 32;

    int numPacketsReceived = 0;

    double time = 0.0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Packet * sendPacket = clientTransport.CreatePacket( CLIENT_SERVER_PACKET_HEARTBEAT );
        check( sendPacket );
        clientTransport.SendPacket( serverAddress, sendPacket, 0, false );

        clientTransport.WritePackets();

        serverTransport.ReadPackets();

        while ( true )
        {
            Address address;
            uint64_t sequence;
            Packet * packet = serverTransport.ReceivePacket( address, &sequence );
            if ( !packet )
                break;
            if ( packet->GetType() == CLIENT_SERVER_PACKET_HEARTBEAT )
                numPacketsReceived++;
        }

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );

        time += 0.1;
    }

    check( numPacketsReceived == 0 );
}

void test_allocator_tlsf()
{
    printf( "test_allocator_tlsf\n" );

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
        blockData[i] = (uint8_t*) allocator.Allocate( BlockSize );
        
        if ( !blockData[i] )
        {
            check( allocator.GetError() == ALLOCATOR_ERROR_FAILED_TO_ALLOCATE );
            allocator.ClearError();
            check( allocator.GetError() == ALLOCATOR_ERROR_NONE );
            stopIndex = i;
            break;
        }
        
        check( blockData[i] );
        check( allocator.GetError() == ALLOCATOR_ERROR_NONE );
        
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

        allocator.Free( blockData[i] );

        blockData[i] = NULL;
    }

    free( memory );
}

void test_client_server_connect()
{
    printf( "test_client_server_connect\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_reconnect()
{
    printf( "test_client_server_reconnect\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    uint64_t connectTokenExpireTimestamp;

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect the client

    client.Disconnect();

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1;

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    // now verify the client is able to reconnect to the same server with a new connect token

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed (2)\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed! (2)\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_client_side_disconnect()
{
    printf( "test_client_server_client_side_disconnect\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    uint64_t connectTokenExpireTimestamp;

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect client side

    client.Disconnect();

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1f;

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_server_side_disconnect()
{
    printf( "test_client_server_server_side_disconnect\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    uint64_t connectTokenExpireTimestamp;

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect client side

    server.DisconnectAllClients();

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 1.0;

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_connection_request_timeout()
{
    printf( "test_client_server_connection_request_timeout\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );

    const int NumIterations = 20;

    double time = 0.0;

    ClientServerConfig config;
    config.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, config );

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();

        clientTransport.WritePackets();

        clientTransport.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 1.0;

        client.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );
}

void test_client_server_connection_response_timeout()
{
    printf( "test_client_server_connection_response_timeout\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    server.SetFlags( SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES );

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 0.1f;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT );

    client.Disconnect();

    server.Stop();
}

void test_client_server_client_side_timeout()
{
    printf( "test_client_server_client_side_timeout\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    while ( true )
    {
        server.SendPackets();

        serverTransport.WritePackets();

        serverTransport.ReadPackets();

        server.ReceivePackets();

        server.CheckForTimeOut();

        time += 1.0;

        if ( server.GetNumConnectedClients() == 0 )
            break;

        server.AdvanceTime( time );

        serverTransport.AdvanceTime( time );
    }

    check( server.GetNumConnectedClients() == 0 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_server_side_timeout()
{
    printf( "test_client_server_server_side_timeout\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    while ( true )
    {
        client.SendPackets();

        clientTransport.WritePackets();

        clientTransport.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        time += 1.0;

        if ( !client.IsConnected() )
            break;

        client.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
    }

    check( !client.IsConnected() );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_TIMEOUT );

    client.Disconnect();

    server.Stop();
}

struct ClientData
{
    Allocator * allocator;
    uint64_t clientId;
    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];
    TestNetworkTransport * transport;
    GameClient * client;
    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];
    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];
    uint64_t connectTokenExpireTimestamp;

    ClientData()
    {
        allocator = NULL;
        clientId = 0;
        numServerAddresses = 0;
        transport = NULL;
        client = NULL;
        memset( connectTokenData, 0, ConnectTokenBytes );
        memset( connectTokenNonce, 0, NonceBytes );
        memset( clientToServerKey, 0, KeyBytes );
        memset( serverToClientKey, 0, KeyBytes );
        connectTokenExpireTimestamp = 0;
    }

    ~ClientData()
    {
        YOJIMBO_DELETE( *allocator, TestNetworkTransport, transport );
        YOJIMBO_DELETE( *allocator, GameClient, client );
    }
};

void test_client_server_server_is_full()
{
    printf( "test_client_server_server_is_full\n" );

    GenerateKey( private_key );

    const int NumClients = 4;

    ClientData clientData[NumClients+1];

    TestNetworkSimulator networkSimulator;

    Allocator & allocator = GetDefaultAllocator();

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    for ( int i = 0; i < NumClients + 1; ++i )
    {
        clientData[i].allocator = &allocator;

        clientData[i].clientId = i + 1;

        Address clientAddress( "::1", ClientPort + i );

        clientData[i].transport = YOJIMBO_NEW( allocator, TestNetworkTransport, networkSimulator, clientAddress );

        if ( !matcher.RequestMatch( clientData[i].clientId, 
                                    clientData[i].connectTokenData, 
                                    clientData[i].connectTokenNonce, 
                                    clientData[i].clientToServerKey, 
                                    clientData[i].serverToClientKey, 
                                    clientData[i].connectTokenExpireTimestamp,
                                    clientData[i].numServerAddresses, 
                                    clientData[i].serverAddresses ) )
        {
            printf( "error: request match failed\n" );
            exit( 1 );
        }

        clientData[i].client = YOJIMBO_NEW( allocator, GameClient, allocator, *clientData[i].transport, clientServerConfig );
    }

    Address serverAddress( "::1", ServerPort );

    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start( NumClients );

    // connect clients so server is full and cannot accept any more client connections

    for ( int i = 0; i < NumClients; ++i )
    {
        clientData[i].client->Connect( serverAddress, 
                                       clientData[i].connectTokenData, 
                                       clientData[i].connectTokenNonce, 
                                       clientData[i].clientToServerKey, 
                                       clientData[i].serverToClientKey,
                                       clientData[i].connectTokenExpireTimestamp );
    }

    while ( true )
    {
        for ( int j = 0; j < NumClients; ++j )
            clientData[j].client->SendPackets();

        server.SendPackets();

        for ( int j = 0; j < NumClients; ++j )
            clientData[j].transport->WritePackets();

        serverTransport.WritePackets();

        for ( int j = 0; j < NumClients; ++j )
            clientData[j].transport->ReadPackets();

        serverTransport.ReadPackets();

        for ( int j = 0; j < NumClients; ++j )
            clientData[j].client->ReceivePackets();

        server.ReceivePackets();

        for ( int j = 0; j < NumClients; ++j )
            clientData[j].client->CheckForTimeOut();

        server.CheckForTimeOut();

        for ( int j = 0; j < NumClients; ++j )
        {
            if ( clientData[j].client->ConnectionFailed() )
            {
                printf( "error: client connect failed!\n" );
                exit( 1 );
            }
        }

        time += 0.1f;

        bool allClientsConnected = server.GetNumConnectedClients() == NumClients;

        for ( int j = 0; j < NumClients; ++j )
        {
            if ( !clientData[j].client->IsConnected() )
                allClientsConnected = false;
        }

        if ( allClientsConnected )
            break;

        for ( int j = 0; j < NumClients; ++j )
        {
            clientData[j].client->AdvanceTime( time );

            clientData[j].transport->AdvanceTime( time );
        }

        server.AdvanceTime( time );

        serverTransport.AdvanceTime( time );
    }

    bool allClientsConnected = server.GetNumConnectedClients() == NumClients;

    for ( int j = 0; j < NumClients; ++j )
    {
        if ( !clientData[j].client->IsConnected() )
            allClientsConnected = false;
    }

    check( allClientsConnected );
    check( server.GetMaxClients() == NumClients );
    check( server.GetNumConnectedClients() == server.GetMaxClients() );

    // try to connect one more client and verify that its connection attempt is rejected

    clientData[NumClients].client->AdvanceTime( time );

    clientData[NumClients].client->Connect( serverAddress, 
                                            clientData[NumClients].connectTokenData, 
                                            clientData[NumClients].connectTokenNonce, 
                                            clientData[NumClients].clientToServerKey, 
                                            clientData[NumClients].serverToClientKey,
                                            clientData[NumClients].connectTokenExpireTimestamp );

    while ( true )
    {
        for ( int j = 0; j <= NumClients; ++j )
            clientData[j].client->SendPackets();

        server.SendPackets();

        for ( int j = 0; j <= NumClients; ++j )
            clientData[j].transport->WritePackets();

        serverTransport.WritePackets();

        for ( int j = 0; j <= NumClients; ++j )
            clientData[j].transport->ReadPackets();

        serverTransport.ReadPackets();

        for ( int j = 0; j <= NumClients; ++j )
            clientData[j].client->ReceivePackets();

        server.ReceivePackets();

        for ( int j = 0; j <= NumClients; ++j )
            clientData[j].client->CheckForTimeOut();

        server.CheckForTimeOut();

        time += 0.1f;

        if ( clientData[NumClients].client->ConnectionFailed() )
            break;

        for ( int j = 0; j <= NumClients; ++j )
        {
            clientData[j].client->AdvanceTime( time );

            clientData[j].transport->AdvanceTime( time );
        }

        server.AdvanceTime( time );

        serverTransport.AdvanceTime( time );
    }

    check( server.GetNumConnectedClients() == NumClients );
    check( clientData[NumClients].client->GetClientState() == CLIENT_STATE_CONNECTION_DENIED );
    for ( int i = 0; i < NumClients; ++i )
    {
        check( clientData[i].client->IsConnected() );
        clientData[i].client->Disconnect();
    }

    server.Stop();
}

void test_client_server_connect_token_reuse()
{
    printf( "test_client_server_connect_token_reuse\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect the client, otherwise the server complains about the client id already being connected instead

    client.Disconnect();

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( server.GetNumConnectedClients() == 0 )
            break;

        time += 0.1f;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    // now try to connect a second client (different address) using the same token. the connect should be ignored

    Address clientAddress2( "::1", ClientPort + 1 );

    TestNetworkTransport clientTransport2( networkSimulator, clientAddress2 );
    
    GameClient client2( GetDefaultAllocator(), clientTransport2, clientServerConfig );

    client2.AdvanceTime( time );

    client2.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client2.SendPackets();
        server.SendPackets();

        clientTransport2.WritePackets();
        serverTransport.WritePackets();

        clientTransport2.ReadPackets();
        serverTransport.ReadPackets();

        client2.ReceivePackets();
        server.ReceivePackets();

        client2.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1;

        if ( client2.ConnectionFailed() )
            break;

        client2.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport2.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( client2.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED ) > 0 );

    client2.Disconnect();

    server.Stop();
}

void test_client_server_connect_token_expired()
{
    printf( "test_client_server_connect_token_expired\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    uint64_t connectTokenExpireTimestamp;

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses, -100 ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    const int NumIterations = 1000;

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 0.1;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );

    server.Stop();
}

void test_client_server_connect_token_whitelist()
{
    printf( "test_client_server_connect_token_whitelist\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    uint64_t connectTokenExpireTimestamp;

    if ( !matcher.RequestMatch( clientId, 
                                connectTokenData, 
                                connectTokenNonce, 
                                clientToServerKey, 
                                serverToClientKey,
                                connectTokenExpireTimestamp, 
                                numServerAddresses, 
                                serverAddresses, 
                                0, 
                                ServerPort + 1 ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    const int NumIterations = 1000;

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

		if ( client.ConnectionFailed() )
            break;

        time += 0.1;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_SERVER_ADDRESS_NOT_IN_WHITELIST ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );

    server.Stop();
}

void test_client_server_connect_token_invalid()
{
    printf( "test_client_server_connect_token_invalid\n" );

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    const int NumIterations = 1000;

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    uint64_t connectTokenExpireTimestamp = ::time( NULL ) + 100;

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 0.1;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_DECRYPT_CONNECT_TOKEN ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );

    server.Stop();
}

void test_client_server_connect_address_already_connected()
{
    printf( "test_client_server_connect_address_already_connected\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now try to connect a second client with the same address, but a different connect token and client id. this connect should be ignored

    if ( !matcher.RequestMatch( clientId + 1, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    TestNetworkTransport clientTransport2( networkSimulator, clientAddress );
    
    GameClient client2( GetDefaultAllocator(), clientTransport2, clientServerConfig );

    client2.AdvanceTime( time );

    client2.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        client2.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        clientTransport2.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        clientTransport2.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        client2.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        client2.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1;

        if ( client2.ConnectionFailed() )
            break;

        client.AdvanceTime( time );
        client2.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        clientTransport2.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
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
    printf( "test_client_server_connect_client_id_already_connected\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now try to connect a second client with a different address and connect token, but the same client id. this connect should be ignored

    Address clientAddress2( "::1", ClientPort + 1 );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    TestNetworkTransport clientTransport2( networkSimulator, clientAddress2 );
    
    GameClient client2( GetDefaultAllocator(), clientTransport2, clientServerConfig );

    client2.AdvanceTime( time );

    client2.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        client2.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        clientTransport2.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        clientTransport2.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        client2.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        client2.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1;

        if ( client2.ConnectionFailed() )
            break;

        client.AdvanceTime( time );
        client2.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        clientTransport2.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( client.GetClientState() == CLIENT_STATE_CONNECTED );
    check( client2.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT );
    check( server.GetCounter( SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED ) > 0 );

    client.Disconnect();
    client2.Disconnect();

    server.Stop();
}

void test_client_server_game_packets()
{
#if 0 // todo: bring across "SendGamePacketToServer" etc. helper functions

    printf( "test_client_server_game_packets\n" );

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

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

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now that the client and server are connected lets exchange game packets and count them getting through

    const int clientIndex = server.FindClientIndex( clientAddress );

    check( clientIndex != -1 );

    const int NumGamePackets = 32;

    while ( true )
    {
        client.SendGamePacketToServer();
        server.SendGamePacketToClient( clientIndex );

        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1f;

        if ( client.GetNumGamePacketsReceived() >= NumGamePackets && server.GetNumGamePacketsReceived( clientIndex ) >= NumGamePackets )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( client.GetNumGamePacketsReceived() >= NumGamePackets && server.GetNumGamePacketsReceived( clientIndex ) >= NumGamePackets );

    client.Disconnect();

    server.Stop();

#endif
}

#if YOJIMBO_INSECURE_CONNECT

void test_client_server_insecure_connect()
{
    printf( "test_client_server_insecure_connect\n" );

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

    server.SetServerAddress( serverAddress );

    clientTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );
    serverTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );

    server.SetFlags( SERVER_FLAG_ALLOW_INSECURE_CONNECT );

    server.Start();

    client.InsecureConnect( serverAddress );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            exit( 1 );
        }

        time += 0.1f;

        if ( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );

    client.Disconnect();

    server.Stop();
}

void test_client_server_insecure_connect_timeout()
{
    printf( "test_client_server_insecure_connect_timeout\n" );

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkTransport clientTransport( networkSimulator, clientAddress );

    double time = 0.0;

    ClientServerConfig config;
    config.enableConnection = false;

    GameClient client( GetDefaultAllocator(), clientTransport, config );

    clientTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );

    client.InsecureConnect( serverAddress );

    while ( true )
    {
        client.SendPackets();

        clientTransport.WritePackets();

        clientTransport.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        time += 0.1;

        if ( !client.IsConnecting() && client.ConnectionFailed() )
            break;

        client.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
    }

    check( !client.IsConnecting() );
    check( !client.IsConnected() );
    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_INSECURE_CONNECT_TIMEOUT );
}

#endif // #if YOJIMBO_INSECURE_CONNECT

void test_matcher()
{
    printf( "test_matcher\n" );

    uint8_t key[] = { 0x60,0x6a,0xbe,0x6e,0xc9,0x19,0x10,0xea,0x9a,0x65,0x62,0xf6,0x6f,0x2b,0x30,0xe4,0x43,0x71,0xd6,0x2c,0xd1,0x99,0x27,0x26,0x6b,0x3c,0x60,0xf4,0xb7,0x15,0xab,0xa1 };

    // test base64 decrypt from matcher.go

    {
        char serverAddress[MaxAddressLength];

        const char serverAddressBase64[] = "MTI3LjAuMC4xOjUwMDAwAA==";

        base64_decode_string( serverAddressBase64, serverAddress, sizeof( serverAddress ) );

        check( strcmp( serverAddress, "127.0.0.1:50000" ) == 0 );
    }

    // test base64 encoded connect token from matcher.go

    {
        const char encryptedConnectTokenBase64[] = "3P9n5y1P/bifBaauDT3ASH1LkRTKP6/WpayDb8sTiTW7dGWnr5WWH1odwBCu4b6c/GkZW24OZbFVOBcCXJF8n3J02o52/sZXMPRNFOBp4aZiicZn/m7gJ8sHJgWZUc67KwTPzZjtSBmUhiMrKKgQOunazQegrSiZmdVvxXEzrySzX8stFV6+VdjYqguoM5hE0JJxCWAlcHVQWiJ8zeYbg2gnBx7AGllNrTQa8E0J2N149ar2Bx38ucDe+W1TPup3V+I9IA4jEK7C1Q1chPgbgcR6utTJSNYxNuy+Vg5YOWq0Om/jAd8wj+3PlaDkjplX3RGmUxTzCfJLahCB41peV2XGaprQOA9T0Ui/lZ2x28GiPUZlg4rVXj+wojgIe7TyCmQf7m6TkQpMgLzaQ4I7KcVfv+fbhwHK0rB78/KhWATaJPdS/yiB2z3ZDzqMHeRyE+8gg7DYIIPB50gJOer4NBe6Z8musO9lkw5MtPR6tqohStSygJXMaKzoHTO6Fgd7WkL30tbLMvg7P6GBSafGUXEdHKboI6TlREY0nxcDZBn5DW9mF/x+8iaVzlX9M1I8UZehKUtH62VDKicdS0ILlrOaTMjH2+fRrr3DyMbGFT4irNLWsazJIeHGvklFCQWrCiY5vTfLrETyH3VQQiToxjbXqoRFxIH+Zx6i4QzTTXMXXx0b+1pZbx0rdNh4/X95GAxA5MwDCNmapvZcW57+fwgg2xI/RekqgQyhBFtaLaGjpfOqjLp2G1uMOZjsAo2J2w5oz7n5C7P40jqDG0hapq6suR3Xov+2Focy4qxKW7qr5jfO1Rd4h4ZH4yBfdJ9xGXtHETOnDIVNJNmfdJhk9o6aLEKAnNFcpBAqqfuLgQfEpTd5mQZaiOFLvpJN/yGcTJrZMg4+mH+nCw0o44cpBmIqunJMPIJnd/4Kse5q8WM8P5cG6MjR+RIxaSyUDdgIaxof2UCP6SCx90W1PhdT465PFbZaeJT4Ji4b9Gm++ku4bwLJ+BwIJh9L87loJUu8Z6iGSRop4rZNobstIzeyoYnGv1MZrcmHspa5mJbpFsMn5XTgQR+3nwt3614GEOy2f8cRWU43bTC0FvbEf+1CsO6Oyu5gDyGs7SRAp9GdYn+0hLPv/gmW+5sVgr392xwpooHTNLQb1PfNnhQeol1l0KSc+MPk9olpXNDvBtijWaiFE+SEnZEmO6Ls8KiCHgu9iLVxwrQHHY3ukFvUbPgvI9gVH4o3UnezOo3JLMUFPm9v9QhBY4H8hLDsg3QrcVHgOQhfpd7V6BOGdyUNtvYT3NrfJBUfVahu6bAP1EcdHxy8wWVSnLnYOTPFob9KgORtKAqkJrsLAINXLhrD3oR+kg==";

        uint8_t encryptedConnectToken[ConnectTokenBytes];

        int encryptedLength = base64_decode_data( encryptedConnectTokenBase64, encryptedConnectToken, ConnectTokenBytes );

        check( encryptedLength == ConnectTokenBytes );

        uint8_t nonce[] = { 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00  };

        uint64_t expireTimestamp = 1477934054ULL;

        ConnectToken connectToken;
        check( DecryptConnectToken( encryptedConnectToken, connectToken, (const uint8_t*) &nonce, key, expireTimestamp ) );
    }
}

void test_bit_array()
{
    printf( "test_bit_array\n" );

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

struct TestPacketData
{
    TestPacketData()
        : sequence(0xFFFF) {}

    explicit TestPacketData( uint16_t _sequence )
        : sequence( _sequence ) {}

    uint32_t sequence : 16;                 // packet sequence #
};

void test_sequence_buffer()
{
    printf( "test_sequence_buffer\n" );

    const int Size = 256;

    SequenceBuffer<TestPacketData> sequence_buffer( GetDefaultAllocator(), Size );

    for ( int i = 0; i < Size; ++i )
        check( sequence_buffer.Find(i) == NULL );

    for ( int i = 0; i <= Size*4; ++i )
    {
        TestPacketData * entry = sequence_buffer.Insert( i );
        entry->sequence = i;
        check( sequence_buffer.GetSequence() == i + 1 );
    }

    for ( int i = 0; i <= Size; ++i )
    {
        // note: outside bounds!
        TestPacketData * entry = sequence_buffer.Insert( i );
        check( !entry );
    }    

    int index = Size * 4;
    for ( int i = 0; i < Size; ++i )
    {
        TestPacketData * entry = sequence_buffer.Find( index );
        check( entry );
        check( entry->sequence == uint32_t( index ) );
        index--;
    }

    sequence_buffer.Reset();

    check( sequence_buffer.GetSequence() == 0 );

    for ( int i = 0; i < Size; ++i )
        check( sequence_buffer.Find(i) == NULL );
}

void test_generate_ack_bits()
{
    printf( "test_generate_ack_bits\n" );

    const int Size = 256;

    SequenceBuffer<TestPacketData> received_packets( GetDefaultAllocator(), Size );

    uint16_t ack = 0xFFFF;
    uint32_t ack_bits = 0xFFFF;

    GenerateAckBits( received_packets, ack, ack_bits );
    check( ack == 0xFFFF );
    check( ack_bits == 0 );

    for ( int i = 0; i <= Size; ++i )
        received_packets.Insert( i );

    GenerateAckBits( received_packets, ack, ack_bits );
    check( ack == Size );
    check( ack_bits == 0xFFFFFFFF );

    received_packets.Reset();
    uint16_t input_acks[] = { 1, 5, 9, 11 };
    int input_num_acks = sizeof( input_acks ) / sizeof( uint16_t );
    for ( int i = 0; i < input_num_acks; ++i )
        received_packets.Insert( input_acks[i] );

    GenerateAckBits( received_packets, ack, ack_bits );

    check( ack == 11 );
    check( ack_bits == ( 1 | (1<<(11-9)) | (1<<(11-5)) | (1<<(11-1)) ) );
}

class TestConnection : public Connection
{
    int * m_ackedPackets;

public:

    TestConnection( PacketFactory & packetFactory, MessageFactory & messageFactory, ConnectionConfig & config ) : Connection( GetDefaultAllocator(), packetFactory, messageFactory, config )
    {
        m_ackedPackets = NULL;
    }

    void SetAckedPackets( int * ackedPackets )
    {
        m_ackedPackets = ackedPackets;
    }

    virtual void OnPacketAcked( uint16_t sequence )
    {
        if ( m_ackedPackets )
            m_ackedPackets[sequence] = true;
    }
};

void test_connection_counters()
{
    printf( "test_connection_counters\n" );

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;

    TestConnection connection( packetFactory, messageFactory, connectionConfig );

    const int NumAcks = 100;

    for ( int i = 0; i < NumAcks * 2; ++i )
    {
        ConnectionPacket * packet = connection.GeneratePacket();

        check( packet );

        check( connection.ProcessPacket( packet ) );

        packet->Destroy();
        packet = NULL;

        if ( connection.GetCounter( CONNECTION_COUNTER_PACKETS_ACKED ) >= NumAcks )
            break;
    }

    check( connection.GetCounter( CONNECTION_COUNTER_PACKETS_ACKED ) == NumAcks );
    check( connection.GetCounter( CONNECTION_COUNTER_PACKETS_GENERATED ) == NumAcks + 1 );
    check( connection.GetCounter( CONNECTION_COUNTER_PACKETS_PROCESSED ) == NumAcks + 1 );
}

void test_connection_acks()
{
    printf( "test_connection_acks\n" );

    const int NumIterations = 10 * 1024;

    int receivedPackets[NumIterations];
    int ackedPackets[NumIterations];

    memset( receivedPackets, 0, sizeof( receivedPackets ) );
    memset( ackedPackets, 0, sizeof( ackedPackets ) );

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;

    TestConnection connection( packetFactory, messageFactory, connectionConfig );

    connection.SetAckedPackets( ackedPackets );

    for ( int i = 0; i < NumIterations; ++i )
    {
        ConnectionPacket * packet = connection.GeneratePacket();

        check( packet );

        if ( rand() % 100 == 0 )
        {
            connection.ProcessPacket( packet );

            if ( packet )
            {
                uint16_t sequence = packet->sequence;

                receivedPackets[sequence] = true;
            }
        }

        packet->Destroy();
        packet = NULL;
    }

    int numAckedPackets = 0;
    int numReceivedPackets = 0;
    for ( int i = 0; i < NumIterations; ++i )
    {
        if ( ackedPackets[i] )
            numAckedPackets++;

        if ( receivedPackets[i] )
            numReceivedPackets++;

        if ( ackedPackets[i] && !receivedPackets[i] )
            check( false );
    }

    check( numAckedPackets > 0 );
    check( numReceivedPackets >= numAckedPackets );
}

void test_connection_reliable_ordered_messages()
{
    printf( "test_connection_reliable_ordered_messages\n" );

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext context;
    context.messageFactory = &messageFactory;
    context.connectionConfig = &connectionConfig;

    const int NumMessagesSent = 64;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestMessage * message = (TestMessage*) messageFactory.Create( TEST_MESSAGE );
        check( message );
        message->sequence = i;
        sender.SendMsg( message );
    }

    TestNetworkSimulator networkSimulator;

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 1000 );
    networkSimulator.SetDuplicates( 50 );
    networkSimulator.SetPacketLoss( 50 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    SimulatorTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId );
    SimulatorTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId );

    senderTransport.SetPacketFactory( packetFactory );
    receiverTransport.SetPacketFactory( packetFactory );

    senderTransport.SetContext( &context );
    receiverTransport.SetContext( &context );

    double time = 0.0;
    double deltaTime = 0.1;

    const int NumIterations = 1000;

    int numMessagesReceived = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Packet * senderPacket = sender.GeneratePacket();
        Packet * receiverPacket = receiver.GeneratePacket();

        check( senderPacket );
        check( receiverPacket );

        senderTransport.SendPacket( receiverAddress, senderPacket, 0, false );
        receiverTransport.SendPacket( senderAddress, receiverPacket, 0, false );

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

            if ( from == receiverAddress && packet->GetType() == TEST_PACKET_CONNECTION )
                sender.ProcessPacket( (ConnectionPacket*) packet );

            packet->Destroy();
        }

        while ( true )
        {
            Address from;
            Packet * packet = receiverTransport.ReceivePacket( from, NULL );
            if ( !packet )
                break;

            if ( from == senderAddress && packet->GetType() == TEST_PACKET_CONNECTION )
            {
                receiver.ProcessPacket( (ConnectionPacket*) packet );
            }

            packet->Destroy();
        }

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

        time += deltaTime;

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );

        senderTransport.AdvanceTime( time );
        receiverTransport.AdvanceTime( time );

        networkSimulator.AdvanceTime( time );
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_blocks()
{
    printf( "test_connection_reliable_ordered_blocks\n" );

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext context;
    context.messageFactory = &messageFactory;
    context.connectionConfig = &connectionConfig;

    const int NumMessagesSent = 32;

    for ( int i = 0; i < NumMessagesSent; ++i )
    {
        TestBlockMessage * message = (TestBlockMessage*) messageFactory.Create( TEST_BLOCK_MESSAGE );
        check( message );
        message->sequence = i;
        const int blockSize = 1 + ( ( i * 901 ) % 3333 );
        uint8_t * blockData = (uint8_t*) messageFactory.GetAllocator().Allocate( blockSize );
        for ( int j = 0; j < blockSize; ++j )
            blockData[j] = i + j;
        message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
        sender.SendMsg( message );
    }

    TestNetworkSimulator networkSimulator;

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 1000 );
    networkSimulator.SetDuplicates( 50 );
    networkSimulator.SetPacketLoss( 50 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    SimulatorTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId );
    SimulatorTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId );

    senderTransport.SetPacketFactory( packetFactory );
    receiverTransport.SetPacketFactory( packetFactory );

    senderTransport.SetContext( &context );
    receiverTransport.SetContext( &context );

    double time = 0.0;
    double deltaTime = 0.1;

    const int NumIterations = 10000;

    int numMessagesReceived = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Packet * senderPacket = sender.GeneratePacket();
        Packet * receiverPacket = receiver.GeneratePacket();

        check( senderPacket );
        check( receiverPacket );

        senderTransport.SendPacket( receiverAddress, senderPacket, 0, false );
        receiverTransport.SendPacket( senderAddress, receiverPacket, 0, false );

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

            if ( from == receiverAddress && packet->GetType() == TEST_PACKET_CONNECTION )
                sender.ProcessPacket( (ConnectionPacket*) packet );

            packet->Destroy();
        }

        while ( true )
        {
            Address from;
            Packet * packet = receiverTransport.ReceivePacket( from, NULL );
            if ( !packet )
                break;

            if ( from == senderAddress && packet->GetType() == TEST_PACKET_CONNECTION )
            {
                receiver.ProcessPacket( (ConnectionPacket*) packet );
            }

            packet->Destroy();
        }

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

        time += deltaTime;

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );

        senderTransport.AdvanceTime( time );
        receiverTransport.AdvanceTime( time );

        networkSimulator.AdvanceTime( time );
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_messages_and_blocks()
{
    printf( "test_connection_reliable_ordered_messages_and_blocks\n" );

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext context;
    context.messageFactory = &messageFactory;
    context.connectionConfig = &connectionConfig;

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
            uint8_t * blockData = (uint8_t*) messageFactory.GetAllocator().Allocate( blockSize );
            for ( int j = 0; j < blockSize; ++j )
                blockData[j] = i + j;
            message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
            sender.SendMsg( message );
        }
    }

    TestNetworkSimulator networkSimulator;

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 1000 );
    networkSimulator.SetDuplicates( 50 );
    networkSimulator.SetPacketLoss( 50 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    SimulatorTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId );
    SimulatorTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId );

    senderTransport.SetPacketFactory( packetFactory );
    receiverTransport.SetPacketFactory( packetFactory );

    senderTransport.SetContext( &context );
    receiverTransport.SetContext( &context );

    double time = 0.0;
    double deltaTime = 0.1;

    const int NumIterations = 10000;

    int numMessagesReceived = 0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Packet * senderPacket = sender.GeneratePacket();
        Packet * receiverPacket = receiver.GeneratePacket();

        check( senderPacket );
        check( receiverPacket );

        senderTransport.SendPacket( receiverAddress, senderPacket, 0, false );
        receiverTransport.SendPacket( senderAddress, receiverPacket, 0, false );

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

            if ( from == receiverAddress && packet->GetType() == TEST_PACKET_CONNECTION )
                sender.ProcessPacket( (ConnectionPacket*) packet );

            packet->Destroy();
        }

        while ( true )
        {
            Address from;
            Packet * packet = receiverTransport.ReceivePacket( from, NULL );
            if ( !packet )
                break;

            if ( from == senderAddress && packet->GetType() == TEST_PACKET_CONNECTION )
            {
                receiver.ProcessPacket( (ConnectionPacket*) packet );
            }

            packet->Destroy();
        }

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

        time += deltaTime;

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );

        senderTransport.AdvanceTime( time );
        receiverTransport.AdvanceTime( time );

        networkSimulator.AdvanceTime( time );
    }

    check( numMessagesReceived == NumMessagesSent );
}

void test_connection_reliable_ordered_messages_and_blocks_multiple_channels()
{
    printf( "test_connection_reliable_ordered_messages_and_blocks_multiple_channels\n" );

    const int NumChannels = 2;

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;
    connectionConfig.numChannels = NumChannels;
    connectionConfig.channelConfig[0].maxMessagesPerPacket = 8;
    connectionConfig.channelConfig[1].maxMessagesPerPacket = 8;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext context;
    context.messageFactory = &messageFactory;
    context.connectionConfig = &connectionConfig;

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
                uint8_t * blockData = (uint8_t*) messageFactory.GetAllocator().Allocate( blockSize );
                for ( int j = 0; j < blockSize; ++j )
                    blockData[j] = i + j;
                message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
                sender.SendMsg( message, channelId );
            }
        }
    }

    TestNetworkSimulator networkSimulator;

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 1000 );
    networkSimulator.SetDuplicates( 50 );
    networkSimulator.SetPacketLoss( 50 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    SimulatorTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId );
    SimulatorTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId );

    senderTransport.SetPacketFactory( packetFactory );
    receiverTransport.SetPacketFactory( packetFactory );

    senderTransport.SetContext( &context );
    receiverTransport.SetContext( &context );

    double time = 0.0;
    double deltaTime = 0.1;

    const int NumIterations = 10000;

    int numMessagesReceived[NumChannels];
    memset( numMessagesReceived, 0, sizeof( numMessagesReceived ) );

    for ( int i = 0; i < NumIterations; ++i )
    {
        Packet * senderPacket = sender.GeneratePacket();
        Packet * receiverPacket = receiver.GeneratePacket();

        check( senderPacket );
        check( receiverPacket );

        senderTransport.SendPacket( receiverAddress, senderPacket, 0, false );
        receiverTransport.SendPacket( senderAddress, receiverPacket, 0, false );

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

            if ( from == receiverAddress && packet->GetType() == TEST_PACKET_CONNECTION )
                sender.ProcessPacket( (ConnectionPacket*) packet );

            packet->Destroy();
        }

        while ( true )
        {
            Address from;
            Packet * packet = receiverTransport.ReceivePacket( from, NULL );
            if ( !packet )
                break;

            if ( from == senderAddress && packet->GetType() == TEST_PACKET_CONNECTION )
            {
                receiver.ProcessPacket( (ConnectionPacket*) packet );
            }

            packet->Destroy();
        }

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

        time += deltaTime;

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );

        senderTransport.AdvanceTime( time );
        receiverTransport.AdvanceTime( time );

        networkSimulator.AdvanceTime( time );
    }

    for ( int channelId = 0; channelId < NumChannels; ++channelId )
    {
        check( numMessagesReceived[channelId] == NumMessagesSent );
    }
}

void test_connection_unreliable_unordered_messages()
{
    printf( "test_connection_unreliable_unordered_messages\n" );

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;
    connectionConfig.numChannels = 1;
    connectionConfig.channelConfig[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext context;
    context.messageFactory = &messageFactory;
    context.connectionConfig = &connectionConfig;

    TestNetworkSimulator networkSimulator;

    networkSimulator.SetPacketLoss( 0 );
    networkSimulator.SetLatency( 0 );
    networkSimulator.SetJitter( 0 );
    networkSimulator.SetDuplicates( 0 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    SimulatorTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId );
    SimulatorTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId );

    senderTransport.SetPacketFactory( packetFactory );
    receiverTransport.SetPacketFactory( packetFactory );

    senderTransport.SetContext( &context );
    receiverTransport.SetContext( &context );

    double time = 0.0;
    double deltaTime = 0.1;

    const int NumIterations = 16;

    for ( int i = 0; i < NumIterations; ++i )
    {
        const int NumMessagesSent = 16;

        for ( int j = 0; j < NumMessagesSent; ++j )
        {
            TestMessage * message = (TestMessage*) messageFactory.Create( TEST_MESSAGE );
            check( message );
            message->sequence = j;
            sender.SendMsg( message );
        }

        Packet * senderPacket = sender.GeneratePacket();
        Packet * receiverPacket = receiver.GeneratePacket();

        check( senderPacket );
        check( receiverPacket );

        senderTransport.SendPacket( receiverAddress, senderPacket, 0, false );
        receiverTransport.SendPacket( senderAddress, receiverPacket, 0, false );

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

            if ( from == receiverAddress && packet->GetType() == TEST_PACKET_CONNECTION )
                sender.ProcessPacket( (ConnectionPacket*) packet );

            packet->Destroy();
        }

        while ( true )
        {
            Address from;
            Packet * packet = receiverTransport.ReceivePacket( from, NULL );
            if ( !packet )
                break;

            if ( from == senderAddress && packet->GetType() == TEST_PACKET_CONNECTION )
            {
                receiver.ProcessPacket( (ConnectionPacket*) packet );
            }

            packet->Destroy();
        }

        int numMessagesReceived = 0;

        while ( true )
        {
            Message * message = receiver.ReceiveMsg();

            if ( !message )
                break;

            check( message->GetId() == uint16_t( i ) );
            check( message->GetType() == TEST_MESSAGE );

            TestMessage * testMessage = (TestMessage*) message;

            check( testMessage->sequence == uint16_t( numMessagesReceived ) );

            ++numMessagesReceived;

            messageFactory.Release( message );
        }

        check( numMessagesReceived == NumMessagesSent );

        time += deltaTime;

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );

        senderTransport.AdvanceTime( time );
        receiverTransport.AdvanceTime( time );

        networkSimulator.AdvanceTime( time );
    }
}

void test_connection_unreliable_unordered_blocks()
{
    printf( "test_connection_unreliable_unordered_blocks\n" );

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig connectionConfig;
    connectionConfig.connectionPacketType = TEST_PACKET_CONNECTION;
    connectionConfig.numChannels = 1;
    connectionConfig.channelConfig[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    TestConnection sender( packetFactory, messageFactory, connectionConfig );

    TestConnection receiver( packetFactory, messageFactory, connectionConfig );

    ConnectionContext context;
    context.messageFactory = &messageFactory;
    context.connectionConfig = &connectionConfig;

    TestNetworkSimulator networkSimulator;

    networkSimulator.SetPacketLoss( 0 );
    networkSimulator.SetLatency( 0 );
    networkSimulator.SetJitter( 0 );
    networkSimulator.SetDuplicates( 0 );

    const int SenderPort = 10000;
    const int ReceiverPort = 10001;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    SimulatorTransport senderTransport( GetDefaultAllocator(), networkSimulator, senderAddress, ProtocolId );
    SimulatorTransport receiverTransport( GetDefaultAllocator(), networkSimulator, receiverAddress, ProtocolId );

    senderTransport.SetPacketFactory( packetFactory );
    receiverTransport.SetPacketFactory( packetFactory );

    senderTransport.SetContext( &context );
    receiverTransport.SetContext( &context );

    double time = 0.0;
    double deltaTime = 0.1;

    const int NumIterations = 8;

    for ( int i = 0; i < NumIterations; ++i )
    {
        const int NumMessagesSent = 8;

        for ( int j = 0; j < NumMessagesSent; ++j )
        {
            TestBlockMessage * message = (TestBlockMessage*) messageFactory.Create( TEST_BLOCK_MESSAGE );
            check( message );
            message->sequence = j;
            const int blockSize = 1 + ( j * 7 );
            uint8_t * blockData = (uint8_t*) messageFactory.GetAllocator().Allocate( blockSize );
            for ( int k = 0; k < blockSize; ++k )
                blockData[k] = j + k;
            message->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
            sender.SendMsg( message );
        }

        Packet * senderPacket = sender.GeneratePacket();
        Packet * receiverPacket = receiver.GeneratePacket();

        check( senderPacket );
        check( receiverPacket );

        senderTransport.SendPacket( receiverAddress, senderPacket, 0, false );
        receiverTransport.SendPacket( senderAddress, receiverPacket, 0, false );

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

            if ( from == receiverAddress && packet->GetType() == TEST_PACKET_CONNECTION )
                sender.ProcessPacket( (ConnectionPacket*) packet );

            packet->Destroy();
        }

        while ( true )
        {
            Address from;
            Packet * packet = receiverTransport.ReceivePacket( from, NULL );
            if ( !packet )
                break;

            if ( from == senderAddress && packet->GetType() == TEST_PACKET_CONNECTION )
            {
                receiver.ProcessPacket( (ConnectionPacket*) packet );
            }

            packet->Destroy();
        }

        int numMessagesReceived = 0;

        while ( true )
        {
            Message * message = receiver.ReceiveMsg();

            if ( !message )
                break;

            check( message->GetId() == uint16_t( i ) );
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

        check( numMessagesReceived == NumMessagesSent );

        time += deltaTime;

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );

        senderTransport.AdvanceTime( time );
        receiverTransport.AdvanceTime( time );

        networkSimulator.AdvanceTime( time );
    }
}

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
            uint8_t * blockData = (uint8_t*) allocator.Allocate( blockSize );
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
            uint8_t * blockData = (uint8_t*) allocator.Allocate( blockSize );
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

void PumpClientServerUpdate( double & time, Client ** client, int numClients, Server ** server, int numServers, Transport ** transport, int numTransports )
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

    time += 0.1;

    for ( int i = 0; i < numClients; ++i )
        client[i]->AdvanceTime( time );

    for ( int i = 0; i < numServers; ++i )
        server[i]->AdvanceTime( time );

    for ( int i = 0; i < numTransports; ++i )
        transport[i]->AdvanceTime( time );
}

void ConnectClient( Client & client, uint64_t clientId, const Address & serverAddress )
{
    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    uint64_t connectTokenExpireTimestamp;

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );
}

void test_client_server_messages()
{
    printf( "test_client_server_messages\n" );

    GenerateKey( private_key );

    const uint64_t clientId = 1;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;
    TestNetworkTransport clientTransport( networkSimulator, clientAddress );
    TestNetworkTransport serverTransport( networkSimulator, serverAddress );

    double time = 0.0;

    ClientServerConfig clientServerConfig;
    clientServerConfig.connectionConfig.maxPacketSize = 256;
    clientServerConfig.connectionConfig.numChannels = 1;
    clientServerConfig.connectionConfig.channelConfig[0].maxBlockSize = 1024;
    clientServerConfig.connectionConfig.channelConfig[0].fragmentSize = 200;

    GameClient client( GetDefaultAllocator(), clientTransport, clientServerConfig );
    GameServer server( GetDefaultAllocator(), serverTransport, clientServerConfig );

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

int main()
{
    srand( time( NULL ) );

    printf( "\n" );
 
    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize yojimbo\n" );
        exit( 1 );
    }

#if SOAK
    signal( SIGINT, interrupt_handler );    
    int iter = 0;
    while ( true )
#endif // #if SOAK
    {
        test_endian();
        test_queue();
        test_base64();
        test_bitpacker();
        test_stream();
        test_packets();
        test_address_ipv4();
        test_address_ipv6();
        test_packet_sequence();
        test_encrypt_and_decrypt();
        test_encryption_manager();
        test_unencrypted_packets();
        test_allocator_tlsf();
        test_client_server_tokens();
        test_client_server_connect();
        test_client_server_reconnect();
        test_client_server_client_side_disconnect();
        test_client_server_server_side_disconnect();
        test_client_server_connection_request_timeout();
        test_client_server_connection_response_timeout();
        test_client_server_client_side_timeout();
        test_client_server_server_side_timeout();
        test_client_server_server_is_full();
        test_client_server_connect_token_reuse();
        test_client_server_connect_token_expired();
        test_client_server_connect_token_whitelist();
        test_client_server_connect_token_invalid();
        test_client_server_connect_address_already_connected();
        test_client_server_connect_client_id_already_connected();
        test_client_server_game_packets();
#if YOJIMBO_INSECURE_CONNECT
        test_client_server_insecure_connect();
        test_client_server_insecure_connect_timeout();
#endif // #if YOJIMBO_INSECURE_CONNECT
        test_matcher();
        test_bit_array();
        test_sequence_buffer();
        test_generate_ack_bits();
        test_connection_counters();
        test_connection_acks();
        test_connection_reliable_ordered_messages();
        test_connection_reliable_ordered_blocks();
        test_connection_reliable_ordered_messages_and_blocks();
        test_connection_reliable_ordered_messages_and_blocks_multiple_channels();
        test_connection_unreliable_unordered_messages();
        test_connection_unreliable_unordered_blocks();
        test_client_server_messages();

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

    ShutdownYojimbo();

    return 0;
}
