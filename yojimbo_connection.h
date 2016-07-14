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

#ifndef YOJIMBO_CONNECTION_H
#define YOJIMBO_CONNECTION_H

#include "yojimbo_packet.h"
#include "yojimbo_message.h"
#include "yojimbo_allocator.h"
#include "yojimbo_channel.h"

namespace yojimbo
{
    struct ConnectionConfig
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

        ConnectionConfig()
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

    struct ConnectionContext
    {
        MessageFactory * messageFactory;
        ConnectionConfig * connectionConfig;
    };

    struct ConnectionPacket : public Packet
    {
        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;

        // todo: clean this up. maybe "ChannelData" with a union?

        int numMessages;
        Message ** messages;
        MessageFactory * messageFactory;

        BlockMessage * blockMessage;
        uint8_t * blockFragmentData;
        uint64_t blockMessageId : 16;
        uint64_t blockFragmentId : 16;
        uint64_t blockFragmentSize : 16;
        uint16_t blockNumFragments : 16;
        int blockMessageType;

        ConnectionPacket()
        {
            sequence = 0;
            ack = 0;
            ack_bits = 0;
            numMessages = 0;
            messages = NULL;
            messageFactory = NULL;
            blockMessage = NULL;
            blockFragmentData = NULL;
            blockMessageId = 0;
            blockFragmentId = 0;
            blockFragmentSize = 0;
            blockNumFragments = 0;
            blockMessageType = 0;
        }

        ~ConnectionPacket();

        template <typename Stream> bool Serialize( Stream & stream );

        bool SerializeInternal( ReadStream & stream );

        bool SerializeInternal( WriteStream & stream );

        bool SerializeInternal( MeasureStream & stream );

    private:

        ConnectionPacket( const ConnectionPacket & other );

        const ConnectionPacket & operator = ( const ConnectionPacket & other );
    };

    enum ConnectionCounters
    {
        CONNECTION_COUNTER_PACKETS_READ,                        // number of packets read
        CONNECTION_COUNTER_PACKETS_WRITTEN,                     // number of packets written
        CONNECTION_COUNTER_PACKETS_ACKED,                       // number of packets acked
        CONNECTION_COUNTER_NUM_COUNTERS
    };

    enum ConnectionError
    {
        CONNECTION_ERROR_NONE = 0,
        CONNECTION_ERROR_MESSAGE_DESYNC,
        CONNECTION_ERROR_MESSAGE_SEND_QUEUE_FULL,
        CONNECTION_ERROR_MESSAGE_SERIALIZE_MEASURE_FAILED,
        CONNECTION_ERROR_OUT_OF_MEMORY
    };

    class Connection;

    class ConnectionListener
    {
    public:

        virtual ~ConnectionListener() {}

        virtual void OnConnectionPacketSent( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketAcked( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketReceived( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionFragmentReceived( Connection * /*connection*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/ ) {}
    };

    struct ConnectionSentPacketData 
    { 
        uint8_t acked; 
    };

    struct ConnectionReceivedPacketData {};

    class Connection
    {
    public:

        Connection( Allocator & allocator, PacketFactory & packetFactory, MessageFactory & messageFactory, const ConnectionConfig & config = ConnectionConfig() );

        ~Connection();

        void Reset();

        bool CanSendMessage() const;

        void SendMessage( Message * message );

        Message * ReceiveMessage();

        ConnectionPacket * WritePacket();

        bool ReadPacket( ConnectionPacket * packet );

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

        void AddMessagesToPacket( const uint16_t * messageIds, int numMessageIds, ConnectionPacket * packet );

        void AddFragmentToPacket( uint16_t messageId, uint16_t fragmentId, uint8_t * fragmentData, int fragmentSize, int numFragments, int messageType, ConnectionPacket * packet );

    private:

        const ConnectionConfig m_config;                                                // const configuration data

        ConnectionError m_error;                                                        // connection error level

        int m_clientIndex;                                                              // optional client index for server client connections. 0 by default.

        Channel * m_channel;                                                            // message channel

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
