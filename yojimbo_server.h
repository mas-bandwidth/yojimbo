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
        Per-client slot data on the server. 
        Stores data for connected clients such as their address, globally unique client id, last packet send and receive times used for timeouts and keep-alive packets and so on.
     */

    struct ServerClientData
    {
        Address address;                                            ///< The address of this client. Packets are sent and received from the client using this address, therefore only one client with the address may be connected at any time.
        uint64_t clientId;                                          ///< Globally unique client id. Only one client with a specific client id may be connected to the server at any time.
#if !YOJIMBO_SECURE_MODE
        uint64_t clientSalt;                                        ///< The client salt is a random number rolled on each insecure client connect. It is used to distinguish one client connect session from another, so reconnects are more reliable. See Client::InsecureConnect for details.
#endif // #if !YOJIMBO_SECURE_MODE
        double connectTime;                                         ///< The time that the client connected to the server. Used to determine how long the client has been connected.
        double lastPacketSendTime;                                  ///< The last time a packet was sent to this client. Used to determine when it's necessary to send keep-alive packets.
        double lastPacketReceiveTime;                               ///< The last time a packet was received from this client. Used for timeouts.
        bool fullyConnected;                                        ///< True if this client is 'fully connected'. Fully connected means the client has received a keep-alive packet from the server containing its client index and replied back to the server with a keep-alive packet confirming that it knows its client index.
#if !YOJIMBO_SECURE_MODE
        bool insecure;                                              ///< True if this client connected in insecure mode. This means the client connected via Client::InsecureConnect and is sending and receiving packets without encryption. Please use insecure mode only during development, it is not suitable for production use.
#endif // #if !YOJIMBO_SECURE_MODE

        ServerClientData()
        {
            clientId = 0;
#if !YOJIMBO_SECURE_MODE
            clientSalt = 0;
#endif // #if !YOJIMBO_SECURE_MODE
            connectTime = 0.0;
            lastPacketSendTime = 0.0;
            lastPacketReceiveTime = 0.0;
            fullyConnected = false;
#if !YOJIMBO_SECURE_MODE
            insecure = false;
#endif // #if !YOJIMBO_SECURE_MODE
        }
    };

    /**
        Connect token entries are used to remember and reject recently used connect tokens sent from clients.

        This protects against replay attacks where the same connect token is used for multiple zombie clients.
     */

    struct ConnectTokenEntry
    {
        double time;                                                       ///< The time for this entry. Used to replace the oldest entries once the connect token array fills up.
        Address address;                                                   ///< Address of the client that sent this connect token. Binds a connect token to a particular address so it can't be exploited.
        uint8_t mac[MacBytes];                                             ///< HMAC of connect token. We use this to avoid replay attacks where the same token is sent repeatedly for different addresses.

        ConnectTokenEntry()
        {
            time = -1000.0;
            memset( mac, 0, MacBytes );
        }
    };

    /**
        The server counters provide insight into the number of times an action was performed by the server.

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
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_IS_ZERO,                            ///< Number of times the server ignored a connection request because the client id is zero. Please contact me if this is a problem for your game/application.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_ADDRESS_ALREADY_CONNECTED,                    ///< Number of times the server ignored a connection request because a client with that address is already connected to the server.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED,                  ///< Number of times the server ignored a connection request because a client with that client id was already connected to the server.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED,                        ///< Number of times the server ignored a connection request because the connect token has expired (they're typically only valid for a short amount of time, like 45-60 seconds, to fight replay attacks).
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED,                   ///< Number of times the server ignored a connection request because a client has already used that connect token to connect to this server. This should typically be zero, a non-zero value indicates shennanigans!
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ADD_ENCRYPTION_MAPPING,             ///< Number of times the server ignored a connection request because it could not add an encryption mapping for that client. If this is non-zero, it would indicate that somebody is somehow attacking the server with valid connection tokens. @see yojimbo::MaxEncryptionMappings.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ALLOCATE_CHALLENGE_PACKET,          ///< Number of times the server ignored a connection request because it could not allocate a challenge packet to send back to the client. This would indicate that the server has insufficient global resources (packet factory, allocator) to handle the connection negotiation load. @see ClientServerConfig::serverGlobalMemory.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_GENERATE_CHALLENGE_TOKEN,           ///< Number of times the server ignored a connection request because it could not generate a challenge token to send back to the client. Something is probably wrong with libsodium.
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ENCRYPT_CHALLENGE_TOKEN,            ///< Number of times the server ignored a connection request because it could not encrypt a challenge token to send back to the client. Something is probably wrong with libsodium.

        SERVER_COUNTER_CHALLENGE_RESPONSE_PACKETS_RECEIVED,                                     ///< Number of challenge response packets received by the server.
        SERVER_COUNTER_CHALLENGE_RESPONSE_ACCEPTED,                                             ///< Number of times the server accepted a challenge response and transitioned that client to connected.
        SERVER_COUNTER_CHALLENGE_RESPONSE_DENIED_SERVER_IS_FULL,                                ///< Number of times the server denied a challenge response because the server is full. This happens when multiple clients are racing to connect to the same client slot.
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_BECAUSE_FLAG_IS_SET,                          ///< Number of times the server ignored a challenge response packet sent from a client because the SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES flag is set.
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_ADDRESS_ALREADY_CONNECTED,                    ///< Number of times the server ignored a challange response packet because a client with that address is already connected.
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_CLIENT_ID_ALREADY_CONNECTED,                  ///< Number of times the server ignored a challenge response because a client with that client id is already connected. 
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_FAILED_TO_DECRYPT_CHALLENGE_TOKEN,            ///< Number of times the server ignored a challenge response because it couldn't decrypt the challenge token.
        
        SERVER_COUNTER_CLIENT_CONNECTS,                                                         ///< Number of times a client has connected to the server.
        SERVER_COUNTER_CLIENT_DISCONNECTS,                                                      ///< Number of times a client has been disconnected from the server.
        SERVER_COUNTER_CLIENT_CLEAN_DISCONNECTS,                                                ///< Number of clean disconnects where the client sent disconnect packets to the server. You want lots of these.
        SERVER_COUNTER_CLIENT_TIMEOUTS,                                                         ///< Number of timeouts where the client disconnected without sending disconnect packets to the server. You want few of these.
        SERVER_COUNTER_CLIENT_ALLOCATOR_ERRORS,                                                 ///< Number of times a client was disconnected from the server because their allocator entered into an error state (eg. failed to allocate a block of memory). This indicates that the client has exhausted their per-client resources. @see yojimbo::serverPerClientMemory.
        SERVER_COUNTER_CLIENT_CONNECTION_ERRORS,                                                ///< Number of times a client was disconnected from the server because their connection entered into an error state. This indicates that something went wrong with the internal protocol for sending messages between client and server.
        SERVER_COUNTER_CLIENT_MESSAGE_FACTORY_ERRORS,                                           ///< Number of times a client was disconnected from the server because their packet factory went into an error state. This indicates that the client tried to create a packet but failed to do so.
        SERVER_COUNTER_CLIENT_PACKET_FACTORY_ERRORS,                                            ///< Number of times a client was disconnected from the server because their message factory went into an error state. This indicates that the client tried to create a message but failed to do so.
        SERVER_COUNTER_GLOBAL_PACKET_FACTORY_ERRORS,                                            ///< Number of times the global packet factory entered into an error state because it could not allocate a packet. This probably indicates insufficient global memory for the connection negotiation process on the server. @see ClientServerConfig::serverGlobalMemory.
        SERVER_COUNTER_GLOBAL_ALLOCATOR_ERRORS,                                                 ///< Number of times the global allocator went into error state because it could not perform an allocation. This probably indicates insufficient global memory for the connection negotiation process on the server. @see ClientServerConfig::serverGlobalMemory.
        
        NUM_SERVER_COUNTERS                                                                     ///< The number of server counters.
    };

    /**
        Describes the action the action taken by the server in response to a connection request.

        @see Server::OnConnectionRequest
     */

    enum ServerConnectionRequestAction
    {
        SERVER_CONNECTION_REQUEST_CHALLENGE_PACKET_SENT,                                        ///< The server replied with a challenge packet.
        SERVER_CONNECTION_REQUEST_DENIED_SERVER_IS_FULL,                                        ///< The server denied the connection request because the server is full.
        SERVER_CONNECTION_REQUEST_IGNORED_BECAUSE_FLAG_IS_SET,                                  ///< The server ignored the connection request because the SERVER_FLAG_IGNORE_CONNECTION_REQUESTS is set.
        SERVER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED,                                ///< The server ignored the connection request because the connect token has expired.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_DECRYPT_CONNECT_TOKEN,                      ///< The server ignored the connection request because it could not decrypt the connect token.
        SERVER_CONNECTION_REQUEST_IGNORED_SERVER_ADDRESS_NOT_IN_WHITELIST,                      ///< The server ignored the connection request because the server address is not in the connect token server address whitelist. @see Server::SetServerAddress.
        SERVER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_IS_ZERO,                                    ///< The server ignored the connection request because the client id is zero.
        SERVER_CONNECTION_REQUEST_IGNORED_ADDRESS_ALREADY_CONNECTED,                            ///< The server ignored the connection request because a client with that address is already connected.
        SERVER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED,                          ///< The server ignored the connection request because a client with thath client id is already connected.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ADD_ENCRYPTION_MAPPING,                     ///< The server ignored the connection request because it could not add an encryption mapping for that client. This is bad.
        SERVER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED,                           ///< The server ignored the connection request because another client has already used that connect token to connect to this server. This is bad.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_GENERATE_CHALLENGE_TOKEN,                   ///< The server ignored the connection request because it couldn't generate a challenge token. This is bad.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ALLOCATE_CHALLENGE_PACKET,                  ///< The server ignored the connection request because it couldn't allocate a challenge packet to send back to the client. This is bad.
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ENCRYPT_CHALLENGE_TOKEN                     ///< The server ignored the connection request because it couldn't encrypt the challenge token to send back to the client. This is bad.
    };

    enum ServerChallengeResponseAction
    {
        SERVER_CHALLENGE_RESPONSE_ACCEPTED,                                                     ///< The server accepted the challenge response and transitioned the client to connected.
        SERVER_CHALLENGE_RESPONSE_DENIED_SERVER_IS_FULL,                                        ///< The server denied the challenge respeonse because the server is full.
        SERVER_CHALLENGE_RESPONSE_IGNORED_BECAUSE_FLAG_IS_SET,                                  ///< The server ignored the challenge response because the SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES flag is set.
        SERVER_CHALLENGE_RESPONSE_IGNORED_FAILED_TO_DECRYPT_CHALLENGE_TOKEN,                    ///< The server ignored the challenge response because it could not decrypt the challenge token.
        SERVER_CHALLENGE_RESPONSE_IGNORED_ADDRESS_ALREADY_CONNECTED,                            ///< The server ignored the challange response because a client with that address is already connected.
        SERVER_CHALLENGE_RESPONSE_IGNORED_CLIENT_ID_ALREADY_CONNECTED,                          ///< The server ignored the challenge response because a client with that client id is already connected.
    };

    /// Server flags are used to enable and disable server features.                            

    enum ServerFlags
    {
        SERVER_FLAG_IGNORE_CONNECTION_REQUESTS = (1<<0),                                        ///< When this flag is set the server ignores all connection requests.
        SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES = (1<<1),                                        ///< When this flag is set the server ignores all challerge respeonses.
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
            Set the server IP address. This should be a public IP address, eg: the address that a client would use to connect to the server.
             
            @param address The server address.
         */

        void SetServerAddress( const Address & address );

        /**
            Start the server and allocate client slots.
            
            Each client that connects to this server occupies one of the client slots allocated by this function.

            @param maxClients The maximum number of client slots to allocate. Must be in range [1,MaxClients]

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
            @param sendDisconectPacket If true, disconnect packets are sent to the other side of the connection so it sees the disconnect as quickly as possible (rather than timing out).

            @see Server::IsClientConnected
         */

        void DisconnectClient( int clientIndex, bool sendDisconnectPacket = true );

        /**
            Disconnect all clients from the server.
            
            Client slots remain allocated as per the last call to Server::Start, they are simply made available for new clients to connect.
            
            @param sendDisconectPacket If true, disconnect packets are sent to the other side of the connection so it sees the disconnect as quickly as possible (rather than timing out).
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

            If no packet has been received within the timeout period, the client is disconnected and its client slot is made available for another client to connect to.

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

        Message * CreateMsg( int clientIndex, int type );

        bool CanSendMsg( int clientIndex ) const;

        void SendMsg( int clientIndex, Message * message );

        Message * ReceiveMsg( int clientIndex );

        void ReleaseMsg( int clientIndex, Message * message );

        MessageFactory & GetMsgFactory( int clientIndex );

        Packet * CreateGlobalPacket( int type );

        Packet * CreateClientPacket( int clientIndex, int type );

        void SetFlags( uint64_t flags );

        void SetUserContext( void * context );

        bool IsRunning() const;

        int GetMaxClients() const;

        bool IsClientConnected( int clientIndex ) const;

        int GetNumConnectedClients() const;

        int FindClientIndex( uint64_t clientId ) const;

        int FindClientIndex( const Address & address ) const;

        uint64_t GetClientId( int clientIndex ) const;

        const Address & GetServerAddress() const;

        const Address & GetClientAddress( int clientIndex ) const;

        uint64_t GetCounter( int index ) const;

        double GetTime() const;

        uint64_t GetFlags() const;

        Allocator & GetGlobalAllocator() { assert( m_globalAllocator ); return *m_globalAllocator; }

        Allocator & GetClientAllocator( int clientIndex ) { assert( clientIndex >= 0 ); assert( clientIndex < m_maxClients ); assert( m_clientAllocator[clientIndex] ); return *m_clientAllocator[clientIndex]; }

    protected:

        virtual void OnStart( int /*maxClients*/ ) {}

        virtual void OnStop() {}

        virtual void OnConnectionRequest( ServerConnectionRequestAction /*action*/, const ConnectionRequestPacket & /*packet*/, const Address & /*address*/, const ConnectToken & /*connectToken*/ ) {}

        virtual void OnChallengeResponse( ServerChallengeResponseAction /*action*/, const ChallengeResponsePacket & /*packet*/, const Address & /*address*/, const ChallengeToken & /*challengeToken*/ ) {}

        virtual void OnClientConnect( int /*clientIndex*/ ) {}

        virtual void OnClientDisconnect( int /*clientIndex*/ ) {}

        virtual void OnClientError( int /*clientIndex*/, ServerClientError /*error*/ ) {}

        virtual void OnPacketSent( int /*packetType*/, const Address & /*to*/, bool /*immediate*/ ) {}

        virtual void OnPacketReceived( int /*packetType*/, const Address & /*from*/ ) {}

        virtual void OnConnectionPacketSent( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketAcked( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketReceived( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionFragmentReceived( Connection * /*connection*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/, int /*channelId*/ ) {}

        virtual bool ProcessUserPacket( int /*clientIndex*/, Packet * /*packet*/ ) { return false; }

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

        void ProcessKeepAlive( const KeepAlivePacket & /*packet*/, const Address & address );

        void ProcessDisconnect( const DisconnectPacket & /*packet*/, const Address & address );

#if !YOJIMBO_SECURE_MODE
        void ProcessInsecureConnect( const InsecureConnectPacket & /*packet*/, const Address & address );
#endif // #if !YOJIMBO_SECURE_MODE

        void ProcessConnectionPacket( ConnectionPacket & packet, const Address & address );

        void ProcessPacket( Packet * packet, const Address & address, uint64_t sequence );

        KeepAlivePacket * CreateKeepAlivePacket( int clientIndex );

        Transport * GetTransport() { return m_transport; }

    private:

        void Defaults();

        ClientServerConfig m_config;                                        // client/server configuration.

        Allocator * m_allocator;                                            // allocator used for creating connections per-client.

        uint8_t * m_globalMemory;                                           // memory backing the global allocator.

        uint8_t * m_clientMemory[MaxClients];                               // memory backing the client allocators.

        Allocator * m_globalAllocator;                                      // global allocator 

        Allocator * m_clientAllocator[MaxClients];                          // per-client allocator

        Transport * m_transport;                                            // transport interface for sending and receiving packets.

        TransportContext m_globalTransportContext;                          // global transport context for reading and writing packets. used for packets not belonging to a connected client.

        TransportContext m_clientTransportContext[MaxClients];              // transport context for reading and writing packets that belong to connected clients.

        ConnectionContext m_clientConnectionContext[MaxClients];            // connection context for reading and writing connection packets (messages) for connected clients.

        PacketFactory * m_globalPacketFactory;                              // packet factory for global packets (eg. conection request, challenge response packets prior to connection).

        PacketFactory * m_clientPacketFactory[MaxClients];                  // packet factory for creating and destroying packets. per-client. required.

        MessageFactory * m_clientMessageFactory[MaxClients];                // message factory for creating and destroying messages. per-client and optional.

        ReplayProtection * m_clientReplayProtection[MaxClients];            // per-client protection against packet replay attacks. discards old and already received packets.

        uint8_t m_privateKey[KeyBytes];                                     // private key used for encrypting and decrypting tokens.

        uint64_t m_challengeTokenNonce;                                     // nonce used for encoding challenge tokens

        double m_time;                                                      // current server time (see "AdvanceTime")

        uint64_t m_flags;                                                   // server flags

        int m_maxClients;                                                   // maximum number of clients supported by this server

        int m_numConnectedClients;                                          // number of connected clients
        
        bool m_clientConnected[MaxClients];                                 // true if client n is connected
        
        uint64_t m_clientId[MaxClients];                                    // array of client id values per-client

        Address m_serverAddress;                                            // the external IP address of this server (what clients will be sending packets to)

        uint64_t m_globalSequence;                                          // global sequence number for packets sent not corresponding to any particular connected client.

        uint64_t m_clientSequence[MaxClients];                              // per-client sequence number for packets sent

        Address m_clientAddress[MaxClients];                                // array of client address values per-client
        
        ServerClientData m_clientData[MaxClients];                          // heavier weight data per-client, eg. not for fast lookup

        bool m_allocateConnections;                                         // true if we should allocate connection objects in start.

        Connection * m_clientConnection[MaxClients];                        // per-client connection. allocated and freed in start/stop according to max clients.

        ConnectTokenEntry m_connectTokenEntries[MaxConnectTokenEntries];    // array of connect tokens entries. used to avoid replay attacks of the same connect token for different addresses.

        uint64_t m_counters[NUM_SERVER_COUNTERS];

    private:

        Server( const Server & other );
        
        const Server & operator = ( const Server & other );
    };
}

#define YOJIMBO_SERVER_ALLOCATOR( allocator_class )                                                                     \
    Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes )                                   \
    {                                                                                                                   \
        return YOJIMBO_NEW( allocator, allocator_class, memory, bytes );                                                \
    }

#define YOJIMBO_SERVER_PACKET_FACTORY( packet_factory_class )                                                           \
    PacketFactory * CreatePacketFactory( Allocator & allocator, ServerResourceType /*type*/, int /*clientIndex*/ )      \
    {                                                                                                                   \
        return YOJIMBO_NEW( allocator, packet_factory_class, allocator );                                               \
    }

#define YOJIMBO_SERVER_MESSAGE_FACTORY( message_factory_class )                                                         \
    MessageFactory * CreateMessageFactory( Allocator & allocator, ServerResourceType /*type*/, int /*clientIndex*/ )    \
    {                                                                                                                   \
        return YOJIMBO_NEW( allocator, message_factory_class, allocator );                                              \
    }

#endif // #ifndef YOJIMBO_SERVER_H
