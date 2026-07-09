/*
    Connection configurations for the connection fuzz targets.

    The first byte of a fuzz input selects one of a fixed set of valid ConnectionConfigs, so
    the fuzzer can explore different channel layouts (reliable/unreliable, ordering,
    blocks-disabled, small fragments) that reach different validation paths in the
    deserializer. The harness and the seed generator share this table so a seed's packets are
    read under the config they were written for.
*/

#ifndef FUZZ_CONFIG_H
#define FUZZ_CONFIG_H

#include "yojimbo.h"

using namespace yojimbo;

const int FuzzNumConfigs = 8;

inline void fuzz_make_config( uint8_t selector, ConnectionConfig & config )
{
    // New configs are appended (never inserted) so an existing corpus input's leading selector
    // byte keeps mapping to the same config: data[0] % 8 == data[0] % 6 for data[0] in [0,5].
    switch ( selector % FuzzNumConfigs )
    {
        case 0: // reliable + unreliable, blocks enabled (the common case)
            config.numChannels = 2;
            config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
            break;

        case 1: // reliable + unreliable, reliable channel with blocks disabled
            config.numChannels = 2;
            config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            config.channel[0].disableBlocks = true;
            config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
            break;

        case 2: // single reliable-ordered channel
            config.numChannels = 1;
            config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            break;

        case 3: // single unreliable-unordered channel
            config.numChannels = 1;
            config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
            break;

        case 4: // swapped ordering: unreliable first, reliable second
            config.numChannels = 2;
            config.channel[0].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
            config.channel[1].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            break;

        case 5: // small block fragments -> many fragments per block
            config.numChannels = 2;
            config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            config.channel[0].blockFragmentSize = 256;
            config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
            break;

        case 6: // maxBlockSize NOT a multiple of blockFragmentSize (1100/500 => ceil 3 fragments).
                // The final fragment's slot starts at 2*500=1000 in an 1100-byte buffer, so an
                // over-long (full-size) final fragment would run past the end - this is the
                // geometry the block-reassembly bounds check has to get right.
            config.numChannels = 1;
            config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            config.channel[0].maxBlockSize = 1100;
            config.channel[0].blockFragmentSize = 500;
            break;

        case 7: // another non-multiple geometry (10000/1024 => ceil 10 fragments), plus an
                // unreliable channel alongside.
            config.numChannels = 2;
            config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
            config.channel[0].maxBlockSize = 10000;
            config.channel[0].blockFragmentSize = 1024;
            config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
            break;
    }
}

#endif // #ifndef FUZZ_CONFIG_H
