/*
    Shared parameters for the netcode fuzz target and its seed generator.

    The generator (tools/gen_seed_corpus) must write packets with the same protocol id
    and keys the harness reads with, or the seeds won't decrypt and would be no better
    than random bytes. Keeping the constants here means the two can't drift apart.

    The harness reads with all-zero packet and private keys, so the generator writes with
    the same zero key.
*/

#ifndef FUZZ_NETCODE_PARAMS_H
#define FUZZ_NETCODE_PARAMS_H

#include <stdint.h>

#define FUZZ_PROTOCOL_ID     0x1122334455667788ULL
#define FUZZ_TIMESTAMP       ( (uint64_t) 1000000 )

#endif // #ifndef FUZZ_NETCODE_PARAMS_H
