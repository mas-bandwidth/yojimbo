/*
    Fuzz target: yojimbo::Connection::ProcessPacket(), stateful / multi-packet.

    Drives the yojimbo deserialization path on arbitrary bytes: the bitpacker ReadStream,
    ConnectionPacket / ChannelPacketData serialization, and per-channel message +
    block-fragment reading, over a reliable-ordered and an unreliable-unordered channel.
    This is the code that turns an authenticated but otherwise attacker-controlled packet
    payload into messages.

    One fuzz input is a *sequence* of packets fed to a single long-lived Connection, so the
    reliable-ordered channel's multi-packet state — block-fragment reassembly and the
    out-of-order message receive queue — is reachable (a single packet on a fresh connection
    can never complete a multi-fragment block). Input framing:

        [u8 config selector][u16 little-endian length][length bytes of packet] ...

    The first byte selects the ConnectionConfig (fuzz_config.h); then length-prefixed packets
    repeat until the input is consumed. A length longer than what remains uses the rest. The
    messages use a factory (fuzz_messages.h) that drives the whole serialize_* vocabulary, not
    just serialize_bits.

    Note: a malformed packet drops the Connection into an error state, after which it rejects
    everything — so reaching deep reassembly state requires several *valid* packets in a row.
    The committed seed corpus carries such sequences (see tools/gen_seed_corpus_connection.cpp);
    the structure-aware target fuzz_connection_structured.cpp reaches the same state from a
    higher-level script.
*/

#include "yojimbo.h"
#include "fuzz_messages.h"
#include "fuzz_config.h"

#include <stdint.h>
#include <string.h>

using namespace yojimbo;

static bool g_init = false;

static void ensure_init()
{
    if ( !g_init )
    {
        InitializeYojimbo();
        // The message-leak check in ~MessageFactory reports at ERROR level before
        // calling exit(1); the default log level (NONE) would swallow the report.
        yojimbo_log_level( YOJIMBO_LOG_LEVEL_ERROR );
        g_init = true;
    }
}

extern "C" int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    ensure_init();

    if ( size < 1 )
        return 0;

    // first byte selects the connection config
    ConnectionConfig config;
    fuzz_make_config( data[0], config );
    size_t pos = 1;

    FuzzMessageFactory messageFactory( GetDefaultAllocator() );
    Connection connection( GetDefaultAllocator(), messageFactory, config, 100.0 );

    // The BitReader reads whole 32-bit words, so it may touch up to 3 bytes past the packet
    // end. Real receive paths always have slack; mirror that with a padded, zeroed buffer.
    static uint8_t buf[ 8 * 1024 + 16 ];
    if ( config.maxPacketSize + 16 > (int) sizeof( buf ) )
        return 0;

    const int maxPacket = config.maxPacketSize;
    const int MAX_PACKETS = 64;

    double time = 100.0;
    uint16_t sequence = 0;

    for ( int p = 0; p < MAX_PACKETS && pos < size; ++p )
    {
        // read a 2-byte length prefix (missing bytes read as zero)
        size_t len = data[pos];
        pos++;
        if ( pos < size )
        {
            len |= (size_t) data[pos] << 8;
            pos++;
        }

        size_t avail = size - pos;
        if ( len > avail )
            len = avail;                 // last packet takes the remainder

        int packetBytes = (int) ( len > (size_t) maxPacket ? (size_t) maxPacket : len );

        if ( packetBytes > 0 )
        {
            memset( buf, 0, (size_t) packetBytes + 16 );
            memcpy( buf, data + pos, (size_t) packetBytes );
            connection.ProcessPacket( NULL, sequence, buf, packetBytes );
        }

        pos += len;
        sequence++;

        time += 0.1;
        connection.AdvanceTime( time );

        // drain anything delivered so messages don't accumulate across packets
        for ( int channelIndex = 0; channelIndex < config.numChannels; ++channelIndex )
        {
            while ( Message * message = connection.ReceiveMessage( channelIndex ) )
                messageFactory.ReleaseMessage( message );
        }
    }

    return 0;
}

#include "fuzz_standalone.h"
