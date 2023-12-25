/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2024, Mas Bandwidth LLC.

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

#ifndef YOJIMBO_H
#define YOJIMBO_H

#include "serialize.h"
#include "yojimbo_config.h"
#include "yojimbo_constants.h"
#include "yojimbo_bit_array.h"
#include "yojimbo_utils.h"
#include "yojimbo_queue.h"
#include "yojimbo_sequence_buffer.h"
#include "yojimbo_address.h"
#include "yojimbo_serialize.h"
#include "yojimbo_message.h"
#include "yojimbo_channel.h"
#include "yojimbo_reliable_ordered_channel.h"
#include "yojimbo_unreliable_unordered_channel.h"
#include "yojimbo_connection.h"
#include "yojimbo_network_simulator.h"
#include "yojimbo_adaptor.h"
#include "yojimbo_network_info.h"
#include "yojimbo_server_interface.h"
#include "yojimbo_base_server.h"
#include "yojimbo_server.h"
#include "yojimbo_client_interface.h"

/** @file */

// windows =p
#ifdef SendMessage
#undef SendMessage
#endif

struct netcode_address_t;
struct netcode_server_t;
struct netcode_client_t;
struct reliable_endpoint_t;

/// The library namespace.

namespace yojimbo
{
    using namespace serialize;
}

/**
    Initialize the yojimbo library.
    Call this before calling any yojimbo library functions.
    @returns True if the library was successfully initialized, false otherwise.
 */

bool InitializeYojimbo();

/**
    Shutdown the yojimbo library.
    Call this after you finish using the library and it will run some checks for you (for example, checking for memory leaks in debug build).
 */

void ShutdownYojimbo();

// ---------------------------------

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
            @param allocator The adapter to the game program. Specifies allocators, message factory to use etc.
         */

        explicit BaseClient( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time );

        ~BaseClient();

        void SetContext( void * context ) { yojimbo_assert( IsDisconnected() ); m_context = context; }

        void Disconnect();

        void AdvanceTime( double time );

        bool IsConnecting() const { return m_clientState == CLIENT_STATE_CONNECTING; }

        bool IsConnected() const { return m_clientState == CLIENT_STATE_CONNECTED; }

        bool IsDisconnected() const { return m_clientState <= CLIENT_STATE_DISCONNECTED; }

        bool ConnectionFailed() const { return m_clientState == CLIENT_STATE_ERROR; }

        ClientState GetClientState() const { return m_clientState; }

        int GetClientIndex() const { return m_clientIndex; }

        double GetTime() const { return m_time; }

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicates( float percent );

        Message * CreateMessage( int type );

        uint8_t * AllocateBlock( int bytes );

        void AttachBlockToMessage( Message * message, uint8_t * block, int bytes );

        void FreeBlock( uint8_t * block );

        bool CanSendMessage( int channelIndex ) const;

        bool HasMessagesToSend( int channelIndex ) const;

        void SendMessage( int channelIndex, Message * message );

        Message * ReceiveMessage( int channelIndex );

        void ReleaseMessage( Message * message );

        void GetNetworkInfo( NetworkInfo & info ) const;

    protected:

        uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        void * GetContext() { return m_context; }

        Adapter & GetAdapter() { yojimbo_assert( m_adapter ); return *m_adapter; }

        void CreateInternal();

        void DestroyInternal();

        void SetClientState( ClientState clientState );

        Allocator & GetClientAllocator() { yojimbo_assert( m_clientAllocator ); return *m_clientAllocator; }

        MessageFactory & GetMessageFactory() { yojimbo_assert( m_messageFactory ); return *m_messageFactory; }

        NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        reliable_endpoint_t * GetEndpoint() { return m_endpoint; }

        Connection & GetConnection() { yojimbo_assert( m_connection ); return *m_connection; }

        virtual void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        virtual int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static int StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static void * StaticAllocateFunction( void * context, uint64_t bytes );

        static void StaticFreeFunction( void * context, void * pointer );

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
        double m_time;                                                      ///< The current client time. See ClientInterface::AdvanceTime
        uint8_t * m_packetBuffer;                                           ///< Buffer used to read and write packets.

    private:

        BaseClient( const BaseClient & other );

        const BaseClient & operator = ( const BaseClient & other );
    };

    /**
        Implementation of client for dedicated servers.
     */

    class Client : public BaseClient
    {
    public:

        /**
            The client constructor.
            @param allocator The allocator for all memory used by the client.
            @param address The address the client should bind to.
            @param config The client/server configuration.
            @param time The current time in seconds. See ClientInterface::AdvanceTime
         */

        explicit Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Client();

        void InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address & address );

        void InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses );

        void Connect( uint64_t clientId, uint8_t * connectToken );

        void Disconnect();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        int GetClientIndex() const;

        uint64_t GetClientId() const { return m_clientId; }

        void ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients );

        void DisconnectLoopback();

        bool IsLoopback() const;

        void ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        const Address & GetAddress() const { return m_boundAddress; }

    private:

        bool GenerateInsecureConnectToken( uint8_t * connectToken,
                                           const uint8_t privateKey[],
                                           uint64_t clientId,
                                           const Address serverAddresses[],
                                           int numServerAddresses );

        void CreateClient( const Address & address );

        void DestroyClient();

        void StateChangeCallbackFunction( int previous, int current );

        static void StaticStateChangeCallbackFunction( void * context, int previous, int current );

        void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        static void StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        ClientServerConfig m_config;                    ///< Client/server configuration.
        netcode_client_t * m_client;                    ///< netcode client data.
        Address m_address;                              ///< Original address passed to ctor.
        Address m_boundAddress;                         ///< Address after socket bind, eg. with valid port
        uint64_t m_clientId;                            ///< The globally unique client id (set on each call to connect)
    };
}

#endif // #ifndef YOJIMBO_H
