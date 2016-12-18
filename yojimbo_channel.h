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
        uint32_t channelId : 16;
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
            MessageData message;                                        // packet data for sending messages
            BlockData block;                                            // packet data for sending a block
        };

        void Initialize();

        void Free( MessageFactory & messageFactory );

        template <typename Stream> bool Serialize( Stream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( MeasureStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );
    };

    /// Implement this interface to recieve callbacks for channel events.

    class ChannelListener
    {
    public:

        virtual ~ChannelListener() {}

        virtual void OnChannelFragmentReceived( class Channel * /*channel*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/, int /*numFragmentsReceived*/, int /*numFragmentsInBlock*/ ) {}
    };

    /**
        Channel counters provide insight into the number of times an action was performed by a channel.

        They are intended for use in a telemetry system, eg. the server would report these counters to some backend logging system to track behavior in a production environment.
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
        CHANNEL_ERROR_FAILED_TO_SERIALIZE                       ///< Serialize read failed for a message sent to this channel. Check your message serialize functions, one of them is returning false on serialize read. This can also be caused by a desync in message read and write.
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

    /// Functionality common across all channel types.

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

        int m_channelId;
        
        ChannelError m_error;
    };

    /// Messages sent across this channel are guaranteed to arrive and in the same order they were sent.

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

    /// Messages sent across this channel are not guaranteed to arrive, and may be received in a different order than they were sent.

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
