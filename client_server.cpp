/*
    Example source code for "Securing Dedicated Servers"

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

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
            case PACKET_CONNECTION_DENIED:          packetTypeString = "connection denied";     break;
            case PACKET_CONNECTION_CHALLENGE:       packetTypeString = "challenge";             break;
            case PACKET_CONNECTION_HEARTBEAT:       packetTypeString = "heartbeat";             break;
            case PACKET_CONNECTION_DISCONNECT:      packetTypeString = "disconnect";            break;

            default:
                return;
        }

        char addressString[256];
        to.ToString( addressString, sizeof( addressString ) );
        printf( "server sent %s packet to %s%s\n", packetTypeString, addressString, immediate ? " (immediate)" : "" );
    }

    void OnPacketReceived( int packetType, const Address & from )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case PACKET_CONNECTION_REQUEST:         packetTypeString = "connection request";        break;
            case PACKET_CONNECTION_RESPONSE:        packetTypeString = "challenge response";        break;
            case PACKET_CONNECTION_HEARTBEAT:       packetTypeString = "heartbeat";                 break;  
            case PACKET_CONNECTION_DISCONNECT:      packetTypeString = "disconnect";                break;

            default:
                return;
        }

        char addressString[256];
        from.ToString( addressString, sizeof( addressString ) );
        printf( "server received %s packet from %s\n", packetTypeString, addressString );
    }
};

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
        printf( "client changed state from %s -> %s\n", previousStateString, currentStateString );
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
            case PACKET_CONNECTION_REQUEST:         packetTypeString = "connection request";        break;
            case PACKET_CONNECTION_RESPONSE:        packetTypeString = "challenge response";        break;
            case PACKET_CONNECTION_HEARTBEAT:       packetTypeString = "heartbeat";                 break;  
            case PACKET_CONNECTION_DISCONNECT:      packetTypeString = "disconnect";                break;

            default:
                return;
        }

        char addressString[256];
        to.ToString( addressString, sizeof( addressString ) );
        printf( "client sent %s packet to %s%s\n", packetTypeString, addressString, immediate ? " (immediate)" : "" );
    }

    void OnPacketReceived( int packetType, const Address & from )
    {
        const char * packetTypeString = NULL;

        switch ( packetType )
        {
            case PACKET_CONNECTION_DENIED:          packetTypeString = "connection denied";     break;
            case PACKET_CONNECTION_CHALLENGE:       packetTypeString = "challenge";             break;
            case PACKET_CONNECTION_HEARTBEAT:       packetTypeString = "heartbeat";             break;
            case PACKET_CONNECTION_DISCONNECT:      packetTypeString = "disconnect";            break;

            default:
                return;
        }

        char addressString[256];
        from.ToString( addressString, sizeof( addressString ) );
        printf( "client received %s packet from %s\n", packetTypeString, addressString );
    }
};

class GamePacketFactory : public ClientServerPacketFactory
{
    // ...
};

class GameNetworkInterface : public SocketInterface
{   
public:

    GameNetworkInterface( GamePacketFactory & packetFactory, uint16_t port ) : SocketInterface( memory_default_allocator(), packetFactory, ProtocolId, port )
    {
        EnablePacketEncryption();
        DisableEncryptionForPacketType( PACKET_CONNECTION_REQUEST );
    }

    ~GameNetworkInterface()
    {
        ClearSendQueue();
        ClearReceiveQueue();
    }
};

int main()
{
    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize Yojimbo!\n" );
        return 1;
    }

    srand( (unsigned int) time( NULL ) );

    memory_initialize();
    {
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
            return 1;
        }

        InitializeNetwork();

        GamePacketFactory packetFactory;

        Address clientAddress( "::1", ClientPort );
        Address serverAddress( "::1", ServerPort );

        GameNetworkInterface clientInterface( packetFactory, ClientPort );
        GameNetworkInterface serverInterface( packetFactory, ServerPort );

        if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
        {
            printf( "error: failed to initialize sockets\n" );
            return 1;
        }
        
        const int NumIterations = 20;

        double time = 0.0;

        GameClient client( clientInterface );

        GameServer server( serverInterface );

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
                break;
            }

            if ( client.IsConnected() && server.GetNumConnectedClients() == 1 )
                client.Disconnect();

            time += 0.1f;

            if ( !client.IsConnecting() && !client.IsConnected() && server.GetNumConnectedClients() == 0 )
                break;

            client.AdvanceTime( time );
            server.AdvanceTime( time );
    
            clientInterface.AdvanceTime( time );
            serverInterface.AdvanceTime( time );
        }

        ShutdownNetwork();
    }

    memory_shutdown();

    ShutdownYojimbo();

    printf( "\n" );

    return 0;
}
