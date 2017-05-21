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

// windows =p
#ifdef SendMessage
#undef SendMessage
#endif

/** @file */

namespace yojimbo
{
    // todo: does connection packet even need to be exposed outside of yojimbo_connection.cpp. I don't think it does... move it there!

    /** 
        This packet carries messages sent across connection channels.

        Connection packets should be generated and sent at a steady rate like 10, 20 or 30 times per-second in both directions across a connection. 
     */

    struct ConnectionPacket
    {
        int numChannelEntries;                                                  ///< The number of channel entries in this packet.
        ChannelPacketData * channelEntry;                                       ///< Per-channel message data that was included in this packet.
        MessageFactory * messageFactory;                                        ///< The message factory is cached so we can release messages included in this packet when it is destroyed.

        /**
            Connection packet constructor.
         */

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
         */

        bool AllocateChannelData( MessageFactory & messageFactory, int numEntries );

        /** 
            The template function for serializing the connection packet.

            Unifies packet read and write, making it harder to accidentally desync one from the other.
         */

        template <typename Stream> bool Serialize( Stream & stream, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig );

        /// Implements serialize read by calling into ConnectionPacket::Serialize with a ReadStream.

        bool SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig );

        /// Implements serialize write by calling into ConnectionPacket::Serialize with a WriteStream.

        bool SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig );

        /// Implements serialize measure by calling into ConnectionPacket::Serialize with a MeasureStream.

        bool SerializeInternal( MeasureStream & stream, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig );

    private:

        ConnectionPacket( const ConnectionPacket & other );

        const ConnectionPacket & operator = ( const ConnectionPacket & other );
    };

    /**
        Connection class.
     */

    class Connection
    {
    public:

        Connection( Allocator & allocator, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig = ConnectionConfig() );

        ~Connection();

        void Reset();

        bool GeneratePacket( uint16_t packetSequence, uint8_t * packetData, int maxPacketBytes, int & packetBytes );

        void ProcessAcks( const uint16_t * acks, int numAcks );

        bool ProcessPacket( uint16_t packetSequence, const uint8_t * packetData, int packetBytes );

        void AdvanceTime( double time );

    private:

        int WritePacket( Allocator & allocator, void * context, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, ConnectionPacket & packet, uint8_t * buffer, int bufferSize );

        Allocator * m_allocator;                                ///< Allocator passed in to the connection constructor.
        MessageFactory * m_messageFactory;                      ///< Message factory for creating and destroying messages.
        ConnectionConfig m_connectionConfig;                    ///< Connection configuration.
        Channel * m_channel[MaxChannels];                       ///< Array of connection channels. Array size corresponds to m_connectionConfig.numChannels
    };
}

#endif // #ifndef YOJIMBO_CONNECTION
