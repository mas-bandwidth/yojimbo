/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.

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

#ifndef YOJIMBO_CONNECTION_H
#define YOJIMBO_CONNECTION_H

#include "yojimbo_message.h"
#include "yojimbo_allocator.h"
#include "yojimbo_channel.h"

/** @file */

namespace yojimbo
{
    class Connection
    {
    public:

        Connection( Allocator & allocator, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig = ConnectionConfig() );

        ~Connection();

        void GeneratePacket( uint8_t * packetData, int maxPacketBytes, int & packetBytes );

        void ProcessAcks( const uint16_t * acks, int numAcks );

        bool ProcessPacket( uint16_t packetSequence, const uint8_t * packetData, int packetBytes );

    private:

        // ...
    };
}














#if 0 // old stuff

namespace yojimbo
{
    /// This magic number is used as a safety check to make sure the context accessed from Stream::GetContext is really a ConnectionContext object.

    const uint32_t ConnectionContextMagic = 0x11223344;

    /**
        Provides information required to read and write connection packets.

        @see ConnectionPacket
        @see Stream::SetContext
        @see Stream::GetContext
     */

    struct ConnectionContext
    {
        uint32_t magic;                                                         ///< The magic number for safety checks. Set to yojimbo::ConnectionContextMagic.
        const ConnectionConfig * connectionConfig;                              ///< The connection config. So we know the number of channels and how they are setup.
        class MessageFactory * messageFactory;                                  ///< The message factory used for creating and destroying messages.

        ConnectionContext()
        {
            magic = ConnectionContextMagic;
            messageFactory = NULL;
            connectionConfig = NULL;
        }
    };

    /** 
        Implements packet level acks and carries messages across a connection.

        Connection packets should be generated and sent at a steady rate like 10, 20 or 30 times per-second in both directions across a connection. 

        The packet ack system is designed around this assumption. There are no separate ack packets!

        @see GenerateAckBits
     */

    struct ConnectionPacket : public Packet
    {
        uint16_t sequence;                                                      ///< The connection packet sequence number. Wraps around and keeps working. See yojimbo::sequence_greater_than and yojimbo::sequence_less_than.

        uint16_t ack;                                                           ///< The sequence number of the most recent packet received from the other side of the connection.

        uint32_t ack_bits;                                                      ///< Bit n is set if packet ack - n was received from the other side of the connection. See yojimbo::GenerateAckBits.

        int numChannelEntries;                                                  ///< The number of channel entries in this packet. Each channel entry corresponds to message data for a particular channel.

        ChannelPacketData * channelEntry;                                       ///< Per-channel message data that was included in this packet.

        ConnectionPacket();

        /** 
            Connection packet destructor.

            Releases all references to messages included in this packet.

            @see Message
            @see MessageFactory
            @see ChannelPacketData
         */

        ~ConnectionPacket();

        /** 
            Allocate channel data in this packet.

            The allocation is performed with the allocator that is set on the message factory.

            When this is used on the server, the allocator corresponds to the per-client allocator corresponding to the client that is sending this connection packet. See Server::m_clientAllocator.

            This is intended to silo each client to their own set of resources on the server, so malicious clients cannot launch an attack to deplete resources shared with other clients.

            @param messageFactory The message factory used to create and destroy messages.
            @param numEntries The number of channel entries to allocate. This corresponds to the number of channels that have data to include in the connection packet.

            @returns True if the allocation succeeded, false otherwise.

            @see Connection::GeneratePacket
         */

        bool AllocateChannelData( MessageFactory & messageFactory, int numEntries );

        /** 
            The template function for serializing the connection packet.

            Unifies packet read and write, making it harder to accidentally desync one from the other.
         */

        template <typename Stream> bool Serialize( Stream & stream );

        bool SerializeInternal( ReadStream & stream );                          ///< Implements serialize read by calling into ConnectionPacket::Serialize with a ReadStream.

        bool SerializeInternal( WriteStream & stream );                         ///< Implements serialize write by calling into ConnectionPacket::Serialize with a WriteStream.

        bool SerializeInternal( MeasureStream & stream );                       ///< Implements serialize measure by calling into ConnectionPacket::Serialize with a MeasureStream.

        void SetMessageFactory( MessageFactory & messageFactory ) { m_messageFactory = &messageFactory; }

    private:

        MessageFactory * m_messageFactory;                                      ///< The message factory is cached so we can release messages included in this packet when the packet is destroyed.

        ConnectionPacket( const ConnectionPacket & other );

        const ConnectionPacket & operator = ( const ConnectionPacket & other );
    };

    /**
        Connection counters provide insight into the number of times an action was performed by the connection.

        They are intended for use in a telemetry system, eg. the server would report these counters to some backend logging system to track behavior in a production environment.
     */

    enum ConnectionCounters
    {
        CONNECTION_COUNTER_PACKETS_GENERATED,                                   ///< Number of connection packets generated.
        CONNECTION_COUNTER_PACKETS_PROCESSED,                                   ///< Number of connection packets processed.
        CONNECTION_COUNTER_PACKETS_STALE,                                       ///< Number of connection packets that could not be processed because they were stale.
        CONNECTION_COUNTER_PACKETS_ACKED,                                       ///< Number of connection packets acked.
        CONNECTION_COUNTER_NUM_COUNTERS                                         ///< The number of connection counters.
    };

    /// Connection error states.

    enum ConnectionError
    {
        CONNECTION_ERROR_NONE = 0,                                              ///< No error. All is well.
        CONNECTION_ERROR_CHANNEL = 1,                                           ///< One of the connection channels is in an error state.
        CONNECTION_ERROR_OUT_OF_MEMORY = 1                                      ///< The connection ran out of memory when it tried to perform an allocation.
    };

    /// Implement this interface to get callbacks when connection events occur.

    class ConnectionListener
    {
    public:

        virtual ~ConnectionListener() {}

        virtual void OnConnectionPacketGenerated( class Connection * connection, uint16_t sequence ) { (void) connection; (void) sequence; }

        virtual void OnConnectionPacketAcked( class Connection * connection, uint16_t sequence ) { (void) connection; (void) sequence; }

        virtual void OnConnectionPacketReceived( class Connection * connection, uint16_t sequence ) { (void) connection; (void) sequence; }

        virtual void OnConnectionFragmentReceived( class Connection * connection, int channelId, uint16_t messageId, uint16_t fragmentId, int fragmentBytes, int numFragmentsReceived, int numFragmentsInBlock ) { (void) connection; (void) channelId; (void) messageId; (void) fragmentId; (void) fragmentBytes; (void) numFragmentsReceived; (void) numFragmentsInBlock; }
    };

    // data stored per-sent connection packet in a sequence buffer

    struct ConnectionSentPacketData 
    { 
        uint8_t acked;
    };

    // data stored per-sent connection packet in a sequence buffer (reserved for future expansion)

    struct ConnectionReceivedPacketData {};

    /** 
        Implements packet level acks and transmits messages.

        The connection class is used internally by client and server to send and receive messages over connection packets. 

        You don't need to interact with this class, unless you are bypassing client/server entirely and using the low-level parts of libyojimbo directly.
     */

    class Connection : public ChannelListener
    {
    public:

        /**
            The connection constructor.

            @param allocator The allocator to use.
            @param packetFactory The packet factory for creating and destroying connection packets.
            @param messageFactory The message factory to use for creating and destroying messages sent across this connection.
            @param connectionConfig The connection configuration. Specifies the number and type of channels on the connection, as well as other per-connection and per-channel properties.
        */

        Connection( Allocator & allocator, PacketFactory & packetFactory, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig = ConnectionConfig() );

        /**
            The connection destructor.
         */

        ~Connection();

        /**
            Reset the connection.

            Resets message send and receive queues and all properties for a connection, so it may be reused again for some other purpose.

            For example, the server calls this function when a client disconnects, so the connection can be reused by the next client that joins in that client slot.
         */

        void Reset();

        /** 
            Check if there is room in the send queue to send a message.

            In reliable-ordered channels, all messages that are sent are guaranteed to arrive, however, if the send queue is full, we have nowhere to buffer the message, so it is lost. This is a fatal error.

            Therefore, in debug builds Connection::SendMsg checks this method and asserts it returns true. In release builds, this function is checked and an error flag is set on the connection if it returns false. 

            This error flag triggers the automatic disconnection of any client that overflows its message send queue. This is by design so you don't have to do this checking manually. 

            All you have to do is make sure you have a sufficiently large send queue configured via ChannelConfig::sendQueueSize.

            @param channelId The id of the channel in [0,numChannels-1].
            @returns True if the channel has room for one more message in its send queue. False otherwise.

            @see ChannelConfig
            @see ConnectionConfig
            @see Connection::SendMsg
         */

        bool CanSendMsg( int channelId = 0 ) const;

        /**
            Queue a message to be sent.

            This function adds a message to the send queue of the specified channel. 

           The reliability and ordering guarantees of how the message will be received on the other side are determined by the configuration of the channel.
    
            @param message The message to be sent. It must be allocated from the message factory set on this connection.
            @param channelId The id of the channel to send the message across in [0,numChannels-1].

            @see Connection::CanSendMsg
            @see ChannelConfig
         */

        void SendMsg( Message * message, int channelId = 0 );

        /** 
            Poll this method to receive messages.

            Typical usage is to iterate across the set of channels and poll this to receive messages until it returns NULL.

            IMPORTANT: The message returned by this function has one reference. You are responsible for releasing this message via MessageFactory::Release.

            @param channelId The id of the channel to try to receive a message from.
            @returns A pointer to the received message, NULL if there are no messages to receive.
         */

        Message * ReceiveMsg( int channelId = 0 );

        /** 
            Generate a connection packet.

            This function generates a connection packet containing messages from the send queues of channels specified on this connection. These messages are sent and resent in connection packets until one of those connection packets are acked.

            This packet is sent across the network and processed be the connection object on the other side, which drives the transmission of messages from this connection to the other. On the other side, when the connection packet is received it is passed in to Connection::ProcessPacket.

            The connection packet also handles packet level acks. This ack system is based around the expectation that connection packets are generated and sent regularly in both directions. There are no separate ack packets. Acks are encoded in the packet header of the connection packet.

            @returns The connection packet that was generated.

            @see ConnectionPacket
            @see ProcessPacket
         */

        ConnectionPacket * GeneratePacket();

        /**
            Process a connection packet.

            Connections are endpoints. Across a logical connection on one side is connection A, and on the other side is connection object B. 

            When a packet generated by A is sent to B, B receives that packet and calls this function to process it, adding any messages contained in the packet its message receive queues.

            This transmission of connection packets is bidirectional. A -> B, and B -> A. The ack system relies on packets being sent regularly (10,20,30HZ) in both directions to work properly. There are no separate ack packets.

            @param packet The connection packet to process.
            @returns True if the packet was processed successfully, false if something went wrong.
         */

        bool ProcessPacket( ConnectionPacket * packet );

        /**
            Advance connection time.

            Call this at the end of each frame to advance the connection time forward. 

            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        void AdvanceTime( double time );

        /**
            Get the connection error level.

            @returns The connection error level. See ConnectionError for details.
         */

        ConnectionError GetError() const;

        /**
            Set a listener object to receive callbacks (optional).

            @param listener The listener object.
         */

        void SetListener( ConnectionListener * listener ) { m_listener = listener; }

        /**
            Set the client index.

            This is used by the server to associate per-client connection objects with the index of their client slot. This makes it easier to work out which client a connection object belongs to on the server.

            @param clientIndex The client index in [0,numClients-1]. If you don't set this, it defaults to zero.

            @see GetClientIndex
         */

        void SetClientIndex( int clientIndex ) { m_clientIndex = clientIndex; }

        /**
            Get the client index set on the connection.

            This is a helper function to make it easy to work out which client a connection object belongs to on the server.

            @returns The client index in [0,numClients-1].

            @see SetClientIndex
         */

        int GetClientIndex() const { return m_clientIndex; }

        /**
            Get a counter value.

            Counters are used to track event and actions performed by the connection. They are useful for debugging, testing and telemetry.

            @returns The counter value. See yojimbo::ConnectionCounters for the set of connection counters.
         */

        uint64_t GetCounter( int index ) const;

    protected:

        /**
            This is called for each connection packet generated, to mark that connection packet as unacked.

            Later on this entry is used to determine if a connection packet has already been acked, so we only trigger packet acked callbacks the first time we receive an ack for that packet.

            @param sequence The sequence number of the connection packet that was generated.
         */

        void InsertAckPacketEntry( uint16_t sequence );

        /**
            This is the payload function called to process acks in the packet header of the connection packet. 

            It walks across the ack bits and if bit n is set, then sequence number "ack - n" has been received be the other side, so it should be acked if it is not already.

            @param ack The most recent acked packet sequence number.
            @param ack_bits The ack bitfield. Bit n is set if ack - n packet has been received.

            @see ConnectionPacket
         */

        void ProcessAcks( uint16_t ack, uint32_t ack_bits );

        /**
            This method is called when a packet is acked.

            It drives all the things that must happen when a packet is acked, such as callbacks, acking messages and data block fragments in send queues, and so on.

            @param sequence The sequence number of the packet that was acked.
         */

        void PacketAcked( uint16_t sequence );

    protected:

        virtual void OnPacketAcked( uint16_t sequence );

        virtual void OnChannelFragmentReceived( class Channel * channel, uint16_t messageId, uint16_t fragmentId, int fragmentBytes, int numFragmentsReceived, int numFragmentsInBlock );

    private:

        const ConnectionConfig m_connectionConfig;                                      ///< The connection configuration.

        ConnectionError m_error;                                                        ///< Connection error level. If the connection is in an error state, it is fatal and the connection should be torn down.

        int m_clientIndex;                                                              ///< Optional client index for server/client connections. Used to get the client index for client connections on the server in callbacks. 0 by default.

        Channel * m_channel[MaxChannels];                                               ///< Array of message channels. Size of array corresponds to m_connectionConfig.numChannels.

        Allocator * m_allocator;                                                        ///< Allocator passed in to the connection constructor.

        PacketFactory * m_packetFactory;                                                ///< Packet factory for creating and destroying connection packets.

        MessageFactory * m_messageFactory;                                              ///< Message factory for creating and destroying messages.

        ConnectionListener * m_listener;                                                ///< Connection listener. Optional. May be NULL.

        SequenceBuffer<ConnectionSentPacketData> * m_sentPackets;                       ///< Sequence buffer for sent connection packets. Used by the ack system.

        SequenceBuffer<ConnectionReceivedPacketData> * m_receivedPackets;               ///< Sequence buffer for received connection packets. Used by the ack system.

        uint64_t m_counters[CONNECTION_COUNTER_NUM_COUNTERS];                           ///< Counters for unit testing, stats, telemetry etc.

    private:

        Connection( const Connection & other );

        Connection & operator = ( const Connection & other );
    };
}

#endif

#endif // #ifndef YOJIMBO_CONNECTION
