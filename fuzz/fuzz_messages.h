/*
    Message types for the connection fuzz targets.

    The stock TestMessage only calls serialize_bits, so it exercises almost none of the
    serialize vocabulary. These messages deliberately drive the full set — int / bits / bool /
    float / double / compressed_float / int_relative / align, a bounded string, and a
    variable-length byte array — so fuzzing Connection::ProcessPacket reaches the read paths of
    each serialize helper (and their length/bounds checks, a classic bug surface). One block
    message type covers the reliable-ordered block path.

    Shared by fuzz_connection.cpp, fuzz_connection_structured.cpp, and the seed generator so the
    message-type indices line up between what writes the seeds and what reads them.
*/

#ifndef FUZZ_MESSAGES_H
#define FUZZ_MESSAGES_H

#include "yojimbo.h"
#include <string.h>

using namespace yojimbo;

enum FuzzMessageType
{
    FUZZ_MESSAGE_PRIMITIVES,
    FUZZ_MESSAGE_STRING,
    FUZZ_MESSAGE_BYTES,
    FUZZ_MESSAGE_BLOCK,
    FUZZ_NUM_MESSAGE_TYPES
};

const int FuzzMaxBytes = 512;
const int FuzzMaxString = 64;

// Every fixed-width serialize helper, including an odd bit width and a relative int.
struct FuzzPrimitivesMessage : public Message
{
    int32_t  i;          // serialize_int with an asymmetric range
    uint32_t bits;       // serialize_bits, odd width
    bool     flag;       // serialize_bool
    float    f;          // serialize_float
    double   d;          // serialize_double
    float    cf;         // serialize_compressed_float, [0,1]
    int32_t  base;       // serialize_int
    int32_t  rel;        // serialize_int_relative against base

    FuzzPrimitivesMessage() : i( 0 ), bits( 0 ), flag( false ), f( 0 ), d( 0 ), cf( 0 ), base( 0 ), rel( 0 ) {}

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, i, -100000, 100000 );
        serialize_bits( stream, bits, 17 );
        serialize_bool( stream, flag );
        serialize_float( stream, f );
        serialize_double( stream, d );
        serialize_compressed_float( stream, cf, 0.0f, 1.0f, 0.01f );
        serialize_align( stream );
        serialize_int( stream, base, 0, 1000 );
        serialize_int_relative( stream, base, rel );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

// Bounded string: exercises serialize_string's length read + bounds check.
struct FuzzStringMessage : public Message
{
    char str[FuzzMaxString];

    FuzzStringMessage() { str[0] = '\0'; }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_string( stream, str, sizeof( str ) );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

// Variable-length byte array: a count then that many bytes — the read path where a bad count
// would over-read if unchecked.
struct FuzzBytesMessage : public Message
{
    int     count;
    uint8_t data[FuzzMaxBytes];

    FuzzBytesMessage() : count( 0 ) { memset( data, 0, sizeof( data ) ); }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, count, 0, FuzzMaxBytes );
        serialize_bytes( stream, data, count );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

// Block message for the reliable-ordered block path.
struct FuzzBlockMessage : public BlockMessage
{
    uint16_t sequence;

    FuzzBlockMessage() : sequence( 0 ) {}

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bits( stream, sequence, 16 );
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()
};

YOJIMBO_MESSAGE_FACTORY_START( FuzzMessageFactory, FUZZ_NUM_MESSAGE_TYPES );
    YOJIMBO_DECLARE_MESSAGE_TYPE( FUZZ_MESSAGE_PRIMITIVES, FuzzPrimitivesMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( FUZZ_MESSAGE_STRING, FuzzStringMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( FUZZ_MESSAGE_BYTES, FuzzBytesMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( FUZZ_MESSAGE_BLOCK, FuzzBlockMessage );
YOJIMBO_MESSAGE_FACTORY_FINISH();

#endif // #ifndef FUZZ_MESSAGES_H
