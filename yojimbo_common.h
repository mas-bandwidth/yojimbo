/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.

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
#include "yojimbo_allocator.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <memory.h>
    
/** @file */

namespace yojimbo
{
    /**
        Generate random bytes.

        This is a cryptographically secure random number.
     */

    void random_bytes( uint8_t * data, int bytes );

    /**
        Generate a random integer between a and b (inclusive).

        IMPORTANT: This is not a cryptographically secure random. It's used only for test functions and in the network simulator.

        @param a The minimum integer value to generate.
        @param b The maximum integer value to generate.

        @returns A pseudo random integer value in [a,b].
     */

    inline int random_int( int a, int b )
    {
        assert( a < b );
        int result = a + rand() % ( b - a + 1 );
        assert( result >= a );
        assert( result <= b );
        return result;
    }

    /** 
        Generate a random float between a and b.

        IMPORTANT: This is not a cryptographically secure random. It's used only for test functions and in the network simulator.

        @param a The minimum integer value to generate.
        @param b The maximum integer value to generate.

        @returns A pseudo random float value in [a,b].
     */

    inline float random_float( float a, float b )
    {
        assert( a < b );
        float random = ( (float) rand() ) / (float) RAND_MAX;
        float diff = b - a;
        float r = random * diff;
        return a + r;
    }

    #ifndef min

    /**
        Template function to get the minimum of two values.

        @param a The first value.
        @param b The second value.

        @returns The minimum of a and b.
     */

    template <typename T> const T & min( const T & a, const T & b )
    {
        return ( a < b ) ? a : b;
    }

    #endif // #ifndef min

    #ifndef max

    /**
        Template function to get the maximum of two values.

        @param a The first value.
        @param b The second value.

        @returns The maximum of a and b.
     */

    template <typename T> const T & max( const T & a, const T & b )
    {
        return ( a > b ) ? a : b;
    }

    #endif // #ifndef max

    /**
        Template function to clamp a value.

        @param value The value to be clamped.
        @param a The minimum value.
        @param b The minimum value.

        @returns The clamped value in [a,b].
     */

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
        Swaps two values.

        @param a First value.
        @param b Second value.
     */

    template <typename T> void swap( T & a, T & b )
    {
        T tmp = a;
        a = b;
        b = tmp;
    };

    /**
        Get the absolute value.

        @param value The input value.

        @returns The absolute value.
     */

    template <typename T> T abs( const T & value )
    {
        return ( value < 0 ) ? -value : value;
    }

    /**
        Calculates the population count of an unsigned 32 bit integer at compile time.

        Population count is the number of bits in the integer that set to 1.

        See "Hacker's Delight" and http://www.hackersdelight.org/hdcodetxt/popArrayHS.c.txt

        @see yojimbo::Log2
        @see yojimbo::BitsRequired
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

        @see yojimbo::Log2
        @see yojimbo::BitsRequired
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
        return 32 - __builtin_clz( max - min );
#else // #ifdef __GNUC__
        return ( min == max ) ? 0 : log2( max - min ) + 1;
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

        IMPORTANT: Yojimbo defines network byte order to be little endian, because most machines running yojimbo are little endian, this creates the least amount of work.

        @param value The input value in local byte order. Supported integer types: uint64_t, uint32_t, uint16_t.

        @returns The input value converted to network byte order. If this processor is little endian the output is the same as the input. If the processor is big endian, the output is the input byte swapped.

        @see yojimbo::bswap
     */

    template <typename T> T host_to_network( T value )
    {
#if YOJIMBO_BIG_ENDIAN
        return bswap( value );
#else // #if YOJIMBO_BIG_ENDIAN
        return value;
#endif // #if YOJIMBO_BIG_ENDIAN
    }

    /**
        Template to convert an integer value from network byte order to local byte order.

        IMPORTANT: Yojimbo defines network byte order to be little endian, because most machines running yojimbo are little endian, this creates the least amount of work.

        @param value The input value in network byte order. Supported integer types: uint64_t, uint32_t, uint16_t.

        @returns The input value converted to local byte order. If this processor is little endian the output is the same as the input. If the processor is big endian, the output is the input byte swapped.

        @see yojimbo::bswap
     */

    template <typename T> T network_to_host( T value )
    {
#if YOJIMBO_BIG_ENDIAN
        return bswap( value );
#else // #if YOJIMBO_BIG_ENDIAN
        return value;
#endif // #if YOJIMBO_BIG_ENDIAN
    }

    /** 
        Compares two 16 bit sequence numbers and returns true if the first one is greater than the second (considering wrapping).

        IMPORTANT: This is not the same as s1 > s2!

        Greater than is defined specially to handle wrapping sequence numbers. 

        If the two sequence numbers are close together, it is as normal, but they are far apart, it is assumed that they have wrapped around.

        Thus, sequence_greater_than( 1, 0 ) returns true, and so does sequence_greater_than( 0, 65535 )!

        @param s1 The first sequence number.
        @param s2 The second sequence number.

        @returns True if the s1 is greater than s2, with sequence number wrapping considered.
     */

    inline bool sequence_greater_than( uint16_t s1, uint16_t s2 )
    {
        return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
               ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
    }

    /** 
        Compares two 16 bit sequence numbers and returns true if the first one is less than the second (considering wrapping).

        IMPORTANT: This is not the same as s1 < s2!

        Greater than is defined specially to handle wrapping sequence numbers. 

        If the two sequence numbers are close together, it is as normal, but they are far apart, it is assumed that they have wrapped around.

        Thus, sequence_less_than( 0, 1 ) returns true, and so does sequence_greater_than( 65535, 0 )!

        @param s1 The first sequence number.
        @param s2 The second sequence number.

        @returns True if the s1 is less than s2, with sequence number wrapping considered.
     */

    inline bool sequence_less_than( uint16_t s1, uint16_t s2 )
    {
        return sequence_greater_than( s2, s1 );
    }

    /**
        Convert a signed integer to an unsigned integer with zig-zag encoding.

        0,-1,+1,-2,+2... becomes 0,1,2,3,4 ...

        @param n The input value

        @returns The input value converted from signed to unsigned with zig-zag encoding.
     */

    inline int signed_to_unsigned( int n )
    {
        return ( n << 1 ) ^ ( n >> 31 );
    }

    /**
        Convert an unsigned integer to as signed integer with zig-zag encoding.

        0,1,2,3,4... becomes 0,-1,+1,-2,+2...

        @param n The input value

        @returns The input value converted from unsigned to signed with zig-zag encoding.
     */

    inline int unsigned_to_signed( uint32_t n )
    {
        return ( n >> 1 ) ^ ( -int32_t( n & 1 ) );
    }

    /**
        Implementation of the 64 bit murmur hash.

        @param key The input value.
        @param length The length of the key (bytes).
        @param seed The initial seed for the hash. Used to chain together multiple hash calls.

        @returns A 64 bit hash of the input value.
     */

    uint64_t murmur_hash_64( const void * key, uint32_t length, uint64_t seed );

    /**
        Base 64 encode a string.
    
        @param input The input string value. Must be null terminated.
        @param output The output base64 encoded string. Will be null terminated.
        @param output_size The size of the output buffer (bytes). Must be large enough to store the base 64 encoded string.

        @returns The number of bytes in the base64 encoded string, including terminating null. -1 if the base64 encode failed because the output buffer was too small.
     */

    int base64_encode_string( const char * input, char * output, int output_size );

    /**
        Base 64 decode a string.

        @param input The base64 encoded string.
        @param output The decoded string. Guaranteed to be null terminated, even if the base64 is maliciously encoded.
        @param output_size The size of the output buffer (bytes).

        @returns The number of bytes in the decoded string, including terminating null. -1 if the base64 decode failed.
     */

    int base64_decode_string( const char * input, char * output, int output_size );

    /**
        Base 64 encode a block of data.

        @param input The data to encode.
        @param input_length The length of the input data (bytes).
        @param output The output base64 encoded string. Will be null terminated.
        @param output_size The size of the output buffer. Must be large enough to store the base 64 encoded string.

        @returns The number of bytes in the base64 encoded string, including terminating null. -1 if the base64 encode failed because the output buffer was too small.
     */

    int base64_encode_data( const uint8_t * input, int input_length, char * output, int output_size );

    /**
        Base 64 decode a block of data.

        @param input The base 64 string to decode. Must be a null terminated string.
        @param output The output data. Will *not* be null terminated.
        @param output_size The size of the output buffer. Must be large enough to store the decoded data.

        @returns The number of bytes of decoded data. -1 if the base64 decode failed.
     */

    int base64_decode_data( const char * input, uint8_t * output, int output_size );

    /**
        Print bytes with a label. 

        Useful for printing out packets, encryption keys, nonce etc.

        @param label The label to print out before the bytes.
        @param data The data to print out to stdout.
        @param data_bytes The number of bytes of data to print.
     */

    void print_bytes( const char * label, const uint8_t * data, int data_bytes );

    /**
        Debug printf used for spammy logs. 

        Off by default. You can enable these logs by setting \#define YOJIMBO_DEBUG_SPAM 1 in yojimbo_config.h

        This is very useful for debugging things, like breakages in the client/server connection protocol, weird bugs and so on. My last resort!
     */

    void debug_printf( const char * format, ... );

    /**
        A simple bit array class.

        You can create a bit array with a number of bits, set, clear and test if each bit is set.
     */

    class BitArray
    {
    public:

        /**
            The bit array constructor.

            @param allocator The allocator to use.
            @param size The number of bits in the bit array.

            All bits are initially set to zero.
         */

        BitArray( Allocator & allocator, int size )
        {
            assert( size > 0 );
            m_allocator = &allocator;
            m_size = size;
            m_bytes = 8 * ( ( size / 64 ) + ( ( size % 64 ) ? 1 : 0 ) );
            assert( m_bytes > 0 );
            m_data = (uint64_t*) YOJIMBO_ALLOCATE( allocator, m_bytes );
            Clear();
        }

        /**
            The bit array destructor.
         */

        ~BitArray()
        {
            assert( m_data );
            assert( m_allocator );
            YOJIMBO_FREE( *m_allocator, m_data );
            m_allocator = NULL;
        }

        /**
            Clear all bit values to zero.
         */

        void Clear()
        {
            assert( m_data );
            memset( m_data, 0, m_bytes );
        }

        /**
            Set a bit to 1.

            @param index The index of the bit.
         */

        void SetBit( int index )
        {
            assert( index >= 0 );
            assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            assert( bit_index >= 0 );
            assert( bit_index < 64 );
            m_data[data_index] |= uint64_t(1) << bit_index;
        }

        /**
            Clear a bit to 0.

            @param index The index of the bit.
         */

        void ClearBit( int index )
        {
            assert( index >= 0 );
            assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            m_data[data_index] &= ~( uint64_t(1) << bit_index );
        }

        /**
            Get the value of the bit.

            Returns 1 if the bit is set, 0 if the bit is not set.

            @param index The index of the bit.
         */

        uint64_t GetBit( int index ) const
        {
            assert( index >= 0 );
            assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            assert( bit_index >= 0 );
            assert( bit_index < 64 );
            return ( m_data[data_index] >> bit_index ) & 1;
        }

        /**
            Gets the size of the bit array, in number of bits.

            @returns The number of bits.
         */

        int GetSize() const
        {
            return m_size;
        }

    private:

        Allocator * m_allocator;                            ///< Allocator passed in to the constructor.

        int m_size;                                         ///< The size of the bit array in bits.

        int m_bytes;                                        ///< The size of the bit array in bytes.
        
        uint64_t * m_data;                                  ///< The data backing the bit array is an array of 64 bit integer values.

        BitArray( const BitArray & other );
        BitArray & operator = ( const BitArray & other );
    };

    /**
        A simple templated queue.

        This is a FIFO queue. First entry in, first entry out.
     */

    template <typename T> class Queue
    {
    public:

        /**
            Queue constructor.

            @param allocator The allocator to use.
            @param size The maximum number of entries in the queue.
         */

        Queue( Allocator & allocator, int size )
        {
            assert( size > 0 );
            m_arraySize = size;
            m_startIndex = 0;
            m_numEntries = 0;
            m_allocator = &allocator;
            m_entries = (T*) YOJIMBO_ALLOCATE( allocator, sizeof(T) * size );
            memset( m_entries, 0, sizeof(T) * size );
        }

        /**
            Queue destructor.
         */

        ~Queue()
        {
            assert( m_allocator );
            
            YOJIMBO_FREE( *m_allocator, m_entries );

            m_arraySize = 0;
            m_startIndex = 0;
            m_numEntries = 0;

            m_allocator = NULL;
        }

        /**
            Clear all entries in the queue and reset back to default state.
         */

        void Clear()
        {
            m_numEntries = 0;
            m_startIndex = 0;
        }

        /**
            Pop a value off the queue.

            IMPORTANT: This will assert if the queue is empty. Check Queue::IsEmpty or Queue::GetNumEntries first!

            @returns The value popped off the queue.
         */

        T Pop()
        {
            assert( !IsEmpty() );
            const T & entry = m_entries[m_startIndex];
            m_startIndex = ( m_startIndex + 1 ) % m_arraySize;
            m_numEntries--;
            return entry;
        }

        /**
            Push a value on to the queue.

            @param value The value to push onto the queue.

            IMPORTANT: Will assert if the queue is already full. Check Queue::IsFull before calling this!   
         */

        void Push( const T & value )
        {
            assert( !IsFull() );
            const int index = ( m_startIndex + m_numEntries ) % m_arraySize;
            m_entries[index] = value;
            m_numEntries++;
        }

        /**
            Random access for entries in the queue.

            @param index The index into the queue. 0 is the oldest entry, Queue::GetNumEntries() - 1 is the newest.

            @returns The value in the queue at the index.
         */

        T & operator [] ( int index )
        {
            assert( !IsEmpty() );
            assert( index >= 0 );
            assert( index < m_numEntries );
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        /**
            Random access for entries in the queue (const version).

            @param index The index into the queue. 0 is the oldest entry, Queue::GetNumEntries() - 1 is the newest.

            @returns The value in the queue at the index.
         */

        const T & operator [] ( int index ) const
        {
            assert( !IsEmpty() );
            assert( index >= 0 );
            assert( index < m_numEntries );
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        /**
            Get the size of the queue.

            This is the maximum number of values that can be pushed on the queue.

            @returns The size of the queue.
         */

        int GetSize() const
        {
            return m_arraySize;
        }

        /**
            Is the queue currently full?

            @returns True if the queue is full. False otherwise.
         */

        bool IsFull() const
        {
            return m_numEntries == m_arraySize;
        }

        /**
            Is the queue currently empty?

            @returns True if there are no entries in the queue.
         */

        bool IsEmpty() const
        {
            return m_numEntries == 0;
        }

        /**
            Get the number of entries in the queue.

            @returns The number of entries in the queue in [0,GetSize()].
         */

        int GetNumEntries() const
        {
            return m_numEntries;
        }

    private:


        Allocator * m_allocator;                        ///< The allocator passed in to the constructor.

        T * m_entries;                                  ///< Array of entries backing the queue (circular buffer).

        int m_arraySize;                                ///< The size of the array, in number of entries. This is the "size" of the queue.
    
        int m_startIndex;                               ///< The start index for the queue. This is the next value that gets popped off.

        int m_numEntries;                               ///< The number of entries currently stored in the queue.
    };

    /**
        Data structure that stores data indexed by sequence number.

        Entries may or may not exist. If they don't exist the sequence value for the entry at that index is set to 0xFFFFFFFF. 

        This provides a constant time lookup for an entry by sequence number. If the entry at sequence modulo buffer size doesn't have the same sequence number, that sequence number is not stored.

        This is incredibly useful and is used as the foundation of the packet level ack system and the reliable message send and receive queues.

        @see Connection
     */

    template <typename T> class SequenceBuffer
    {
    public:

        /**
            Sequence buffer constructor.

            @param allocator The allocator to use.
            @param size The size of the sequence buffer.
         */

        SequenceBuffer( Allocator & allocator, int size )
        {
            assert( size > 0 );
            m_size = size;
            m_sequence = 0;
            m_allocator = &allocator;
            m_entry_sequence = (uint32_t*) YOJIMBO_ALLOCATE( allocator, sizeof( uint32_t ) * size );
            m_entries = (T*) YOJIMBO_ALLOCATE( allocator, sizeof(T) * size );
            Reset();
        }

        /**
            Sequence buffer destructor.
         */

        ~SequenceBuffer()
        {
            assert( m_allocator );

            YOJIMBO_FREE( *m_allocator, m_entries );
            YOJIMBO_FREE( *m_allocator, m_entry_sequence );

            m_allocator = NULL;
        }

        /**
            Reset the sequence buffer.

            Removes all entries from the sequence buffer and restores it to initial state.
         */

        void Reset()
        {
            m_sequence = 0;
            memset( m_entry_sequence, 0xFF, sizeof( uint32_t ) * m_size );
        }

        /**
            Insert an entry in the sequence buffer.

            IMPORTANT: If another entry exists at the sequence modulo buffer size, it is overwritten.

            @param sequence The sequence number.

            @returns The sequence buffer entry, which you must fill with your data. NULL if a sequence buffer entry could not be added for your sequence number (if the sequence number is too old for example).
         */

        T * Insert( uint16_t sequence )
        {
            if ( sequence_greater_than( sequence + 1, m_sequence ) )
            {
                RemoveEntries( m_sequence, sequence );

                m_sequence = sequence + 1;
            }
            else if ( sequence_less_than( sequence, m_sequence - m_size ) )
            {
                return NULL;
            }

            const int index = sequence % m_size;

            m_entry_sequence[index] = sequence;

            return &m_entries[index];
        }

        /**
            Remove an entry from the sequence buffer.

            @param sequence The sequence number of the entry to remove.
         */

        void Remove( uint16_t sequence )
        {
            m_entry_sequence[ sequence % m_size ] = 0xFFFFFFFF;
        }

        /**
            Is the entry corresponding to the sequence number available? eg. Currently unoccupied.

            This works because older entries are automatically set back to unoccupied state as the sequence buffer advances forward.

            @param sequence The sequence number.

            @returns True if the sequence buffer entry is available, false if it is already occupied.
         */

        bool Available( uint16_t sequence ) const
        {
            return m_entry_sequence[ sequence % m_size ] == 0xFFFFFFFF;
        }

        /**
            Does an entry exist for a sequence number?

            @param sequence The sequence number.

            @returns True if an entry exists for this sequence number.
         */

        bool Exists( uint16_t sequence ) const
        {
            return m_entry_sequence[ sequence % m_size ] == uint32_t( sequence );
        }

        /**
            Get the entry corresponding to a sequence number.

            @param sequence The sequence number.

            @returns The entry if it exists. NULL if no entry is in the buffer for this sequence number.
         */

        T * Find( uint16_t sequence )
        {
            const int index = sequence % m_size;
            if ( m_entry_sequence[index] == uint32_t( sequence ) )
                return &m_entries[index];
            else
                return NULL;
        }

        /**
            Get the entry corresponding to a sequence number (const version).

            @param sequence The sequence number.

            @returns The entry if it exists. NULL if no entry is in the buffer for this sequence number.
         */

        const T * Find( uint16_t sequence ) const
        {
            const int index = sequence % m_size;
            if ( m_entry_sequence[index] == uint32_t( sequence ) )
                return &m_entries[index];
            else
                return NULL;
        }

        /**
            Get the entry at the specified index.
    
            Use this to iterate across entries in the sequence buffer.

            @param index The entry index in [0,GetSize()-1].

            @returns The entry if it exists. NULL if no entry is in the buffer at the specified index.
         */

        T * GetAtIndex( int index )
        {
            assert( index >= 0 );
            assert( index < m_size );
            return m_entry_sequence[index] != 0xFFFFFFFF ? &m_entries[index] : NULL;
        }

        /**
            Get the most recent sequence number added to the buffer.

            This sequence number can wrap around, so if you are at 65535 and add an entry for sequence 0, then 0 becomes the new "most recent" sequence number.

            @returns The most recent sequence number.

            @see yojimbo::sequence_greater_than
            @see yojimbo::sequence_less_than
         */

        uint16_t GetSequence() const 
        {
            return m_sequence;
        }

        /**
            Get the entry index for a sequence number.

            This is simply the sequence number modulo the sequence buffer size.

            @param sequence The sequence number.

            @returns The sequence buffer index corresponding of the sequence number.
         */

        int GetIndex( uint16_t sequence ) const
        {
            return sequence % m_size;
        }

        /** 
            Get the size of the sequence buffer.

            @returns The size of the sequence buffer (number of entries).
         */

        int GetSize() const
        {
            return m_size;
        }

    protected:

        /** 
            Helper function to remove entries.

            This is used to remove old entries as we advance the sequence buffer forward. 

            Otherwise, if when entries are added with holes (eg. receive buffer for packets or messages, where not all sequence numbers are added to the buffer because we have high packet loss), 
            and we are extremely unlucky, we can have old sequence buffer entries from the previous sequence # wrap around still in the buffer, which corrupts our internal connection state.

            This actually happened in the soak test at high packet loss levels (>90%). It took me days to track it down :)
         */

        void RemoveEntries( int start_sequence, int finish_sequence )
        {
            if ( finish_sequence < start_sequence ) 
                finish_sequence += 65535;

            assert( finish_sequence >= start_sequence );

            if ( finish_sequence - start_sequence < m_size )
            {
                for ( int sequence = start_sequence; sequence <= finish_sequence; ++sequence )
                    m_entry_sequence[sequence % m_size] = 0xFFFFFFFF;
            }
            else
            {
                for ( int i = 0; i < m_size; ++i )
                    m_entry_sequence[i] = 0xFFFFFFFF;
            }
        }

    private:

        Allocator * m_allocator;                                            ///< The allocator passed in to the constructor.

        int m_size;                                                         ///< The size of the sequence buffer.
        
        uint16_t m_sequence;                                                ///< The most recent sequence number added to the buffer.

        uint32_t * m_entry_sequence;                                        ///< Array of sequence numbers corresponding to each sequence buffer entry for fast lookup. Set to 0xFFFFFFFF if no entry exists at that index.
        
        T * m_entries;                                                      ///< The sequence buffer entries. This is where the data is stored per-entry. Separate from the sequence numbers for fast lookup (hot/cold split) when the data per-sequence number is relatively large.
        
        SequenceBuffer( const SequenceBuffer<T> & other );

        SequenceBuffer<T> & operator = ( const SequenceBuffer<T> & other );
    };
}

#endif // #ifndef YOJIMBO_COMMON_H
