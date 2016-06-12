/*
    Client/Server Testbed

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#define CLIENT 1

#include "shared.h"
#include <signal.h>

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

int ClientMain()
{
    if ( !InitializeNetwork() )
    {
        printf( "error: failed to initialize network\n" );
        return 1;
    }

    GamePacketFactory packetFactory;

    GameNetworkInterface clientInterface( packetFactory, 0 );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client socket\n" );
        return 1;
    }
    
    clientInterface.SetFlags( NETWORK_INTERFACE_FLAG_INSECURE_MODE );

    GameClient client( clientInterface );

    Address serverAddress( "173.255.195.190", ServerPort );
    //Address serverAddress( "::1", ServerPort );

    client.InsecureConnect( serverAddress );

    double time = 0.0;

    const double deltaTime = 0.1;

    signal( SIGINT, interrupt_handler );    

    while ( !quit )
    {
        client.SendPackets();

        clientInterface.WritePackets();

        clientInterface.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        if ( client.ConnectionFailed() )
            break;

        time += deltaTime;

        client.AdvanceTime( time );

        clientInterface.AdvanceTime( time );

        platform_sleep( deltaTime );
    }

    if ( quit )
        printf( "\nclient stopped\n" );

    if ( client.IsConnected() )
        client.Disconnect();

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

    int result = ClientMain();

    memory_shutdown();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
