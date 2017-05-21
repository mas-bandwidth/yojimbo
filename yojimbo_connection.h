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
    struct ConnectionPacket;

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
