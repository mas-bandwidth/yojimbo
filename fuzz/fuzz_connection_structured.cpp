/*
    Fuzz target: yojimbo::Connection round trip, driven by a structured script.

    Where fuzz_connection.cpp feeds arbitrary bytes to ProcessPacket (great for malformed
    input, but random bytes rarely form a valid packet), this target interprets the fuzz input
    as a *script* of operations against a sender/receiver Connection pair:

        - send a message (fuzz picks type, channel, and contents) on the sender;
        - "tick": generate a packet from the sender and either deliver it to the receiver (and
          ack it) or drop it (fuzz-controlled loss), then advance time and drain the receiver.

    Every packet is produced by the real write path, so it is always structurally valid and the
    receiver never bails out early — which lets the fuzzer drive the reliable-ordered channel's
    reassembly and in-order delivery state machine (out-of-order arrival via loss + resend)
    under fuzzer-chosen message streams. ASan/UBSan/MSan then watch the receive side.

    Shares the config table (fuzz_config.h) and message factory (fuzz_messages.h) with
    fuzz_connection. Seeds: fuzz/corpus/fuzz_connection_structured.
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
        yojimbo_log_level( YOJIMBO_LOG_LEVEL_ERROR );
        g_init = true;
    }
}

// Minimal cursor over the fuzz input; out-of-data reads return 0.
struct Reader
{
    const uint8_t * data;
    size_t size;
    size_t pos;

    uint8_t u8() { return pos < size ? data[pos++] : 0; }
    uint16_t u16() { uint16_t a = u8(); uint16_t b = u8(); return (uint16_t) ( a | ( b << 8 ) ); }
    bool done() const { return pos >= size; }
};

static void fill_message( Message * message, int type, Reader & r, ConnectionConfig & config, int channel, Allocator & allocator )
{
    switch ( type )
    {
        case FUZZ_MESSAGE_PRIMITIVES:
        {
            FuzzPrimitivesMessage * m = (FuzzPrimitivesMessage*) message;
            m->i = (int32_t) ( r.u16() ) - 32768;
            m->bits = r.u16();
            m->flag = ( r.u8() & 1 ) != 0;
            m->f = (float) ( (int) r.u16() - 32768 );
            m->d = (double) ( (int) r.u16() - 32768 );
            m->cf = ( r.u8() % 101 ) / 100.0f;   // [0,1]
            m->base = r.u16() % 900;
            m->rel = m->base + 1 + ( r.u8() % 100 );   // serialize_int_relative needs base < rel
            break;
        }
        case FUZZ_MESSAGE_STRING:
        {
            FuzzStringMessage * m = (FuzzStringMessage*) message;
            int n = r.u8() % FuzzMaxString;       // leave room for the terminator
            for ( int i = 0; i < n; ++i )
            {
                uint8_t c = r.u8();
                m->str[i] = (char) ( c ? c : ' ' );   // no embedded NUL
            }
            m->str[n] = '\0';
            break;
        }
        case FUZZ_MESSAGE_BYTES:
        {
            FuzzBytesMessage * m = (FuzzBytesMessage*) message;
            m->count = r.u16() % ( FuzzMaxBytes + 1 );
            for ( int i = 0; i < m->count; ++i )
                m->data[i] = r.u8();
            break;
        }
        case FUZZ_MESSAGE_BLOCK:
        {
            FuzzBlockMessage * m = (FuzzBlockMessage*) message;
            m->sequence = r.u16();
            int maxBlock = config.channel[channel].maxBlockSize;
            int blockSize = 1 + ( r.u16() % ( maxBlock < 8192 ? maxBlock : 8192 ) );
            uint8_t * block = (uint8_t*) YOJIMBO_ALLOCATE( allocator, blockSize );
            if ( block )
            {
                uint8_t seed = r.u8();
                for ( int i = 0; i < blockSize; ++i )
                    block[i] = (uint8_t) ( i + seed );
                m->AttachBlock( allocator, block, blockSize );
            }
            break;
        }
        default: break;
    }
}

extern "C" int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    ensure_init();

    if ( size < 1 )
        return 0;

    Reader r = { data, size, 0 };

    ConnectionConfig config;
    fuzz_make_config( r.u8(), config );

    FuzzMessageFactory senderFactory( GetDefaultAllocator() );
    FuzzMessageFactory receiverFactory( GetDefaultAllocator() );

    double time = 100.0;
    Connection sender( GetDefaultAllocator(), senderFactory, config, time );
    Connection receiver( GetDefaultAllocator(), receiverFactory, config, time );

    uint8_t packetData[8 * 1024];
    uint16_t sequence = 0;
    const int MAX_OPS = 512;

    for ( int op = 0; op < MAX_OPS && !r.done(); ++op )
    {
        uint8_t action = r.u8();

        if ( ( action & 3 ) != 0 )
        {
            // send a message
            int type = r.u8() % FUZZ_NUM_MESSAGE_TYPES;
            int channel = ( config.numChannels > 1 ) ? ( r.u8() % config.numChannels ) : 0;

            // Block messages are only valid on a reliable-ordered channel with blocks enabled:
            // the unreliable channel serializes blocks inline and asserts if one won't fit a
            // single packet, and a disableBlocks channel asserts outright. Fall back otherwise.
            if ( type == FUZZ_MESSAGE_BLOCK &&
                 ( config.channel[channel].type != CHANNEL_TYPE_RELIABLE_ORDERED ||
                   config.channel[channel].disableBlocks ) )
            {
                type = FUZZ_MESSAGE_PRIMITIVES;
            }

            if ( sender.CanSendMessage( channel ) )
            {
                Message * message = senderFactory.CreateMessage( type );
                if ( message )
                {
                    fill_message( message, type, r, config, channel, senderFactory.GetAllocator() );
                    sender.SendMessage( channel, message );
                }
            }
        }
        else
        {
            // tick: generate a packet and deliver or drop it
            int packetBytes = 0;
            if ( sender.GeneratePacket( NULL, sequence, packetData, (int) sizeof( packetData ), packetBytes ) && packetBytes > 0 )
            {
                bool drop = ( action & 4 ) != 0;   // fuzz-controlled packet loss
                if ( !drop )
                {
                    receiver.ProcessPacket( NULL, sequence, packetData, packetBytes );
                    sender.ProcessAcks( &sequence, 1 );
                }
            }

            time += 0.1;
            sender.AdvanceTime( time );
            receiver.AdvanceTime( time );
            sequence++;

            for ( int ch = 0; ch < config.numChannels; ++ch )
                while ( Message * m = receiver.ReceiveMessage( ch ) )
                    receiverFactory.ReleaseMessage( m );
        }
    }

    // final drain
    for ( int ch = 0; ch < config.numChannels; ++ch )
        while ( Message * m = receiver.ReceiveMessage( ch ) )
            receiverFactory.ReleaseMessage( m );

    return 0;
}

#include "fuzz_standalone.h"
