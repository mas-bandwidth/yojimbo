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

// Returns false if the message should be discarded rather than sent (e.g. a block message whose
// block couldn't be allocated - sending a zero-size block would trip a send-time assert; under
// allocation-fault injection that path is reachable).
static bool fill_message( Message * message, int type, Reader & r, ConnectionConfig & config, int channel, Allocator & allocator )
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
            if ( !block )
                return false;   // allocation failed (fault injection) -> don't send a blockless block message
            uint8_t seed = r.u8();
            for ( int i = 0; i < blockSize; ++i )
                block[i] = (uint8_t) ( i + seed );
            m->AttachBlock( allocator, block, blockSize );
            break;
        }
        default: break;
    }
    return true;
}

// Allocator used to fuzz allocation-failure paths. Delegates to malloc/free but can be told to
// fail the Nth upcoming allocation (then it disarms), driven by the fuzz input. Failures are only
// armed *after* construction, since the library allocates its fixed pools up front and treats
// those as infallible; every runtime allocation (packet generate/process, message create, block
// attach) is expected to handle NULL gracefully - that is exactly what this exercises.
class FaultAllocator : public Allocator
{
public:
    FaultAllocator() : m_countdown( -1 ) {}
    void FailIn( int n ) { m_countdown = n; }       // fail the n-th allocation from now, once
    void Disarm() { m_countdown = -1; }

    void * Allocate( size_t size, const char * file, int line )
    {
        if ( m_countdown == 0 )
        {
            m_countdown = -1;                       // one-shot: fail this allocation, then rearm off
            SetErrorLevel( ALLOCATOR_ERROR_OUT_OF_MEMORY );
            return NULL;
        }
        if ( m_countdown > 0 )
            m_countdown--;
        void * p = malloc( size );
        if ( !p )
        {
            SetErrorLevel( ALLOCATOR_ERROR_OUT_OF_MEMORY );
            return NULL;
        }
        TrackAlloc( p, size, file, line );
        return p;
    }

    void Free( void * p, const char * file, int line )
    {
        if ( !p )
            return;
        TrackFree( p, file, line );
        free( p );
    }

private:
    int m_countdown;
};

// Serialize just the connection-packet channel-entry count, matching ConnectionPacket::Serialize.
// (ConnectionPacket itself is internal; the per-entry body is written by the library below.)
template <typename Stream> static bool write_channel_entry_count( Stream & stream, int & numChannelEntries, int numChannels )
{
    serialize_int( stream, numChannelEntries, 0, numChannels );
    return true;
}

// Feed the receiver a *hand-built* reliable-ordered block fragment whose header fields
// (numFragments / fragmentId / fragmentSize) are fuzz-chosen and deliberately out of spec - the
// thing the real write path never emits (e.g. a full-size final fragment). The wire bytes are
// produced by the library's own ChannelPacketData writer, so the format stays in sync; only the
// adversarial field *values* come from the fuzzer. advConfig uses a maxBlockSize that is not a
// multiple of blockFragmentSize, so this targets the block-reassembly bounds check directly. The
// receiver is Reset first so messageId 0 is the expected next block and the fragment reaches the
// reassembly memcpy.
static void inject_adversarial_fragment( Connection & receiver, const ConnectionConfig & advConfig,
                                         FuzzMessageFactory & factory, Reader & r )
{
    const ChannelConfig & cc = advConfig.channel[0];
    const int maxFrags = cc.GetMaxFragmentsPerBlock();

    receiver.Reset();

    ChannelPacketData channelData;
    channelData.Initialize();
    channelData.channelIndex = 0;
    channelData.blockMessage = 1;
    channelData.block.messageId = 0;

    int numFragments = ( maxFrags > 1 ) ? ( 1 + ( r.u8() % maxFrags ) ) : 1;
    channelData.block.numFragments = numFragments;

    int fragmentId = ( numFragments > 1 ) ? ( r.u8() % numFragments ) : 0;
    if ( r.u8() & 1 )
        fragmentId = numFragments - 1;              // bias toward the final fragment (overflow case)
    channelData.block.fragmentId = fragmentId;

    int fragmentSize = 1 + ( r.u16() % cc.blockFragmentSize );
    if ( r.u8() & 1 )
        fragmentSize = cc.blockFragmentSize;        // bias toward a full-size (adversarial) fragment
    channelData.block.fragmentSize = fragmentSize;

    channelData.block.fragmentData = (uint8_t*) YOJIMBO_ALLOCATE( factory.GetAllocator(), fragmentSize );
    if ( !channelData.block.fragmentData )
    {
        channelData.Free( factory );
        return;
    }
    memset( channelData.block.fragmentData, r.u8(), (size_t) fragmentSize );
    channelData.block.messageType = FUZZ_MESSAGE_BLOCK;

    if ( fragmentId == 0 )
    {
        // Fragment 0 carries the block message; the writer serializes it after the fragment data.
        FuzzBlockMessage * bm = (FuzzBlockMessage*) factory.CreateMessage( FUZZ_MESSAGE_BLOCK );
        if ( !bm )
        {
            channelData.Free( factory );
            return;
        }
        bm->sequence = r.u16();
        channelData.block.message = bm;
    }

    // Zero-initialized, with 8 bytes of slack past the write size. The serialize reader loads
    // a 64-bit window and reads up to 7 bytes past the packet data end (masked out of the
    // result, but MemorySanitizer sees the load), so the over-read must stay in-bounds and
    // initialized. Matches the padded buffer in fuzz_connection.
    uint8_t packet[8 * 1024 + 8] = { 0 };
    WriteStream stream( packet, 8 * 1024 );
    int numChannelEntries = 1;
    if ( write_channel_entry_count( stream, numChannelEntries, advConfig.numChannels ) &&
         channelData.SerializeInternal( stream, factory, advConfig.channel, advConfig.numChannels ) )
    {
        stream.Flush();
        int packetBytes = stream.GetBytesProcessed();
        channelData.Free( factory );                // release our write-side fragment data + message
        if ( packetBytes > 0 )
            receiver.ProcessPacket( NULL, 0, packet, packetBytes );
    }
    else
    {
        channelData.Free( factory );
    }

    while ( Message * m = receiver.ReceiveMessage( 0 ) )
        factory.ReleaseMessage( m );
}

extern "C" int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    ensure_init();

    if ( size < 1 )
        return 0;

    Reader r = { data, size, 0 };

    // The fault allocator is declared first so it outlives (and its leak check runs after) the
    // factories and connections built on top of it.
    FaultAllocator allocator;

    ConnectionConfig config;
    fuzz_make_config( r.u8(), config );

    FuzzMessageFactory senderFactory( allocator );
    FuzzMessageFactory receiverFactory( allocator );

    double time = 100.0;
    Connection sender( allocator, senderFactory, config, time );
    Connection receiver( allocator, receiverFactory, config, time );

    // A dedicated receiver + factory for adversarial block fragments, on a config whose
    // maxBlockSize is not a multiple of blockFragmentSize (the overflow-prone geometry).
    ConnectionConfig advConfig;
    advConfig.numChannels = 1;
    advConfig.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    advConfig.channel[0].maxBlockSize = 1100;
    advConfig.channel[0].blockFragmentSize = 500;
    FuzzMessageFactory advFactory( allocator );
    Connection advReceiver( allocator, advFactory, advConfig, time );

    // Zero-initialized with 8 bytes of slack past the generate size, for the reader's 64-bit
    // over-read window (see the note on the block-fragment packet buffer above).
    uint8_t packetData[8 * 1024 + 8] = { 0 };
    uint16_t sequence = 0;
    const int MAX_OPS = 512;

    for ( int op = 0; op < MAX_OPS && !r.done(); ++op )
    {
        uint8_t action = r.u8();

        // Occasionally schedule an allocation failure a few allocations out (one-shot). Armed only
        // here, inside the op loop, so it never disturbs the up-front pool allocations.
        if ( action & 0x20 )
            allocator.FailIn( 1 + ( r.u8() % 12 ) );

        // Occasionally feed the dedicated receiver an adversarial block fragment.
        if ( ( action & 0xC0 ) == 0xC0 )
            inject_adversarial_fragment( advReceiver, advConfig, advFactory, r );

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
                    if ( fill_message( message, type, r, config, channel, senderFactory.GetAllocator() ) )
                        sender.SendMessage( channel, message );
                    else
                        senderFactory.ReleaseMessage( message );
                }
            }
        }
        else
        {
            // tick: generate a packet and deliver or drop it
            int packetBytes = 0;
            if ( sender.GeneratePacket( NULL, sequence, packetData, 8 * 1024, packetBytes ) && packetBytes > 0 )
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
