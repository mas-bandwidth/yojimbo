/*
    Fuzz target: reliable_endpoint_receive_packet().

    Feeds arbitrary bytes into the reliable.io packet-receive path — packet-header
    parsing, ack decoding, and fragment reassembly — the code that runs on raw,
    attacker-controlled UDP payloads. The process-packet callback accepts everything
    so reassembled payloads are exercised too.
*/

#include "reliable.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static struct reliable_endpoint_t * g_endpoint = NULL;
static double g_time = 100.0;

static int fuzz_process_packet( void * context, uint64_t id, uint16_t sequence, uint8_t * data, int bytes )
{
    (void) context; (void) id; (void) sequence; (void) data; (void) bytes;
    return 1; /* accept every packet */
}

static void fuzz_transmit_packet( void * context, uint64_t id, uint16_t sequence, uint8_t * data, int bytes )
{
    (void) context; (void) id; (void) sequence; (void) data; (void) bytes;
}

static void ensure_init( void )
{
    if ( g_endpoint )
        return;
    reliable_init();
    struct reliable_config_t config;
    reliable_default_config( &config );
    config.transmit_packet_function = fuzz_transmit_packet;
    config.process_packet_function = fuzz_process_packet;
    g_endpoint = reliable_endpoint_create( &config, g_time );
}

int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    ensure_init();

    /* reliable_endpoint_receive_packet asserts packet_bytes > 0 and rejects packets
       larger than max_packet_size + headers; keep inputs in a sane range. */
    if ( size == 0 || size > 4096 )
        return 0;

    /* the API takes a mutable buffer, so copy the immutable fuzzer input */
    uint8_t buf[4096];
    memcpy( buf, data, size );

    reliable_endpoint_receive_packet( g_endpoint, buf, (int) size );

    /* advance time so the reassembly / received-packet sequence buffers cycle */
    g_time += 0.01;
    reliable_endpoint_update( g_endpoint, g_time );
    reliable_endpoint_clear_acks( g_endpoint );

    return 0;
}

#include "fuzz_standalone.h"
