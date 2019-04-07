/*
    Yojimbo Client/Server Example.

    Copyright © 2016 - 2019, The Network Protocol Company, Inc.

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
#include <signal.h>
#include "shared.h"

using namespace yojimbo;

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

int ClientServerMain()
{
    double time = 100.0;

    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    printf( "starting server on port %d\n", ServerPort );

    Server server( GetDefaultAllocator(), privateKey, Address( "127.0.0.1", ServerPort ), config, adapter, time );

    server.Start( MaxClients );

    if ( !server.IsRunning() )
        return 1;

    printf( "started server\n" );

    uint64_t clientId = 0;
    random_bytes( (uint8_t*) &clientId, 8 );
    printf( "client id is %.16" PRIx64 "\n", clientId );

    Client client( GetDefaultAllocator(), Address("0.0.0.0"), config, adapter, time );

    Address serverAddress( "127.0.0.1", ServerPort );

    client.InsecureConnect( privateKey, clientId, serverAddress );

    const double deltaTime = 0.1;

    signal( SIGINT, interrupt_handler );    

    while ( !quit )
    {
        server.SendPackets();
        client.SendPackets();

        server.ReceivePackets();
        client.ReceivePackets();
     
        time += deltaTime;

        client.AdvanceTime( time );

        if ( client.IsDisconnected() )
            break;

        time += deltaTime;

        server.AdvanceTime( time );

        yojimbo_sleep( deltaTime );
    }

    client.Disconnect();
    server.Stop();

    return 0;
}

int main()
{
    printf( "\n[client/server]\n" );

    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize Yojimbo!\n" );
        return 1;
    }

    yojimbo_log_level( YOJIMBO_LOG_LEVEL_INFO );

    srand( (unsigned int) time( NULL ) );

    int result = ClientServerMain();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
