/*
    Protocol2 Library.

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

#ifndef PROTOCOL2_H
#define PROTOCOL2_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define PROTOCOL2_SERIALIZE_CHECKS              1
#define PROTOCOL2_DEBUG_PACKET_LEAKS            0
#define PROTOCOL2_PACKET_AGGREGATION            1

#if PROTOCOL2_DEBUG_PACKET_LEAKS
#include <stdio.h>
#include <map>
#endif // #if PROTOCOL2_DEBUG_PACKET_LEAKS

#if    defined(__386__) || defined(i386)    || defined(__i386__)  \
    || defined(__X86)   || defined(_M_IX86)                       \
    || defined(_M_X64)  || defined(__x86_64__)                    \
    || defined(alpha)   || defined(__alpha) || defined(__alpha__) \
    || defined(_M_ALPHA)                                          \
    || defined(ARM)     || defined(_ARM)    || defined(__arm__)   \
    || defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) \
    || defined(_WIN32_WCE) || defined(__NT__)                     \
    || defined(__MIPSEL__)
  #define PROTOCOL2_LITTLE_ENDIAN 1
#else
  #define PROTOCOL2_BIG_ENDIAN 1
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#endif // #ifdef _MSC_VER

namespace protocol2
{
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
#if PROTOCOL2_BIG_ENDIAN
        return bswap( value );
#else // #if PROTOCOL2_BIG_ENDIAN
        return value;
#endif // #if PROTOCOL2_BIG_ENDIAN
    }

    inline uint32_t network_to_host( uint32_t value )
    {
#if PROTOCOL2_BIG_ENDIAN
        return bswap( value );
#else // #if PROTOCOL2_BIG_ENDIAN
        return value;
#endif // #if PROTOCOL2_BIG_ENDIAN
    }

    inline uint16_t host_to_network( uint16_t value )
    {
#if PROTOCOL2_BIG_ENDIAN
        return bswap( value );
#else // #if PROTOCOL2_BIG_ENDIAN
        return value;
#endif // #if PROTOCOL2_BIG_ENDIAN
    }

    inline uint16_t network_to_host( uint16_t value )
    {
#if PROTOCOL2_BIG_ENDIAN
        return bswap( value );
#else // #if PROTOCOL2_BIG_ENDIAN
        return value;
#endif // #if PROTOCOL2_BIG_ENDIAN
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

    uint32_t calculate_crc32( const uint8_t *buffer, size_t length, uint32_t crc32 = 0 );

    uint32_t hash_data( const uint8_t * data, uint32_t length, uint32_t hash );

    uint32_t hash_string( const char string[], uint32_t hash );

    uint64_t murmur_hash_64( const void * key, uint32_t length, uint64_t seed );

    class BitWriter
    {
    public:

        BitWriter( void* data, int bytes ) 
            : m_data( (uint32_t*)data ), m_numWords( bytes / 4 )
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

        void WriteBytes( const uint8_t* data, int bytes )
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

        const uint8_t* GetData() const
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

        uint32_t* m_data;
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
        BitReader( const void* data, int bytes ) : m_data( (const uint32_t*)data ), m_numBytes( bytes ), m_numWords( ( bytes + 3 ) / 4)
#else // #ifdef DEBUG
        BitReader( const void* data, int bytes ) : m_data( (const uint32_t*)data ), m_numBytes( bytes )
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

        void ReadBytes( uint8_t* data, int bytes )
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

        int GetTotalBits() const 
        {
            return m_numBits;
        }

        int GetTotalBytes() const
        {
            return m_numBits / 8;
        }

    private:

        const uint32_t* m_data;
        uint64_t m_scratch;
        int m_numBits;
        int m_numBytes;
#ifdef DEBUG
        int m_numWords;
#endif // #ifdef DEBUG
        int m_bitsRead;
        int m_scratchBits;
        int m_wordIndex;
    };

    #define PROTOCOL2_ERROR_NONE                        0
    #define PROTOCOL2_ERROR_CRC32_MISMATCH              1
    #define PROTOCOL2_ERROR_INVALID_PACKET_TYPE         2
    #define PROTOCOL2_ERROR_PACKET_TYPE_NOT_ALLOWED     3
    #define PROTOCOL2_ERROR_CREATE_PACKET_FAILED        4
    #define PROTOCOL2_ERROR_SERIALIZE_HEADER_FAILED     5
    #define PROTOCOL2_ERROR_SERIALIZE_PACKET_FAILED     6
    #define PROTOCOL2_ERROR_SERIALIZE_CHECK_FAILED      7
    #define PROTOCOL2_ERROR_STREAM_OVERFLOW             8
    #define PROTOCOL2_ERROR_STREAM_ABORTED              9

    class WriteStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        WriteStream( uint8_t* buffer, int bytes ) : m_error( PROTOCOL2_ERROR_NONE ), m_context( NULL ), m_writer( buffer, bytes ) {}

        bool SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            assert( min < max );
            assert( value >= min );
            assert( value <= max );
            const int bits = bits_required( min, max );
            uint32_t unsigned_value = value - min;
            m_writer.WriteBits( unsigned_value, bits );
            return true;
        }

        bool SerializeBits( uint32_t value, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
            m_writer.WriteBits( value, bits );
            return true;
        }

        bool SerializeBytes( const uint8_t* data, int bytes )
        {
            assert( data );
            assert( bytes >= 0 );
            if ( !SerializeAlign() )
                return false;
            m_writer.WriteBytes( data, bytes );
            return true;
        }

        bool SerializeAlign()
        {
            m_writer.WriteAlign();
            return true;
        }

        int GetAlignBits() const
        {
            return m_writer.GetAlignBits();
        }

        bool SerializeCheck( const char * string )
        {
#if PROTOCOL2_SERIALIZE_CHECKS
            SerializeAlign();
            const uint32_t magic = hash_string( string, 0 );
            SerializeBits( magic, 32 );
#endif // #if PROTOCOL2_SERIALIZE_CHECKS
            return true;
        }

        void Flush()
        {
            m_writer.FlushBits();
        }

        const uint8_t* GetData() const
        {
            return m_writer.GetData();
        }

        int GetBytesProcessed() const
        {
            return m_writer.GetBytesWritten();
        }

        int GetBitsProcessed() const
        {
            return m_writer.GetBitsWritten();
        }

        int GetBitsRemaining() const
        {
            return GetTotalBits() - GetBitsProcessed();
        }

        int GetTotalBits() const
        {
            return m_writer.GetTotalBytes() * 8;
        }

        int GetTotalBytes() const
        {
            return m_writer.GetTotalBytes();
        }

        void SetContext( void *context )
        {
            m_context = context;
        }

        void* GetContext() const
        {
            return m_context;
        }

        int GetError() const
        {
            return m_error;
        }

    private:

        int m_error;
        void *m_context;
        BitWriter m_writer;
    };

    class ReadStream
    {
    public:

        enum { IsWriting = 0 };
        enum { IsReading = 1 };

        ReadStream( const uint8_t* buffer, int bytes ) : m_context( NULL ), m_error( PROTOCOL2_ERROR_NONE ), m_bitsRead(0), m_reader( buffer, bytes ) {}

        bool SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            assert( min < max );
            const int bits = bits_required( min, max );
            if ( m_reader.WouldOverflow( bits ) )
            {
                m_error = PROTOCOL2_ERROR_STREAM_OVERFLOW;
                return false;
            }
            uint32_t unsigned_value = m_reader.ReadBits( bits );
            value = (int32_t) unsigned_value + min;
            m_bitsRead += bits;
            return true;
        }

        bool SerializeBits( uint32_t & value, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
            if ( m_reader.WouldOverflow( bits ) )
            {
                m_error = PROTOCOL2_ERROR_STREAM_OVERFLOW;
                return false;
            }
            uint32_t read_value = m_reader.ReadBits( bits );
            value = read_value;
            m_bitsRead += bits;
            return true;
        }

        bool SerializeBytes( uint8_t* data, int bytes )
        {
            if ( !SerializeAlign() )
                return false;
            if ( m_reader.WouldOverflow( bytes * 8 ) )
            {
                m_error = PROTOCOL2_ERROR_STREAM_OVERFLOW;
                return false;
            }
            m_reader.ReadBytes( data, bytes );
            m_bitsRead += bytes * 8;
            return true;
        }

        bool SerializeAlign()
        {
            const int alignBits = m_reader.GetAlignBits();
            if ( m_reader.WouldOverflow( alignBits ) )
            {
                m_error = PROTOCOL2_ERROR_STREAM_OVERFLOW;
                return false;
            }
            if ( !m_reader.ReadAlign() )
                return false;
            m_bitsRead += alignBits;
            return true;
        }

        int GetAlignBits() const
        {
            return m_reader.GetAlignBits();
        }

        bool SerializeCheck( const char * string )
        {
#if PROTOCOL2_SERIALIZE_CHECKS            
            SerializeAlign();
            uint32_t value = 0;
            SerializeAlign();
            SerializeBits( value, 32 );
            const uint32_t magic = hash_string( string, 0 );
            if ( magic != value )
            {
                printf( "serialize check failed: '%s'. expected %x, got %x\n", string, magic, value );
            }
            return value == magic;
#else // #if PROTOCOL2_SERIALIZE_CHECKS
            return true;
#endif // #if PROTOCOL2_SERIALIZE_CHECKS
        }

        int GetBitsProcessed() const
        {
            return m_bitsRead;
        }

        int GetBitsRemaining() const
        {
            return m_reader.GetBitsRemaining();
        }

        int GetBytesProcessed() const
        {
            return ( m_bitsRead + 7 ) / 8;
        }

        void SetContext( void* context )
        {
            m_context = context;
        }

        void* GetContext() const
        {
            return m_context;
        }

        int GetError() const
        {
            return m_error;
        }

        int GetBytesRead() const
        {
            return m_reader.GetBytesRead();
        }

    private:

        void* m_context;
        int m_error;
        int m_bitsRead;
        BitReader m_reader;
    };

    class MeasureStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        MeasureStream( int bytes ) : m_context( NULL ), m_error( PROTOCOL2_ERROR_NONE ), m_totalBytes( bytes ), m_bitsWritten(0) {}

#ifdef DEBUG
        bool SerializeInteger( int32_t value, int32_t min, int32_t max )
#else // #ifdef DEBUG
        bool SerializeInteger( int32_t /*value*/, int32_t min, int32_t max )
#endif // #ifdef DEBUG
        {
            assert( min < max );
            assert( value >= min );
            assert( value <= max );
            const int bits = bits_required( min, max );
            m_bitsWritten += bits;
            return true;
        }

        bool SerializeBits( uint32_t /*value*/, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
            m_bitsWritten += bits;
            return true;
        }

        bool SerializeBytes( const uint8_t* /*data*/, int bytes )
        {
            SerializeAlign();
            m_bitsWritten += bytes * 8;
            return true;
        }

        bool SerializeAlign()
        {
            const int alignBits = GetAlignBits();
            m_bitsWritten += alignBits;
            return true;
        }

        int GetAlignBits() const
        {
            return 7;       // we can't know for sure, so be conservative and assume worst case
        }

        bool SerializeCheck( const char * /*string*/ )
        {
#if PROTOCOL2_SERIALIZE_CHECKS
            SerializeAlign();
            m_bitsWritten += 32;
#endif // #if PROTOCOL2_SERIALIZE_CHECKS
            return true;
        }

        int GetBitsProcessed() const
        {
            return m_bitsWritten;
        }

        int GetBitsRemaining() const
        {
            return m_totalBytes * 8 - GetBitsProcessed();
        }

        int GetBytesProcessed() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

        int GetTotalBytes() const
        {
            return m_totalBytes;
        }

        int GetTotalBits() const
        {
            return m_totalBytes * 8;
        }

        void SetContext( void* context )
        {
            m_context = context;
        }

        void* GetContext() const
        {
            return m_context;
        }

        int GetError() const
        {
            return m_error;
        }

    private:

        void* m_context;
        int m_error;
        int m_totalBytes;
        int m_bitsWritten;
	};

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

	#define serialize_bits( stream, value, bits )						\
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

	#define serialize_bool( stream, value )								\
		do																\
		{																\
            uint32_t uint32_bool_value;									\
			if ( Stream::IsWriting )									\
				uint32_bool_value = value ? 1 : 0; 						\
			serialize_bits( stream, uint32_bool_value, 1 );				\
			if ( Stream::IsReading )									\
				value = uint32_bool_value ? true : false;				\
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
            if ( !protocol2::serialize_float_internal( stream, value ) )            \
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
            if ( !protocol2::serialize_uint64_internal( stream, value ) )           \
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
            if ( !protocol2::serialize_double_internal( stream, value ) )           \
                return false;                                                       \
        } while (0)

    template <typename Stream> bool serialize_bytes_internal( Stream & stream, uint8_t* data, int bytes )
    {
        return stream.SerializeBytes( data, bytes );
    }

    #define serialize_bytes( stream, data, bytes )                                  \
        do                                                                          \
        {                                                                           \
            if ( !protocol2::serialize_bytes_internal( stream, data, bytes ) )      \
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
            if ( !protocol2::serialize_string_internal( stream, string, buffer_size ) )     \
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

    class Object
    {  
    public:

        virtual ~Object() {}

        virtual bool SerializeRead( class ReadStream & stream ) = 0;

        virtual bool SerializeWrite( class WriteStream & stream ) = 0;

        virtual bool SerializeMeasure( class MeasureStream & stream ) = 0;
    };

    #define PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS()                                                   \
        bool SerializeRead( class protocol2::ReadStream & stream ) { return Serialize( stream ); };           \
        bool SerializeWrite( class protocol2::WriteStream & stream ) { return Serialize( stream ); };         \
        bool SerializeMeasure( class protocol2::MeasureStream & stream ) { return Serialize( stream ); };     \

    class Packet : public Object
    {
        int type;

    public:
        
        Packet( int _type ) : type(_type) {}

        virtual ~Packet() {}

        int GetType() const { return type; }

    private:

        Packet( const Packet & other );
        Packet & operator = ( const Packet & other );
    };

    class PacketFactory
    {        
        int m_numPacketTypes;
        int m_numAllocatedPackets;  
#if PROTOCOL2_DEBUG_MEMORY_LEAKS
        std::map<void*,int> allocated_packets;
#endif // #if PROTOCOL2_DEBUG_MEMORY_LEAKS

    public:

        PacketFactory( int numTypes );

        ~PacketFactory();

        Packet* CreatePacket( int type );

        void DestroyPacket( Packet *packet );

        int GetNumPacketTypes() const;

    protected:

        virtual Packet* Create( int type ) = 0;
        virtual void Destroy( Packet *packet ) = 0;
    };

    struct PacketInfo
    {
        bool rawFormat;                             // if true packets are written in "raw" format without crc32 (useful for encrypted packets).
        int prefixBytes;                            // prefix this number of bytes when reading and writing packets. stick your own data there.
        uint32_t protocolId;                        // protocol id that distinguishes your protocol from other packets sent over UDP.
        PacketFactory * packetFactory;              // create packets and determine information about packet types. required.
        const uint8_t * allowedPacketTypes;         // array of allowed packet types. if a packet type is not allowed the serialize read or write will fail.
        void * context;                             // context for the packet serialization (optional, pass in NULL)

        PacketInfo()
        {
            rawFormat = false;
            prefixBytes = 0;
            protocolId = 0;
            packetFactory = NULL;
            allowedPacketTypes = NULL;
            context = NULL;
        }
    };

    int WritePacket( const PacketInfo & info, 
                     Packet *packet, 
                     uint8_t *buffer, 
                     int bufferSize, 
                     Object *header = NULL );

    Packet * ReadPacket( const PacketInfo & info, 
                         const uint8_t *buffer, 
                         int bufferSize, 
                         Object *header = NULL, 
                         int *errorCode = NULL );

#if PROTOCOL2_PACKET_AGGREGATION

    int WriteAggregatePacket( const PacketInfo & info, 
                              int numPackets, 
                              Packet **packets, 
                              uint8_t *buffer, 
                              int bufferSize, 
                              int & numPacketsWritten, 
                              Object *aggregatePacketHeader = NULL, 
                              Object **packetHeaders = NULL );

    void ReadAggregatePacket( const PacketInfo & info, 
                              int maxPacketsToRead, 
                              Packet **packets, 
                              const uint8_t *buffer, 
                              int bufferSize, 
                              int & numPacketsRead, 
                              Object *aggregatePacketHeader = NULL, 
                              Object **packetHeaders = NULL, 
                              int *errorCode = NULL );

#endif // #if PROTOCOL2_PACKET_AGGREGATION

    const char* GetErrorString( int error );
}

#endif // #ifndef PROTOCOL2_H

#ifdef PROTOCOL2_IMPLEMENTATION

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <alloca.h>
#endif

namespace protocol2
{
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
#if PROTOCOL2_LITTLE_ENDIAN
                uint64_t k = *data++;
#else // #if PROTOCOL2_LITTLE_ENDIAN
                uint64_t k = *data++;
                uint8_t * p = (uint8_t*) &k;
                uint8_t c;
                c = p[0]; p[0] = p[7]; p[7] = c;
                c = p[1]; p[1] = p[6]; p[6] = c;
                c = p[2]; p[2] = p[5]; p[5] = c;
                c = p[3]; p[3] = p[4]; p[4] = c;
#endif // #if PROTOCOL2_LITTLE_ENDIAN

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

    int WritePacket( const PacketInfo & info, 
                     Packet *packet,
                     uint8_t *buffer, 
                     int bufferSize, 
                     Object *header )
    {
        assert( packet );
        assert( buffer );
        assert( bufferSize > 0 );
        assert( info.protocolId );
        assert( info.packetFactory );

        const int numPacketTypes = info.packetFactory->GetNumPacketTypes();

        WriteStream stream( buffer, bufferSize );

        stream.SetContext( info.context );

        for ( int i = 0; i < info.prefixBytes; ++i )
        {
            uint8_t zero = 0;
            stream.SerializeBits( zero, 8 );
        }

        uint32_t crc32 = 0;

        if ( !info.rawFormat )
            stream.SerializeBits( crc32, 32 );

        if ( header )
        {
            if ( !header->SerializeWrite( stream ) )
                return 0;
        }

        int packetType = packet->GetType();

        assert( numPacketTypes > 0 );

        if ( numPacketTypes > 1 )
        {
            stream.SerializeInteger( packetType, 0, numPacketTypes - 1 );
        }

        if ( !packet->SerializeWrite( stream ) )
            return 0;

        stream.SerializeCheck( "end of packet" );

        stream.Flush();

        if ( !info.rawFormat )
        {
            uint32_t network_protocolId = host_to_network( info.protocolId );
            crc32 = calculate_crc32( (uint8_t*) &network_protocolId, 4 );
            crc32 = calculate_crc32( buffer + info.prefixBytes, stream.GetBytesProcessed() - info.prefixBytes, crc32 );
            *((uint32_t*)(buffer+info.prefixBytes)) = host_to_network( crc32 );
        }

        if ( stream.GetError() )
            return 0;

        return stream.GetBytesProcessed();
    }

    Packet* ReadPacket( const PacketInfo & info, 
                        const uint8_t * buffer, 
                        int bufferSize, 
                        Object * header, 
                        int * errorCode )
    {
        assert( buffer );
        assert( bufferSize > 0 );
        assert( info.protocolId != 0 );
        assert( info.packetFactory );

        if ( errorCode )
            *errorCode = PROTOCOL2_ERROR_NONE;

        ReadStream stream( buffer, bufferSize );

        stream.SetContext( info.context );

        for ( int i = 0; i < info.prefixBytes; ++i )
        {
            uint32_t dummy = 0;
            stream.SerializeBits( dummy, 8 );
        }

        uint32_t read_crc32 = 0;
        if ( !info.rawFormat )
        {
            stream.SerializeBits( read_crc32, 32 );

            uint32_t network_protocolId = host_to_network( info.protocolId );
            uint32_t crc32 = calculate_crc32( (const uint8_t*) &network_protocolId, 4 );
            uint32_t zero = 0;
            crc32 = calculate_crc32( (const uint8_t*) &zero, 4, crc32 );
            crc32 = calculate_crc32( buffer + info.prefixBytes + 4, bufferSize - 4 - info.prefixBytes, crc32 );

            if ( crc32 != read_crc32 )
            {
                printf( "corrupt packet. expected crc32 %x, got %x\n", crc32, read_crc32 );

                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_CRC32_MISMATCH;
                return NULL;
            }
        }

        if ( header )
        {
            if ( !header->SerializeRead( stream ) )
            {
                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_SERIALIZE_HEADER_FAILED;
                return NULL;
            }
        }

        int packetType = 0;

        const int numPacketTypes = info.packetFactory->GetNumPacketTypes();

        assert( numPacketTypes > 0 );

        if ( numPacketTypes > 1 )
        {
            if ( !stream.SerializeInteger( packetType, 0, numPacketTypes - 1 ) )
            {
                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_INVALID_PACKET_TYPE;
                return NULL;
            }
        }

        if ( info.allowedPacketTypes )
        {
            if ( !info.allowedPacketTypes[packetType] )
            {
                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_PACKET_TYPE_NOT_ALLOWED;
                return NULL;
            }
        }

        protocol2::Packet *packet = info.packetFactory->CreatePacket( packetType );
        if ( !packet )
        {
            if ( errorCode )
                *errorCode = PROTOCOL2_ERROR_CREATE_PACKET_FAILED;
            return NULL;
        }

        if ( !packet->SerializeRead( stream ) )
        {
            if ( errorCode )
                *errorCode = PROTOCOL2_ERROR_SERIALIZE_PACKET_FAILED;
            goto cleanup;
        }

#if PROTOCOL2_SERIALIZE_CHECKS
        if ( !stream.SerializeCheck( "end of packet" ) )
        {
            if ( errorCode )
                *errorCode = PROTOCOL2_ERROR_SERIALIZE_CHECK_FAILED;
            goto cleanup;
        }
#endif // #if PROTOCOL2_SERIALIZE_CHECKS

        if ( stream.GetError() )
        {
            if ( errorCode )
                *errorCode = stream.GetError();
            goto cleanup;
        }

        return packet;

cleanup:
        info.packetFactory->DestroyPacket( packet );
        return NULL;
    }

#if PROTOCOL2_PACKET_AGGREGATION

    int WriteAggregatePacket( const PacketInfo & info, 
                              int numPackets, 
                              Packet **packets, 
                              uint8_t *buffer, 
                              int bufferSize, 
                              int & numPacketsWritten, 
                              Object *aggregatePacketHeader, 
                              Object **packetHeaders )
    {
        assert( numPackets >= 0 );
        assert( packets );
        assert( buffer );
        assert( bufferSize > 0 );
        assert( info.protocolId != 0 );
        assert( info.packetFactory );

        const int numPacketTypes = info.packetFactory->GetNumPacketTypes();

        assert( numPacketTypes > 0 );
        assert( numPacketTypes + 1 <= 65535 );

        const int packetTypeBytes = ( numPacketTypes > 255 ) ? 2 : 1;

        numPacketsWritten = 0;

        int aggregatePacketBytes = 0;

        if ( info.prefixBytes > 0 )
        {
            memset( buffer, 0, info.prefixBytes );
            aggregatePacketBytes += info.prefixBytes;
        }

        if ( !info.rawFormat )
        {
            // reserve space for crc32
            memset( buffer + aggregatePacketBytes, 0, 4 );
            aggregatePacketBytes += 4;
        }

        // write the optional aggregate packet header

        if ( aggregatePacketHeader )
        {
			uint8_t *scratch = (uint8_t*) alloca( bufferSize );

            typedef WriteStream Stream;

            Stream stream( scratch, bufferSize );

            stream.SetContext( info.context );

            if ( !aggregatePacketHeader->SerializeWrite( stream ) )
                return 0;

            stream.SerializeCheck( "aggregate packet header" );

            stream.SerializeAlign();

            stream.Flush();

            if ( stream.GetError() )
                return 0;

            int packetSize = stream.GetBytesProcessed();

            memcpy( buffer + aggregatePacketBytes, scratch, packetSize );

            aggregatePacketBytes += packetSize;
        }

        // write packet type, packet header (optional) and packet data for each packet passed in

        for ( int i = 0; i < numPackets; ++i )
        {
			uint8_t *scratch = (uint8_t*) alloca( bufferSize );

            typedef WriteStream Stream;

            Stream stream( scratch, bufferSize );

            Packet *packet = packets[i];

            int packetTypePlusOne = packet->GetType() + 1;

            assert( numPacketTypes > 0 );

            assert( stream.GetAlignBits() == 0 );       // must be byte aligned at this point

            stream.SerializeInteger( packetTypePlusOne, 0, numPacketTypes );

            if ( packetHeaders )
            {
                assert( packetHeaders[i] );

                if ( !packetHeaders[i]->SerializeWrite( stream ) )
                    return 0;
            }

            if ( !packet->SerializeWrite( stream ) )
                return 0;

            stream.SerializeCheck( "end of packet" );

            stream.SerializeAlign();

            stream.Flush();

            if ( stream.GetError() )
                return 0;

            int packetSize = stream.GetBytesProcessed();

            if ( aggregatePacketBytes + packetSize >= bufferSize + packetTypeBytes )
                break;

            memcpy( buffer + aggregatePacketBytes, scratch, packetSize );

            aggregatePacketBytes += packetSize;

            numPacketsWritten++;
        }

        // write END marker packet type (0)

        memset( buffer + aggregatePacketBytes, 0, packetTypeBytes );

        aggregatePacketBytes += packetTypeBytes;

        assert( aggregatePacketBytes > 0 );
        assert( aggregatePacketBytes <= bufferSize );

        if ( !info.rawFormat )
        {
            // calculate header crc32 for aggregate packet as a whole

            uint32_t network_protocolId = host_to_network( info.protocolId );
            uint32_t crc32 = calculate_crc32( (uint8_t*) &network_protocolId, 4 );
            crc32 = calculate_crc32( buffer, aggregatePacketBytes, crc32 );

            *((uint32_t*)(buffer+info.prefixBytes)) = host_to_network( crc32 );
        }

        return aggregatePacketBytes;
    }

    void ReadAggregatePacket( const PacketInfo & info,
                              int maxPacketsToRead, 
                              Packet ** packets, 
                              const uint8_t * buffer, 
                              int bufferSize, 
                              int & numPacketsRead, 
                              Object * aggregatePacketHeader, 
                              Object ** packetHeaders, 
                              int * errorCode )
    {
        assert( info.protocolId );
        assert( info.packetFactory );

        numPacketsRead = 0;

        for ( int i = 0; i < maxPacketsToRead; ++i )
            packets[i] = NULL;

        typedef protocol2::ReadStream Stream;

        Stream stream( buffer, bufferSize );

        stream.SetContext( info.context );

        for ( int i = 0; i < info.prefixBytes; ++i )
        {
            uint32_t dummy = 0;
            stream.SerializeBits( dummy, 8 );
        }

        if ( !info.rawFormat )
        {
            // verify crc32 for packet matches, otherwise discard with error

            uint32_t read_crc32 = 0;
            stream.SerializeBits( read_crc32, 32 );

            uint32_t network_protocolId = host_to_network( info.protocolId );
            uint32_t crc32 = calculate_crc32( (const uint8_t*) &network_protocolId, 4 );
            uint32_t zero = 0;
            crc32 = calculate_crc32( (const uint8_t*) &zero, 4, crc32 );
            crc32 = calculate_crc32( buffer + 4, bufferSize - 4, crc32 );

            if ( crc32 != read_crc32 )
            {
                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_CRC32_MISMATCH;
                return;
            }
        }

        // read optional aggregate packet header

        if ( aggregatePacketHeader )
        {
            if ( !aggregatePacketHeader->SerializeRead( stream ) )
            {
                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_SERIALIZE_HEADER_FAILED;
                return;
            }

            stream.SerializeCheck( "aggregate packet header" );

            stream.SerializeAlign();
        }

        // serialize read packets in the aggregate packet until we hit an end packet marker

        while ( numPacketsRead < maxPacketsToRead )
        {
            assert( info.packetFactory->GetNumPacketTypes() > 0 );

            assert( stream.GetAlignBits() == 0 );       // must be byte aligned at this point

            int packetTypePlusOne = 0;

            stream.SerializeInteger( packetTypePlusOne, 0, info.packetFactory->GetNumPacketTypes() );

            if ( packetTypePlusOne == 0 )               // end of packet marker
                break;

            const int packetType = packetTypePlusOne - 1;

            if ( info.allowedPacketTypes )
            {
                if ( !info.allowedPacketTypes[packetType] )
                {
                    if ( errorCode )
                        *errorCode = PROTOCOL2_ERROR_PACKET_TYPE_NOT_ALLOWED;
                    goto cleanup;
                }
            }

            if ( packetHeaders )
            {
                assert( packetHeaders[numPacketsRead] );

                if ( !packetHeaders[numPacketsRead]->SerializeRead( stream ) )
                {
                    if ( errorCode )
                        *errorCode = PROTOCOL2_ERROR_SERIALIZE_HEADER_FAILED;
                    goto cleanup;
                }
            }

            packets[numPacketsRead] = info.packetFactory->CreatePacket( packetType );

            if ( !packets[numPacketsRead] )
            {
                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_CREATE_PACKET_FAILED;
                goto cleanup;
            }

            if ( !packets[numPacketsRead]->SerializeRead( stream ) )
            {
                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_SERIALIZE_PACKET_FAILED;
                goto cleanup;
            }

            if ( !stream.SerializeCheck( "end of packet" ) )
            {
                if ( errorCode )
                    *errorCode = PROTOCOL2_ERROR_SERIALIZE_CHECK_FAILED;
                goto cleanup;
            }

            stream.SerializeAlign();

            if ( stream.GetError() )
            {
                if ( errorCode )
                    *errorCode = stream.GetError();
                goto cleanup;
            }

            ++numPacketsRead;
        }

        return;

    cleanup:

        for ( int j = 0; j < numPacketsRead; ++j )
        {
            info.packetFactory->DestroyPacket( packets[j] );
            packets[j] = NULL;
        }

        numPacketsRead = 0;
    }

#endif // #if PROTOCOL2_PACKET_AGGREGATION

    PacketFactory::PacketFactory( int numPacketTypes )
    {
        m_numPacketTypes = numPacketTypes;
        m_numAllocatedPackets = 0;
    }

    PacketFactory::~PacketFactory()
    {
#if PROTOCOL2_DEBUG_MEMORY_LEAKS
        if ( allocated_packets.size() )
        {
            printf( "you leaked packets!\n" );
            printf( "%d packets leaked\n", m_numAllocatedPackets );
            for ( auto itor : allocated_packets )
            {
                auto p = itor.first;
                printf( "leaked packet %p\n", p );
            }
        }
#endif // #if PROTOCOL2_DEBUG_MEMORY_LEAKS

        assert( m_numAllocatedPackets == 0 );
    }

    Packet* PacketFactory::CreatePacket( int type )
    {
        assert( type >= 0 );
        assert( type < m_numPacketTypes );

        Packet * packet = Create( type );
        if ( !packet )
            return NULL;
        
#if PROTOCOL2_DEBUG_MEMORY_LEAKS
        printf( "create packet %p\n", packet );
        allocated_packets[packet] = 1;
        auto itor = allocated_packets.find( packet );
        assert( itor != allocated_packets.end() );
#endif // #if PROTOCOL2_DEBUG_MEMORY_LEAKS
        
        m_numAllocatedPackets++;

        return packet;
    }

    void PacketFactory::DestroyPacket( Packet* packet )
    {
        if ( !packet )
            return;

#if PROTOCOL2_DEBUG_MEMORY_LEAKS
        printf( "destroy packet %p\n", packet );
        auto itor = allocated_packets.find( packet );
        assert( itor != allocated_packets.end() );
        allocated_packets.erase( packet );
#endif // #if PROTOCOL2_DEBUG_MEMORY_LEAKS

        assert( m_numAllocatedPackets > 0 );

        m_numAllocatedPackets--;

        Destroy( packet );
    }

    int PacketFactory::GetNumPacketTypes() const
    {
        return m_numPacketTypes;
    }

    const char* GetErrorString( int error )
    {
        switch ( error )
        {
            case PROTOCOL2_ERROR_NONE:                          return "no error";
            case PROTOCOL2_ERROR_CRC32_MISMATCH:                return "crc32 mismatch";
            case PROTOCOL2_ERROR_INVALID_PACKET_TYPE:           return "invalid packet type";
            case PROTOCOL2_ERROR_CREATE_PACKET_FAILED:          return "create packet failed";
            case PROTOCOL2_ERROR_SERIALIZE_HEADER_FAILED:       return "serialize header failed";
            case PROTOCOL2_ERROR_SERIALIZE_PACKET_FAILED:       return "serialize packet failed";
            case PROTOCOL2_ERROR_SERIALIZE_CHECK_FAILED:        return "serialize check failed";
            case PROTOCOL2_ERROR_STREAM_OVERFLOW:               return "stream overflow";
            case PROTOCOL2_ERROR_STREAM_ABORTED:                return "stream aborted";

            default:
                return "???";
        }
    }
}

#endif // #ifdef PROTOCOL2_IMPLEMENTATION
