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

#ifndef YOJIMBO_BASE_CLIENT_H
#define YOJIMBO_BASE_CLIENT_H

#include "yojimbo_config.h"
#include "yojimbo_platform.h"
#include "yojimbo_client_interface.h"
#include <stdlib.h>

struct reliable_endpoint_t;

namespace yojimbo
{
    /**
        Functionality that is common across all client implementations.
     */

    class BaseClient : public ClientInterface
    {
    public:

        /**
            Base client constructor.
            @param allocator The allocator for all memory used by the client.
            @param config The base client/server configuration.
            @param time The current time in seconds. See ClientInterface::AdvanceTime
            @param adapter The adapter to the game program. Specifies allocators, message factory to use etc.
         */

        explicit BaseClient( class Allocator & allocator, const ClientServerConfig & config, class Adapter & adapter, double time );

        /**
            Base client destructor.
            IMPORTANT: Disconnect the client before destroying it.
         */

        ~BaseClient();

        void SetContext( void * context ) YOJIMBO_OVERRIDE { yojimbo_assert( IsDisconnected() ); m_context = context; }

        void Disconnect() YOJIMBO_OVERRIDE;

        void AdvanceTime( double time ) YOJIMBO_OVERRIDE;

        bool IsConnecting() const YOJIMBO_OVERRIDE { return m_clientState == CLIENT_STATE_CONNECTING; }

        bool IsConnected() const YOJIMBO_OVERRIDE { return m_clientState == CLIENT_STATE_CONNECTED; }

        bool IsDisconnected() const YOJIMBO_OVERRIDE { return m_clientState <= CLIENT_STATE_DISCONNECTED; }

        bool ConnectionFailed() const YOJIMBO_OVERRIDE { return m_clientState == CLIENT_STATE_ERROR; }

        ClientState GetClientState() const YOJIMBO_OVERRIDE { return m_clientState; }

        int GetClientIndex() const YOJIMBO_OVERRIDE { return m_clientIndex; }

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

        Message * CreateMessage( int type ) YOJIMBO_OVERRIDE;

        uint8_t * AllocateBlock( int bytes ) YOJIMBO_OVERRIDE;

        void AttachBlockToMessage( Message * message, uint8_t * block, int bytes ) YOJIMBO_OVERRIDE;

        void FreeBlock( uint8_t * block ) YOJIMBO_OVERRIDE;

        bool CanSendMessage( int channelIndex ) const YOJIMBO_OVERRIDE;

        /**
            Are there any messages in the send queue for a channel?
            @param channelIndex The channel index in [0,numChannels-1].
            @returns True if there is at least one message in the channel send queue.
         */

        bool HasMessagesToSend( int channelIndex ) const;

        void SendMessage( int channelIndex, Message * message ) YOJIMBO_OVERRIDE;

        Message * ReceiveMessage( int channelIndex ) YOJIMBO_OVERRIDE;

        void ReleaseMessage( Message * message ) YOJIMBO_OVERRIDE;

        void GetNetworkInfo( NetworkInfo & info ) const YOJIMBO_OVERRIDE;

        /**
            Get the reason this client was last disconnected.
            See ClientDisconnectReason for the values. Cleared back to YOJIMBO_CLIENT_DISCONNECT_REASON_NONE
            when a new connect attempt starts.
            @returns The disconnect reason (a ClientDisconnectReason value). YOJIMBO_CLIENT_DISCONNECT_REASON_NONE if no disconnect has happened yet.
         */

        int GetDisconnectReason() const { return m_disconnectReason; }

    protected:

        /**
            Set the reason this client was last disconnected.
            @param disconnectReason The disconnect reason (a ClientDisconnectReason value).
         */

        void SetDisconnectReason( int disconnectReason ) { m_disconnectReason = disconnectReason; }

        /**
            Get the buffer used to read and write packets.
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
            Allocate everything the client needs to run.
            Transactional: on the first allocation or adapter factory failure everything already
            created is destroyed, the client is left as it was before the call, and false is
            returned. Checked in every build, not by an assert.
            @returns True if the client was created, false if it ran out of memory.
         */

        bool CreateInternal();

        /**
            Destroy everything created by CreateInternal.
            Safe to call on a partially created client: every pointer is checked rather than assumed.
         */

        void DestroyInternal();

        /**
            Set the client state.
            @param clientState The new client state.
         */

        void SetClientState( ClientState clientState );

        /**
            Get the client allocator. Everything allocated between connect and disconnect
            uses this allocator.
            @returns The client allocator.
         */

        class Allocator & GetClientAllocator() { yojimbo_assert( m_clientAllocator ); return *m_clientAllocator; }

        /**
            Get the client message factory, created and destroyed on each connection attempt.
            @returns The message factory.
         */

        class MessageFactory & GetMessageFactory() { yojimbo_assert( m_messageFactory ); return *m_messageFactory; }

        /**
            Get the network simulator, if configured.
            @returns The network simulator, or NULL if ClientServerConfig::networkSimulator is false.
         */

        class NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        /**
            Get the reliable endpoint.
            @returns The reliable endpoint, or NULL if the client has not been created.
         */

        reliable_endpoint_t * GetEndpoint() { return m_endpoint; }

        /**
            Get the client connection used to exchange messages with the server.
            @returns The connection.
         */

        class Connection & GetConnection() { yojimbo_assert( m_connection ); return *m_connection; }

        /**
            Transmit a packet to the server. Implemented by the derived class.
            @param packetSequence The sequence number of the packet.
            @param packetData The packet data.
            @param packetBytes The number of bytes in the packet.
         */

        virtual void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        /**
            Process a packet received from the server. Implemented by the derived class.
            @param packetSequence The sequence number of the packet.
            @param packetData The packet data.
            @param packetBytes The number of bytes in the packet.
            @returns 1 to accept and ack the packet, 0 to reject it.
         */

        virtual int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        /**
            Static trampoline passed to reliable as the transmit packet function.
            Casts context to a BaseClient and calls TransmitPacketFunction.
         */

        static void StaticTransmitPacketFunction( void * context, uint64_t index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        /**
            Static trampoline passed to reliable as the process packet function.
            Casts context to a BaseClient and calls ProcessPacketFunction.
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
            Reset the client connection to its initial state.
         */

        virtual void Reset();

    private:

        ClientServerConfig m_config;                                        ///< The client/server configuration.
        Allocator * m_allocator;                                            ///< The allocator passed to the client on creation.
        Adapter * m_adapter;                                                ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                                   ///< Context lets the user pass information to packet serialize functions.
        uint8_t * m_clientMemory;                                           ///< The memory backing the client allocator. Allocated from m_allocator.
        Allocator * m_clientAllocator;                                      ///< The client allocator. Everything allocated between connect and disconnected is allocated and freed via this allocator.
        reliable_endpoint_t * m_endpoint;                                   ///< reliable endpoint.
        MessageFactory * m_messageFactory;                                  ///< The client message factory. Created and destroyed on each connection attempt.
        Connection * m_connection;                                          ///< The client connection for exchanging messages with the server.
        NetworkSimulator * m_networkSimulator;                              ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional.
        ClientState m_clientState;                                          ///< The current client state. See ClientInterface::GetClientState
        int m_clientIndex;                                                  ///< The client slot index on the server [0,maxClients-1]. -1 if not connected.
        int m_disconnectReason;                                             ///< The reason this client was last disconnected (ClientDisconnectReason). Cleared back to none when a new connect attempt starts.
        double m_time;                                                      ///< The current client time. See ClientInterface::AdvanceTime
        uint8_t * m_packetBuffer;                                           ///< Buffer used to read and write packets.

    private:

        BaseClient( const BaseClient & other );

        const BaseClient & operator = ( const BaseClient & other );
    };
}

#endif // #ifndef YOJIMBO_BASE_CLIENT_H
