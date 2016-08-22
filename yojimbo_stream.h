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

#ifndef YOJIMBO_STREAM_H
#define YOJIMBO_STREAM_H

#include "yojimbo_config.h"
#include "yojimbo_bitpack.h"
#include "yojimbo_allocator.h"
#ifdef DEBUG
#include <stdio.h>
#endif // #ifdef DEBUG

namespace yojimbo
{
    #define YOJIMBO_PROTOCOL_ERROR_NONE                         0
    #define YOJIMBO_PROTOCOL_ERROR_CRC32_MISMATCH               1
    #define YOJIMBO_PROTOCOL_ERROR_INVALID_PACKET_TYPE          2
    #define YOJIMBO_PROTOCOL_ERROR_PACKET_TYPE_NOT_ALLOWED      3
    #define YOJIMBO_PROTOCOL_ERROR_CREATE_PACKET_FAILED         4
    #define YOJIMBO_PROTOCOL_ERROR_SERIALIZE_HEADER_FAILED      5
    #define YOJIMBO_PROTOCOL_ERROR_SERIALIZE_PACKET_FAILED      6
    #define YOJIMBO_PROTOCOL_ERROR_SERIALIZE_CHECK_FAILED       7
    #define YOJIMBO_PROTOCOL_ERROR_STREAM_OVERFLOW              8
    #define YOJIMBO_PROTOCOL_ERROR_STREAM_ABORTED               9

    class WriteStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        WriteStream( uint8_t * buffer, int bytes, Allocator & allocator = GetDefaultAllocator() ) 
            : m_allocator( &allocator ), m_context( NULL ), m_error( YOJIMBO_PROTOCOL_ERROR_NONE ), m_writer( buffer, bytes ) {}

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

        bool SerializeBytes( const uint8_t * data, int bytes )
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
#if YOJIMBO_SERIALIZE_CHECKS
            if ( !SerializeAlign() )
                return false;
            const uint32_t magic = hash_string( string, 0 );
            if ( !SerializeBits( magic, 32 ) )
                return false;
#else // #if YOJIMBO_SERIALIZE_CHECKS
            (void)string;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
        }

        void Flush()
        {
            m_writer.FlushBits();
        }

        const uint8_t * GetData() const
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

        void SetContext( void *context )
        {
            m_context = context;
        }

        void * GetContext() const
        {
            return m_context;
        }

        int GetError() const
        {
            return m_error;
        }

        Allocator & GetAllocator()
        {
            return *m_allocator;
        }

    private:

        Allocator * m_allocator;
        void * m_context;
        int m_error;
        BitWriter m_writer;
    };

    class ReadStream
    {
    public:

        enum { IsWriting = 0 };
        enum { IsReading = 1 };

        ReadStream( const uint8_t * buffer, int bytes, Allocator & allocator = GetDefaultAllocator() ) 
            : m_allocator( &allocator ), m_context( NULL ), m_error( YOJIMBO_PROTOCOL_ERROR_NONE ), m_bitsRead(0), m_reader( buffer, bytes ) {}

        bool SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            assert( min < max );
            const int bits = bits_required( min, max );
            if ( m_reader.WouldOverflow( bits ) )
            {
                m_error = YOJIMBO_PROTOCOL_ERROR_STREAM_OVERFLOW;
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
                m_error = YOJIMBO_PROTOCOL_ERROR_STREAM_OVERFLOW;
                return false;
            }
            uint32_t read_value = m_reader.ReadBits( bits );
            value = read_value;
            m_bitsRead += bits;
            return true;
        }

        bool SerializeBytes( uint8_t * data, int bytes )
        {
            if ( !SerializeAlign() )
                return false;
            if ( m_reader.WouldOverflow( bytes * 8 ) )
            {
                m_error = YOJIMBO_PROTOCOL_ERROR_STREAM_OVERFLOW;
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
                m_error = YOJIMBO_PROTOCOL_ERROR_STREAM_OVERFLOW;
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
#if YOJIMBO_SERIALIZE_CHECKS            
            if ( !SerializeAlign() )
                return false;
            uint32_t value = 0;
            if ( !SerializeBits( value, 32 ) )
                return false;
            const uint32_t magic = hash_string( string, 0 );
#ifdef DEBUG
            if ( magic != value )
            {
                printf( "serialize check failed: '%s'. expected %x, got %x\n", string, magic, value );
            }
#endif // #ifdef DEBUG
            return value == magic;
#else // #if YOJIMBO_SERIALIZE_CHECKS
            (void)string;
            return true;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
        }

        int GetBitsProcessed() const
        {
            return m_bitsRead;
        }

        int GetBytesProcessed() const
        {
            return ( m_bitsRead + 7 ) / 8;
        }

        void SetContext( void * context )
        {
            m_context = context;
        }

        void * GetContext() const
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

        Allocator & GetAllocator()
        {
            return *m_allocator;
        }

    private:

        Allocator * m_allocator;
        void * m_context;
        int m_error;
        int m_bitsRead;
        BitReader m_reader;
    };

    class MeasureStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        explicit MeasureStream( Allocator & allocator = GetDefaultAllocator() ) 
            : m_allocator( &allocator ), m_context( NULL ), m_bitsWritten(0) {}

        bool SerializeInteger( int32_t value, int32_t min, int32_t max )
        {   
            (void)value;
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

        bool SerializeBytes( const uint8_t * /*data*/, int bytes )
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
#if YOJIMBO_SERIALIZE_CHECKS
            SerializeAlign();
            m_bitsWritten += 32;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
        }

        int GetBitsProcessed() const
        {
            return m_bitsWritten;
        }

        int GetBitsRemaining() const
        {
            return 0;
        }

        int GetBytesProcessed() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

        int GetTotalBytes() const
        {
            return 0;
        }

        int GetTotalBits() const
        {
            return 0;
        }

        void SetContext( void * context )
        {
            m_context = context;
        }

        void * GetContext() const
        {
            return m_context;
        }

        int GetError() const
        {
            return YOJIMBO_PROTOCOL_ERROR_NONE;
        }

        Allocator & GetAllocator()
        {
            return *m_allocator;
        }

    private:

        Allocator * m_allocator;
        void * m_context;
        int m_bitsWritten;
    };
}

#endif // #ifndef YOJIMBO_STREAM_H
