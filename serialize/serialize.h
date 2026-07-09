/*
    serialize

    Copyright © 2016 - 2026, Más Bandwidth LLC.

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

#ifndef SERIALIZE_H
#define SERIALIZE_H

/** @file */

#define SERIALIZE_VERSION_MAJOR 1
#define SERIALIZE_VERSION_MINOR 3
#define SERIALIZE_VERSION_PATCH 0
#define SERIALIZE_VERSION "1.3.0"

#if defined(_MSC_VER)
#define serialize_restrict __restrict
#else // #if defined(_MSC_VER)
#define serialize_restrict __restrict__
#endif // #if defined(_MSC_VER)

#ifndef serialize_assert
#include <assert.h>
#define serialize_assert assert
#endif // #ifndef serialize_assert

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#if !defined(SERIALIZE_DEBUG) && !defined(SERIALIZE_RELEASE)
#if defined(NDEBUG)
#define SERIALIZE_RELEASE
#else
#define SERIALIZE_DEBUG
#endif
#elif defined(SERIALIZE_DEBUG) && defined(SERIALIZE_RELEASE)
#error Can only define one of debug & release
#endif

#if !defined(SERIALIZE_LITTLE_ENDIAN ) && !defined( SERIALIZE_BIG_ENDIAN )

  #ifdef __BYTE_ORDER__
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      #define SERIALIZE_LITTLE_ENDIAN 1
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      #define SERIALIZE_BIG_ENDIAN 1
    #else
      #error Unknown machine endianess detected. User needs to define SERIALIZE_LITTLE_ENDIAN or SERIALIZE_BIG_ENDIAN.
    #endif // __BYTE_ORDER__

  // Detect with GLIBC's endian.h
  #elif defined(__GLIBC__)
    #include <endian.h>
    #if (__BYTE_ORDER == __LITTLE_ENDIAN)
      #define SERIALIZE_LITTLE_ENDIAN 1
    #elif (__BYTE_ORDER == __BIG_ENDIAN)
      #define SERIALIZE_BIG_ENDIAN 1
    #else
      #error Unknown machine endianess detected. User needs to define SERIALIZE_LITTLE_ENDIAN or SERIALIZE_BIG_ENDIAN.
    #endif // __BYTE_ORDER

  // Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
  #elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
    #define SERIALIZE_LITTLE_ENDIAN 1
  #elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
    #define SERIALIZE_BIG_ENDIAN 1

  // Detect with architecture macros
  #elif    defined(__sparc)     || defined(__sparc__)                           \
        || defined(_POWER)      || defined(__powerpc__)                         \
        || defined(__ppc__)     || defined(__hpux)      || defined(__hppa)      \
        || defined(_MIPSEB)     || defined(_POWER)      || defined(__s390__)
    #define SERIALIZE_BIG_ENDIAN 1
  #elif    defined(__i386__)    || defined(__alpha__)   || defined(__ia64)      \
        || defined(__ia64__)    || defined(_M_IX86)     || defined(_M_IA64)     \
        || defined(_M_ALPHA)    || defined(__amd64)     || defined(__amd64__)   \
        || defined(_M_AMD64)    || defined(__x86_64)    || defined(__x86_64__)  \
        || defined(_M_X64)      || defined(__bfin__)    || defined(_M_ARM64)
    #define SERIALIZE_LITTLE_ENDIAN 1
  #elif defined(_MSC_VER) && defined(_M_ARM)
    #define SERIALIZE_LITTLE_ENDIAN 1
  #else
    #error Unknown machine endianess detected. User needs to define SERIALIZE_LITTLE_ENDIAN or SERIALIZE_BIG_ENDIAN.
  #endif
#endif

#ifndef SERIALIZE_LITTLE_ENDIAN
#define SERIALIZE_LITTLE_ENDIAN 0
#endif

#ifndef SERIALIZE_BIG_ENDIAN
#define SERIALIZE_BIG_ENDIAN 0
#endif

// implicit narrowing is a deliberate style inside this header. push/pop so the disabled
// warnings do not leak into code that includes it. note that code using the serialize_* macros
// compiles at the including file's warning state: disable 4127 and 4244 there if needed.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#endif // #ifdef _MSC_VER

// everything the library itself needs. serialize.h is intentionally self contained:
// including it into a translation unit with no prior includes must compile.
#include <stdint.h>     // fixed width integer types
#include <stddef.h>     // size_t, NULL
#include <string.h>     // memcpy, memset, strlen
#include <wchar.h>      // wcslen
#include <math.h>       // ceil, floor

namespace serialize 
{
    /**
        Calculates the population count of an unsigned 32 bit integer at compile time.
        Population count is the number of bits in the integer that set to 1.
        See "Hacker's Delight" and http://www.hackersdelight.org/hdcodetxt/popArrayHS.c.txt
        @see serialize::Log2
        @see serialize::BitsRequired
     */

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

    /**
        Calculates the log 2 of an unsigned 32 bit integer at compile time.
        @see serialize::Log2
        @see serialize::BitsRequired
     */

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

    /**
        Calculates the number of bits required to serialize an integer value in [min,max] at compile time.
        @see Log2
        @see PopCount
     */

    template <int64_t min, int64_t max> struct BitsRequired
    {
        static const uint32_t result = ( min == max ) ? 0 : ( Log2<uint32_t(max-min)>::result + 1 );
    };

    /**
        Calculates the population count of an unsigned 32 bit integer.
        The population count is the number of bits in the integer set to 1.
        @param x The input integer value.
        @returns The number of bits set to 1 in the input value.
     */

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

    /**
        Calculates the log base 2 of an unsigned 32 bit integer.
        @param x The input integer value.
        @returns The log base 2 of the input.
     */

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

    /**
        Calculates the number of bits required to serialize an integer in range [min,max].
        @param min The minimum value.
        @param max The maximum value.
        @returns The number of bits required to serialize the integer.
     */

    inline int bits_required( uint32_t min, uint32_t max )
    {
#ifdef __GNUC__
        return ( min == max ) ? 0 : 32 - __builtin_clz( max - min );
#else // #ifdef __GNUC__
        return ( min == max ) ? 0 : log2( max - min ) + 1;
#endif // #ifdef __GNUC__
    }

    /**
        Calculates the number of bits required to serialize a 64 bit integer in range [min,max].
        @param min The minimum value.
        @param max The maximum value.
        @returns The number of bits required to serialize the integer in [0,64].
     */

    inline int bits_required64( uint64_t min, uint64_t max )
    {
        if ( min == max )
        {
            return 0;
        }
        // subtract in the unsigned domain: max - min overflows signed arithmetic when the range is wider than 2^63
        const uint64_t diff = max - min;
#ifdef __GNUC__
        return 64 - __builtin_clzll( diff );
#else // #ifdef __GNUC__
        const uint32_t high = uint32_t( diff >> 32 );
        return high ? ( 32 + bits_required( 0, high ) ) : bits_required( 0, uint32_t( diff ) );
#endif // #ifdef __GNUC__
    }

    /**
        Reverse the order of bytes in a 64 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint64_t bswap( uint64_t value )
    {
#ifdef __GNUC__
        return __builtin_bswap64( value );
#else // #ifdef __GNUC__
        value = ( value & 0x00000000FFFFFFFF ) << 32 | ( value & 0xFFFFFFFF00000000 ) >> 32;
        value = ( value & 0x0000FFFF0000FFFF ) << 16 | ( value & 0xFFFF0000FFFF0000 ) >> 16;
        value = ( value & 0x00FF00FF00FF00FF ) << 8  | ( value & 0xFF00FF00FF00FF00 ) >> 8;
        return value;
#endif // #ifdef __GNUC__
    }

    /**
        Reverse the order of bytes in a 32 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint32_t bswap( uint32_t value )
    {
#ifdef __GNUC__
        return __builtin_bswap32( value );
#else // #ifdef __GNUC__
        return ( value & 0x000000ff ) << 24 | ( value & 0x0000ff00 ) << 8 | ( value & 0x00ff0000 ) >> 8 | ( value & 0xff000000 ) >> 24;
#endif // #ifdef __GNUC__
    }

    /**
        Reverse the order of bytes in a 16 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint16_t bswap( uint16_t value )
    {
        return ( value & 0x00ff ) << 8 | ( value & 0xff00 ) >> 8;
    }

    /**
        Template to convert an integer value from local byte order to network byte order.
        IMPORTANT: Because most machines are little endian, serialize defines network byte order to be little endian.
        @param value The input value in local byte order. Supported integer types: uint64_t, uint32_t, uint16_t.
        @returns The input value converted to network byte order. If this processor is little endian the output is the same as the input. If the processor is big endian, the output is the input byte swapped.
        @see serialize::bswap
     */

    template <typename T> T host_to_network( T value )
    {
#if SERIALIZE_BIG_ENDIAN
        return bswap( value );
#else // #if SERIALIZE_BIG_ENDIAN
        return value;
#endif // #if SERIALIZE_BIG_ENDIAN
    }

    /**
        Template to convert an integer value from network byte order to local byte order.
        IMPORTANT: Because most machines are little endian, serialize defines network byte order to be little endian.
        @param value The input value in network byte order. Supported integer types: uint64_t, uint32_t, uint16_t.
        @returns The input value converted to local byte order. If this processor is little endian the output is the same as the input. If the processor is big endian, the output is the input byte swapped.
        @see serialize::bswap
     */

    template <typename T> T network_to_host( T value )
    {
#if SERIALIZE_BIG_ENDIAN
        return bswap( value );
#else // #if SERIALIZE_BIG_ENDIAN
        return value;
#endif // #if SERIALIZE_BIG_ENDIAN
    }

    /**
        Convert a signed integer to an unsigned integer with zig-zag encoding.
        0,-1,+1,-2,+2... becomes 0,1,2,3,4 ...
        @param n The input value.
        @returns The input value converted from signed to unsigned with zig-zag encoding.
     */

    inline uint32_t signed_to_unsigned( int32_t n )
    {
        // shift in the unsigned domain: left shift of a negative signed value is undefined behavior pre C++20
        return ( uint32_t(n) << 1 ) ^ ( 0 - ( uint32_t(n) >> 31 ) );
    }

    /**
        Convert an unsigned integer to as signed integer with zig-zag encoding.
        0,1,2,3,4... becomes 0,-1,+1,-2,+2...
        @param n The input value.
        @returns The input value converted from unsigned to signed with zig-zag encoding.
     */

    inline int32_t unsigned_to_signed( uint32_t n )
    {
        return int32_t( ( n >> 1 ) ^ ( 0 - ( n & 1 ) ) );
    }

    template <typename T> T clamp( const T & value, const T & a, const T & b )
    {
        if ( value < a )
            return a;
        else if ( value > b )
            return b;
        else
            return value;
    }

    /**
        Bitpacks unsigned integer values to a buffer.
        Integer bit values are written to a 64 bit scratch value from right to left.
        Once the low 32 bits of the scratch is filled with bits it is flushed to memory as a dword and the scratch value is shifted right by 32.
        The bit stream is written to memory in little endian order, which is considered network byte order for this library.
        @see BitReader
     */

    class BitWriter
    {
    public:

        BitWriter() : m_data( NULL ), m_scratch( 0 ), m_numBits( 0 ), m_numWords( 0 ), m_bitsWritten( 0 ), m_wordIndex( 0 ), m_scratchBits( 0 ) {}

        void Initialize( void * serialize_restrict data, int bytes )
        {
            serialize_assert( data );
            serialize_assert( ( bytes % 4 ) == 0 );
            m_data = (uint32_t*) data;
            m_numWords = bytes / 4;
            m_numBits = m_numWords * 32;
            m_bitsWritten = 0;
            m_wordIndex = 0;
            m_scratch = 0;
            m_scratchBits = 0;
        }

        /**
            Bit writer constructor.
            Creates a bit writer object to write to the specified buffer.
            @param data The pointer to the buffer to fill with bitpacked data. Does not need to be aligned: each dword is stored with memcpy, matching the bit reader.
            @param bytes The size of the buffer in bytes. Must be a multiple of 4, because the bitpacker reads and writes memory as dwords, not bytes. Buffers up to 256 megabytes are supported, because bit counts are stored in 32 bit signed integers.
         */

        BitWriter( void * serialize_restrict data, int bytes ) : m_data( (uint32_t*) data ), m_numWords( bytes / 4 )
        {
            serialize_assert( data );
            serialize_assert( ( bytes % 4 ) == 0 );
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
            @see BitReader::ReadBits
         */

        void WriteBits( uint32_t value, int bits )
        {
            serialize_assert( m_data );                 // if this fires, the writer was used before Initialize
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            serialize_assert( m_bitsWritten + bits <= m_numBits );
            serialize_assert( uint64_t( value ) <= ( ( 1ULL << bits ) - 1 ) );

            m_scratch |= uint64_t( value ) << m_scratchBits;

            m_scratchBits += bits;

            if ( m_scratchBits >= 32 )
            {
                serialize_assert( m_wordIndex < m_numWords );
                const uint32_t word = host_to_network( uint32_t( m_scratch & 0xFFFFFFFF ) );
                memcpy( (uint8_t*) m_data + (size_t) m_wordIndex * 4, &word, sizeof( word ) );
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
            @see BitReader::ReadAlign
         */

        void WriteAlign()
        {
            const int remainderBits = m_bitsWritten % 8;

            if ( remainderBits != 0 )
            {
                uint32_t zero = 0;
                WriteBits( zero, 8 - remainderBits );
                serialize_assert( ( m_bitsWritten % 8 ) == 0 );
            }
        }

        /**
            Write an array of bytes to the bit stream.
            Use this when you have to copy a large block of data into your bitstream.
            Faster than just writing each byte to the bit stream via BitWriter::WriteBits( value, 8 ), because it aligns to byte index and copies into the buffer without bitpacking.
            @param data The byte array data to write to the bit stream.
            @param bytes The number of bytes to write.
            @see BitReader::ReadBytes
         */

        void WriteBytes( const uint8_t * serialize_restrict data, int bytes )
        {
            serialize_assert( m_data );                 // if this fires, the writer was used before Initialize
            serialize_assert( GetAlignBits() == 0 );
            serialize_assert( uint64_t(m_bitsWritten) + uint64_t(bytes) * 8 <= uint64_t(m_numBits) );
            serialize_assert( ( m_bitsWritten % 32 ) == 0 || ( m_bitsWritten % 32 ) == 8 || ( m_bitsWritten % 32 ) == 16 || ( m_bitsWritten % 32 ) == 24 );

            int headBytes = ( 4 - ( m_bitsWritten % 32 ) / 8 ) % 4;
            if ( headBytes > bytes )
                headBytes = bytes;
            for ( int i = 0; i < headBytes; ++i )
                WriteBits( data[i], 8 );
            if ( headBytes == bytes )
                return;

            FlushBits();

            serialize_assert( GetAlignBits() == 0 );

            int numWords = ( bytes - headBytes ) / 4;
            if ( numWords > 0 )
            {
                serialize_assert( ( m_bitsWritten % 32 ) == 0 );
                memcpy( (uint8_t*) m_data + (size_t) m_wordIndex * 4, data + headBytes, numWords * 4 );
                m_bitsWritten += numWords * 32;
                m_wordIndex += numWords;
                m_scratch = 0;
            }

            serialize_assert( GetAlignBits() == 0 );

            int tailStart = headBytes + numWords * 4;
            int tailBytes = bytes - tailStart;
            serialize_assert( tailBytes >= 0 && tailBytes < 4 );
            for ( int i = 0; i < tailBytes; ++i )
                WriteBits( data[tailStart+i], 8 );

            serialize_assert( GetAlignBits() == 0 );

            serialize_assert( headBytes + numWords * 4 + tailBytes == bytes );
        }

        /**
            Flush any remaining bits to memory.
            Call this once after you've finished writing bits to flush the last dword of scratch to memory!
            @see BitWriter::WriteBits
         */

        void FlushBits()
        {
            if ( m_scratchBits != 0 )
            {
                serialize_assert( m_data );             // if this fires, the writer was used before Initialize
                serialize_assert( m_scratchBits <= 32 );
                serialize_assert( m_wordIndex < m_numWords );
                const uint32_t word = host_to_network( uint32_t( m_scratch & 0xFFFFFFFF ) );
                memcpy( (uint8_t*) m_data + (size_t) m_wordIndex * 4, &word, sizeof( word ) );
                m_scratch >>= 32;
                m_scratchBits = 0;
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
            @returns Pointer to the data written by the bit writer.
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

        uint32_t * m_data;              ///< The buffer we are writing to, as a uint32_t * because we're writing dwords at a time.
        uint64_t m_scratch;             ///< The scratch value where we write bits to (right to left). 64 bit for overflow. Once # of bits in scratch is >= 32, the low 32 bits are flushed to memory.
        int m_numBits;                  ///< The number of bits in the buffer. This is equivalent to the size of the buffer in bytes multiplied by 8. Note that the buffer size must always be a multiple of 4.
        int m_numWords;                 ///< The number of words in the buffer. This is equivalent to the size of the buffer in bytes divided by 4. Note that the buffer size must always be a multiple of 4.
        int m_bitsWritten;              ///< The number of bits written so far.
        int m_wordIndex;                ///< The current word index. The next word flushed to memory will be at this index in m_data.
        int m_scratchBits;              ///< The number of bits in scratch. When this is >= 32, the low 32 bits of scratch is flushed to memory as a dword and scratch is shifted right by 32.
    };

    /**
        Reads bit packed integer values from a buffer.
        Relies on the user reconstructing the exact same set of bit reads as bit writes when the buffer was written. This is an unattributed bitpacked binary stream!
        Implementation: 32 bit dwords are read in from memory to the high bits of a scratch value as required. The user reads off bit values from the scratch value from the right, after which the scratch value is shifted by the same number of bits.
     */

    class BitReader
    {
    public:

        BitReader()
        {
            m_data = NULL;
            m_numBytes = 0;
            m_numWords = 0;
            m_numBits = m_numBytes * 8;
            m_bitsRead = 0;
            m_scratch = 0;
            m_scratchBits = 0;
            m_wordIndex = 0;
        }

        void Initialize( const void * serialize_restrict data, int bytes )
        {
            serialize_assert( data );
            m_data = (const uint32_t*) data;
            m_numBytes = bytes;
#ifdef SERIALIZE_DEBUG
            m_numWords = ( bytes + 3 ) / 4;
#endif // #ifdef SERIALIZE_DEBUG
            m_numBits = m_numBytes * 8;
            m_bitsRead = 0;
            m_scratch = 0;
            m_scratchBits = 0;
            m_wordIndex = 0;            
        }

        /**
            Bit reader constructor.
            Non-multiples of four buffer sizes are supported, as this naturally tends to occur when packets are read from the network.
            However, actual buffer allocated for the packet data must round up at least to the next 4 bytes in memory, because the bit reader reads dwords from memory not bytes.
            @param data Pointer to the bitpacked data to read. Does not need to be aligned: the reader loads each dword with memcpy, which packet payloads require because they typically start at an unaligned offset once the transport header is stripped.
            @param bytes The number of bytes of bitpacked data to read. Buffers up to 256 megabytes are supported, because bit counts are stored in 32 bit signed integers.
            @see BitWriter
         */

        BitReader( const void * serialize_restrict data, int bytes ) : m_data( (const uint32_t*) data ), m_numBytes( bytes ), m_numWords( ( bytes + 3 ) / 4 )
        {
            serialize_assert( data );
            m_numBits = m_numBytes * 8;
            m_bitsRead = 0;
            m_scratch = 0;
            m_scratchBits = 0;
            m_wordIndex = 0;
        }

        /**
            Would the bit reader would read past the end of the buffer if it read this many bits?
            @param bits The number of bits that would be read.
            @returns True if reading the number of bits would read past the end of the buffer.
         */

        bool WouldReadPastEnd( int bits ) const
        {
            return m_bitsRead + bits > m_numBits;
        }

        /**
            Read bits from the bit buffer.
            This function will assert in debug builds if this read would read past the end of the buffer.
            In production situations, the higher level ReadStream takes care of checking all packet data and never calling this function if it would read past the end of the buffer.
            @param bits The number of bits to read in [1,32].
            @returns The integer value read in range [0,(1<<bits)-1].
            @see BitReader::WouldReadPastEnd
            @see BitWriter::WriteBits
         */

        uint32_t ReadBits( int bits )
        {
            serialize_assert( m_data );                 // if this fires, the reader was used before Initialize
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            serialize_assert( m_bitsRead + bits <= m_numBits );

            m_bitsRead += bits;

            serialize_assert( m_scratchBits >= 0 && m_scratchBits <= 64 );

            if ( m_scratchBits < bits )
            {
#ifdef SERIALIZE_DEBUG
                serialize_assert( m_wordIndex < m_numWords );
#endif // SERIALIZE_DEBUG
                uint32_t word;
                memcpy( &word, (const uint8_t*) m_data + (size_t) m_wordIndex * 4, sizeof( word ) );
                m_scratch |= uint64_t( network_to_host( word ) ) << m_scratchBits;
                m_scratchBits += 32;
                m_wordIndex++;
            }

            serialize_assert( m_scratchBits >= bits );

            const uint32_t output = m_scratch & ( (uint64_t(1)<<bits) - 1 );

            m_scratch >>= bits;
            m_scratchBits -= bits;

            return output;
        }

        /**
            Read an align.
            Call this on read to correspond to a WriteAlign call when the bitpacked buffer was written.
            This makes sure we skip ahead to the next aligned byte index. As a safety check, we verify that the padding to next byte is zero bits and return false if that's not the case.
            This will typically abort packet read. Just another safety measure...
            @returns True if we successfully read an align and skipped ahead past zero pad, false otherwise (probably means, no align was written to the stream).
            @see BitWriter::WriteAlign
         */

        bool ReadAlign()
        {
            const int remainderBits = m_bitsRead % 8;
            if ( remainderBits != 0 )
            {
                uint32_t value = ReadBits( 8 - remainderBits );
                serialize_assert( m_bitsRead % 8 == 0 );
                if ( value != 0 )
                    return false;
            }
            return true;
        }

        /**
            Read bytes from the bitpacked data.
            @see BitWriter::WriteBytes
         */

        void ReadBytes( uint8_t * serialize_restrict data, int bytes )
        {
            serialize_assert( m_data );                 // if this fires, the reader was used before Initialize
            serialize_assert( GetAlignBits() == 0 );
            serialize_assert( uint64_t(m_bitsRead) + uint64_t(bytes) * 8 <= uint64_t(m_numBits) );
            serialize_assert( ( m_bitsRead % 32 ) == 0 || ( m_bitsRead % 32 ) == 8 || ( m_bitsRead % 32 ) == 16 || ( m_bitsRead % 32 ) == 24 );

            int headBytes = ( 4 - ( m_bitsRead % 32 ) / 8 ) % 4;
            if ( headBytes > bytes )
                headBytes = bytes;
            for ( int i = 0; i < headBytes; ++i )
                data[i] = (uint8_t) ReadBits( 8 );
            if ( headBytes == bytes )
                return;

            serialize_assert( GetAlignBits() == 0 );

            int numWords = ( bytes - headBytes ) / 4;
            if ( numWords > 0 )
            {
                serialize_assert( ( m_bitsRead % 32 ) == 0 );
                memcpy( (char*) data + headBytes, (const uint8_t*) m_data + (size_t) m_wordIndex * 4, numWords * 4 );
                m_bitsRead += numWords * 32;
                m_wordIndex += numWords;
                m_scratchBits = 0;
            }

            serialize_assert( GetAlignBits() == 0 );

            int tailStart = headBytes + numWords * 4;
            int tailBytes = bytes - tailStart;
            serialize_assert( tailBytes >= 0 && tailBytes < 4 );
            for ( int i = 0; i < tailBytes; ++i )
                data[tailStart+i] = (uint8_t) ReadBits( 8 );

            serialize_assert( GetAlignBits() == 0 );

            serialize_assert( headBytes + numWords * 4 + tailBytes == bytes );
        }

        /**
            How many align bits would be read, if we were to read an align right now?
            @returns Result in [0,7], where 0 is zero bits required to align (already aligned) and 7 is worst case.
         */

        int GetAlignBits() const
        {
            return ( 8 - m_bitsRead % 8 ) % 8;
        }

        /**
            How many bits have we read so far?
            @returns The number of bits read from the bit buffer so far.
         */

        int GetBitsRead() const
        {
            return m_bitsRead;
        }

        /**
            How many bits are still available to read?
            For example, if the buffer size is 4, we have 32 bits available to read, if we have already written 10 bytes then 22 are still available.
            @returns The number of bits available to read.
         */

        int GetBitsRemaining() const
        {
            return m_numBits - m_bitsRead;
        }

    private:

        const uint32_t * serialize_restrict m_data;         ///< The bitpacked data we're reading as a dword array.
        uint64_t m_scratch;                                 ///< The scratch value. New data is read in 32 bits at a top to the left of this buffer, and data is read off to the right.
        int m_numBits;                                      ///< Number of bits to read in the buffer. Of course, we can't *really* know this so it's actually m_numBytes * 8.
        int m_numBytes;                                     ///< Number of bytes to read in the buffer. We know this, and this is the non-rounded up version.
        int m_numWords;                                     ///< Number of words to read in the buffer. This is rounded up to the next word if necessary. Only used in debug builds.
        int m_bitsRead;                                     ///< Number of bits read from the buffer so far.
        int m_scratchBits;                                  ///< Number of bits currently in the scratch value. If the user wants to read more bits than this, we have to go fetch another dword from memory.
        int m_wordIndex;                                    ///< Index of the next word to read from memory.
    };

    /**
        Functionality common to all stream classes.
     */

    class BaseStream
    {
    public:

        /**
            Base stream constructor.
         */

        explicit BaseStream() : m_context( NULL ), m_allocator( NULL ) {}

        /**
            Set a context on the stream.
            The context lets you pass data through to your serialize functions, for example lookup tables or min/max ranges needed to read and write values.
            Call BaseStream::GetContext inside your serialize method to retrieve it.
         */

        void SetContext( void * context )
        {
            m_context = context;
        }

        /**
            Get the context pointer set on the stream.

            @returns The context pointer. May be NULL.
         */

        void * GetContext() const
        {
            return m_context;
        }

        /**
            Set an allocator pointer on the stream.
            This can be helpful if you want to perform allocations within serialize functions.
         */

        void SetAllocator( void * allocator )
        {
            m_allocator = allocator;
        }

        /**
            Get the allocator pointer set on the stream.

            @returns The allocator pointer. May be NULL.
         */

        void * GetAllocator() const
        {
            return m_allocator;
        }

    private:

        void * m_context;                           ///< The context pointer set on the stream. May be NULL.
        void * m_allocator;                         ///< The allocator pointer set on the stream. May be NULL.
    };

    /**
        Stream class for writing bitpacked data.
        This class is a wrapper around the bit writer class. Its purpose is to provide unified interface for reading and writing.
        You can determine if you are writing to a stream by calling Stream::IsWriting inside your templated serialize method.
        This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.
        IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead.
        @see BitWriter
     */

    class WriteStream : public BaseStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        WriteStream() : m_writer() {}

        void Initialize( uint8_t * buffer, int bytes )
        {
            m_writer.Initialize( buffer, bytes );
        }

        /**
            Write stream constructor.
            @param buffer The buffer to write to. Does not need to be aligned.
            @param bytes The number of bytes in the buffer. Must be a multiple of four.
         */

        WriteStream( uint8_t * buffer, int bytes ) : m_writer( buffer, bytes ) {}

        /**
            Serialize an integer (write).
            @param value The integer value in [min,max].
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on write.
         */

        bool SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            serialize_assert( min < max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required( min, max );
            // subtract in the unsigned domain: value - min overflows signed arithmetic when the range is wider than 2^31
            uint32_t unsigned_value = uint32_t(value) - uint32_t(min);
            m_writer.WriteBits( unsigned_value, bits );
            return true;
        }

        /**
            Serialize a 64 bit integer (write).
            @param value The integer value in [min,max].
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on write.
         */

        bool SerializeInteger64( int64_t value, int64_t min, int64_t max )
        {
            serialize_assert( min < max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required64( uint64_t(min), uint64_t(max) );
            // subtract in the unsigned domain: value - min overflows signed arithmetic when the range is wider than 2^63
            const uint64_t unsigned_value = uint64_t(value) - uint64_t(min);
            if ( bits <= 32 )
            {
                m_writer.WriteBits( uint32_t( unsigned_value ), bits );
            }
            else
            {
                // low dword first, then the high remainder: same convention as serialize_bits and serialize_uint64
                m_writer.WriteBits( uint32_t( unsigned_value & 0xFFFFFFFF ), 32 );
                m_writer.WriteBits( uint32_t( unsigned_value >> 32 ), bits - 32 );
            }
            return true;
        }

        /**
            Serialize a number of bits (write).
            @param value The unsigned integer value to serialize. Must be in range [0,(1<<bits)-1].
            @param bits The number of bits to write in [1,32].
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBits( uint32_t value, int bits )
        {
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            m_writer.WriteBits( value, bits );
            return true;
        }

        /**
            Serialize an array of bytes (write).
            @param data Array of bytes to be written.
            @param bytes The number of bytes to write.
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBytes( const uint8_t * data, int bytes )
        {
            serialize_assert( data );
            serialize_assert( bytes >= 0 );
            SerializeAlign();
            m_writer.WriteBytes( data, bytes );
            return true;
        }

        /**
            Serialize an align (write).
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeAlign()
        {
            m_writer.WriteAlign();
            return true;
        }

        /**
            If we were to write an align right now, how many bits would be required?
            @returns The number of zero pad bits required to achieve byte alignment in [0,7].
         */

        int GetAlignBits() const
        {
            return m_writer.GetAlignBits();
        }

        /**
            Flush the stream to memory after you finish writing.
            Always call this after you finish writing and before you call WriteStream::GetData, or you'll potentially truncate the last dword of data you wrote.
            @see BitWriter::FlushBits
         */

        void Flush()
        {
            m_writer.FlushBits();
        }

        /**
            Get a pointer to the data written by the stream.
            IMPORTANT: Call WriteStream::Flush before you call this function!
            @returns A pointer to the data written by the stream
         */

        const uint8_t * GetData() const
        {
            return m_writer.GetData();
        }

        /**
            How many bytes have been written so far?
            @returns Number of bytes written. This is effectively the packet size.
         */

        int GetBytesProcessed() const
        {
            return m_writer.GetBytesWritten();
        }

        /**
            Get number of bits written so far.
            @returns Number of bits written.
         */

        int GetBitsProcessed() const
        {
            return m_writer.GetBitsWritten();
        }

    private:

        BitWriter m_writer;                 ///< The bit writer used for all bitpacked write operations.
    };

    /**
        Stream class for reading bitpacked data.
        This class is a wrapper around the bit reader class. Its purpose is to provide unified interface for reading and writing.
        You can determine if you are reading from a stream by calling Stream::IsReading inside your templated serialize method.
        This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.
        IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead.
        @see BitReader
     */

    class ReadStream : public BaseStream
    {
    public:

        enum { IsWriting = 0 };
        enum { IsReading = 1 };

        ReadStream()
        {
            // ...
        }

        void Initialize( const uint8_t * buffer, int bytes )
        {
            m_reader.Initialize( buffer, bytes );
        }

        /**
            Read stream constructor.
            @param buffer The buffer to read from.
            @param bytes The number of bytes in the buffer. May be a non-multiple of four, however if it is, the underlying buffer allocated should be large enough to read the any remainder bytes as a dword.
         */

        ReadStream( const uint8_t * buffer, int bytes ) : m_reader( buffer, bytes ) {}

        /**
            Serialize an integer (read).
            @param value The integer value read is stored here. It is guaranteed to be in [min,max] if this function succeeds.
            @param min The minimum allowed value.
            @param max The maximum allowed value.
            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
         */

        bool SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            serialize_assert( min < max );
            const int bits = bits_required( min, max );
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint32_t unsigned_value = m_reader.ReadBits( bits );
            if ( unsigned_value > uint32_t(max) - uint32_t(min) )
                return false;
            // add in the unsigned domain: unsigned_value + min overflows signed arithmetic when the range is wider than 2^31
            value = int32_t( unsigned_value + uint32_t(min) );
            return true;
        }

        /**
            Serialize a 64 bit integer (read).
            @param value The integer value read is stored here. It is guaranteed to be in [min,max] if this function succeeds.
            @param min The minimum allowed value.
            @param max The maximum allowed value.
            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
         */

        bool SerializeInteger64( int64_t & value, int64_t min, int64_t max )
        {
            serialize_assert( min < max );
            const int bits = bits_required64( uint64_t(min), uint64_t(max) );
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint64_t unsigned_value;
            if ( bits <= 32 )
            {
                unsigned_value = m_reader.ReadBits( bits );
            }
            else
            {
                // low dword first, then the high remainder: same convention as serialize_bits and serialize_uint64
                const uint32_t lo = m_reader.ReadBits( 32 );
                const uint32_t hi = m_reader.ReadBits( bits - 32 );
                unsigned_value = ( uint64_t(hi) << 32 ) | lo;
            }
            if ( unsigned_value > uint64_t(max) - uint64_t(min) )
                return false;
            // add in the unsigned domain: unsigned_value + min overflows signed arithmetic when the range is wider than 2^63
            value = int64_t( unsigned_value + uint64_t(min) );
            return true;
        }

        /**
            Serialize a number of bits (read).
            @param value The integer value read is stored here. Will be in range [0,(1<<bits)-1].
            @param bits The number of bits to read in [1,32].
            @returns Returns true if the serialize read succeeded, false otherwise.
         */

        bool SerializeBits( uint32_t & value, int bits )
        {
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint32_t read_value = m_reader.ReadBits( bits );
            value = read_value;
            return true;
        }

        /**
            Serialize an array of bytes (read).
            @param data Array of bytes to read.
            @param bytes The number of bytes to read.
            @returns Returns true if the serialize read succeeded. False otherwise.
         */

        bool SerializeBytes( uint8_t * data, int bytes )
        {
            if ( bytes < 0 )
                return false;
            if ( !SerializeAlign() )
                return false;
            // compare in bytes rather than bits so a huge byte count can't overflow int
            if ( bytes > m_reader.GetBitsRemaining() / 8 )
                return false;
            m_reader.ReadBytes( data, bytes );
            return true;
        }

        /**
            Serialize an align (read).
            @returns Returns true if the serialize read succeeded. False otherwise.
         */

        bool SerializeAlign()
        {
            const int alignBits = m_reader.GetAlignBits();
            if ( m_reader.WouldReadPastEnd( alignBits ) )
                return false;
            if ( !m_reader.ReadAlign() )
                return false;
            return true;
        }

        /**
            If we were to read an align right now, how many bits would we need to read?
            @returns The number of zero pad bits required to achieve byte alignment in [0,7].
         */

        int GetAlignBits() const
        {
            return m_reader.GetAlignBits();
        }

        /**
            Get number of bits read so far.
            @returns Number of bits read.
         */

        int GetBitsProcessed() const
        {
            return m_reader.GetBitsRead();
        }

        /**
            How many bytes have been read so far?
            @returns Number of bytes read. Effectively this is the number of bits read, rounded up to the next byte where necessary.
         */

        int GetBytesProcessed() const
        {
            return ( m_reader.GetBitsRead() + 7 ) / 8;
        }

    private:

        BitReader m_reader;             ///< The bit reader used for all bitpacked read operations.
    };

    /**
        Stream class for estimating how many bits it would take to serialize something.
        This class acts like a bit writer (IsWriting is 1, IsReading is 0), but instead of writing data, it counts how many bits would be written.
        Note that when the serialization includes alignment to byte (see MeasureStream::SerializeAlign), this is an estimate and not an exact measurement. The estimate is guaranteed to be conservative.
        @see BitWriter
        @see BitReader
     */

    class MeasureStream : public BaseStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        /**
            Measure stream constructor.
         */

        explicit MeasureStream() : m_bitsWritten(0) {}

        /**
            Serialize an integer (measure).
            @param value The integer value to write. Not actually used or checked.
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on measure.
         */

        bool SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            (void) value;
            serialize_assert( min < max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required( min, max );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize a 64 bit integer (measure).
            @param value The integer value to write. Not actually used or checked.
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on measure.
         */

        bool SerializeInteger64( int64_t value, int64_t min, int64_t max )
        {
            (void) value;
            serialize_assert( min < max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required64( uint64_t(min), uint64_t(max) );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize a number of bits (write).
            @param value The unsigned integer value to serialize. Not actually used or checked.
            @param bits The number of bits to write in [1,32].
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBits( uint32_t value, int bits )
        {
            (void) value;
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize an array of bytes (measure).
            @param data Array of bytes to 'write'. Not actually used.
            @param bytes The number of bytes to 'write'.
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBytes( const uint8_t * data, int bytes )
        {
            (void) data;
            serialize_assert( bytes >= 0 );
            SerializeAlign();
            m_bitsWritten += bytes * 8;
            return true;
        }

        /**
            Serialize an align (measure).
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeAlign()
        {
            const int alignBits = GetAlignBits();
            m_bitsWritten += alignBits;
            return true;
        }

        /**
            If we were to write an align right now, how many bits would be required?
            IMPORTANT: Since the number of bits required for alignment depends on where an object is written in the final bit stream, this measurement is conservative.
            @returns Always returns worst case 7 bits.
         */

        int GetAlignBits() const
        {
            return 7;
        }

        /**
            Get number of bits written so far.
            @returns Number of bits written.
         */

        int GetBitsProcessed() const
        {
            return m_bitsWritten;
        }

        /**
            How many bytes have been written so far?
            @returns Number of bytes written.
         */

        int GetBytesProcessed() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

    private:

        int m_bitsWritten;              ///< Counts the number of bits written.
    };

    /**
        Serialize integer value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The integer value to serialize in [min,max].
        @param min The minimum value.
        @param max The maximum value.
     */

    #define serialize_int( stream, value, min, max )                    \
        do                                                              \
        {                                                               \
            serialize_assert( (min) < (max) );                          \
            int32_t int32_value = 0;                                    \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                serialize_assert( int64_t(value) >= int64_t(min) );     \
                serialize_assert( int64_t(value) <= int64_t(max) );     \
                int32_value = (int32_t) ( value );                      \
            }                                                           \
            if ( !stream.SerializeInteger( int32_value, min, max ) )    \
            {                                                           \
                return false;                                           \
            }                                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = int32_value;                                    \
                if ( int64_t(value) < int64_t(min) ||                   \
                     int64_t(value) > int64_t(max) )                    \
                {                                                       \
                    return false;                                       \
                }                                                       \
            }                                                           \
        } while (0)

    /**
        Serialize a 64 bit integer value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        The full 64 bit range is supported, and the minimal number of bits for [min,max] is used on the wire.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The 64 bit integer value to serialize in [min,max].
        @param min The minimum value.
        @param max The maximum value.
     */

    #define serialize_int64( stream, value, min, max )                  \
        do                                                              \
        {                                                               \
            serialize_assert( int64_t(min) < int64_t(max) );            \
            int64_t int64_value = 0;                                    \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                serialize_assert( int64_t(value) >= int64_t(min) );     \
                serialize_assert( int64_t(value) <= int64_t(max) );     \
                int64_value = (int64_t) ( value );                      \
            }                                                           \
            if ( !stream.SerializeInteger64( int64_value, min, max ) )  \
            {                                                           \
                return false;                                           \
            }                                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = int64_value;                                    \
                if ( int64_t(value) < int64_t(min) ||                   \
                     int64_t(value) > int64_t(max) )                    \
                {                                                       \
                    return false;                                       \
                }                                                       \
            }                                                           \
        } while (0)

    /**
        Serialize bits to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned integer value to serialize.
        @param bits The number of bits to serialize in [1,32].
     */

    #define serialize_bits( stream, value, bits )                       \
        do                                                              \
        {                                                               \
            serialize_assert( (bits) > 0 );                             \
            serialize_assert( (bits) <= 64 );                           \
            if ( (bits) <= 32 )                                         \
            {                                                           \
                uint32_t uint32_value = 0;                              \
                if ( Stream::IsWriting )                                \
                {                                                       \
                    uint32_value = (uint32_t) ( value );                \
                }                                                       \
                if ( !stream.SerializeBits( uint32_value, bits ) )      \
                {                                                       \
                    return false;                                       \
                }                                                       \
                if ( Stream::IsReading )                                \
                {                                                       \
                    value = uint32_value;                               \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                uint32_t hi = 0, lo = 0;                                \
                if ( Stream::IsWriting )                                \
                {                                                       \
                    lo = uint32_t( uint64_t(value) & 0xFFFFFFFF );      \
                    hi = uint32_t( uint64_t(value) >> 32 );             \
                }                                                       \
                if ( !stream.SerializeBits( lo, 32 ) )                  \
                {                                                       \
                    return false;                                       \
                }                                                       \
                if ( !stream.SerializeBits( hi, (bits) - 32 ) )         \
                {                                                       \
                    return false;                                       \
                }                                                       \
                if ( Stream::IsReading )                                \
                {                                                       \
                    value = ( uint64_t(hi) << 32 ) | lo;                \
                }                                                       \
            }                                                           \
        } while (0)


    /**
        Serialize a boolean value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The boolean value to serialize.
     */

    #define serialize_bool( stream, value )                             \
        do                                                              \
        {                                                               \
            uint32_t uint32_bool_value = 0;                             \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                uint32_bool_value = ( value ) ? 1 : 0;                  \
            }                                                           \
            serialize_bits( stream, uint32_bool_value, 1 );             \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = uint32_bool_value ? true : false;               \
            }                                                           \
        } while (0)

    template <typename Stream> bool serialize_float_internal( Stream & stream, float & value )
    {
        uint32_t int_value = 0;
        if ( Stream::IsWriting )
        {
            memcpy( (char*) &int_value, &value, 4 );
        }
        bool result = stream.SerializeBits( int_value, 32 );
        if ( Stream::IsReading )
        {
            memcpy( (char*) &value, &int_value, 4 );
        }
        return result;
    }

    /**
        Serialize floating point value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The float value to serialize.
     */

    #define serialize_float( stream, value )                                        \
        do                                                                          \
        {                                                                           \
            if ( !serialize::serialize_float_internal( stream, value ) )            \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    template <typename Stream> bool serialize_compressed_float_internal( Stream & stream, float & value, float min, float max, float res )
    {
        serialize_assert( min < max && res > 0 );

        const float delta = max - min;

        float values = delta / res;

        // clamp so the uint32_t cast below is defined even for pathological delta / res (the !>= form also catches NaN)
        if ( !( values >= 1.0f ) )
        {
            values = 1.0f;
        }
        else if ( values > 4294967040.0f )      // largest float below 2^32
        {
            values = 4294967040.0f;
        }

        const uint32_t maxIntegerValue = (uint32_t) ceil(values);

        const int bits = bits_required( 0, maxIntegerValue );
        
        uint32_t integerValue = 0;
        
        if ( Stream::IsWriting )
        {
            // clamp with the !>= / !<= form so a NaN value is forced into range instead of reaching the uint32 cast below
            float normalizedValue = (value - min) / delta;
            if ( !( normalizedValue >= 0.0f ) )
            {
                normalizedValue = 0.0f;
            }
            else if ( !( normalizedValue <= 1.0f ) )
            {
                normalizedValue = 1.0f;
            }
            integerValue = (uint32_t) floor( normalizedValue * maxIntegerValue + 0.5f );
        }

        if ( !stream.SerializeBits( integerValue, bits ) )
        {
            return false;
        }
        
        if ( Stream::IsReading )
        {
            if ( integerValue > maxIntegerValue )
            {
                return false;
            }
            const float normalizedValue = integerValue / float(maxIntegerValue);
            value = normalizedValue * delta + min;
        }

        return true;
    }

    /**
        Serialize compressed floating point value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The float value to serialize.
     */

    #define serialize_compressed_float(stream, value, min, max, res)                                \
    do                                                                                              \
    {                                                                                               \
        if ( !serialize::serialize_compressed_float_internal( stream, value, min, max, res) )       \
        {                                                                                           \
            return false;                                                                           \
        }                                                                                           \
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
        {
            tmp.double_value = value;
        }
        serialize_bits( stream, tmp.int_value, 64 );
        if ( Stream::IsReading )
        {
            value = tmp.double_value;
        }
        return true;
    }

    /**
        Serialize double precision floating point value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The double precision floating point value to serialize.
     */

    #define serialize_double( stream, value )                                       \
        do                                                                          \
        {                                                                           \
            if ( !serialize::serialize_double_internal( stream, value ) )           \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    template <typename Stream> bool serialize_bytes_internal( Stream & stream, uint8_t * data, int bytes )
    {
        return stream.SerializeBytes( data, bytes );
    }

    /**
        Serialize unsigned 8 bit integer (read/write/measure).
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 16 bit integer value.
     */

    #define serialize_uint8( stream, value ) serialize_bits( stream, value, 8 )

    /**
        Serialize unsigned 16 bit integer (read/write/measure).
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 16 bit integer value.
     */

    #define serialize_uint16( stream, value ) serialize_bits( stream, value, 16 )

    /**
        Serialize unsigned 32 bit integer (read/write/measure).
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 32 bit integer value.
     */

    #define serialize_uint32( stream, value ) serialize_bits( stream, value, 32 )

    /**
        Serialize unsigned 64 bit integer (read/write/measure).
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 64 bit integer value.
     */

    #define serialize_uint64( stream, value ) serialize_bits( stream, value, 64 )

    /**
        Serialize an array of bytes to the stream (read/write/measure).
        This is a helper macro to make unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param data Pointer to the data to be serialized.
        @param bytes The number of bytes to serialize.
     */

    #define serialize_bytes( stream, data, bytes )                                  \
        do                                                                          \
        {                                                                           \
            if ( !serialize::serialize_bytes_internal( stream, data, bytes ) )      \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    template <typename Stream> bool serialize_string_internal( Stream & stream, char * string, int buffer_size )
    {
        int length = 0;
        if ( Stream::IsWriting )
        {
            length = (int) strlen( string );
            serialize_assert( length < buffer_size );
        }
        serialize_int( stream, length, 0, buffer_size - 1 );
        serialize_bytes( stream, (uint8_t*)string, length );
        if ( Stream::IsReading )
        {
            string[length] = '\0';
        }
        return true;
    }

    // Wire format is 32 bits per character, so streams are compatible between platforms with 2 and 4 byte wchar_t.
    // Code points above 0xFFFF are not translated between UTF-16 and UTF-32 platforms: reading a value that doesn't
    // fit in the local wchar_t fails rather than truncating.

    template <typename Stream> bool serialize_wstring_internal( Stream & stream, wchar_t * string, int buffer_size )
    {
        int length = 0;
        if ( Stream::IsWriting )
        {
            length = (int) wcslen( string );
            serialize_assert( length < buffer_size );
        }

        serialize_int( stream, length, 0, buffer_size - 1 );
        for ( int i = 0; i < length; i++ )
        {
            uint32_t char_value = 0;
            if ( Stream::IsWriting )
            {
                char_value = (uint32_t) string[i];
            }
            serialize_bits( stream, char_value, 32 );
            if ( Stream::IsReading )
            {
                if ( sizeof(wchar_t) == 2 && char_value > 0xFFFF )
                {
                    return false;
                }
                string[i] = (wchar_t) char_value;
            }
        }
        if ( Stream::IsReading )
        {
            string[length] = L'\0';
        }
        return true;
    }

    /**
        Serialize a string to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param string The string to serialize write/measure. Pointer to buffer to be filled on read.
        @param buffer_size The size of the string buffer. String with terminating null character must fit into this buffer.
     */

    #define serialize_string( stream, string, buffer_size )                                 \
        do                                                                                  \
        {                                                                                   \
            if ( !serialize::serialize_string_internal( stream, string, buffer_size ) )     \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)
    
    #define serialize_wstring( stream, string, buffer_size )                                \
        do                                                                                  \
        {                                                                                   \
            if ( !serialize::serialize_wstring_internal( stream, string, buffer_size ) )    \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    /**
        Serialize an alignment to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
     */

    #define serialize_align( stream )                                                       \
        do                                                                                  \
        {                                                                                   \
            if ( !stream.SerializeAlign() )                                                 \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    /**
        Serialize an object to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param object The object to serialize. Must have a serialize method on it.
     */

    #define serialize_object( stream, object )                                              \
        do                                                                                  \
        {                                                                                   \
            if ( !( object ).Serialize( stream ) )                                          \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        }                                                                                   \
        while(0)

    template <typename Stream, typename T> bool serialize_int_relative_internal( Stream & stream, T previous, T & current )
    {
        uint32_t difference = 0;
        if ( Stream::IsWriting )
        {
            serialize_assert( previous < current );
            // subtract in the unsigned domain: current - previous overflows signed arithmetic when the gap is wider than 2^31
            difference = uint32_t( current ) - uint32_t( previous );
        }

        bool oneBit = false;
        if ( Stream::IsWriting )
        {
            oneBit = difference == 1;
        }
        serialize_bool( stream, oneBit );
        if ( oneBit )
        {
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + 1 );
            }
            return true;
        }

        bool twoBits = false;
        if ( Stream::IsWriting )
        {
            twoBits = difference <= 6;
        }
        serialize_bool( stream, twoBits );
        if ( twoBits )
        {
            serialize_int( stream, difference, 2, 6 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        bool fourBits = false;
        if ( Stream::IsWriting )
        {
            fourBits = difference <= 23;
        }
        serialize_bool( stream, fourBits );
        if ( fourBits )
        {
            serialize_int( stream, difference, 7, 23 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        bool eightBits = false;
        if ( Stream::IsWriting )
        {
            eightBits = difference <= 280;
        }
        serialize_bool( stream, eightBits );
        if ( eightBits )
        {
            serialize_int( stream, difference, 24, 280 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        bool twelveBits = false;
        if ( Stream::IsWriting )
        {
            twelveBits = difference <= 4377;
        }
        serialize_bool( stream, twelveBits );
        if ( twelveBits )
        {
            serialize_int( stream, difference, 281, 4377 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        bool sixteenBits = false;
        if ( Stream::IsWriting )
        {
            sixteenBits = difference <= 69914;
        }
        serialize_bool( stream, sixteenBits );
        if ( sixteenBits )
        {
            serialize_int( stream, difference, 4378, 69914 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        uint32_t value = current;
        serialize_bits( stream, value, 32 );
        if ( Stream::IsReading )
        {
            current = value;
            if ( current <= previous )
            {
                return false;
            }
        }

        return true;
    }

    /**
        Serialize an integer value relative to another (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param previous The previous integer value.
        @param current The current integer value.
     */

    #define serialize_int_relative( stream, previous, current )                             \
        do                                                                                  \
        {                                                                                   \
            if ( !serialize::serialize_int_relative_internal( stream, previous, current ) ) \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    // read macros corresponding to each serialize_*. useful when you want separate read and write functions.

    #define read_bits( stream, value, bits )                                                \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( (bits) > 0 );                                                 \
            serialize_assert( (bits) <= 64 );                                               \
            if ( (bits) <= 32 )                                                             \
            {                                                                               \
                uint32_t uint32_value;                                                      \
                if ( !stream.SerializeBits( uint32_value, bits ) )                          \
                {                                                                           \
                    return false;                                                           \
                }                                                                           \
                value = uint32_value;                                                       \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                uint32_t lo = 0;                                                            \
                uint32_t hi = 0;                                                            \
                if ( !stream.SerializeBits( lo, 32 ) )                                      \
                {                                                                           \
                    return false;                                                           \
                }                                                                           \
                if ( !stream.SerializeBits( hi, (bits) - 32 ) )                             \
                {                                                                           \
                    return false;                                                           \
                }                                                                           \
                value = ( uint64_t(hi) << 32 ) | lo;                                        \
            }                                                                               \
        } while (0)

    #define read_int( stream, value, min, max )                                             \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( (min) < (max) );                                              \
            int32_t int32_value = 0;                                                        \
            if ( !stream.SerializeInteger( int32_value, min, max ) )                        \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            value = int32_value;                                                            \
            if ( (value) < (min) || (value) > (max) )                                       \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_int64( stream, value, min, max )                                           \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( int64_t(min) < int64_t(max) );                                \
            int64_t int64_value = 0;                                                        \
            if ( !stream.SerializeInteger64( int64_value, min, max ) )                      \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            value = int64_value;                                                            \
            if ( (value) < (min) || (value) > (max) )                                       \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_bool( stream, value )      read_bits( stream, value, 1 )
    #define read_uint8( stream, value )     read_bits( stream, value, 8 )
    #define read_uint16( stream, value )    read_bits( stream, value, 16 )
    #define read_uint32( stream, value )    read_bits( stream, value, 32 )
    #define read_uint64( stream, value )    read_bits( stream, value, 64 )

    #define read_float                  serialize_float
    #define read_double                 serialize_double

    #define read_bytes( stream, data, bytes )                                               \
        do                                                                                  \
        {                                                                                   \
            uint8_t * data_ptr = (uint8_t*) ( data );                                       \
            if ( !stream.SerializeBytes( data_ptr, bytes ) )                                \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_string( stream, string, buffer_size )                                      \
        do                                                                                  \
        {                                                                                   \
            char * string_ptr = (char*) ( string );                                         \
            if ( !serialize_string_internal( stream, string_ptr, buffer_size ) )            \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_wstring( stream, string, buffer_size )                                      \
        do                                                                                   \
        {                                                                                    \
            wchar_t * string_ptr = (wchar_t*) ( string );                                    \
            if ( !serialize_wstring_internal( stream, string_ptr, buffer_size ) )            \
            {                                                                                \
                return false;                                                                \
            }                                                                                \
        } while (0)

    #define read_align                  serialize_align
    #define read_object                 serialize_object
    #define read_int_relative           serialize_int_relative

    // write macros corresponding to each serialize_*. useful when you want separate read and write functions.

    #define write_bits( stream, value, bits )                                               \
        do                                                                                  \
        {                                                                                   \
            uint64_t uint64_value = value;                                                  \
            if ( (bits) <= 32 )                                                             \
            {                                                                               \
                uint32_t uint32_value = (uint32_t) uint64_value;                            \
                stream.SerializeBits( uint32_value, bits );                                 \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                uint32_t lo = uint32_t( uint64_value & 0xFFFFFFFF );                        \
                uint32_t hi = uint32_t( uint64_value >> 32 );                               \
                stream.SerializeBits( lo, 32 );                                             \
                stream.SerializeBits( hi, (bits) - 32 );                                    \
            }                                                                               \
        } while (0)

    #define write_int( stream, value, min, max )                                            \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( (int32_t) ( min ) < (int32_t) ( max ) );                      \
            serialize_assert( (int32_t) ( value ) >= (int32_t) ( min ) );                   \
            serialize_assert( (int32_t) ( value ) <= (int32_t) ( max ) );                   \
            int32_t int32_value = (int32_t) ( value );                                      \
            stream.SerializeInteger( int32_value, min, max );                               \
        } while (0)

    #define write_int64( stream, value, min, max )                                          \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( int64_t( min ) < int64_t( max ) );                            \
            serialize_assert( int64_t( value ) >= int64_t( min ) );                         \
            serialize_assert( int64_t( value ) <= int64_t( max ) );                         \
            int64_t int64_value = (int64_t) ( value );                                      \
            stream.SerializeInteger64( int64_value, min, max );                             \
        } while (0)

    #define write_bool( stream, value )         write_bits( stream, value, 1 )
    #define write_uint8( stream, value )        write_bits( stream, value, 8 )
    #define write_uint16( stream, value )       write_bits( stream, value, 16 )
    #define write_uint32( stream, value )       write_bits( stream, value, 32 )
    #define write_uint64( stream, value )       write_bits( stream, value, 64 )

    #define write_float( stream, value )                                                    \
        do                                                                                  \
        {                                                                                   \
            float float_value = (float) ( value );                                          \
            uint32_t int_value;                                                             \
            memcpy( (char*) &int_value, &float_value, 4 );                                  \
            stream.SerializeBits( int_value, 32 );                                          \
        } while (0)

    #define write_double( stream, value )                                                   \
        do                                                                                  \
        {                                                                                   \
            double double_value = (double) ( value );                                       \
            uint64_t int64_value;                                                           \
            memcpy( (char*) &int64_value, &double_value, 8 );                               \
            write_bits( stream, int64_value, 64 );                                          \
        } while (0)

    #define write_bytes( stream, data, bytes )                                              \
        do                                                                                  \
        {                                                                                   \
            const uint8_t * data_ptr = (const uint8_t*) ( data );                           \
            stream.SerializeBytes( data_ptr, bytes );                                       \
        } while (0)

    #define write_string( stream, string, buffer_size )                                     \
        do                                                                                  \
        {                                                                                   \
            int length = (int) strlen( string );                                            \
            serialize_assert( length < (buffer_size) );                                     \
            write_int( stream, length, 0, (buffer_size) - 1 );                              \
            write_bytes( stream, (uint8_t*) ( string ), length );                           \
        } while (0)

    #define write_wstring( stream, string, buffer_size )                                    \
        do                                                                                  \
        {                                                                                   \
            int length = (int) wcslen( string );                                            \
            serialize_assert( length < (buffer_size) );                                     \
            write_int( stream, length, 0, (buffer_size) - 1 );                              \
            for ( int i = 0; i < length; i++ )                                              \
            {                                                                               \
                uint32_t wchar_value = (uint32_t) ( string )[i];                            \
                write_bits( stream, wchar_value, 32 );                                      \
            }                                                                               \
        } while (0)

    #define write_align( stream )                                                           \
        do                                                                                  \
        {                                                                                   \
            stream.SerializeAlign();                                                        \
        } while (0)

    #define write_object( stream, object )                                                  \
        do                                                                                  \
        {                                                                                   \
            ( object ).Serialize( stream );                                                 \
        }                                                                                   \
        while(0)

    #define write_int_relative( stream, previous, current )                                 \
        do                                                                                  \
        {                                                                                   \
            int current_value = (int) ( current );                                          \
            serialize::serialize_int_relative_internal( stream, previous, current_value );  \
        } while (0)
}

inline void serialize_copy_string( char * dest, const char * source, size_t dest_size )
{
    serialize_assert( dest );
    serialize_assert( source );
    serialize_assert( dest_size >= 1 );
    memset( dest, 0, dest_size );
    for ( size_t i = 0; i < dest_size - 1; i++ )
    {
        if ( source[i] == '\0' )
            break;
        dest[i] = source[i];
    }
}

inline void serialize_copy_wstring( wchar_t * dest, const wchar_t * source, size_t dest_size )
{
    serialize_assert( dest );
    serialize_assert( source );
    serialize_assert( dest_size >= 1 );
    memset( dest, 0, dest_size * sizeof(wchar_t) );
    for ( size_t i = 0; i < dest_size - 1; i++ )
    {
        if ( source[i] == L'\0' )
            break;
        dest[i] = source[i];
    }
}

#if SERIALIZE_ENABLE_TESTS

#include <stdio.h>      // printf
#include <stdlib.h>     // exit

inline void SerializeCheckHandler( const char * condition,
                                   const char * function,
                                   const char * file,
                                   int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifndef NDEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define serialize_check( condition )                                                    \
do                                                                                      \
{                                                                                       \
    if ( !(condition) )                                                                 \
    {                                                                                   \
        SerializeCheckHandler( #condition, __FUNCTION__, __FILE__, __LINE__ );          \
    }                                                                                   \
} while(0)

inline void test_endian()
{
    uint32_t value = 0x11223344;

    const char * bytes = (const char*) &value;

#if SERIALIZE_LITTLE_ENDIAN

    serialize_check( bytes[0] == 0x44 );
    serialize_check( bytes[1] == 0x33 );
    serialize_check( bytes[2] == 0x22 );
    serialize_check( bytes[3] == 0x11 );

#else // #if SERIALIZE_LITTLE_ENDIAN

    serialize_check( bytes[3] == 0x44 );
    serialize_check( bytes[2] == 0x33 );
    serialize_check( bytes[1] == 0x22 );
    serialize_check( bytes[0] == 0x11 );

#endif // #if SERIALIZE_LITTLE_ENDIAN
}

inline void test_bitpacker()
{
    const int BufferSize = 256;

    uint8_t buffer[BufferSize];

    serialize::BitWriter writer( buffer, BufferSize );

    serialize_check( writer.GetData() == buffer );
    serialize_check( writer.GetBitsWritten() == 0 );
    serialize_check( writer.GetBytesWritten() == 0 );
    serialize_check( writer.GetBitsAvailable() == BufferSize * 8 );

    writer.WriteBits( 0, 1 );
    writer.WriteBits( 1, 1 );
    writer.WriteBits( 10, 8 );
    writer.WriteBits( 255, 8 );
    writer.WriteBits( 1000, 10 );
    writer.WriteBits( 50000, 16 );
    writer.WriteBits( 9999999, 32 );
    writer.FlushBits();

    const int bitsWritten = 1 + 1 + 8 + 8 + 10 + 16 + 32;

    serialize_check( writer.GetBytesWritten() == 10 );
    serialize_check( writer.GetBitsWritten() == bitsWritten );
    serialize_check( writer.GetBitsAvailable() == BufferSize * 8 - bitsWritten );

    const int bytesWritten = writer.GetBytesWritten();

    serialize_check( bytesWritten == 10 );

    memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );

    serialize::BitReader reader( buffer, bytesWritten );

    serialize_check( reader.GetBitsRead() == 0 );
    serialize_check( reader.GetBitsRemaining() == bytesWritten * 8 );

    uint32_t a = reader.ReadBits( 1 );
    uint32_t b = reader.ReadBits( 1 );
    uint32_t c = reader.ReadBits( 8 );
    uint32_t d = reader.ReadBits( 8 );
    uint32_t e = reader.ReadBits( 10 );
    uint32_t f = reader.ReadBits( 16 );
    uint32_t g = reader.ReadBits( 32 );

    serialize_check( a == 0 );
    serialize_check( b == 1 );
    serialize_check( c == 10 );
    serialize_check( d == 255 );
    serialize_check( e == 1000 );
    serialize_check( f == 50000 );
    serialize_check( g == 9999999 );

    serialize_check( reader.GetBitsRead() == bitsWritten );
    serialize_check( reader.GetBitsRemaining() == bytesWritten * 8 - bitsWritten );
}

inline void test_bits_required()
{
    serialize_check( serialize::bits_required( 0, 0 ) == 0 );
    serialize_check( serialize::bits_required( 0, 1 ) == 1 );
    serialize_check( serialize::bits_required( 0, 2 ) == 2 );
    serialize_check( serialize::bits_required( 0, 3 ) == 2 );
    serialize_check( serialize::bits_required( 0, 4 ) == 3 );
    serialize_check( serialize::bits_required( 0, 5 ) == 3 );
    serialize_check( serialize::bits_required( 0, 6 ) == 3 );
    serialize_check( serialize::bits_required( 0, 7 ) == 3 );
    serialize_check( serialize::bits_required( 0, 8 ) == 4 );
    serialize_check( serialize::bits_required( 0, 255 ) == 8 );
    serialize_check( serialize::bits_required( 0, 65535 ) == 16 );
    serialize_check( serialize::bits_required( 0, 4294967295 ) == 32 );
}

inline void test_bits_required64()
{
    serialize_check( serialize::bits_required64( 0, 0 ) == 0 );
    serialize_check( serialize::bits_required64( 0, 1 ) == 1 );
    serialize_check( serialize::bits_required64( 0, 255 ) == 8 );
    serialize_check( serialize::bits_required64( 0, 4294967295ULL ) == 32 );
    serialize_check( serialize::bits_required64( 0, 4294967296ULL ) == 33 );
    serialize_check( serialize::bits_required64( 0, ( 1ULL << 40 ) ) == 41 );
    serialize_check( serialize::bits_required64( 0, 0xFFFFFFFFFFFFFFFFULL ) == 64 );
    serialize_check( serialize::bits_required64( uint64_t(INT64_MIN), uint64_t(INT64_MAX) ) == 64 );
    serialize_check( serialize::bits_required64( uint64_t(-5000000000LL), uint64_t(+5000000000LL) ) == 34 );
}

inline void test_zigzag()
{
    serialize_check( serialize::signed_to_unsigned( 0 ) == 0 );
    serialize_check( serialize::signed_to_unsigned( -1 ) == 1 );
    serialize_check( serialize::signed_to_unsigned( +1 ) == 2 );
    serialize_check( serialize::signed_to_unsigned( -2 ) == 3 );
    serialize_check( serialize::signed_to_unsigned( +2 ) == 4 );
    serialize_check( serialize::signed_to_unsigned( INT32_MAX ) == 0xFFFFFFFE );
    serialize_check( serialize::signed_to_unsigned( INT32_MIN ) == 0xFFFFFFFF );

    serialize_check( serialize::unsigned_to_signed( 0 ) == 0 );
    serialize_check( serialize::unsigned_to_signed( 1 ) == -1 );
    serialize_check( serialize::unsigned_to_signed( 2 ) == +1 );
    serialize_check( serialize::unsigned_to_signed( 3 ) == -2 );
    serialize_check( serialize::unsigned_to_signed( 4 ) == +2 );
    serialize_check( serialize::unsigned_to_signed( 0xFFFFFFFE ) == INT32_MAX );
    serialize_check( serialize::unsigned_to_signed( 0xFFFFFFFF ) == INT32_MIN );

    const int32_t values[] = { 0, -1, +1, -2, +2, 12345, -12345, INT32_MAX, INT32_MIN };

    for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
    {
        serialize_check( serialize::unsigned_to_signed( serialize::signed_to_unsigned( values[i] ) ) == values[i] );
    }
}

const int MaxItems = 11;

struct TestData
{
    TestData()
    {
        memset( this, 0, sizeof( TestData ) );
    }

    int a,b,c;
    uint32_t d : 8;
    uint32_t e : 8;
    uint32_t f : 8;
    bool g;
    uint32_t v32;
    uint64_t v64;
    int numItems;
    int items[MaxItems];
    float float_value;
    float compressed_float_value;
    double double_value;
    uint8_t uint8_value;
    uint16_t uint16_value;
    uint32_t uint32_value;
    uint64_t uint64_value;
    int int_relative;
    int64_t int64_full;
    int64_t int64_range;
    uint8_t bytes[17];
    char string[256];
    wchar_t wstring[256];
};

struct TestContext
{
    int min;
    int max;
};

struct TestObject
{
    TestData data;

    void Init()
    {
        data.a = 1;
        data.b = -2;
        data.c = 150;
        data.d = 55;
        data.e = 255;
        data.f = 127;
        data.g = true;

        data.numItems = MaxItems / 2;
        for ( int i = 0; i < data.numItems; ++i )
            data.items[i] = i + 10;

        data.compressed_float_value = 2.13f;
        data.float_value = 3.1415926f;
        data.double_value = 1 / 3.0;
        data.uint8_value = 123;
        data.uint16_value = 0x1234;
        data.uint32_value = 0x12345678;
        data.uint64_value = 0x1234567898765432L;
        data.int_relative = 5;
        data.int64_full = -123456789012345LL;
        data.int64_range = 4123456789LL;

        for ( int i = 0; i < (int) sizeof( data.bytes ); ++i )
            data.bytes[i] = (uint8_t) ( i + 5 ) * 13;

        serialize_copy_string( data.string, "hello world!", sizeof(data.string) - 1 );

        serialize_copy_wstring( data.wstring, L"привіт, світ!", sizeof(data.wstring) / sizeof(wchar_t) - 1 );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        const TestContext & context = *(const TestContext*) stream.GetContext();

        serialize_int( stream, data.a, context.min, context.max );
        serialize_int( stream, data.b, context.min, context.max );

        serialize_int( stream, data.c, -100, 10000 );

        serialize_bits( stream, data.d, 6 );
        serialize_bits( stream, data.e, 8 );
        serialize_bits( stream, data.f, 7 );

        serialize_align( stream );

        serialize_bool( stream, data.g );

        serialize_int( stream, data.numItems, 0, MaxItems - 1 );
        for ( int i = 0; i < data.numItems; ++i )
            serialize_bits( stream, data.items[i], 8 );

        serialize_float( stream, data.float_value );

        serialize_compressed_float( stream, data.compressed_float_value, 0, 10, 0.01 );

        serialize_double( stream, data.double_value );

        serialize_uint8( stream, data.uint8_value );
        serialize_uint16( stream, data.uint16_value );
        serialize_uint32( stream, data.uint32_value );
        serialize_uint64( stream, data.uint64_value );

        serialize_int_relative( stream, data.a, data.int_relative );

        serialize_int64( stream, data.int64_full, INT64_MIN, INT64_MAX );
        serialize_int64( stream, data.int64_range, -5000000000LL, +5000000000LL );

        serialize_bytes( stream, data.bytes, sizeof( data.bytes ) );

        serialize_string( stream, data.string, sizeof( data.string ) );
        serialize_wstring( stream, data.wstring, sizeof( data.wstring ) / sizeof( wchar_t ) );

        return true;
    }

    bool operator == ( const TestObject & other ) const
    {
        return memcmp( &data, &other.data, sizeof( TestData ) ) == 0;
    }

    bool operator != ( const TestObject & other ) const
    {
        return ! ( *this == other );
    }
};

inline void test_serialize()
{
    const int BufferSize = 1024;

    uint8_t buffer[BufferSize];

    TestContext context;
    context.min = -10;
    context.max = +10;

    serialize::WriteStream writeStream( buffer, BufferSize );

    TestObject writeObject;
    writeObject.Init();
    writeStream.SetContext( &context );
    writeObject.Serialize( writeStream );
    writeStream.Flush();

    const int bytesWritten = writeStream.GetBytesProcessed();

    memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );

    TestObject readObject;
    serialize::ReadStream readStream( buffer, bytesWritten );
    readStream.SetContext( &context );
    readObject.Serialize( readStream );

    serialize_check( readObject == writeObject );
}

bool ReadFunction( serialize::ReadStream & readStream )
{
    // IMPORTANT: You wouldn't normally write a read function like this, but I'm just checking each value as it's read in
    // Note that the only thing the read function has to have is to return bool: true on success, false on failing to read.
    // This is important because protects you from maliciously crafted packets.

    {
        uint32_t value;
        read_bits( readStream, value, 4 );
        serialize_check( value == 13 );
    }

    {
        bool value;
        read_bool( readStream, value );
        serialize_check( value == true );
    }

    {
        uint8_t value;
        read_uint8( readStream, value );
        serialize_check( value == 255 );
    }

    {
        uint16_t value;
        read_uint16( readStream, value );
        serialize_check( value == 65535 );
    }

    {
        uint32_t value;
        read_uint32( readStream, value );
        serialize_check( value == 0xFFFFFFFF );
    }

    {
        uint64_t value;
        read_uint64( readStream, value );
        serialize_check( value == 0xFFFFFFFFFFFFFFFFULL );      // i am very full
    }

    {
        int value;
        read_int( readStream, value, 10, 90 );
        serialize_check( value == 55 );
    }

    {
        int64_t value;
        read_int64( readStream, value, -60000000000LL, 60000000000LL );
        serialize_check( value == -50000000001LL );
    }

    {
        float value;
        read_float( readStream, value );
        serialize_check( value == 100.0f );
    }

    {
        double value;
        read_double( readStream, value );
        serialize_check( value == 1000000000.0 );
    }

    {
        char value[5];
        read_bytes( readStream, value, 5 );
        serialize_check( value[0] == 1 );
        serialize_check( value[1] == 2 );
        serialize_check( value[2] == 3 );
        serialize_check( value[3] == 4 );
        serialize_check( value[4] == 5 );
    }

    {
        char string[10];
        read_string( readStream, string, 10 );
        serialize_check( string[0] == 'h' );
        serialize_check( string[1] == 'e' );
        serialize_check( string[2] == 'l' );
        serialize_check( string[3] == 'l' );
        serialize_check( string[4] == 'o' );
        serialize_check( string[5] == '\0' );
    }

    {
        wchar_t wstring[20];
        read_wstring( readStream, wstring, 20 );
        serialize_check( wstring[0] == L'п' );
        serialize_check( wstring[1] == L'р' );
        serialize_check( wstring[2] == L'и' );
        serialize_check( wstring[3] == L'в' );
        serialize_check( wstring[4] == L'і' );
        serialize_check( wstring[5] == L'т' );
    }

    read_align( readStream );

    TestContext context;
    context.min = -10;
    context.max = +10;

    readStream.SetContext( &context );
    {
        TestObject expectedObject;
        expectedObject.Init();

        TestObject readObject;

        read_object( readStream, readObject );

        serialize_check( readObject == expectedObject );
    }

    {
        int value;
        read_int_relative( readStream, 100, value );
        serialize_check( value == 105 );
    }

    return true;
}

inline void test_read_write()
{
    const int BufferSize = 10 * 1024;

    uint8_t buffer[BufferSize];

    int bytesWritten = 0;

    // write to the buffer
    {
        serialize::WriteStream writeStream;
        writeStream.Initialize( buffer, BufferSize );

        write_bits( writeStream, 13, 4 );
        write_bool( writeStream, true );
        write_uint8( writeStream, 255 );
        write_uint16( writeStream, 65535 );
        write_uint32( writeStream, 0xFFFFFFFF );
        write_uint64( writeStream, 0xFFFFFFFFFFFFFFFFULL );
        write_int( writeStream, 55, 10, 90 );
        write_int64( writeStream, -50000000001LL, -60000000000LL, 60000000000LL );
        write_float( writeStream, 100.0f );
        write_double( writeStream, 1000000000.0f );

        char data[5] = { 1, 2, 3, 4, 5 };
        write_bytes( writeStream, data, 5 );

        const char * string = "hello";
        write_string( writeStream, string, 10 );

        const wchar_t * wstring = L"привіт";
        write_wstring( writeStream, wstring, 20 );

        write_align( writeStream );

        TestContext context;
        context.min = -10;
        context.max = +10;

        writeStream.SetContext( &context );

        TestObject object;
        object.Init();

        write_object( writeStream, object );

        write_int_relative( writeStream, 100, 105 );

        writeStream.Flush();

        bytesWritten = writeStream.GetBytesProcessed();

        memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );
    }

    // read from the buffer
    {
        serialize::ReadStream readStream;
        readStream.Initialize( buffer, bytesWritten );
        serialize_check( ReadFunction( readStream ) );
    }
}

inline void test_serialize_integer_validation()
{
    // bits_required(0,5) is 3 bits, so a malicious packet can encode 6 or 7. reads must reject values above max.
    uint8_t buffer[4] = { 0 };

    serialize::WriteStream writeStream( buffer, sizeof(buffer) );
    uint32_t out_of_range = 7;
    writeStream.SerializeBits( out_of_range, 3 );
    writeStream.Flush();

    serialize::ReadStream readStream( buffer, sizeof(buffer) );
    int32_t value = 0;
    serialize_check( readStream.SerializeInteger( value, 0, 5 ) == false );
}

inline void test_serialize_integer_full_range()
{
    // ranges wider than 2^31 overflow if [min,max] arithmetic is done signed (undefined behavior)
    const int32_t values[] = { INT32_MIN, INT32_MIN + 1, -1, 0, +1, INT32_MAX - 1, INT32_MAX };

    for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        serialize_check( writeStream.SerializeInteger( values[i], INT32_MIN, INT32_MAX ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        int32_t value = 0;
        serialize_check( readStream.SerializeInteger( value, INT32_MIN, INT32_MAX ) == true );
        serialize_check( value == values[i] );
    }

    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        serialize_check( writeStream.SerializeInteger( 1000000000, -2000000000, 2000000000 ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        int32_t value = 0;
        serialize_check( readStream.SerializeInteger( value, -2000000000, 2000000000 ) == true );
        serialize_check( value == 1000000000 );
    }
}

inline void test_serialize_int64_full_range()
{
    // ranges wider than 2^63 overflow if [min,max] arithmetic is done signed (undefined behavior)
    {
        const int64_t values[] = { INT64_MIN, INT64_MIN + 1, -1, 0, +1, INT64_MAX - 1, INT64_MAX };

        for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
        {
            uint8_t buffer[16] = { 0 };

            serialize::WriteStream writeStream( buffer, sizeof(buffer) );
            serialize_check( writeStream.SerializeInteger64( values[i], INT64_MIN, INT64_MAX ) == true );
            writeStream.Flush();

            serialize::ReadStream readStream( buffer, sizeof(buffer) );
            int64_t value = 0;
            serialize_check( readStream.SerializeInteger64( value, INT64_MIN, INT64_MAX ) == true );
            serialize_check( value == values[i] );
        }
    }

    // ranges spanning more than 32 bits use the two dword path
    {
        const int64_t min = -5000000000LL;
        const int64_t max = +5000000000LL;
        const int64_t values[] = { min, min + 1, -1, 0, +1, 4123456789LL, max - 1, max };

        for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
        {
            uint8_t buffer[16] = { 0 };

            serialize::WriteStream writeStream( buffer, sizeof(buffer) );
            serialize_check( writeStream.SerializeInteger64( values[i], min, max ) == true );
            writeStream.Flush();

            serialize::ReadStream readStream( buffer, sizeof(buffer) );
            int64_t value = 0;
            serialize_check( readStream.SerializeInteger64( value, min, max ) == true );
            serialize_check( value == values[i] );
        }
    }

    // small ranges use the single dword path and the minimal number of bits
    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        serialize_check( writeStream.SerializeInteger64( 55, -100, +100 ) == true );
        writeStream.Flush();

        serialize_check( writeStream.GetBitsProcessed() == 8 );        // bits_required64(-100,100) == 8, same as the 32 bit path

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        int64_t value = 0;
        serialize_check( readStream.SerializeInteger64( value, -100, +100 ) == true );
        serialize_check( value == 55 );
    }
}

inline void test_serialize_int64_validation()
{
    // a malicious packet can smuggle an out of range value into the bit headroom of the two dword path. reads must reject it.
    {
        uint8_t buffer[16] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        const uint64_t out_of_range = ( 1ULL << 34 ) + 5;               // range [0, 2^34] is 35 bits, so values above 2^34 fit in the headroom
        uint32_t lo = uint32_t( out_of_range & 0xFFFFFFFF );
        uint32_t hi = uint32_t( out_of_range >> 32 );
        writeStream.SerializeBits( lo, 32 );
        writeStream.SerializeBits( hi, 3 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        int64_t value = 0;
        serialize_check( readStream.SerializeInteger64( value, 0, int64_t( 1ULL << 34 ) ) == false );
    }

    // reads past the end of the buffer must fail cleanly
    {
        uint8_t buffer[4] = { 0 };

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        int64_t value = 0;
        serialize_check( readStream.SerializeInteger64( value, INT64_MIN, INT64_MAX ) == false );
    }
}

inline void test_serialize_bytes_validation()
{
    // negative and huge byte counts must be rejected, not overflow the bounds check in bits
    uint8_t buffer[16] = { 0 };
    uint8_t data[16];

    {
        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        serialize_check( readStream.SerializeBytes( data, -1 ) == false );
    }

    {
        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        serialize_check( readStream.SerializeBytes( data, 1 << 29 ) == false );
    }
}

inline void test_int_relative_validation()
{
    // the 32 bit fallback must reject values that violate the previous < current contract
    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        uint32_t six_false_bools = 0;
        writeStream.SerializeBits( six_false_bools, 6 );
        uint32_t bad_current = 50;
        writeStream.SerializeBits( bad_current, 32 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        int previous = 100;
        int current = 0;
        serialize_check( serialize::serialize_int_relative_internal( readStream, previous, current ) == false );
    }

    // a legitimate fallback round trip must still succeed
    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        int previous = 100;
        int written = 100000;
        serialize_check( serialize::serialize_int_relative_internal( writeStream, previous, written ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        int current = 0;
        serialize_check( serialize::serialize_int_relative_internal( readStream, previous, current ) == true );
        serialize_check( current == written );
    }

    // gaps wider than 2^31 overflow if the difference is computed in signed arithmetic (undefined behavior)
    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        int previous = -1000;
        int written = INT32_MAX;
        serialize_check( serialize::serialize_int_relative_internal( writeStream, previous, written ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        int current = 0;
        serialize_check( serialize::serialize_int_relative_internal( readStream, previous, current ) == true );
        serialize_check( current == written );
    }

    // read side reconstructs current = previous + difference; a large previous overflows signed arithmetic.
    // this must wrap in the unsigned domain rather than invoke undefined behavior.
    {
        // difference of 1 exercises the oneBit branch, difference of 5 exercises a bucket branch
        const int differences[] = { 1, 5 };

        for ( int d = 0; d < (int) ( sizeof(differences) / sizeof(differences[0]) ); d++ )
        {
            uint8_t buffer[8] = { 0 };

            serialize::WriteStream writeStream( buffer, sizeof(buffer) );
            int prevWrite = 10;
            int curWrite = prevWrite + differences[d];
            serialize_check( serialize::serialize_int_relative_internal( writeStream, prevWrite, curWrite ) == true );
            writeStream.Flush();

            serialize::ReadStream readStream( buffer, sizeof(buffer) );
            int previous = INT32_MAX;                        // previous + difference exceeds INT32_MAX
            int current = 0;
            serialize_check( serialize::serialize_int_relative_internal( readStream, previous, current ) == true );
            serialize_check( current == int32_t( uint32_t( INT32_MAX ) + uint32_t( differences[d] ) ) );
        }
    }
}

inline void test_compressed_float_validation()
{
    // a malicious packet can encode integer values above maxIntegerValue in the bit headroom. reads must reject them.
    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        uint32_t out_of_range = 1023;                       // maxIntegerValue is 1000 for [0,10] at res 0.01 -> 10 bits
        writeStream.SerializeBits( out_of_range, 10 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        float value = 0.0f;
        serialize_check( serialize::serialize_compressed_float_internal( readStream, value, 0.0f, 10.0f, 0.01f ) == false );
    }

    // huge delta / res ratios must not overflow the uint32 quantization range (undefined behavior)
    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        float written = 5000000000.0f;
        serialize_check( serialize::serialize_compressed_float_internal( writeStream, written, 0.0f, 10000000000.0f, 1.0f ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        float value = 0.0f;
        serialize_check( serialize::serialize_compressed_float_internal( readStream, value, 0.0f, 10000000000.0f, 1.0f ) == true );
        serialize_check( fabs( value - written ) <= 4096.0f );
    }

    // a NaN value must not reach the uint32 cast (clamp comparisons are all false for NaN)
    {
        uint8_t buffer[8] = { 0 };

        serialize::WriteStream writeStream( buffer, sizeof(buffer) );
        uint32_t nan_bits = 0x7fc00000;                 // quiet NaN bit pattern, built without the NAN macro (finite-math builds reject it)
        float written = 0.0f;
        memcpy( &written, &nan_bits, 4 );
        serialize_check( serialize::serialize_compressed_float_internal( writeStream, written, 0.0f, 10.0f, 0.01f ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, sizeof(buffer) );
        float value = -1.0f;
        serialize_check( serialize::serialize_compressed_float_internal( readStream, value, 0.0f, 10.0f, 0.01f ) == true );
        serialize_check( value >= 0.0f && value <= 10.0f );      // NaN clamps to the low end of the range
    }
}

// Golden wire format test. The exact bytes produced by the serializer are pinned down here and must never change.
// If this test fails, the wire format has changed and previously written data no longer decodes: a breaking change.
// The values below are chosen so every platform quantizes identically (see the compressed float: 5.0 in [0,10]
// normalizes to exactly 0.5, so fp contraction differences between compilers cannot shift the quantized integer).

struct GoldenWireData
{
    uint32_t bits4;
    uint32_t bits11;
    uint32_t bits24;
    uint32_t bits32;
    int32_t int_small;
    int32_t int_full;
    bool flag;
    float float_value;
    float compressed_float_value;
    double double_value;
    uint8_t uint8_value;
    uint16_t uint16_value;
    uint32_t uint32_value;
    uint64_t uint64_value;
    int relative_near;
    int relative_far;
    uint8_t bytes[7];
    char string[16];
    wchar_t wstring[8];
};

inline void GoldenWireInit( GoldenWireData & data )
{
    memset( (void*) &data, 0, sizeof( GoldenWireData ) );
    data.bits4 = 13;
    data.bits11 = 1445;
    data.bits24 = 11259375;
    data.bits32 = 0xDEADBEEF;
    data.int_small = -37;
    data.int_full = -123456789;
    data.flag = true;
    data.float_value = 3.1415926f;
    data.compressed_float_value = 5.0f;
    data.double_value = 1.0 / 3.0;
    data.uint8_value = 0x7F;
    data.uint16_value = 0x1234;
    data.uint32_value = 0x12345678;
    data.uint64_value = 0x123456789ABCDEF0ULL;
    data.relative_near = 101;                   // difference of 1 from the base: exercises the one bit branch
    data.relative_far = 2100;                   // difference of 2000 from the base: exercises the twelve bit bucket
    const uint8_t golden_byte_data[7] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0x01 };
    memcpy( data.bytes, golden_byte_data, sizeof( golden_byte_data ) );
    serialize_copy_string( data.string, "golden", sizeof( data.string ) );
    // built from explicit code points so the source file encoding can never change the golden bytes
    const wchar_t golden_wide_string[4] = { 0x043C, 0x0438, 0x0440, 0 };            // cyrillic, BMP only
    serialize_copy_wstring( data.wstring, golden_wide_string, sizeof( data.wstring ) / sizeof( wchar_t ) );
}

template <typename Stream> bool GoldenWireSerialize( Stream & stream, GoldenWireData & data )
{
    const int relative_base = 100;
    serialize_bits( stream, data.bits4, 4 );
    serialize_bits( stream, data.bits11, 11 );
    serialize_bits( stream, data.bits24, 24 );
    serialize_bits( stream, data.bits32, 32 );
    serialize_int( stream, data.int_small, -100, +100 );
    serialize_int( stream, data.int_full, INT32_MIN, INT32_MAX );
    serialize_bool( stream, data.flag );
    serialize_float( stream, data.float_value );
    serialize_compressed_float( stream, data.compressed_float_value, 0.0f, 10.0f, 0.01f );
    serialize_double( stream, data.double_value );
    serialize_uint8( stream, data.uint8_value );
    serialize_uint16( stream, data.uint16_value );
    serialize_uint32( stream, data.uint32_value );
    serialize_uint64( stream, data.uint64_value );
    serialize_int_relative( stream, relative_base, data.relative_near );
    serialize_int_relative( stream, relative_base, data.relative_far );
    serialize_align( stream );
    serialize_bytes( stream, data.bytes, (int) sizeof( data.bytes ) );
    serialize_string( stream, data.string, (int) sizeof( data.string ) );
    serialize_wstring( stream, data.wstring, (int) ( sizeof( data.wstring ) / sizeof( wchar_t ) ) );
    return true;
}

static const uint8_t golden_wire_bytes[] =
{
    0x5D, 0xDA, 0xF7, 0xE6, 0xD5, 0x77, 0xDF, 0x56, 0xEF, 0x9F, 0x75, 0x19,
    0x52, 0xBC, 0xDA, 0x0F, 0x49, 0x40, 0xF4, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0xFF, 0xFC, 0xD1, 0x48, 0xE0, 0x59, 0xD1, 0x48, 0xC0, 0x7B,
    0xF3, 0x6A, 0xE2, 0x59, 0xD1, 0x48, 0x84, 0xB7, 0x06, 0xDE, 0xAD, 0xBE,
    0xEF, 0xCA, 0xFE, 0x01, 0x06, 0x67, 0x6F, 0x6C, 0x64, 0x65, 0x6E, 0xE3,
    0x21, 0x00, 0x00, 0xC0, 0x21, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00
};

inline void test_golden_wire_format()
{
    // write side: serializing the golden values must produce exactly the golden bytes
    {
        uint8_t buffer[256];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        GoldenWireData data;
        GoldenWireInit( data );
        serialize_check( GoldenWireSerialize( stream, data ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( golden_wire_bytes ) );
        serialize_check( memcmp( buffer, golden_wire_bytes, sizeof( golden_wire_bytes ) ) == 0 );
    }

    // read side: the golden bytes must decode to the expected values, on every platform, forever
    {
        uint8_t buffer[256];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, golden_wire_bytes, sizeof( golden_wire_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( golden_wire_bytes ) );
        GoldenWireData data;
        memset( (void*) &data, 0, sizeof( GoldenWireData ) );
        serialize_check( GoldenWireSerialize( stream, data ) == true );

        GoldenWireData expected;
        GoldenWireInit( expected );
        serialize_check( data.bits4 == expected.bits4 );
        serialize_check( data.bits11 == expected.bits11 );
        serialize_check( data.bits24 == expected.bits24 );
        serialize_check( data.bits32 == expected.bits32 );
        serialize_check( data.int_small == expected.int_small );
        serialize_check( data.int_full == expected.int_full );
        serialize_check( data.flag == expected.flag );
        serialize_check( data.float_value == expected.float_value );
        serialize_check( fabs( data.compressed_float_value - expected.compressed_float_value ) <= 0.01f );
        serialize_check( data.double_value == expected.double_value );
        serialize_check( data.uint8_value == expected.uint8_value );
        serialize_check( data.uint16_value == expected.uint16_value );
        serialize_check( data.uint32_value == expected.uint32_value );
        serialize_check( data.uint64_value == expected.uint64_value );
        serialize_check( data.relative_near == expected.relative_near );
        serialize_check( data.relative_far == expected.relative_far );
        serialize_check( memcmp( data.bytes, expected.bytes, sizeof( data.bytes ) ) == 0 );
        serialize_check( strcmp( data.string, expected.string ) == 0 );
        serialize_check( wcscmp( data.wstring, expected.wstring ) == 0 );
    }
}

inline void test_unaligned_writer()
{
    // the bit writer stores each dword with memcpy, so the write buffer does not need 4 byte alignment.
    // exercise every offset within a dword, covering the WriteBits, WriteBytes and FlushBits store paths.

    alignas( 4 ) uint8_t storage[256 + 4];

    for ( int offset = 0; offset < 4; offset++ )
    {
        memset( storage, 0, sizeof( storage ) );

        uint8_t * buffer = storage + offset;

        uint8_t data[13];
        for ( int i = 0; i < (int) sizeof( data ); i++ )
            data[i] = (uint8_t) ( i * 47 + offset );

        serialize::WriteStream writeStream( buffer, 256 );
        writeStream.SerializeBits( 0x12345678, 32 );
        writeStream.SerializeBits( 123, 7 );
        writeStream.SerializeBytes( data, (int) sizeof( data ) );
        writeStream.SerializeBits( 0xDEADBEEF, 32 );
        writeStream.Flush();

        const int bytesWritten = writeStream.GetBytesProcessed();

        serialize::ReadStream readStream( buffer, bytesWritten );
        uint32_t a = 0;
        serialize_check( readStream.SerializeBits( a, 32 ) == true );
        serialize_check( a == 0x12345678 );
        uint32_t b = 0;
        serialize_check( readStream.SerializeBits( b, 7 ) == true );
        serialize_check( b == 123 );
        uint8_t read_data[13];
        memset( read_data, 0, sizeof( read_data ) );
        serialize_check( readStream.SerializeBytes( read_data, (int) sizeof( read_data ) ) == true );
        serialize_check( memcmp( read_data, data, sizeof( data ) ) == 0 );
        uint32_t c = 0;
        serialize_check( readStream.SerializeBits( c, 32 ) == true );
        serialize_check( c == 0xDEADBEEF );
    }
}

#define SERIALIZE_RUN_TEST( test_function )                                 \
    do                                                                      \
    {                                                                       \
        printf( #test_function "\n" );                                      \
        test_function();                                                    \
    }                                                                       \
    while (0)

inline void serialize_test()
{
    // while ( 1 )
    {
        SERIALIZE_RUN_TEST( test_endian );
        SERIALIZE_RUN_TEST( test_bitpacker );
        SERIALIZE_RUN_TEST( test_bits_required );
        SERIALIZE_RUN_TEST( test_bits_required64 );
        SERIALIZE_RUN_TEST( test_zigzag );
        SERIALIZE_RUN_TEST( test_serialize );
        SERIALIZE_RUN_TEST( test_read_write );
        SERIALIZE_RUN_TEST( test_serialize_integer_validation );
        SERIALIZE_RUN_TEST( test_serialize_integer_full_range );
        SERIALIZE_RUN_TEST( test_serialize_int64_full_range );
        SERIALIZE_RUN_TEST( test_serialize_int64_validation );
        SERIALIZE_RUN_TEST( test_serialize_bytes_validation );
        SERIALIZE_RUN_TEST( test_int_relative_validation );
        SERIALIZE_RUN_TEST( test_compressed_float_validation );
        SERIALIZE_RUN_TEST( test_golden_wire_format );
        SERIALIZE_RUN_TEST( test_unaligned_writer );
    }
}

#endif // #if SERIALIZE_ENABLE_TESTS

#ifdef _MSC_VER
#pragma warning( pop )
#endif // #ifdef _MSC_VER

#endif // #ifndef SERIALIZE_H
