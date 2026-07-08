/*
    Fuzz target: netcode_read_packet().

    Feeds arbitrary bytes into netcode's packet-read path — prefix/type parsing, length
    checks, sequence decoding, AEAD decryption and connect-token decryption — the code
    that runs on raw, attacker-controlled UDP payloads before a client or server trusts
    anything. Every packet type is allowed and fixed (zero) keys are used, so the header
    parse and the decrypt/MAC-reject paths are all exercised.

    netcode_read_packet and the replay-protection / packet-type symbols are internal to
    netcode.c (not declared in netcode.h), so the harness includes the translation unit
    directly. netcode.c's own tests are behind NETCODE_ENABLE_TESTS (off) and it defines
    no main, so this just pulls in the implementation.
*/

#include "netcode.c"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define FUZZ_PROTOCOL_ID     0x1122334455667788ULL
#define FUZZ_TIMESTAMP       ( (uint64_t) 1000000 )

static int g_init = 0;
static uint8_t g_packet_key[NETCODE_KEY_BYTES];
static uint8_t g_private_key[NETCODE_KEY_BYTES];

static void ensure_init( void )
{
    if ( g_init )
        return;
    netcode_init();
    memset( g_packet_key, 0, sizeof( g_packet_key ) );
    memset( g_private_key, 0, sizeof( g_private_key ) );
    g_init = 1;
}

int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    ensure_init();

    // Mirror the real receive path: netcode never hands more than a full UDP packet to
    // the reader, and it ignores empty buffers.
    if ( size == 0 || size > NETCODE_MAX_PACKET_BYTES )
        return 0;

    // netcode_read_packet takes a mutable buffer; copy the immutable fuzzer input.
    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];
    memcpy( buffer, data, size );

    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 1, sizeof( allowed_packets ) );

    // Reset replay protection every input so a repeated sequence number never masks the
    // post-decrypt path on a later input.
    struct netcode_replay_protection_t replay_protection;
    netcode_replay_protection_reset( &replay_protection );

    uint64_t sequence = 0;

    void * packet = netcode_read_packet( buffer,
                                         (int) size,
                                         &sequence,
                                         g_packet_key,
                                         FUZZ_PROTOCOL_ID,
                                         FUZZ_TIMESTAMP,
                                         g_private_key,
                                         allowed_packets,
                                         &replay_protection,
                                         NULL,
                                         NULL );

    if ( packet )
        netcode_default_free_function( NULL, packet );

    return 0;
}

#include "fuzz_standalone.h"
