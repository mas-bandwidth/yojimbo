/*
    Yojimbo Secure Client Example.

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

int ClientMain( int argc, char * argv[] )
{   
    (void) argc;
    (void) argv;

    printf( "\nconnecting client (secure)\n" );

    uint64_t clientId = 0;
    random_bytes( (uint8_t*) &clientId, 8 );
    printf( "client id is %.16" PRIx64 "\n", clientId );

    Matcher matcher( GetDefaultAllocator() );

    if ( !matcher.Initialize() )
    {
        printf( "error: failed to initialize matcher\n" );
        return 1;
    }

    printf( "requesting match from https://localhost:8080\n" );

    matcher.RequestMatch( ProtocolId, clientId, false );

    if ( matcher.GetMatchStatus() == MATCH_FAILED )
    {
        printf( "\nRequest match failed. Is the matcher running? Please run \"premake5 matcher\" before you connect a secure client\n" );
        return 1;
    }

    uint8_t connectToken[ConnectTokenBytes];
    matcher.GetConnectToken( connectToken );
    printf( "received connect token from matcher\n" );

    double time = 100.0;

    ClientServerConfig config;
    config.protocolId = ProtocolId;

    Client client( GetDefaultAllocator(), Address("0.0.0.0"), config, adapter, time );

    Address serverAddress( "127.0.0.1", ServerPort );

    if ( argc == 2 )
    {
        Address commandLineAddress( argv[1] );
        if ( commandLineAddress.IsValid() )
        {
            if ( commandLineAddress.GetPort() == 0 )
                commandLineAddress.SetPort( ServerPort );
            serverAddress = commandLineAddress;
        }
    }

    client.Connect( clientId, connectToken );

    if ( client.IsDisconnected() )
        return 1;

    char addressString[256];
    client.GetAddress().ToString( addressString, sizeof( addressString ) );
    printf( "client address is %s\n", addressString );

    const double deltaTime = 0.1;

    signal( SIGINT, interrupt_handler );    

    while ( !quit )
    {
        client.SendPackets();

        client.ReceivePackets();

        if ( client.IsDisconnected() )
            break;
     
        time += deltaTime;

        client.AdvanceTime( time );

        if ( client.ConnectionFailed() )
            break;

        yojimbo_sleep( deltaTime );    
    }

    client.Disconnect();
    
    return 0;
}

int main( int argc, char * argv[] )
{
    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize Yojimbo!\n" );
        return 1;
    }

    yojimbo_log_level( YOJIMBO_LOG_LEVEL_INFO );

    srand( (unsigned int) time( NULL ) );

    int result = ClientMain( argc, argv );

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
