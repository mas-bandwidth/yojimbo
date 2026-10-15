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

#ifndef YOJIMBO_BASE_SERVER_H
#define YOJIMBO_BASE_SERVER_H

#include "yojimbo_config.h"
#include "yojimbo_allocator.h"
#include "yojimbo_server_interface.h"

struct reliable_endpoint_t;

namespace yojimbo
{
    /**
        Common functionality across all server implementations.
     */

    class BaseServer : public ServerInterface
    {
    public:

        /**
            Base server constructor.
            @param allocator The allocator for all memory used by the server.
            @param config The base client/server configuration.
            @param adapter The adapter to the game program. Specifies allocators, message factory to use etc.
            @param time The current time in seconds. See ServerInterface::AdvanceTime
         */

        BaseServer( Allocator & allocator, const ClientServerConfig & config, class Adapter & adapter, double time );

        /**
            Base server destructor.
            IMPORTANT: Stop the server before destroying it.
         */

        ~BaseServer();

        void SetContext( void * context ) YOJIMBO_OVERRIDE;

        /**
            Allocate the global and per-client memory the server needs to run.
            Transactional: on the first allocation or adapter factory failure everything already
            created is destroyed, the server is left stopped, and false is returned. Checked in
            every build, not by an assert.
         */

        bool Start( int maxClients ) YOJIMBO_OVERRIDE;

        void Stop() YOJIMBO_OVERRIDE;

        void AdvanceTime( double time ) YOJIMBO_OVERRIDE;

        bool IsRunning() const YOJIMBO_OVERRIDE { return m_running; }

        int GetMaxClients() const YOJIMBO_OVERRIDE { return m_maxClients; }

        double GetTime() const YOJIMBO_OVERRIDE { return m_time; }

        /**
            Set the latency applied by the network simulator.
            Only has an effect when ClientServerConfig::networkSimulator is true.
            @param milliseconds The latency in milliseconds.
         */

        void SetLatency( float milliseconds );

        /**
            Set the jitter applied by the network simulator.
            Only has an effect when ClientServerConfig::networkSimulator is true.
            @param milliseconds The jitter in milliseconds.
         */

        void SetJitter( float milliseconds );

        /**
            Set the packet loss applied by the network simulator.
            Only has an effect when ClientServerConfig::networkSimulator is true.
            @param percent The packet loss percentage, in [0,100].
         */

        void SetPacketLoss( float percent );

        /**
            Set the duplicate packet percentage applied by the network simulator.
            Only has an effect when ClientServerConfig::networkSimulator is true.
            @param percent The duplicate percentage, in [0,100].
         */

        void SetDuplicates( float percent );

        Message * CreateMessage( int clientIndex, int type ) YOJIMBO_OVERRIDE;

        uint8_t * AllocateBlock( int clientIndex, int bytes ) YOJIMBO_OVERRIDE;

        void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes ) YOJIMBO_OVERRIDE;

        void FreeBlock( int clientIndex, uint8_t * block ) YOJIMBO_OVERRIDE;

        bool CanSendMessage( int clientIndex, int channelIndex ) const YOJIMBO_OVERRIDE;

        /**
            Are there any messages in the send queue for a client's channel?
            @param clientIndex The index of the client slot in [0,maxClients-1].
            @param channelIndex The channel index in [0,numChannels-1].
            @returns True if there is at least one message in the channel send queue.
         */

        bool HasMessagesToSend( int clientIndex, int channelIndex ) const;

        void SendMessage( int clientIndex, int channelIndex, Message * message ) YOJIMBO_OVERRIDE;

        Message * ReceiveMessage( int clientIndex, int channelIndex ) YOJIMBO_OVERRIDE;

        void ReleaseMessage( int clientIndex, Message * message ) YOJIMBO_OVERRIDE;

        void GetNetworkInfo( int clientIndex, NetworkInfo & info ) const YOJIMBO_OVERRIDE;

        /**
            Get the reason the client in this slot was last disconnected.
            Valid while the server is running. Recorded before Adapter::OnServerClientDisconnected is called,
            so you can query it from inside that callback. See ServerClientDisconnectReason for the values.
            @param clientIndex The index of the client slot in [0,maxClients-1].
            @returns The disconnect reason (a ServerClientDisconnectReason value). YOJIMBO_SERVER_CLIENT_DISCONNECT_REASON_NONE if no client has disconnected from this slot since the server started, or if a new client has since connected to it.
         */

        int GetClientDisconnectReason( int clientIndex ) const;

    protected:

        /**
            Set the reason the client in this slot was last disconnected.
            @param clientIndex The index of the client slot in [0,maxClients-1].
            @param disconnectReason The disconnect reason (a ServerClientDisconnectReason value).
         */

        void SetClientDisconnectReason( int clientIndex, int disconnectReason );

        /**
            Get the buffer used to write packets.
            @returns The packet buffer.
         */

        uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        /**
            Get the serialization context passed to message serialize functions.
            @returns The context, or NULL if none was set.
         */

        void * GetContext() { return m_context; }

        /**
            Get the adapter passed in on construction.
            @returns The adapter.
         */

        Adapter & GetAdapter() { yojimbo_assert( m_adapter ); return *m_adapter; }

        /**
            Get the global allocator, used for allocations that don't belong to a specific client.
            @returns The global allocator.
         */

        Allocator & GetGlobalAllocator() { yojimbo_assert( m_globalAllocator ); return *m_globalAllocator; }

        /**
            Get the message factory for a client slot.
            @param clientIndex The index of the client slot in [0,maxClients-1].
            @returns The per-client message factory.
         */

        class MessageFactory & GetClientMessageFactory( int clientIndex );

        /**
            Get the network simulator, if configured.
            @returns The network simulator, or NULL if ClientServerConfig::networkSimulator is false.
         */

        class NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        /**
            Get the reliable endpoint for a client slot.
            @param clientIndex The index of the client slot in [0,maxClients-1].
            @returns The per-client reliable endpoint.
         */

        reliable_endpoint_t * GetClientEndpoint( int clientIndex );

        /**
            Get the connection used to exchange messages with a client.
            @param clientIndex The index of the client slot in [0,maxClients-1].
            @returns The per-client connection.
         */

        class Connection & GetClientConnection( int clientIndex );

        /**
            Transmit a packet to a client. Implemented by the derived class.
            @param clientIndex The index of the client slot in [0,maxClients-1].
            @param packetSequence The sequence number of the packet.
            @param packetData The packet data.
            @param packetBytes The number of bytes in the packet.
         */

        virtual void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        /**
            Process a packet received from a client. Implemented by the derived class.
            @param clientIndex The index of the client slot in [0,maxClients-1].
            @param packetSequence The sequence number of the packet.
            @param packetData The packet data.
            @param packetBytes The number of bytes in the packet.
            @returns 1 to accept and ack the packet, 0 to reject it.
         */

        virtual int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        /**
            Static trampoline passed to reliable as the transmit packet function.
            Casts context to a BaseServer and calls TransmitPacketFunction.
         */

        static void StaticTransmitPacketFunction( void * context, uint64_t index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        /**
            Static trampoline passed to reliable as the process packet function.
            Casts context to a BaseServer and calls ProcessPacketFunction.
         */

        static int StaticProcessPacketFunction( void * context, uint64_t index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        /**
            Static trampoline passed to reliable as the allocate function.
            Casts context to an Allocator and allocates bytes from it.
         */

        static void * StaticAllocateFunction( void * context, size_t bytes );

        /**
            Static trampoline passed to reliable as the free function.
            Casts context to an Allocator and frees the pointer with it.
         */

        static void StaticFreeFunction( void * context, void * pointer );

    protected:

        /**
            Reset a client's connection to its initial state.
            @param clientIndex The index of the client slot in [0,maxClients-1].
         */

        virtual void ResetClient( int clientIndex );

    private:

        ClientServerConfig m_config;                                ///< Base client/server config.
        Allocator * m_allocator;                                    ///< Allocator passed in to constructor.
        Adapter * m_adapter;                                        ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                           ///< Optional serialization context.
        int m_maxClients;                                           ///< Maximum number of clients supported.
        bool m_running;                                             ///< True if server is currently running, eg. after "Start" is called, before "Stop".
        double m_time;                                              ///< Current server time in seconds.
        uint8_t * m_globalMemory;                                   ///< The block of memory backing the global allocator. Allocated with m_allocator.
        uint8_t * m_clientMemory[MaxClients];                       ///< The block of memory backing the per-client allocators. Allocated with m_allocator.
        Allocator * m_globalAllocator;                              ///< The global allocator. Used for allocations that don't belong to a specific client.
        Allocator * m_clientAllocator[MaxClients];                  ///< Array of per-client allocator. These are used for allocations related to connected clients.
        MessageFactory * m_clientMessageFactory[MaxClients];        ///< Array of per-client message factories. This silos message allocations per-client slot.
        Connection * m_clientConnection[MaxClients];                ///< Array of per-client connection classes. This is how messages are exchanged with clients.
        reliable_endpoint_t * m_clientEndpoint[MaxClients];         ///< Array of per-client reliable endpoints.
        int m_clientDisconnectReason[MaxClients];                   ///< Per-client slot reason the last client in that slot was disconnected (ServerClientDisconnectReason). Reset to none at server start, and when a new client connects to the slot.
        NetworkSimulator * m_networkSimulator;                      ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional.
        uint8_t * m_packetBuffer;                                   ///< Buffer used when writing packets.
    };
}

#endif // #ifndef YOJIMBO_BASE_SERVER_H
