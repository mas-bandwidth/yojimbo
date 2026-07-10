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

#ifndef YOJIMBO_SERVER_H
#define YOJIMBO_SERVER_H

#include "yojimbo_config.h"
#include "yojimbo_base_server.h"
#include "yojimbo_address.h"

struct netcode_server_t;

namespace yojimbo
{
    /**
        The reason the client in a server client slot was last disconnected.

        This lets you distinguish problems you need to act on (eg. serialize failures indicating a client/server protocol
        mismatch, or per-client memory exhaustion indicating an undersized config) from normal network behavior (a client
        cleanly disconnecting or timing out), and route that into your own logging, metrics and analytics.

        Tracked per-client slot. All slots reset to YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE when the server starts,
        and a slot resets back to NONE when a new client connects to it. The reason is recorded before
        Adapter::OnServerClientDisconnected is called, so you can query it from inside that callback.

        @see Server::GetClientDisconnectReason
     */

    enum ServerClientDisconnectReason
    {
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE = 0,                   ///< No client has disconnected from this slot since the server started, or a new client has since connected to this slot.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_DISCONNECTED,               ///< The client cleanly disconnected. It sent disconnect packets, eg. the player quit.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_TIMED_OUT,                  ///< The client timed out. We stopped hearing from it, eg. network problem or the client crashed.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_KICKED,                     ///< Server code called Server::DisconnectClient or Server::DisconnectAllClients for this client.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_FAILED_TO_SERIALIZE,        ///< A message from this client failed to serialize read. Usually a client/server protocol version mismatch, or a bug in a message serialize function.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_DESYNC,                     ///< A channel for this client desynced and cannot recover. See CHANNEL_ERROR_DESYNC.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_SEND_QUEUE_FULL,            ///< A channel send queue for this client filled up. See CHANNEL_ERROR_SEND_QUEUE_FULL.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_BLOCKS_DISABLED,            ///< This client sent block data on a channel that is configured with blocks disabled.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_MESSAGE_TOO_LARGE,          ///< A message sent to this client is too large to ever fit into a packet. See CHANNEL_ERROR_MESSAGE_TOO_LARGE.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_OUT_OF_MEMORY,              ///< The per-client memory budget for this client was exhausted. Consider increasing ClientServerConfig::serverPerClientMemory.
        YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_READ_PACKET_FAILED,         ///< A connection packet from this client failed to deserialize.
    };

    /// Helper function to convert a server client disconnect reason to a user friendly string.

    inline const char * GetServerClientDisconnectReasonString( int reason )
    {
        switch ( reason )
        {
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE:                  return "none";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_DISCONNECTED:          return "disconnected";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_TIMED_OUT:             return "timed out";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_KICKED:                return "kicked";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_FAILED_TO_SERIALIZE:   return "failed to serialize";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_DESYNC:                return "desync";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_SEND_QUEUE_FULL:       return "send queue full";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_BLOCKS_DISABLED:       return "blocks disabled";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_MESSAGE_TOO_LARGE:     return "message too large";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_OUT_OF_MEMORY:         return "out of memory";
            case YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_READ_PACKET_FAILED:    return "read packet failed";
            default:
                yojimbo_assert( false );
                return "(unknown)";
        }
    }

    /**
        Server implementation.
     */

    class Server : public BaseServer
    {
    public:

        Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Server();

        void Start( int maxClients );

        void Stop();

        void DisconnectClient( int clientIndex );

        void DisconnectAllClients();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        bool IsClientConnected( int clientIndex ) const;

        uint64_t GetClientId( int clientIndex ) const;

        const uint8_t * GetClientUserData( int clientIndex ) const;

        netcode_address_t * GetClientAddress( int clientIndex ) const;

        int GetNumConnectedClients() const;

        void ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData );

        void DisconnectLoopbackClient( int clientIndex );

        bool IsLoopbackClient( int clientIndex ) const;

        void ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        const Address & GetAddress() const { return m_boundAddress; }

    private:

        void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void ConnectDisconnectCallbackFunction( int clientIndex, int connected );

        void SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        static void StaticConnectDisconnectCallbackFunction( void * context, int clientIndex, int connected );

        static void StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        ClientServerConfig m_config;
        netcode_server_t * m_server;
        Address m_address;                                  // original address passed to ctor
        Address m_boundAddress;                             // address after socket bind, eg. valid port
        uint8_t m_privateKey[KeyBytes];
    };
}

#endif // #ifndef YOJIMBO_SERVER_H
