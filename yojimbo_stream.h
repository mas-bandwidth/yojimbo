/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#ifndef YOJIMBO_STREAM_H
#define YOJIMBO_STREAM_H

#include "yojimbo_config.h"
#include "yojimbo_bitpack.h"
#ifdef DEBUG
#include <stdio.h>
#endif // #ifdef DEBUG

namespace yojimbo
{
    // todo: move this somewhere else, and split apart the stuff that corresponds to stream errors vs. packet read/write errors
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

        WriteStream( uint8_t * buffer, int bytes ) : m_error( YOJIMBO_PROTOCOL_ERROR_NONE ), m_context( NULL ), m_writer( buffer, bytes ) {}

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
            SerializeAlign();
            const uint32_t magic = hash_string( string, 0 );
            SerializeBits( magic, 32 );
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

        void * GetContext() const
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

        ReadStream( const uint8_t * buffer, int bytes ) : m_context( NULL ), m_error( YOJIMBO_PROTOCOL_ERROR_NONE ), m_bitsRead(0), m_reader( buffer, bytes ) {}

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
            SerializeAlign();
            uint32_t value = 0;
            SerializeAlign();
            SerializeBits( value, 32 );
            const uint32_t magic = hash_string( string, 0 );
#ifdef DEBUG
            if ( magic != value )
            {
                printf( "serialize check failed: '%s'. expected %x, got %x\n", string, magic, value );
            }
#endif // #ifdef DEBUG
            return value == magic;
#else // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
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

    private:

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

        MeasureStream( int bytes ) : m_context( NULL ), m_error( YOJIMBO_PROTOCOL_ERROR_NONE ), m_totalBytes( bytes ), m_bitsWritten(0) {}

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

    private:

        void * m_context;
        int m_error;
        int m_totalBytes;
        int m_bitsWritten;
    };
}

#endif // #ifndef YOJIMBO_STREAM_H
