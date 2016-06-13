/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define SOAK_TEST 1

#if SOAK_TEST
#include <signal.h>
static volatile int quit = 0;
void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}
#endif // #if SOAK_TEST

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

void test_bitpacker()
{
    printf( "test_bitpacker\n" );

    const int BufferSize = 256;

    uint8_t buffer[256];

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
    char string[64];
};

struct TestContext
{
    int min;
    int max;
};

struct TestObject : public Serializable
{
    TestData data;

    TestObject()
    {
        memset( &data, 0, sizeof( data ) );
    }

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

    YOJIMBO_SERIALIZE_FUNCTIONS();

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
    writeObject.SerializeWrite( writeStream );
    writeStream.Flush();

    const int bytesWritten = writeStream.GetBytesProcessed();

    memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );

    TestObject readObject;
    ReadStream readStream( buffer, bytesWritten );
    readStream.SetContext( &context );
    readObject.SerializeRead( readStream );

    check( readObject == writeObject );
}

enum TestPacketTypes
{
    TEST_PACKET_A,
    TEST_PACKET_B,
    TEST_PACKET_C,
    TEST_PACKET_NUM_TYPES
};

struct TestPacketA : public Packet
{
    int a,b,c;

    TestPacketA() : Packet( TEST_PACKET_A )
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

    YOJIMBO_SERIALIZE_FUNCTIONS();
};

struct TestPacketB : public Packet
{
    int x,y;

    TestPacketB() : Packet( TEST_PACKET_B )
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

    YOJIMBO_SERIALIZE_FUNCTIONS();
};

struct TestPacketC : public Packet
{
    uint8_t data[16];

    TestPacketC() : Packet( TEST_PACKET_C )
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

    YOJIMBO_SERIALIZE_FUNCTIONS();
};

struct TestPacketFactory : public PacketFactory
{
    TestPacketFactory( Allocator & allocator ) : PacketFactory( allocator, TEST_PACKET_NUM_TYPES ) {}

    Packet * Create( int type )
    {
        Allocator & allocator = GetAllocator();

        switch ( type )
        {
            case TEST_PACKET_A: return YOJIMBO_NEW( allocator, TestPacketA );
            case TEST_PACKET_B: return YOJIMBO_NEW( allocator, TestPacketB );
            case TEST_PACKET_C: return YOJIMBO_NEW( allocator, TestPacketC );
        }
        return NULL;
    }

    void Destroy( Packet * packet )
    {
        YOJIMBO_DELETE( GetAllocator(), Packet, packet );
    }
};

void test_packets()
{
    printf( "test_packets\n" );

    TestPacketFactory packetFactory( GetDefaultAllocator() );

    TestPacketA * a = (TestPacketA*) packetFactory.CreatePacket( TEST_PACKET_A );
    TestPacketB * b = (TestPacketB*) packetFactory.CreatePacket( TEST_PACKET_B );
    TestPacketC * c = (TestPacketC*) packetFactory.CreatePacket( TEST_PACKET_C );

    check( a );
    check( b );
    check( c );

    check( a->GetType() == TEST_PACKET_A );
    check( b->GetType() == TEST_PACKET_B );
    check( c->GetType() == TEST_PACKET_C );

    packetFactory.DestroyPacket( a );
    packetFactory.DestroyPacket( b );
    packetFactory.DestroyPacket( c );
}

void test_address_ipv4()
{
    printf( "test_address_ipv4\n" );

    char buffer[256];

    {
        Address address( 127, 0, 0, 1 );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 0 );
        check( address.GetAddress4() == 0x100007f );
        check( strcmp( address.ToString( buffer, 256 ), "127.0.0.1" ) == 0 );
    }

    {
        Address address( 127, 0, 0, 1, 1000 );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 1000 );
        check( address.GetAddress4() == 0x100007f );
        check( strcmp( address.ToString( buffer, 256 ), "127.0.0.1:1000" ) == 0 );
    }

    {
        Address address( "127.0.0.1" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 0 );
        check( address.GetAddress4() == 0x100007f );
        check( strcmp( address.ToString( buffer, 256 ), "127.0.0.1" ) == 0 );
    }

    {
        Address address( "127.0.0.1:65535" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 65535 );
        check( address.GetAddress4() == 0x100007f );
        check( strcmp( address.ToString( buffer, 256 ), "127.0.0.1:65535" ) == 0 );
    }

    {
        Address address( "10.24.168.192:3000" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 3000 );
        check( address.GetAddress4() == 0xc0a8180a );
        check( strcmp( address.ToString( buffer, 256 ), "10.24.168.192:3000" ) == 0 );
    }

    {
        Address address( "255.255.255.255:65535" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV4 );
        check( address.GetPort() == 65535 );
        check( address.GetAddress4() == 0xffffffff );
        check( strcmp( address.ToString( buffer, 256 ), "255.255.255.255:65535" ) == 0 );
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

    char buffer[256];

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

        check( strcmp( address.ToString( buffer, 256 ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, 256 ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, 256 ), "::1" ) == 0 );
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

        check( strcmp( address.ToString( buffer, 256 ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6, 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, 256 ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6, 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( test_htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, 256 ), "[::1]:65535" ) == 0 );
    }

    // parse addresses from strings (no ports)

    {
        Address address( "fe80::202:b3ff:fe1e:8329" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( strcmp( address.ToString( buffer, 256 ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        Address address( "::1" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );
        check( strcmp( address.ToString( buffer, 256 ), "::1" ) == 0 );
    }

    // parse addresses from strings (with ports)

    {
        Address address( "[fe80::202:b3ff:fe1e:8329]:65535" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );
        check( strcmp( address.ToString( buffer, 256 ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        Address address( "[::1]:65535" );
        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );
        check( strcmp( address.ToString( buffer, 256 ), "[::1]:65535" ) == 0 );
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

    for ( uint64_t sequence = 0; sequence < 100000000LL; sequence += 101 )
    {
        uint8_t prefix_byte;
        uint8_t sequence_bytes[8];
        int num_sequence_bytes;
        yojimbo::compress_packet_sequence( sequence, prefix_byte, num_sequence_bytes, sequence_bytes );
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
    const int ServerPort = 50000;
    const int ClientPort = 60000;

    uint8_t private_key[KeyBytes];

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

    GenerateKey( private_key );

    numServerAddresses = 1;
    serverAddresses[0] = Address( "::1", ServerPort );

    memset( connectTokenNonce, 0, NonceBytes );

    {
        ConnectToken token;
        GenerateConnectToken( token, clientId, numServerAddresses, serverAddresses, ProtocolId );

        memcpy( clientToServerKey, token.clientToServerKey, KeyBytes );
        memcpy( serverToClientKey, token.serverToClientKey, KeyBytes );

        if ( !EncryptConnectToken( token, connectTokenData, NULL, 0, connectTokenNonce, private_key ) )
        {
            printf( "error: failed to encrypt connect token\n" );
            exit( 1 );
        }
    }

    ConnectToken connectToken;
    if ( !DecryptConnectToken( connectTokenData, connectToken, NULL, 0, connectTokenNonce, private_key ) )
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

    if ( !EncryptChallengeToken( challengeToken, challengeTokenData, NULL, 0, challengeTokenNonce, private_key ) )
    {
        printf( "error: failed to encrypt challenge token\n" );
        exit( 1 );
    }

    ChallengeToken decryptedChallengeToken;
    if ( !DecryptChallengeToken( challengeTokenData, decryptedChallengeToken, NULL, 0, challengeTokenNonce, private_key ) )
    {
        printf( "error: failed to decrypt challenge token\n" );
        exit( 1 );
    }

    check( challengeToken.clientId == clientId );
    check( memcmp( challengeToken.connectTokenMac, connectTokenData, MacBytes ) == 0 );
    check( memcmp( challengeToken.clientToServerKey, clientToServerKey, KeyBytes ) == 0 );
    check( memcmp( challengeToken.serverToClientKey, serverToClientKey, KeyBytes ) == 0 );
}

const uint32_t ProtocolId = 0x01231111;

const int ClientPort = 40000;
const int ServerPort = 50000;

static uint8_t private_key[KeyBytes];

class Matcher
{
    uint64_t m_nonce;

public:

    Matcher()
    {
        m_nonce = 0;
    }

    bool RequestMatch( uint64_t clientId, 
                       uint8_t * tokenData, 
                       uint8_t * tokenNonce, 
                       uint8_t * clientToServerKey, 
                       uint8_t * serverToClientKey, 
                       int & numServerAddresses, 
                       Address * serverAddresses, 
                       int timestampOffsetInSeconds = 0, 
                       int serverPortOverride = -1 )
    {
        if ( clientId == 0 )
            return false;

        numServerAddresses = 1;
        serverAddresses[0] = Address( "::1", serverPortOverride == -1 ? ServerPort : serverPortOverride );

        ConnectToken token;
        GenerateConnectToken( token, clientId, numServerAddresses, serverAddresses, ProtocolId );
        token.expiryTimestamp += timestampOffsetInSeconds;

        memcpy( clientToServerKey, token.clientToServerKey, KeyBytes );
        memcpy( serverToClientKey, token.serverToClientKey, KeyBytes );

        if ( !EncryptConnectToken( token, tokenData, NULL, 0, (const uint8_t*) &m_nonce, private_key ) )
            return false;

        assert( NonceBytes == 8 );

        memcpy( tokenNonce, &m_nonce, NonceBytes );

        m_nonce++;

        return true;
    }
};

enum GamePackets
{
    GAME_PACKET = CLIENT_SERVER_NUM_PACKETS,
    GAME_NUM_PACKETS
};

struct GamePacket : public Packet
{
    uint32_t sequence;
    uint32_t a,b,c;

    GamePacket() : Packet( GAME_PACKET )
    {
        sequence = 0;
        a = 0;
        b = 0;
        c = 0;
    }

    void Initialize( uint32_t seq )
    {
        assert( seq > 0 );
        sequence = seq;
        a = seq % 2;
        b = seq % 3;
        c = seq % 5;
    }

    template <typename Stream> bool Serialize( Stream & stream ) 
    { 
        serialize_bits( stream, sequence, 32 );
        
        serialize_bits( stream, a, 32 );
        serialize_bits( stream, b, 32 );
        serialize_bits( stream, c, 32 );

        assert( sequence > 0 );
        assert( a == sequence % 2 );
        assert( b == sequence % 3 );
        assert( c == sequence % 5 );

        return true;
    }

    YOJIMBO_SERIALIZE_FUNCTIONS();
};

static bool verbose_logging = false;

class GameServer : public Server
{
    uint32_t m_gamePacketSequence;
    uint64_t m_numGamePacketsReceived[MaxClients];

public:

    GameServer( NetworkInterface & networkInterface ) : Server( networkInterface )
    {
        SetPrivateKey( private_key );
        m_gamePacketSequence = 0;
        memset( m_numGamePacketsReceived, 0, sizeof( m_numGamePacketsReceived ) );
    }

    void OnClientConnect( int clientIndex )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        m_numGamePacketsReceived[clientIndex] = 0;

        if ( verbose_logging )
        {
            char addressString[256];
            GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
            printf( "client %d connected (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
        }
    }

    void OnClientDisconnect( int clientIndex )
    {
        if ( verbose_logging )
        {
            char addressString[256];
            GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
            printf( "client %d disconnected (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
        }
    }

    void OnClientTimedOut( int clientIndex )
    {
        if ( verbose_logging )
        {
            char addressString[256];
            GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
            printf( "client %d timed out (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
        }
    }

    void OnPacketSent( int packetType, const Address & to, bool immediate )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:          packetTypeString = "connection denied";     break;
            case CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE:       packetTypeString = "challenge";             break;
            case CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT:       packetTypeString = "heartbeat";             break;
            case CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT:      packetTypeString = "disconnect";            break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[256];
            to.ToString( addressString, sizeof( addressString ) );
            printf( "server sent %s packet to %s%s\n", packetTypeString, addressString, immediate ? " (immediate)" : "" );
        }
    }

    void OnPacketReceived( int packetType, const Address & from, uint64_t /*sequence*/ )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:         packetTypeString = "connection request";        break;
            case CLIENT_SERVER_PACKET_CONNECTION_RESPONSE:        packetTypeString = "challenge response";        break;
            case CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT:       packetTypeString = "heartbeat";                 break;  
            case CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT:      packetTypeString = "disconnect";                break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[256];
            from.ToString( addressString, sizeof( addressString ) );
            printf( "server received '%s' packet from %s\n", packetTypeString, addressString );
        }
    }

    uint64_t GetNumGamePacketsReceived( int clientIndex ) const
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        return m_numGamePacketsReceived[clientIndex];
    }

    void SendGamePacketToClient( int clientIndex )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( IsClientConnected( clientIndex ) );
        GamePacket * packet = (GamePacket*) m_networkInterface->CreatePacket( GAME_PACKET );
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

public:

    GameClient( NetworkInterface & networkInterface ) : Client( networkInterface )
    {
        m_numGamePacketsReceived = 0;
        m_gamePacketSequence = 0;
    }

    void SendGamePacketToServer()
    {
        GamePacket * packet = (GamePacket*) m_networkInterface->CreatePacket( GAME_PACKET );
        assert( packet );
        packet->Initialize( ++m_gamePacketSequence );
        SendPacketToServer( packet );
    }

    void OnConnect( const Address & address )
    {
        m_numGamePacketsReceived = 0;

        if ( verbose_logging )
        {
            char addressString[256];
            address.ToString( addressString, sizeof( addressString ) );
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

    void OnPacketSent( int packetType, const Address & to, bool immediate )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:         packetTypeString = "connection request";        break;
            case CLIENT_SERVER_PACKET_CONNECTION_RESPONSE:        packetTypeString = "challenge response";        break;
            case CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT:       packetTypeString = "heartbeat";                 break;  
            case CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT:      packetTypeString = "disconnect";                break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[256];
            to.ToString( addressString, sizeof( addressString ) );
            printf( "client sent %s packet to %s%s\n", packetTypeString, addressString, immediate ? " (immediate)" : "" );
        }
    }

    void OnPacketReceived( int packetType, const Address & from, uint64_t /*sequence*/ )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:          packetTypeString = "connection denied";     break;
            case CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE:       packetTypeString = "challenge";             break;
            case CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT:       packetTypeString = "heartbeat";             break;
            case CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT:      packetTypeString = "disconnect";            break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[256];
            from.ToString( addressString, sizeof( addressString ) );
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

class GamePacketFactory : public ClientServerPacketFactory
{
public:

    GamePacketFactory() : ClientServerPacketFactory( GetDefaultAllocator(), GAME_NUM_PACKETS ) {}

    Packet * Create( int type )
    {
        Packet * packet = ClientServerPacketFactory::Create( type );
        if ( packet )
            return packet;

        Allocator & allocator = GetAllocator();

        if ( type == GAME_PACKET )
            return YOJIMBO_NEW( allocator, GamePacket );

        return NULL;
    }
};

class TestNetworkSimulator : public NetworkSimulator
{
public:

    TestNetworkSimulator() : NetworkSimulator( GetDefaultAllocator() )
    {
        SetLatency( 1000 );
        SetJitter( 250 );
        SetDuplicates( 10 );
        SetPacketLoss( 10 );
    }   
};

class TestNetworkInterface : public SimulatorInterface
{   
public:

    TestNetworkInterface( GamePacketFactory & packetFactory, NetworkSimulator & networkSimulator, const Address & address ) 
        : SimulatorInterface( GetDefaultAllocator(), networkSimulator, packetFactory, address, ProtocolId )
    {
        EnablePacketEncryption();
        DisableEncryptionForPacketType( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );
    }

    ~TestNetworkInterface()
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

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    // make sure that encrypted packet types will *not* be received if they are sent as unencrypted

    check( clientInterface.IsEncryptedPacketType( CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT ) );
    check( serverInterface.IsEncryptedPacketType( CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT ) );

    clientInterface.DisableEncryptionForPacketType( CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT );

    check( !clientInterface.IsEncryptedPacketType( CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT ) );

    const int NumIterations = 32;

    int numPacketsReceived = 0;

    double time = 0.0;

    for ( int i = 0; i < NumIterations; ++i )
    {
        Packet * sendPacket = clientInterface.CreatePacket( CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT );
        check( sendPacket );
        clientInterface.SendPacket( serverAddress, sendPacket, 0, false );

        clientInterface.WritePackets();

        serverInterface.ReadPackets();

        while ( true )
        {
            Address address;
            uint64_t sequence;
            Packet * packet = serverInterface.ReceivePacket( address, &sequence );
            if ( !packet )
                break;
            if ( packet->GetType() == CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT )
                numPacketsReceived++;
        }

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );

        time += 0.1;
    }

    check( numPacketsReceived == 0 );
}

void test_client_server_connect()
{
    printf( "test_client_server_connect\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );
}

void test_client_server_reconnect()
{
    printf( "test_client_server_reconnect\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect the client

    client.Disconnect();

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1;

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );

    // now verify the client is able to reconnect to the same server with a new connect token

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed (2)\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );
}

void test_client_server_client_side_disconnect()
{
    printf( "test_client_server_client_side_disconnect\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect client side

    client.Disconnect();

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1f;

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );
}

void test_client_server_server_side_disconnect()
{
    printf( "test_client_server_server_side_disconnect\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // disconnect client side

    server.DisconnectAllClients();

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 1.0;

        if ( !client.IsConnected() && server.GetNumConnectedClients() == 0 )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnected() && server.GetNumConnectedClients() == 0 );
}

void test_client_server_connection_request_timeout()
{
    printf( "test_client_server_connection_request_timeout\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );

    const int NumIterations = 20;

    double time = 0.0;

    GameClient client( clientInterface );

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();

        clientInterface.WritePackets();

        clientInterface.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 1.0;

        client.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
}

void test_client_server_connection_response_timeout()
{
    printf( "test_client_server_connection_response_timeout\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    server.SetFlags( SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES );

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 0.1f;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT );
}

void test_client_server_client_side_timeout()
{
    printf( "test_client_server_client_side_timeout\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    while ( true )
    {
        server.SendPackets();

        serverInterface.WritePackets();

        serverInterface.ReadPackets();

        server.ReceivePackets();

        server.CheckForTimeOut();

        time += 1.0;

        if ( server.GetNumConnectedClients() == 0 )
            break;

        server.AdvanceTime( time );

        serverInterface.AdvanceTime( time );
    }

    check( server.GetNumConnectedClients() == 0 );
}

void test_client_server_server_side_timeout()
{
    printf( "test_client_server_server_side_timeout\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    while ( true )
    {
        client.SendPackets();

        clientInterface.WritePackets();

        clientInterface.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        time += 1.0;

        if ( !client.IsConnected() )
            break;

        client.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
    }

    check( !client.IsConnected() );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_TIMED_OUT );
}

struct ClientData
{
    Allocator * allocator;
    uint64_t clientId;
    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];
    TestNetworkInterface * networkInterface;
    GameClient * client;
    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];
    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    ClientData()
    {
        allocator = NULL;
        clientId = 0;
        numServerAddresses = 0;
        networkInterface = NULL;
        client = NULL;
        memset( connectTokenData, 0, ConnectTokenBytes );
        memset( connectTokenNonce, 0, NonceBytes );
        memset( clientToServerKey, 0, KeyBytes );
        memset( serverToClientKey, 0, KeyBytes );
    }

    ~ClientData()
    {
        YOJIMBO_DELETE( *allocator, TestNetworkInterface, networkInterface );
        YOJIMBO_DELETE( *allocator, GameClient, client );
    }
};

void test_client_server_server_is_full()
{
    printf( "test_client_server_server_is_full\n" );

    Matcher matcher;

    GenerateKey( private_key );

    const int NumClients = 4;

    ClientData clientData[NumClients+1];

    GamePacketFactory packetFactory;

    TestNetworkSimulator networkSimulator;

    Allocator & allocator = GetDefaultAllocator();

    for ( int i = 0; i < NumClients + 1; ++i )
    {
        clientData[i].allocator = &allocator;

        clientData[i].clientId = i + 1;

        Address clientAddress( "::1", ClientPort + i );

        clientData[i].networkInterface = YOJIMBO_NEW( allocator, TestNetworkInterface, packetFactory, networkSimulator, clientAddress );

        if ( !matcher.RequestMatch( clientData[i].clientId, 
                                    clientData[i].connectTokenData, 
                                    clientData[i].connectTokenNonce, 
                                    clientData[i].clientToServerKey, 
                                    clientData[i].serverToClientKey, 
                                    clientData[i].numServerAddresses, 
                                    clientData[i].serverAddresses ) )
        {
            printf( "error: request match failed\n" );
            exit( 1 );
        }

        clientData[i].client = YOJIMBO_NEW( allocator, GameClient, *clientData[i].networkInterface );
    }

    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start( NumClients );

    // connect clients so server is full and cannot accept any more client connections

    for ( int i = 0; i < NumClients; ++i )
    {
        clientData[i].client->Connect( serverAddress, 
                                       clientData[i].connectTokenData, 
                                       clientData[i].connectTokenNonce, 
                                       clientData[i].clientToServerKey, 
                                       clientData[i].serverToClientKey );
    }

    while ( true )
    {
        for ( int j = 0; j < NumClients; ++j )
            clientData[j].client->SendPackets();

        server.SendPackets();

        for ( int j = 0; j < NumClients; ++j )
            clientData[j].networkInterface->WritePackets();

        serverInterface.WritePackets();

        for ( int j = 0; j < NumClients; ++j )
            clientData[j].networkInterface->ReadPackets();

        serverInterface.ReadPackets();

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

            clientData[j].networkInterface->AdvanceTime( time );
        }

        server.AdvanceTime( time );

        serverInterface.AdvanceTime( time );
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
                                            clientData[NumClients].serverToClientKey );

    while ( true )
    {
        for ( int j = 0; j <= NumClients; ++j )
            clientData[j].client->SendPackets();

        server.SendPackets();

        for ( int j = 0; j <= NumClients; ++j )
            clientData[j].networkInterface->WritePackets();

        serverInterface.WritePackets();

        for ( int j = 0; j <= NumClients; ++j )
            clientData[j].networkInterface->ReadPackets();

        serverInterface.ReadPackets();

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

            clientData[j].networkInterface->AdvanceTime( time );
        }

        server.AdvanceTime( time );

        serverInterface.AdvanceTime( time );
    }

    check( server.GetNumConnectedClients() == NumClients );
    check( clientData[NumClients].client->GetClientState() == CLIENT_STATE_CONNECTION_DENIED );
    for ( int i = 0; i < NumClients; ++i )
    {
        check( clientData[i].client->IsConnected() );
    }
}

void test_client_server_connect_token_reuse()
{
    printf( "test_client_server_connect_token_reuse\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );

    // now try to connect a second client (different address) using the same token. the connect should be ignored

    Address clientAddress2( "::1", ClientPort + 1 );

    TestNetworkInterface clientInterface2( packetFactory, networkSimulator, clientAddress2 );
    
    GameClient client2( clientInterface2 );

    client2.AdvanceTime( time );

    client2.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        client2.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        clientInterface2.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        clientInterface2.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        clientInterface2.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() && client.IsConnected() && server.GetNumConnectedClients() == 1 );
    check( client2.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
    check( server.GetCounter( SERVER_COUNTER_CONNECT_TOKEN_ALREADY_USED ) > 0 );
}

void test_client_server_connect_token_expiry()
{
    printf( "test_client_server_connect_token_expiry\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses, -100 ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    const int NumIterations = 20;

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 1.0;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECT_TOKEN_EXPIRED ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
}

void test_client_server_connect_token_whitelist()
{
    printf( "test_client_server_connect_token_whitelist\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, 
                                connectTokenData, 
                                connectTokenNonce, 
                                clientToServerKey, 
                                serverToClientKey, 
                                numServerAddresses, 
                                serverAddresses, 
                                0, 
                                ServerPort + 1 ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    const int NumIterations = 20;

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 1.0;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECT_TOKEN_SERVER_ADDRESS_NOT_IN_WHITELIST ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
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

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    const int NumIterations = 20;

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 1.0;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( server.GetCounter( SERVER_COUNTER_CONNECT_TOKEN_FAILED_TO_DECRYPT ) > 0 );
    check( client.GetClientState() == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
}

void test_client_server_game_packets()
{
    printf( "test_client_server_game_packets\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
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

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        client.CheckForTimeOut();
        server.CheckForTimeOut();

        time += 0.1f;

        if ( client.GetNumGamePacketsReceived() >= NumGamePackets && server.GetNumGamePacketsReceived( clientIndex ) >= NumGamePackets )
            break;

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( client.GetNumGamePacketsReceived() >= NumGamePackets && server.GetNumGamePacketsReceived( clientIndex ) >= NumGamePackets );
}

#if YOJIMBO_INSECURE_CONNECT

void test_client_server_insecure_connect()
{
    printf( "test_client_server_insecure_connect\n" );

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );
    TestNetworkInterface serverInterface( packetFactory, networkSimulator, serverAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );

    clientInterface.SetFlags( NETWORK_INTERFACE_FLAG_INSECURE_MODE );
    serverInterface.SetFlags( NETWORK_INTERFACE_FLAG_INSECURE_MODE );

    server.SetFlags( SERVER_FLAG_ALLOW_INSECURE_CONNECT );

    server.Start();

    client.InsecureConnect( serverAddress );

    while ( true )
    {
        client.SendPackets();
        server.SendPackets();

        clientInterface.WritePackets();
        serverInterface.WritePackets();

        clientInterface.ReadPackets();
        serverInterface.ReadPackets();

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

        clientInterface.AdvanceTime( time );
        serverInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() );
    check( client.IsConnected() );
    check( server.GetNumConnectedClients() == 1 );
}

void test_client_server_insecure_connect_timeout()
{
    printf( "test_client_server_insecure_connect_timeout\n" );

    GamePacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkSimulator networkSimulator;

    TestNetworkInterface clientInterface( packetFactory, networkSimulator, clientAddress );

    double time = 0.0;

    GameClient client( clientInterface );

    clientInterface.SetFlags( NETWORK_INTERFACE_FLAG_INSECURE_MODE );

    client.InsecureConnect( serverAddress );

    while ( true )
    {
        client.SendPackets();

        clientInterface.WritePackets();

        clientInterface.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        time += 0.1;

        if ( !client.IsConnecting() && client.ConnectionFailed() )
            break;

        client.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
    }

    check( !client.IsConnecting() );
    check( !client.IsConnected() );
    check( client.ConnectionFailed() );
    check( client.GetClientState() == CLIENT_STATE_INSECURE_CONNECT_TIMED_OUT );
}

#endif // #if YOJIMBO_INSECURE_CONNECT

int main()
{
    srand( time( NULL ) );

    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize yojimbo\n" );
        exit( 1 );
    }

#if SOAK_TEST
    signal( SIGINT, interrupt_handler );    
    int iter = 0;
    while ( !quit )
#endif // #if SOAK_TEST
    {
        test_bitpacker();
        test_stream();
        test_packets();
        test_address_ipv4();
        test_address_ipv6();
        test_packet_sequence();
        test_encrypt_and_decrypt();
        test_encryption_manager();
        test_unencrypted_packets();
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
        test_client_server_connect_token_expiry();
        test_client_server_connect_token_whitelist();
        test_client_server_connect_token_invalid();
        test_client_server_game_packets();
#if YOJIMBO_INSECURE_CONNECT
        test_client_server_insecure_connect();
        test_client_server_insecure_connect_timeout();
#endif // #if YOJIMBO_INSECURE_CONNECT

#if SOAK_TEST
        iter++;
        for ( int i = 0; i < iter; ++i )
            printf( "." );
        printf( "\n" );
        if ( iter > 10 )
            iter = 0;
#endif // #if SOAK_TEST
    }

#if SOAK_TEST
    printf( "\ntest stopped\n\n" );
#endif // #if SOAK_TEST

    ShutdownYojimbo();

    return 0;
}
