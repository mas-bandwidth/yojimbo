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
    /// Per-channel data inside a connection packet.

    struct ChannelPacketData
    {
        uint32_t channelId : 16;                                        ///< The id of the channel this data belongs to in [0,numChannels-1].
        
        uint32_t initialized : 1;                                       ///< 1 if this channel packet data was properly initialized, 0 otherwise. This is a safety measure to make sure ChannelPacketData::Initialize gets called.
        
        uint32_t blockMessage : 1;                                      ///< 1 if this channel data contains data for a block (eg. a fragment of that block), 0 if this channel data contains messages.
        
        uint32_t messageFailedToSerialize : 1;                          ///< Set to 1 if a message for this channel fails to serialized. Used to set CHANNEL_ERROR_FAILED_TO_SERIALIZE on the Channel object.

        /// Packet data when the channel is sending regular messages.

        struct MessageData
        {
            int numMessages;                                            ///< The number of messages included in the packet for this channel.
            Message ** messages;                                        ///< Array of message pointers (dynamically allocated). The messages in this array have references added, so they must be released when the packet containing this channel data is destroyed.
        };

        /// Packet data when the channel is sending a message with a block attached. @see BlockMessage.

        struct BlockData
        {
            BlockMessage * message;                                     ///< The message the block is attached to. The message is serialized and included as well as the block data which is split up into fragments.
            uint8_t * fragmentData;                                     ///< Pointer to the fragment data being sent in this packet. Blocks are split up into fragments of size ChannelConfig::fragmentSize.
            uint64_t messageId : 16;                                    ///< The message id that this block is attached to. Used for ordering. Message id increases with each packet sent across a channel.
            uint64_t fragmentId : 16;                                   ///< The id of the fragment being sent in [0,numFragments-1].
            uint64_t fragmentSize : 16;                                 ///< The size of the fragment. Typically this is ChannelConfig::fragmentSize, except for the last fragment, which may be smaller.
            uint64_t numFragments : 16;                                 ///< The number of fragments this block is split up into. Lets the receiver know when all fragments have been received.
            int messageType;                                            ///< The message type. Used to create the corresponding message object on the receiver side once all fragments are received.
        };

        union
        {
            MessageData message;                                        ///< Data for sending messages.
            BlockData block;                                            ///< Data for sending a block fragment.
        };

        /**
            Initialize the channel packet data to default values.
         */

        void Initialize();

        /**
            Release messages stored in channel packet data and free allocations.

            @param messageFactory Since we don't cache the message factory on this class, it must be passed in so we can release the messages. This is the reason there isn't a constructor/destructor for this struct.
         */

        void Free( MessageFactory & messageFactory );

        /** 
            Templated serialize function for the channel packet data.

            Unifies packet read and write, making it harder to accidentally desync.

            @param stream The stream used for serialization.
            @param messageFactory The message factory used to create message objects on serialize read.
            @param channelConfigs Array of channel configs, indexed by channel id in [0,numChannels-1].
            @param numChannels The number of channels configured on the connection.
         */

        template <typename Stream> bool Serialize( Stream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        /// Implements serialize read by a calling into ChannelPacketData::Serialize with a ReadStream.

        bool SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        /// Implements serialize write by a calling into ChannelPacketData::Serialize with a WriteStream.

        bool SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        /// Implements serialize measure by a calling into ChannelPacketData::Serialize with a MeasureStream.

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

    /// Channel error codes.

    enum ChannelError
    {
        CHANNEL_ERROR_NONE = 0,                                 ///< No error. All is well.
        CHANNEL_ERROR_DESYNC,                                   ///< This channel has desynced. This happens when something super-weird happens. Typically a message has a desync in its serialize read and write.
        CHANNEL_ERROR_SEND_QUEUE_FULL,                          ///< The user tried to send a message but the send queue was full. This will assert out in development, but in production it sets this error on the channel.
        CHANNEL_ERROR_BLOCKS_DISABLED,                          ///< The channel received a packet containing data for blocks, but this channel is configured to disable blocks. See ChannelConfig::disableBlocks.
        CHANNEL_ERROR_FAILED_TO_SERIALIZE,                      ///< Serialize read failed for a message sent to this channel. Check your message serialize functions, one of them is returning false on serialize read. This can also be caused by a desync in message read and write.
        CHANNEL_ERROR_OUT_OF_MEMORY,                            ///< The channel tried to allocate some memory but couldn't.
    };

    /// Helper function to convert a channel error to a user friendly string.

    inline const char * GetChannelErrorString( ChannelError error )
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

        Channel() : m_channelId(0), m_error( CHANNEL_ERROR_NONE ) {}

        virtual ~Channel() {}

        virtual void SetListener( ChannelListener * listener ) = 0;

        /**
            Reset the channel. 
         */

        virtual void Reset() = 0;

        /**
            Returns true if a message can be sent over this channel.
         */            

        virtual bool CanSendMsg() const = 0;

        /**
            Queue a message to be sent across this channel.

            @param message The message to be sent.
         */

        virtual void SendMsg( Message * message ) = 0;

        /** 
            Pops the next message off the receive queue if one is available.

            @returns A pointer to the received message, NULL if there are no messages to receive. The caller owns message objects returned by this function and is responsible for releasing them via Message::Release.
         */

        virtual Message * ReceiveMsg() = 0;

        /**
            Advance channel time.

            Called by Connection::AdvanceTime for each channel configured on the connection.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Get packet data for this channel to include in a connection packet.

            @param packetData The packet data to be generated [out]
            @param packetSequence The sequence number of the packet being generated.
            @param availableBits The maximum number of bits of packet data the channel may write.

            @returns The number of bits of packet data taken up by this channel.

            @see ConnectionPacket
            @see Connection::GeneratePacket
         */

        virtual int GetPacketData( ChannelPacketData & packetData, uint16_t packetSequence, int availableBits ) = 0;

        /**
            Process packet data included in a connection packet.

            @param packetData The channel packet data to process.
            @param packetSequence The sequence number of the connection packet that contains the channel packet data.
         */

        virtual void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence ) = 0;

        /**
            Process a connection packet ack.

            Depending on the channel type, this could ack messages so they stop being sent (reliable-ordered), or do nothing at all (unreliable-unordered).

            @param sequence The sequence number of the connection packet that was acked.
         */

        virtual void ProcessAck( uint16_t sequence ) = 0;

        /**
            Get the channel error level.

            @returns The channel error. 
         */

        ChannelError GetError() const { return m_error; }

        /** 
            Gets the channel id.

            @returns The channel id in [0,numChannels-1].
         */

        int GetChannelId() const { return m_channelId; }

    protected:

        void SetError( ChannelError error )
        {
            if ( error != m_error && error != CHANNEL_ERROR_NONE )
                debug_printf( "channel error: %s\n", GetChannelErrorString( error ) );
            m_error = error;
        }

        void SetChannelId( int channelId )
        {
            assert( channelId >= 0 );
            assert( channelId < MaxChannels );
            m_channelId = channelId;
        }

    protected:

        int m_channelId;                                            ///< The channel id in [0,numChannels-1].
        
        ChannelError m_error;                                       ///< The channel error level.
    };

    /**
        Messages sent across this channel are guaranteed to arrive, and in the order they were sent.

        This channel type is best used for control messages and RPCs.

        Messages sent over this channel are included in connection packets until one of those packets is acked.

        Blocks attached to messages sent over this channel are split up into fragments. Each fragment of the block is included in a connection packet until one of those packets are acked. Eventually, all fragments are received on the other side, and the block is reassembled and attached to the block message and added to the message receive queue for this channel.

        Only one message block may be in flight over the network at any time, so blocks stall out message delivery slightly. Therefore, only use blocks for large data that won't fit inside a single connection packet. 
     */

    class ReliableOrderedChannel : public Channel
    {
    public:

        // todo: document this class

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

        const ChannelConfig m_config;                                                   ///< Channel configuration data.

        Allocator * m_allocator;                                                        ///< Allocator for allocations matching the life cycle of the channel.

        ChannelListener * m_listener;                                                   ///< Channel listener for callbacks. Optional.

        MessageFactory * m_messageFactory;                                              ///< Message factory creates and destroys messages.

        double m_time;                                                                  ///< The current time.

        uint16_t m_sendMessageId;                                                       ///< Id for next message added to send queue.

        uint16_t m_receiveMessageId;                                                    ///< Id for next message to be received.

        uint16_t m_oldestUnackedMessageId;                                              ///< Id for oldest unacked message in send queue.

        SequenceBuffer<MessageSendQueueEntry> * m_messageSendQueue;                     ///< Message send queue.

        SequenceBuffer<MessageSentPacketEntry> * m_messageSentPackets;                  ///< Stores information per sent connection packet about messages and block data included in each packet. Used to walk from connection packet level acks to message and data block fragment level acks.

        SequenceBuffer<MessageReceiveQueueEntry> * m_messageReceiveQueue;               ///< Message receive queue

        uint16_t * m_sentPacketMessageIds;                                              ///< Array of message ids per-sent connection packet. Allows the maximum number of messages per-packet to be allocated dynamically.

        SendBlockData * m_sendBlock;                                                    ///< Data about the block being currently sent.

        ReceiveBlockData * m_receiveBlock;                                              ///< Data about the block being currently received.

        uint64_t m_counters[CHANNEL_COUNTER_NUM_COUNTERS];                              ///< Counters for unit testing, stats etc.

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

        // todo: document this class

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

    protected:

        const ChannelConfig m_config;                                                   ///< Channel configuration data.

        Allocator * m_allocator;                                                        ///< Allocator for allocations matching life cycle of this channel.

        ChannelListener * m_listener;                                                   ///< Channel listener for callbacks. Optional.

        MessageFactory * m_messageFactory;                                              ///< Message factory creates and destroys messages.

        Queue<Message*> * m_messageSendQueue;                                           ///< Message send queue. Messages that don't fit in the next connection packet sent are discarded.

        Queue<Message*> * m_messageReceiveQueue;                                        ///< Message receive queue. Should generally be larger than the send queue.

        uint64_t m_counters[CHANNEL_COUNTER_NUM_COUNTERS];                              ///< Counters for unit testing, stats etc.

    private:

        UnreliableUnorderedChannel( const UnreliableUnorderedChannel & other );

        UnreliableUnorderedChannel & operator = ( const UnreliableUnorderedChannel & other );
    };
}

#endif
