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

#ifndef YOJIMBO_CONNECTION_H
#define YOJIMBO_CONNECTION_H

#include "yojimbo_packet.h"
#include "yojimbo_message.h"
#include "yojimbo_allocator.h"
#include "yojimbo_channel.h"

namespace yojimbo
{
    /// This magic number is used as a safety check to make sure the context accessed from Stream::GetContext is really a ConnectionContext object.

    const uint32_t ConnectionContextMagic = 0x11223344;

    /**
        Provides information required to read and write connection packets.

        @see ConnectionPacket.
     */

    struct ConnectionContext
    {
        uint32_t magic;                                                         ///< The magic number for safety checks. Set to ConnectionContextMagic.
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
        The connection packet implements packet level acks and transmits messages across a Connection.

        Connection packets should be generated and sent at a steady rate like 10, 20 or 30 times per-second in both directions across a connection. 

        The packet ack system is designed around this assumption (there are no separate ack packets, only connection packets).
     */

    struct ConnectionPacket : public Packet
    {
        uint16_t sequence;                                                      ///< The connection packet sequence number. Wraps around and keeps working. See yojimbo::sequence_greater_than and yojimbo::sequence_less_than.

        uint16_t ack;                                                           ///< The sequence number of the most recent packet received from the other side of the connection.

        uint32_t ack_bits;                                                      ///< Bit n is set if packet ack - n was received from the other side of the connection. See yojimbo::GenerateAckBits.

        int numChannelEntries;                                                  ///< The number of channel entries in this packet. Each channel entry corresponds to message data for a particular channel.

        ChannelPacketData * channelEntry;                                       ///< Per-channel message data that was included in this packet.

        ConnectionPacket();

        ~ConnectionPacket();

        bool AllocateChannelData( MessageFactory & messageFactory, int numEntries );

        template <typename Stream> bool Serialize( Stream & stream );

        bool SerializeInternal( ReadStream & stream );

        bool SerializeInternal( WriteStream & stream );

        bool SerializeInternal( MeasureStream & stream );

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
        CONNECTION_COUNTER_PACKETS_GENERATED,                   ///< Number of connection packets generated
        CONNECTION_COUNTER_PACKETS_PROCESSED,                   ///< Number of connection packets processed
        CONNECTION_COUNTER_PACKETS_ACKED,                       ///< Number of connection packets acked
        CONNECTION_COUNTER_NUM_COUNTERS                         ///< Number of connection counters.
    };

    /// Connection error states.

    enum ConnectionError
    {
        CONNECTION_ERROR_NONE = 0,                              ///< No error. All is well.
        CONNECTION_ERROR_CHANNEL = 1,                           ///< One of the connection channels is in an error state.
        CONNECTION_ERROR_OUT_OF_MEMORY = 1                      ///< The connection ran out of memory when it tried to perform an allocation.
    };

    /// Implement this interface to get callbacks when connection events occur.

    class ConnectionListener
    {
    public:

        virtual ~ConnectionListener() {}

        virtual void OnConnectionPacketSent( class Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketAcked( class Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketReceived( class Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionFragmentReceived( class Connection * /*connection*/, int /*channelId*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/, int /*numFragmentsReceived*/, int /*numFragmentsInBlock*/ ) {}
    };

    struct ConnectionSentPacketData 
    { 
        uint8_t acked;
    };

    struct ConnectionReceivedPacketData {};

    /// Implements packet level acks and transmits messages between two endpoints.

    class Connection : public ChannelListener
    {
    public:

        Connection( Allocator & allocator, PacketFactory & packetFactory, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig = ConnectionConfig() );

        ~Connection();

        void Reset();

        bool CanSendMsg( int channelId = 0 ) const;

        void SendMsg( Message * message, int channelId = 0 );

        Message * ReceiveMsg( int channelId = 0 );

        ConnectionPacket * GeneratePacket();

        bool ProcessPacket( ConnectionPacket * packet );

        void AdvanceTime( double time );

        ConnectionError GetError() const;

        void SetListener( ConnectionListener * listener ) { m_listener = listener; }

        void SetClientIndex( int clientIndex ) { m_clientIndex = clientIndex; }

        int GetClientIndex() const { return m_clientIndex; }

        uint64_t GetCounter( int index ) const;

    protected:

        virtual void OnPacketSent( uint16_t /*sequence*/ ) {}

        virtual void OnPacketAcked( uint16_t /*sequence*/ ) {}

        virtual void OnPacketReceived( uint16_t /*sequence*/ ) {}

    protected:

        void InsertAckPacketEntry( uint16_t sequence );

        void ProcessAcks( uint16_t ack, uint32_t ack_bits );

        void PacketAcked( uint16_t sequence );

        void OnChannelFragmentReceived( class Channel * channel, uint16_t messageId, uint16_t fragmentId, int fragmentBytes, int numFragmentsReceived, int numFragmentsInBlock );

    private:

        const ConnectionConfig m_connectionConfig;                                      // connection configuration

        ConnectionError m_error;                                                        // connection error level

        int m_clientIndex;                                                              // optional client index for server client connections. 0 by default.

        Channel * m_channel[MaxChannels];                                               // message channels. see config.numChannels for size of this array.

        Allocator * m_allocator;                                                        // allocator for allocations matching life cycle of object

        PacketFactory * m_packetFactory;                                                // packet factory for creating and destroying connection packets

        MessageFactory * m_messageFactory;                                              // message factory creates and destroys messages

        ConnectionListener * m_listener;                                                // connection listener

        SequenceBuffer<ConnectionSentPacketData> * m_sentPackets;                       // sequence buffer of recently sent packets

        SequenceBuffer<ConnectionReceivedPacketData> * m_receivedPackets;               // sequence buffer of recently received packets

        uint64_t m_counters[CONNECTION_COUNTER_NUM_COUNTERS];                           // counters for unit testing, stats etc.

    private:

        Connection( const Connection & other );

        Connection & operator = ( const Connection & other );
    };
}

#endif // #ifndef YOJIMBO_CONNECTION
