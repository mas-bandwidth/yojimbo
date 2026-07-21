// yojimbo error-path driver: connection failure reaches CLIENT_STATE_ERROR

#include "yojimbo.h"
#include "shared.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

using namespace yojimbo;

static int last_state = -999;
static void note( ClientInterface & client, const char * phase )
{
    int s = client.GetClientState();
    if ( s != last_state )
    {
        printf( "STATE %d %d %s\n", last_state == -999 ? 0 : last_state, s, phase );
        last_state = s;
    }
}

int main()
{
    if ( !InitializeYojimbo() ) { printf( "RESULT fail init\n" ); return 1; }
    setvbuf( stdout, NULL, _IONBF, 0 );

    static TestAdapter errAdapter;

    ClientServerConfig config;
    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    double time = 100.0;
    {
        Client client( GetDefaultAllocator(), Address( "0.0.0.0" ), config, errAdapter, time );
        note( client, "initial" );
        client.InsecureConnect( privateKey, 0x1234ULL, Address( "127.0.0.1", 40599 ) );
        note( client, "after_connect_call" );

        for ( int i = 0; i < 4000; ++i )
        {
            client.SendPackets();
            client.ReceivePackets();
            time += 0.1;
            client.AdvanceTime( time );
            note( client, "waiting" );
            if ( client.ConnectionFailed() ) break;
        }

        if ( client.ConnectionFailed() )
            printf( "RESULT ok reached_error\n" );
        else
            printf( "RESULT fail no_error_state\n" );
    }
    ShutdownYojimbo();
    return 0;
}
