/*
    Test Server

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#define SERVER 1

#include "shared.h"
#include <signal.h>

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

int ServerMain()
{
    if ( ! InitializeNetwork() )
    {
        printf( "error: failed to initialize network\n" );
        return 1;
    }

    GamePacketFactory packetFactory;

    Address serverAddress = GetFirstLocalAddress_IPV4();

    if ( !serverAddress.IsValid() )
    {
        printf( "error: no valid local IPV4 address\n" );
        return 1;
    }

    serverAddress.SetPort( ServerPort );

//    Address serverAddress( "::1", ServerPort );

    GameNetworkInterface serverInterface( packetFactory, serverAddress );

    if ( serverInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize server socket\n" );
        return 1;
    }

    char serverAddressString[64];
    serverAddress.ToString( serverAddressString, sizeof( serverAddressString ) );
    printf( "server started on %s\n", serverAddressString );
    
    GameServer server( serverInterface );

    server.SetServerAddress( serverAddress );

    server.SetFlags( SERVER_FLAG_ALLOW_INSECURE_CONNECT );

    serverInterface.SetFlags( NETWORK_INTERFACE_FLAG_INSECURE_MODE );

    server.Start();

    double time = 0.0;

    const double deltaTime = 0.1;

    signal( SIGINT, interrupt_handler );    

    while ( !quit )
    {
        server.SendPackets();

        serverInterface.WritePackets();

        serverInterface.ReadPackets();

        server.ReceivePackets();

        server.CheckForTimeOut();

        time += deltaTime;

        server.AdvanceTime( time );

        serverInterface.AdvanceTime( time );

        platform_sleep( deltaTime );
    }

    printf( "\nserver stopped\n" );

    server.DisconnectAllClients();

    ShutdownNetwork();

    return 0;
}

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

    int result = ServerMain();

    memory_shutdown();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
