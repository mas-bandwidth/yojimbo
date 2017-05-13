/*
    Yojimbo Network Library.
    
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

#ifndef YOJIMBO_CLIENT_H
#define YOJIMBO_CLIENT_H

#include <assert.h>
#include "yojimbo_config.h"
#include "yojimbo_allocator.h"

/** @file */

namespace yojimbo
{
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

        virtual bool IsError() const = 0;

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

        /**
            Get the protocol id.
         */

        virtual uint64_t GetProtocolId() const = 0;
    };

    /**
        Functionality shared across all client implementations.
     */

    class BaseClient : public ClientInterface
    {
    public:

        /**
            The base client constructor.

            @param allocator The allocator for all memory used by the client.
            @param config The base client/server configuration.
            @param time The current time in seconds. See Client::AdvanceTime
         */

        explicit BaseClient( Allocator & allocator, const BaseClientServerConfig & config, double time );

        /**
            Base client destructor.
         */

        ~BaseClient();

        void SetContext( void * context ) { assert( IsDisconnected() ); m_context = context; }

        void Disconnect();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        bool IsConnecting() const { return m_clientState == CLIENT_STATE_CONNECTING; }

        bool IsConnected() const { return m_clientState == CLIENT_STATE_CONNECTED; }

        bool IsDisconnected() const { return m_clientState <= CLIENT_STATE_DISCONNECTED; }

        bool IsError() const { return m_clientState == CLIENT_STATE_ERROR; }

        ClientState GetClientState() const { return m_clientState; }

        int GetClientIndex() const { return m_clientIndex; }

        double GetTime() const { return m_time; }

        uint64_t GetProtocolId() const { return m_config.protocolId; }
    protected:

        virtual void CreateAllocators();

        virtual void DestroyAllocators();

        virtual Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes );

    private:

        BaseClientServerConfig m_config;                                    ///< The base client/server configuration.
        Allocator * m_allocator;                                            ///< The allocator passed to the client on creation.
        void * m_context;                                                   ///< Context lets the user pass information to packet serialize functions.
        uint8_t * m_clientMemory;                                           ///< The memory backing the client allocator. Allocated from m_allocator.
        Allocator * m_clientAllocator;                                      ///< The client allocator. Everything allocated between connect and disconnected is allocated and freed via this allocator.
        uint64_t m_clientId;                                                ///< The globally unique client id (set on each call to connect).
        int m_clientIndex;                                                  ///< The client slot index on the server [0,maxClients-1]. -1 if not connected.
        ClientState m_clientState;                                          ///< The current client state. See ClientInterface::GetClientState.
        double m_time;                                                      ///< The current client time. See ClientInterface::AdvanceTime

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
            @param config The client/server configuration.
            @param time The current time in seconds. See Client::AdvanceTime
         */

        explicit Client( Allocator & allocator, const ClientServerConfig & config, double time );

        /**
            Client destructor.
         */

        ~Client();

#ifndef YOJIMBO_SECURE_MODE

        void InsecureConnect( uint64_t clientId, const Address & address );

        void InsecureConnect( uint64_t clientId, const Address serverAddresses[], int numServerAddresses );

#endif // #ifndef YOJIMBO_SECURE_MODE

        void Connect( uint8_t * connectToken );

        // ...

    private:

        // ...
    };
}













// old code below!


#if 0 // todo



        /**
            Disconnect from the server.

            This function is safe to call if not currently connected. In that case it will do nothing.

            @param clientState The client state to transition to on disconnect. By default CLIENT_STATE_DISCONNECTED, but you may also specify one of the negative client state values for error states.
            @param sendDisconnectPacket If true then disconnect packets will be sent immediately to the server to notify it that the client disconnected. This makes the server client slot get cleaned up and recycled faster for other clients to connect, vs. timing out in 5-10 seconds. The only situation where this should be false is if the client is disconnecting because it timed out from the server.
         */

        void Disconnect( ClientState clientState = CLIENT_STATE_DISCONNECTED, bool sendDisconnectPacket = true );

        /**
            Send packets to server.
         
            This function drives the sending of packets to the server such as connection request, challenge response, keep-alive packet and connection packets to transmit messages from client to server.

            Packets sent from this function are queued for sending on the transport, and are not actually serialized and sent to the network until you call Transport::WritePackets.
         */

        void SendPackets();

        /**
            Receive packets sent from the server.

            This function receives and processes packets from the receive queue of the Transport. To minimize the latency of packet processing, make sure you call Transport::ReadPackets shortly before calling this function.

            @see ProcessPackets
         */

        void ReceivePackets();

        /**
            Check for timeouts.

            Compares the last time a packet was received from the server vs. the current time.

            If no packet has been received within the timeout period, the client is disconnected and set to an error state. See ClientState for details.

            @see Client::AdvanceTime
            @see ClientServerConfig::connectionTimeOut
            @see ClientServerConfig::connectionNegotiationTimeOut
         */

        void CheckForTimeOut();

        /**
            Advance client time.

            Call this at the end of each frame to advance the client time forward. 

            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        void AdvanceTime( double time );

        /**
            Is the client connecting to a server?

            This is true while the client is negotiation connection with a server. This is the period after a call to Client::Connect or Client::InsecureConnect, but before the client establishes a connection, or goes into an error state because it couldn't connect.

            @returns true if the client is currently connecting to, but is not yet connected to a server.
         */

        bool IsConnecting() const;

        /**
            Is the client connected to a server?

            This is true once a client successfully finishes connection negotiation, and connects to a server. It is false while connecting to a server.

            Corresponds to the client being in CLIENT_STATE_CONNECTED.

            @returns true if the client is connected to a server.
         */

        bool IsConnected() const;

        /**
            Is the client in a disconnected state?

            A disconnected state is CLIENT_STATE_DISCONNECTED, or any of the negative client error state values. Effectively, true if client state is <= 0.

            @returns true if the client is in a disconnected state.
         */

        bool IsDisconnected() const;

        /**
            Is the client in an error state?

            When the client disconnects because of a client-side error, it disconnects and sets one of the negative client state values. Effectively, true if client state < 0.

            @returns true if the client disconnected to an error state.
         */

        bool ConnectionFailed() const;

        /**
            Get the current client state.

            The client state machine is used to negotiate connection with the server, and handle error states. Each state corresponds to an entry in the ClientState enum.

            @see yojimbo::GetClientStateName
         */

        ClientState GetClientState() const;

        /**
            Gets the current client time.

            @see Client::AdvanceTime
         */

        double GetTime() const;

        /**
            Get the client id (globally unique).

            This corresponds to the client id parameter passed into the last call to Client::Connect or Client::InsecureConnect

            @returns The globally unique client id for this client.
         */

        uint64_t GetClientId() const;

        /**
            Get the client index.

            The client index is the slot number that the client is occupying on the server. 

            @returns The client index in [0,maxClients-1], where maxClients is the number of client slots allocated on the server in Server::Start.
         */

        int GetClientIndex() const;

        /**
            Get a counter value.

            Counters are used to track event and actions performed by the client. They are useful for debugging, testing and telemetry.

            @returns The counter value. See yojimbo::ClientCounters for the set of client counters.
         */

        uint64_t GetCounter( int index ) const;

        /**
            Create a message of the specified type.

            The message created by this function is typically passed to Client::SendMsg. In this case, the send message function takes ownership of the message pointer and will release it for you.

            If you are using the message in some other way, you are responsible for manually releasing it via Client::ReleaseMsg.

            @param type The message type. The set of message types depends on the message factory set on the client.

            @returns A pointer to the message created, or NULL if no message could be created.

            @see MessageFactory
         */

        Message * CreateMsg( int type );

        /** 
            Check if there is room in the channel send queue to send one message.

            This function is useful in soak tests and unit tests where I want to send messages as quickly as possible, but don't want to overflow the send queue.

            You don't need to call this function manually each time you call Client::SendMsg. It's already asserted on in debug build and in release if it returns false it sets a runtime error that disconnects the client.

            @param channelId The id of the channel in [0,numChannels-1].
            
            @returns True if the channel has room for one more message to be added to its send queue. False otherwise.
         */

        bool CanSendMsg( int channelId = 0 );

        /**
            Queue a message to be sent to the server.

            Adds a message to the send queue of the specified channel. 

            The reliability and ordering guarantees of how the message will be received on the other side are determined by the configuration of the channel.
    
            IMPORTANT: This function takes ownership of the message and ensures that the message is released when it finished being sent. This lets you create a message with Client::CreateMsg and pass it directly into this function. You don't need to manually release the message.

            @param message The message to be sent. It must be allocated from the message factory set on this client.
            @param channelId The id of the channel to send the message across in [0,numChannels-1].

            @see ChannelConfig
            @see ClientServerConfig
         */

        void SendMsg( Message * message, int channelId = 0 );

        /** 
            Poll this method to receive messages from the server.

            Typical usage is to iterate across the set of channels and poll this to receive messages until it returns NULL.

            IMPORTANT: The message returned by this function has one reference. You are responsible for releasing this message via Client::ReleaseMsg.

            @param channelId The id of the channel to try to receive a message from.

            @returns A pointer to the received message, NULL if there are no messages to receive.
         */

        Message * ReceiveMsg( int channelId = 0 );

        /**
            Release a message returned by Client::ReceiveMsg.

            This is a convenience function. It is equivalent to calling MessageFactory::Release on the message factory set on this client (see Client::GetMsgFactory).

            @param message The message to release. Must be non-NULL.

            @see Client::ReceiveMsg
         */

        void ReleaseMsg( Message * message );

        /**
            Get the message factory used by the client.

            The message factory determines the set of messages exchanged between the client and server.

            @returns The message factory.

            @see YOJIMBO_CLIENT_MESSAGE_FACTORY
         */

        MessageFactory & GetMsgFactory();

        /**
            Get the allocator used for client allocations.

            This memory is used for packet, message allocations and stream allocations while the client is connecting/connected to the server.

            The amount of memory backing this allocator is specified by ClientServerConfig::clientMemory.

            @returns The allocator client allocator.
         */

        Allocator & GetClientAllocator();

    protected:

        virtual void InitializeConnection( uint64_t clientId );

        virtual void ShutdownConnection();

        virtual void SetEncryptedPacketTypes();

        virtual PacketFactory * CreatePacketFactory( Allocator & allocator );

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator );

        virtual void SetClientState( ClientState clientState );

        virtual void ResetConnectionData( ClientState clientState = CLIENT_STATE_DISCONNECTED );

        virtual void ResetBeforeNextConnect();

        virtual bool ConnectToNextServer();

#if !YOJIMBO_SECURE_MODE
        virtual void InternalInsecureConnect( const Address & serverAddress );
#endif // #if !YOJIMBO_SECURE_MODE

        virtual void InternalSecureConnect( const Address & serverAddress );

        virtual void SendPacketToServer( Packet * packet );

    private:

        void SendPacketToServer_Internal( Packet * packet, bool immediate = false );

    protected:

        void ProcessConnectionDenied( const ConnectionDeniedPacket & packet, const Address & address );

        void ProcessChallenge( const ChallengePacket & packet, const Address & address );

        void ProcessKeepAlive( const KeepAlivePacket & packet, const Address & address );

        void ProcessDisconnect( const DisconnectPacket & packet, const Address & address );

        void ProcessConnectionPacket( ConnectionPacket & packet, const Address & address );

        void ProcessPacket( Packet * packet, const Address & address, uint64_t sequence );

        bool IsPendingConnect();

        void CompletePendingConnect( int clientIndex );

    protected:

        void Defaults();

        virtual void CreateAllocators();

        virtual void DestroyAllocators();

        virtual Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes );

        ClientServerConfig m_config;                                        ///< The client/server configuration.

        Allocator * m_allocator;                                            ///< The allocator passed in to the client on creation.

        void * m_userContext;                                               ///< The user context. Lets the user pass information to packet serialize functions.

        uint8_t * m_clientMemory;                                           ///< Memory backing the client allocator. Allocated from m_allocator.

        Allocator * m_clientAllocator;                                      ///< The client allocator. Everything allocated between Client::Connect and Client::Disconnect is allocated and freed via this allocator.

        PacketFactory * m_packetFactory;                                    ///< Packet factory for creating and destroying messages. Created via Client::CreatePacketFactory.

        MessageFactory * m_messageFactory;                                  ///< Message factory for creating and destroying messages. Created via Client::CreateMessageFactory. Optional.

        ReplayProtection * m_replayProtection;                              ///< Replay protection discards old and duplicate packets. Protects against packet replay attacks.

        bool m_allocateConnection;                                          ///< True if we should allocate a connection object. This is true when ClientServerConfig::enableMessages is true.

        Connection * m_connection;                                          ///< The connection object for exchanging messages with the server. Optional.

        uint64_t m_clientId;                                                ///< The globally unique client id (set on each call to connect).

        int m_clientIndex;                                                  ///< The client slot index on the server [0,maxClients-1]. -1 if not connected.

        TransportContext m_transportContext;                                ///< Transport context for reading and writing packets.

        ConnectionContext m_connectionContext;                              ///< Connection context for serializing connection packets.

        ClientState m_clientState;                                          ///< The current client state.

        uint64_t m_connectTokenExpireTimestamp;                             ///< Expire timestamp for connect token used in secure connect. This is used as the additional data in the connect token AEAD, so we can quickly reject stale connect tokens without decrypting them.

        int m_serverAddressIndex;                                           ///< Current index in the server address array. This is the server we are currently connecting to.

        int m_numServerAddresses;                                           ///< Number of server addresses in the array.

        Address m_serverAddresses[MaxServersPerConnect];                    ///< List of server addresses we are connecting to.

        Address m_serverAddress;                                            ///< The current server address we are connecting/connected to.

        double m_lastPacketSendTime;                                        ///< The last time we sent a packet to the server.

        double m_lastPacketReceiveTime;                                     ///< The last time we received a packet from the server.

        Transport * m_transport;                                            ///< The transport for sending and receiving packets.

        bool m_shouldDisconnect;                                            ///< Set to true when we receive a disconnect packet from the server so we can defer disconnection until the update.

        ClientState m_shouldDisconnectState;                                ///< The client state to transition to on disconnect via m_shouldDisconnect

        double m_time;                                                      ///< The current client time. See Client::AdvanceTime

#if !YOJIMBO_SECURE_MODE
        uint64_t m_clientSalt;                                              ///< The client salt used for insecure connect. This distinguishes the current insecure client session from a previous one from the same IP:port combination, making insecure reconnects much more robust.
#endif // #if !YOJIMBO_SECURE_MODE

        uint64_t m_sequence;                                                ///< Sequence # for packets sent from client to server.

        uint64_t m_counters[NUM_CLIENT_COUNTERS];                           ///< Counters to aid with debugging and telemetry.

        uint8_t m_connectTokenData[ConnectTokenBytes];                      ///< Encrypted connect token data for the connection request packet.

        uint8_t m_connectTokenNonce[NonceBytes];                            ///< Nonce to send to server so it can decrypt the connect token.

        uint8_t m_challengeTokenData[ChallengeTokenBytes];                  ///< Encrypted challenge token data for challenge response packet

        uint8_t m_challengeTokenNonce[NonceBytes];                          ///< Nonce to send to server so it can decrypt the challenge token.

        uint8_t m_clientToServerKey[KeyBytes];                              ///< Client to server packet encryption key.

        uint8_t m_serverToClientKey[KeyBytes];                              ///< Server to client packet encryption key.

    private:

        Client( const Client & other );
        
        const Client & operator = ( const Client & other );
    };
}

/** 
    Helper macro to set the client allocator class.

    You can use this macro to specify that the client uses your own custom allocator class. The default allocator to use is TLSF_Allocator. 

    The constructor of your derived allocator class must match the signature of the TLSF_Allocator constructor to work with this macro.
    
    See tests/shared.h for an example of usage.
 */

#define YOJIMBO_CLIENT_ALLOCATOR( allocator_class )                                                     \
    yojimbo::Allocator * CreateAllocator( yojimbo::Allocator & allocator, void * memory, size_t bytes ) \
    {                                                                                                   \
        return YOJIMBO_NEW( allocator, allocator_class, memory, bytes );                                \
    }

/** 
    Helper macro to set the client message factory class.
    
    See tests/shared.h for an example of usage.
 */

#define YOJIMBO_CLIENT_MESSAGE_FACTORY( message_factory_class )                                         \
    yojimbo::MessageFactory * CreateMessageFactory( yojimbo::Allocator & allocator )                    \
    {                                                                                                   \
        return YOJIMBO_NEW( allocator, message_factory_class, allocator );                              \
    }

#endif

#endif // #ifndef YOJIMBO_CLIENT_H
