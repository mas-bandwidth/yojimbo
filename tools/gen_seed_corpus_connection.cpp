/*
    Seed-corpus generator for fuzz_connection.

    Produces valid yojimbo::Connection packets (the already-decrypted payload that
    Connection::ProcessPacket parses) so libFuzzer starts from inputs that exercise the
    channel / message / block-fragment deserialization instead of random bytes. The channel
    layout matches fuzz_connection.cpp: channel 0 reliable-ordered, channel 1
    unreliable-unordered, TestMessageFactory.

    Run it (see fuzz/README.md) to (re)generate fuzz/corpus/fuzz_connection; the produced
    files are committed so CI needs only the corpus, not this generator.

    Usage: gen_seed_corpus_connection <corpus-root>   (writes <root>/fuzz_connection)
*/

#include "yojimbo.h"
#include "shared.h"

#include <stdio.h>
#include <string.h>
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

static void write_seed( const char * dir, const char * name, const uint8_t * data, int bytes )
{
    char path[512];
    snprintf( path, sizeof( path ), "%s/%s", dir, name );
    FILE * f = fopen( path, "wb" );
    if ( !f )
    {
        fprintf( stderr, "error: cannot write %s: %s\n", path, strerror( errno ) );
        exit( 1 );
    }
    if ( bytes > 0 )
        fwrite( data, 1, (size_t) bytes, f );
    fclose( f );
    printf( "  %s (%d bytes)\n", path, bytes );
}

static void config_channels( ConnectionConfig & config )
{
    config.numChannels = 2;
    config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
}

// Generate one packet from a freshly-seeded connection and write it as a seed. The caller's
// `fill` lambda queues whatever messages/blocks it wants before the packet is generated.
template <typename Fill>
static void emit( const char * dir, const char * name, Fill fill )
{
    TestMessageFactory messageFactory( GetDefaultAllocator() );

    ConnectionConfig config;
    config_channels( config );

    Connection connection( GetDefaultAllocator(), messageFactory, config, 100.0 );

    fill( connection, messageFactory );

    uint8_t packetData[4096];
    int packetBytes = 0;
    if ( !connection.GeneratePacket( NULL, 0, packetData, (int) sizeof( packetData ), packetBytes ) || packetBytes <= 0 )
    {
        fprintf( stderr, "error: no packet generated for seed %s\n", name );
        exit( 1 );
    }

    // Read the packet back through a fresh receiver exactly as fuzz_connection.c does, to
    // prove the seed parses cleanly (reaches channel/message/block deserialization).
    TestMessageFactory rxFactory( GetDefaultAllocator() );
    Connection receiver( GetDefaultAllocator(), rxFactory, config, 100.0 );
    bool ok = receiver.ProcessPacket( NULL, 0, packetData, packetBytes );
    assert( ok && receiver.GetErrorLevel() == CONNECTION_ERROR_NONE && "generated connection seed failed to read back" );
    for ( int ch = 0; ch < config.numChannels; ++ch )
        while ( Message * m = receiver.ReceiveMessage( ch ) )
            rxFactory.ReleaseMessage( m );

    write_seed( dir, name, packetData, packetBytes );
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

    // reliable-ordered messages on channel 0
    emit( dir, "reliable_messages", []( Connection & c, TestMessageFactory & mf ) {
        for ( int i = 0; i < 8; ++i )
        {
            TestMessage * m = (TestMessage*) mf.CreateMessage( TEST_MESSAGE );
            m->sequence = (uint16_t) i;
            c.SendMessage( 0, m );
        }
    } );

    // unreliable-unordered messages on channel 1
    emit( dir, "unreliable_messages", []( Connection & c, TestMessageFactory & mf ) {
        for ( int i = 0; i < 8; ++i )
        {
            TestMessage * m = (TestMessage*) mf.CreateMessage( TEST_MESSAGE );
            m->sequence = (uint16_t) i;
            c.SendMessage( 1, m );
        }
    } );

    // a block message on channel 0 -> a top-level block fragment
    emit( dir, "reliable_block", []( Connection & c, TestMessageFactory & mf ) {
        TestBlockMessage * m = (TestBlockMessage*) mf.CreateMessage( TEST_BLOCK_MESSAGE );
        m->sequence = 0;
        const int blockSize = 512;
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( mf.GetAllocator(), blockSize );
        for ( int i = 0; i < blockSize; ++i )
            blockData[i] = (uint8_t) i;
        m->AttachBlock( mf.GetAllocator(), blockData, blockSize );
        c.SendMessage( 0, m );
    } );

    // a block larger than the fragment size -> a fragment of a multi-fragment block
    // (numFragments > 1 path in SerializeBlockFragment, distinct from the single-fragment seed)
    emit( dir, "reliable_block_multifragment", []( Connection & c, TestMessageFactory & mf ) {
        TestBlockMessage * m = (TestBlockMessage*) mf.CreateMessage( TEST_BLOCK_MESSAGE );
        m->sequence = 0;
        const int blockSize = 3000;  // > default blockFragmentSize (1024) -> 3 fragments
        uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( mf.GetAllocator(), blockSize );
        for ( int i = 0; i < blockSize; ++i )
            blockData[i] = (uint8_t) ( i * 3 );
        m->AttachBlock( mf.GetAllocator(), blockData, blockSize );
        c.SendMessage( 0, m );
    } );

    // both channels populated in a single packet
    emit( dir, "mixed_channels", []( Connection & c, TestMessageFactory & mf ) {
        for ( int i = 0; i < 4; ++i )
        {
            TestMessage * a = (TestMessage*) mf.CreateMessage( TEST_MESSAGE );
            a->sequence = (uint16_t) i;
            c.SendMessage( 0, a );
            TestMessage * b = (TestMessage*) mf.CreateMessage( TEST_MESSAGE );
            b->sequence = (uint16_t) ( 100 + i );
            c.SendMessage( 1, b );
        }
    } );

    ShutdownYojimbo();
    printf( "done\n" );
    return 0;
}
