/*
    Seed-corpus generator for fuzz_connection (the stateful, multi-packet target).

    fuzz_connection feeds a Connection a *sequence* of packets, framed as
    [u16 little-endian length][length bytes] .... These seeds provide valid sequences so the
    fuzzer starts from inputs that reach the reliable-ordered channel's reassembly and
    receive-queue state — most importantly a complete run of block fragments that reassembles.

    The channel layout matches fuzz_connection.cpp: channel 0 reliable-ordered, channel 1
    unreliable-unordered, FuzzMessageFactory. Each captured packet is the real GeneratePacket
    output, verified by feeding it through a receiver Connection.

    Run it (see fuzz/README.md) to (re)generate fuzz/corpus/fuzz_connection; the produced
    files are committed so CI needs only the corpus, not this generator.

    Usage: gen_seed_corpus_connection <corpus-root>   (writes <root>/fuzz_connection)
*/

#include "yojimbo.h"
#include "fuzz_messages.h"
#include "fuzz_config.h"

#include <stdio.h>
#include <string.h>
#include <vector>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

using namespace yojimbo;

static void make_dir( const char * path )
{
    if ( mkdir( path, 0755 ) != 0 && errno != EEXIST )
    {
        fprintf( stderr, "error: cannot create %s: %s\n", path, strerror( errno ) );
        exit( 1 );
    }
}

// A seed is [u8 config selector][ u16-length-prefixed packet ]..., matching fuzz_connection's
// framing. All seeds use config selector 0 (reliable + unreliable, blocks on); the fuzzer
// reaches the other configs by mutating the leading byte.
static const uint8_t FuzzConfigSelector = 0;

struct SeedWriter
{
    std::vector<uint8_t> packets;   // the length-prefixed packet stream (without the selector)
    int npackets = 0;

    void add_packet( const uint8_t * data, int len )
    {
        packets.push_back( (uint8_t) ( len & 0xFF ) );
        packets.push_back( (uint8_t) ( ( len >> 8 ) & 0xFF ) );
        packets.insert( packets.end(), data, data + len );
        npackets++;
    }

    void write( const char * dir, const char * name ) const
    {
        char path[512];
        snprintf( path, sizeof( path ), "%s/%s", dir, name );
        FILE * f = fopen( path, "wb" );
        if ( !f ) { fprintf( stderr, "error: cannot write %s\n", path ); exit( 1 ); }
        fputc( FuzzConfigSelector, f );
        if ( !packets.empty() )
            fwrite( packets.data(), 1, packets.size(), f );
        fclose( f );
        printf( "  %s (%d packets, %zu bytes)\n", path, npackets, packets.size() + 1 );
    }
};

static void config_channels( ConnectionConfig & config )
{
    fuzz_make_config( FuzzConfigSelector, config );
}

// Generate one packet from a freshly-seeded connection, verify it reads back, and add it as a
// single-packet seed. `fill` queues whatever it wants before the packet is generated.
template <typename Fill>
static void emit_single( const char * dir, const char * name, Fill fill )
{
    FuzzMessageFactory mf( GetDefaultAllocator() );
    ConnectionConfig config; config_channels( config );
    Connection connection( GetDefaultAllocator(), mf, config, 100.0 );

    fill( connection, mf );

    uint8_t packetData[8 * 1024];
    int packetBytes = 0;
    if ( !connection.GeneratePacket( NULL, 0, packetData, (int) sizeof( packetData ), packetBytes ) || packetBytes <= 0 )
    {
        fprintf( stderr, "error: no packet generated for seed %s\n", name );
        exit( 1 );
    }

    FuzzMessageFactory rxFactory( GetDefaultAllocator() );
    Connection receiver( GetDefaultAllocator(), rxFactory, config, 100.0 );
    bool ok = receiver.ProcessPacket( NULL, 0, packetData, packetBytes );
    assert( ok && receiver.GetErrorLevel() == CONNECTION_ERROR_NONE && "seed failed to read back" );
    for ( int ch = 0; ch < config.numChannels; ++ch )
        while ( Message * m = receiver.ReceiveMessage( ch ) )
            rxFactory.ReleaseMessage( m );

    SeedWriter seed;
    seed.add_packet( packetData, packetBytes );
    seed.write( dir, name );
}

// Pump a sender -> receiver pair (with simulated acks so the sender advances through block
// fragments) until the sender has nothing left to send, capturing every data packet it emits.
// The captured sequence, replayed against one fresh Connection, reproduces the same
// receive-side state (block reassembly / in-order delivery). Stops as soon as the send queues
// drain, so the seed holds only the meaningful packets (no trailing idle keep-alives).
template <typename Fill>
static void emit_sequence( const char * dir, const char * name, Fill fill )
{
    FuzzMessageFactory senderFactory( GetDefaultAllocator() );
    FuzzMessageFactory receiverFactory( GetDefaultAllocator() );
    ConnectionConfig config; config_channels( config );

    double time = 100.0;
    Connection sender( GetDefaultAllocator(), senderFactory, config, time );
    Connection receiver( GetDefaultAllocator(), receiverFactory, config, time );

    fill( sender, senderFactory );

    SeedWriter seed;
    uint8_t packetData[8 * 1024];
    uint16_t sequence = 0;

    for ( int iter = 0; iter < 4096; ++iter )
    {
        if ( !sender.HasMessagesToSend( 0 ) && !sender.HasMessagesToSend( 1 ) )
            break;

        int packetBytes = 0;
        if ( sender.GeneratePacket( NULL, sequence, packetData, (int) sizeof( packetData ), packetBytes ) && packetBytes > 0 )
        {
            seed.add_packet( packetData, packetBytes );
            receiver.ProcessPacket( NULL, sequence, packetData, packetBytes );
            sender.ProcessAcks( &sequence, 1 );        // simulate the packet being acked
            assert( receiver.GetErrorLevel() == CONNECTION_ERROR_NONE && "sequence seed read back failed" );
        }

        time += 0.1;
        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );
        sequence++;

        for ( int ch = 0; ch < config.numChannels; ++ch )
            while ( Message * m = receiver.ReceiveMessage( ch ) )
                receiverFactory.ReleaseMessage( m );
    }

    seed.write( dir, name );
}

int main( int argc, char ** argv )
{
    const char * root = ( argc > 1 ) ? argv[1] : "fuzz/corpus";

    if ( !InitializeYojimbo() )
    {
        fprintf( stderr, "error: failed to initialize yojimbo\n" );
        return 1;
    }

    make_dir( root );
    char dir[512];
    snprintf( dir, sizeof( dir ), "%s/fuzz_connection", root );
    make_dir( dir );

    printf( "generating connection seeds:\n" );

    // Queue one of each non-block message type, exercising the full serialize_* vocabulary, on
    // the given channel.
    auto queue_variety = []( Connection & c, FuzzMessageFactory & mf, int channel ) {
        FuzzPrimitivesMessage * p = (FuzzPrimitivesMessage*) mf.CreateMessage( FUZZ_MESSAGE_PRIMITIVES );
        p->i = -12345; p->bits = 0x1abcd & ( ( 1u << 17 ) - 1 ); p->flag = true;
        p->f = 3.14159f; p->d = 2.718281828; p->cf = 0.42f; p->base = 500; p->rel = 512;
        c.SendMessage( channel, p );

        FuzzStringMessage * s = (FuzzStringMessage*) mf.CreateMessage( FUZZ_MESSAGE_STRING );
        snprintf( s->str, sizeof( s->str ), "fuzz-seed-string" );
        c.SendMessage( channel, s );

        FuzzBytesMessage * b = (FuzzBytesMessage*) mf.CreateMessage( FUZZ_MESSAGE_BYTES );
        b->count = 200;
        for ( int i = 0; i < b->count; ++i ) b->data[i] = (uint8_t) ( i * 5 + 1 );
        c.SendMessage( channel, b );
    };

    auto attach_block = []( FuzzMessageFactory & mf, int blockSize, int salt ) -> FuzzBlockMessage * {
        FuzzBlockMessage * m = (FuzzBlockMessage*) mf.CreateMessage( FUZZ_MESSAGE_BLOCK );
        m->sequence = 0;
        uint8_t * b = (uint8_t*) YOJIMBO_ALLOCATE( mf.GetAllocator(), blockSize );
        for ( int i = 0; i < blockSize; ++i ) b[i] = (uint8_t) ( i + salt );
        m->AttachBlock( mf.GetAllocator(), b, blockSize );
        return m;
    };

    // variety of message types on the reliable channel
    emit_single( dir, "reliable_variety", [&]( Connection & c, FuzzMessageFactory & mf ) {
        queue_variety( c, mf, 0 );
    } );

    // variety on the unreliable channel
    emit_single( dir, "unreliable_variety", [&]( Connection & c, FuzzMessageFactory & mf ) {
        queue_variety( c, mf, 1 );
    } );

    // both channels populated in one packet
    emit_single( dir, "mixed_channels", [&]( Connection & c, FuzzMessageFactory & mf ) {
        queue_variety( c, mf, 0 );
        queue_variety( c, mf, 1 );
    } );

    // A small (single-fragment) block, fully delivered.
    emit_sequence( dir, "reliable_block", [&]( Connection & c, FuzzMessageFactory & mf ) {
        c.SendMessage( 0, attach_block( mf, 512, 0 ) );
    } );

    // A multi-fragment block (5000 bytes > 1024 fragment size -> 5 fragments), the whole
    // sequence, so replaying it drives the reassembly state machine to completion.
    emit_sequence( dir, "reliable_block_multifragment", [&]( Connection & c, FuzzMessageFactory & mf ) {
        c.SendMessage( 0, attach_block( mf, 5000, 3 ) );
    } );

    // Varied messages plus a block, delivered over many packets: exercises the in-order
    // receive queue alongside reassembly and the varied serialize paths.
    emit_sequence( dir, "reliable_messages_and_block", [&]( Connection & c, FuzzMessageFactory & mf ) {
        for ( int i = 0; i < 4; ++i )
            queue_variety( c, mf, 0 );
        c.SendMessage( 0, attach_block( mf, 3000, 7 ) );
    } );

    ShutdownYojimbo();
    printf( "done\n" );
    return 0;
}
