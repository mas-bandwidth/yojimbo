#include "yojimbo_matcher.h"

using namespace yojimbo;

int main( int argc, char * argv[] )
{
    (void)argc;
    (void)argv;

    Matcher matcher;

    if ( !matcher.Initialize() )
    {
        printf( "error: failed to initialize matcher\n" );
        return 1;
    }

    const uint64_t ClientId = 1;

    const uint32_t ProtocolId = 0x12342141;

    matcher.RequestMatch( ProtocolId, ClientId );

    // ...

    return 0;
}
