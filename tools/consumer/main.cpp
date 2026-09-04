// Clean-prefix consumer for an installed yojimbo. See tools/consumer/CMakeLists.txt.
//
// Exercises the parts of the install that used to be missing: the library initializes (which
// reaches libsodium), a message factory and a client are constructed and destroyed through the
// public headers, and netcode.h and serialize.h are included the way yojimbo.h expects to find
// them in the prefix. yojimbo.h includes serialize.h itself; netcode.h is included here the way
// a consumer that touches netcode_address_t does.

#include <yojimbo.h>
#include <netcode.h>

#include <stdio.h>
#include <string.h>

using namespace yojimbo;

enum ConsumerMessageType
{
    CONSUMER_MESSAGE,
    NUM_CONSUMER_MESSAGE_TYPES
};

struct ConsumerMessage : public Message
{
    int value;

    ConsumerMessage() : value( 0 ) {}

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, value, 0, 1024 );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

YOJIMBO_MESSAGE_FACTORY_START( ConsumerMessageFactory, NUM_CONSUMER_MESSAGE_TYPES );
YOJIMBO_DECLARE_MESSAGE_TYPE( CONSUMER_MESSAGE, ConsumerMessage );
YOJIMBO_MESSAGE_FACTORY_FINISH();

class ConsumerAdapter : public Adapter
{
public:
    MessageFactory * CreateMessageFactory( Allocator & allocator )
    {
        return YOJIMBO_NEW( allocator, ConsumerMessageFactory, allocator );
    }
};

int main()
{
    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize yojimbo\n" );
        return 1;
    }

    ConsumerAdapter adapter;
    ClientServerConfig config;

    {
        Client client( GetDefaultAllocator(), Address( "0.0.0.0", 0 ), config, adapter, 0.0 );

        uint8_t privateKey[KeyBytes];
        memset( privateKey, 0, sizeof( privateKey ) );

        if ( !client.InsecureConnect( privateKey, 1, Address( "127.0.0.1", 40000 ) ) )
        {
            printf( "error: client failed to start connecting\n" );
            ShutdownYojimbo();
            return 1;
        }

        Message * message = client.CreateMessage( CONSUMER_MESSAGE );
        if ( !message )
        {
            printf( "error: could not create a message\n" );
            ShutdownYojimbo();
            return 1;
        }
        client.ReleaseMessage( message );

        client.Disconnect();
    }

    // netcode's address parser, reached through the installed netcode.h that yojimbo.h includes.
    netcode_address_t address;
    if ( netcode_parse_address( "127.0.0.1:40000", &address ) != NETCODE_OK )
    {
        printf( "error: netcode_parse_address failed\n" );
        ShutdownYojimbo();
        return 1;
    }

    ShutdownYojimbo();

    printf( "consumer linked and ran against the installed yojimbo\n" );
    return 0;
}
