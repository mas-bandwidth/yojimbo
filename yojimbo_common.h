/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.

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

#ifndef YOJIMBO_COMMON_H
#define YOJIMBO_COMMON_H

#include "yojimbo_config.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
    
namespace yojimbo
{
    inline int random_int( int a, int b )
    {
        assert( a < b );
        int result = a + rand() % ( b - a + 1 );
        assert( result >= a );
        assert( result <= b );
        return result;
    }

    inline float random_float( float a, float b )
    {
        assert( a < b );
        float random = ( (float) rand() ) / (float) RAND_MAX;
        float diff = b - a;
        float r = random * diff;
        return a + r;
    }

    #ifndef min
    template <typename T> const T & min( const T & a, const T & b )
    {
        return ( a < b ) ? a : b;
    }
    #endif

    #ifndef max
    template <typename T> const T & max( const T & a, const T & b )
    {
        return ( a > b ) ? a : b;
    }
    #endif

    template <typename T> T clamp( const T & value, const T & min, const T & max )
    {
        if ( value < min )
            return min;
        else if ( value > max )
            return max;
        else
            return value;
    }

    template <typename T> void swap( T & a, T & b )
    {
        T tmp = a;
        a = b;
        b = tmp;
    };

    template <typename T> T abs( const T & value )
    {
        return ( value < 0 ) ? -value : value;
    }

    template <uint32_t x> struct PopCount
    {
        enum {   a = x - ( ( x >> 1 )       & 0x55555555 ),
                 b =   ( ( ( a >> 2 )       & 0x33333333 ) + ( a & 0x33333333 ) ),
                 c =   ( ( ( b >> 4 ) + b ) & 0x0f0f0f0f ),
                 d =   c + ( c >> 8 ),
                 e =   d + ( d >> 16 ),

            result = e & 0x0000003f 
        };
    };

    template <uint32_t x> struct Log2
    {
        enum {   a = x | ( x >> 1 ),
                 b = a | ( a >> 2 ),
                 c = b | ( b >> 4 ),
                 d = c | ( c >> 8 ),
                 e = d | ( d >> 16 ),
                 f = e >> 1,

            result = PopCount<f>::result
        };
    };

    template <int64_t min, int64_t max> struct BitsRequired
    {
        static const uint32_t result = ( min == max ) ? 0 : ( Log2<uint32_t(max-min)>::result + 1 );
    };

    #define BITS_REQUIRED( min, max ) BitsRequired<min,max>::result

    inline uint32_t popcount( uint32_t x )
    {
#ifdef __GNUC__
        return __builtin_popcount( x );
#else // #ifdef __GNUC__
        const uint32_t a = x - ( ( x >> 1 )       & 0x55555555 );
        const uint32_t b =   ( ( ( a >> 2 )       & 0x33333333 ) + ( a & 0x33333333 ) );
        const uint32_t c =   ( ( ( b >> 4 ) + b ) & 0x0f0f0f0f );
        const uint32_t d =   c + ( c >> 8 );
        const uint32_t e =   d + ( d >> 16 );
        const uint32_t result = e & 0x0000003f;
        return result;
#endif // #ifdef __GNUC__
    }

#ifdef __GNUC__

    inline int bits_required( uint32_t min, uint32_t max )
    {
        return 32 - __builtin_clz( max - min );
    }

#else // #ifdef __GNUC__

    inline uint32_t log2( uint32_t x )
    {
        const uint32_t a = x | ( x >> 1 );
        const uint32_t b = a | ( a >> 2 );
        const uint32_t c = b | ( b >> 4 );
        const uint32_t d = c | ( c >> 8 );
        const uint32_t e = d | ( d >> 16 );
        const uint32_t f = e >> 1;
        return popcount( f );
    }

    inline int bits_required( uint32_t min, uint32_t max )
    {
        return ( min == max ) ? 0 : log2( max - min ) + 1;
    }

#endif // #ifdef __GNUC__

    inline uint32_t bswap( uint32_t value )
    {
#ifdef __GNUC__
        return __builtin_bswap32( value );
#else // #ifdef __GNUC__
        return ( value & 0x000000ff ) << 24 | ( value & 0x0000ff00 ) << 8 | ( value & 0x00ff0000 ) >> 8 | ( value & 0xff000000 ) >> 24;
#endif // #ifdef __GNUC__
    }

    inline uint16_t bswap( uint16_t value )
    {
        return ( value & 0x00ff ) << 8 | ( value & 0xff00 ) >> 8;
    }

    // IMPORTANT: These functions consider network order to be little endian because most modern processors are little endian. Least amount of work!

    inline uint32_t host_to_network( uint32_t value )
    {
#if YOJIMBO_BIG_ENDIAN
        return bswap( value );
#else // #if YOJIMBO_BIG_ENDIAN
        return value;
#endif // #if YOJIMBO_BIG_ENDIAN
    }

    inline uint32_t network_to_host( uint32_t value )
    {
#if YOJIMBO_BIG_ENDIAN
        return bswap( value );
#else // #if YOJIMBO_BIG_ENDIAN
        return value;
#endif // #if YOJIMBO_BIG_ENDIAN
    }

    inline uint16_t host_to_network( uint16_t value )
    {
#if YOJIMBO_BIG_ENDIAN
        return bswap( value );
#else // #if YOJIMBO_BIG_ENDIAN
        return value;
#endif // #if YOJIMBO_BIG_ENDIAN
    }

    inline uint16_t network_to_host( uint16_t value )
    {
#if YOJIMBO_BIG_ENDIAN
        return bswap( value );
#else // #if YOJIMBO_BIG_ENDIAN
        return value;
#endif // #if YOJIMBO_BIG_ENDIAN
    }

    inline bool sequence_greater_than( uint16_t s1, uint16_t s2 )
    {
        return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
               ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
    }

    inline bool sequence_less_than( uint16_t s1, uint16_t s2 )
    {
        return sequence_greater_than( s2, s1 );
    }

    inline int sequence_difference( uint16_t _s1, uint16_t _s2 )
    {
        int s1 = _s1;
        int s2 = _s2;
        if ( abs( s1 - s2 ) >= 32786 )
        {
            if ( s1 > s2 )
                s2 += 65536;
            else
                s1 += 65536;
        }
        return s1 - s2;
    }

    inline int signed_to_unsigned( int n )
    {
        return ( n << 1 ) ^ ( n >> 31 );
    }

    inline int unsigned_to_signed( uint32_t n )
    {
        return ( n >> 1 ) ^ ( -int32_t( n & 1 ) );
    }

    void compress_packet_sequence( uint64_t sequence, uint8_t & prefix_byte, int & num_sequence_bytes, uint8_t * sequence_bytes );

    int get_packet_sequence_bytes( uint8_t prefix_byte );

    uint64_t decompress_packet_sequence( uint8_t prefix_byte, const uint8_t * sequence_bytes );

    uint32_t calculate_crc32( const uint8_t *buffer, size_t length, uint32_t crc32 = 0 );

    uint32_t hash_data( const uint8_t * data, uint32_t length, uint32_t hash );

    uint32_t hash_string( const char string[], uint32_t hash );

    uint64_t murmur_hash_64( const void * key, uint32_t length, uint64_t seed );

    void print_bytes( const char * label, const uint8_t * data, int data_bytes );
}

#endif // #ifndef YOJIMBO_COMMON_H
