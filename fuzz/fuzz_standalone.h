/*
    Minimal standalone driver for the fuzz targets.

    libFuzzer isn't always available (notably Apple clang doesn't ship it), so each
    target can also be built with -DFUZZ_STANDALONE to get an ordinary executable:

      - with file arguments, it replays those inputs through the target;
      - otherwise it feeds pseudo-random buffers (set FUZZ_ITERS to change the count).

    Build it with -fsanitize=address,undefined to shake out crashes locally; CI runs
    the same targets under real libFuzzer on Linux. The target defines
    LLVMFuzzerTestOneInput above its #include of this header.
*/

#ifdef FUZZ_STANDALONE

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char ** argv )
{
    if ( argc > 1 )
    {
        int i;
        for ( i = 1; i < argc; ++i )
        {
            FILE * f = fopen( argv[i], "rb" );
            if ( !f )
                continue;
            fseek( f, 0, SEEK_END );
            long n = ftell( f );
            fseek( f, 0, SEEK_SET );
            if ( n < 0 ) { fclose( f ); continue; }
            uint8_t * buf = (uint8_t*) malloc( (size_t) n + 1 );
            size_t got = fread( buf, 1, (size_t) n, f );
            fclose( f );
            LLVMFuzzerTestOneInput( buf, got );
            free( buf );
        }
        return 0;
    }

    unsigned int seed = 0x1234567u;
    long iters = 200000;
    const char * e = getenv( "FUZZ_ITERS" );
    if ( e )
        iters = atol( e );

    long it;
    for ( it = 0; it < iters; ++it )
    {
        uint8_t buf[4096];
        seed = seed * 1103515245u + 12345u;
        size_t n = ( seed >> 8 ) % ( sizeof( buf ) + 1 );
        size_t j;
        for ( j = 0; j < n; ++j )
        {
            seed = seed * 1103515245u + 12345u;
            buf[j] = (uint8_t) ( seed >> 16 );
        }
        LLVMFuzzerTestOneInput( buf, n );
    }
    return 0;
}

#endif // #ifdef FUZZ_STANDALONE
