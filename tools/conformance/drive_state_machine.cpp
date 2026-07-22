/*
    Drives a real yojimbo client and server through a full lifecycle over UDP and
    prints every client state transition, for tools/conformance/verify_state_machine.py.

    Output: one line per transition,  STATE <from> <to> <phase>
    plus    RESULT <ok|fail> <note>
*/
#include "yojimbo.h"
#include "shared.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

using namespace yojimbo;

static const uint16_t ConformancePort = 41777;
static const int NumClientSlots = 4;

static TestAdapter conformanceAdapter;

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

    int rc = 0;
    {
    double time = 100.0;
    ClientServerConfig config;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    Server server( GetDefaultAllocator(), privateKey, Address( "127.0.0.1", ConformancePort ), config, conformanceAdapter, time );
    server.Start( NumClientSlots );
    if ( !server.IsRunning() ) { printf( "RESULT fail server_start\n" ); rc = 1; }

    uint64_t clientId = 0;
    yojimbo_random_bytes( (uint8_t*) &clientId, 8 );
    Client client( GetDefaultAllocator(), Address( "0.0.0.0" ), config, conformanceAdapter, time );

    note( client, "initial" );
    client.InsecureConnect( privateKey, clientId, Address( "127.0.0.1", ConformancePort ) );
    note( client, "after_connect_call" );

    const double dt = 0.1;
    int iterations = 0;
    while ( iterations++ < 200 )
    {
        client.SendPackets(); server.SendPackets();
        client.ReceivePackets(); server.ReceivePackets();
        time += dt;
        client.AdvanceTime( time ); server.AdvanceTime( time );
        note( client, "handshake" );
        if ( client.IsConnected() ) break;
        if ( client.ConnectionFailed() ) { printf( "RESULT fail handshake\n" ); rc = 1; break; }
        yojimbo_sleep( 0.001 );
    }
    if ( !client.IsConnected() && rc == 0 ) { printf( "RESULT fail never_connected\n" ); rc = 1; }

    /* stay connected a while so a spurious transition would show up */
    for ( int i = 0; i < 20; i++ )
    {
        client.SendPackets(); server.SendPackets();
        client.ReceivePackets(); server.ReceivePackets();
        time += dt;
        client.AdvanceTime( time ); server.AdvanceTime( time );
        note( client, "steady" );
        yojimbo_sleep( 0.001 );
    }

    client.Disconnect();
    note( client, "after_disconnect_call" );
    for ( int i = 0; i < 10; i++ )
    {
        client.SendPackets(); server.SendPackets();
        client.ReceivePackets(); server.ReceivePackets();
        time += dt;
        client.AdvanceTime( time ); server.AdvanceTime( time );
        note( client, "post_disconnect" );
    }

    server.Stop();
    printf( "RESULT ok complete\n" );
    }
    ShutdownYojimbo();
    return rc;
}
