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

static bool verbose_logging = false;

inline int GetNumBitsForMessage( uint16_t sequence )
{
    static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 5, 3, 7, 50 };
    const int modulus = sizeof( messageBitsArray ) / sizeof( int );
    const int index = sequence % modulus;
    return messageBitsArray[index];
}

struct GameMessage : public Message
{
    uint16_t sequence;

    GameMessage()
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

struct GameBlockMessage : public BlockMessage
{
    uint16_t sequence;

    GameBlockMessage()
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

enum GameMessageType
{
    GAME_MESSAGE,
    GAME_BLOCK_MESSAGE,
    NUM_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START( GameMessageFactory, MessageFactory, NUM_MESSAGE_TYPES );
    YOJIMBO_DECLARE_MESSAGE_TYPE( GAME_MESSAGE, GameMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( GAME_BLOCK_MESSAGE, GameBlockMessage );
YOJIMBO_MESSAGE_FACTORY_FINISH();

#if SERVER

static uint8_t private_key[KeyBytes] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                                         0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                                         0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                         0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

class LocalMatcher
{
    uint64_t m_nonce;

public:

    LocalMatcher()
    {
        m_nonce = 0;
    }

    bool RequestMatch( uint64_t clientId, uint8_t * tokenData, uint8_t * tokenNonce, uint8_t * clientToServerKey, uint8_t * serverToClientKey, uint64_t & connectTokenExpireTimestamp, int & numServerAddresses, Address * serverAddresses )
    {
        if ( clientId == 0 )
            return false;

        numServerAddresses = 1;
        serverAddresses[0] = Address( "::1", ServerPort );

        ConnectToken token;
        GenerateConnectToken( token, clientId, numServerAddresses, serverAddresses, ProtocolId, 10 );

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

class GameServer : public Server
{
public:

    explicit GameServer( Allocator & allocator, Transport & transport ) : Server( allocator, transport )
    {
        SetPrivateKey( private_key );
    }

    explicit GameServer( Allocator & allocator, Transport & transport, const ClientServerConfig & config ) 
        : Server( allocator, transport, config )
    {
        SetPrivateKey( private_key );
    }

    MessageFactory * CreateMessageFactory( int /*clientIndex*/ )
    {
        return YOJIMBO_NEW( GetDefaultAllocator(), GameMessageFactory, GetDefaultAllocator() );
    }

protected:

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

            case SERVER_CHALLENGE_RESPONSE_IGNORED_ADDRESS_ALREADY_CONNECTED:
                printf( "ignored challenge response from %s. address already connected\n", addressString );
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

#ifndef QUIET

    void OnPacketSent( int packetType, const Address & to, bool immediate )
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
            case CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE:         packetTypeString = "challenge response";        break;
            case CLIENT_SERVER_PACKET_HEARTBEAT:                  packetTypeString = "heartbeat";                 break;  
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

    void OnConnectionFragmentReceived( Connection * /*connection*/, uint16_t /*messageId*/, uint16_t fragmentId, int /*fragmentBytes*/, int /*channelId*/ )
    {
#if !YOJIMBO_DEBUG_SPAM
        printf( "received fragment %d\n", fragmentId );
#else
        (void)fragmentId;
#endif // #if !YOJIMBO_DEBUG_SPAM
    }

#endif // #ifndef QUIET
};

#endif // #if SERVER

#if CLIENT

class GameClient : public Client
{
public:

    explicit GameClient( Allocator & allocator, Transport & transport ) : Client( allocator, transport )
    {
        // ...
    }

    explicit GameClient( Allocator & allocator, Transport & transport, const ClientServerConfig & config ) 
        : Client( allocator, transport, config )
    {
        // ...
    }

    ~GameClient()
    {
        // ...
    }

    MessageFactory * CreateMessageFactory()
    {
        return YOJIMBO_NEW( GetDefaultAllocator(), GameMessageFactory, GetDefaultAllocator() );
    }

#ifndef QUIET

    void OnConnect( const Address & address )
    {
        char addressString[MaxAddressLength];
        address.ToString( addressString, sizeof( addressString ) );
        printf( "client connecting to %s\n", addressString );
    }

    void OnClientStateChange( int previousState, int currentState )
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
            case CLIENT_SERVER_PACKET_HEARTBEAT:                  packetTypeString = "heartbeat";                 break;  
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
            from.ToString( addressString, sizeof( addressString ) );
            printf( "client received %s packet from %s\n", packetTypeString, addressString );
        }
    }

#endif // #ifndef QUIET
};

#endif // #if CLIENT

class GameNetworkTransport : public SocketTransport
{   
public:

    GameNetworkTransport( PacketFactory & packetFactory, const Address & address = Address( "0.0.0.0" ) ) 
        : SocketTransport( GetDefaultAllocator(), packetFactory, address, ProtocolId )
    {
        // ...
    }

    ~GameNetworkTransport()
    {
        ClearSendQueue();
        ClearReceiveQueue();
    }
};

#endif // #ifndef SHARED_H
