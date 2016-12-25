/*
    Yojimbo Client/Server Network Protocol Library.
    
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

#ifndef YOJIMBO_BITPACK_H
#define YOJIMBO_BITPACK_H

#include "yojimbo_config.h"
#include "yojimbo_common.h"
#include <assert.h>
#include <string.h>

namespace yojimbo
{
    // todo: document this file

    class BitWriter
    {
    public:

        BitWriter( void * data, int bytes ) 
            : m_data( (uint32_t*) data ), m_numWords( bytes / 4 )
        {
            assert( data );
            assert( ( bytes % 4 ) == 0 );           // buffer size must be a multiple of four
            m_numBits = m_numWords * 32;
            m_bitsWritten = 0;
            m_wordIndex = 0;
            m_scratch = 0;
            m_scratchBits = 0;
        }

        void WriteBits( uint32_t value, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
            assert( m_bitsWritten + bits <= m_numBits );

            value &= ( uint64_t(1) << bits ) - 1;

            m_scratch |= uint64_t( value ) << m_scratchBits;

            m_scratchBits += bits;

            if ( m_scratchBits >= 32 )
            {
                assert( m_wordIndex < m_numWords );
                m_data[m_wordIndex] = host_to_network( uint32_t( m_scratch & 0xFFFFFFFF ) );
                m_scratch >>= 32;
                m_scratchBits -= 32;
                m_wordIndex++;
            }

            m_bitsWritten += bits;
        }

        void WriteAlign()
        {
            const int remainderBits = m_bitsWritten % 8;
            if ( remainderBits != 0 )
            {
                uint32_t zero = 0;
                WriteBits( zero, 8 - remainderBits );
                assert( ( m_bitsWritten % 8 ) == 0 );
            }
        }

        void WriteBytes( const uint8_t * data, int bytes )
        {
            assert( GetAlignBits() == 0 );
            assert( m_bitsWritten + bytes * 8 <= m_numBits );
            assert( ( m_bitsWritten % 32 ) == 0 || ( m_bitsWritten % 32 ) == 8 || ( m_bitsWritten % 32 ) == 16 || ( m_bitsWritten % 32 ) == 24 );

            int headBytes = ( 4 - ( m_bitsWritten % 32 ) / 8 ) % 4;
            if ( headBytes > bytes )
                headBytes = bytes;
            for ( int i = 0; i < headBytes; ++i )
                WriteBits( data[i], 8 );
            if ( headBytes == bytes )
                return;

            FlushBits();

            assert( GetAlignBits() == 0 );

            int numWords = ( bytes - headBytes ) / 4;
            if ( numWords > 0 )
            {
                assert( ( m_bitsWritten % 32 ) == 0 );
                memcpy( &m_data[m_wordIndex], data + headBytes, numWords * 4 );
                m_bitsWritten += numWords * 32;
                m_wordIndex += numWords;
                m_scratch = 0;
            }

            assert( GetAlignBits() == 0 );

            int tailStart = headBytes + numWords * 4;
            int tailBytes = bytes - tailStart;
            assert( tailBytes >= 0 && tailBytes < 4 );
            for ( int i = 0; i < tailBytes; ++i )
                WriteBits( data[tailStart+i], 8 );

            assert( GetAlignBits() == 0 );

            assert( headBytes + numWords * 4 + tailBytes == bytes );
        }

        void FlushBits()
        {
            if ( m_scratchBits != 0 )
            {
                assert( m_wordIndex < m_numWords );
                m_data[m_wordIndex] = host_to_network( uint32_t( m_scratch & 0xFFFFFFFF ) );
                m_scratch >>= 32;
                m_scratchBits -= 32;
                m_wordIndex++;                
            }
        }

        int GetAlignBits() const
        {
            return ( 8 - ( m_bitsWritten % 8 ) ) % 8;
        }

        int GetBitsWritten() const
        {
            return m_bitsWritten;
        }

        int GetBitsAvailable() const
        {
            return m_numBits - m_bitsWritten;
        }

        const uint8_t * GetData() const
        {
            return (uint8_t*) m_data;
        }

        int GetBytesWritten() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

        int GetTotalBytes() const
        {
            return m_numWords * 4;
        }

    private:

        uint32_t * m_data;
        uint64_t m_scratch;
        int m_numBits;
        int m_numWords;
        int m_bitsWritten;
        int m_wordIndex;
        int m_scratchBits;
    };

    class BitReader
    {
    public:

#ifdef DEBUG
        BitReader( const void * data, int bytes ) : m_data( (const uint32_t*) data ), m_numBytes( bytes ), m_numWords( ( bytes + 3 ) / 4)
#else // #ifdef DEBUG
        BitReader( const void * data, int bytes ) : m_data( (const uint32_t*) data ), m_numBytes( bytes )
#endif // #ifdef DEBUG
        {
            // IMPORTANT: Although we support non-multiples of four bytes passed in, the actual buffer
            // underneath the bit reader must round up to at least 4 bytes because we read a dword at a time.
            assert( data );
            m_numBits = m_numBytes * 8;
            m_bitsRead = 0;
            m_scratch = 0;
            m_scratchBits = 0;
            m_wordIndex = 0;
        }

        bool WouldOverflow( int bits ) const
        {
            return m_bitsRead + bits > m_numBits;
        }

        uint32_t ReadBits( int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
            assert( m_bitsRead + bits <= m_numBits );

            m_bitsRead += bits;

            assert( m_scratchBits >= 0 && m_scratchBits <= 64 );

            if ( m_scratchBits < bits )
            {
                assert( m_wordIndex < m_numWords );
                m_scratch |= uint64_t( network_to_host( m_data[m_wordIndex] ) ) << m_scratchBits;
                m_scratchBits += 32;
                m_wordIndex++;
            }

            assert( m_scratchBits >= bits );

            const uint32_t output = m_scratch & ( (uint64_t(1)<<bits) - 1 );

            m_scratch >>= bits;
            m_scratchBits -= bits;

            return output;
        }

        bool ReadAlign()
        {
            const int remainderBits = m_bitsRead % 8;
            if ( remainderBits != 0 )
            {
                uint32_t value = ReadBits( 8 - remainderBits );
                assert( m_bitsRead % 8 == 0 );
                if ( value != 0 )
                    return false;
            }
            return true;
        }

        void ReadBytes( uint8_t * data, int bytes )
        {
            assert( GetAlignBits() == 0 );
            assert( m_bitsRead + bytes * 8 <= m_numBits );
            assert( ( m_bitsRead % 32 ) == 0 || ( m_bitsRead % 32 ) == 8 || ( m_bitsRead % 32 ) == 16 || ( m_bitsRead % 32 ) == 24 );

            int headBytes = ( 4 - ( m_bitsRead % 32 ) / 8 ) % 4;
            if ( headBytes > bytes )
                headBytes = bytes;
            for ( int i = 0; i < headBytes; ++i )
                data[i] = (uint8_t) ReadBits( 8 );
            if ( headBytes == bytes )
                return;

            assert( GetAlignBits() == 0 );

            int numWords = ( bytes - headBytes ) / 4;
            if ( numWords > 0 )
            {
                assert( ( m_bitsRead % 32 ) == 0 );
                memcpy( data + headBytes, &m_data[m_wordIndex], numWords * 4 );
                m_bitsRead += numWords * 32;
                m_wordIndex += numWords;
                m_scratchBits = 0;
            }

            assert( GetAlignBits() == 0 );

            int tailStart = headBytes + numWords * 4;
            int tailBytes = bytes - tailStart;
            assert( tailBytes >= 0 && tailBytes < 4 );
            for ( int i = 0; i < tailBytes; ++i )
                data[tailStart+i] = (uint8_t) ReadBits( 8 );

            assert( GetAlignBits() == 0 );

            assert( headBytes + numWords * 4 + tailBytes == bytes );
        }

        int GetAlignBits() const
        {
            return ( 8 - m_bitsRead % 8 ) % 8;
        }

        int GetBitsRead() const
        {
            return m_bitsRead;
        }

        int GetBytesRead() const
        {
            return m_wordIndex * 4;
        }

        int GetBitsRemaining() const
        {
            return m_numBits - m_bitsRead;
        }

        int GetBytesRemaining() const
        {
            return GetBitsRemaining() / 8;
        }

    private:

        const uint32_t * m_data;
        uint64_t m_scratch;
        int m_numBits;
        int m_numBytes;
#ifndef NDEBUG
        int m_numWords;
#endif // #ifndef NDEBUG
        int m_bitsRead;
        int m_scratchBits;
        int m_wordIndex;
    };
}

#endif // #ifndef YOJIMBO_BITPACK_H
