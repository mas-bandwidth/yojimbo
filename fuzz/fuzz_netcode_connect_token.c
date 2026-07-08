/*
    Fuzz target: netcode_read_connect_token_private().

    This is the parser for the *decrypted* private connect token — the server-side path that
    reads a client id, a timeout, and a variable-length list of server addresses (each tagged
    IPv4 or IPv6), plus the session keys and user data. It's a separate target from
    fuzz_netcode because the AEAD boundary makes it unreachable by mutation there: any change
    to the encrypted token bytes fails the MAC in netcode_read_packet, so the bytes that reach
    this parser in production are never attacker-mutated through that path. Fuzzing it directly
    exercises the address-list loop and length handling on arbitrary input.

    Includes netcode.c directly (the reader is internal to that TU, same as fuzz_netcode).

    Seed corpus: fuzz/corpus/fuzz_netcode_connect_token (a valid serialized private token).
*/

#include "netcode.c"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int g_init = 0;

int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    if ( !g_init )
    {
        netcode_init();
        g_init = 1;
    }

    // The reader requires a full private-token buffer (NETCODE_CONNECT_TOKEN_PRIVATE_BYTES)
    // and never reads past it; mirror the caller by copying the input into a zero-padded
    // buffer of exactly that size. Larger inputs are ignored, as in the real receive path.
    if ( size > NETCODE_CONNECT_TOKEN_PRIVATE_BYTES )
        return 0;

    uint8_t buffer[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    memset( buffer, 0, sizeof( buffer ) );
    memcpy( buffer, data, size );

    struct netcode_connect_token_private_t token;
    netcode_read_connect_token_private( buffer, (int) sizeof( buffer ), &token );

    return 0;
}

#include "fuzz_standalone.h"
