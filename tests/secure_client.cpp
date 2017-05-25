/*
    Secure Client.

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
#define LOGGING 1

#include "shared.h"
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

int ConnectMain( int argc, char * argv[] )
{
    (void)argc;
    (void)argv;

    printf( "\nconnecting client (secure)\n" );

    Allocator & allocator = GetDefaultAllocator();

    uint64_t clientId = 0;
    RandomBytes( (uint8_t*) &clientId, 8 );
    printf( "client id is %.16" PRIx64 "\n", clientId );

    Matcher matcher( allocator );

    if ( !matcher.Initialize() )
    {
        printf( "error: failed to initialize matcher\n" );
        return 1;
    }

    printf( "requesting match from https://localhost:8080\n" );

    matcher.RequestMatch( ProtocolId, clientId );

    if ( matcher.GetMatchStatus() == MATCH_FAILED )
    {
        printf( "\nRequest match failed. Is the matcher running? Please run \"premake5 matcher\" before you connect a secure client\n\n" );
        return 1;
    }

    MatchResponse matchResponse;

    matcher.GetMatchResponse( matchResponse );

    if ( matchResponse.numServerAddresses == 0 )
    {
        printf( "error: could not find a match\n" );
        return 1;
    }

    printf( "received match response\n" );

    double time = 100.0;

    NetworkTransport clientTransport( GetDefaultAllocator(), Address("0.0.0.0"), ProtocolId, time );

    if ( clientTransport.GetError() != SOCKET_ERROR_NONE )
    {
        printf( "error: failed to initialize client socket\n" );
        return 1;
    }
    
    printf( "client started on port %d\n", clientTransport.GetAddress().GetPort() );

    GameClient client( GetDefaultAllocator(), clientTransport, ClientServerConfig(), time );

    client.Connect( clientId,
                    matchResponse.serverAddresses,
                    matchResponse.numServerAddresses,
                    matchResponse.connectTokenData, 
                    matchResponse.connectTokenNonce, 
                    matchResponse.clientToServerKey,
                    matchResponse.serverToClientKey,
                    matchResponse.connectTokenExpireTimestamp );

    const double deltaTime = 0.1;

    signal( SIGINT, interrupt_handler );    

    while ( !quit )
    {
        client.SendPackets();

        clientTransport.WritePackets();

        clientTransport.ReadPackets();

        client.ReceivePackets();

        client.CheckForTimeOut();

        if ( client.IsDisconnected() )
            break;

        time += deltaTime;

        client.AdvanceTime( time );

        clientTransport.AdvanceTime( time );

        if ( client.ConnectionFailed() )
            break;

        yojimbo_sleep( deltaTime );
    }

    if ( quit )
        printf( "\nclient stopped\n" );

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

    int result = ConnectMain( argc, argv );

    ShutdownYojimbo();

    return result;
}
