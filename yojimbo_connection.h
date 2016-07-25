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
        int maxPacketSize;                                      // maximum connection packet size in bytes
        int slidingWindowSize;                                  // sliding window size for packet ack system (# of packets)
        int connectionPacketType;                               // connection packet type (so you may override it)
        int numChannels;                                        // number of channels: [1,MaxChannels]
        ChannelConfig channelConfig[MaxChannels];

        ConnectionConfig()
        {
            maxPacketSize = 4 * 1024;
            slidingWindowSize = 1024;
            numChannels = 1;
            connectionPacketType = 0;
        }
    };

    const uint32_t ConnectionContextMagic = 0x11223344;

    struct ConnectionContext
    {
        uint32_t magic;
        MessageFactory * messageFactory;
        const ConnectionConfig * connectionConfig;

        ConnectionContext()
        {
            magic = ConnectionContextMagic;
            messageFactory = NULL;
            connectionConfig = NULL;
        }
    };

    struct ConnectionPacket : public Packet
    {
        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;

        int numChannelEntries;

        ChannelPacketData * channelEntry;

        ConnectionPacket();

        ~ConnectionPacket();

        bool AllocateChannelData( MessageFactory & messageFactory, int numEntries );

        template <typename Stream> bool Serialize( Stream & stream );

        bool SerializeInternal( ReadStream & stream );

        bool SerializeInternal( WriteStream & stream );

        bool SerializeInternal( MeasureStream & stream );

        void SetMessageFactory( MessageFactory & messageFactory ) { m_messageFactory = &messageFactory; }

    private:

        MessageFactory * m_messageFactory;

        ConnectionPacket( const ConnectionPacket & other );

        const ConnectionPacket & operator = ( const ConnectionPacket & other );
    };

    enum ConnectionCounters
    {
        CONNECTION_COUNTER_PACKETS_GENERATED,                   // number of packets generated
        CONNECTION_COUNTER_PACKETS_PROCESSED,                   // number of packets processed
        CONNECTION_COUNTER_PACKETS_ACKED,                       // number of packets acked
        CONNECTION_COUNTER_NUM_COUNTERS
    };

    enum ConnectionError
    {
        CONNECTION_ERROR_NONE = 0,
        CONNECTION_ERROR_CHANNEL = 1,
        CONNECTION_ERROR_OUT_OF_MEMORY = 1
    };

    class ConnectionListener
    {
    public:

        virtual ~ConnectionListener() {}

        virtual void OnConnectionPacketSent( class Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketAcked( class Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketReceived( class Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionFragmentReceived( class Connection * /*connection*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/, int /*channelId*/ ) {}
    };

    struct ConnectionSentPacketData 
    { 
        uint8_t acked;                              // todo: put send time in here for RTT estimate
    };

    struct ConnectionReceivedPacketData {};         // todo: put receive time in here for RTT estimate

    class Connection : public ChannelListener
    {
    public:

        Connection( Allocator & allocator, PacketFactory & packetFactory, MessageFactory & messageFactory, const ConnectionConfig & config = ConnectionConfig() );

        ~Connection();

        void Reset();

        bool CanSendMessage( int channelId = 0 ) const;

        void SendMessage( Message * message, int channelId = 0 );

        Message * ReceiveMessage( int channelId = 0 );

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

        void OnChannelFragmentReceived( class Channel * channel, uint16_t messageId, uint16_t fragmentId, int fragmentBytes );

    private:

        const ConnectionConfig m_config;                                                // const configuration data

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
