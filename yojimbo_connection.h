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

#ifndef PROTOCOL_CONNECTION_H
#define PROTOCOL_CONNECTION_H

#include "yojimbo_packet.h"
#include "yojimbo_allocator.h"
#include "yojimbo_sequence_buffer.h"

namespace yojimbo
{
    const int CONNECTION_PACKET = 0;        // todo

    struct ConnectionPacket : public Packet
    {
        uint16_t clientId;
        uint16_t serverId;
        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;
//        ChannelData * channelData[MaxChannels];

        ConnectionPacket() : Packet( CONNECTION_PACKET )
        {
            clientId = 0;
            serverId = 0;
            sequence = 0;
            ack = 0;
            ack_bits = 0;
//            memset( channelData, 0, sizeof( ChannelData* ) * MaxChannels );
        }

    private:

        ~ConnectionPacket()
        {
            /*
            for ( int i = 0; i < MaxChannels; ++i )
            {
                if ( channelData[i] )
                {
                    CORE_DELETE( core::memory::scratch_allocator(), ChannelData, channelData[i] );
                    channelData[i] = nullptr;
                }
            }
            */
        }

    public:

        template <typename Stream> bool Serialize( Stream & stream )
        {
            bool perfect;
            if ( Stream::IsWriting )
                 perfect = ack_bits == 0xFFFFFFFF;

            serialize_bool( stream, perfect );

            if ( !perfect )
                serialize_bits( stream, ack_bits, 32 );
            else
                ack_bits = 0xFFFFFFFF;

            serialize_align( stream );

            /*
            if ( Stream::IsWriting )
            {
                for ( int i = 0; i < numChannels; ++i )
                {
                    bool has_data = channelData[i] != nullptr;
                    serialize_bool( stream, has_data );
                }
            }
            else                
            {
                for ( int i = 0; i < numChannels; ++i )
                {
                    bool has_data;
                    serialize_bool( stream, has_data );
                    if ( has_data )
                    {
                        channelData[i] = channelStructure->CreateChannelData( i );
                        CORE_ASSERT( channelData[i] );
                    }
                }
            }
            */

            serialize_bits( stream, sequence, 16 );

            int ack_delta = 0;
            bool ack_in_range = false;

            if ( Stream::IsWriting )
            {
                if ( ack < sequence )
                    ack_delta = sequence - ack;
                else
                    ack_delta = (int)sequence + 65536 - ack;

                assert( ack_delta > 0 );
                
                ack_in_range = ack_delta <= 128;
            }

            serialize_bool( stream, ack_in_range );
    
            if ( ack_in_range )
            {
                serialize_int( stream, ack_delta, 1, 128 );
                if ( Stream::IsReading )
                    ack = sequence - ack_delta;
            }
            else
                serialize_bits( stream, ack, 16 );

            return true;
        }

        YOJIMBO_SERIALIZE_FUNCTIONS();

        bool operator ==( const ConnectionPacket & other ) const
        {
            return sequence == other.sequence &&
                        ack == other.ack &&
                   ack_bits == other.ack_bits;
        }

        bool operator !=( const ConnectionPacket & other ) const
        {
            return !( *this == other );
        }

    private:

        ConnectionPacket( const ConnectionPacket & other );
        const ConnectionPacket & operator = ( const ConnectionPacket & other );
    };

    struct ConnectionConfig
    {
        Allocator * allocator;
        int packetType;
        int maxPacketSize;
        int slidingWindowSize;
        PacketFactory * packetFactory;
        //ChannelStructure * channelStructure;
        const void ** context;

        ConnectionConfig()
        {
            allocator = NULL;
            packetType = 0; // todo -- protocol::CONNECTION_PACKET;
            maxPacketSize = 1024;
            slidingWindowSize = 256;
            packetFactory = NULL;
            //channelStructure = NULL;
            context = NULL;
        }
    };

    struct SentPacketData { uint8_t acked; };

    struct ReceivedPacketData {};
    
    typedef SequenceBuffer<SentPacketData> SentPackets;
    
    typedef SequenceBuffer<ReceivedPacketData> ReceivedPackets;

    class Connection
    {
        //ConnectionError m_error;

        const ConnectionConfig m_config;                            // const configuration data

        Allocator * m_allocator;                                    // allocator for allocations matching life cycle of object.

        double m_time;                                              // current connection time

        SentPackets * m_sentPackets;                                // sliding window of recently sent packets

        ReceivedPackets * m_receivedPackets;                        // sliding window of recently received packets

        /*
        int m_numChannels;                                          // cached number of channels

        Channel * m_channels[MaxChannels];                          // array of channels created according to channel structure
        */

        //uint64_t m_counters[CONNECTION_COUNTER_NUM_COUNTERS];       // counters for unit testing, stats etc.

    public:

        Connection( const ConnectionConfig & config );

        ~Connection();

        /*
        Channel * GetChannel( int index );
        */

        void Reset();

        /*
        void Update( const core::TimeBase & timeBase );
        */

        /*
        ConnectionError GetError() const;
        */

        int GetChannelError( int channelIndex ) const;

        /*
        const core::TimeBase & GetTimeBase() const;
        */

        ConnectionPacket * WritePacket();

        bool ReadPacket( ConnectionPacket * packet );

        uint64_t GetCounter( int index ) const;

        void ProcessAcks( uint16_t ack, uint32_t ack_bits );

        void PacketAcked( uint16_t sequence );
    };
}

#endif
