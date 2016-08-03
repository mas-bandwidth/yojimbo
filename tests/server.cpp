/*
    Test Server

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
    ClientServerPacketFactory packetFactory;

#if SECURE_SERVER 
    printf( "secure server started on port %d\n", ServerPort );
#else // #if SECURE_SERVER
    printf( "server started on port %d\n", ServerPort );
#endif // #if SECURE_SERVER

    Address serverBindAddress( "0.0.0.0", ServerPort );

    Address serverPublicAddress( "127.0.0.1", ServerPort );

    GameNetworkTransport serverTransport( packetFactory, serverBindAddress );

    if ( serverTransport.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize server socket\n" );
        return 1;
    }
    
    GameServer server( GetDefaultAllocator(), serverTransport );

    server.SetServerAddress( serverPublicAddress );

#if !SECURE_SERVER && YOJIMBO_INSECURE_CONNECT

    server.SetFlags( SERVER_FLAG_ALLOW_INSECURE_CONNECT );
    
    serverTransport.SetFlags( TRANSPORT_FLAG_INSECURE_MODE );
    
#endif // #if !SECURE_SERVER && YOJIMBO_INSECURE_CONNECT

    server.Start();

    double time = 0.0;

    const double deltaTime = 0.1;

    signal( SIGINT, interrupt_handler );    

    while ( !quit )
    {
        server.SendPackets();

        serverTransport.WritePackets();

        serverTransport.ReadPackets();

        server.ReceivePackets();

        server.CheckForTimeOut();

        time += deltaTime;

        server.AdvanceTime( time );

        serverTransport.AdvanceTime( time );

        platform_sleep( deltaTime );
    }

    printf( "\nserver stopped\n" );

    server.DisconnectAllClients();

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

    int result = ServerMain();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
