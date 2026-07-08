/*
    Seed-corpus generator for the C fuzz targets (fuzz_netcode, fuzz_reliable).

    Emits a handful of valid packets so libFuzzer starts from inputs that already reach the
    post-decrypt / reassembly code instead of rediscovering the wire format from random
    bytes. Run it (see fuzz/README.md) to (re)generate the fuzz/corpus/<target> seed files;
    the produced files are committed so CI needs only the corpus, not this generator.

    Includes netcode.c directly (its writer and packet-type symbols are internal to that TU,
    same as the harness) and links reliable.c through its public API.

    Usage: gen_seed_corpus <corpus-root>   (writes <root>/fuzz_netcode, <root>/fuzz_reliable)
*/

#include "netcode.c"

#include "fuzz_netcode_params.h"
#include "reliable.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>

static void make_dir( const char * path )
{
    // single level; corpus root is created before its subdirs
    if ( mkdir( path, 0755 ) != 0 && errno != EEXIST )
    {
        fprintf( stderr, "error: cannot create %s: %s\n", path, strerror( errno ) );
        exit( 1 );
    }
}

static void write_seed( const char * dir, const char * name, const uint8_t * data, int bytes )
{
    char path[512];
    snprintf( path, sizeof( path ), "%s/%s", dir, name );
    FILE * f = fopen( path, "wb" );
    if ( !f )
    {
        fprintf( stderr, "error: cannot write %s: %s\n", path, strerror( errno ) );
        exit( 1 );
    }
    if ( bytes > 0 )
        fwrite( data, 1, (size_t) bytes, f );
    fclose( f );
    printf( "  %s (%d bytes)\n", path, bytes );
}

// --- netcode -------------------------------------------------------------------------

// Read the packet back exactly as fuzz_netcode.c does, to prove the seed decrypts and
// parses (i.e. it reaches the code past the AEAD check, not just the prefix parse).
static void verify_netcode( const uint8_t * data, int bytes )
{
    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];
    memcpy( buffer, data, (size_t) bytes );

    uint8_t key[NETCODE_KEY_BYTES];
    memset( key, 0, sizeof( key ) );

    uint8_t allowed[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed, 1, sizeof( allowed ) );

    struct netcode_replay_protection_t replay;
    netcode_replay_protection_reset( &replay );

    uint64_t sequence = 0;
    void * packet = netcode_read_packet( buffer, bytes, &sequence, key, FUZZ_PROTOCOL_ID,
                                         FUZZ_TIMESTAMP, key, allowed, &replay, NULL, NULL );
    assert( packet && "generated netcode seed failed to read back" );
    netcode_default_free_function( NULL, packet );
}

static void gen_netcode( const char * root )
{
    char dir[512];
    snprintf( dir, sizeof( dir ), "%s/fuzz_netcode", root );
    make_dir( dir );

    uint8_t key[NETCODE_KEY_BYTES];
    memset( key, 0, sizeof( key ) );          // harness reads with an all-zero packet key

    const uint64_t sequence = 1000;
    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    // keep-alive: encrypts to a fixed small packet
    {
        struct netcode_connection_keep_alive_packet_t p;
        p.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
        p.client_index = 0;
        p.max_clients = 2;
        int bytes = netcode_write_packet( &p, buffer, sizeof( buffer ), sequence, key, FUZZ_PROTOCOL_ID );
        verify_netcode( buffer, bytes );
        write_seed( dir, "keep_alive", buffer, bytes );
    }

    // payload: exercises the variable-length decrypt + payload-size checks
    {
        const int payload_bytes = 256;
        struct netcode_connection_payload_packet_t * p =
            netcode_create_payload_packet( payload_bytes, NULL, NULL );
        int i;
        for ( i = 0; i < payload_bytes; ++i )
            p->payload_data[i] = (uint8_t) i;
        int bytes = netcode_write_packet( p, buffer, sizeof( buffer ), sequence + 1, key, FUZZ_PROTOCOL_ID );
        verify_netcode( buffer, bytes );
        write_seed( dir, "payload", buffer, bytes );
        netcode_default_free_function( NULL, p );
    }

    // disconnect: zero-length encrypted payload
    {
        struct netcode_connection_disconnect_packet_t p;
        p.packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;
        int bytes = netcode_write_packet( &p, buffer, sizeof( buffer ), sequence + 2, key, FUZZ_PROTOCOL_ID );
        verify_netcode( buffer, bytes );
        write_seed( dir, "disconnect", buffer, bytes );
    }
}

// --- netcode connect token (decrypted private token) ---------------------------------

static void gen_connect_token( const char * root )
{
    char dir[512];
    snprintf( dir, sizeof( dir ), "%s/fuzz_netcode_connect_token", root );
    make_dir( dir );

    struct
    {
        const char * name;
        const char * address;
    } cases[] = {
        { "ipv4", "127.0.0.1:40000" },
        { "ipv6", "[::1]:40000" },
    };

    for ( size_t c = 0; c < sizeof( cases ) / sizeof( cases[0] ); ++c )
    {
        struct netcode_address_t address;
        char address_string[64];
        snprintf( address_string, sizeof( address_string ), "%s", cases[c].address );
        if ( netcode_parse_address( address_string, &address ) != NETCODE_OK )
        {
            fprintf( stderr, "error: cannot parse %s\n", cases[c].address );
            exit( 1 );
        }

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        memset( user_data, 0, sizeof( user_data ) );

        struct netcode_connect_token_private_t token;
        netcode_generate_connect_token_private( &token, 0x1234567890abcdefULL, 30, 1, &address, user_data );

        uint8_t buffer[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
        netcode_write_connect_token_private( &token, buffer, sizeof( buffer ) );

        // prove the seed reads back through the target parser
        struct netcode_connect_token_private_t readback;
        assert( netcode_read_connect_token_private( buffer, sizeof( buffer ), &readback ) == NETCODE_OK
                && "generated connect-token seed failed to read back" );

        write_seed( dir, cases[c].name, buffer, (int) sizeof( buffer ) );
    }
}

// --- reliable ------------------------------------------------------------------------

static const char * g_reliable_dir;
static int g_reliable_index;
static struct reliable_endpoint_t * g_reliable_receiver;
static int g_reliable_received;

static int reliable_accept_all( void * ctx, uint64_t id, uint16_t seq, uint8_t * data, int bytes )
{
    (void) ctx; (void) id; (void) seq; (void) data; (void) bytes;
    return 1;
}

static int reliable_count_received( void * ctx, uint64_t id, uint16_t seq, uint8_t * data, int bytes )
{
    (void) ctx; (void) id; (void) seq; (void) data; (void) bytes;
    g_reliable_received++;
    return 1;
}

static void reliable_receiver_noop_transmit( void * ctx, uint64_t id, uint16_t seq, uint8_t * data, int bytes )
{
    (void) ctx; (void) id; (void) seq; (void) data; (void) bytes;
}

// As each wire packet is produced, write it as a seed and also feed a copy into a receiver
// endpoint so the generator proves the seeds parse and (for the fragments) reassemble.
static void reliable_capture_transmit( void * ctx, uint64_t id, uint16_t seq, uint8_t * data, int bytes )
{
    (void) ctx; (void) id; (void) seq;
    char name[64];
    snprintf( name, sizeof( name ), "packet_%02d", g_reliable_index++ );
    write_seed( g_reliable_dir, name, data, bytes );

    uint8_t copy[16 * 1024];
    assert( bytes <= (int) sizeof( copy ) );
    memcpy( copy, data, (size_t) bytes );
    reliable_endpoint_receive_packet( g_reliable_receiver, copy, bytes );
}

static void gen_reliable( const char * root )
{
    char dir[512];
    snprintf( dir, sizeof( dir ), "%s/fuzz_reliable", root );
    make_dir( dir );
    g_reliable_dir = dir;
    g_reliable_index = 0;
    g_reliable_received = 0;

    reliable_init();

    struct reliable_config_t receiver_config;
    reliable_default_config( &receiver_config );
    receiver_config.transmit_packet_function = reliable_receiver_noop_transmit;
    receiver_config.process_packet_function = reliable_count_received;
    g_reliable_receiver = reliable_endpoint_create( &receiver_config, 100.0 );

    struct reliable_config_t config;
    reliable_default_config( &config );
    config.transmit_packet_function = reliable_capture_transmit;
    config.process_packet_function = reliable_accept_all;

    struct reliable_endpoint_t * endpoint = reliable_endpoint_create( &config, 100.0 );

    // a small payload -> one regular (non-fragmented) packet: prefix byte + header + payload
    uint8_t small_payload[512];
    int i;
    for ( i = 0; i < (int) sizeof( small_payload ); ++i )
        small_payload[i] = (uint8_t) i;
    reliable_endpoint_send_packet( endpoint, small_payload, (int) sizeof( small_payload ) );

    // a payload above config.fragment_above -> several fragment packets (reassembly path)
    uint8_t large_payload[3000];
    for ( i = 0; i < (int) sizeof( large_payload ); ++i )
        large_payload[i] = (uint8_t) ( i * 7 );
    reliable_endpoint_send_packet( endpoint, large_payload, (int) sizeof( large_payload ) );

    // the small packet delivers directly and the large one delivers once reassembled
    assert( g_reliable_received >= 2 && "generated reliable seeds failed to parse/reassemble" );

    reliable_endpoint_destroy( endpoint );
    reliable_endpoint_destroy( g_reliable_receiver );
    reliable_term();
}

int main( int argc, char ** argv )
{
    const char * root = ( argc > 1 ) ? argv[1] : "fuzz/corpus";
    make_dir( root );

    netcode_init();  // once for the whole process; netcode asserts on a second init

    printf( "generating netcode seeds:\n" );
    gen_netcode( root );

    printf( "generating netcode connect-token seeds:\n" );
    gen_connect_token( root );

    printf( "generating reliable seeds:\n" );
    gen_reliable( root );

    printf( "done\n" );
    return 0;
}
