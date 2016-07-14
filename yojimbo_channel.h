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
        CHANNEL_ERROR_OUT_OF_MEMORY
    };

    struct ChannelConfig
    {
        int packetType;                                         // connect packet type (override)
        int maxPacketSize;                                      // maximum connection packet size in bytes
        int slidingWindowSize;                                  // size of ack sliding window (in packets)
        float messageResendRate;                                // message max resend rate in seconds, until acked.
        int messageSendQueueSize;                               // message send queue size
        int messageReceiveQueueSize;                            // receive queue size
        int messagePacketBudget;                                // budget of how many bytes messages can take up in the connection packet
        int maxMessagesPerPacket;                               // maximum number of messages per-packet
        int maxBlockSize;                                       // maximum block size in bytes
        int fragmentSize;                                       // size of block fragments sent in packets (bytes)
        float fragmentResendRate;                               // min seconds to wait before resending the same fragment

        ChannelConfig()
        {
            packetType = 0;
            maxPacketSize = 4 * 1024;
            slidingWindowSize = 1024;
            messageResendRate = 0.1f;
            messageSendQueueSize = 1024;
            messageReceiveQueueSize = 1024;
            messagePacketBudget = 1024;
            maxMessagesPerPacket = 64;
            maxBlockSize = 256 * 1024;
            fragmentSize = 1024;
            fragmentResendRate = 0.25f;
        }

        int GetMaxFragmentsPerBlock() const
        {
            return maxBlockSize / fragmentSize;
        }
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
    };

    class Channel
    {
    public:

        Channel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config = ChannelConfig() );

        ~Channel();

        void Reset();

        bool CanSendMessage() const;

        void SendMessage( Message * message );

        Message * ReceiveMessage();

        void AdvanceTime( double time );

        ChannelError GetError() const;

        bool HasMessagesToSend() const;

        void GetMessagesToSend( uint16_t * messageIds, int & numMessageIds );

        void AddMessagePacketEntry( const uint16_t * messageIds, int numMessageIds, uint16_t sequence );

        void ProcessPacketMessages( int numMessages, Message ** messages );

        void ProcessAck( uint16_t ack );

        void UpdateOldestUnackedMessageId();

        int CalculateMessageOverheadBits();
            
        bool SendingBlockMessage();

        uint8_t * GetFragmentToSend( uint16_t & messageId, uint16_t & fragmentId, int & fragmentBytes, int & numFragments, int & messageType );

        void AddFragmentPacketEntry( uint16_t messageId, uint16_t fragmentId, uint16_t sequence );

        void ProcessPacketFragment( int messageType, uint16_t messageId, int numFragments, uint16_t fragmentId, const uint8_t * fragmentData, int fragmentBytes, BlockMessage * blockMessage );

        Message * GetSendQueueMessage( uint16_t messageId );

        uint64_t GetCounter( int index ) const;

    private:

        const ChannelConfig m_config;                                                   // const configuration data

        Allocator * m_allocator;                                                        // allocator for allocations matching life cycle of object

        MessageFactory * m_messageFactory;                                              // message factory creates and destroys messages

        double m_time;                                                                  // current time

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
