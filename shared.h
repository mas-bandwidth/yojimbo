/*
    Test Game shared code (client and server)

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
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

const int ClientPort = 40000;
const int ServerPort = 50000;

static uint8_t private_key[KeyBytes];

static bool verbose_logging = true; //false;

#if MATCHER

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

#endif // #if MATCHER

#if SERVER

class GameServer : public Server
{
public:

    GameServer( NetworkInterface & networkInterface ) : Server( networkInterface )
    {
        SetPrivateKey( private_key );
    }

protected:

    void OnClientConnect( int clientIndex )
    {
        char addressString[256];
        GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
        printf( "client %d connected (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
    }

    void OnClientDisconnect( int clientIndex )
    {
        char addressString[256];
        GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
        printf( "client %d disconnected (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
    }

    void OnClientTimedOut( int clientIndex )
    {
        char addressString[256];
        GetClientAddress( clientIndex ).ToString( addressString, sizeof( addressString ) );
        printf( "client %d timed out (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, GetClientId( clientIndex ) );
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
};

#endif // #if SERVER

#if CLIENT

class GameClient : public Client
{
public:

    GameClient( NetworkInterface & networkInterface ) : Client( networkInterface )
    {
        // ...
    }

    ~GameClient()
    {
        // ...
    }

    void OnConnect( const Address & address )
    {
        char addressString[256];
        address.ToString( addressString, sizeof( addressString ) );
        printf( "client connecting to %s\n", addressString );
    }

    void OnClientStateChange( int previousState, int currentState )
    {
        assert( previousState != currentState );
        const char * previousStateString = GetClientStateName( previousState );
        const char * currentStateString = GetClientStateName( currentState );
        printf( "client changed state from '%s' to '%s'\n", previousStateString, currentStateString );
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
};

#endif // #if CLIENT

class GamePacketFactory : public ClientServerPacketFactory
{
public:
    GamePacketFactory() : ClientServerPacketFactory( CLIENT_SERVER_NUM_PACKETS ) {}
};

class GameNetworkInterface : public SocketInterface
{   
public:

    GameNetworkInterface( GamePacketFactory & packetFactory, uint16_t port ) : SocketInterface( memory_default_allocator(), packetFactory, ProtocolId, port )
    {
        EnablePacketEncryption();
        DisableEncryptionForPacketType( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );
    }

    ~GameNetworkInterface()
    {
        ClearSendQueue();
        ClearReceiveQueue();
    }
};

#endif // #ifndef SHARED_H
