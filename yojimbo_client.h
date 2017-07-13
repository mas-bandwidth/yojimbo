/*
    Yojimbo Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.
*/

#ifndef YOJIMBO_CLIENT_H
#define YOJIMBO_CLIENT_H

#include "yojimbo_config.h"
#include "yojimbo_adapter.h"
#include "yojimbo_address.h"
#include "yojimbo_allocator.h"

struct netcode_client_t;
struct reliable_endpoint_t;

/** @file */

namespace yojimbo
{
    class Connection;
    class NetworkSimulator;

    /**
        The set of client states.
     */

    enum ClientState
    {
        CLIENT_STATE_ERROR = -1,
        CLIENT_STATE_DISCONNECTED = 0,
        CLIENT_STATE_CONNECTING,
        CLIENT_STATE_CONNECTED,
    };

    /** 
        Client interface.
     */

    class ClientInterface
    {
    public:

        virtual ~ClientInterface() {}

        /**
            Set the context for reading and writing packets.

            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.

            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time. 

            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Disconnect from the server.
         */

        virtual void Disconnect() = 0;

        /**
            Send packets to server.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from the server.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance client time.

            Call this at the end of each frame to advance the client time forward. 

            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the client connecting to a server?

            This is true while the client is negotiation connection with a server.

            @returns true if the client is currently connecting to, but is not yet connected to a server.
         */

        virtual bool IsConnecting() const = 0;

        /**
            Is the client connected to a server?

            This is true once a client successfully finishes connection negotiatio, and connects to a server. It is false while connecting to a server.

            @returns true if the client is connected to a server.
         */

        virtual bool IsConnected() const = 0;

        /**
            Is the client in a disconnected state?

            A disconnected state corresponds to the client being in the disconnected, or in an error state. Both are logically "disconnected".

            @returns true if the client is disconnected.
         */

        virtual bool IsDisconnected() const = 0;

        /**
            Is the client in an error state?

            When the client disconnects because of an error, it enters into this error state.

            @returns true if the client is in an error state.
         */

        virtual bool ConnectionFailed() const = 0;

        /**
            Get the current client state.
         */

        virtual ClientState GetClientState() const = 0;

        /**
            Get the client index.

            The client index is the slot number that the client is occupying on the server. 

            @returns The client index in [0,maxClients-1], where maxClients is the number of client slots allocated on the server in Server::Start.
         */

        virtual int GetClientIndex() const = 0;

        /**
            Get the current client time.

            @see Client::AdvanceTime
         */

        virtual double GetTime() const = 0;

        // todo: document these methods

        virtual Message * CreateMessage( int type ) = 0;

        virtual uint8_t * AllocateBlock( int bytes ) = 0;

        virtual void AttachBlockToMessage( Message * message, uint8_t * block, int bytes ) = 0;

        virtual void FreeBlock( uint8_t * block ) = 0;

        virtual bool CanSendMessage( int channelIndex ) const = 0;

        virtual void SendMessage( int channelIndex, Message * message ) = 0;

        virtual Message * ReceiveMessage( int channelIndex ) = 0;

        virtual void ReleaseMessage( Message * message ) = 0;
    };

    /**
        Functionality shared across all client implementations.
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

        explicit BaseClient( Allocator & allocator, const BaseClientServerConfig & config, Adapter & adapter, double time );

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

        void SendMessage( int channelIndex, Message * message );

        Message * ReceiveMessage( int channelIndex );

        void ReleaseMessage( Message * message );

    protected:

		uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        void * GetContext() { return m_context; }

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

    private:

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );
        
        static int StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        BaseClientServerConfig m_config;                                    ///< The base client/server configuration.
        Allocator * m_allocator;                                            ///< The allocator passed to the client on creation.
        Adapter * m_adapter;                                                ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                                   ///< Context lets the user pass information to packet serialize functions.
        uint8_t * m_clientMemory;                                           ///< The memory backing the client allocator. Allocated from m_allocator.
        Allocator * m_clientAllocator;                                      ///< The client allocator. Everything allocated between connect and disconnected is allocated and freed via this allocator.
        reliable_endpoint_t * m_endpoint;                                   ///< reliable.io endpoint.
        MessageFactory * m_messageFactory;                                  ///< The client message factory. Created and destroyed on each connection attempt.
        Connection * m_connection;                                          ///< The client connection for exchanging messages with the server.
        NetworkSimulator * m_networkSimulator;                              ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional. 
        ClientState m_clientState;                                          ///< The current client state. See ClientInterface::GetClientState
        int m_clientIndex;                                                  ///< The client slot index on the server [0,maxClients-1]. -1 if not connected.
        double m_time;                                                      ///< The current client time. See ClientInterface::AdvanceTime
		uint8_t * m_packetBuffer;											///< Buffer used to read and write packets.

    private:

        BaseClient( const BaseClient & other );
        
        const BaseClient & operator = ( const BaseClient & other );
    };

    /**
        Client implementation.
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

    private:

        bool GenerateInsecureConnectToken( uint8_t * connectToken, const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses, int timeout = 45 );

        void CreateClient( const Address & address );

        void DestroyClient();

        void StateChangeCallbackFunction( int previous, int current );

        static void StaticStateChangeCallbackFunction( void * context, int previous, int current );

        void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        ClientServerConfig m_config;                                        ///< Client/server configuration.
        netcode_client_t * m_client;                                        ///< netcode.io client data.
        Address m_address;                                                  ///< The client address.
        uint64_t m_clientId;                                                ///< The globally unique client id (set on each call to connect)
    };
}

#endif // #ifndef YOJIMBO_CLIENT_H
