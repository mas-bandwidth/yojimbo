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

#ifndef YOJIMBO_CHANNEL_H
#define YOJIMBO_CHANNEL_H

#include "yojimbo_message.h"
#include "yojimbo_allocator.h"
#include "yojimbo_bit_array.h"
#include "yojimbo_sequence_buffer.h"

namespace yojimbo
{
    const int MaxChannels = 64;
    const int ConservativeConnectionPacketHeaderEstimate = 80;
    const int ConservativeMessageHeaderEstimate = 32;
    const int ConservativeFragmentHeaderEstimate = 64;
    const int ConservativeChannelHeaderEstimate = 32;

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
        CHANNEL_ERROR_SERIALIZE_MEASURE_FAILED,
        CHANNEL_ERROR_OUT_OF_MEMORY,
        CHANNEL_ERROR_BLOCKS_DISABLED
    };

    struct ChannelConfig
    {
        int messagePacketBudget;                                // maximum bytes of message data per-packet. -1 = no limit
        int maxMessagesPerPacket;                               // maximum number of messages per-packet
        int messageSendQueueSize;                               // message send queue size
        int messageReceiveQueueSize;                            // receive queue size
        float messageResendTime;                                // message resend time (seconds)
        int maxBlockSize;                                       // maximum block size in bytes
        int fragmentSize;                                       // block fragments size in bytes
        float fragmentResendTime;                               // fragment resend time (seconds)
        int sentPacketsSize;                                    // size of sent packets buffer (maps packet level acks to messages & fragments)
        bool disableBlocks;                                     // disable blocks for this channel. saves maxBlockSize * 2 in memory.

        ChannelConfig()
        {
            messagePacketBudget = 1100;
            maxMessagesPerPacket = 64;
            messageSendQueueSize = 1024;
            messageReceiveQueueSize = 1024;
            messageResendTime = 0.1f;
            maxBlockSize = 256 * 1024;
            fragmentSize = 1024;
            fragmentResendTime = 0.25f;
            sentPacketsSize = 1024;
            disableBlocks = false;
        }

        int GetMaxFragmentsPerBlock() const
        {
            return maxBlockSize / fragmentSize;
        }
    };

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

        virtual void OnChannelFragmentReceived( class Channel * /*channel*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/ ) {}
    };

    class Channel
    {
    public:

        Channel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelId );

        ~Channel();

        void Reset();

        bool CanSendMessage() const;

        void SendMessage( Message * message );

        Message * ReceiveMessage();

        void AdvanceTime( double time );

        ChannelError GetError() const;

        bool HasMessagesToSend() const;

        void GetMessagesToSend( uint16_t * messageIds, int & numMessageIds, int remainingPacketBits );

        int GetMessagePacketData( ChannelPacketData & packetData, const uint16_t * messageIds, int numMessageIds );

        void AddMessagePacketEntry( const uint16_t * messageIds, int numMessageIds, uint16_t sequence );

        void ProcessPacketMessages( int numMessages, Message ** messages );

        void ProcessPacketData( ChannelPacketData & packetData );

        void ProcessAck( uint16_t ack );

        void UpdateOldestUnackedMessageId();

        int CalculateMessageOverheadBits();
            
        bool SendingBlockMessage();

        uint8_t * GetFragmentToSend( uint16_t & messageId, uint16_t & fragmentId, int & fragmentBytes, int & numFragments, int & messageType );

        int GetFragmentPacketData( ChannelPacketData & packetData, uint16_t messageId, uint16_t fragmentId, uint8_t * fragmentData, int fragmentSize, int numFragments, int messageType );

        void AddFragmentPacketEntry( uint16_t messageId, uint16_t fragmentId, uint16_t sequence );

        void ProcessPacketFragment( int messageType, uint16_t messageId, int numFragments, uint16_t fragmentId, const uint8_t * fragmentData, int fragmentBytes, BlockMessage * blockMessage );

        Message * GetSendQueueMessage( uint16_t messageId );

        uint64_t GetCounter( int index ) const;

        void SetListener( ChannelListener * listener ) { m_listener = listener; }

        int GetChannelId() const { return m_channelId; }

    private:

        const ChannelConfig m_config;                                                   // const configuration data

        Allocator * m_allocator;                                                        // allocator for allocations matching life cycle of object

        ChannelListener * m_listener;                                                   // channel listener for callbacks. optional.

        MessageFactory * m_messageFactory;                                              // message factory creates and destroys messages

        double m_time;                                                                  // current time

        int m_channelId;                                                                // channel id [0,MaxChannels-1]

        ChannelError m_error;                                                           // channel error level

        int m_messageOverheadBits;                                                      // number of bits overhead per-serialized message

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

        Channel( const Channel & other );

        Channel & operator = ( const Channel & other );
    };
}

#endif
