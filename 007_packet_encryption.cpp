    /*
    Example source code for "Packet Encryption"

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

using namespace yojimbo;
using namespace protocol2;
using namespace network2;

const uint32_t ProtocolId = 0x12341651;
const int ServerPort = 50000;
const int ClientPort = 60000;

enum TestPacketTypes
{
    TEST_PACKET_A,
    TEST_PACKET_B,
    TEST_PACKET_C,
    TEST_PACKET_NUM_TYPES
};

struct Vector
{
    float x,y,z;
};

struct TestPacketA : public protocol2::Packet
{
    int a,b,c;

    TestPacketA() : Packet( TEST_PACKET_A )
    {
        a = random_int( -10, +10 );
        b = random_int( -20, +20 );
        c = random_int( -30, +30 );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -20, 20 );
        serialize_int( stream, c, -30, 30 );
        
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();

    bool operator == ( const TestPacketA & other ) const
    {
        return a == other.a && b == other.b && c == other.c;
    }

    bool operator != ( const TestPacketA & other ) const
    {
        return ! ( *this == other );
    }
};

static const int MaxItems = 32;

struct TestPacketB : public protocol2::Packet
{
    int numItems;
    int items[MaxItems];

    TestPacketB() : Packet( TEST_PACKET_B )
    {
        numItems = random_int( 0, MaxItems );
        for ( int i = 0; i < numItems; ++i )
            items[i] = random_int( -100, +100 );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, numItems, 0, MaxItems );
        for ( int i = 0; i < numItems; ++i )
            serialize_int( stream, items[i], -100, +100 );

        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();

    bool operator == ( const TestPacketB & other ) const
    {
        if ( numItems != other.numItems )
            return false;
        for ( int i = 0; i < numItems; ++i )
        {
            if ( items[i] != other.items[i] )
                return false;
        }
        return true;
    }

    bool operator != ( const TestPacketB & other ) const
    {
        return ! ( *this == other );
    }
};

struct TestPacketC : public protocol2::Packet
{
    Vector position;
    Vector velocity;

    TestPacketC() : Packet( TEST_PACKET_C )
    {
        position.x = random_float( -1000, +1000 );
        position.y = random_float( -1000, +1000 );
        position.z = random_float( -1000, +1000 );

        if ( rand() % 2 )
        {
            velocity.x = random_float( -100, +100 );
            velocity.y = random_float( -100, +100 );
            velocity.z = random_float( -100, +100 );
        }
        else
        {
            velocity.x = 0.0f;
            velocity.y = 0.0f;
            velocity.z = 0.0f;
        }
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_float( stream, position.x );
        serialize_float( stream, position.y );
        serialize_float( stream, position.z );

        bool at_rest = Stream::IsWriting && velocity.x == 0.0f && velocity.y == 0.0f && velocity.z == 0.0f;

        serialize_bool( stream, at_rest );

        if ( !at_rest )
        {
            serialize_float( stream, velocity.x );
            serialize_float( stream, velocity.y );
            serialize_float( stream, velocity.z );
        }
        else
        {
            if ( Stream::IsReading )
            {
                velocity.x = 0.0f;
                velocity.y = 0.0f;
                velocity.z = 0.0f;
            }
        }

        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();

    bool operator == ( const TestPacketC & other )
    {
        return position.x == other.position.x &&
               position.y == other.position.y &&
               position.z == other.position.z &&
               velocity.x == other.velocity.x &&
               velocity.y == other.velocity.y &&
               velocity.z == other.velocity.z;
    }
};

struct TestPacketFactory : public protocol2::PacketFactory
{
    TestPacketFactory() : PacketFactory( TEST_PACKET_NUM_TYPES ) {}

    protocol2::Packet* Create( int type )
    {
        switch ( type )
        {
            case TEST_PACKET_A: return new TestPacketA();
            case TEST_PACKET_B: return new TestPacketB();
            case TEST_PACKET_C: return new TestPacketC();
        }
        return NULL;
    }

    void Destroy( protocol2::Packet *packet )
    {
        delete packet;
    }
};

class TestNetworkInterface : public SocketInterface
{   
public:

    TestNetworkInterface( TestPacketFactory & packetFactory, uint16_t port ) : SocketInterface( memory_default_allocator(), packetFactory, ProtocolId, port )
    {
        EnablePacketEncryption();

        DisableEncryptionForPacketType( TEST_PACKET_A );

        assert( IsEncryptedPacketType( TEST_PACKET_A ) == false );
        assert( IsEncryptedPacketType( TEST_PACKET_B ) == true );
        assert( IsEncryptedPacketType( TEST_PACKET_C ) == true );
    }

    ~TestNetworkInterface()
    {
        ClearSendQueue();
        ClearReceiveQueue();
    }
};

int main()
{
    printf( "\npacket encryption\n\n" );

    if ( !InitializeCrypto() )
    {
        printf( "error: failed to initialize crypto!\n" );
        return 1;
    }

    memory_initialize();
    {
        srand( time( NULL ) );

        InitializeNetwork();

        Address clientAddress( "::1", ClientPort );
        Address serverAddress( "::1", ServerPort );

		TestPacketFactory packetFactory;

        TestNetworkInterface clientInterface( packetFactory, ClientPort );
        TestNetworkInterface serverInterface( packetFactory, ServerPort );

        uint8_t client_to_server_key[KeyBytes];
        uint8_t server_to_client_key[KeyBytes];

        GenerateKey( client_to_server_key );
        GenerateKey( server_to_client_key );

        uint64_t clientSequence = 0;
        uint64_t serverSequence = 0;

        clientInterface.AddEncryptionMapping( serverAddress, client_to_server_key, server_to_client_key );
        serverInterface.AddEncryptionMapping( clientAddress, server_to_client_key, client_to_server_key );

        if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
        {
            printf( "error: failed to initialize sockets\n" );
            return 1;
        }
        
        const int NumIterations = 10;

        double time = 0.0;

        printf( "----------------------------------------------------------\n" );

        for ( int i = 0; i < NumIterations; ++i )
        {
            printf( "t = %f\n", time );

            protocol2::Packet * clientToServerPacket = clientInterface.CreatePacket( rand() % TEST_PACKET_NUM_TYPES );
            protocol2::Packet * serverToClientPacket = serverInterface.CreatePacket( rand() % TEST_PACKET_NUM_TYPES );

            clientInterface.SendPacket( serverAddress, clientToServerPacket, clientSequence++ );
            serverInterface.SendPacket( clientAddress, serverToClientPacket, serverSequence++ );

            clientInterface.WritePackets( time );
            serverInterface.WritePackets( time );

            clientInterface.ReadPackets( time );
            serverInterface.ReadPackets( time );

            while ( true )
            {
                Address address;
                Packet * packet = clientInterface.ReceivePacket( address );
                if ( !packet )
                    break;
                
                printf( "client received packet type %d\n", packet->GetType() );

                clientInterface.DestroyPacket( packet );
            }

            while ( true )
            {
                Address address;
                Packet * packet = serverInterface.ReceivePacket( address );
                if ( !packet )
                    break;

                printf( "server received packet type %d\n", packet->GetType() );

                serverInterface.DestroyPacket( packet );
            }

            time += 0.1f;

            printf( "----------------------------------------------------------\n" );
        }

        printf( "client sent %" PRIu64 " encrypted packets\n", clientInterface.GetCounter( SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_WRITTEN ) );
        printf( "client sent %" PRIu64 " unencrypted packets\n", clientInterface.GetCounter( SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_WRITTEN ) );
        printf( "client received %" PRIu64 " encrypted packets\n", clientInterface.GetCounter( SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_READ ) );
        printf( "client received %" PRIu64 " unencrypted packets\n", clientInterface.GetCounter( SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_READ ) );
        printf( "client had %" PRIu64 " packet encrypt failures\n", clientInterface.GetCounter( SOCKET_INTERFACE_COUNTER_ENCRYPT_PACKET_FAILURES ) );
        printf( "client had %" PRIu64 " packet decrypt failures\n", clientInterface.GetCounter( SOCKET_INTERFACE_COUNTER_DECRYPT_PACKET_FAILURES ) );
        printf( "client had %" PRIu64 " packet read failures\n", clientInterface.GetCounter( SOCKET_INTERFACE_COUNTER_READ_PACKET_FAILURES ) );
        printf( "client had %" PRIu64 " packet write failures\n", clientInterface.GetCounter( SOCKET_INTERFACE_COUNTER_WRITE_PACKET_FAILURES ) );

        printf( "----------------------------------------------------------\n" );

        printf( "server sent %" PRIu64 " encrypted packets\n", serverInterface.GetCounter( SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_WRITTEN ) );
        printf( "server sent %" PRIu64 " unencrypted packets\n", serverInterface.GetCounter( SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_WRITTEN ) );
        printf( "server received %" PRIu64 " encrypted packets\n", serverInterface.GetCounter( SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_READ ) );
        printf( "server received %" PRIu64 " unencrypted packets\n", serverInterface.GetCounter( SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_READ ) );
        printf( "server had %" PRIu64 " packet encrypt failures\n", serverInterface.GetCounter( SOCKET_INTERFACE_COUNTER_ENCRYPT_PACKET_FAILURES ) );
        printf( "server had %" PRIu64 " packet decrypt failures\n", serverInterface.GetCounter( SOCKET_INTERFACE_COUNTER_DECRYPT_PACKET_FAILURES ) );
        printf( "client had %" PRIu64 " packet read failures\n", serverInterface.GetCounter( SOCKET_INTERFACE_COUNTER_READ_PACKET_FAILURES ) );
        printf( "client had %" PRIu64 " packet write failures\n", serverInterface.GetCounter( SOCKET_INTERFACE_COUNTER_WRITE_PACKET_FAILURES ) );

        printf( "----------------------------------------------------------\n" );

        ShutdownNetwork();
    }

    memory_shutdown();

    printf( "\n" );

    return 0;
}
