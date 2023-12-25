/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2024, Mas Bandwidth LLC.

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

#ifndef YOJIMBO_H
#define YOJIMBO_H

#include "serialize.h"
#include "yojimbo_config.h"
#include "yojimbo_constants.h"
#include "yojimbo_bit_array.h"
#include "yojimbo_utils.h"
#include "yojimbo_queue.h"
#include "yojimbo_sequence_buffer.h"
#include "yojimbo_address.h"
#include "yojimbo_serialize.h"
#include "yojimbo_message.h"

/** @file */

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <inttypes.h>
#if YOJIMBO_DEBUG_MESSAGE_LEAKS
#include <map>
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS

// windows =p
#ifdef SendMessage
#undef SendMessage
#endif

struct netcode_address_t;
struct netcode_server_t;
struct netcode_client_t;
struct reliable_endpoint_t;

/// The library namespace.

namespace yojimbo
{
    using namespace serialize;
}

/**
    Initialize the yojimbo library.
    Call this before calling any yojimbo library functions.
    @returns True if the library was successfully initialized, false otherwise.
 */

bool InitializeYojimbo();

/**
    Shutdown the yojimbo library.
    Call this after you finish using the library and it will run some checks for you (for example, checking for memory leaks in debug build).
 */

void ShutdownYojimbo();

// ---------------------------------

namespace yojimbo
{
    struct ChannelPacketData
    {
        uint32_t channelIndex : 16;
        uint32_t initialized : 1;
        uint32_t blockMessage : 1;
        uint32_t messageFailedToSerialize : 1;

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
            MessageData message;
            BlockData block;
        };

        void Initialize();

        void Free( MessageFactory & messageFactory );

        template <typename Stream> bool Serialize( Stream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( MeasureStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );
    };

    /**
        Channel counters provide insight into the number of times an action was performed by a channel.
        They are intended for use in a telemetry system, eg. reported to some backend logging system to track behavior in a production environment.
     */

    enum ChannelCounters
    {
        CHANNEL_COUNTER_MESSAGES_SENT,                          ///< Number of messages sent over this channel.
        CHANNEL_COUNTER_MESSAGES_RECEIVED,                      ///< Number of messages received over this channel.
        CHANNEL_COUNTER_NUM_COUNTERS                            ///< The number of channel counters.
    };

    /**
        Channel error level.
        If the channel gets into an error state, it sets an error state on the corresponding connection. See yojimbo::CONNECTION_ERROR_CHANNEL.
        This way if any channel on a client/server connection gets into a bad state, that client is automatically kicked from the server.
        @see Client
        @see Server
        @see Connection
     */

    enum ChannelErrorLevel
    {
        CHANNEL_ERROR_NONE = 0,                                 ///< No error. All is well.
        CHANNEL_ERROR_DESYNC,                                   ///< This channel has desynced. This means that the connection protocol has desynced and cannot recover. The client should be disconnected.
        CHANNEL_ERROR_SEND_QUEUE_FULL,                          ///< The user tried to send a message but the send queue was full. This will assert out in development, but in production it sets this error on the channel.
        CHANNEL_ERROR_BLOCKS_DISABLED,                          ///< The channel received a packet containing data for blocks, but this channel is configured to disable blocks. See ChannelConfig::disableBlocks.
        CHANNEL_ERROR_FAILED_TO_SERIALIZE,                      ///< Serialize read failed for a message sent to this channel. Check your message serialize functions, one of them is returning false on serialize read. This can also be caused by a desync in message read and write.
        CHANNEL_ERROR_OUT_OF_MEMORY,                            ///< The channel tried to allocate some memory but couldn't.
    };

    /// Helper function to convert a channel error to a user friendly string.

    inline const char * GetChannelErrorString( ChannelErrorLevel error )
    {
        switch ( error )
        {
            case CHANNEL_ERROR_NONE:                    return "none";
            case CHANNEL_ERROR_DESYNC:                  return "desync";
            case CHANNEL_ERROR_SEND_QUEUE_FULL:         return "send queue full";
            case CHANNEL_ERROR_OUT_OF_MEMORY:           return "out of memory";
            case CHANNEL_ERROR_BLOCKS_DISABLED:         return "blocks disabled";
            case CHANNEL_ERROR_FAILED_TO_SERIALIZE:     return "failed to serialize";
            default:
                yojimbo_assert( false );
                return "(unknown)";
        }
    }

    /// Common functionality shared across all channel types.

    class Channel
    {
    public:

        /**
            Channel constructor.
         */

        Channel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time );

        /**
            Channel destructor.
         */

        virtual ~Channel() {}

        /**
            Reset the channel.
         */

        virtual void Reset() = 0;

        /**
            Returns true if a message can be sent over this channel.
         */

        virtual bool CanSendMessage() const = 0;

        /**
            Are there any messages in the send queue?
            @returns True if there is at least one message in the send queue.
         */

         virtual bool HasMessagesToSend() const = 0;

        /**
            Queue a message to be sent across this channel.
            @param message The message to be sent.
         */

        virtual void SendMessage( Message * message, void *context) = 0;

        /**
            Pops the next message off the receive queue if one is available.
            @returns A pointer to the received message, NULL if there are no messages to receive. The caller owns the message object returned by this function and is responsible for releasing it via Message::Release.
         */

        virtual Message * ReceiveMessage() = 0;

        /**
            Advance channel time.
            Called by Connection::AdvanceTime for each channel configured on the connection.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Get channel packet data for this channel.
            @param packetData The channel packet data to be filled [out]
            @param packetSequence The sequence number of the packet being generated.
            @param availableBits The maximum number of bits of packet data the channel is allowed to write.
            @returns The number of bits of packet data written by the channel.
            @see ConnectionPacket
            @see Connection::GeneratePacket
         */

        virtual int GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits) = 0;

        /**
            Process packet data included in a connection packet.
            @param packetData The channel packet data to process.
            @param packetSequence The sequence number of the connection packet that contains the channel packet data.
            @see ConnectionPacket
            @see Connection::ProcessPacket
         */

        virtual void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence ) = 0;

        /**
            Process a connection packet ack.
            Depending on the channel type:
                1. Acks messages and block fragments so they stop being included in outgoing connection packets (reliable-ordered channel),
                2. Does nothing at all (unreliable-unordered).
            @param sequence The sequence number of the connection packet that was acked.
         */

        virtual void ProcessAck( uint16_t sequence ) = 0;

    public:

        /**
            Get the channel error level.
            @returns The channel error level.
         */

        ChannelErrorLevel GetErrorLevel() const;

        /**
            Gets the channel index.
            @returns The channel index in [0,numChannels-1].
         */

        int GetChannelIndex() const;

        /**
            Get a counter value.
            @param index The index of the counter to retrieve. See ChannelCounters.
            @returns The value of the counter.
            @see ResetCounters
         */

        uint64_t GetCounter( int index ) const;

        /**
            Resets all counter values to zero.
         */

        void ResetCounters();

    protected:

        /**
            Set the channel error level.
            All errors go through this function to make debug logging easier.
         */

        void SetErrorLevel( ChannelErrorLevel errorLevel );

    protected:

        const ChannelConfig m_config;                                                   ///< Channel configuration data.
        Allocator * m_allocator;                                                        ///< Allocator for allocations matching life cycle of this channel.
        int m_channelIndex;                                                             ///< The channel index in [0,numChannels-1].
        double m_time;                                                                  ///< The current time.
        ChannelErrorLevel m_errorLevel;                                                 ///< The channel error level.
        MessageFactory * m_messageFactory;                                              ///< Message factory for creating and destroying messages.
        uint64_t m_counters[CHANNEL_COUNTER_NUM_COUNTERS];                              ///< Counters for unit testing, stats etc.
    };

    /**
        Messages sent across this channel are guaranteed to arrive in the order they were sent.
        This channel type is best used for control messages and RPCs.
        Messages sent over this channel are included in connection packets until one of those packets is acked. Messages are acked individually and remain in the send queue until acked.
        Blocks attached to messages sent over this channel are split up into fragments. Each fragment of the block is included in a connection packet until one of those packets are acked. Eventually, all fragments are received on the other side, and block is reassembled and attached to the message.
        Only one message block may be in flight over the network at any time, so blocks stall out message delivery slightly. Therefore, only use blocks for large data that won't fit inside a single connection packet where you actually need the channel to split it up into fragments. If your block fits inside a packet, just serialize it inside your message serialize via serialize_bytes instead.
     */

    class ReliableOrderedChannel : public Channel
    {
    public:

        /**
            Reliable ordered channel constructor.
            @param allocator The allocator to use.
            @param messageFactory Message factory for creating and destroying messages.
            @param config The configuration for this channel.
            @param channelIndex The channel index in [0,numChannels-1].
         */

        ReliableOrderedChannel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time );

        /**
            Reliable ordered channel destructor.
            Any messages still in the send or receive queues will be released.
         */

        ~ReliableOrderedChannel();

        void Reset();

        bool CanSendMessage() const;

        void SendMessage( Message * message, void *context );

        Message * ReceiveMessage();

        void AdvanceTime( double time );

        int GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits );

        void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence );

        void ProcessAck( uint16_t ack );

        /**
            Are there any unacked messages in the send queue?
            Messages are acked individually and remain in the send queue until acked.
            @returns True if there is at least one unacked message in the send queue.
         */

        bool HasMessagesToSend() const;

        /**
            Get messages to include in a packet.
            Messages are measured to see how many bits they take, and only messages that fit within the channel packet budget will be included. See ChannelConfig::packetBudget.
            Takes care not to send messages too rapidly by respecting ChannelConfig::messageResendTime for each message, and to only include messages that that the receiver is able to buffer in their receive queue. In other words, won't run ahead of the receiver.
            @param messageIds Array of message ids to be filled [out]. Fills up to ChannelConfig::maxMessagesPerPacket messages, make sure your array is at least this size.
            @param numMessageIds The number of message ids written to the array.
            @param remainingPacketBits Number of bits remaining in the packet. Considers this as a hard limit when determining how many messages can fit into the packet.
            @returns Estimate of the number of bits required to serialize the messages (upper bound).
            @see GetMessagePacketData
         */

        int GetMessagesToSend( uint16_t * messageIds, int & numMessageIds, int remainingPacketBits, void *context );

        /**
            Fill channel packet data with messages.
            This is the payload function to fill packet data while sending regular messages (without blocks attached).
            Messages have references added to them when they are added to the packet. They also have a reference while they are stored in a send or receive queue. Messages are cleaned up when they are no longer in a queue, and no longer referenced by any packets.
            @param packetData The packet data to fill [out]
            @param messageIds Array of message ids identifying which messages to add to the packet from the message send queue.
            @param numMessageIds The number of message ids in the array.
            @see GetMessagesToSend
         */

        void GetMessagePacketData( ChannelPacketData & packetData, const uint16_t * messageIds, int numMessageIds );

        /**
            Add a packet entry for the set of messages included in a packet.
            This lets us look up the set of messages that were included in that packet later on when it is acked, so we can ack those messages individually.
            @param messageIds The set of message ids that were included in the packet.
            @param numMessageIds The number of message ids in the array.
            @param sequence The sequence number of the connection packet the messages were included in.
         */

        void AddMessagePacketEntry( const uint16_t * messageIds, int numMessageIds, uint16_t sequence );

        /**
            Process messages included in a packet.
            Any messages that have not already been received are added to the message receive queue. Messages that are added to the receive queue have a reference added. See Message::AddRef.
            @param numMessages The number of messages to process.
            @param messages Array of pointers to messages.
         */

        void ProcessPacketMessages( int numMessages, Message ** messages );

        /**
            Track the oldest unacked message id in the send queue.
            Because messages are acked individually, the send queue is not a true queue and may have holes.
            Because of this it is necessary to periodically walk forward from the previous oldest unacked message id, to find the current oldest unacked message id.
            This lets us know our starting point for considering messages to include in the next packet we send.
            @see GetMessagesToSend
         */

        void UpdateOldestUnackedMessageId();

        /**
            True if we are currently sending a block message.
            Block messages are treated differently to regular messages.
            Regular messages are small so we try to fit as many into the packet we can. See ReliableChannelData::GetMessagesToSend.
            Blocks attached to block messages are usually larger than the maximum packet size or channel budget, so they are split up fragments.
            While in the mode of sending a block message, each channel packet data generated has exactly one fragment from the current block in it. Fragments keep getting included in packets until all fragments of that block are acked.
            @returns True if currently sending a block message over the network, false otherwise.
            @see BlockMessage
            @see GetFragmentToSend
         */

        bool SendingBlockMessage();

        /**
            Get the next block fragment to send.
            The next block fragment is selected by scanning left to right over the set of fragments in the block, skipping over any fragments that have already been acked or have been sent within ChannelConfig::fragmentResendTime.
            @param messageId The id of the message that the block is attached to [out].
            @param fragmentId The id of the fragment to send [out].
            @param fragmentBytes The size of the fragment in bytes.
            @param numFragments The total number of fragments in this block.
            @param messageType The type of message the block is attached to. See MessageFactory.
            @returns Pointer to the fragment data.
         */

        uint8_t * GetFragmentToSend( uint16_t & messageId, uint16_t & fragmentId, int & fragmentBytes, int & numFragments, int & messageType );

        /**
            Fill the packet data with block and fragment data.
            This is the payload function that fills the channel packet data while we are sending a block message.
            @param packetData The packet data to fill [out]
            @param messageId The id of the message that the block is attached to.
            @param fragmentId The id of the block fragment being sent.
            @param fragmentData The fragment data.
            @param fragmentSize The size of the fragment data (bytes).
            @param numFragments The number of fragments in the block.
            @param messageType The type of message the block is attached to.
            @returns An estimate of the number of bits required to serialize the block message and fragment data (upper bound).
         */

        int GetFragmentPacketData( ChannelPacketData & packetData,
                                   uint16_t messageId,
                                   uint16_t fragmentId,
                                   uint8_t * fragmentData,
                                   int fragmentSize,
                                   int numFragments,
                                   int messageType );

        /**
            Adds a packet entry for the fragment.
            This lets us look up the fragment that was in the packet later on when it is acked, so we can ack that block fragment.
            @param messageId The message id that the block was attached to.
            @param fragmentId The fragment id.
            @param sequence The sequence number of the packet the fragment was included in.
         */

        void AddFragmentPacketEntry( uint16_t messageId, uint16_t fragmentId, uint16_t sequence );

        /**
            Process a packet fragment.
            The fragment is added to the set of received fragments for the block. When all packet fragments are received, that block is reconstructed, attached to the block message and added to the message receive queue.
            @param messageType The type of the message this block fragment is attached to. This is used to make sure this message type actually allows blocks to be attached to it.
            @param messageId The id of the message the block fragment belongs to.
            @param numFragments The number of fragments in the block.
            @param fragmentId The id of the fragment in [0,numFragments-1].
            @param fragmentData The fragment data.
            @param fragmentBytes The size of the fragment data in bytes.
            @param blockMessage Pointer to the block message. Passed this in only with the first fragment (0), pass NULL for all other fragments.
         */

        void ProcessPacketFragment( int messageType,
                                    uint16_t messageId,
                                    int numFragments,
                                    uint16_t fragmentId,
                                    const uint8_t * fragmentData,
                                    int fragmentBytes,
                                    BlockMessage * blockMessage );

    protected:

        /**
            An entry in the send queue of the reliable-ordered channel.
            Messages stay into the send queue until acked. Each message is acked individually, so there can be "holes" in the message send queue.
         */

        struct MessageSendQueueEntry
        {
            Message * message;                                                          ///< Pointer to the message. When inserted in the send queue the message has one reference. It is released when the message is acked and removed from the send queue.
            double timeLastSent;                                                        ///< The time the message was last sent. Used to implement ChannelConfig::messageResendTime.
            uint32_t measuredBits : 31;                                                 ///< The number of bits the message takes up in a bit stream.
            uint32_t block : 1;                                                         ///< 1 if this is a block message. Block messages are treated differently to regular messages when sent over a reliable-ordered channel.
        };

        /**
            An entry in the receive queue of the reliable-ordered channel.
         */

        struct MessageReceiveQueueEntry
        {
            Message * message;                                                          ///< The message pointer. Has at a reference count of at least 1 while in the receive queue. Ownership of the message is passed back to the caller when the message is dequeued.
        };

        /**
            Maps packet level acks to messages and fragments for the reliable-ordered channel.
         */

        struct SentPacketEntry
        {
            double timeSent;                                                            ///< The time the packet was sent. Used to estimate round trip time.
            uint16_t * messageIds;                                                      ///< Pointer to an array of message ids. Dynamically allocated because the user can configure the maximum number of messages in a packet per-channel with ChannelConfig::maxMessagesPerPacket.
            uint32_t numMessageIds : 16;                                                ///< The number of message ids in in the array.
            uint32_t acked : 1;                                                         ///< 1 if this packet has been acked.
            uint64_t block : 1;                                                         ///< 1 if this packet contains a fragment of a block message.
            uint64_t blockMessageId : 16;                                               ///< The block message id. Valid only if "block" is 1.
            uint64_t blockFragmentId : 16;                                              ///< The block fragment id. Valid only if "block" is 1.
        };

        /**
            Internal state for a block being sent across the reliable ordered channel.
            Stores the block data and tracks which fragments have been acked. The block send completes when all fragments have been acked.
            IMPORTANT: Although there can be multiple block messages in the message send and receive queues, only one data block can be in flights over the wire at a time.
         */

        struct SendBlockData
        {
            SendBlockData( Allocator & allocator, int maxFragmentsPerBlock )
            {
                m_allocator = &allocator;
                ackedFragment = YOJIMBO_NEW( allocator, BitArray, allocator, maxFragmentsPerBlock );
                fragmentSendTime = (double*) YOJIMBO_ALLOCATE( allocator, sizeof( double) * maxFragmentsPerBlock );
                yojimbo_assert( ackedFragment );
                yojimbo_assert( fragmentSendTime );
                Reset();
            }

            ~SendBlockData()
            {
                YOJIMBO_DELETE( *m_allocator, BitArray, ackedFragment );
                YOJIMBO_FREE( *m_allocator, fragmentSendTime );
            }

            void Reset()
            {
                active = false;
                numFragments = 0;
                numAckedFragments = 0;
                blockMessageId = 0;
                blockSize = 0;
            }

            bool active;                                                                ///< True if we are currently sending a block.
            int blockSize;                                                              ///< The size of the block (bytes).
            int numFragments;                                                           ///< Number of fragments in the block being sent.
            int numAckedFragments;                                                      ///< Number of acked fragments in the block being sent.
            uint16_t blockMessageId;                                                    ///< The message id the block is attached to.
            BitArray * ackedFragment;                                                   ///< Has fragment n been received?
            double * fragmentSendTime;                                                  ///< Last time fragment was sent.

        private:

            Allocator * m_allocator;                                                    ///< Allocator used to create the block data.

            SendBlockData( const SendBlockData & other );

            SendBlockData & operator = ( const SendBlockData & other );
        };

        /**
            Internal state for a block being received across the reliable ordered channel.
            Stores the fragments received over the network for the block, and completes once all fragments have been received.
            IMPORTANT: Although there can be multiple block messages in the message send and receive queues, only one data block can be in flights over the wire at a time.
         */

        struct ReceiveBlockData
        {
            ReceiveBlockData( Allocator & allocator, int maxBlockSize, int maxFragmentsPerBlock )
            {
                m_allocator = &allocator;
                receivedFragment = YOJIMBO_NEW( allocator, BitArray, allocator, maxFragmentsPerBlock );
                blockData = (uint8_t*) YOJIMBO_ALLOCATE( allocator, maxBlockSize );
                yojimbo_assert( receivedFragment && blockData );
                blockMessage = NULL;
                Reset();
            }

            ~ReceiveBlockData()
            {
                YOJIMBO_DELETE( *m_allocator, BitArray, receivedFragment );
                YOJIMBO_FREE( *m_allocator, blockData );
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

            bool active;                                                                ///< True if we are currently receiving a block.
            int numFragments;                                                           ///< The number of fragments in this block
            int numReceivedFragments;                                                   ///< The number of fragments received.
            uint16_t messageId;                                                         ///< The message id corresponding to the block.
            int messageType;                                                            ///< Message type of the block being received.
            uint32_t blockSize;                                                         ///< Block size in bytes.
            BitArray * receivedFragment;                                                ///< Has fragment n been received?
            uint8_t * blockData;                                                        ///< Block data for receive.
            BlockMessage * blockMessage;                                                ///< Block message (sent with fragment 0).

        private:

            Allocator * m_allocator;                                                    ///< Allocator used to free the data on shutdown.

            ReceiveBlockData( const ReceiveBlockData & other );

            ReceiveBlockData & operator = ( const ReceiveBlockData & other );
        };

    private:

        uint16_t m_sendMessageId;                                                       ///< Id of the next message to be added to the send queue.
        uint16_t m_receiveMessageId;                                                    ///< Id of the next message to be added to the receive queue.
        uint16_t m_oldestUnackedMessageId;                                              ///< Id of the oldest unacked message in the send queue.
        SequenceBuffer<SentPacketEntry> * m_sentPackets;                                ///< Stores information per sent connection packet about messages and block data included in each packet. Used to walk from connection packet level acks to message and data block fragment level acks.
        SequenceBuffer<MessageSendQueueEntry> * m_messageSendQueue;                     ///< Message send queue.
        SequenceBuffer<MessageReceiveQueueEntry> * m_messageReceiveQueue;               ///< Message receive queue.
        uint16_t * m_sentPacketMessageIds;                                              ///< Array of n message ids per sent connection packet. Allows the maximum number of messages per-packet to be allocated dynamically.
        SendBlockData * m_sendBlock;                                                    ///< Data about the block being currently sent.
        ReceiveBlockData * m_receiveBlock;                                              ///< Data about the block being currently received.

    private:

        ReliableOrderedChannel( const ReliableOrderedChannel & other );

        ReliableOrderedChannel & operator = ( const ReliableOrderedChannel & other );
    };

    /**
        Messages sent across this channel are not guaranteed to arrive, and may be received in a different order than they were sent.
        This channel type is best used for time critical data like snapshots and object state.
     */

    class UnreliableUnorderedChannel : public Channel
    {
    public:

        /**
            Reliable ordered channel constructor.
            @param allocator The allocator to use.
            @param messageFactory Message factory for creating and destroying messages.
            @param config The configuration for this channel.
            @param channelIndex The channel index in [0,numChannels-1].
         */

        UnreliableUnorderedChannel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time );

        /**
            Unreliable unordered channel destructor.
            Any messages still in the send or receive queues will be released.
         */

        ~UnreliableUnorderedChannel();

        void Reset();

        bool CanSendMessage() const;

        bool HasMessagesToSend() const;

        void SendMessage( Message * message, void *context );

        Message * ReceiveMessage();

        void AdvanceTime( double time );

        int GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits );

        void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence );

        void ProcessAck( uint16_t ack );

    protected:

        Queue<Message*> * m_messageSendQueue;                   ///< Message send queue.
        Queue<Message*> * m_messageReceiveQueue;                ///< Message receive queue.

    private:

        UnreliableUnorderedChannel( const UnreliableUnorderedChannel & other );

        UnreliableUnorderedChannel & operator = ( const UnreliableUnorderedChannel & other );
    };

    /// Connection error level.

    enum ConnectionErrorLevel
    {
        CONNECTION_ERROR_NONE = 0,                              ///< No error. All is well.
        CONNECTION_ERROR_CHANNEL,                               ///< A channel is in an error state.
        CONNECTION_ERROR_ALLOCATOR,                             ///< The allocator is an error state.
        CONNECTION_ERROR_MESSAGE_FACTORY,                       ///< The message factory is in an error state.
        CONNECTION_ERROR_READ_PACKET_FAILED,                    ///< Failed to read packet. Received an invalid packet?
    };

    /**
        Sends and receives messages across a set of user defined channels.
     */

    class Connection
    {
    public:

        Connection( Allocator & allocator, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, double time );

        ~Connection();

        void Reset();

        bool CanSendMessage( int channelIndex ) const;

        bool HasMessagesToSend( int channelIndex ) const;

        void SendMessage( int channelIndex, Message * message, void *context = 0);

        Message * ReceiveMessage( int channelIndex );

        void ReleaseMessage( Message * message );

        bool GeneratePacket( void * context, uint16_t packetSequence, uint8_t * packetData, int maxPacketBytes, int & packetBytes );

        bool ProcessPacket( void * context, uint16_t packetSequence, const uint8_t * packetData, int packetBytes );

        void ProcessAcks( const uint16_t * acks, int numAcks );

        void AdvanceTime( double time );

        ConnectionErrorLevel GetErrorLevel() { return m_errorLevel; }

    private:

        Allocator * m_allocator;                                ///< Allocator passed in to the connection constructor.
        MessageFactory * m_messageFactory;                      ///< Message factory for creating and destroying messages.
        ConnectionConfig m_connectionConfig;                    ///< Connection configuration.
        Channel * m_channel[MaxChannels];                       ///< Array of connection channels. Array size corresponds to m_connectionConfig.numChannels
        ConnectionErrorLevel m_errorLevel;                      ///< The connection error level.
    };

    /**
        Simulates packet loss, latency, jitter and duplicate packets.
        This is useful during development, so your game is tested and played under real world conditions, instead of ideal LAN conditions.
        This simulator works on packet send. This means that if you want 125ms of latency (round trip), you must to add 125/2 = 62.5ms of latency to each side.
     */

    class NetworkSimulator
    {
    public:

        /**
            Create a network simulator.
            Initial network conditions are set to:
                Latency: 0ms
                Jitter: 0ms
                Packet Loss: 0%
                Duplicates: 0%
            @param allocator The allocator to use.
            @param numPackets The maximum number of packets that can be stored in the simulator at any time.
            @param time The initial time value in seconds.
         */

        NetworkSimulator( Allocator & allocator, int numPackets, double time );

        /**
            Network simulator destructor.
            Any packet data still in the network simulator is destroyed.
         */

        ~NetworkSimulator();

        /**
            Set the latency in milliseconds.
            This latency is added on packet send. To simulate a round trip time of 100ms, add 50ms of latency to both sides of the connection.
            @param milliseconds The latency to add in milliseconds.
         */

        void SetLatency( float milliseconds );

        /**
            Set the packet jitter in milliseconds.
            Jitter is applied +/- this amount in milliseconds. To be truly effective, jitter must be applied together with some latency.
            @param milliseconds The amount of jitter to add in milliseconds (+/-).
         */

        void SetJitter( float milliseconds );

        /**
            Set the amount of packet loss to apply on send.
            @param percent The packet loss percentage. 0% = no packet loss. 100% = all packets are dropped.
         */

        void SetPacketLoss( float percent );

        /**
            Set percentage chance of packet duplicates.
            If the duplicate chance succeeds, a duplicate packet is added to the queue with a random delay of up to 1 second.
            @param percent The percentage chance of a packet duplicate being sent. 0% = no duplicate packets. 100% = all packets have a duplicate sent.
         */

        void SetDuplicates( float percent );

        /**
            Is the network simulator active?
            The network simulator is active when packet loss, latency, duplicates or jitter are non-zero values.
            This is used by the transport to know whether it should shunt packets through the simulator, or send them directly to the network. This is a minor optimization.
         */

        bool IsActive() const;

        /**
            Queue a packet to send.
            IMPORTANT: Ownership of the packet data pointer is *not* transferred to the network simulator. It makes a copy of the data instead.
            @param to The slot index the packet should be sent to.
            @param packetData The packet data.
            @param packetBytes The packet size (bytes).
         */

        void SendPacket( int to, uint8_t * packetData, int packetBytes );

        /**
            Receive packets sent to any address.
            IMPORTANT: You take ownership of the packet data you receive and are responsible for freeing it. See NetworkSimulator::GetAllocator.
            @param maxPackets The maximum number of packets to receive.
            @param packetData Array of packet data pointers to be filled [out].
            @param packetBytes Array of packet sizes to be filled [out].
            @param to Array of to indices to be filled [out].
            @returns The number of packets received.
         */

        int ReceivePackets( int maxPackets, uint8_t * packetData[], int packetBytes[], int to[] );

        /**
            Discard all packets in the network simulator.
            This is useful if the simulator needs to be reset and used for another purpose.
         */

        void DiscardPackets();

        /**
            Discard packets sent to a particular client index.
            This is called when a client disconnects from the server.
         */

        void DiscardClientPackets( int clientIndex );

        /**
            Advance network simulator time.
            You must pump this regularly otherwise the network simulator won't work.
            @param time The current time value. Please make sure you use double values for time so you retain sufficient precision as time increases.
         */

        void AdvanceTime( double time );

        /**
            Get the allocator to use to free packet data.
            @returns The allocator that packet data is allocated with.
         */

        Allocator & GetAllocator() { yojimbo_assert( m_allocator ); return *m_allocator; }

    protected:

        /**
            Helper function to update the active flag whenever network settings are changed.
            Active is set to true if any of the network conditions are non-zero. This allows you to quickly check if the network simulator is active and would actually do something.
         */

        void UpdateActive();

    private:

        Allocator * m_allocator;                        ///< The allocator passed in to the constructor. It's used to allocate and free packet data.
        float m_latency;                                ///< Latency in milliseconds
        float m_jitter;                                 ///< Jitter in milliseconds +/-
        float m_packetLoss;                             ///< Packet loss percentage.
        float m_duplicates;                             ///< Duplicate packet percentage
        bool m_active;                                  ///< True if network simulator is active, eg. if any of the network settings above are enabled.

        /// A packet buffered in the network simulator.

        struct PacketEntry
        {
            PacketEntry()
            {
                to = 0;
                deliveryTime = 0.0;
                packetData = NULL;
                packetBytes = 0;
            }

            int to;                                     ///< To index this packet should be sent to (for server -> client packets).
            double deliveryTime;                        ///< Delivery time for this packet (seconds).
            uint8_t * packetData;                       ///< Packet data (owns this pointer).
            int packetBytes;                            ///< Size of packet in bytes.
        };

        double m_time;                                  ///< Current time from last call to advance time.
        int m_currentIndex;                             ///< Current index in the packet entry array. New packets are inserted here.
        int m_numPacketEntries;                         ///< Number of elements in the packet entry array.
        PacketEntry * m_packetEntries;                  ///< Pointer to dynamically allocated packet entries. This is where buffered packets are stored.
    };

    /**
        Specifies the message factory and callbacks for clients and servers.
        An instance of this class is passed into the client and server constructors.
        You can share the same adapter across a client/server pair if you have local multiplayer, eg. loopback.
     */

    class Adapter
    {
    public:

        virtual ~Adapter() {}

        /**
            Override this function to specify your own custom allocator class.
            @param allocator The base allocator that must be used to allocate your allocator instance.
            @param memory The block of memory backing your allocator.
            @param bytes The number of bytes of memory available to your allocator.
            @returns A pointer to the allocator instance you created.
         */

        virtual Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes )
        {
            return YOJIMBO_NEW( allocator, TLSF_Allocator, memory, bytes );
        }

        /**
            You must override this method to create the message factory used by the client and server.
            @param allocator The allocator that must be used to create your message factory instance via YOJIMBO_NEW
            @returns The message factory pointer you created.

         */

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator )
        {
            (void) allocator;
            yojimbo_assert( false );
            return NULL;
        }

        /**
            Override this callback to process packets sent from client to server over loopback.
            @param clientIndex The client index in range [0,maxClients-1]
            @param packetData The packet data (raw) to be sent to the server.
            @param packetBytes The number of packet bytes in the server.
            @param packetSequence The sequence number of the packet.
            @see Client::ConnectLoopback
         */

        virtual void ClientSendLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
        {
            (void) clientIndex;
            (void) packetData;
            (void) packetBytes;
            (void) packetSequence;
            yojimbo_assert( false );
        }

        /**
            Override this callback to process packets sent from client to server over loopback.
            @param clientIndex The client index in range [0,maxClients-1]
            @param packetData The packet data (raw) to be sent to the server.
            @param packetBytes The number of packet bytes in the server.
            @param packetSequence The sequence number of the packet.
            @see Server::ConnectLoopbackClient
         */

        virtual void ServerSendLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
        {
            (void) clientIndex;
            (void) packetData;
            (void) packetBytes;
            (void) packetSequence;
            yojimbo_assert( false );
        }

        /**
            Override this to get a callback when a client connects on the server.
         */

        virtual void OnServerClientConnected( int clientIndex )
        {
            (void) clientIndex;
        }

        /**
            Override this to get a callback when a client disconnects from the server.
         */

        virtual void OnServerClientDisconnected( int clientIndex )
        {
            (void) clientIndex;
        }
    };

    /**
        Network information for a connection.
        Contains statistics like round trip time (RTT), packet loss %, bandwidth estimates, number of packets sent, received and acked.
     */

    struct NetworkInfo
    {
        float RTT;                                  ///< Round trip time estimate (milliseconds).
        float packetLoss;                           ///< Packet loss percent.
        float sentBandwidth;                        ///< Sent bandwidth (kbps).
        float receivedBandwidth;                    ///< Received bandwidth (kbps).
        float ackedBandwidth;                       ///< Acked bandwidth (kbps).
        uint64_t numPacketsSent;                    ///< Number of packets sent.
        uint64_t numPacketsReceived;                ///< Number of packets received.
        uint64_t numPacketsAcked;                   ///< Number of packets acked.
    };

    /**
        The server interface.
     */

    class ServerInterface
    {
    public:

        virtual ~ServerInterface() {}

        /**
            Set the context for reading and writing packets.
            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.
            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time.
            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Start the server and allocate client slots.
            Each client that connects to this server occupies one of the client slots allocated by this function.
            @param maxClients The number of client slots to allocate. Must be in range [1,MaxClients]
            @see Server::Stop
         */

        virtual void Start( int maxClients ) = 0;

        /**
            Stop the server and free client slots.
            Any clients that are connected at the time you call stop will be disconnected.
            When the server is stopped, clients cannot connect to the server.
            @see Server::Start.
         */

        virtual void Stop() = 0;

        /**
            Disconnect the client at the specified client index.
            @param clientIndex The index of the client to disconnect in range [0,maxClients-1], where maxClients is the number of client slots allocated in Server::Start.
            @see Server::IsClientConnected
         */

        virtual void DisconnectClient( int clientIndex ) = 0;

        /**
            Disconnect all clients from the server.
            Client slots remain allocated as per the last call to Server::Start, they are simply made available for new clients to connect.
         */

        virtual void DisconnectAllClients() = 0;

        /**
            Send packets to connected clients.
            This function drives the sending of packets that transmit messages to clients.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from connected clients.
            This function drives the procesing of messages included in packets received from connected clients.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance server time.
            Call this at the end of each frame to advance the server time forward.
            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the server running?
            The server is running after you have called Server::Start. It is not running before the first server start, and after you call Server::Stop.
            Clients can only connect to the server while it is running.
            @returns true if the server is currently running.
         */

        virtual bool IsRunning() const = 0;

        /**
            Get the maximum number of clients that can connect to the server.
            Corresponds to the maxClients parameter passed into the last call to Server::Start.
            @returns The maximum number of clients that can connect to the server. In other words, the number of client slots.
         */

        virtual int GetMaxClients() const = 0;

        /**
            Is a client connected to a client slot?
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns True if the client is connected.
         */

        virtual bool IsClientConnected( int clientIndex ) const = 0;

        /**
            Get the unique id of the client
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns The unique id of the client.
         */

        virtual uint64_t GetClientId( int clientIndex ) const = 0;

        /**
            Get the address of the client
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns The address of the client.
         */

        virtual netcode_address_t * GetClientAddress( int clientIndex ) const = 0;

        /**
            Get the number of clients that are currently connected to the server.
            @returns the number of connected clients.
         */

        virtual int GetNumConnectedClients() const = 0;

        /**
            Gets the current server time.
            @see Server::AdvanceTime
         */

        virtual double GetTime() const = 0;

        /**
            Create a message of the specified type for a specific client.
            @param clientIndex The index of the client this message belongs to. Determines which client heap is used to allocate the message.
            @param type The type of the message to create. The message types corresponds to the message factory created by the adaptor set on the server.
         */

        virtual Message * CreateMessage( int clientIndex, int type ) = 0;

        /**
            Helper function to allocate a data block.
            This is typically used to create blocks of data to attach to block messages. See BlockMessage for details.
            @param clientIndex The index of the client this message belongs to. Determines which client heap is used to allocate the data.
            @param bytes The number of bytes to allocate.
            @returns The pointer to the data block. This must be attached to a message via Client::AttachBlockToMessage, or freed via Client::FreeBlock.
         */

        virtual uint8_t * AllocateBlock( int clientIndex, int bytes ) = 0;

        /**
            Attach data block to message.
            @param clientIndex The index of the client this block belongs to.
            @param message The message to attach the block to. This message must be derived from BlockMessage.
            @param block Pointer to the block of data to attach. Must be created via Client::AllocateBlock.
            @param bytes Length of the block of data in bytes.
         */

        virtual void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes ) = 0;

        /**
            Free a block of memory.
            @param clientIndex The index of the client this block belongs to.
            @param block The block of memory created by Client::AllocateBlock.
         */

        virtual void FreeBlock( int clientIndex, uint8_t * block ) = 0;

        /**
            Can we send a message to a particular client on a channel?
            @param clientIndex The index of the client to send a message to.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns True if a message can be sent over the channel, false otherwise.
         */

        virtual bool CanSendMessage( int clientIndex, int channelIndex ) const = 0;

        /**
            Send a message to a client over a channel.
            @param clientIndex The index of the client to send a message to.
            @param channelIndex The channel index in range [0,numChannels-1].
            @param message The message to send.
         */

        virtual void SendMessage( int clientIndex, int channelIndex, Message * message ) = 0;

        /**
            Receive a message from a client over a channel.
            @param clientIndex The index of the client to receive messages from.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns The message received, or NULL if no message is available. Make sure to release this message by calling Server::ReleaseMessage.
         */

        virtual Message * ReceiveMessage( int clientIndex, int channelIndex ) = 0;

        /**
            Release a message.
            Call this for messages received by Server::ReceiveMessage.
            @param clientIndex The index of the client that the message belongs to.
            @param message The message to release.
         */

        virtual void ReleaseMessage( int clientIndex, Message * message ) = 0;

        /**
            Get client network info.
            Call this to receive information about the client network connection, eg. round trip time, packet loss %, # of packets sent and so on.
            @param clientIndex The index of the client.
            @param info The struct to be filled with network info [out].
         */

        virtual void GetNetworkInfo( int clientIndex, NetworkInfo & info ) const = 0;

        /**
            Connect a loopback client.
            This allows you to have local clients connected to a server, for example for integrated server or singleplayer.
            @param clientIndex The index of the client.
            @param clientId The unique client id.
            @param userData User data for this client. Optional. Pass NULL if not needed.
         */

        virtual void ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData ) = 0;

        /**
            Disconnect a loopback client.
            Loopback clients are not disconnected by regular Disconnect or DisconnectAllClient calls. You need to call this function instead.
            @param clientIndex The index of the client to disconnect. Must already be a connected loopback client.
         */

        virtual void DisconnectLoopbackClient( int clientIndex ) = 0;

        /**
            Is this client a loopback client?
            @param clientIndex The client index.
            @returns true if the client is a connected loopback client, false otherwise.
         */

        virtual bool IsLoopbackClient( int clientIndex ) const = 0;

        /**
            Process loopback packet.
            Use this to pass packets from a client directly to the loopback client slot on the server.
            @param clientIndex The client index. Must be an already connected loopback client.
            @param packetData The packet data to process.
            @param packetBytes The number of bytes of packet data.
            @param packetSequence The packet sequence number.
         */

        virtual void ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) = 0;
    };

    /**
        Common functionality across all server implementations.
     */

    class BaseServer : public ServerInterface
    {
    public:

        BaseServer( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time );

        ~BaseServer();

        void SetContext( void * context );

        void Start( int maxClients );

        void Stop();

        void AdvanceTime( double time );

        bool IsRunning() const { return m_running; }

        int GetMaxClients() const { return m_maxClients; }

        double GetTime() const { return m_time; }

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicates( float percent );

        Message * CreateMessage( int clientIndex, int type );

        uint8_t * AllocateBlock( int clientIndex, int bytes );

        void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes );

        void FreeBlock( int clientIndex, uint8_t * block );

        bool CanSendMessage( int clientIndex, int channelIndex ) const;

        bool HasMessagesToSend( int clientIndex, int channelIndex ) const;

        void SendMessage( int clientIndex, int channelIndex, Message * message );

        Message * ReceiveMessage( int clientIndex, int channelIndex );

        void ReleaseMessage( int clientIndex, Message * message );

        void GetNetworkInfo( int clientIndex, NetworkInfo & info ) const;

    protected:

        uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        void * GetContext() { return m_context; }

        Adapter & GetAdapter() { yojimbo_assert( m_adapter ); return *m_adapter; }

        Allocator & GetGlobalAllocator() { yojimbo_assert( m_globalAllocator ); return *m_globalAllocator; }

        MessageFactory & GetClientMessageFactory( int clientIndex );

        NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        reliable_endpoint_t * GetClientEndpoint( int clientIndex );

        Connection & GetClientConnection( int clientIndex );

        virtual void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        virtual int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static int StaticProcessPacketFunction( void * context,int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static void * StaticAllocateFunction( void * context, uint64_t bytes );

        static void StaticFreeFunction( void * context, void * pointer );

    private:

        ClientServerConfig m_config;                                ///< Base client/server config.
        Allocator * m_allocator;                                    ///< Allocator passed in to constructor.
        Adapter * m_adapter;                                        ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                           ///< Optional serialization context.
        int m_maxClients;                                           ///< Maximum number of clients supported.
        bool m_running;                                             ///< True if server is currently running, eg. after "Start" is called, before "Stop".
        double m_time;                                              ///< Current server time in seconds.
        uint8_t * m_globalMemory;                                   ///< The block of memory backing the global allocator. Allocated with m_allocator.
        uint8_t * m_clientMemory[MaxClients];                       ///< The block of memory backing the per-client allocators. Allocated with m_allocator.
        Allocator * m_globalAllocator;                              ///< The global allocator. Used for allocations that don't belong to a specific client.
        Allocator * m_clientAllocator[MaxClients];                  ///< Array of per-client allocator. These are used for allocations related to connected clients.
        MessageFactory * m_clientMessageFactory[MaxClients];        ///< Array of per-client message factories. This silos message allocations per-client slot.
        Connection * m_clientConnection[MaxClients];                ///< Array of per-client connection classes. This is how messages are exchanged with clients.
        reliable_endpoint_t * m_clientEndpoint[MaxClients];         ///< Array of per-client reliable endpoints.
        NetworkSimulator * m_networkSimulator;                      ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional.
        uint8_t * m_packetBuffer;                                   ///< Buffer used when writing packets.
    };

    /**
        Dedicated server implementation.
     */

    class Server : public BaseServer
    {
    public:

        Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Server();

        void Start( int maxClients );

        void Stop();

        void DisconnectClient( int clientIndex );

        void DisconnectAllClients();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        bool IsClientConnected( int clientIndex ) const;

        uint64_t GetClientId( int clientIndex ) const;

        netcode_address_t * GetClientAddress( int clientIndex ) const;

        int GetNumConnectedClients() const;

        void ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData );

        void DisconnectLoopbackClient( int clientIndex );

        bool IsLoopbackClient( int clientIndex ) const;

        void ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        const Address & GetAddress() const { return m_boundAddress; }

    private:

        void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void ConnectDisconnectCallbackFunction( int clientIndex, int connected );

        void SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        static void StaticConnectDisconnectCallbackFunction( void * context, int clientIndex, int connected );

        static void StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        ClientServerConfig m_config;
        netcode_server_t * m_server;
        Address m_address;                                  // original address passed to ctor
        Address m_boundAddress;                             // address after socket bind, eg. valid port
        uint8_t m_privateKey[KeyBytes];
    };

    /**
        The set of client states.
     */

    enum ClientState
    {
        CLIENT_STATE_ERROR = -1,
        CLIENT_STATE_DISCONNECTED = 0,
        CLIENT_STATE_CONNECTING,
        CLIENT_STATE_CONNECTED,
    };

    /**
        The common interface for all clients.
     */

    class ClientInterface
    {
    public:

        virtual ~ClientInterface() {}

        /**
            Set the context for reading and writing packets.
            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.
            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time.
            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Disconnect from the server.
         */

        virtual void Disconnect() = 0;

        /**
            Send packets to server.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from the server.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance client time.
            Call this at the end of each frame to advance the client time forward.
            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the client connecting to a server?
            This is true while the client is negotiation connection with a server.
            @returns true if the client is currently connecting to, but is not yet connected to a server.
         */

        virtual bool IsConnecting() const = 0;

        /**
            Is the client connected to a server?
            This is true once a client successfully finishes connection negotiatio, and connects to a server. It is false while connecting to a server.
            @returns true if the client is connected to a server.
         */

        virtual bool IsConnected() const = 0;

        /**
            Is the client in a disconnected state?
            A disconnected state corresponds to the client being in the disconnected, or in an error state. Both are logically "disconnected".
            @returns true if the client is disconnected.
         */

        virtual bool IsDisconnected() const = 0;

        /**
            Is the client in an error state?
            When the client disconnects because of an error, it enters into this error state.
            @returns true if the client is in an error state.
         */

        virtual bool ConnectionFailed() const = 0;

        /**
            Get the current client state.
         */

        virtual ClientState GetClientState() const = 0;

        /**
            Get the client index.
            The client index is the slot number that the client is occupying on the server.
            @returns The client index in [0,maxClients-1], where maxClients is the number of client slots allocated on the server in Server::Start.
         */

        virtual int GetClientIndex() const = 0;

        /**
            Get the client id.
            The client id is a unique identifier of this client.
            @returns The client id.
         */

        virtual uint64_t GetClientId() const = 0;

        /**
            Get the current client time.
            @see Client::AdvanceTime
         */

        virtual double GetTime() const = 0;

        /**
            Create a message of the specified type.
            @param type The type of the message to create. The message types corresponds to the message factory created by the adaptor set on this client.
         */

        virtual Message * CreateMessage( int type ) = 0;

        /**
            Helper function to allocate a data block.
            This is typically used to create blocks of data to attach to block messages. See BlockMessage for details.
            @param bytes The number of bytes to allocate.
            @returns The pointer to the data block. This must be attached to a message via Client::AttachBlockToMessage, or freed via Client::FreeBlock.
         */

        virtual uint8_t * AllocateBlock( int bytes ) = 0;

        /**
            Attach data block to message.
            @param message The message to attach the block to. This message must be derived from BlockMessage.
            @param block Pointer to the block of data to attach. Must be created via Client::AllocateBlock.
            @param bytes Length of the block of data in bytes.
         */

        virtual void AttachBlockToMessage( Message * message, uint8_t * block, int bytes ) = 0;

        /**
            Free a block of memory.
            @param block The block of memory created by Client::AllocateBlock.
         */

        virtual void FreeBlock( uint8_t * block ) = 0;

        /**
            Can we send a message on a channel?
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns True if a message can be sent over the channel, false otherwise.
         */

        virtual bool CanSendMessage( int channelIndex ) const = 0;

        /**
            Send a message on a channel.
            @param channelIndex The channel index in range [0,numChannels-1].
            @param message The message to send.
         */

        virtual void SendMessage( int channelIndex, Message * message ) = 0;

        /**
            Receive a message from a channel.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns The message received, or NULL if no message is available. Make sure to release this message by calling Client::ReleaseMessage.
         */

        virtual Message * ReceiveMessage( int channelIndex ) = 0;

        /**
            Release a message.
            Call this for messages received by Client::ReceiveMessage.
            @param message The message to release.
         */

        virtual void ReleaseMessage( Message * message ) = 0;

        /**
            Get client network info.
            Call this to receive information about the client network connection to the server, eg. round trip time, packet loss %, # of packets sent and so on.
            @param info The struct to be filled with network info [out].
         */

        virtual void GetNetworkInfo( NetworkInfo & info ) const = 0;

        /**
            Connect to server over loopback.
            This allows you to have local clients connected to a server, for example for integrated server or singleplayer.
            @param clientIndex The index of the client.
            @param clientId The unique client id.
            @param maxClients The maximum number of clients supported by the server.
         */

        virtual void ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients ) = 0;

        /**
            Disconnect from server over loopback.
         */

        virtual void DisconnectLoopback() = 0;

        /**
            Is this a loopback client?
            @returns true if the client is a loopback client, false otherwise.
         */

        virtual bool IsLoopback() const = 0;

        /**
            Process loopback packet.
            Use this to pass packets from a server directly to the loopback client.
            @param packetData The packet data to process.
            @param packetBytes The number of bytes of packet data.
            @param packetSequence The packet sequence number.
         */

        virtual void ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) = 0;
    };

    /**
        Functionality that is common across all client implementations.
     */

    class BaseClient : public ClientInterface
    {
    public:

        /**
            Base client constructor.
            @param allocator The allocator for all memory used by the client.
            @param config The base client/server configuration.
            @param time The current time in seconds. See ClientInterface::AdvanceTime
            @param allocator The adapter to the game program. Specifies allocators, message factory to use etc.
         */

        explicit BaseClient( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time );

        ~BaseClient();

        void SetContext( void * context ) { yojimbo_assert( IsDisconnected() ); m_context = context; }

        void Disconnect();

        void AdvanceTime( double time );

        bool IsConnecting() const { return m_clientState == CLIENT_STATE_CONNECTING; }

        bool IsConnected() const { return m_clientState == CLIENT_STATE_CONNECTED; }

        bool IsDisconnected() const { return m_clientState <= CLIENT_STATE_DISCONNECTED; }

        bool ConnectionFailed() const { return m_clientState == CLIENT_STATE_ERROR; }

        ClientState GetClientState() const { return m_clientState; }

        int GetClientIndex() const { return m_clientIndex; }

        double GetTime() const { return m_time; }

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicates( float percent );

        Message * CreateMessage( int type );

        uint8_t * AllocateBlock( int bytes );

        void AttachBlockToMessage( Message * message, uint8_t * block, int bytes );

        void FreeBlock( uint8_t * block );

        bool CanSendMessage( int channelIndex ) const;

        bool HasMessagesToSend( int channelIndex ) const;

        void SendMessage( int channelIndex, Message * message );

        Message * ReceiveMessage( int channelIndex );

        void ReleaseMessage( Message * message );

        void GetNetworkInfo( NetworkInfo & info ) const;

    protected:

        uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        void * GetContext() { return m_context; }

        Adapter & GetAdapter() { yojimbo_assert( m_adapter ); return *m_adapter; }

        void CreateInternal();

        void DestroyInternal();

        void SetClientState( ClientState clientState );

        Allocator & GetClientAllocator() { yojimbo_assert( m_clientAllocator ); return *m_clientAllocator; }

        MessageFactory & GetMessageFactory() { yojimbo_assert( m_messageFactory ); return *m_messageFactory; }

        NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        reliable_endpoint_t * GetEndpoint() { return m_endpoint; }

        Connection & GetConnection() { yojimbo_assert( m_connection ); return *m_connection; }

        virtual void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        virtual int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static int StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static void * StaticAllocateFunction( void * context, uint64_t bytes );

        static void StaticFreeFunction( void * context, void * pointer );

    private:

        ClientServerConfig m_config;                                        ///< The client/server configuration.
        Allocator * m_allocator;                                            ///< The allocator passed to the client on creation.
        Adapter * m_adapter;                                                ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                                   ///< Context lets the user pass information to packet serialize functions.
        uint8_t * m_clientMemory;                                           ///< The memory backing the client allocator. Allocated from m_allocator.
        Allocator * m_clientAllocator;                                      ///< The client allocator. Everything allocated between connect and disconnected is allocated and freed via this allocator.
        reliable_endpoint_t * m_endpoint;                                   ///< reliable endpoint.
        MessageFactory * m_messageFactory;                                  ///< The client message factory. Created and destroyed on each connection attempt.
        Connection * m_connection;                                          ///< The client connection for exchanging messages with the server.
        NetworkSimulator * m_networkSimulator;                              ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional.
        ClientState m_clientState;                                          ///< The current client state. See ClientInterface::GetClientState
        int m_clientIndex;                                                  ///< The client slot index on the server [0,maxClients-1]. -1 if not connected.
        double m_time;                                                      ///< The current client time. See ClientInterface::AdvanceTime
        uint8_t * m_packetBuffer;                                           ///< Buffer used to read and write packets.

    private:

        BaseClient( const BaseClient & other );

        const BaseClient & operator = ( const BaseClient & other );
    };

    /**
        Implementation of client for dedicated servers.
     */

    class Client : public BaseClient
    {
    public:

        /**
            The client constructor.
            @param allocator The allocator for all memory used by the client.
            @param address The address the client should bind to.
            @param config The client/server configuration.
            @param time The current time in seconds. See ClientInterface::AdvanceTime
         */

        explicit Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Client();

        void InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address & address );

        void InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses );

        void Connect( uint64_t clientId, uint8_t * connectToken );

        void Disconnect();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        int GetClientIndex() const;

        uint64_t GetClientId() const { return m_clientId; }

        void ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients );

        void DisconnectLoopback();

        bool IsLoopback() const;

        void ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        const Address & GetAddress() const { return m_boundAddress; }

    private:

        bool GenerateInsecureConnectToken( uint8_t * connectToken,
                                           const uint8_t privateKey[],
                                           uint64_t clientId,
                                           const Address serverAddresses[],
                                           int numServerAddresses );

        void CreateClient( const Address & address );

        void DestroyClient();

        void StateChangeCallbackFunction( int previous, int current );

        static void StaticStateChangeCallbackFunction( void * context, int previous, int current );

        void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        static void StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        ClientServerConfig m_config;                    ///< Client/server configuration.
        netcode_client_t * m_client;                    ///< netcode client data.
        Address m_address;                              ///< Original address passed to ctor.
        Address m_boundAddress;                         ///< Address after socket bind, eg. with valid port
        uint64_t m_clientId;                            ///< The globally unique client id (set on each call to connect)
    };
}

#endif // #ifndef YOJIMBO_H
