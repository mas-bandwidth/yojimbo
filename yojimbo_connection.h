/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
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
        Connection class.
     */

    class Connection
    {
    public:

        Connection( Allocator & allocator, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, double time );

        ~Connection();

        void Reset();

        bool CanSendMessage( int channelIndex ) const;

        void SendMessage( int channelIndex, Message * message );

        Message * ReceiveMessage( int channelIndex );

        void ReleaseMessage( Message * message );

        bool GeneratePacket( void * context, uint16_t packetSequence, uint8_t * packetData, int maxPacketBytes, int & packetBytes );

        bool ProcessPacket( void * context, uint16_t packetSequence, const uint8_t * packetData, int packetBytes );

        void ProcessAcks( const uint16_t * acks, int numAcks );

        void AdvanceTime( double time );

        ConnectionErrorLevel GetErrorLevel() { return m_errorLevel; }

    private:

        Allocator * m_allocator;                                ///< Allocator passed in to the connection constructor.
        MessageFactory * m_messageFactory;                      ///< Message factory for creating and destroying messages.
        ConnectionConfig m_connectionConfig;                    ///< Connection configuration.
        Channel * m_channel[MaxChannels];                       ///< Array of connection channels. Array size corresponds to m_connectionConfig.numChannels
        ConnectionErrorLevel m_errorLevel;                      ///< The connection error level.
    };
}

#endif // #ifndef YOJIMBO_CONNECTION
