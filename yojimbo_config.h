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

#define YOJIMBO_PLATFORM_WINDOWS                    1
#define YOJIMBO_PLATFORM_MAC                        2
#define YOJIMBO_PLATFORM_UNIX                       3

#if defined(_WIN32)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_MAC
#else
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_UNIX
#endif

#define YOJIMBO_SOCKETS                             1


#if !defined( YOJIMBO_SECURE_MODE )
#define YOJIMBO_SECURE_MODE                         0           // IMPORTANT: This should be set to 1 in your retail build
#endif // #if !defined( YOJIMBO_SECURE_MODE )

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
    const int MaxServersPerConnect = 8;
    const int NonceBytes = 8;
    const int KeyBytes = 32;
    const int AuthBytes = 16;
    const int MacBytes = 16;

    const int MaxContextMappings = MaxClients;
    const int MaxEncryptionMappings = MaxClients * 8;
    const double DefaultEncryptionMappingTimeout = 5;

    const int MaxChannels = 64;
    const int ConservativeConnectionPacketHeaderEstimate = 128;
    const int ConservativeMessageHeaderEstimate = 32;
    const int ConservativeFragmentHeaderEstimate = 64;
    const int ConservativeChannelHeaderEstimate = 32;

    const int DefaultMaxPacketSize = 4 * 1024;
    const int DefaultPacketSendQueueSize = 1024;
    const int DefaultPacketReceiveQueueSize = 1024;
    const int DefaultSocketBufferSize = 1024 * 1024;

    const int ReplayProtectionBufferSize = 64;

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

    struct MessageConfig
    {
        int connectionPacketType;                               ///< Connection packet type (so you can override it). Only necessary to set this if you are using Connection directly. Not necessary to set when using client/server as it overrides it to CLIENT_SERVER_PACKET_CONNECTION for you automatically.
        int maxPacketSize;                                      ///< Limits the maximum size of packets generated to transmit messages between client and server to this size (bytes).
        int slidingWindowSize;                                  ///< The size of the sliding window used for packet acks (# of packets in history). Depending on your packet send rate, it should be configured to cover at least a few seconds worth of packets.
        int numChannels;                                        ///< Number of channels: [1,MaxChannels-1]
        ChannelConfig channel[MaxChannels];                     ///< Per-channel configuration.

        MessageConfig()
        {
            connectionPacketType = 0;
            maxPacketSize = 4 * 1024;
            slidingWindowSize = 1024;
            numChannels = 1;
        }
    };

    const uint32_t ConnectionContextMagic = 0x11223344;

    /** Yojimbo client/server configuration.
     *
     *  This struct is passed to Client and Server constructors to configure them.
     */

    struct ClientServerConfig
    {
        int clientMemory;                                       ///< Memory allocated inside Client for packets, messages and stream allocations (bytes)
        int serverGlobalMemory;                                 ///< Memory allocated inside Server for global connection request and challenge response packets (bytes)
        int serverPerClientMemory;                              ///< Memory allocated inside Server for packets, messages and stream allocations per-client. IMPORTANT: The total amount of memory allocated is this amount multiplied by numClients passed into Server::Start (bytes)
        int numDisconnectPackets;                               ///< Number of disconnect packets to send on clean disconnect. Make sure the other side of the connection receives a disconnect packet and disconnects cleanly, even under packet loss. Without this, the other side times out and this ties up that slot on the server for an extended period.
        float connectionNegotiationSendRate;                    ///< Send rate for packets sent during connection negotiation process. eg. connection request and challenge response packets (packets per-second).
        float connectionNegotiationTimeOut;                     ///< Connection negotiation times out if no response is received from the other side in this amount of time (seconds).
        float connectionKeepAliveSendRate;                      ///< Keep alive packets are sent at this rate between client and server if no other packets are sent by the client or server. Avoids timeout in situations where you are not sending packets at a steady rate (packets per-second).
        float connectionTimeOut;                                ///< Once a connection is established, it times out if it hasn't received any packets from the other side in this amount of time (seconds).
        bool enableMessages;                                    ///< If this is true then you can send messages between client and server. Disable if you don't want to use messages and you want to extend the protocol by adding new packet types instead.
        MessageConfig messageConfig;                            ///< Configures the number and type of message channels for the connection between client and server. Must be identical between client and server to work properly. Only used if enableMessages is true.

        ClientServerConfig()
        {
            clientMemory = 2 * 1024 * 1024;
            serverGlobalMemory = 2 * 1024 * 1024;
            serverPerClientMemory = 2 * 1024 * 1024;
            numDisconnectPackets = 10;
            connectionNegotiationSendRate = 10.0f;
            connectionNegotiationTimeOut = 5.0f;
            connectionKeepAliveSendRate = 10.0f;
            connectionTimeOut = 5.0f;
            enableMessages = true;
        }
    };
}

#endif // #ifndef YOJIMBO_CONFIG_H
