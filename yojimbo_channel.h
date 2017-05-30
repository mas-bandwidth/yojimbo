/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#ifndef YOJIMBO_CHANNEL_H
#define YOJIMBO_CHANNEL_H

#include "yojimbo_message.h"
#include "yojimbo_allocator.h"

// windows =p
#ifdef SendMessage
#undef SendMessage
#endif

/** @file */

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

    /// Implement this interface to receive callbacks for channel events.

    class ChannelListener
    {
    public:

        virtual ~ChannelListener() {}

        /**
            Override this method to get a callback when a block fragment is received.

            @param channel The channel the block is being sent over.
            @param messageId The message id the block is attached to.
            @param fragmentId The fragment id that is being processed. Fragment ids are in the range [0,numFragments-1].
            @param fragmentBytes The size of the fragment in bytes.
            @param numFragmentsReceived The number of fragments received for this block so far (including this one).
            @param numFragmentsInBlock The total number of fragments in this block. The block receive completes when all fragments are received.

            @see BlockMessage::AttachBlock
         */

        virtual void OnChannelFragmentReceived( class Channel * channel, uint16_t messageId, uint16_t fragmentId, int fragmentBytes, int numFragmentsReceived, int numFragmentsInBlock ) { (void) channel; (void) messageId; (void) fragmentId; (void) fragmentBytes; (void) numFragmentsReceived; (void) numFragmentsInBlock; }
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
                assert( false );
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
            Queue a message to be sent across this channel.

            @param message The message to be sent.
         */

        virtual void SendMessage( Message * message ) = 0;

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

        virtual int GetPacketData( ChannelPacketData & packetData, uint16_t packetSequence, int availableBits ) = 0;

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

                1. Acks messages and block fragments so they stop being included in outgoing connection packets (reliable-ordered channel)

                2. Does nothing at all (unreliable-unordered).

            @param sequence The sequence number of the connection packet that was acked.
         */

        virtual void ProcessAck( uint16_t sequence ) = 0;

    public:

        /** 
            Set the channel listener.

            The typical usage is to set the connection as the channel listener, so it gets callbacks from channels it owns.

            @see Connection
         */

        void SetListener( ChannelListener * listener ) { m_listener = listener; }

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

        ChannelListener * m_listener;                                                   ///< Channel listener for callbacks. Optional.

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

        // -----------------------------

        void Reset();

        bool CanSendMessage() const;

        void SendMessage( Message * message );

        Message * ReceiveMessage();

        void AdvanceTime( double time );

        int GetPacketData( ChannelPacketData & packetData, uint16_t packetSequence, int availableBits );

        void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence );

        void ProcessAck( uint16_t ack );

        // -----------------------------

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

        int GetMessagesToSend( uint16_t * messageIds, int & numMessageIds, int remainingPacketBits );

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

        int GetFragmentPacketData( ChannelPacketData & packetData, uint16_t messageId, uint16_t fragmentId, uint8_t * fragmentData, int fragmentSize, int numFragments, int messageType );

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

        void ProcessPacketFragment( int messageType, uint16_t messageId, int numFragments, uint16_t fragmentId, const uint8_t * fragmentData, int fragmentBytes, BlockMessage * blockMessage );

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
            SendBlockData( Allocator & allocator, int maxBlockSize, int maxFragmentsPerBlock )
            {
                m_allocator = &allocator;
                ackedFragment = YOJIMBO_NEW( allocator, BitArray, allocator, maxFragmentsPerBlock );
                fragmentSendTime = (double*) YOJIMBO_ALLOCATE( allocator, sizeof( double) * maxFragmentsPerBlock );
                blockData = (uint8_t*) YOJIMBO_ALLOCATE( allocator, maxBlockSize );
                assert( ackedFragment );
                assert( fragmentSendTime );
                assert( blockData );
                Reset();
            }

            ~SendBlockData()
            {
                YOJIMBO_DELETE( *m_allocator, BitArray, ackedFragment );
                YOJIMBO_FREE( *m_allocator, blockData );
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
            uint8_t * blockData;                                                        ///< The block data.

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
                assert( receivedFragment && blockData );
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

        void SendMessage( Message * message );

        Message * ReceiveMessage();

        void AdvanceTime( double time );

        int GetPacketData( ChannelPacketData & packetData, uint16_t packetSequence, int availableBits );

        void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence );

        void ProcessAck( uint16_t ack );

    protected:

        Queue<Message*> * m_messageSendQueue;                                           ///< Message send queue.
        Queue<Message*> * m_messageReceiveQueue;                                        ///< Message receive queue.

    private:

        UnreliableUnorderedChannel( const UnreliableUnorderedChannel & other );

        UnreliableUnorderedChannel & operator = ( const UnreliableUnorderedChannel & other );
    };
}

#endif
