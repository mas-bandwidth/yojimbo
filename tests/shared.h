/*
    Test Game shared code (client and server)

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

#ifndef SHARED_H
#define SHARED_H

#include "yojimbo.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <map>

using namespace std;
using namespace yojimbo;

const uint32_t ProtocolId = 0x12341651;

const int ClientPort = 30000;
const int ServerPort = 40000;

#if LOGGING
static bool verbose_logging = false;
#endif // #if LOGGING

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

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
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

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
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

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestUserPacket : public Packet
{
    uint32_t sequence;
    uint32_t a,b,c;

    TestUserPacket()
    {
        sequence = 0;
        a = 0;
        b = 0;
        c = 0;
    }

    void Initialize( uint32_t seq )
    {
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

        assert( a == sequence % 2 );
        assert( b == sequence % 3 );
        assert( c == sequence % 5 );

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum TestPacketTypes
{
    TEST_PACKET_A = CLIENT_SERVER_NUM_PACKETS,          // because we are extending client/server packets
    TEST_PACKET_B,
    TEST_PACKET_C,
    TEST_USER_PACKET,
    NUM_TEST_PACKETS
};

static const int TEST_PACKET_CONNECTION = CLIENT_SERVER_PACKET_CONNECTION;

YOJIMBO_PACKET_FACTORY_START( TestPacketFactory, ClientServerPacketFactory, NUM_TEST_PACKETS );
    YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_A, TestPacketA );
    YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_B, TestPacketB );
    YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_C, TestPacketC );
    YOJIMBO_DECLARE_PACKET_TYPE( TEST_USER_PACKET, TestUserPacket );
YOJIMBO_PACKET_FACTORY_FINISH();

inline int GetNumBitsForMessage( uint16_t sequence )
{
    static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 5, 3, 7, 50 };
    const int modulus = sizeof( messageBitsArray ) / sizeof( int );
    const int index = sequence % modulus;
    return messageBitsArray[index];
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

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
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

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestSerializeFailOnReadMessage : public Message
{
    template <typename Stream> bool Serialize( Stream & /*stream*/ )
    {        
        return !Stream::IsReading;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestExhaustStreamAllocatorOnReadMessage : public Message
{
    template <typename Stream> bool Serialize( Stream & stream )
    {        
        if ( Stream::IsReading )
        {
            const int NumBuffers = 100;

            void * buffers[NumBuffers];

            memset( buffers, 0, sizeof( buffers ) );

            for ( int i = 0; i < NumBuffers; ++i )
            {
                buffers[i] = YOJIMBO_ALLOCATE( stream.GetAllocator(), 1024 * 1024 );
            }

            for ( int i = 0; i < NumBuffers; ++i )
            {
                YOJIMBO_FREE( stream.GetAllocator(), buffers[i] );
            }
        }

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum TestMessageType
{
    TEST_MESSAGE,
    TEST_BLOCK_MESSAGE,
    TEST_SERIALIZE_FAIL_ON_READ_MESSAGE,
    TEST_EXHAUST_STREAM_ALLOCATOR_ON_READ_MESSAGE,
    NUM_TEST_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START( TestMessageFactory, MessageFactory, NUM_TEST_MESSAGE_TYPES );
    YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_MESSAGE, TestMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_BLOCK_MESSAGE, TestBlockMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_SERIALIZE_FAIL_ON_READ_MESSAGE, TestSerializeFailOnReadMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_EXHAUST_STREAM_ALLOCATOR_ON_READ_MESSAGE, TestExhaustStreamAllocatorOnReadMessage );
YOJIMBO_MESSAGE_FACTORY_FINISH();

#if SERVER || MATCHER

static uint8_t private_key[KeyBytes] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                                         0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                                         0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                         0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

#endif // #if SERVER || MATCHER

#if MATCHER

class LocalMatcher
{
    uint64_t m_nonce;

public:

    LocalMatcher()
    {
        m_nonce = 0;
    }

    bool RequestMatch( uint64_t clientId, 
                       uint8_t * tokenData, 
                       uint8_t * tokenNonce, 
                       uint8_t * clientToServerKey, 
                       uint8_t * serverToClientKey, 
                       uint64_t & connectTokenExpireTimestamp,
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
        GenerateConnectToken( token, clientId, numServerAddresses, serverAddresses, ProtocolId, 10 );
        token.expireTimestamp += timestampOffsetInSeconds;

        memcpy( clientToServerKey, token.clientToServerKey, KeyBytes );
        memcpy( serverToClientKey, token.serverToClientKey, KeyBytes );

        if ( !EncryptConnectToken( token, tokenData, (const uint8_t*) &m_nonce, private_key ) )
            return false;

        assert( NonceBytes == 8 );

        memcpy( tokenNonce, &m_nonce, NonceBytes );

        connectTokenExpireTimestamp = token.expireTimestamp;

        m_nonce++;

        return true;
    }
};

#endif // #if MATCHER

#if SERVER

class GameServer : public Server
{
    uint32_t m_userPacketSequence;
    uint64_t m_numUserPacketsReceived[MaxClients];

    void Initialize()
    {
        SetPrivateKey( private_key );
        m_userPacketSequence = 0;
        memset( m_numUserPacketsReceived, 0, sizeof( m_numUserPacketsReceived ) );
    }

public:

    explicit GameServer( Allocator & allocator, Transport & transport, const ClientServerConfig & config, double time ) 
        : Server( allocator, transport, config, time )
    {
        Initialize();
    }

    uint64_t GetNumUserPacketsReceived( int clientIndex ) const
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < GetMaxClients() );
        return m_numUserPacketsReceived[clientIndex];
    }

    void SendUserPacketToClient( int clientIndex )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < GetMaxClients() );
        assert( IsClientConnected( clientIndex ) );
        TestUserPacket * packet = (TestUserPacket*) CreateClientPacket( clientIndex, TEST_USER_PACKET );
        assert( packet );
        packet->Initialize( ++m_userPacketSequence );
        SendPacketToConnectedClient( clientIndex, packet );
    }

    bool ProcessUserPacket( int clientIndex, Packet * packet )
    {
        if ( packet->GetType() == TEST_USER_PACKET )
        {
            m_numUserPacketsReceived[clientIndex]++;
            return true;
        }

        return false;
    }

protected:

    YOJIMBO_SERVER_PACKET_FACTORY( TestPacketFactory );

    YOJIMBO_SERVER_MESSAGE_FACTORY( TestMessageFactory );

#if LOGGING

    void OnConnectionRequest( ServerConnectionRequestAction action, const ConnectionRequestPacket & packet, const Address & address, const ConnectToken & connectToken )
    {
        char addressString[MaxAddressLength];
        address.ToString( addressString, sizeof( addressString ) );

        switch ( action )
        {
            case SERVER_CONNECTION_REQUEST_DENIED_SERVER_IS_FULL:
                printf( "denied connection request from %s. server is full\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_BECAUSE_FLAG_IS_SET:
                printf( "ignored connection request from %s. flag is set to ignore connection requests\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED:
                printf( "ignored connection request from %s. connect token has expired (expire timestamp = %" PRIx64 ", current time = %" PRIx64 ")\n", addressString, packet.connectTokenExpireTimestamp, (uint64_t) ::time( NULL ) );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_DECRYPT_CONNECT_TOKEN:
                printf( "ignored connection request from %s. failed to decrypt connect token\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_SERVER_ADDRESS_NOT_IN_WHITELIST:
                printf( "ignored connection request from %s. server address not in whitelist\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_IS_ZERO:
                printf( "ignored connection request from %s. client id is zero\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_ADDRESS_ALREADY_CONNECTED:
                printf( "ignored connection request from %s. address already connected\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED:
                printf( "ignored connection request from %s. client id %" PRIx64 " already connected\n", addressString, connectToken.clientId );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ADD_ENCRYPTION_MAPPING:
                printf( "ignored connection request from %s. failed to add encryption mapping\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED:
                printf( "ignored connection request from %s. connect token already used\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_GENERATE_CHALLENGE_TOKEN:
                printf( "ignored connection request from %s. failed to generate challenge token\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ALLOCATE_CHALLENGE_PACKET:
                printf( "ignored connection request from %s. failed to allocate challenge packet\n", addressString );
                break;

            case SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ENCRYPT_CHALLENGE_TOKEN:
                printf( "ignored connection request from %s. failed to encrypt challenge token\n", addressString );
                break;

            default:
                break;
        }
    }

    void OnChallengeResponse( ServerChallengeResponseAction action, const ChallengeResponsePacket & /*packet*/, const Address & address, const ChallengeToken & challengeToken )
    {
        char addressString[MaxAddressLength];
        address.ToString( addressString, sizeof( addressString ) );

        switch ( action )
        {
            case SERVER_CHALLENGE_RESPONSE_DENIED_SERVER_IS_FULL:
                printf( "denied challenge response from %s. server is full\n", addressString );
                break;

            case SERVER_CHALLENGE_RESPONSE_IGNORED_BECAUSE_FLAG_IS_SET:
                printf( "ignored challenge response from %s. flag is set to ignore challenge responses\n", addressString );
                break;

            case SERVER_CHALLENGE_RESPONSE_IGNORED_FAILED_TO_DECRYPT_CHALLENGE_TOKEN:
                printf( "ignored challenge response from %s. failed to decrypt challenge token\n", addressString );
                break;

            case SERVER_CHALLENGE_RESPONSE_IGNORED_CLIENT_ID_ALREADY_CONNECTED:
                printf( "ignored connection response from %s. client id %" PRIx64 " already connected\n", addressString, challengeToken.clientId );
                break;

            default:
                break;
        }
    }

    void OnClientConnect( int clientIndex )
    {
        char addressString[MaxAddressLength];
        GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
        printf( "client %d connected (client address = %s, client id = %.16" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
    }

    void OnClientDisconnect( int clientIndex )
    {
        char addressString[MaxAddressLength];
        GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
        printf( "client %d disconnected (client address = %s, client id = %.16" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
    }

    void OnClientTimedOut( int clientIndex )
    {
        char addressString[MaxAddressLength];
        GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
        printf( "client %d timed out (client address = %s, client id = %.16" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
    }

    void OnPacketSent( int packetType, const Address & to, bool immediate )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:          packetTypeString = "connection denied";     break;
            case CLIENT_SERVER_PACKET_CHALLENGE:                  packetTypeString = "challenge";             break;
            case CLIENT_SERVER_PACKET_KEEPALIVE:                  packetTypeString = "keep alive";            break;
            case CLIENT_SERVER_PACKET_DISCONNECT:                 packetTypeString = "disconnect";            break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            to.ToString( addressString, sizeof( addressString ) );
            printf( "server sent %s packet to %s%s\n", packetTypeString, addressString, immediate ? " (immediate)" : "" );
        }
    }

    void OnPacketReceived( int packetType, const Address & from )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:         packetTypeString = "connection request";        break;
            case CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:         packetTypeString = "challenge response";        break;
            case CLIENT_SERVER_PACKET_KEEPALIVE:                  packetTypeString = "keep alive";                break;  
            case CLIENT_SERVER_PACKET_DISCONNECT:                 packetTypeString = "disconnect";                break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            from.ToString( addressString, sizeof( addressString ) );
            printf( "server received '%s' packet from %s\n", packetTypeString, addressString );
        }
    }

    void OnConnectionFragmentReceived( Connection * /*connection*/, int /*channelId*/, uint16_t /*messageId*/, uint16_t fragmentId, int /*fragmentBytes*/, int /*numFragmentsReceived*/, int /*numFragmentsInBlock*/ )
    {
#if !YOJIMBO_DEBUG_SPAM
        printf( "received fragment %d\n", fragmentId );
#else
        (void)fragmentId;
#endif // #if !YOJIMBO_DEBUG_SPAM
    }

#endif // #if LOGGING
};

#endif // #if SERVER

#if CLIENT

class GameClient : public Client
{
    uint64_t m_numGamePacketsReceived;
    uint32_t m_gamePacketSequence;

public:

    void Initialize()
    {
        m_numGamePacketsReceived = 0;
        m_gamePacketSequence = 0;
    }

    explicit GameClient( Allocator & allocator, Transport & transport, const ClientServerConfig & config, double time ) 
        : Client( allocator, transport, config, time )
    {
        Initialize();
    }

    uint64_t GetNumUserPacketsReceived() const
    {
        return m_numGamePacketsReceived;
    }

    void SendUserPacketToServer()
    {
        TestUserPacket * packet = (TestUserPacket*) CreatePacket( TEST_USER_PACKET );
        assert( packet );
        packet->Initialize( ++m_gamePacketSequence );
        SendPacketToServer( packet );
    }

    bool ProcessUserPacket( Packet * packet )
    {
        if ( packet->GetType() == TEST_USER_PACKET )
        {
            m_numGamePacketsReceived++;
            return true;
        }

        return false;
    }

protected:

    YOJIMBO_CLIENT_PACKET_FACTORY( TestPacketFactory );

    YOJIMBO_CLIENT_MESSAGE_FACTORY( TestMessageFactory );

#if LOGGING

    void OnConnect( const Address & address )
    {
        char addressString[MaxAddressLength];
        address.ToString( addressString, sizeof( addressString ) );
        printf( "client connecting to %s\n", addressString );
    }

    void OnClientStateChange( ClientState previousState, ClientState currentState )
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

    void OnDisconnect()
    {
        printf( "client disconnected\n" );
    }

    void OnPacketSent( int packetType, const Address & to, bool immediate )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:         packetTypeString = "connection request";        break;
            case CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:         packetTypeString = "challenge response";        break;
            case CLIENT_SERVER_PACKET_KEEPALIVE:                  packetTypeString = "keep alive";                break;  
            case CLIENT_SERVER_PACKET_DISCONNECT:                 packetTypeString = "disconnect";                break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            to.ToString( addressString, sizeof( addressString ) );
            printf( "client sent %s packet to %s%s\n", packetTypeString, addressString, immediate ? " (immediate)" : "" );
        }
    }

    void OnPacketReceived( int packetType, const Address & from )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:          packetTypeString = "connection denied";     break;
            case CLIENT_SERVER_PACKET_CHALLENGE:                  packetTypeString = "challenge";             break;
            case CLIENT_SERVER_PACKET_KEEPALIVE:                  packetTypeString = "keep alive";            break;
            case CLIENT_SERVER_PACKET_DISCONNECT:                 packetTypeString = "disconnect";            break;

            default:
                return;
        }

        if ( verbose_logging )
        {
            char addressString[MaxAddressLength];
            from.ToString( addressString, sizeof( addressString ) );
            printf( "client received %s packet from %s\n", packetTypeString, addressString );
        }
    }

#endif // #if LOGGING
};

#endif // #if CLIENT

#endif // #ifndef SHARED_H
