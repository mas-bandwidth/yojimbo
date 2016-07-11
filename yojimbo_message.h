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

#ifndef YOJIMBO_MESSAGE_H
#define YOJIMBO_MESSAGE_H

#include "yojimbo_allocator.h"

namespace yojimbo
{
    // todo: wrap message magic with #if YOJIMBO_MESSAGE_MAGIC and have it off by default

    const int MessageMagic = 0x12345;

    class Message : public Serializable
    {
    public:

        Message( int type, int blockMessage = 0 ) : m_magic( MessageMagic ), m_refCount(1), m_id(0), m_type( type ), m_blockMessage( blockMessage ) {}

        void AssignId( uint16_t id ) { assert( m_magic == MessageMagic ); m_id = id; }

        int GetId() const { assert( m_magic == MessageMagic ); return m_id; }

        int GetType() const { assert( m_magic == MessageMagic ); return m_type; }

        int GetRefCount() { assert( m_magic == MessageMagic ); return m_refCount; }

        bool IsBlockMessage() const { assert( m_magic == MessageMagic ); return m_blockMessage; }

        virtual bool SerializeInternal( ReadStream & stream ) = 0;

        virtual bool SerializeInternal( WriteStream & stream ) = 0;

        virtual bool SerializeInternal ( MeasureStream & stream ) = 0;

    protected:

        void AddRef() { m_refCount++; }

        void Release() { assert( m_magic == MessageMagic ); assert( m_refCount > 0 ); m_refCount--; }

        virtual ~Message()
        {
            assert( m_magic == MessageMagic );
            assert( m_refCount == 0 );
            m_magic = 0;
        }

    private:

        friend class MessageFactory;
      
        Message( const Message & other );
        
        const Message & operator = ( const Message & other );

        uint32_t m_magic;
        int m_refCount;
        uint32_t m_id : 16;
        uint32_t m_type : 15;       
        uint32_t m_blockMessage : 1;
    };

    class BlockMessage : public Message
    {
    public:

        explicit BlockMessage( int type ) : Message( type, 1 ), m_allocator(NULL), m_blockSize(0), m_blockData(NULL) {}

        ~BlockMessage()
        {
            Disconnect();
        }

        void Connect( Allocator & allocator, uint8_t * blockData, int blockSize )
        {
            assert( blockData );
            assert( blockSize > 0 );
            assert( !m_blockData );

            m_allocator = &allocator;
            m_blockData = blockData;
            m_blockSize = blockSize;
        }

        void Disconnect()
        {
            if ( m_allocator )
            {
                m_allocator->Free( m_blockData );
                m_blockSize = 0;
                m_blockData = NULL;
                m_allocator = NULL;
            }
        }

        template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

        YOJIMBO_SERIALIZE_FUNCTIONS();

        Allocator * GetAllocator()
        {
            return m_allocator;
        }

        uint8_t * GetBlockData()
        {
            return m_blockData;
        }

        int GetBlockSize() const
        {
            return m_blockSize;
        }

    private:

        Allocator * m_allocator;
        int m_blockSize;
        uint8_t * m_blockData;
    };

    class MessageFactory
    {        
        #if YOJIMBO_DEBUG_MESSAGE_LEAKS
        std::map<void*,int> allocated_messages;
        #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS

        Allocator * m_allocator;

        int m_numTypes;

    public:

        MessageFactory( Allocator & allocator, int numTypes )
        {
            m_allocator = &allocator;
            m_numTypes = numTypes;
        }

        ~MessageFactory()
        {
            assert( m_allocator );
            m_allocator = NULL;

            #if YOJIMBO_DEBUG_MESSAGE_LEAKS
            if ( allocated_messages.size() )
            {
                printf( "you leaked messages!\n" );
                printf( "%d messages leaked\n", (int) allocated_messages.size() );
                typedef std::map<void*,int>::iterator itor_type;
                for ( itor_type i = allocated_messages.begin(); i != allocated_messages.end(); ++i ) 
                {
                    Message * message = (Message*) i->first;
                    printf( "leaked message %p (type %d, refcount %d)\n", message, message->GetType(), message->GetRefCount() );
                }
                exit(1);
            }
            #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
        }

        Message * Create( int type )
        {
            assert( type >= 0 );
            assert( type < m_numTypes );

            Message * message = CreateInternal( type );

            assert( message );

            #if YOJIMBO_DEBUG_MESSAGE_LEAKS
            allocated_messages[message] = 1;
            assert( allocated_messages.find( message ) != allocated_messages.end() );
            #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS

            return message;
        }

        void AddRef( Message * message )
        {
            assert( message );
            
            message->AddRef();
        }

        void Release( Message * message )
        {
            assert( message );

            message->Release();
            
            if ( message->GetRefCount() == 0 )
            {
                #if YOJIMBO_DEBUG_MESSAGE_LEAKS
                assert( allocated_messages.find( message ) != allocated_messages.end() );
                allocated_messages.erase( message );
                #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
            
                assert( m_allocator );

                YOJIMBO_DELETE( *m_allocator, Message, message );
            }
        }

        int GetNumTypes() const
        {
            return m_numTypes;
        }

        Allocator & GetAllocator()
        {
            assert( m_allocator );
            return *m_allocator;
        }

    protected:

        virtual Message * CreateInternal( int type ) = 0;
    };
}

#endif
