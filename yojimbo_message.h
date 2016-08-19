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

#include "yojimbo_stream.h"
#include "yojimbo_serialize.h"
#include "yojimbo_allocator.h"
#include "yojimbo_bit_array.h"

#if YOJIMBO_DEBUG_MESSAGE_LEAKS
#include <map>
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS

namespace yojimbo
{
    class Message : public Serializable
    {
    public:

        Message( int blockMessage = 0 ) : m_refCount(1), m_id(0), m_type(0), m_blockMessage( blockMessage ) {}

        void AssignId( uint16_t id ) { m_id = id; }

        int GetId() const { return m_id; }

        int GetType() const { return m_type; }

        int GetRefCount() { return m_refCount; }

        bool IsBlockMessage() const { return m_blockMessage; }

        virtual bool SerializeInternal( ReadStream & stream ) = 0;

        virtual bool SerializeInternal( WriteStream & stream ) = 0;

        virtual bool SerializeInternal ( MeasureStream & stream ) = 0;

    protected:

        void SetType( int type ) { m_type = type; }

        void AddRef() { m_refCount++; }

        void Release() { assert( m_refCount > 0 ); m_refCount--; }

        virtual ~Message()
        {
            assert( m_refCount == 0 );
        }

    private:

        friend class MessageFactory;
      
        Message( const Message & other );
        
        const Message & operator = ( const Message & other );

        int m_refCount;
        uint32_t m_id : 16;
        uint32_t m_type : 15;       
        uint32_t m_blockMessage : 1;
    };

    class BlockMessage : public Message
    {
    public:

        explicit BlockMessage() : Message( 1 ), m_allocator(NULL), m_blockData(NULL), m_blockSize(0) {}

        ~BlockMessage()
        {
            if ( m_allocator )
            {
                m_allocator->Free( m_blockData );
                m_blockSize = 0;
                m_blockData = NULL;
                m_allocator = NULL;
            }
        }

        void AttachBlock( Allocator & allocator, uint8_t * blockData, int blockSize )
        {
            assert( blockData );
            assert( blockSize > 0 );
            assert( !m_blockData );

            m_allocator = &allocator;
            m_blockData = blockData;
            m_blockSize = blockSize;
        }

        void DetachBlock()
        {
            m_allocator = NULL;
            m_blockData = NULL;
            m_blockSize = 0;
        }

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

        template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

        YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();

    private:

        Allocator * m_allocator;
        uint8_t * m_blockData;
        int m_blockSize;
    };

    enum MessageFactoryError
    {
        MESSAGE_FACTORY_ERROR_NONE,
        MESSAGE_FACTORY_ERROR_FAILED_TO_ALLOCATE_MESSAGE,
        MESSAGE_FACTORY_ERROR_ALLOCATOR_IS_EXHAUSTED
    };

    class MessageFactory
    {        
        #if YOJIMBO_DEBUG_MESSAGE_LEAKS
        std::map<void*,int> allocated_messages;
        #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS

        Allocator * m_allocator;
        int m_numTypes;
        int m_error;

    public:

        MessageFactory( Allocator & allocator, int numTypes )
        {
            m_allocator = &allocator;
            m_numTypes = numTypes;
            m_error = MESSAGE_FACTORY_ERROR_NONE;
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

            if ( !message )
            {
                m_error = MESSAGE_FACTORY_ERROR_FAILED_TO_ALLOCATE_MESSAGE;
                return NULL;
            }

            #if YOJIMBO_DEBUG_MESSAGE_LEAKS
            allocated_messages[message] = 1;
            assert( allocated_messages.find( message ) != allocated_messages.end() );
            #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS

            return message;
        }

        void AddRef( Message * message )
        {
            assert( message );
            if ( !message )
                return;

            message->AddRef();
        }

        void Release( Message * message )
        {
            assert( message );
            if ( !message )
                return;

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

        int GetError() const
        {
            return m_allocator->GetError() ? MESSAGE_FACTORY_ERROR_ALLOCATOR_IS_EXHAUSTED : m_error;
        }

        void ClearError()
        {
            m_allocator->ClearError();
            m_error = MESSAGE_FACTORY_ERROR_NONE;
        }

    protected:

        void SetMessageType( Message * message, int type ) { message->SetType( type ); }

        virtual Message * CreateInternal( int /*type*/ ) { return NULL; }
    };

    struct MessageSendQueueEntry
    {
        Message * message;
        double timeLastSent;
        uint32_t measuredBits : 31;
        uint32_t block : 1;
    };

    struct MessageSentPacketEntry
    {
        double timeSent;
        uint16_t * messageIds;
        uint32_t numMessageIds : 16;                 // number of messages in this packet
        uint32_t acked : 1;                          // 1 if this sent packet has been acked
        uint64_t block : 1;                          // 1 if this sent packet contains a block fragment
        uint64_t blockMessageId : 16;                // block id. valid only when sending block.
        uint64_t blockFragmentId : 16;               // fragment id. valid only when sending block.
    };

    struct MessageReceiveQueueEntry
    {
        Message * message;
    };

    struct SendBlockData
    {
        SendBlockData( Allocator & allocator, int maxBlockSize, int maxFragmentsPerBlock )
        {
            m_allocator = &allocator;
            ackedFragment = YOJIMBO_NEW( allocator, BitArray, allocator, maxFragmentsPerBlock );
            fragmentSendTime = (double*) allocator.Allocate( sizeof( double) * maxFragmentsPerBlock );
            blockData = (uint8_t*) allocator.Allocate( maxBlockSize );            
            assert( ackedFragment && blockData && fragmentSendTime );
            Reset();
        }

        ~SendBlockData()
        {
            YOJIMBO_DELETE( *m_allocator, BitArray, ackedFragment );
            m_allocator->Free( blockData );
            m_allocator->Free( fragmentSendTime );
            fragmentSendTime = NULL;
            blockData = NULL;
        }

        void Reset()
        {
            active = false;
            numFragments = 0;
            numAckedFragments = 0;
            blockMessageId = 0;
            blockSize = 0;
        }

        bool active;                                                    // true if we are currently sending a block
        int numFragments;                                               // number of fragments in the current block being sent
        int numAckedFragments;                                          // number of acked fragments in current block being sent
        int blockSize;                                                  // send block size in bytes
        uint16_t blockMessageId;                                        // the message id of the block being sent
        BitArray * ackedFragment;                                       // has fragment n been received?
        double * fragmentSendTime;                                      // time fragment was last sent in seconds.
        uint8_t * blockData;                                            // block data storage as it is received.

    private:

        Allocator * m_allocator;                                        // allocator used to free the data on shutdown
    
        SendBlockData( const SendBlockData & other );
        
        SendBlockData & operator = ( const SendBlockData & other );
    };

    struct ReceiveBlockData
    {
        ReceiveBlockData( Allocator & allocator, int maxBlockSize, int maxFragmentsPerBlock )
        {
            m_allocator = &allocator;
            receivedFragment = YOJIMBO_NEW( allocator, BitArray, allocator, maxFragmentsPerBlock );
            blockData = (uint8_t*) allocator.Allocate( maxBlockSize );            
            assert( receivedFragment && blockData );
            blockMessage = NULL;
            Reset();
        }

        ~ReceiveBlockData()
        {
            YOJIMBO_DELETE( *m_allocator, BitArray, receivedFragment );
            m_allocator->Free( blockData );
            blockData = NULL;
        }

        void Reset()
        {
            active = false;
            numFragments = 0;
            numReceivedFragments = 0;
            messageId = 0;
            messageType = 0;
            blockSize = 0;
        }

        bool active;                                                    // true if we are currently receiving a block
        int numFragments;                                               // number of fragments in this block
        int numReceivedFragments;                                       // number of fragments received.
        uint16_t messageId;                                             // message id of block being currently received.
        int messageType;                                                // message type of the block being received.
        uint32_t blockSize;                                             // block size in bytes.
        BitArray * receivedFragment;                                    // has fragment n been received?
        uint8_t * blockData;                                            // block data for receive
        BlockMessage * blockMessage;                                    // block message (sent with fragment 0)

    private:

        Allocator * m_allocator;                                        // allocator used to free the data on shutdown

        ReceiveBlockData( const ReceiveBlockData & other );
        
        ReceiveBlockData & operator = ( const ReceiveBlockData & other );
    };
}

#define YOJIMBO_MESSAGE_FACTORY_START( factory_class, base_factory_class, num_message_types )                                           \
                                                                                                                                        \
    class factory_class : public base_factory_class                                                                                     \
    {                                                                                                                                   \
    public:                                                                                                                             \
        factory_class( yojimbo::Allocator & allocator = yojimbo::GetDefaultAllocator(), int numMessageTypes = num_message_types )       \
         : base_factory_class( allocator, numMessageTypes ) {}                                                                          \
        yojimbo::Message * CreateInternal( int type )                                                                                   \
        {                                                                                                                               \
            yojimbo::Message * message = base_factory_class::CreateInternal( type );                                                    \
            if ( message )                                                                                                              \
                return message;                                                                                                         \
            yojimbo::Allocator & allocator = GetAllocator();                                                                            \
            (void)allocator;                                                                                                            \
            switch ( type )                                                                                                             \
            {                                                                                                                           \


#define YOJIMBO_DECLARE_MESSAGE_TYPE( message_type, message_class )                                                 \
                                                                                                                    \
                case message_type:                                                                                  \
                    message = YOJIMBO_NEW( allocator, message_class );                                              \
                    if ( !message )                                                                                 \
                        return NULL;                                                                                \
                    SetMessageType( message, message_type );                                                        \
                    return message;

#define YOJIMBO_MESSAGE_FACTORY_FINISH()                                                                            \
                                                                                                                    \
                default: return NULL;                                                                               \
            }                                                                                                       \
        }                                                                                                           \
    };

#endif
