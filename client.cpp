/*
    Client/Server Testbed

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
    GamePacketFactory packetFactory;

    Address clientAddress = GetFirstNetworkAddress_IPV4();

    if ( !clientAddress.IsValid() )
    {
        printf( "error: no valid IPV4 address\n" );
        return 1;
    }

    GameNetworkInterface clientInterface( packetFactory, clientAddress );

    if ( clientInterface.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client socket\n" );
        return 1;
    }
    
    char clientAddressString[64];
    clientInterface.GetAddress().ToString( clientAddressString, sizeof( clientAddressString ) );
    printf( "client started on %s\n", clientAddressString );

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

        if ( client.IsDisconnected() )
            break;

        time += deltaTime;

        client.AdvanceTime( time );

        clientInterface.AdvanceTime( time );

        if ( client.ConnectionFailed() )
            break;

        platform_sleep( deltaTime );
    }

    if ( quit )
        printf( "\nclient stopped\n" );

    if ( client.IsConnected() )
        client.Disconnect();

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

    int result = ClientMain();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
