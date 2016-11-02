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

#ifndef YOJIMBO_CHANNEL_H
#define YOJIMBO_CHANNEL_H

#include "yojimbo_message.h"
#include "yojimbo_allocator.h"
#include "yojimbo_queue.h"
#include "yojimbo_sequence_buffer.h"

namespace yojimbo
{
    struct ChannelPacketData
    {
        uint32_t channelId : 16;
        uint32_t blockMessage : 1;

        struct MessageData
        {
            int numMessages;
            Message ** messages;
        };

        struct BlockData
        {
            BlockMessage * message;
            uint8_t * fragmentData;
            uint64_t messageId : 16;
            uint64_t fragmentId : 16;
            uint64_t fragmentSize : 16;
            uint64_t numFragments : 16;
            int messageType;
        };

        union
        {
            MessageData message;                                        // packet data for sending messages
            BlockData block;                                            // packet data for sending a block
        };

        ChannelPacketData()
        {
            channelId = 0;
            blockMessage = 0;
            message.numMessages = 0;
        }

        void Free( MessageFactory & messageFactory );

        template <typename Stream> bool Serialize( Stream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( MeasureStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );
    };

    class ChannelListener
    {
    public:

        virtual ~ChannelListener() {}

        virtual void OnChannelFragmentReceived( class Channel * /*channel*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/ ) {}
    };

    enum ChannelCounters
    {
        CHANNEL_COUNTER_MESSAGES_SENT,                          // number of messages sent
        CHANNEL_COUNTER_MESSAGES_RECEIVED,                      // number of messages received
        CHANNEL_COUNTER_NUM_COUNTERS
    };

    enum ChannelError
    {
        CHANNEL_ERROR_NONE = 0,
        CHANNEL_ERROR_DESYNC,
        CHANNEL_ERROR_SEND_QUEUE_FULL,
        CHANNEL_ERROR_OUT_OF_MEMORY,
        CHANNEL_ERROR_BLOCKS_DISABLED
    };

    class Channel
    {
    public:

        Channel() : m_channelId(0), m_error( CHANNEL_ERROR_NONE ) {}

        virtual ~Channel() {}

        virtual void SetListener( ChannelListener * listener ) = 0;

        virtual void Reset() = 0;

        virtual bool CanSendMsg() const = 0;

        virtual void SendMsg( Message * message ) = 0;

        virtual Message * ReceiveMsg() = 0;

        virtual void AdvanceTime( double time ) = 0;

        virtual int GetPacketData( ChannelPacketData & packetData, uint16_t packetSequence, int availableBits ) = 0;

        virtual void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence ) = 0;

        virtual void ProcessAck( uint16_t sequence ) = 0;

        ChannelError GetError() const { return m_error; }

        int GetChannelId() const { return m_channelId; }

    protected:

        void SetError( ChannelError error )
        {
            m_error = error;
        }

        void SetChannelId( int channelId )
        {
            assert( channelId >= 0 );
            assert( channelId < MaxChannels );
            m_channelId = channelId;
        }

    private:

        int m_channelId;
        
        ChannelError m_error;
    };

    class ReliableOrderedChannel : public Channel
    {
    public:

        ReliableOrderedChannel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelId );

        ~ReliableOrderedChannel();

        void Reset();

        bool CanSendMsg() const;

        void SendMsg( Message * message );

        Message * ReceiveMsg();

        void AdvanceTime( double time );

        int GetPacketData( ChannelPacketData & packetData, uint16_t packetSequence, int availableBits );

        bool HasMessagesToSend() const;

        int GetMessagesToSend( uint16_t * messageIds, int & numMessageIds, int remainingPacketBits );

        void GetMessagePacketData( ChannelPacketData & packetData, const uint16_t * messageIds, int numMessageIds );

        void AddMessagePacketEntry( const uint16_t * messageIds, int numMessageIds, uint16_t sequence );

        void ProcessPacketMessages( int numMessages, Message ** messages );

        void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence );

        void ProcessAck( uint16_t ack );

        void UpdateOldestUnackedMessageId();

        bool SendingBlockMessage();

        uint8_t * GetFragmentToSend( uint16_t & messageId, uint16_t & fragmentId, int & fragmentBytes, int & numFragments, int & messageType );

        int GetFragmentPacketData( ChannelPacketData & packetData, uint16_t messageId, uint16_t fragmentId, uint8_t * fragmentData, int fragmentSize, int numFragments, int messageType );

        void AddFragmentPacketEntry( uint16_t messageId, uint16_t fragmentId, uint16_t sequence );

        void ProcessPacketFragment( int messageType, uint16_t messageId, int numFragments, uint16_t fragmentId, const uint8_t * fragmentData, int fragmentBytes, BlockMessage * blockMessage );

        Message * GetSendQueueMessage( uint16_t messageId );

        uint64_t GetCounter( int index ) const;

        void SetListener( ChannelListener * listener ) { m_listener = listener; }

    private:

        const ChannelConfig m_config;                                                   // const configuration data

        Allocator * m_allocator;                                                        // allocator for allocations matching life cycle of object

        ChannelListener * m_listener;                                                   // channel listener for callbacks. optional.

        MessageFactory * m_messageFactory;                                              // message factory creates and destroys messages

        double m_time;                                                                  // current time

        uint16_t m_sendMessageId;                                                       // id for next message added to send queue

        uint16_t m_receiveMessageId;                                                    // id for next message to be received

        uint16_t m_oldestUnackedMessageId;                                              // id for oldest unacked message in send queue

        SequenceBuffer<MessageSendQueueEntry> * m_messageSendQueue;                     // message send queue

        SequenceBuffer<MessageSentPacketEntry> * m_messageSentPackets;                  // messages in sent packets (for acks)

        SequenceBuffer<MessageReceiveQueueEntry> * m_messageReceiveQueue;               // message receive queue

        uint16_t * m_sentPacketMessageIds;                                              // array of message ids, n ids per-sent packet

        SendBlockData * m_sendBlock;                                                    // block being sent

        ReceiveBlockData * m_receiveBlock;                                              // block being received

        uint64_t m_counters[CHANNEL_COUNTER_NUM_COUNTERS];                              // counters for unit testing, stats etc.

    private:

        ReliableOrderedChannel( const ReliableOrderedChannel & other );

        ReliableOrderedChannel & operator = ( const ReliableOrderedChannel & other );
    };

    class UnreliableUnorderedChannel : public Channel
    {
    public:

        UnreliableUnorderedChannel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelId );

        ~UnreliableUnorderedChannel();

        void Reset();

        bool CanSendMsg() const;

        void SendMsg( Message * message );

        Message * ReceiveMsg();

        void AdvanceTime( double time );

        int GetPacketData( ChannelPacketData & packetData, uint16_t packetSequence, int availableBits );

        void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence );

        void ProcessAck( uint16_t ack );

        uint64_t GetCounter( int index ) const;

        void SetListener( ChannelListener * listener ) { m_listener = listener; }

    private:

        const ChannelConfig m_config;                                                   // const configuration data

        Allocator * m_allocator;                                                        // allocator for allocations matching life cycle of object

        ChannelListener * m_listener;                                                   // channel listener for callbacks. optional.

        MessageFactory * m_messageFactory;                                              // message factory creates and destroys messages

        Queue<Message*> * m_messageSendQueue;                                           // message send queue. messages that don't fit in the next packet are discarded.

        Queue<Message*> * m_messageReceiveQueue;                                        // message receive queue. should generally be larger than the send queue.

        uint64_t m_counters[CHANNEL_COUNTER_NUM_COUNTERS];                              // counters for unit testing, stats etc.

    private:

        UnreliableUnorderedChannel( const UnreliableUnorderedChannel & other );

        UnreliableUnorderedChannel & operator = ( const UnreliableUnorderedChannel & other );
    };
}

#endif
