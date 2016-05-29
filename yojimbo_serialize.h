/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#ifndef YOJIMBO_SERIALIZE_H
#define YOJIMBO_SERIALIZE_H

#include "yojimbo_config.h"
#include "yojimbo_bitpack.h"
#include "yojimbo_stream.h"

namespace yojimbo
{
    #define serialize_int( stream, value, min, max )                    \
        do                                                              \
        {                                                               \
            assert( min < max );                                        \
            int32_t int32_value;                                        \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                assert( int64_t(value) >= int64_t(min) );               \
                assert( int64_t(value) <= int64_t(max) );               \
                int32_value = (int32_t) value;                          \
            }                                                           \
            if ( !stream.SerializeInteger( int32_value, min, max ) )    \
                return false;                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = int32_value;                                    \
                if ( value < min || value > max )                       \
                    return false;                                       \
            }                                                           \
        } while (0)

    #define serialize_bits( stream, value, bits )                       \
        do                                                              \
        {                                                               \
            assert( bits > 0 );                                         \
            assert( bits <= 32 );                                       \
            uint32_t uint32_value;                                      \
            if ( Stream::IsWriting )                                    \
                uint32_value = (uint32_t) value;                        \
            if ( !stream.SerializeBits( uint32_value, bits ) )          \
                return false;                                           \
            if ( Stream::IsReading )                                    \
                value = uint32_value;                                   \
        } while (0)

    #define serialize_bool( stream, value )                             \
        do                                                              \
        {                                                               \
            uint32_t uint32_bool_value;                                 \
            if ( Stream::IsWriting )                                    \
                uint32_bool_value = value ? 1 : 0;                      \
            serialize_bits( stream, uint32_bool_value, 1 );             \
            if ( Stream::IsReading )                                    \
                value = uint32_bool_value ? true : false;               \
        } while (0)

    #define serialize_enum( stream, value, type, num_entries )          \
        do                                                              \
        {                                                               \
            uint32_t int_value;                                         \
            if ( Stream::IsWriting )                                    \
                int_value = value;                                      \
            serialize_int( stream, int_value, 0, num_entries - 1 );     \
            if ( Stream::IsReading )                                    \
                value = (type) value;                                   \
        } while (0) 

    template <typename Stream> bool serialize_float_internal( Stream & stream, float & value )
    {
        uint32_t int_value;

        if ( Stream::IsWriting )
            memcpy( &int_value, &value, 4 );

        bool result = stream.SerializeBits( int_value, 32 );

        if ( Stream::IsReading )
            memcpy( &value, &int_value, 4 );

        return result;
    }

    #define serialize_float( stream, value )                                        \
        do                                                                          \
        {                                                                           \
            if ( !yojimbo::serialize_float_internal( stream, value ) )              \
                return false;                                                       \
        } while (0)

    #define serialize_uint32( stream, value )                                       \
        serialize_bits( stream, value, 32 );

    template <typename Stream> bool serialize_uint64_internal( Stream & stream, uint64_t & value )
    {
        uint32_t hi,lo;
        if ( Stream::IsWriting )
        {
            lo = value & 0xFFFFFFFF;
            hi = value >> 32;
        }
        serialize_bits( stream, lo, 32 );
        serialize_bits( stream, hi, 32 );
        if ( Stream::IsReading )
            value = ( uint64_t(hi) << 32 ) | lo;
        return true;
    }

    #define serialize_uint64( stream, value )                                       \
        do                                                                          \
        {                                                                           \
            if ( !yojimbo::serialize_uint64_internal( stream, value ) )             \
                return false;                                                       \
        } while (0)

    template <typename Stream> bool serialize_double_internal( Stream & stream, double & value )
    {
        union DoubleInt
        {
            double double_value;
            uint64_t int_value;
        };

        DoubleInt tmp;
        if ( Stream::IsWriting )
            tmp.double_value = value;

        serialize_uint64( stream, tmp.int_value );

        if ( Stream::IsReading )
            value = tmp.double_value;

        return true;
    }

    #define serialize_double( stream, value )                                       \
        do                                                                          \
        {                                                                           \
            if ( !yojimbo::serialize_double_internal( stream, value ) )             \
                return false;                                                       \
        } while (0)

    template <typename Stream> bool serialize_bytes_internal( Stream & stream, uint8_t* data, int bytes )
    {
        return stream.SerializeBytes( data, bytes );
    }

    #define serialize_bytes( stream, data, bytes )                                  \
        do                                                                          \
        {                                                                           \
            if ( !yojimbo::serialize_bytes_internal( stream, data, bytes ) )        \
                return false;                                                       \
        } while (0)

    template <typename Stream> bool serialize_string_internal( Stream & stream, char* string, int buffer_size )
    {
        int length;
        if ( Stream::IsWriting )
        {
            length = (int) strlen( string );
            assert( length < buffer_size - 1 );
        }
        serialize_int( stream, length, 0, buffer_size - 1 );
        serialize_bytes( stream, (uint8_t*)string, length );
        if ( Stream::IsReading )
            string[length] = '\0';
        return true;
    }

    #define serialize_string( stream, string, buffer_size )                                 \
        do                                                                                  \
        {                                                                                   \
            if ( !yojimbo::serialize_string_internal( stream, string, buffer_size ) )     \
                return false;                                                               \
        } while (0)

    #define serialize_align( stream )                                                       \
        do                                                                                  \
        {                                                                                   \
            if ( !stream.SerializeAlign() )                                                 \
                return false;                                                               \
        } while (0)

    #define serialize_check( stream, string )                                               \
        do                                                                                  \
        {                                                                                   \
            if ( !stream.SerializeCheck( string ) )                                         \
                return false;                                                               \
        } while (0)

    #define serialize_object( stream, object )                                              \
        do                                                                                  \
        {                                                                                   \
            if ( !object.Serialize( stream ) )                                              \
                return false;                                                               \
        }                                                                                   \
        while(0)

    #define read_bits( stream, value, bits )                                                \
    do                                                                                      \
    {                                                                                       \
        assert( bits > 0 );                                                                 \
        assert( bits <= 32 );                                                               \
        uint32_t uint32_value;                                                              \
        if ( !stream.SerializeBits( uint32_value, bits ) )                                  \
            return false;                                                                   \
        value = uint32_value;                                                               \
    } while (0)

    #define read_int( stream, value, min, max )                                             \
        do                                                                                  \
        {                                                                                   \
            assert( min < max );                                                            \
            int32_t int32_value;                                                            \
            if ( !stream.SerializeInteger( int32_value, min, max ) )                        \
                return false;                                                               \
            value = int32_value;                                                            \
            if ( value < min || value > max )                                               \
                return false;                                                               \
        } while (0)

    #define read_bool( stream, value ) read_bits( stream, value, 1 )

    #define read_float     serialize_float
    #define read_uint32    serialize_uint32
    #define read_uint64    serialize_uint64
    #define read_double    serialize_double
    #define read_bytes     serialize_bytes
    #define read_string    serialize_string
    #define read_align     serialize_align
    #define read_check     serialize_check
    #define read_object    serialize_object

    #define write_bits( stream, value, bits )                                               \
        do                                                                                  \
        {                                                                                   \
            assert( bits > 0 );                                                             \
            assert( bits <= 32 );                                                           \
            uint32_t uint32_value = (uint32_t) value;                                       \
            if ( !stream.SerializeBits( uint32_value, bits ) )                              \
                return false;                                                               \
        } while (0)

    #define write_int( stream, value, min, max )                                            \
        do                                                                                  \
        {                                                                                   \
            assert( min < max );                                                            \
            assert( value >= min );                                                         \
            assert( value <= max );                                                         \
            int32_t int32_value = (int32_t) value;                                          \
            if ( !stream.SerializeInteger( int32_value, min, max ) )                        \
                return false;                                                               \
        } while (0)

    #define write_float    serialize_float
    #define write_uint32   serialize_uint32
    #define write_uint64   serialize_uint64
    #define write_double   serialize_double
    #define write_bytes    serialize_bytes
    #define write_string   serialize_string
    #define write_align    serialize_align
    #define write_check    serialize_check
    #define write_object   serialize_object

    class Serializable
    {  
    public:

        virtual ~Serializable() {}

        virtual bool SerializeRead( class ReadStream & stream ) = 0;

        virtual bool SerializeWrite( class WriteStream & stream ) = 0;

        virtual bool SerializeMeasure( class MeasureStream & stream ) = 0;
    };

    #define YOJIMBO_SERIALIZE_FUNCTIONS()                                                                       \
        bool SerializeRead( class yojimbo::ReadStream & stream ) { return Serialize( stream ); };               \
        bool SerializeWrite( class yojimbo::WriteStream & stream ) { return Serialize( stream ); };             \
        bool SerializeMeasure( class yojimbo::MeasureStream & stream ) { return Serialize( stream ); };         
}

#endif // #ifndef YOJIMBO_SERIALIZE_H
