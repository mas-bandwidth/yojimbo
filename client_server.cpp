/*
    Client/Server Testbed

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#define SERVER 1
#define CLIENT 1
#define MATCHER 1

#include "shared.h"

int main()
{
    printf( "\n" );

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
