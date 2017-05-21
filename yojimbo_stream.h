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

#ifndef YOJIMBO_STREAM_H
#define YOJIMBO_STREAM_H

#include "yojimbo_config.h"
#include "yojimbo_bitpack.h"
#include "yojimbo_allocator.h"
#ifndef NDEBUG
#include <stdio.h>
#endif // #ifndef NDEBUG

/** @file */

namespace yojimbo
{
    /** 
        Functionality common to all stream classes.
     */

    class BaseStream
    {
    public:

        /**
            Base stream constructor.

            @param allocator The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.
         */

        explicit BaseStream( Allocator & allocator ) : m_allocator( &allocator ), m_context( NULL ), m_userContext( NULL ) {}

        /**
            Set a context on the stream.

            Contexts are used by the library supply data that is needed to read and write packets.

            Specifically, this context is used by the connection to supply data needed to read and write connection packets.

            If you are using the yojimbo client/server or connection classes you should NOT set this manually. It's already taken!

            However, if you are using only the low-level parts of yojimbo, feel free to take this over and use it for whatever you want.

            @see ConnectionContext
            @see ConnectionPacket
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
            Get the allocator set on the stream.

            You can use this allocator to dynamically allocate memory while reading and writing packets.

            @returns The stream allocator.
         */

        Allocator & GetAllocator()
        {
            return *m_allocator;
        }

    private:

        Allocator * m_allocator;                            ///< The allocator passed into the constructor.
        void * m_context;                                   ///< The context pointer set on the stream. May be NULL.
    };

    /**
        Stream class for writing bitpacked data.

        This class is a wrapper around the bit writer class. Its purpose is to provide unified interface for reading and writing.

        You can determine if you are writing to a stream by calling Stream::IsWriting inside your templated serialize method.

        This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.

        IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead. See test/shared.h for some examples.

        @see BitWriter
     */

    class WriteStream : public BaseStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        /**
            Write stream constructor.

            @param buffer The buffer to write to.
            @param bytes The number of bytes in the buffer. Must be a multiple of four.
            @param allocator The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.
         */

        WriteStream( uint8_t * buffer, int bytes, Allocator & allocator = GetDefaultAllocator() ) : BaseStream( allocator ), m_writer( buffer, bytes ) {}

        /**
            Serialize an integer (write).

            @param value The integer value in [min,max].
            @param min The minimum value.
            @param max The maximum value.

            @returns Always returns true. All checking is performed by debug asserts only on write.
         */

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

        /**
            Serialize a number of bits (write).

            @param value The unsigned integer value to serialize. Must be in range [0,(1<<bits)-1].
            @param bits The number of bits to write in [1,32].

            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBits( uint32_t value, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );
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
            assert( data );
            assert( bytes >= 0 );
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
            Serialize a safety check to the stream (write).

            Safety checks help track down desyncs. A check is written to the stream, and on the other side if the check is not present it asserts and fails the serialize.

            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeCheck()
        {
#if YOJIMBO_SERIALIZE_CHECKS
            SerializeAlign();
            SerializeBits( SerializeCheckValue, 32 );
#else // #if YOJIMBO_SERIALIZE_CHECKS
            (void)string;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
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

        BitWriter m_writer;                                 ///< The bit writer used for all bitpacked write operations.
    };

    /**
        Stream class for reading bitpacked data.

        This class is a wrapper around the bit reader class. Its purpose is to provide unified interface for reading and writing.

        You can determine if you are reading from a stream by calling Stream::IsReading inside your templated serialize method.

        This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.

        IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead. See test/shared.h for some examples.

        @see BitReader
     */

    class ReadStream : public BaseStream
    {
    public:

        enum { IsWriting = 0 };
        enum { IsReading = 1 };

        /**
            Read stream constructor.

            @param buffer The buffer to read from.
            @param bytes The number of bytes in the buffer. May be a non-multiple of four, however if it is, the underlying buffer allocated should be large enough to read the any remainder bytes as a dword.
            @param allocator The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.
         */

        ReadStream( const uint8_t * buffer, int bytes, Allocator & allocator = GetDefaultAllocator() ) : BaseStream( allocator ), m_reader( buffer, bytes ) {}

        /**
            Serialize an integer (read).

            @param value The integer value read is stored here. It is guaranteed to be in [min,max] if this function succeeds.
            @param min The minimum allowed value.
            @param max The maximum allowed value.

            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
         */

        bool SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            assert( min < max );
            const int bits = bits_required( min, max );
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint32_t unsigned_value = m_reader.ReadBits( bits );
            value = (int32_t) unsigned_value + min;
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
            assert( bits > 0 );
            assert( bits <= 32 );
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
            if ( !SerializeAlign() )
                return false;
            if ( m_reader.WouldReadPastEnd( bytes * 8 ) )
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
            Serialize a safety check from the stream (read).

            Safety checks help track down desyncs. A check is written to the stream, and on the other side if the check is not present it asserts and fails the serialize.

            @returns Returns true if the serialize check passed. False otherwise.
         */

        bool SerializeCheck()
        {
#if YOJIMBO_SERIALIZE_CHECKS            
            if ( !SerializeAlign() )
                return false;
            uint32_t value = 0;
            if ( !SerializeBits( value, 32 ) )
                return false;
            if ( value != SerializeCheckValue )
            {
                debug_printf( "serialize check failed: expected %x, got %x\n", SerializeCheckValue, value );
            }
            return value == SerializeCheckValue;
#else // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
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

        BitReader m_reader;                                 ///< The bit reader used for all bitpacked read operations.
    };

    /**
        Stream class for estimating how many bits it would take to serialize something.

        This class acts like a bit writer (IsWriting is 1, IsReading is 0), but it doesn't actually write anything, it just counts how many bits would be written.

        It's used by the connection channel classes to work out how many messages will fit in the channel packet budget.

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

            @param allocator The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.
         */

        explicit MeasureStream( Allocator & allocator = GetDefaultAllocator() ) : BaseStream( allocator ), m_bitsWritten(0) {}

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
            assert( min < max );
            assert( value >= min );
            assert( value <= max );
            const int bits = bits_required( min, max );
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
            assert( bits > 0 );
            assert( bits <= 32 );
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
            Serialize a safety check to the stream (measure).

            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeCheck()
        {
#if YOJIMBO_SERIALIZE_CHECKS
            SerializeAlign();
            m_bitsWritten += 32;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
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

        int m_bitsWritten;                                  ///< Counter for the number of bits written.
    };
}

#endif // #ifndef YOJIMBO_STREAM_H
