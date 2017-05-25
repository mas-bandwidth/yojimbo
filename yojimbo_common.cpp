/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#include "yojimbo_config.h"
#include "yojimbo_common.h"
#ifdef _MSC_VER
#include <malloc.h>
#endif // #ifdef _MSC_VER
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <mbedtls/base64.h>

extern "C" void netcode_random_bytes( uint8_t*, int );

namespace yojimbo
{
    void random_bytes( uint8_t * data, int bytes )
    {
        netcode_random_bytes( data, bytes );
    }

    uint64_t murmur_hash_64( const void * key, uint32_t length, uint64_t seed )
    {
        const uint64_t m = 0xc6a4a7935bd1e995ULL;
        const uint32_t r = 47;

        uint64_t h = seed ^ ( length * m );

        const uint64_t * data = ( const uint64_t*) key;
        const uint64_t * end = data + length / 8;

        while ( data != end )
        {
#if YOJIMBO_LITTLE_ENDIAN
            uint64_t k = *data++;
#else // #if YOJIMBO_LITTLE_ENDIAN
            uint64_t k = *data++;
            uint8_t * p = (uint8_t*) &k;
            uint8_t c;
            c = p[0]; p[0] = p[7]; p[7] = c;
            c = p[1]; p[1] = p[6]; p[6] = c;
            c = p[2]; p[2] = p[5]; p[5] = c;
            c = p[3]; p[3] = p[4]; p[4] = c;
#endif // #if YOJIMBO_LITTLE_ENDIAN

            k *= m;
            k ^= k >> r;
            k *= m;
            
            h ^= k;
            h *= m;
        }

        const uint8_t * data2 = (const uint8_t*) data;

        switch ( length & 7 )
        {
            case 7: h ^= uint64_t( data2[6] ) << 48;
            case 6: h ^= uint64_t( data2[5] ) << 40;
            case 5: h ^= uint64_t( data2[4] ) << 32;
            case 4: h ^= uint64_t( data2[3] ) << 24;
            case 3: h ^= uint64_t( data2[2] ) << 16;
            case 2: h ^= uint64_t( data2[1] ) << 8;
            case 1: h ^= uint64_t( data2[0] );
                    h *= m;
        };
        
        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
    }

    void print_bytes( const char * label, const uint8_t * data, int data_bytes )
    {
        printf( "%s: ", label );
        for ( int i = 0; i < data_bytes; ++i )
        {
            printf( "0x%02x,", (int) data[i] );
        }
        printf( " (%d bytes)\n", data_bytes );
    }

    int base64_encode_string( const char * input, char * output, int output_size )
    {
        assert( input );
        assert( output );
        assert( output_size > 0 );

        size_t output_length = 0;

        const int input_length = (int) ( strlen( input ) + 1 );

        int result = mbedtls_base64_encode( (unsigned char*) output, output_size, &output_length, (unsigned char*) input, input_length );

        return ( result == 0 ) ? (int) output_length + 1 : -1;
    }

    int base64_decode_string( const char * input, char * output, int output_size )
    {
        assert( input );
        assert( output );
        assert( output_size > 0 );

        size_t output_length = 0;

        int result = mbedtls_base64_decode( (unsigned char*) output, output_size, &output_length, (const unsigned char*) input, strlen( input ) );

        if ( result != 0 || output[output_length-1] != '\0' )
        {
            output[0] = '\0';
            return -1;
        }

        return (int) output_length;
    }

    int base64_encode_data( const uint8_t * input, int input_length, char * output, int output_size )
    {
        assert( input );
        assert( output );
        assert( output_size > 0 );

        size_t output_length = 0;

        int result = mbedtls_base64_encode( (unsigned char*) output, output_size, &output_length, (unsigned char*) input, input_length );

        return ( result == 0 ) ? (int) output_length : -1;
    }

    int base64_decode_data( const char * input, uint8_t * output, int output_size )
    {
        assert( input );
        assert( output );
        assert( output_size > 0 );

        size_t output_length = 0;

        int result = mbedtls_base64_decode( (unsigned char*) output, output_size, &output_length, (const unsigned char*) input, strlen( input ) );

        return ( result == 0 ) ? (int) output_length : -1;
    }

#if YOJIMBO_DEBUG_SPAM

    void debug_printf( const char * format, ... ) 
    {
        va_list args;
        va_start( args, format );
        vprintf( format, args );
        va_end( args );
    }

#else // #if YOJIMBO_DEBUG_SPAM

    void debug_printf( const char * format, ... ) 
    {
        (void)format;
    }

#endif // #if YOJIMBO_DEBUG_SPAM
}
