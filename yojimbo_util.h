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

#ifndef YOJIMBO_UTIL_H
#define YOJIMBO_UTIL_H

#include "yojimbo_config.h"
#include <stdint.h>
#include <assert.h>

namespace yojimbo
{
    inline void CompressPacketSequence( uint64_t sequence, uint8_t & prefix_byte, int & num_sequence_bytes, uint8_t * sequence_bytes )
    {
        // algorithm: encode a mask of 7 bits. each bit is set if byte n in the sequence is non-zero

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

    inline int GetPacketSequenceBytes( uint8_t prefix_byte )
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

    inline uint64_t DecompressPacketSequence( uint8_t prefix_byte, const uint8_t * sequence_bytes )
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

#if 0

    inline uint64_t DecompressPacketSequence( uint8_t prefix_byte, int & num_postfix_bytes, const uint8_t * postfix_bytes_end )
    {
        num_postfix_bytes = 0;

        for ( int i = 7; i > 0; --i )
        {
            if ( prefix_byte & ( 1 << (i-1) ) )
                num_postfix_bytes++;
        }

        num_postfix_bytes++;

        assert( num_postfix_bytes <= 8 );

        const uint8_t *postfix_bytes = postfix_bytes_end - num_postfix_bytes;

        uint64_t sequence = 0;

        int index = 0;

        for ( int i = 7; i > 0; --i )
        {
            /*
            const uint8_t current_sequence_byte = uint8_t( ( sequence & byte_mask ) >> (i*8) );

            if ( current_sequence_byte != 0 )
            {
                postfix_bytes[num_postfix_bytes++] = current_sequence_byte;
                prefix_byte |= ( 1 << (i-1) );
            }

            byte_mask >>= 8;
            */
        }

        assert( postfix_bytes + index + 1 == postfix_bytes_end );

        sequence |= uint64_t( postfix_bytes[index] );

        return sequence;
    }

#endif
}

#endif // #ifndef YOJIMBO_UTIL_H
