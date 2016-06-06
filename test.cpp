/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

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
    TestPacketFactory() : PacketFactory( TEST_PACKET_NUM_TYPES ) {}

    Packet * Create( int type )
    {
        switch ( type )
        {
            case TEST_PACKET_A: return new TestPacketA();
            case TEST_PACKET_B: return new TestPacketB();
            case TEST_PACKET_C: return new TestPacketC();
        }
        return NULL;
    }

    void Destroy( Packet * packet )
    {
        delete packet;
    }
};

void test_packets()
{
    printf( "test packets\n" );

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

// todo: this is annoying -- all this just for htons? -- just implement your own htons glenn
#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX
#include <arpa/inet.h>
#include <netinet/in.h>
#elif YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX

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
            check( htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, 256 ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, 256 ), "fe80::202:b3ff:fe1e:8329" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 0 );

        for ( int i = 0; i < 8; ++i )
            check( htons( address6[i] ) == address.GetAddress6()[i] );

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
            check( htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, 256 ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

        Address address( address6, 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( htons( address6[i] ) == address.GetAddress6()[i] );

        check( strcmp( address.ToString( buffer, 256 ), "[fe80::202:b3ff:fe1e:8329]:65535" ) == 0 );
    }

    {
        const uint16_t address6[] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 };

        Address address( address6, 65535 );

        check( address.IsValid() );
        check( address.GetType() == ADDRESS_IPV6 );
        check( address.GetPort() == 65535 );

        for ( int i = 0; i < 8; ++i )
            check( htons( address6[i] ) == address.GetAddress6()[i] );

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

void PrintBytes( const uint8_t * data, int data_bytes )
{
    for ( int i = 0; i < data_bytes; ++i )
    {
        printf( "%02x", (int) data[i] );
        if ( i != data_bytes - 1 )
            printf( "-" );
    }
    printf( " (%d bytes)", data_bytes );
}

#include <sodium.h>

void test_packet_encryption()
{
    printf( "test_packet_encryption\n" );

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
        printf( "\npacket encryption failure!\n" );

        printf( " expected: " );
        PrintBytes( expected_encrypted_packet, expected_encrypted_length );
        printf( "\n" );

        printf( "      got: " );
        PrintBytes( encrypted_packet, encrypted_length );
        printf( "\n\n" );
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
    if ( !GenerateChallengeToken( connectToken, clientAddress, serverAddresses[0], connectTokenData, challengeToken ) )
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
    check( challengeToken.clientAddress == clientAddress );
    check( challengeToken.serverAddress == serverAddresses[0] );
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

    bool RequestMatch( uint64_t clientId, uint8_t * tokenData, uint8_t * tokenNonce, uint8_t * clientToServerKey, uint8_t * serverToClientKey, int & numServerAddresses, Address * serverAddresses )
    {
        if ( clientId == 0 )
            return false;

        numServerAddresses = 1;
        serverAddresses[0] = Address( "::1", ServerPort );

        ConnectToken token;
        GenerateConnectToken( token, clientId, numServerAddresses, serverAddresses, ProtocolId );

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

class TestServer : public Server
{
public:

    TestServer( NetworkInterface & networkInterface ) : Server( networkInterface )
    {
        SetPrivateKey( private_key );
    }

    // ...
};

class TestClient : public Client
{
public:

    TestClient( NetworkInterface & networkInterface ) : Client( networkInterface )
    {
        // ...
    }

    // ...
};

class TestClientServerPacketFactory : public ClientServerPacketFactory
{
    // ...
};

class TestNetworkInterface : public SocketInterface
{   
public:

    TestNetworkInterface( TestClientServerPacketFactory & packetFactory, uint16_t port ) : SocketInterface( memory_default_allocator(), packetFactory, ProtocolId, port )
    {
        EnablePacketEncryption();
        DisableEncryptionForPacketType( PACKET_CONNECTION_REQUEST );
    }

    ~TestNetworkInterface()
    {
        ClearSendQueue();
        ClearReceiveQueue();
    }
};

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

    TestClientServerPacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface clientInterface( packetFactory, ClientPort );
    TestNetworkInterface serverInterface( packetFactory, ServerPort );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client/server sockets\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestClient client( clientInterface );

    TestServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

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

    TestClientServerPacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface clientInterface( packetFactory, ClientPort );
    TestNetworkInterface serverInterface( packetFactory, ServerPort );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client/server sockets\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestClient client( clientInterface );

    TestServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

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

    // disconnect the client

    client.Disconnect();

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

        time += 0.1f;

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

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

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

    TestClientServerPacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface clientInterface( packetFactory, ClientPort );
    TestNetworkInterface serverInterface( packetFactory, ServerPort );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client/server sockets\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestClient client( clientInterface );

    TestServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

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

    TestClientServerPacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface clientInterface( packetFactory, ClientPort );
    TestNetworkInterface serverInterface( packetFactory, ServerPort );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client/server sockets\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestClient client( clientInterface );

    TestServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    // connect client to the server

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        exit( 1 );
    }

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

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

    TestClientServerPacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface clientInterface( packetFactory, ClientPort );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client/server sockets\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestClient client( clientInterface );

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();

        clientInterface.WritePackets();

        clientInterface.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += 1.0f;

        client.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
    }

    check( client.ConnectionFailed() );
    check( client.GetState() == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
}

void test_client_server_connection_response_timeout()
{
    printf( "test_client_server_connection_response_timeout\n" );

    // todo: to do this one need to provide the server with a callback that can reject (ignore) processing of a challenge response
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

    TestClientServerPacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface clientInterface( packetFactory, ClientPort );
    TestNetworkInterface serverInterface( packetFactory, ServerPort );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client/server sockets\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestClient client( clientInterface );

    TestServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

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

    for ( int i = 0; i < NumIterations; ++i )
    {
        server.SendPackets();

        serverInterface.WritePackets();

        serverInterface.ReadPackets();

        server.ReceivePackets();

        server.CheckForTimeOut();

        time += 1.0f;

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

    TestClientServerPacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface clientInterface( packetFactory, ClientPort );
    TestNetworkInterface serverInterface( packetFactory, ServerPort );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client/server sockets\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestClient client( clientInterface );

    TestServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

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

    for ( int i = 0; i < NumIterations; ++i )
    {
        client.SendPackets();

        clientInterface.WritePackets();

        clientInterface.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        time += 1.0f;

        if ( !client.IsConnected() )
            break;

        client.AdvanceTime( time );

        clientInterface.AdvanceTime( time );
    }

    check( !client.IsConnected() );
    check( client.GetState() == CLIENT_STATE_CONNECTION_TIMED_OUT );
}

struct ClientData
{
    uint64_t clientId;
    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];
    TestNetworkInterface * networkInterface;
    TestClient * client;
    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];
    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    ClientData()
    {
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
        delete networkInterface;
        delete client;
        networkInterface = NULL;
        client = NULL;
    }
};

void test_client_server_server_is_full()
{
    printf( "test_client_server_server_is_full\n" );

    Matcher matcher;

    GenerateKey( private_key );

    const int NumClients = 4;

    ClientData clientData[NumClients+1];

    TestClientServerPacketFactory packetFactory;

    for ( int i = 0; i < NumClients + 1; ++i )
    {
        clientData[i].clientId = i + 1;

        Address clientAddress( "::1", ClientPort + i );

        clientData[i].networkInterface = new TestNetworkInterface( packetFactory, ClientPort + i );

        if ( clientData[i].networkInterface->GetError() != SOCKET_ERROR_NONE )
        {
            printf( "error: failed to initialize client socket\n" );
        }

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

        clientData[i].client = new TestClient( *clientData[i].networkInterface );
    }

    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface serverInterface( packetFactory, ServerPort );

    if ( serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize server socket\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start( NumClients );

    // connect clients so server is full and cannot accept any more client connections

    for ( int i = 0; i < NumClients; ++i )
    {
        clientData[i].client->Connect( clientData[i].clientId, 
                                       serverAddress, 
                                       clientData[i].connectTokenData, 
                                       clientData[i].connectTokenNonce, 
                                       clientData[i].clientToServerKey, 
                                       clientData[i].serverToClientKey );
    }

    for ( int i = 0; i < NumIterations; ++i )
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

    clientData[NumClients].client->Connect( clientData[NumClients].clientId, 
                                            serverAddress, 
                                            clientData[NumClients].connectTokenData, 
                                            clientData[NumClients].connectTokenNonce, 
                                            clientData[NumClients].clientToServerKey, 
                                            clientData[NumClients].serverToClientKey );

    for ( int i = 0; i < NumIterations; ++i )
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

        if ( clientData[NumClients].client->GetState() == CLIENT_STATE_CONNECTION_DENIED )
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
    check( clientData[NumClients].client->GetState() == CLIENT_STATE_CONNECTION_DENIED );
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

    TestClientServerPacketFactory packetFactory;

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    TestNetworkInterface clientInterface( packetFactory, ClientPort );
    TestNetworkInterface serverInterface( packetFactory, ServerPort );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client/server sockets\n" );
        exit( 1 );
    }
    
    const int NumIterations = 20;

    double time = 0.0;

    TestClient client( clientInterface );

    TestServer server( serverInterface );

    server.SetServerAddress( serverAddress );
    
    server.Start();

    client.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

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

    TestNetworkInterface clientInterface2( packetFactory, ClientPort + 1 );

    if ( clientInterface2.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client #2 socket\n" );
        exit( 1 );
    }
    
    TestClient client2( clientInterface2 );

    client2.Connect( clientId, serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    for ( int i = 0; i < NumIterations; ++i )
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

        time += 1.0f;

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
    check( client2.GetState() == CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );
    check( server.GetCounter( SERVER_COUNTER_CONNECT_TOKEN_ALREADY_USED ) > 0 );
}

int main()
{
    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize yojimbo\n" );
        exit( 1 );
    }

    if ( !InitializeNetwork() )
    {
        printf( "error: failed to initialize network\n" );
        exit( 1 );
    }

    memory_initialize();
    {
        test_bitpacker();
        test_stream();
        test_packets();
        test_address_ipv4();
        test_address_ipv6();
        test_packet_sequence();
        test_packet_encryption();
        test_encryption_manager();
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

        // todo: connect token reuse
        // todo: connect token expiry
        // todo: connect token whitelist
        // todo: connect token invalid (random bytes), check counter

        // todo: challenge token reuse (different client address)
        // todo: challenge token whitelist (different server address)
        // todo: challenge token invalid (random bytes), check counter
    }

    memory_shutdown();

    ShutdownNetwork();

    ShutdownYojimbo();

    return 0;
}
