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

#include "yojimbo_common.h"
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace yojimbo
{
    void compress_packet_sequence( uint64_t sequence, uint8_t & prefix_byte, int & num_sequence_bytes, uint8_t * sequence_bytes )
    {
        // algorithm: encode a mask of 7 bits into one byte. each bit is set if byte n in the sequence is non-zero
        // non-zero bytes follow after the first byte. the low byte in sequence is always sent (hence 7 bits, not 8).

        assert( sequence_bytes );

        prefix_byte = 0;
        
        num_sequence_bytes = 0;

        uint64_t byte_mask = 0xFF00000000000000ULL;

        for ( int i = 7; i > 0; --i )
        {
            const uint8_t current_sequence_byte = uint8_t( ( sequence & byte_mask ) >> (i*8) );

            if ( current_sequence_byte != 0 )
            {
                sequence_bytes[num_sequence_bytes++] = current_sequence_byte;
                prefix_byte |= ( 1 << (i-1) );
            }

            byte_mask >>= 8;
        }

        assert( num_sequence_bytes <= 7 );
        assert( ( prefix_byte & (1<<7) ) == 0 );

        sequence_bytes[num_sequence_bytes++] = (uint8_t) ( sequence & 0xFF );
    }

    int get_packet_sequence_bytes( uint8_t prefix_byte )
    {
        int num_sequence_bytes = 0;

        for ( int i = 7; i > 0; --i )
        {
            if ( prefix_byte & ( 1 << (i-1) ) )
                num_sequence_bytes++;
        }

        num_sequence_bytes++;

        return num_sequence_bytes;
    }

    uint64_t decompress_packet_sequence( uint8_t prefix_byte, const uint8_t * sequence_bytes )
    {
        assert( sequence_bytes );

        uint64_t sequence = 0;

        int index = 0;

        for ( int i = 7; i > 0; --i )
        {
            if ( prefix_byte & ( 1 << (i-1) ) )
            {
                sequence |= ( uint64_t( sequence_bytes[index] ) << (i*8) );
                ++index;
            }
        }

        sequence |= uint64_t( sequence_bytes[index] );

        return sequence;
    }

    static const uint32_t crc32_table[256] = 
    {
        0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
        0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
        0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
        0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
        0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
        0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
        0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
        0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
        0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
        0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
        0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
        0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
        0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
        0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
        0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
        0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
        0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
        0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
        0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
        0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
        0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
        0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
        0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
        0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
        0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
        0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
        0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
        0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
        0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
        0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
        0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
        0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D 
    };

    uint32_t calculate_crc32( const uint8_t *buffer, size_t length, uint32_t crc32 )
    {
        crc32 ^= 0xFFFFFFFF;
        for ( size_t i = 0; i < length; ++i ) 
            crc32 = ( crc32 >> 8 ) ^ crc32_table[ ( crc32 ^ buffer[i] ) & 0xFF ];
        return crc32 ^ 0xFFFFFFFF;
    }

    uint32_t hash_data( const uint8_t * data, uint32_t length, uint32_t hash )
    {
        assert( data );
        for ( uint32_t i = 0; i < length; ++i )
        {
            hash += data[i];
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        return hash;
    } 

    uint32_t hash_string( const char string[], uint32_t hash )
    {
        assert( string );
        while ( *string != '\0' )
        {
            char c = *string++;
            if ( ( c >= 'a' ) && ( c <= 'z' ) ) 
                c = ( c - 'a' ) + 'A';
            hash += c;
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        return hash;
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
            printf( "%02x", (int) data[i] );
            if ( i != data_bytes - 1 )
                printf( "-" );
        }
        printf( " (%d bytes)\n", data_bytes );
    }

    enum base64_encode_step
    {
        step_A, step_B, step_C
    };

    struct base64_encode_state
    {
        base64_encode_step step;
        char result;
        int stepcount;
    } ;

    enum base64_decode_step
    {
        step_a, step_b, step_c, step_d
    };

    struct base64_decode_state
    {
        base64_decode_step step;
        char plainchar;
    };

    void base64_init_encode_state( base64_encode_state * state_in )
    {
        state_in->step = step_A;
        state_in->result = 0;
        state_in->stepcount = 0;
    }

    char base64_encode_value( char value_in )
    {
        static const char * encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        if ( value_in > 63 ) return '=';
        return encoding[(int)value_in];
    }

    int base64_encode_block( const char * plaintext_in, int length_in, char * code_out, base64_encode_state * state_in )
    {
        const char * plainchar = plaintext_in;
        const char * const plaintextend = plaintext_in + length_in;
        char * codechar = code_out;
        char result;
        char fragment;
        
        result = state_in->result;
        
        switch ( state_in->step )
        {
            while (1)
            {
        case step_A:
                if ( plainchar == plaintextend )
                {
                    state_in->result = result;
                    state_in->step = step_A;
                    return codechar - code_out;
                }
                fragment = *plainchar++;
                result = ( fragment & 0x0fc ) >> 2;
                *codechar++ = base64_encode_value( result );
                result = ( fragment & 0x003 ) << 4;
        case step_B:
                if ( plainchar == plaintextend )
                {
                    state_in->result = result;
                    state_in->step = step_B;
                    return codechar - code_out;
                }
                fragment = *plainchar++;
                result |= ( fragment & 0x0f0 ) >> 4;
                *codechar++ = base64_encode_value( result );
                result = ( fragment & 0x00f ) << 2;
        case step_C:
                if ( plainchar == plaintextend )
                {
                    state_in->result = result;
                    state_in->step = step_C;
                    return codechar - code_out;
                }
                fragment = *plainchar++;
                result |= ( fragment & 0x0c0 ) >> 6;
                *codechar++ = base64_encode_value( result );
                result  = (fragment & 0x03f) >> 0;
                *codechar++ = base64_encode_value( result );
                
                ++( state_in->stepcount );
            }
        }
        // control should not reach here
        assert( false );
        return codechar - code_out;
    }

    int base64_encode_block_end( char * code_out, base64_encode_state * state_in )
    {
        char * codechar = code_out;
        
        switch (state_in->step)
        {
        case step_B:
            *codechar++ = base64_encode_value( state_in->result );
            *codechar++ = '=';
            *codechar++ = '=';
            break;
        case step_C:
            *codechar++ = base64_encode_value( state_in->result );
            *codechar++ = '=';
            break;
        case step_A:
            break;
        }

        int length = codechar - code_out;

        code_out[length] = '\0';

        return length + 1;
    }

    int base64_decode_value( char value_in )
    {
        static const char decoding[] = { 62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51 };
        static const char decoding_size = sizeof( decoding );
        value_in -= 43;
        if ( value_in < 0 || value_in > decoding_size ) return -1;
        return decoding[(int)value_in];
    }

    void base64_init_decode_state( base64_decode_state * state_in )
    {
        state_in->step = step_a;
        state_in->plainchar = 0;
    }

    int base64_decode_block( const char * code_in, const int length_in, char * plaintext_out, base64_decode_state * state_in )
    {
        const char * codechar = code_in;
        char * plainchar = plaintext_out;
        char fragment;
        
        *plainchar = state_in->plainchar;
        
        switch ( state_in->step )
        {
            while (1)
            {
        case step_a:
                do {
                    if ( codechar == code_in + length_in )
                    {
                        state_in->step = step_a;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char) base64_decode_value( *codechar++ );
                } while ( fragment < 0 );
                *plainchar = ( fragment & 0x03f ) << 2;
        case step_b:
                do {
                    if ( codechar == code_in + length_in )
                    {
                        state_in->step = step_b;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char) base64_decode_value( *codechar++ );
                } while ( fragment < 0 );
                *plainchar++ |= ( fragment & 0x030 ) >> 4;
                *plainchar    = ( fragment & 0x00f ) << 4;
        case step_c:
                do {
                    if ( codechar == code_in + length_in )
                    {
                        state_in->step = step_c;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char) base64_decode_value( *codechar++ );
                } while ( fragment < 0 );
                *plainchar++ |= ( fragment & 0x03c ) >> 2;
                *plainchar    = ( fragment & 0x003 ) << 6;
        case step_d:
                do {
                    if ( codechar == code_in + length_in )
                    {
                        state_in->step = step_d;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char) base64_decode_value( *codechar++ );
                } while ( fragment < 0 );
                *plainchar++ |= ( fragment & 0x03f );
            }
        }
        // control should not reach here
        assert( false );
        return plainchar - plaintext_out;
    }

    int base64_encode_string( const char * input, char * output, int output_size )
    {
        assert( input );
        assert( output );
        assert( output_size > 0 );

        (void)output_size;

        base64_encode_state encode_state;

        base64_init_encode_state( &encode_state );

        int input_length = (int) strlen( input );

        assert( output_size >= input_length * 2 );

        const int output_body = base64_encode_block( input, input_length, output, &encode_state );

        const int output_tail = base64_encode_block_end( output + output_body, &encode_state );

        return output_body + output_tail;
    }

    int base64_decode_string( const char * input, char * output, int output_size )
    {
        assert( input );
        assert( output );
        assert( output_size > 0 );

        (void)output_size;

        base64_decode_state decode_state;

        base64_init_decode_state( &decode_state );

        const int input_length = strlen( input );

        assert( output_size >= input_length / 2 );

        const int decoded_bytes = base64_decode_block( input, input_length, output, &decode_state );

        output[decoded_bytes] = '\0';

        return decoded_bytes + 1;
    }

    int base64_encode_data( const uint8_t * input, int input_length, char * output, int output_size )
    {
        assert( input );
        assert( input_length > 0 );
        assert( output );
        assert( output_size > 0 );

        (void)output_size;

        base64_encode_state encode_state;

        base64_init_encode_state( &encode_state );

        assert( output_size >= input_length * 2 );

        const int output_body = base64_encode_block( (const char*) input, input_length, output, &encode_state );

        const int output_tail = base64_encode_block_end( output + output_body, &encode_state );

        return output_body + output_tail;
    }

    int base64_decode_data( const char * input, uint8_t * output, int output_size )
    {
        assert( input );
        assert( output );
        assert( output_size > 0 );

        (void)output_size;

        base64_decode_state decode_state;

        base64_init_decode_state( &decode_state );

        const int input_length = strlen( input );

        assert( output_size >= input_length / 2 );

        const int decoded_bytes = base64_decode_block( input, input_length, (char*) output, &decode_state );

        return decoded_bytes;
    }
}
