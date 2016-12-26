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
    /**
        Bitpacks unsigned integer values, flushing to memory one dword at a time.

        How does it work? Integer bit values are written to a scratch value that is 64 bits wide, writing bits from right to left.

        Once low 32 bits of the scratch is filled with bits, the low 32 bits of scratch are flushed to memory as a dword, and the scratch buffer is shifted right by 32.

        The bit stream is written to memory in little endian order, which is considered network byte order for this library.
     */

    class BitWriter
    {
    public:

        /**
            Bit writer constructor.

			Creates a bit writer object to write to the specified buffer. 
			
            @param data The pointer to the buffer to fill with bitpacked data.
            @param bytes The size of the buffer in bytes. Must be a multiple of 4, because the bitpacker reads and writes memory as dwords, not bytes.
         */

        BitWriter( void * data, int bytes ) : m_data( (uint32_t*) data ), m_numWords( bytes / 4 )
        {
            assert( data );
            assert( ( bytes % 4 ) == 0 );
            m_numBits = m_numWords * 32;
            m_bitsWritten = 0;
            m_wordIndex = 0;
            m_scratch = 0;
            m_scratchBits = 0;
        }

        /**
            Write bits to the buffer.

            Bits are written to the buffer as-is, without padding to nearest byte. Will assert if you try to write past the end of the buffer.

            A boolean value writes just 1 bit to the buffer, a value in range [0,31] can be written with just 5 bits and so on.

	        IMPORTANT: When you have finished writing to your buffer, take care to call BitWrite::FlushBits, otherwise the last dword of data will not get flushed to memory!

            @param value The integer value to write to the buffer. Must be in [0,(1<<bits)-1].
            @param bits The number of bits to encode in [1,32].
         */

        void WriteBits( uint32_t value, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
            assert( m_bitsWritten + bits <= m_numBits );
            assert( uint64_t( value ) <= ( ( 1ULL << bits ) - 1 ) );

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

		/**
			Write an alignment to the bit stream, padding zeros so the bit index becomes is a multiple of 8.

			This is useful if you want to write some data to a packet that should be byte aligned. For example, an array of bytes, or a string.

			IMPORTANT: If the current bit index is already a multiple of 8, nothing is written.
		 */

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

		/**
			Write an array of bytes to the bit stream.

			Use this when you have to copy a large block of data into your bitstream.

			Faster than just writing each byte to the bit stream via BitWriter::WriteBits( value, 8 ), because it aligns to byte index and copies into the buffer without bitpacking.

			@param data The byte array data to write to the bit stream.
			@param bytes The number of bytes to write.
		 */

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

		/**
			Flush any remaining bits to memory.

			Call this once after you've finished writing bits to flush the last dword of scratch to memory!
		 */

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

		/**
			How many align bits would be written, if we were to write an align right now?

			@returns Result in [0,7], where 0 is zero bits required to align (already aligned) and 7 is worst case.
		 */

        int GetAlignBits() const
        {
            return ( 8 - ( m_bitsWritten % 8 ) ) % 8;
        }

		/**	
			How many bits have we written so far?

			@returns The number of bits written to the bit buffer.
		 */

        int GetBitsWritten() const
        {
            return m_bitsWritten;
        }

		/**
			How many bits are still available to write?

			For example, if the buffer size is 4, we have 32 bits available to write, if we have already written 10 bytes then 22 are still available to write.

			@returns The number of bits available to write.
		 */

        int GetBitsAvailable() const
        {
            return m_numBits - m_bitsWritten;
        }
		
		/**
			Get a pointer to the data written by the bit writer.

			Corresponds to the data block passed in to the constructor.

			@param returns Pointer to the data written by the bit writer.
		 */

        const uint8_t * GetData() const
        {
            return (uint8_t*) m_data;
        }

		/**
			The number of bytes flushed to memory.

			This is effectively the size of the packet that you should send after you have finished bitpacking values with this class.

			The returned value is not always a multiple of 4, even though we flush dwords to memory. You won't miss any data in this case because the order of bits written is designed to work with the little endian memory layout.

			IMPORTANT: Make sure you call BitWriter::FlushBits before calling this method, otherwise you risk missing the last dword of data.
		 */

        int GetBytesWritten() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

    private:

        uint32_t * m_data;									///< The buffer we are writing to, as a uint32_t * because we're writing dwords at a time.
        uint64_t m_scratch;									///< The scratch buffer where we write bits to (right to left). 64 bit for overflow. Once # of bits in scratch is >= 32, the low 32 bits are flushed to memory.
        int m_numBits;										///< The number of bits in the buffer. This is equivalent to the size of the buffer in bytes multiplied by 8. Note that the buffer size must always be a multiple of 4.
        int m_numWords;										///< The number of words in the buffer. This is equivalent to the size of the buffer in bytes divided by 4. Note that the buffer size mult always be a multiple of 4.
        int m_bitsWritten;									///< The number of bits written so far.
        int m_wordIndex;									///< The current word index. The next word flushed to memory will be at this index in m_data.
        int m_scratchBits;									///< The number of bits in scratch. When this is >= 32, the low 32 bits of scratch is flushed to memory as a dword and scratch is shifted right by 32.
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
