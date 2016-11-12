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

#ifndef YOJIMBO_CONFIG_H
#define YOJIMBO_CONFIG_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define YOJIMBO_MAJOR_VERSION 0
#define YOJIMBO_MINOR_VERSION 3
#define YOJIMBO_PATCH_VERSION 0

#if    defined(__386__) || defined(i386)    || defined(__i386__)  \
    || defined(__X86)   || defined(_M_IX86)                       \
    || defined(_M_X64)  || defined(__x86_64__)                    \
    || defined(alpha)   || defined(__alpha) || defined(__alpha__) \
    || defined(_M_ALPHA)                                          \
    || defined(ARM)     || defined(_ARM)    || defined(__arm__)   \
    || defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) \
    || defined(_WIN32_WCE) || defined(__NT__)                     \
    || defined(__MIPSEL__)
  #define YOJIMBO_LITTLE_ENDIAN 1
#else
  #define YOJIMBO_BIG_ENDIAN 1
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#endif // #ifdef _MSC_VER

#define YOJIMBO_PLATFORM_WINDOWS                1
#define YOJIMBO_PLATFORM_MAC                    2
#define YOJIMBO_PLATFORM_UNIX                   3

#if defined(_WIN32)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_MAC
#else
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_UNIX
#endif

#define YOJIMBO_SOCKETS                             1

#define YOJIMBO_INSECURE_CONNECT                    1           // IMPORTANT: You should probably disable this in retail build

#define YOJIMBO_SERIALIZE_CHECKS                    1

#if DEBUG

#define YOJIMBO_DEBUG_MEMORY_LEAKS                  1

#define YOJIMBO_DEBUG_PACKET_LEAKS                  1
    
#define YOJIMBO_DEBUG_MESSAGE_LEAKS                 1

#define YOJIMBO_VALIDATE_PACKET_BUDGET              1

#endif // #if DEBUG

#define YOJIMBO_DEBUG_SPAM                          0

#include <stdint.h>
#include <stdlib.h>

namespace yojimbo
{
    const int MaxClients = 64;
    const int MaxConnectTokenEntries = MaxClients * 16;
    const int ConnectTokenBytes = 1024;
    const int ChallengeTokenBytes = 256;
    const int MaxServersPerConnectToken = 8;
    const int NonceBytes = 8;
    const int KeyBytes = 32;
    const int AuthBytes = 16;
    const int MacBytes = 16;

    const int MaxContextMappings = MaxClients;
    const int MaxEncryptionMappings = MaxClients * 16;
    const double DefaultEncryptionMappingTimeout = 10;

    const int MaxChannels = 64;
    const int ConservativeConnectionPacketHeaderEstimate = 128;
    const int ConservativeMessageHeaderEstimate = 32;
    const int ConservativeFragmentHeaderEstimate = 64;
    const int ConservativeChannelHeaderEstimate = 32;

    const int DefaultMaxPacketSize = 4 * 1024;
    const int DefaultPacketSendQueueSize = 1024;
    const int DefaultPacketReceiveQueueSize = 1024;
    const int DefaultSocketBufferSize = 1024 * 1024;

    const uint32_t LocalProtocolId = 1;

    enum ChannelType
    {
        CHANNEL_TYPE_RELIABLE_ORDERED,                          // reliable ordered stream of messages
        CHANNEL_TYPE_UNRELIABLE_UNORDERED                       // unreliable unordered stream of messages
    };

    struct ChannelConfig
    {
        ChannelType type;                                       // channel type: reliable ordered or unreliable unordered.
        int packetBudget;                                       // maximum bytes of message data per-packet. -1 = no limit
        int sendQueueSize;                                      // message send queue size
        int receiveQueueSize;                                   // message receive queue size
        int maxMessagesPerPacket;                               // maximum number of messages per-packet
        float messageResendTime;                                // message resend time (seconds)
        int maxBlockSize;                                       // maximum block size in bytes
        int fragmentSize;                                       // block fragments size in bytes
        float fragmentResendTime;                               // fragment resend time (seconds)
        int sentPacketBufferSize;                               // size of sent packets buffer in # of packets stored (maps packet level acks to messages & fragments)
        bool disableBlocks;                                     // disable blocks for this channel. saves maxBlockSize * 2 in memory.

        ChannelConfig() : type ( CHANNEL_TYPE_RELIABLE_ORDERED )
        {
            packetBudget = 1100;
            sendQueueSize = 1024;
            receiveQueueSize = 1024;
            maxMessagesPerPacket = 64;
            messageResendTime = 0.1f;
            maxBlockSize = 256 * 1024;
            fragmentSize = 1024;
            fragmentResendTime = 0.25f;
            sentPacketBufferSize = 1024;
            disableBlocks = false;
        }

        int GetMaxFragmentsPerBlock() const
        {
            return maxBlockSize / fragmentSize;
        }
    };

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

    struct ClientServerConfig
    {
        int clientMemory;                                       // memory allocated on the client for packets, messages and stream allocations (bytes)
        int serverGlobalMemory;                                 // memory allocated for global connection request handling on the server (bytes)
        int serverPerClientMemory;                              // memory allocated for packets, messages and stream allocations per-client (bytes)

        int numDisconnectPackets;                               // number of disconnect packets to spam on clean disconnect. avoids timeout.
        
        float connectionRequestSendRate;                        // seconds between connection request packets sent.
        float connectionResponseSendRate;                       // seconds between connection response packets sent.
        float connectionKeepAliveSendRate;                      // seconds between keep alive packets sent after connection has been confirmed. sent only if other packets are not sent.
        float connectionRequestTimeOut;                         // seconds before client connection requests gives up and times out.
        float challengeResponseTimeOut;                         // seconds before challenge response times out.
        float connectionTimeOut;                                // seconds before connection times out after connection has been established.

#if YOJIMBO_INSECURE_CONNECT
        float insecureConnectSendRate;                          // seconds between insecure connect packets sent.
        float insecureConnectTimeOut;                           // time in seconds after with an insecure connection request times out.
#endif // #if YOJIMBO_INSECURE_CONNECT

        bool enableConnection;                                  // enable per-client connection and messages.

        ConnectionConfig connectionConfig;                      // connection configuration.

        ClientServerConfig()
        {
            clientMemory = 2 * 1024 * 1024;
            serverGlobalMemory = 2 * 1024 * 1024;
            serverPerClientMemory = 2 * 1024 * 1024;
            numDisconnectPackets = 10;
            connectionRequestSendRate = 0.1f;
            connectionResponseSendRate = 0.1f;
            connectionKeepAliveSendRate = 0.1f;
            connectionRequestTimeOut = 5.0f;
            challengeResponseTimeOut = 5.0f;
            connectionTimeOut = 10.0f;
            enableConnection = true;
#if YOJIMBO_INSECURE_CONNECT
            insecureConnectSendRate = 0.1f;
            insecureConnectTimeOut = 5.0f;
#endif // #if YOJIMBO_INSECURE_CONNECT
        }        
    };

    struct ConnectionContext
    {
        uint32_t magic;
        class MessageFactory * messageFactory;
        const ConnectionConfig * connectionConfig;

        ConnectionContext()
        {
            magic = ConnectionContextMagic;
            messageFactory = NULL;
            connectionConfig = NULL;
        }
    };

    struct ClientServerContext : public ConnectionContext {};
}

#endif // #ifndef YOJIMBO_CONFIG_H
