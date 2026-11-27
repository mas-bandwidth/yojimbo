/*
    Drives a real yojimbo client and server through a full lifecycle and prints
    every client state transition, for tools/conformance/verify_state_machine.py.

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

class ConformanceAdapter : public TestAdapter
{
public:

    Client * client;
    Server * server;

    ConformanceAdapter()
    {
        client = NULL;
        server = NULL;
    }

    void ClientSendLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) YOJIMBO_OVERRIDE
    {
        if ( server )
            server->ProcessLoopbackPacket( clientIndex, packetData, packetBytes, packetSequence );
    }

    void ServerSendLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) YOJIMBO_OVERRIDE
    {
        (void) clientIndex;
        if ( client )
            client->ProcessLoopbackPacket( packetData, packetBytes, packetSequence );
    }
};

static ConformanceAdapter conformanceAdapter;

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

    conformanceAdapter.client = &client;
    conformanceAdapter.server = &server;

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

    client.ConnectLoopback( 0, clientId, NumClientSlots );
    server.ConnectLoopbackClient( 0, clientId, NULL );
    note( client, "loopback_connect" );
    for ( int i = 0; i < 20; i++ )
    {
        client.SendPackets(); server.SendPackets();
        client.ReceivePackets(); server.ReceivePackets();
        time += dt;
        client.AdvanceTime( time ); server.AdvanceTime( time );
        note( client, "loopback_steady" );
        yojimbo_sleep( 0.001 );
    }
    client.DisconnectLoopback();
    note( client, "loopback_disconnect" );
    server.DisconnectLoopbackClient( 0 );

    client.InsecureConnect( privateKey, clientId, Address( "127.0.0.1", ConformancePort ) );
    note( client, "reconnect" );
    iterations = 0;
    while ( iterations++ < 200 )
    {
        client.SendPackets(); server.SendPackets();
        client.ReceivePackets(); server.ReceivePackets();
        time += dt;
        client.AdvanceTime( time ); server.AdvanceTime( time );
        note( client, "reconnect_handshake" );
        if ( client.IsConnected() ) break;
        if ( client.ConnectionFailed() ) { printf( "RESULT fail reconnect\n" ); rc = 1; break; }
        yojimbo_sleep( 0.001 );
    }
    if ( !client.IsConnected() && rc == 0 ) { printf( "RESULT fail reconnect_never_connected\n" ); rc = 1; }

    /* stop answering the connected client so the connection times out */
    for ( int i = 0; i < 400 && !client.ConnectionFailed(); i++ )
    {
        client.SendPackets();
        client.ReceivePackets();
        time += dt;
        client.AdvanceTime( time );
        note( client, "connection_error" );
        yojimbo_sleep( 0.001 );
    }
    if ( !client.ConnectionFailed() ) { printf( "RESULT fail no_connection_error\n" ); rc = 1; }

    client.Disconnect();
    note( client, "error_cleared" );

    server.Stop();
    if ( rc == 0 )
        printf( "RESULT ok complete\n" );
    }
    ShutdownYojimbo();
    return rc;
}
