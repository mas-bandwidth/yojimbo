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

#ifndef YOJIMBO_SERVER_H
#define YOJIMBO_SERVER_H

#include "yojimbo_config.h"
#include "yojimbo_packet.h"
#include "yojimbo_allocator.h"
#include "yojimbo_transport.h"
#include "yojimbo_encryption.h"
#include "yojimbo_connection.h"
#include "yojimbo_packet_processor.h"
#include "yojimbo_client_server_packets.h"
#include "yojimbo_tokens.h"

/** @file */

namespace yojimbo
{
    /// Server resource type. Resources are either global, or per-client. Resources include things like packet factories, message factories, and allocators.

    enum ServerResourceType
    {
        SERVER_RESOURCE_GLOBAL,                                     ///< Resource is global for the server. This is used for resources that are used by clients that are negotiating connection.
        SERVER_RESOURCE_PER_CLIENT                                  ///< Resource is for a specific client slot. This is used for resources that belong to a connected client. The idea is that by giving clients their own resources, a client cannot launch an attack to exhaust server resources that affect other clients.
    };

    /// Per-client error state. These error types are used to identify the reason why a client was disconnected on the server when something goes wrong.

    enum ServerClientError
    {
        SERVER_CLIENT_ERROR_TIMEOUT,                                ///< The client timed out on the server
        SERVER_CLIENT_ERROR_ALLOCATOR,                              ///< The allocator is in error state for this client
        SERVER_CLIENT_ERROR_CONNECTION,                             ///< The connection is in error state for this client
        SERVER_CLIENT_ERROR_MESSAGE_FACTORY,                        ///< The message factory is in error state for this client
        SERVER_CLIENT_ERROR_PACKET_FACTORY,                         ///< The packet factory is in error state for this client
    };

    /**
        Per-client data stored on the server. 

        Stores data for connected clients such as their address, globally unique client id, last packet send and receive times used for timeouts and keep-alive packets and so on.
     */

    struct ServerClientData
    {
        Address address;                                            ///< The address of this client. Packets are sent and received from the client using this address, therefore only one client with the address may be connected at any time.
        uint64_t clientId;                                          ///< Globally unique client id. Only one client with a specific client id may be connected to the server at any time.
        double connectTime;                                         ///< The time that the client connected to the server. Used to determine how long the client has been connected.
        double lastPacketSendTime;                                  ///< The last time a packet was sent to this client. Used to determine when it's necessary to send keep-alive packets.
        double lastPacketReceiveTime;                               ///< The last time a packet was received from this client. Used for timeouts.
        bool fullyConnected;                                        ///< True if this client is 'fully connected'. Fully connected means the client has received a keep-alive packet from the server containing its client index and replied back to the server with a keep-alive packet confirming that it knows its client index.
#if !YOJIMBO_SECURE_MODE
        uint64_t clientSalt;                                        ///< The client salt is a random number rolled on each insecure client connect. It is used to distinguish one client connect session from another, so reconnects are more reliable. See Client::InsecureConnect for details.
        bool insecure;                                              ///< True if this client connected in insecure mode. This means the client connected via Client::InsecureConnect and is sending and receiving packets without encryption. Please use insecure mode only during development, it is not suitable for production use.
#endif // #if !YOJIMBO_SECURE_MODE

        ServerClientData()
        {
            clientId = 0;
            connectTime = 0.0;
            lastPacketSendTime = 0.0;
            lastPacketReceiveTime = 0.0;
            fullyConnected = false;
#if !YOJIMBO_SECURE_MODE
            clientSalt = 0;
            insecure = false;
#endif // #if !YOJIMBO_SECURE_MODE
        }
    };

    /**
        Used by the server to remember and reject connect tokens that have already been used.

        This protects against attacks where the same connect token is used to connect multiple clients to the server in a short period of time.
     */

    struct ConnectTokenEntry
    {
        double time;                                                ///< The time for this entry. Used to replace the oldest entries once the connect token array fills up.
        Address address;                                            ///< Address of the client that sent this connect token. Binds a connect token to a particular address so it can't be exploited.
        uint8_t mac[MacBytes];                                      ///< HMAC of connect token. We use this to avoid replay attacks where the same token is sent repeatedly for different addresses.

        ConnectTokenEntry()
        {
            time = -1000.0;
            memset( mac, 0, MacBytes );
        }
    };

    /**
        Server counters provide insight into the number of times an action was performed by the server.

        They are intended for use in a telemetry system, eg. the server would report these counters to some backend logging system to track behavior in a production environment.

        They're also pretty useful for debugging and seeing what happened after the fact, and functional testing because counters let tests verify that expected codepaths were hit.
     */

    enum ServerCounters
    {
        SERVER_COUNTER_CONNECTION_REQUEST_PACKETS_RECEIVED,                                     ///< Number of connection request packets received by the server
        SERVER_COUNTER_CONNECTION_REQUEST_CHALLENGE_PACKETS_SENT,                               ///< Number of challenge packets sent from the server to clients requesting connection
        SERVER_COUNTER_CONNECTION_REQUEST_DENIED_SERVER_IS_FULL,                                ///< Number of times the server denied a connection request because the server was full (all client slots occupied)
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_BECAUSE_FLAG_IS_SET,                          ///< Number of times the server ignored a connection request because the SERVER_FLAG_IGNORE_CONNECTION_REQUESTS flag is set.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_DECRYPT_CONNECT_TOKEN,              ///< Number of times the server ignored a connection request because the connect token was invalid and failed to decrypt.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_SERVER_ADDRESS_NOT_IN_WHITELIST,              ///< Number of times the server ignored a connection request because the server address (see Server::SetServerAddress) is not in the whitelist of server addresses in the connect token.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_PROTOCOL_ID_MISMATCH,                         ///< Number of times the server ignored a connection request because the protocol id in the connect token doesn't match the protocol id set on the transport.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_IS_ZERO,                            ///< Number of times the server ignored a connection request because the client id is zero. Please contact me if this is a problem for your game/application.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_ADDRESS_ALREADY_CONNECTED,                    ///< Number of times the server ignored a connection request because a client with that address is already connected to the server.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED,                  ///< Number of times the server ignored a connection request because a client with that client id was already connected to the server.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED,                        ///< Number of times the server ignored a connection request because the connect token has expired (they're typically only valid for a short amount of time, like 45-60 seconds, to fight replay attacks).
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED,                   ///< Number of times the server ignored a connection request because a client has already used that connect token to connect to this server. This should typically be zero, a non-zero value indicates shennanigans!
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ADD_ENCRYPTION_MAPPING,             ///< Number of times the server ignored a connection request because it could not add an encryption mapping for that client. If this is non-zero, it would indicate that somebody is somehow attacking the server with valid connection tokens. See yojimbo::MaxEncryptionMappings.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ALLOCATE_CHALLENGE_PACKET,          ///< Number of times the server ignored a connection request because it could not allocate a challenge packet to send back to the client. This would indicate that the server has insufficient global resources (packet factory, allocator) to handle the connection negotiation load. @see ClientServerConfig::serverGlobalMemory.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_GENERATE_CHALLENGE_TOKEN,           ///< Number of times the server ignored a connection request because it could not generate a challenge token to send back to the client. Something is probably wrong with libsodium.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ENCRYPT_CHALLENGE_TOKEN,            ///< Number of times the server ignored a connection request because it could not encrypt a challenge token to send back to the client. Something is probably wrong with libsodium.

        SERVER_COUNTER_CHALLENGE_RESPONSE_PACKETS_RECEIVED,                                     ///< Number of challenge response packets received by the server.
        SERVER_COUNTER_CHALLENGE_RESPONSE_ACCEPTED,                                             ///< Number of times the server accepted a challenge response and transitioned that client to connected.
        SERVER_COUNTER_CHALLENGE_RESPONSE_DENIED_SERVER_IS_FULL,                                ///< Number of times the server denied a challenge response because the server is full. This happens when multiple clients are racing to connect to the same client slot.
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_BECAUSE_FLAG_IS_SET,                          ///< Number of times the server ignored a challenge response packet sent from a client because the SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES flag is set.
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_ADDRESS_ALREADY_CONNECTED,                    ///< Number of times the server ignored a challenge response packet because a client with that address is already connected.
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_CLIENT_ID_ALREADY_CONNECTED,                  ///< Number of times the server ignored a challenge response because a client with that client id is already connected. 
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_FAILED_TO_DECRYPT_CHALLENGE_TOKEN,            ///< Number of times the server ignored a challenge response because it couldn't decrypt the challenge token.
        
        SERVER_COUNTER_CLIENT_CONNECTS,                                                         ///< Number of times a client has connected to the server.
        SERVER_COUNTER_CLIENT_DISCONNECTS,                                                      ///< Number of times a client has been disconnected from the server.
        SERVER_COUNTER_CLIENT_CLEAN_DISCONNECTS,                                                ///< Number of clean disconnects where the client sent disconnect packets to the server. You want lots of these.
        SERVER_COUNTER_CLIENT_TIMEOUTS,                                                         ///< Number of timeouts where the client disconnected without sending disconnect packets to the server. You want few of these.
        SERVER_COUNTER_CLIENT_ALLOCATOR_ERRORS,                                                 ///< Number of times a client was disconnected from the server because their allocator entered into an error state (eg. failed to allocate a block of memory). This indicates that the client has exhausted their per-client resources. See yojimbo::serverPerClientMemory.
        SERVER_COUNTER_CLIENT_CONNECTION_ERRORS,                                                ///< Number of times a client was disconnected from the server because their connection entered into an error state. This indicates that something went wrong with the internal protocol for sending messages between client and server. Common situations include, sending too many messages and overflowing the message send queue in a channel. See Connection::CanSendMsg.
        SERVER_COUNTER_CLIENT_MESSAGE_FACTORY_ERRORS,                                           ///< Number of times a client was disconnected from the server because their packet factory went into an error state. This indicates that the client tried to create a packet but failed to do so.
        SERVER_COUNTER_CLIENT_PACKET_FACTORY_ERRORS,                                            ///< Number of times a client was disconnected from the server because their message factory went into an error state. This indicates that the client tried to create a message but failed to do so.
        SERVER_COUNTER_GLOBAL_PACKET_FACTORY_ERRORS,                                            ///< Number of times the global packet factory entered into an error state because it could not allocate a packet. This probably indicates insufficient global memory for the connection negotiation process on the server. See ClientServerConfig::serverGlobalMemory.
        SERVER_COUNTER_GLOBAL_ALLOCATOR_ERRORS,                                                 ///< Number of times the global allocator went into error state because it could not perform an allocation. This probably indicates insufficient global memory for the connection negotiation process on the server. See ClientServerConfig::serverGlobalMemory.
        
        NUM_SERVER_COUNTERS                                                                     ///< The number of server counters.
    };

    /**
        The action taken by the server in response to a connection request packet.

        @see Server::OnConnectionRequest
     */

    enum ServerConnectionRequestAction
    {
        SERVER_CONNECTION_REQUEST_CHALLENGE_PACKET_SENT,                                        ///< The server replied with a challenge packet.
        SERVER_CONNECTION_REQUEST_DENIED_SERVER_IS_FULL,                                        ///< The server denied the connection request because the server is full.
        SERVER_CONNECTION_REQUEST_IGNORED_BECAUSE_FLAG_IS_SET,                                  ///< The server ignored the connection request because the SERVER_FLAG_IGNORE_CONNECTION_REQUESTS is set.
        SERVER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED,                                ///< The server ignored the connection request because the connect token has expired.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_DECRYPT_CONNECT_TOKEN,                      ///< The server ignored the connection request because it could not decrypt the connect token.
        SERVER_CONNECTION_REQUEST_IGNORED_SERVER_ADDRESS_NOT_IN_WHITELIST,                      ///< The server ignored the connection request because the server address is not in the connect token server address whitelist. See Server::SetServerAddress.
        SERVER_CONNECTION_REQUEST_IGNORED_PROTOCOL_ID_MISMATCH,                                 ///< The server ignored the connection request because the connect token protocol id does not match the protocol id set on the transport.
        SERVER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_IS_ZERO,                                    ///< The server ignored the connection request because the client id is zero.
        SERVER_CONNECTION_REQUEST_IGNORED_ADDRESS_ALREADY_CONNECTED,                            ///< The server ignored the connection request because a client with that address is already connected.
        SERVER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED,                          ///< The server ignored the connection request because a client with that client id is already connected.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ADD_ENCRYPTION_MAPPING,                     ///< The server ignored the connection request because it could not add an encryption mapping for that client. This is bad.
        SERVER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED,                           ///< The server ignored the connection request because another client has already used that connect token to connect to this server. This is bad.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_GENERATE_CHALLENGE_TOKEN,                   ///< The server ignored the connection request because it couldn't generate a challenge token. This is bad.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ALLOCATE_CHALLENGE_PACKET,                  ///< The server ignored the connection request because it couldn't allocate a challenge packet to send back to the client. This is bad.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ENCRYPT_CHALLENGE_TOKEN                     ///< The server ignored the connection request because it couldn't encrypt the challenge token to send back to the client. This is bad.
    };

    /**
        The action taken by the server in response to a challenge response packet.

        @see Server::OnChallengeResponse
     */

    enum ServerChallengeResponseAction
    {
        SERVER_CHALLENGE_RESPONSE_ACCEPTED,                                                     ///< The server accepted the challenge response and transitioned the client to connected.
        SERVER_CHALLENGE_RESPONSE_DENIED_SERVER_IS_FULL,                                        ///< The server denied the challenge response because the server is full.
        SERVER_CHALLENGE_RESPONSE_IGNORED_BECAUSE_FLAG_IS_SET,                                  ///< The server ignored the challenge response because the SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES flag is set.
        SERVER_CHALLENGE_RESPONSE_IGNORED_FAILED_TO_DECRYPT_CHALLENGE_TOKEN,                    ///< The server ignored the challenge response because it could not decrypt the challenge token.
        SERVER_CHALLENGE_RESPONSE_IGNORED_ADDRESS_ALREADY_CONNECTED,                            ///< The server ignored the challenge response because a client with that address is already connected.
        SERVER_CHALLENGE_RESPONSE_IGNORED_CLIENT_ID_ALREADY_CONNECTED,                          ///< The server ignored the challenge response because a client with that client id is already connected.
    };

    /**
        Server flags are used to enable and disable server features.                            
     */

    enum ServerFlags
    {
        SERVER_FLAG_IGNORE_CONNECTION_REQUESTS = (1<<0),                                        ///< When this flag is set the server ignores all connection requests.
        SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES = (1<<1),                                        ///< When this flag is set the server ignores all challenge response.
#if !YOJIMBO_SECURE_MODE
        SERVER_FLAG_ALLOW_INSECURE_CONNECT = (1<<2)                                             ///< When this flag is set the server allows insecure connects via Client::InsecureConnect. Please don't use this in production environments!
#endif // #if !YOJIMBO_SECURE_MODE
    };

    /** 
        A server with n slots for clients to connect to.

        This class is designed to be inherited from to create your own server class.
     */

    class Server : public ConnectionListener
    {
    public:

        /**
            The server constructor.

            @param allocator The allocator for all memory used by the server.
            @param transport The transport for sending and receiving packets.
            @param config The client/server configuration.
            @param time The current time in seconds. See Server::AdvanceTime
         */

        Server( Allocator & allocator, Transport & transport, const ClientServerConfig & config, double time );

        /**
            The server destructor.

            IMPORTANT: Please call Server::Stop before destroying the server. This is necessary because Stop is virtual and calling virtual methods from destructors does not give the expected behavior when you override that method.
         */

        virtual ~Server();

        /**
            Set the private key used to decrypt connect tokens.
            
            The private key must be known only to the dedicated server instance and the matchmaker backend that generates connect tokens. 

            @param privateKey The private key of size yojimbo::KeyBytes.
         */

        void SetPrivateKey( const uint8_t * privateKey );

        /**
            Set the user context.

            The user context is set on the stream when packets and read and written. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets.

            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time. 

            If you do use a user context, please make sure the data that contributes to the user context is checksummed and included in the protocol id, so clients and servers with incompatible data can't connect to each other.

            @see Stream::GetUserContext
         */

        void SetUserContext( void * context );

        /**      
            Set the server IP address. 

            This should be a public IP address, eg. the address that a client would use to connect to the server.
             
            @param address The server address.
         */

        void SetServerAddress( const Address & address );

        /**
            Start the server and allocate client slots.
            
            Each client that connects to this server occupies one of the client slots allocated by this function.

            @param maxClients The number of client slots to allocate. Must be in range [1,MaxClients]

            @see Server::Stop
         */

        void Start( int maxClients = MaxClients );

        /**
            Stop the server and free client slots.
        
            Any clients that are connected at the time you call stop will be disconnected.
            
            When the server is stopped, clients cannot connect to the server.

            @see Server::Start.
         */

        void Stop();

        /**
            Disconnect the client at the specified client index.
            
            IMPORTANT: This function will assert if you attempt to disconnect a client that is not connected.
         
            @param clientIndex The index of the client to disconnect in range [0,maxClients-1], where maxClients is the number of client slots allocated in Server::Start.
            @param sendDisconnectPacket If true, disconnect packets are sent to the other side of the connection so it sees the disconnect as quickly as possible (rather than timing out).

            @see Server::IsClientConnected
         */

        void DisconnectClient( int clientIndex, bool sendDisconnectPacket = true );

        /**
            Disconnect all clients from the server.
            
            Client slots remain allocated as per the last call to Server::Start, they are simply made available for new clients to connect.
            
            @param sendDisconnectPacket If true, disconnect packets are sent to the other side of the connection so it sees the disconnect as quickly as possible (rather than timing out).
         */

        void DisconnectAllClients( bool sendDisconnectPacket = true );

        /**
            Send packets to connected clients.
         
            This function drives the sending of packets to clients such as keep-alives and connection packets to transmit messages from server to client.

            Packets sent from this function are queued for sending on the transport, and are not actually serialized and sent to the network until you call Transport::WritePackets.
         */

        void SendPackets();

        /**
            Receive packets sent from potential and connected clients.

            This function receives and processes packets from the receive queue of the Transport. To minimize the latency of packet processing, make sure you call Transport::ReadPackets shortly before calling this function.

            @see ProcessPackets
         */

        void ReceivePackets();

        /**
            Check for timeouts.

            Walks across the set of connected clients and compares the last time a packet was received from that client vs. the current time.

            If no packet has been received within the timeout period, the client is disconnected and its client slot is made available for other clients to connect to.

            @see Server::AdvanceTime
            @see ClientServerConfig::connectionTimeOut
            @see ClientServerConfig::connectionNegotiationTimeOut
         */

        void CheckForTimeOut();

        /**
            Advance server time.

            Call this at the end of each frame to advance the server time forward. 

            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        void AdvanceTime( double time );

        /**
            Is the server running?

            The server is running after you have called Server::Start. It is not running before the first server start, and after you call Server::Stop.

            Clients can only connect to the server while it is running.

            @returns true if the server is currently running.
         */

        bool IsRunning() const;

        /**
            Get the maximum number of clients that can connect to the server.

            Corresponds to the maxClients parameter passed into the last call to Server::Start.

            @returns The maximum number of clients that can connect to the server. In other words, the number of client slots.
         */

        int GetMaxClients() const;

        /**
            Is a client connected to a client slot?

            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.

            @returns True if the client is connected.
         */

        bool IsClientConnected( int clientIndex ) const;

        /** 
            Get the number of clients that are currently connected to the server.

            @returns the number of connected clients.
         */

        int GetNumConnectedClients() const;

        /**
            Find the client index for the client with the specified client id.

            @returns The client index if a client with the client id is connected to the server, otherwise -1.
         */

        int FindClientIndex( uint64_t clientId ) const;

        /**
            Find the client index for the client with the specified address.

            @returns The client index if a client with the address is connected to the server, otherwise -1.
         */

        int FindClientIndex( const Address & address ) const;

        /**
            Get the client id for the client at the specified client index.

            @returns The client id of the client.
         */

        uint64_t GetClientId( int clientIndex ) const;

        /**
            Get the address of the client at the specified client index.

            @returns The address of the client.
         */

        const Address & GetClientAddress( int clientIndex ) const;

        /**
            Get the server address.

            This is the public address that clients connect to.

            @returns The server address.

            @see Server::SetServerAddress
         */

        const Address & GetServerAddress() const;

        /**
            Set server flags.

            Flags are used to enable and disable server functionality.

            @param flags The server flags to set. See yojimbo::ServerFlags for the set of server flags that can be passed in.

            @see Server::GetFlags
         */

        void SetFlags( uint64_t flags );

        /**
            Get the current server flags.

            @returns The server flags. See yojimbo::ServerFlags for the set of server flags.
    
            @see Server::SetFlags
         */

        uint64_t GetFlags() const;

        /**
            Get a counter value.

            Counters are used to track event and actions performed by the server. They are useful for debugging, testing and telemetry.

            @returns The counter value. See yojimbo::ServerCounters for the set of server counters.
         */

        uint64_t GetCounter( int index ) const;

        /** 
            Reset all counters to zero.

            This is typically used with a telemetry application after uploading the current set of counters to the telemetry backend. 

            This way you can continue to accumulate events, and upload them at some frequency, like every 5 minutes to the telemetry backend, without double counting events.
         */

        void ResetCounters();

        /**
            Gets the current server time.

            @see Server::AdvanceTime
         */

        double GetTime() const;

        /**
            Create a message of the specified type.

            The message created by this function is typically passed to Server::SendMsg. In this case, the send message function takes ownership of the message pointer and will release it for you.

            If you are using the message in some other way, you are responsible for manually releasing it via Server::ReleaseMsg.

            @param clientIndex The index of the client the message will be sent to. This is necessary because each client has their own message factory and allocator.
            @param type The message type. The set of message types depends on the message factory set on the client.

            @returns A pointer to the message created, or NULL if no message could be created.

            @see MessageFactory
         */

        Message * CreateMsg( int clientIndex, int type );

        /** 
            Check if there is room in the channel send queue to send one message to a client.

            This function is useful in soak tests and unit tests where I want to send messages as quickly as possible, but don't want to overflow the send queue.

            You don't need to call this function manually each time you call Server::SendMsg. It's already asserted on in debug build and in release if it returns false it sets a runtime error that disconnects the client from the server.

            @param clientIndex The index of the client the message will be sent to.
            @param channelId The id of the channel in [0,numChannels-1].
            
            @returns True if the channel has room for one more message to be added to its send queue. False otherwise.
         */

        bool CanSendMsg( int clientIndex, int channelId = 0 ) const;

        /**
            Queue a message to be sent to a server.

            Adds a message to the send queue of the specified channel of the client connection.

            The reliability and ordering guarantees of how the message will be received on the other side are determined by the configuration of the channel.
    
            IMPORTANT: This function takes ownership of the message and ensures that the message is released when it finished being sent. This lets you create a message with Server::CreateMsg and pass it directly into this function. You don't need to manually release the message.

            @param clientIndex The index of the client the message will be sent to.
            @param message The message to be sent. It must be allocated from the message factory set on this client.
            @param channelId The id of the channel to send the message across in [0,numChannels-1].

            @see ChannelConfig
            @see ClientServerConfig
         */

        void SendMsg( int clientIndex, Message * message, int channelId = 0 );

        /** 
            Poll this method to receive messages sent from a client.

            Typical usage is to iterate across the set of clients, iterate across the set of the channels, and poll this to receive messages until it returns NULL.

            IMPORTANT: The message returned by this function has one reference. You are responsible for releasing this message via Server::ReleaseMsg.

            @param clientIndex The index of the client that we want to receive messages from.
            @param channelId The id of the channel to try to receive a message from.

            @returns A pointer to the received message, NULL if there are no messages to receive.
         */

        Message * ReceiveMsg( int clientIndex, int channelId = 0 );

        /**
            Release a message returned by Server::ReceiveMsg.

            This is a convenience function. It is equivalent to calling MessageFactory::Release on the message factory set on this client (see Server::GetMsgFactory).

            @param clientIndex The index of the client. This is necessary because each client has their own message factory, and messages must be released with the same message factory they were created with.
            @param message The message to release. Must be non-NULL.

            @see Server::ReceiveMsg
         */

        void ReleaseMsg( int clientIndex, Message * message );

        /**
            Get the message factory instance belonging to a particular client.

            The message factory determines the set of messages exchanged between the client and server.

            @param clientIndex The index of the client. Each client has their own message factory.

            @returns The message factory for the client.

            @see YOJIMBO_SERVER_MESSAGE_FACTORY
         */

        MessageFactory & GetMsgFactory( int clientIndex );

        /**
            Get the allocator used for global allocations.

            Global allocations are allocations that aren't tied to any particular client. 

            Typically, this means data structures that correspond to connection negotiation, processing connection requests and so on.

            The amount of memory backing this allocator is specified by ClientServerConfig::serverGlobalMemory.

            @returns The global allocator.
         */

        Allocator & GetGlobalAllocator() { assert( m_globalAllocator ); return *m_globalAllocator; }

        /**
            Get the allocator used for per-client.

            Per-client allocations are allocations that are tied to a particular client index. The idea is to silo each client on the server to their own set of resources, making it impossible for a client to exhaust resources shared with other clients connected to the server.

            The amount of memory backing this allocator is specified by ClientServerConfig::perClientMemory. There is one allocator per-client slot allocated in Server::Start. These allocators are undefined outside of Start/Stop and will assert in that case.

            @param clientIndex The index of the client.

            @returns The per-client allocator corresponding to the client index.
         */

        Allocator & GetClientAllocator( int clientIndex ) { assert( clientIndex >= 0 ); assert( clientIndex < m_maxClients ); assert( m_clientAllocator[clientIndex] ); return *m_clientAllocator[clientIndex]; }

    protected:

        /**
            Helper function to create a global packet by type.

            Global packets don't belong to any particular client. These are packets send and received as part of the connection negotiation protocol.

            @param type The type of packet to create.

            @returns The packet object that was created. NULL if a packet could be created. You *must* check this. It *will* happen when the packet factory runs out of memory to allocate packets!
         */

        Packet * CreateGlobalPacket( int type );

        /**
            Helper function to create a client packet by type.

            Client packets belong to particular client index. These are packets sent after a client/server connection has been established.

            @param clientIndex Index of the client that the packet will be sent to.
            @param type The type of packet to create.

            @returns The packet object that was created. NULL if a packet could be created. You *must* check this. It *will* happen when the packet factory runs out of memory to allocate packets!
         */

        Packet * CreateClientPacket( int clientIndex, int type );

        /**
            Override this method to get a callback when the server is started.

            @param maxClients The number of client slots are being allocated. eg. maximum number of clients that can connect to the server.
         */

        virtual void OnStart( int maxClients );

        /**
            Override this method to get a callback when the server is stopped.
         */

        virtual void OnStop();

        /**
            Override this method to get a callback when the server processes a connection request packet.

            @param action The action that the server took in response to the connection request packet.
            @param packet The connection request packet being processed.
            @param address The address the packet was sent from.
            @param connectToken The decrypted connect token. Depending on the action, this may or may not be valid. See yojimbo::ServerConnectionRequestAction for details.
         */

        virtual void OnConnectionRequest( ServerConnectionRequestAction action, const ConnectionRequestPacket & packet, const Address & address, const ConnectToken & connectToken );

        /**
            Override this method to get a callback when the server processes a challenge response packet.

            @param action The action that the server took in response to the challenge response packet.
            @param packet The challenge response packet being processed.
            @param address The address the packet was sent from.
            @param challengeToken The decrypted challenge token. Depending on the action, this may or may not be valid. See yojimbo::ServerChallengeResponseAction for details.
         */

        virtual void OnChallengeResponse( ServerChallengeResponseAction action, const ChallengeResponsePacket & packet, const Address & address, const ChallengeToken & challengeToken );

        /**
            Override this method to get a callback when a client connects to the server.

            IMPORTANT: The client is fully connected at the point this callback is made, so all data structures are setup for this client, and query functions based around client index will work properly.

            @param clientIndex The index of the slot that the client connected to.
         */

        virtual void OnClientConnect( int clientIndex );

        /**
            Override this method to get a callback when a client disconnects from the server.

            IMPORTANT: The client is (still) fully connected at the point this callback is made, so all data structures are setup for the client, and query functions based around client index will work properly.

            @param clientIndex The client slot index of the disconnecting client.
         */

        virtual void OnClientDisconnect( int clientIndex );

        /**
            Override this method to get a callback when an error occurs that will result in a client being disconnected from the server.

            IMPORTANT: The client is (still) fully connected at the point this callback is made, so all data structures are setup for the client, and query functions based around client index will work properly.

            @param clientIndex The client slot index of the client that is about to error out.
            @param error The error that occurred.
         */

        virtual void OnClientError( int clientIndex, ServerClientError error );

        /**
            Override this method to get a callback when a packet is sent.

            @param packetType The type of packet being sent according to the packet factory that is set on the server. See PacketFactory.
            @param to The address the packet is being sent to.
            @param immediate If true then this packet will be serialized and flushed to the network immediately. This is used for disconnect packets. See Server::DisconnectClient.
         */

        virtual void OnPacketSent( int packetType, const Address & to, bool immediate );

        /**
            Override this method to get a callback when a packet is received.

            @param packetType The type of packet being sent according to the packet factory that is set on the server. See PacketFactory.
            @param from The address the packet was received from.
         */

        virtual void OnPacketReceived( int packetType, const Address & from );

        /**
            Override this method to get a callback when a connection packet is sent to a client.

            Connection packets are carrier packets that transmit messages between the client and server. They are only generated if you enabled messages in the ClientServerConfig (true by default).

            @param connection The connection that the packet belongs to. To get the client index call Connection::GetClientIndex.
            @param sequence The sequence number of the connection packet being sent.

            @see ClientServerConfig::enableMessages
            @see Connection::GetClientIndex
         */

        virtual void OnConnectionPacketGenerated( Connection * connection, uint16_t sequence );

        /**
            Override this method to get a callback when a connection packet is acked by the client (eg. the client notified the server that packet was received).

            Connection packets are carrier packets that transmit messages between the client and server. They are automatically generated when messages are enabled in ClientServerConfig (true by default).

            @param connection The connection that the packet belongs to. To get the client index call Connection::GetClientIndex.
            @param sequence The packet sequence number of the connection packet that was acked by the client.

            @see ClientServerConfig::enableMessages
            @see Connection::GetClientIndex
         */

        virtual void OnConnectionPacketAcked( Connection * connection, uint16_t sequence );

        /**
            Override this method to get a callback when a connection packet is received from a client.

            Connection packets are carrier packets that transmit messages between the client and server. They are automatically generated when messages are enabled in ClientServerConfig (true by default).

            @param connection The connection that the packet belongs to. To get the client index call Connection::GetClientIndex.
            @param sequence The sequence number of the connection packet that was received.

            @see ClientServerConfig::enableMessages
            @see Connection::GetClientIndex
         */

        virtual void OnConnectionPacketReceived( Connection * connection, uint16_t sequence );

        /**
            Override this method to get a callback when a block fragment is received from a client.

            This callback lets you implement a progress bar for large block transmissions.

            @param connection The connection that the block fragment belongs to. To get the client index call Connection::GetClientIndex.
            @param channelId The channel the block is being sent over.
            @param messageId The message id the block is attached to.
            @param fragmentId The fragment id that is being processed. Fragment ids are in the range [0,numFragments-1].
            @param fragmentBytes The size of the fragment in bytes.
            @param numFragmentsReceived The number of fragments received for this block so far (including this one).
            @param numFragmentsInBlock The total number of fragments in this block. The block receive completes when all fragments are received.

            @see Connection::GetClientIndex
            @see BlockMessage::AttachBlock
         */

        virtual void OnConnectionFragmentReceived( Connection * connection, int channelId, uint16_t messageId, uint16_t fragmentId, int fragmentBytes, int numFragmentsReceived, int numFragmentsInBlock );

        /** 
            Override this method to process user packets sent from a client.

            User packets let you extend the yojimbo by adding your own packet types to be exchanged between client and server. See PacketFactory.

            Most users won't need to create custom packet types, and will extend the protocol by defining their own message types instead. See MessageFactory.

            @param clientIndex Identifies which client this packet came from.
            @param packet The user packet received from the client.

            @returns Return true if the user packet was processed successfully. Returning false if the packet could not be processed, or if is of a type you don't expect. This ensures that unexpected packet types don't keep the connection alive when it should time out.
         */

        virtual bool ProcessUserPacket( int clientIndex, Packet * packet );

    protected:

        virtual void SetEncryptedPacketTypes();

        virtual void CreateAllocators();

        virtual void DestroyAllocators(); 

        virtual Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes );

        Allocator & GetAllocator( ServerResourceType type, int clientIndex = 0 );

        virtual PacketFactory * CreatePacketFactory( Allocator & allocator, ServerResourceType type, int clientIndex = 0 );

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator, ServerResourceType type, int clientIndex = 0 );

        virtual void ResetClientState( int clientIndex );

        int FindFreeClientIndex() const;

        bool FindConnectTokenEntry( const uint8_t * mac );
        
        bool FindOrAddConnectTokenEntry( const Address & address, const uint8_t * mac );

        void ConnectClient( int clientIndex, const Address & clientAddress, uint64_t clientId );

        void SendPacket( const Address & address, Packet * packet, bool immediate = false );

        void SendPacketToConnectedClient( int clientIndex, Packet * packet, bool immediate = false );

        void ProcessConnectionRequest( const ConnectionRequestPacket & packet, const Address & address );

        void ProcessChallengeResponse( const ChallengeResponsePacket & packet, const Address & address );

        void ProcessKeepAlive( const KeepAlivePacket & packet, const Address & address );

        void ProcessDisconnect( const DisconnectPacket & packet, const Address & address );

#if !YOJIMBO_SECURE_MODE
        void ProcessInsecureConnect( const InsecureConnectPacket & /*packet*/, const Address & address );
#endif // #if !YOJIMBO_SECURE_MODE

        void ProcessConnectionPacket( ConnectionPacket & packet, const Address & address );

        void ProcessPacket( Packet * packet, const Address & address, uint64_t sequence );

        KeepAlivePacket * CreateKeepAlivePacket( int clientIndex );

        Transport * GetTransport() { return m_transport; }

    private:

        void Defaults();

        ClientServerConfig m_config;                                        ///< The client/server configuration passed in to the constructor.

        Allocator * m_allocator;                                            ///< The allocator passed in to the constructor. All memory used by the server is allocated using this.

        uint8_t * m_globalMemory;                                           ///< The block of memory backing the global allocator. Allocated with m_allocator.

        uint8_t * m_clientMemory[MaxClients];                               ///< The block of memory backing the per-client allocators. Allocated with m_allocator.

        Allocator * m_globalAllocator;                                      ///< The global allocator. This is used for allocations related to connection negotiation.

        Allocator * m_clientAllocator[MaxClients];                          ///< Array of per-client allocator. These are used for allocations related to connected clients.

        Transport * m_transport;                                            ///< Transport interface for sending and receiving packets.

        void * m_userContext;                                               ///< The user context specified by Server::SetUserContext. Provides a way for the user to pass a pointer to data so it's accessible when reading and writing packets.

        TransportContext m_globalTransportContext;                          ///< Global transport context for reading and writing packets. Used for packets that don't belong to a connected client. eg. connection negotiation packets.

        TransportContext m_clientTransportContext[MaxClients];              ///< Array of per-client transport contexts for reading and writing packets that belong to connected clients.

        ConnectionContext m_clientConnectionContext[MaxClients];            ///< Connection context for reading and writing connection packets to connected clients. These packets contain messages sent between the client and server.

        PacketFactory * m_globalPacketFactory;                              ///< Global packet factory for creating global packets such as connection request packets and challenge response packets sent during connection negotiation.

        PacketFactory * m_clientPacketFactory[MaxClients];                  ///< Per-client packet factory for creating and destroying packets sent to and received from connected clients.

        MessageFactory * m_clientMessageFactory[MaxClients];                ///< Per-client message factory for creating and destroying messages. These are only allocated if ClientServerConfig::enableMessages is true.

        ReplayProtection * m_clientReplayProtection[MaxClients];            ///< Per-client protection against packet replay attacks. Discards old and already received packets.

        uint8_t m_privateKey[KeyBytes];                                     ///< Private key used for encrypting and decrypting connect and challenge tokens. Must be the same between the matcher and the server and not know to clients.

        uint8_t m_challengeKey[KeyBytes];                                   ///< Random key rolled each time Server::Start is called. Used to encrypt and decrypt challenge tokens without risking exposing the private key if two different servers use the same nonce (of course they will...).

        uint64_t m_challengeTokenNonce;                                     ///< Nonce used for encoding challenge tokens

        double m_time;                                                      ///< Current server time. See Server::AdvanceTime

        uint64_t m_flags;                                                   ///< Server flags. See Server::SetFlags.

        int m_maxClients;                                                   ///< The maximum number of clients supported by this server. Corresponds to maxClients passed in to the last Server::Start call.

        int m_numConnectedClients;                                          ///< The number of clients that are currently connected to the server.
        
        bool m_clientConnected[MaxClients];                                 ///< Array of connected flags per-client. Provides quick testing if a client is connected by client index.
        
        uint64_t m_clientId[MaxClients];                                    ///< Array of client id values per-client. Provides quick access to client id by client index.

        Address m_serverAddress;                                            ///< The address of this server (the address that clients will be connecting to).

        uint64_t m_globalSequence;                                          ///< The global sequence number for packets sent not corresponding to any particular connected client, eg. packets sent as part of connection negotiation.

        uint64_t m_clientSequence[MaxClients];                              ///< Per-client sequence number for packets sent to this client. Resets to zero each time the client slot is reset.

        Address m_clientAddress[MaxClients];                                ///< Array of client addresses. Provides quick access to client address by client index.
        
        ServerClientData m_clientData[MaxClients];                          ///< Per-client data. This is the bulk of the data, and contains duplicates of data used for fast access.

        bool m_allocateConnections;                                         ///< True if we should allocate connection objects in start. This is true if ClientServerConfig::enableMessages is true.

        Connection * m_clientConnection[MaxClients];                        ///< Per-client connection object. The connect object manages the set of channels and sending and receiving messages between client and server. Allocated in Server::Start according to maxClients and freed in Server::Stop.

        ConnectTokenEntry m_connectTokenEntries[MaxConnectTokenEntries];    ///< Array of connect tokens entries. Used to avoid replay attacks of the same connect token for different addresses.

        uint64_t m_counters[NUM_SERVER_COUNTERS];                           ///< Array of server counters. Used for debugging, testing and telemetry in production environments.

    private:

        Server( const Server & other );
        
        const Server & operator = ( const Server & other );
    };
}

/** 
    Helper macro to set the server allocator class.

    You can use this macro to specify that the server uses your own custom allocator class. The default allocator to use is TLSF_Allocator. 

    The constructor of your derived allocator class must match the signature of the TLSF_Allocator constructor to work with this macro.
    
    See tests/shared.h for an example of usage.
 */

#define YOJIMBO_SERVER_ALLOCATOR( allocator_class )                                                                     \
    Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes )                                   \
    {                                                                                                                   \
        return YOJIMBO_NEW( allocator, allocator_class, memory, bytes );                                                \
    }

/** 
    Helper macro to set the server packet factory class.
   
    See tests/shared.h for an example of usage.
 */

#define YOJIMBO_SERVER_PACKET_FACTORY( packet_factory_class )                                                           \
    PacketFactory * CreatePacketFactory( Allocator & allocator, ServerResourceType type, int clientIndex )              \
    {                                                                                                                   \
        (void) type;                                                                                                    \
        (void) clientIndex;                                                                                             \
        return YOJIMBO_NEW( allocator, packet_factory_class, allocator );                                               \
    }

/** 
    Helper macro to set the server message factory class.
   
    See tests/shared.h for an example of usage.
 */

#define YOJIMBO_SERVER_MESSAGE_FACTORY( message_factory_class )                                                         \
    MessageFactory * CreateMessageFactory( Allocator & allocator, ServerResourceType type, int clientIndex )            \
    {                                                                                                                   \
        (void) type;                                                                                                    \
        (void) clientIndex;                                                                                             \
        return YOJIMBO_NEW( allocator, message_factory_class, allocator );                                              \
    }

#endif // #ifndef YOJIMBO_SERVER_H
