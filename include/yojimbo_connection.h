/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2026, Más Bandwidth LLC.

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

#include "yojimbo_config.h"
#include "yojimbo_allocator.h"
#include "yojimbo_message.h"
#include "yojimbo_channel.h"

namespace yojimbo
{
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

        /**
            Connection constructor.
            Creates the channels configured in connectionConfig.
            @param allocator The allocator used to create the channels.
            @param messageFactory The message factory for creating and destroying messages.
            @param connectionConfig The connection configuration.
            @param time The current time in seconds.
         */

        Connection( Allocator & allocator, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, double time );

        ~Connection();

        /**
            Reset the connection and all its channels to their initial state.
         */

        void Reset();

        /**
            Is a message able to be sent over a channel?
            @param channelIndex The channel index in [0,numChannels-1].
            @returns True if a message can be sent over the channel.
         */

        bool CanSendMessage( int channelIndex ) const;

        /**
            Are there any messages in the send queue for a channel?
            @param channelIndex The channel index in [0,numChannels-1].
            @returns True if there is at least one message in the channel send queue.
         */

        bool HasMessagesToSend( int channelIndex ) const;

        /**
            Queue a message to be sent over a channel.
            @param channelIndex The channel index in [0,numChannels-1].
            @param message The message to be sent.
            @param context The serialization context, passed through to the message serialize functions. May be NULL.
         */

        void SendMessage( int channelIndex, Message * message, void *context = 0);

        /**
            Pops the next message off a channel receive queue if one is available.
            @param channelIndex The channel index in [0,numChannels-1].
            @returns A pointer to the received message, NULL if there are no messages to receive. The caller owns the message and is responsible for releasing it.
         */

        Message * ReceiveMessage( int channelIndex );

        /**
            Release a message back to the message factory.
            @param message The message to release.
         */

        void ReleaseMessage( Message * message );

        /**
            Generate a connection packet containing data from each channel with data to send.
            @param context The serialization context, passed through to the message serialize functions. May be NULL.
            @param packetSequence The sequence number of the packet being generated.
            @param packetData The buffer to write the packet into.
            @param maxPacketBytes The maximum number of bytes the packet can occupy.
            @param packetBytes [out] The number of bytes written.
            @returns True on success, false on error.
         */

        bool GeneratePacket( void * context, uint16_t packetSequence, uint8_t * packetData, int maxPacketBytes, int & packetBytes );

        /**
            Process a connection packet received from the other side.
            @param context The serialization context, passed through to the message serialize functions. May be NULL.
            @param packetSequence The sequence number of the packet.
            @param packetData The packet data.
            @param packetBytes The number of bytes in the packet.
            @returns True on success, false on error (the connection is put into an error state).
         */

        bool ProcessPacket( void * context, uint16_t packetSequence, const uint8_t * packetData, int packetBytes );

        /**
            Process connection packet acks.
            @param acks Array of connection packet sequence numbers that were acked.
            @param numAcks The number of acks in the array.
         */

        void ProcessAcks( const uint16_t * acks, int numAcks );

        /**
            Advance connection time.
            @param time The current time in seconds.
         */

        void AdvanceTime( double time );

        /**
            Get the connection error level.
            @returns The connection error level.
         */

        ConnectionErrorLevel GetErrorLevel() { return m_errorLevel; }

        /**
            Get the error level of a channel on this connection.
            Use this to find out which channel error drove the connection into CONNECTION_ERROR_CHANNEL.
            @param channelIndex The channel index in [0,numChannels-1].
            @returns The channel error level.
         */

        ChannelErrorLevel GetChannelErrorLevel( int channelIndex ) const;

    private:

        Allocator * m_allocator;                                ///< Allocator passed in to the connection constructor.
        MessageFactory * m_messageFactory;                      ///< Message factory for creating and destroying messages.
        ConnectionConfig m_connectionConfig;                    ///< Connection configuration.
        Channel * m_channel[MaxChannels];                       ///< Array of connection channels. Array size corresponds to m_connectionConfig.numChannels
        ConnectionErrorLevel m_errorLevel;                      ///< The connection error level.
    };
}

#endif // #ifndef YOJIMBO_CONNECTION_H
