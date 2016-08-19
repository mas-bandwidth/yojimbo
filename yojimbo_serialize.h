/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef YOJIMBO_SERIALIZE_H
#define YOJIMBO_SERIALIZE_H

#include "yojimbo_config.h"
#include "yojimbo_bitpack.h"
#include "yojimbo_stream.h"
#include "yojimbo_address.h"

namespace yojimbo
{
    #define serialize_int( stream, value, min, max )                    \
        do                                                              \
        {                                                               \
            assert( min < max );                                        \
            int32_t int32_value = 0;                                    \
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
                if ( int64_t(value) < int64_t(min) ||                   \
                     int64_t(value) > int64_t(max) )                    \
                    return false;                                       \
            }                                                           \
        } while (0)

    #define serialize_bits( stream, value, bits )                       \
        do                                                              \
        {                                                               \
            assert( bits > 0 );                                         \
            assert( bits <= 32 );                                       \
            uint32_t uint32_value = 0;                                  \
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
            uint32_t uint32_bool_value = 0;                             \
            if ( Stream::IsWriting )                                    \
                uint32_bool_value = value ? 1 : 0;                      \
            serialize_bits( stream, uint32_bool_value, 1 );             \
            if ( Stream::IsReading )                                    \
                value = uint32_bool_value ? true : false;               \
        } while (0)

    #define serialize_enum( stream, value, type, num_entries )          \
        do                                                              \
        {                                                               \
            uint32_t int_value = 0;                                     \
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
        uint32_t hi = 0, lo = 0;
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

        DoubleInt tmp = { 0 };
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
        int length = 0;
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
            if ( !yojimbo::serialize_string_internal( stream, string, buffer_size ) )       \
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

    template <typename Stream> bool serialize_address_internal( Stream & stream, Address & address )
    {
        char buffer[MaxAddressLength];

        if ( Stream::IsWriting )
        {
            assert( address.IsValid() );
            address.ToString( buffer, sizeof( buffer ) );
        }

        serialize_string( stream, buffer, sizeof( buffer ) );

        if ( Stream::IsReading )
        {
            address = Address( buffer );
            if ( !address.IsValid() )
                return false;
        }

        return true;
    }

    #define serialize_address( stream, value )                                              \
        do                                                                                  \
        {                                                                                   \
            if ( !yojimbo::serialize_address_internal( stream, value ) )                    \
                return false;                                                               \
        } while (0)

    template <typename Stream, typename T> bool serialize_int_relative_internal( Stream & stream, T previous, T & current )
    {
        uint32_t difference = 0;

        if ( Stream::IsWriting )
        {
            assert( previous < current );
            difference = current - previous;
        }

        bool oneBit = false;
        if ( Stream::IsWriting )
            oneBit = difference == 1;
        serialize_bool( stream, oneBit );
        if ( oneBit )
        {
            if ( Stream::IsReading )
                current = previous + 1;
            return true;
        }

        bool twoBits = false;
        if ( Stream::IsWriting )
            twoBits = difference <= 6;
        serialize_bool( stream, twoBits );
        if ( twoBits )
        {
            serialize_int( stream, difference, 2, 6 );
            if ( Stream::IsReading )
                current = previous + difference;
            return true;
        }

        bool fourBits = false;
        if ( Stream::IsWriting )
            fourBits = difference <= 23;
        serialize_bool( stream, fourBits );
        if ( fourBits )
        {
            serialize_int( stream, difference, 7, 23 );
            if ( Stream::IsReading )
                current = previous + difference;
            return true;
        }

        bool eightBits = false;
        if ( Stream::IsWriting )
            eightBits = difference <= 280;
        serialize_bool( stream, eightBits );
        if ( eightBits )
        {
            serialize_int( stream, difference, 24, 280 );
            if ( Stream::IsReading )
                current = previous + difference;
            return true;
        }

        bool twelveBits = false;
        if ( Stream::IsWriting )
            twelveBits = difference <= 4377;
        serialize_bool( stream, twelveBits );
        if ( twelveBits )
        {
            serialize_int( stream, difference, 281, 4377 );
            if ( Stream::IsReading )
                current = previous + difference;
            return true;
        }

        bool sixteenBits = false;
        if ( Stream::IsWriting ) 
            sixteenBits = difference <= 69914;
        serialize_bool( stream, sixteenBits );
        if ( sixteenBits )
        {
            serialize_int( stream, difference, 4378, 69914 );
            if ( Stream::IsReading )
                current = previous + difference;
            return true;
        }

        uint32_t value = current;
        serialize_uint32( stream, value );
        if ( Stream::IsReading )
            current = value;

        return true;
    }

    #define serialize_int_relative( stream, string, buffer_size )                           \
        do                                                                                  \
        {                                                                                   \
            if ( !yojimbo::serialize_int_relative_internal( stream, string, buffer_size ) ) \
                return false;                                                               \
        } while (0)

    template <typename Stream> bool serialize_ack_relative_internal( Stream & stream, uint16_t sequence, uint16_t & ack )
    {
        int ack_delta = 0;
        bool ack_in_range = false;

        if ( Stream::IsWriting )
        {
            if ( ack < sequence )
                ack_delta = sequence - ack;
            else
                ack_delta = (int)sequence + 65536 - ack;

            assert( ack_delta > 0 );
            assert( uint16_t( sequence - ack_delta ) == ack );
            
            ack_in_range = ack_delta <= 64;
        }

        serialize_bool( stream, ack_in_range );

        if ( ack_in_range )
        {
            serialize_int( stream, ack_delta, 1, 64 );
            if ( Stream::IsReading )
                ack = sequence - ack_delta;
        }
        else
        {
            serialize_bits( stream, ack, 16 );
        }

        return true;
    }

    #define serialize_ack_relative( stream, string, buffer_size )                                   \
        do                                                                                          \
        {                                                                                           \
            if ( !yojimbo::serialize_ack_relative_internal( stream, string, buffer_size ) )         \
                return false;                                                                       \
        } while (0)

    template <typename Stream> bool serialize_sequence_relative_internal( Stream & stream, uint16_t messageId1, uint16_t & messageId2 )
    {
        if ( Stream::IsWriting )
        {
            uint32_t a = messageId1;
            uint32_t b = messageId2 + ( ( messageId1 > messageId2 ) ? 65536 : 0 );
            serialize_int_relative( stream, a, b );
        }
        else
        {
            uint32_t a = messageId1;
            uint32_t b = 0;
            serialize_int_relative( stream, a, b );
            if ( b >= 65536 )
                b -= 65536;
            messageId2 = uint16_t( b );
        }

        return true;
    }

    #define serialize_sequence_relative( stream, string, buffer_size )                              \
        do                                                                                          \
        {                                                                                           \
            if ( !yojimbo::serialize_sequence_relative_internal( stream, string, buffer_size ) )    \
                return false;                                                                       \
        } while (0)

    #define read_bits( stream, value, bits )                                                \
    do                                                                                      \
    {                                                                                       \
        assert( bits > 0 );                                                                 \
        assert( bits <= 32 );                                                               \
        uint32_t uint32_value= 0;                                                           \
        if ( !stream.SerializeBits( uint32_value, bits ) )                                  \
            return false;                                                                   \
        value = uint32_value;                                                               \
    } while (0)

    #define read_int( stream, value, min, max )                                             \
        do                                                                                  \
        {                                                                                   \
            assert( min < max );                                                            \
            int32_t int32_value = 0;                                                        \
            if ( !stream.SerializeInteger( int32_value, min, max ) )                        \
                return false;                                                               \
            value = int32_value;                                                            \
            if ( value < min || value > max )                                               \
                return false;                                                               \
        } while (0)

    #define read_bool( stream, value ) read_bits( stream, value, 1 )

    #define read_float                  serialize_float
    #define read_uint32                 serialize_uint32
    #define read_uint64                 serialize_uint64
    #define read_double                 serialize_double
    #define read_bytes                  serialize_bytes
    #define read_string                 serialize_string
    #define read_align                  serialize_align
    #define read_check                  serialize_check
    #define read_object                 serialize_object
    #define read_address                serialize_address
    #define read_int_relative           serialize_int_relative
    #define read_ack_relative           serialize_ack_relative
    #define read_sequence_relative      serialize_sequence_relative

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

    #define write_float                 serialize_float
    #define write_uint32                serialize_uint32
    #define write_uint64                serialize_uint64
    #define write_double                serialize_double
    #define write_bytes                 serialize_bytes
    #define write_string                serialize_string
    #define write_align                 serialize_align
    #define write_check                 serialize_check
    #define write_object                serialize_object
    #define write_address               serialize_address
    #define write_int_relative          serialize_int_relative
    #define write_ack_relative          serialize_ack_relative
    #define write_sequence_relative     serialize_sequence_relative
    
    class Serializable
    {  
    public:

        virtual ~Serializable() {}

        virtual bool SerializeInternal( class ReadStream & stream ) = 0;

        virtual bool SerializeInternal( class WriteStream & stream ) = 0;

        virtual bool SerializeInternal( class MeasureStream & stream ) = 0;
    };

    #define YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS()                                                           \
        bool SerializeInternal( class yojimbo::ReadStream & stream ) { return Serialize( stream ); };           \
        bool SerializeInternal( class yojimbo::WriteStream & stream ) { return Serialize( stream ); };          \
        bool SerializeInternal( class yojimbo::MeasureStream & stream ) { return Serialize( stream ); };         
}

#endif // #ifndef YOJIMBO_SERIALIZE_H
