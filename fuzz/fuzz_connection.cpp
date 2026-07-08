/*
    Fuzz target: yojimbo::Connection::ProcessPacket().

    Drives the yojimbo deserialization path on arbitrary bytes: the bitpacker
    ReadStream, ConnectionPacket / ChannelPacketData serialization, and per-channel
    message + block-fragment reading, over both a reliable-ordered and an
    unreliable-unordered channel. This is the code that turns an authenticated but
    otherwise attacker-controlled packet payload into messages.
*/

#include "yojimbo.h"
#include "shared.h"

using namespace yojimbo;

static bool g_init = false;

static void ensure_init()
{
    if ( !g_init )
    {
        InitializeYojimbo();
        g_init = true;
    }
}

extern "C" int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    ensure_init();

    ConnectionConfig config;
    config.numChannels = 2;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

    if ( (int) size > config.maxPacketSize )
        return 0;

    // The BitReader reads whole 32-bit words, so it may touch up to 3 bytes past
    // packetBytes. The real receive paths always hand it a buffer with slack; mirror
    // that with a padded, zeroed buffer so exact-size reads don't look like overflows.
    static uint8_t buf[ 8 * 1024 + 16 ];
    if ( config.maxPacketSize + 16 > (int) sizeof( buf ) )
        return 0;
    memset( buf, 0, size + 16 );
    memcpy( buf, data, size );

    TestMessageFactory messageFactory( GetDefaultAllocator() );
    Connection connection( GetDefaultAllocator(), messageFactory, config, 100.0 );

    uint16_t sequence = ( size >= 2 ) ? (uint16_t) ( data[0] | ( data[1] << 8 ) ) : 0;
    connection.ProcessPacket( NULL, sequence, buf, (int) size );

    // Drain anything that was successfully received so messages don't accumulate.
    for ( int channelIndex = 0; channelIndex < config.numChannels; ++channelIndex )
    {
        while ( Message * message = connection.ReceiveMessage( channelIndex ) )
            messageFactory.ReleaseMessage( message );
    }

    return 0;
}

#include "fuzz_standalone.h"
