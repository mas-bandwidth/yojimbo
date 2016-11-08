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
    enum ServerResourceType
    {
        SERVER_RESOURCE_GLOBAL,                                     // this resource is global for a client/server instance.
        SERVER_RESOURCE_PER_CLIENT                                  // this resource is for a particular client slot. see client index.
    };

    enum ServerClientError
    {
        SERVER_CLIENT_ERROR_TIMEOUT,                                // the client timed out on the server
        SERVER_CLIENT_ERROR_ALLOCATOR,                              // the allocator is in error state for this client
        SERVER_CLIENT_ERROR_CONNECTION,                             // the connection is in error state for this client
        SERVER_CLIENT_ERROR_MESSAGE_FACTORY,                        // the message factory is in error state for this client
        SERVER_CLIENT_ERROR_PACKET_FACTORY,                         // the packet factory is in error state for this client
    };

    struct ServerClientData
    {
        Address address;
        uint64_t clientId;
#if YOJIMBO_INSECURE_CONNECT
        uint64_t clientSalt;
#endif // #if YOJIMBO_INSECURE_CONNECT
        double connectTime;
        double lastPacketSendTime;
        double lastHeartBeatSendTime;
        double lastPacketReceiveTime;
        bool fullyConnected;

        ServerClientData()
        {
            clientId = 0;
#if YOJIMBO_INSECURE_CONNECT
            clientSalt = 0;
#endif // #if YOJIMBO_INSECURE_CONNECT
            connectTime = 0.0;
            lastPacketSendTime = 0.0;
            lastHeartBeatSendTime = 0.0;
            lastPacketReceiveTime = 0.0;
            fullyConnected = false;
        }
    };

    struct ConnectTokenEntry
    {
        double time;                                                       // time for this entry. used to replace the oldest entries once the connect token array fills up.
        Address address;                                                   // address of the client that sent the connect token. binds a connect token to a particular address so it can't be exploited.
        uint8_t mac[MacBytes];                                             // hmac of connect token. we use this to avoid replay attacks where the same token is sent repeatedly for different addresses.

        ConnectTokenEntry()
        {
            time = -1000.0;
            memset( mac, 0, MacBytes );
        }
    };

    enum ServerCounters
    {
        SERVER_COUNTER_CONNECTION_REQUEST_PACKETS_RECEIVED,
        SERVER_COUNTER_CONNECTION_REQUEST_CHALLENGE_PACKETS_SENT,
        SERVER_COUNTER_CONNECTION_REQUEST_DENIED_SERVER_IS_FULL,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_BECAUSE_FLAG_IS_SET,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_DECRYPT_CONNECT_TOKEN,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_SERVER_ADDRESS_NOT_IN_WHITELIST,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_IS_ZERO,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_ADDRESS_ALREADY_CONNECTED,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ADD_ENCRYPTION_MAPPING,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ALLOCATE_CHALLENGE_PACKET,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_GENERATE_CHALLENGE_TOKEN,
        SERVER_COUNTER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ENCRYPT_CHALLENGE_TOKEN,

        SERVER_COUNTER_CHALLENGE_RESPONSE_ACCEPTED,
        SERVER_COUNTER_CHALLENGE_RESPONSE_DENIED_SERVER_IS_FULL,
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_BECAUSE_FLAG_IS_SET,
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_ADDRESS_ALREADY_CONNECTED,
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_CLIENT_ID_ALREADY_CONNECTED,
        SERVER_COUNTER_CHALLENGE_RESPONSE_IGNORED_FAILED_TO_DECRYPT_CHALLENGE_TOKEN,
        SERVER_COUNTER_CHALLENGE_RESPONSE_PACKETS_RECEIVED,
        
        SERVER_COUNTER_CLIENT_CONNECTS,
        SERVER_COUNTER_CLIENT_DISCONNECTS,
        SERVER_COUNTER_CLIENT_CLEAN_DISCONNECTS,
        SERVER_COUNTER_CLIENT_TIMEOUTS,
        SERVER_COUNTER_CLIENT_ALLOCATOR_ERRORS,
        SERVER_COUNTER_CLIENT_CONNECTION_ERRORS,
        SERVER_COUNTER_CLIENT_MESSAGE_FACTORY_ERRORS,
        SERVER_COUNTER_CLIENT_PACKET_FACTORY_ERRORS,
        SERVER_COUNTER_GLOBAL_PACKET_FACTORY_ERRORS,
        SERVER_COUNTER_GLOBAL_ALLOCATOR_ERRORS,
        
        NUM_SERVER_COUNTERS
    };

    enum ServerConnectionRequestAction
    {
        SERVER_CONNECTION_REQUEST_CHALLENGE_PACKET_SENT,
        SERVER_CONNECTION_REQUEST_DENIED_SERVER_IS_FULL,
        SERVER_CONNECTION_REQUEST_IGNORED_BECAUSE_FLAG_IS_SET,
        SERVER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_EXPIRED,
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_DECRYPT_CONNECT_TOKEN,
        SERVER_CONNECTION_REQUEST_IGNORED_SERVER_ADDRESS_NOT_IN_WHITELIST,
        SERVER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_IS_ZERO,
        SERVER_CONNECTION_REQUEST_IGNORED_ADDRESS_ALREADY_CONNECTED,
        SERVER_CONNECTION_REQUEST_IGNORED_CLIENT_ID_ALREADY_CONNECTED,
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ADD_ENCRYPTION_MAPPING,
        SERVER_CONNECTION_REQUEST_IGNORED_CONNECT_TOKEN_ALREADY_USED,
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_GENERATE_CHALLENGE_TOKEN,
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ALLOCATE_CHALLENGE_PACKET,
        SERVER_CONNECTION_REQUEST_IGNORED_FAILED_TO_ENCRYPT_CHALLENGE_TOKEN
    };

    enum ServerChallengeResponseAction
    {
        SERVER_CHALLENGE_RESPONSE_ACCEPTED,
        SERVER_CHALLENGE_RESPONSE_DENIED_SERVER_IS_FULL,
        SERVER_CHALLENGE_RESPONSE_IGNORED_BECAUSE_FLAG_IS_SET,
        SERVER_CHALLENGE_RESPONSE_IGNORED_FAILED_TO_DECRYPT_CHALLENGE_TOKEN,
        SERVER_CHALLENGE_RESPONSE_IGNORED_ADDRESS_ALREADY_CONNECTED,
        SERVER_CHALLENGE_RESPONSE_IGNORED_CLIENT_ID_ALREADY_CONNECTED,
    };

    enum ServerFlags
    {
        SERVER_FLAG_IGNORE_CONNECTION_REQUESTS = (1<<0),
        SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES = (1<<1),
        SERVER_FLAG_ALLOW_INSECURE_CONNECT = (1<<2)
    };

    class Server : public ConnectionListener
    {
    public:

        Server( Allocator & allocator, Transport & transport, const ClientServerConfig & config = ClientServerConfig() );

        virtual ~Server();

        void SetPrivateKey( const uint8_t * privateKey );
        
        void SetServerAddress( const Address & address );

        void Start( int maxClients = MaxClients );

        void Stop();

        void DisconnectClient( int clientIndex, bool sendDisconnectPacket = true );

        void DisconnectAllClients( bool sendDisconnectPacket = true );

        Message * CreateMsg( int clientIndex, int type );

        bool CanSendMsg( int clientIndex ) const;

        void SendMsg( int clientIndex, Message * message );

        Message * ReceiveMsg( int clientIndex );

        void ReleaseMsg( int clientIndex, Message * message );

        MessageFactory & GetMsgFactory( int clientIndex );

        Packet * CreateGlobalPacket( int type );

        Packet * CreateClientPacket( int clientIndex, int type );

        void SendPackets();

        void ReceivePackets();

        void CheckForTimeOut();

        void AdvanceTime( double time );

        void SetFlags( uint64_t flags );

        void SetUserContext( void * context );

        bool IsRunning() const;

        int GetMaxClients() const;

        bool IsClientConnected( int clientIndex ) const;

        int GetNumConnectedClients() const;

        int FindClientIndex( const Address & address ) const;

        uint64_t GetClientId( int clientIndex ) const;

        const Address & GetServerAddress() const;

        const Address & GetClientAddress( int clientIndex ) const;

        uint64_t GetCounter( int index ) const;

        double GetTime() const;

        uint64_t GetFlags() const;

        const ConnectionConfig & GetConnectionConfig() const { return m_config.connectionConfig; }

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

        virtual void CreateAllocators();

        virtual void DestroyAllocators(); 

        Allocator & GetAllocator( ServerResourceType type, int clientIndex );

        virtual void InitializeGlobalContext();

        virtual void SetEncryptedPacketTypes();

        virtual PacketFactory * CreatePacketFactory( Allocator & allocator, ServerResourceType type, int clientIndex );

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator, ServerResourceType type, int clientIndex );

        virtual ClientServerContext * CreateContext( Allocator & allocator, ServerResourceType type, int clientIndex );

        virtual void ResetClientState( int clientIndex );

        int FindFreeClientIndex() const;

        int FindExistingClientIndex( const Address & address ) const;

        int FindExistingClientIndex( const Address & address, uint64_t clientId ) const;

        bool FindConnectTokenEntry( const uint8_t * mac );
        
        bool FindOrAddConnectTokenEntry( const Address & address, const uint8_t * mac );

        int FindAddress( const Address & address ) const;

        int FindClientId( uint64_t clientId ) const;

        void ConnectClient( int clientIndex, const Address & clientAddress, uint64_t clientId );

        void SendPacket( const Address & address, Packet * packet, bool immediate = false );

        void SendPacketToConnectedClient( int clientIndex, Packet * packet, bool immediate = false );

        void ProcessConnectionRequest( const ConnectionRequestPacket & packet, const Address & address );

        void ProcessChallengeResponse( const ChallengeResponsePacket & packet, const Address & address );

        void ProcessKeepAlive( const KeepAlivePacket & /*packet*/, const Address & address );

        void ProcessDisconnect( const DisconnectPacket & /*packet*/, const Address & address );

#if YOJIMBO_INSECURE_CONNECT
        void ProcessInsecureConnect( const InsecureConnectPacket & /*packet*/, const Address & address );
#endif // #if YOJIMBO_INSECURE_CONNECT

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

        ClientServerContext * m_globalContext;                              // global serialization context for client/server packets. used prior to connection.

        ClientServerContext * m_clientContext[MaxClients];                  // per-client serialization context for client/server packets once connected.

        PacketFactory * m_globalPacketFactory;                              // packet factory for global packets (eg. conection request, challenge response packets prior to connection).

        PacketFactory * m_clientPacketFactory[MaxClients];                  // packet factory for creating and destroying packets. per-client. required.

        MessageFactory * m_clientMessageFactory[MaxClients];                // message factory for creating and destroying messages. per-client and optional.

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

        Connection * m_connection[MaxClients];                              // per-client connection. allocated and freed in start/stop according to max clients.

        ConnectTokenEntry m_connectTokenEntries[MaxConnectTokenEntries];    // array of connect tokens entries. used to avoid replay attacks of the same connect token for different addresses.

        uint64_t m_counters[NUM_SERVER_COUNTERS];

    private:

        Server( const Server & other );
        
        const Server & operator = ( const Server & other );
    };
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
