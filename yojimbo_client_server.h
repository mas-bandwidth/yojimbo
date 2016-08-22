/*
    Yojimbo Client/Server Network Library.
    
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

#ifndef YOJIMBO_CLIENT_SERVER_H
#define YOJIMBO_CLIENT_SERVER_H

#include "yojimbo_config.h"
#include "yojimbo_packet.h"
#include "yojimbo_allocator.h"
#include "yojimbo_transport.h"
#include "yojimbo_encryption.h"
#include "yojimbo_connection.h"
#include "yojimbo_packet_processor.h"

namespace yojimbo
{
    const int MaxClients = 64;
    const int MaxConnectTokenEntries = MaxClients * 16;
    const int ConnectTokenBytes = 1024;
    const int ChallengeTokenBytes = 256;
    const int MaxServersPerConnectToken = 8;

    struct ClientServerConfig
    {
        int globalMemory;                                                   // memory allocated for connection request handling on the server only (bytes)

        int clientMemory;                                                   // per-client memory allocated once on the client and per-client slot on the server (bytes)

        int numDisconnectPackets;                                           // number of disconnect packets to spam on clean disconnect. avoids timeout.

        float connectionRequestSendRate;                                    // seconds between connection request packets sent.

        float connectionResponseSendRate;                                   // seconds between connection response packets sent.

        float connectionConfirmSendRate;                                    // seconds between heartbeat packets sent from server -> client prior to connection confirmation.
            
        float connectionHeartBeatRate;                                      // seconds between heartbeat packets sent after connection has been confirmed. sent only if other packets are not sent.

        float connectionRequestTimeOut;                                     // seconds before client connection requests gives up and times out.

        float challengeResponseTimeOut;                                     // seconds before challenge response times out.

        float connectionTimeOut;                                            // seconds before connection times out after connection has been established.

#if YOJIMBO_INSECURE_CONNECT

        float insecureConnectSendRate;                                      // seconds between insecure connect packets sent.

        float insecureConnectTimeOut;                                       // time in seconds after with an insecure connection request times out.

#endif // #if YOJIMBO_INSECURE_CONNECT

        bool enableConnection;                                              // enable per-client connection and messages. enabled by default.

        ConnectionConfig connectionConfig;                                  // connection configuration.

        ClientServerConfig()
        {
            globalMemory = 2 * 1024 * 1024;
            clientMemory = 2 * 1024 * 1024;
            numDisconnectPackets = 10;
            connectionRequestSendRate = 0.1f;
            connectionResponseSendRate = 0.1f;
            connectionConfirmSendRate = 0.1f;
            connectionHeartBeatRate = 1.0f;
            connectionRequestTimeOut = 5.0f;
            challengeResponseTimeOut = 5.0f;
            connectionTimeOut = 10.0f;
            enableConnection = true;
#if YOJIMBO_INSECURE_CONNECT
            insecureConnectSendRate = 0.1f;
            insecureConnectTimeOut = 5.0f;
#endif // #if YOJIMBO_INSECURE_CONNECT
        }        
    };

    struct ConnectToken
    {
        uint32_t protocolId;                                                // the protocol id this connect token corresponds to.
     
        uint64_t clientId;                                                  // the unique client id. max one connection per-client id, per-server.
     
        uint64_t expiryTimestamp;                                           // timestamp the connect token expires (eg. ~10 seconds after token creation)
     
        int numServerAddresses;                                             // the number of server addresses in the connect token whitelist.
     
        Address serverAddresses[MaxServersPerConnectToken];                 // connect token only allows connection to these server addresses.
     
        uint8_t clientToServerKey[KeyBytes];                                // the key for encrypted communication from client -> server.
     
        uint8_t serverToClientKey[KeyBytes];                                // the key for encrypted communication from server -> client.

        uint8_t random[KeyBytes];                                           // random data the client cannot possibly know.

        ConnectToken()
        {
            protocolId = 0;
            clientId = 0;
            expiryTimestamp = 0;
            numServerAddresses = 0;
            memset( clientToServerKey, 0, KeyBytes );
            memset( serverToClientKey, 0, KeyBytes );
            memset( random, 0, KeyBytes );
        }

        template <typename Stream> bool Serialize( Stream & stream )
        {
            serialize_uint32( stream, protocolId );

            serialize_uint64( stream, clientId );
            
            serialize_uint64( stream, expiryTimestamp );
            
            serialize_int( stream, numServerAddresses, 0, MaxServersPerConnectToken - 1 );
            
            for ( int i = 0; i < numServerAddresses; ++i )
                serialize_address( stream, serverAddresses[i] );

            serialize_bytes( stream, clientToServerKey, KeyBytes );

            serialize_bytes( stream, serverToClientKey, KeyBytes );

            serialize_bytes( stream, random, KeyBytes );

            return true;
        }

        bool operator == ( const ConnectToken & other ) const;
        bool operator != ( const ConnectToken & other ) const;
    };

    struct ChallengeToken
    {
        uint64_t clientId;                                                  // the unique client id. max one connection per-client id, per-server.

        uint8_t connectTokenMac[MacBytes];                                  // mac of the initial connect token this challenge corresponds to.
     
        uint8_t clientToServerKey[KeyBytes];                                // the key for encrypted communication from client -> server.
     
        uint8_t serverToClientKey[KeyBytes];                                // the key for encrypted communication from server -> client.

        uint8_t random[KeyBytes];                                           // random bytes the client cannot possibly know.

        ChallengeToken()
        {
            clientId = 0;
            memset( connectTokenMac, 0, MacBytes );
            memset( clientToServerKey, 0, KeyBytes );
            memset( serverToClientKey, 0, KeyBytes );
            memset( random, 0, KeyBytes );
        }

        template <typename Stream> bool Serialize( Stream & stream )
        {
            serialize_uint64( stream, clientId );

            serialize_bytes( stream, connectTokenMac, MacBytes );

            serialize_bytes( stream, clientToServerKey, KeyBytes );

            serialize_bytes( stream, serverToClientKey, KeyBytes );

            serialize_bytes( stream, random, KeyBytes );

            return true;
        }
    };

    void GenerateConnectToken( ConnectToken & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses, uint32_t protocolId, int expirySeconds );

    bool EncryptConnectToken( const ConnectToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool DecryptConnectToken( const uint8_t * encryptedMessage, ConnectToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool WriteConnectTokenToJSON( const ConnectToken & connectToken, char * output, int outputSize );

    bool ReadConnectTokenFromJSON( const char * json, ConnectToken & connectToken );

    bool GenerateChallengeToken( const ConnectToken & connectToken, const uint8_t * connectTokenMac, ChallengeToken & challengeToken );

    bool EncryptChallengeToken( ChallengeToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool DecryptChallengeToken( const uint8_t * encryptedMessage, ChallengeToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    struct ConnectionRequestPacket : public Packet
    {
        uint8_t connectTokenData[ConnectTokenBytes];                        // encrypted connect token data generated by matchmaker
        uint8_t connectTokenNonce[NonceBytes];                              // nonce required to decrypt the connect token on the server

        ConnectionRequestPacket()
        {
            memset( connectTokenData, 0, sizeof( connectTokenData ) );
            memset( connectTokenNonce, 0, sizeof( connectTokenNonce ) );
        }

        template <typename Stream> bool Serialize( Stream & stream )
        {
            serialize_bytes( stream, connectTokenData, sizeof( connectTokenData ) );
            serialize_bytes( stream, connectTokenNonce, sizeof( connectTokenNonce ) );
            return true;
        }

        YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionDeniedPacket : public Packet
    {
        template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

        YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionChallengePacket : public Packet
    {
        uint8_t challengeTokenData[ChallengeTokenBytes];                      // encrypted challenge token data generated by matchmaker
        uint8_t challengeTokenNonce[NonceBytes];                              // nonce required to decrypt the challenge token on the server

        ConnectionChallengePacket()
        {
            memset( challengeTokenData, 0, sizeof( challengeTokenData ) );
            memset( challengeTokenNonce, 0, sizeof( challengeTokenNonce ) );
        }

        template <typename Stream> bool Serialize( Stream & stream )
        {
            serialize_bytes( stream, challengeTokenData, sizeof( challengeTokenData ) );
            serialize_bytes( stream, challengeTokenNonce, sizeof( challengeTokenNonce ) );
            return true;
        }

        YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionResponsePacket : public Packet
    {
        uint8_t challengeTokenData[ChallengeTokenBytes];                      // encrypted challenge token data generated by matchmaker
        uint8_t challengeTokenNonce[NonceBytes];                              // nonce required to decrypt the challenge token on the server

        ConnectionResponsePacket()
        {
            memset( challengeTokenData, 0, sizeof( challengeTokenData ) );
            memset( challengeTokenNonce, 0, sizeof( challengeTokenNonce ) );
        }

        template <typename Stream> bool Serialize( Stream & stream )
        {
            serialize_bytes( stream, challengeTokenData, sizeof( challengeTokenData ) );
            serialize_bytes( stream, challengeTokenNonce, sizeof( challengeTokenNonce ) );
            return true;
        }

        YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionHeartBeatPacket : public Packet
    {
        int clientIndex;

        ConnectionHeartBeatPacket()
        {
            clientIndex = 0;
        }

        template <typename Stream> bool Serialize( Stream & stream )
        { 
            serialize_int( stream, clientIndex, 0, MaxClients - 1 );
            return true; 
        }

        YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionDisconnectPacket : public Packet
    {
        template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

        YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
    };

#if YOJIMBO_INSECURE_CONNECT
    struct InsecureConnectPacket : public Packet
    {
        uint64_t clientSalt;

        InsecureConnectPacket()
        {
            clientSalt = 0;
        }

        template <typename Stream> bool Serialize( Stream & stream )
        {
            serialize_uint64( stream, clientSalt );
            return true;
        }

        YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
    };
#endif // #if YOJIMBO_INSECURE_CONNECT

    enum ClientServerPacketTypes
    {
        CLIENT_SERVER_PACKET_CONNECTION_REQUEST,                      // client requests a connection.
        CLIENT_SERVER_PACKET_CONNECTION_DENIED,                       // server denies client connection request.
        CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE,                    // server response to client connection request.
        CLIENT_SERVER_PACKET_CONNECTION_RESPONSE,                     // client response to server connection challenge.
        CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT,                    // heartbeat packet sent at some low rate (once per-second) to keep the connection alive.
        CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT,                   // courtesy packet to indicate that the other side has disconnected. better than a timeout.
#if YOJIMBO_INSECURE_CONNECT
        CLIENT_SERVER_PACKET_INSECURE_CONNECT,                        // client requests an insecure connection (dev only!)
#endif // #if YOJIMBO_INSECURE_CONNECT
        CLIENT_SERVER_PACKET_CONNECTION,                              // connection packet carries messages and other data once connection is established.
        CLIENT_SERVER_NUM_PACKETS
    };

    YOJIMBO_PACKET_FACTORY_START( ClientServerPacketFactory, PacketFactory, CLIENT_SERVER_NUM_PACKETS );
        YOJIMBO_DECLARE_PACKET_TYPE( CLIENT_SERVER_PACKET_CONNECTION_REQUEST,       ConnectionRequestPacket );
        YOJIMBO_DECLARE_PACKET_TYPE( CLIENT_SERVER_PACKET_CONNECTION_DENIED,        ConnectionDeniedPacket );
        YOJIMBO_DECLARE_PACKET_TYPE( CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE,     ConnectionChallengePacket );
        YOJIMBO_DECLARE_PACKET_TYPE( CLIENT_SERVER_PACKET_CONNECTION_RESPONSE,      ConnectionResponsePacket );
        YOJIMBO_DECLARE_PACKET_TYPE( CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT,     ConnectionHeartBeatPacket );
        YOJIMBO_DECLARE_PACKET_TYPE( CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT,    ConnectionDisconnectPacket );
#if YOJIMBO_INSECURE_CONNECT
        YOJIMBO_DECLARE_PACKET_TYPE( CLIENT_SERVER_PACKET_INSECURE_CONNECT,         InsecureConnectPacket );
#endif // #if YOJIMBO_INSECURE_CONNECT
        YOJIMBO_DECLARE_PACKET_TYPE( CLIENT_SERVER_PACKET_CONNECTION,               ConnectionPacket );
    YOJIMBO_PACKET_FACTORY_FINISH()

    enum ServerResourceType
    {
        SERVER_RESOURCE_GLOBAL,                                     // this resource is global for a client/server instance.
        SERVER_RESOURCE_PER_CLIENT                                  // this resource is for a particular client slot. see client index.
    };

    enum ServerClientError
    {
        SERVER_CLIENT_ERROR_TIMEOUT,                                // the client timed out on the server
        SERVER_CLIENT_ERROR_CONNECTION,                             // the connection is in error state for this client
        SERVER_CLIENT_ERROR_STREAM_ALLOCATOR,                       // the stream allocator is in error state for this client
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

    struct ClientServerContext : public ConnectionContext {};

    enum ClientState
    {
#if YOJIMBO_INSECURE_CONNECT
        CLIENT_STATE_INSECURE_CONNECT_TIMEOUT = -9,
#endif // #if YOJIMBO_INSECURE_CONNECT
        CLIENT_STATE_PACKET_FACTORY_ERROR = -8,
        CLIENT_STATE_MESSAGE_FACTORY_ERROR = -7,
        CLIENT_STATE_STREAM_ALLOCATOR_ERROR = -6,
        CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT = -5,
        CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT = -4,
        CLIENT_STATE_CONNECTION_TIMEOUT = -3,
        CLIENT_STATE_CONNECTION_ERROR = -2,
        CLIENT_STATE_CONNECTION_DENIED = -1,
        CLIENT_STATE_DISCONNECTED = 0,
#if YOJIMBO_INSECURE_CONNECT
        CLIENT_STATE_SENDING_INSECURE_CONNECT,
#endif // #if YOJIMBO_INSECURE_CONNECT
        CLIENT_STATE_SENDING_CONNECTION_REQUEST,
        CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,
        CLIENT_STATE_CONNECTED
    };

    const char * GetClientStateName( int clientState );

    class Client : public ConnectionListener
    {
    public:

        explicit Client( Allocator & allocator, Transport & transport, const ClientServerConfig & config = ClientServerConfig() );

        virtual ~Client();

#if YOJIMBO_INSECURE_CONNECT
        void InsecureConnect( const Address & address );
#endif // #if YOJIMBO_INSECURE_CONNECT

        void Connect( const Address & address, 
                      const uint8_t * connectTokenData, 
                      const uint8_t * connectTokenNonce,
                      const uint8_t * clientToServerKey,
                      const uint8_t * serverToClientKey );

        bool IsConnecting() const;

        bool IsConnected() const;

        bool IsDisconnected() const;

        bool ConnectionFailed() const;

        ClientState GetClientState() const;

        void Disconnect( int clientState = CLIENT_STATE_DISCONNECTED, bool sendDisconnectPacket = true );

        bool CanSendMessage();

        Message * CreateMessage( int type );

        void SendMessage( Message * message );

        Message * ReceiveMessage();

        void ReleaseMessage( Message * message );

        MessageFactory & GetMessageFactory();

        void SendPackets();

        void ReceivePackets();

        void CheckForTimeOut();

        void AdvanceTime( double time );

        double GetTime() const;

        int GetClientIndex() const;

    protected:

        virtual void OnConnect( const Address & /*address*/ ) {}

        virtual void OnClientStateChange( int /*previousState*/, int /*currentState*/ ) {}

        virtual void OnDisconnect() {}

        virtual void OnPacketSent( int /*packetType*/, const Address & /*to*/, bool /*immediate*/ ) {}

        virtual void OnPacketReceived( int /*packetType*/, const Address & /*from*/, uint64_t /*sequence*/ ) {}

        virtual void OnConnectionPacketSent( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketAcked( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketReceived( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionFragmentReceived( Connection * /*connection*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/, int /*channelId*/ ) {}

        virtual bool ProcessGamePacket( Packet * /*packet*/, uint64_t /*sequence*/ ) { return false; }

    protected:

        virtual void InitializeConnection();

        virtual void SetEncryptedPacketTypes();

        virtual Allocator * CreateStreamAllocator();

        virtual PacketFactory * CreatePacketFactory();

        virtual MessageFactory * CreateMessageFactory();

        void SetClientState( int clientState );

        void ResetConnectionData( int clientState = CLIENT_STATE_DISCONNECTED );

        void SendPacketToServer( Packet * packet );

    private:

        void SendPacketToServer_Internal( Packet * packet, bool immediate = false );

    protected:

        void ProcessConnectionDenied( const ConnectionDeniedPacket & packet, const Address & address );

        void ProcessConnectionChallenge( const ConnectionChallengePacket & packet, const Address & address );

        void ProcessConnectionHeartBeat( const ConnectionHeartBeatPacket & packet, const Address & address );

        void ProcessConnectionDisconnect( const ConnectionDisconnectPacket & packet, const Address & address );

        void ProcessConnectionPacket( ConnectionPacket & packet, const Address & address );

        void ProcessPacket( Packet * packet, const Address & address, uint64_t sequence );

        bool IsPendingConnect();

        void CompletePendingConnect( int clientIndex );

    protected:

        void Defaults();

        ClientServerConfig m_config;                                        // client/server configuration.

        Allocator * m_allocator;                                            // the allocator used to create and destroy the client connection object.

        Allocator * m_streamAllocator;                                      // stream allocator passed in to the packet serialize for in-place allocations.

        MessageFactory * m_messageFactory;                                  // message factory for creating and destroying messages. optional.

        bool m_allocateConnection;                                          // true if we should allocate a connection.

        Connection * m_connection;                                          // the connection object for exchanging messages with the server. optional.

        int m_clientIndex;                                                  // the client index on the server [0,maxClients-1]. -1 if not connected.

        ClientServerContext m_context;                                      // serialization context for client/server packets.

        ClientState m_clientState;                                          // current client state

        Address m_serverAddress;                                            // server address we are connecting or connected to.

        double m_lastPacketSendTime;                                        // time we last sent a packet to the server.

        double m_lastPacketReceiveTime;                                     // time we last received a packet from the server (used for timeouts).

        Transport * m_transport;                                            // transport for sending and receiving packets

        double m_time;                                                      // current client time (see "AdvanceTime")

#if YOJIMBO_INSECURE_CONNECT
        uint64_t m_clientSalt;                                              // client salt for insecure connect
#endif // #if YOJIMBO_INSECURE_CONNECT

        uint64_t m_sequence;                                                // packet sequence # for packets sent to the server

        uint8_t m_connectTokenData[ConnectTokenBytes];                      // encrypted connect token data for connection request packet

        uint8_t m_connectTokenNonce[NonceBytes];                            // nonce required to send to server so it can decrypt connect token

        uint8_t m_challengeTokenData[ChallengeTokenBytes];                  // encrypted challenge token data for challenge response packet

        uint8_t m_challengeTokenNonce[NonceBytes];                          // nonce required to send to server so it can decrypt challenge token

    private:

        Client( const Client & other );
        
        const Client & operator = ( const Client & other );
    };

    enum ServerCounters
    {
        SERVER_COUNTER_CONNECTION_REQUEST_PACKETS_RECEIVED,
        SERVER_COUNTER_CONNECT_TOKEN_FAILED_TO_DECRYPT,
        SERVER_COUNTER_CONNECT_TOKEN_SERVER_ADDRESS_NOT_IN_WHITELIST,
        SERVER_COUNTER_CONNECT_TOKEN_CLIENT_ID_IS_ZERO,
        SERVER_COUNTER_CONNECT_TOKEN_CLIENT_ID_ALREADY_CONNECTED,
        SERVER_COUNTER_CONNECT_TOKEN_EXPIRED,
        SERVER_COUNTER_CONNECT_TOKEN_ALREADY_USED,
        SERVER_COUNTER_ENCRYPTION_MAPPING_CANNOT_ADD,
        SERVER_COUNTER_CONNECTION_DENIED_SERVER_IS_FULL,
        SERVER_COUNTER_CHALLENGE_TOKEN_FAILED_TO_GENERATE,
        SERVER_COUNTER_CHALLENGE_TOKEN_FAILED_TO_ENCRYPT,
        SERVER_COUNTER_CHALLENGE_TOKEN_FAILED_TO_DECRYPT,
        SERVER_COUNTER_CHALLENGE_PACKETS_SENT,
        SERVER_COUNTER_CHALLENGE_RESPONSE_PACKETS_RECEIVED,
        SERVER_COUNTER_CLIENT_CONNECTS,
        SERVER_COUNTER_CLIENT_DISCONNECTS,
        SERVER_COUNTER_CLIENT_CLEAN_DISCONNECTS,
        SERVER_COUNTER_CLIENT_TIMEOUTS,
        SERVER_COUNTER_CLIENT_CONNECTION_ERRORS,
        SERVER_COUNTER_CLIENT_STREAM_ALLOCATOR_ERRORS,
        SERVER_COUNTER_CLIENT_MESSAGE_FACTORY_ERRORS,
        SERVER_COUNTER_CLIENT_PACKET_FACTORY_ERRORS,
        SERVER_COUNTER_GLOBAL_PACKET_FACTORY_ERRORS,
        SERVER_COUNTER_GLOBAL_STREAM_ALLOCATOR_ERRORS,
        SERVER_COUNTER_NUM_COUNTERS
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

        Message * CreateMessage( int clientIndex, int type );

        bool CanSendMessage( int clientIndex ) const;

        void SendMessage( int clientIndex, Message * message );

        Message * ReceiveMessage( int clientIndex );

        void ReleaseMessage( int clientIndex, Message * message );

        MessageFactory & GetMessageFactory( int clientIndex );

        Packet * CreateGlobalPacket( int type );

        Packet * CreateClientPacket( int clientIndex, int type );

        void SendPackets();

        void ReceivePackets();

        void CheckForTimeOut();

        void AdvanceTime( double time );

        void SetFlags( uint64_t flags );

        // accessors

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

    protected:

        virtual void OnStart( int /*maxClients*/ ) {}

        virtual void OnStop() {}

        virtual void OnClientConnect( int /*clientIndex*/ ) {}

        virtual void OnClientDisconnect( int /*clientIndex*/ ) {}

        virtual void OnClientError( int /*clientIndex*/, ServerClientError /*error*/ ) {}

        virtual void OnPacketSent( int /*packetType*/, const Address & /*to*/, bool /*immediate*/ ) {}

        virtual void OnPacketReceived( int /*packetType*/, const Address & /*from*/, uint64_t /*sequence*/ ) {}

        virtual void OnConnectionPacketSent( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketAcked( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketReceived( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionFragmentReceived( Connection * /*connection*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/, int /*channelId*/ ) {}

        virtual bool ProcessGamePacket( int /*clientIndex*/, Packet * /*packet*/, uint64_t /*sequence*/ ) { return false; }

    protected:

        virtual void InitializeGlobalContext();

        virtual void SetEncryptedPacketTypes();

        virtual Allocator * CreateStreamAllocator( ServerResourceType type, int clientIndex );

        virtual PacketFactory * CreatePacketFactory( int clientIndex );

        virtual MessageFactory * CreateMessageFactory( int clientIndex );

        virtual ClientServerContext * CreateContext( ServerResourceType type, int clientIndex );

        virtual void ResetClientState( int clientIndex );

        int FindFreeClientIndex() const;

        int FindExistingClientIndex( const Address & address ) const;

        int FindExistingClientIndex( const Address & address, uint64_t clientId ) const;

        bool FindConnectTokenEntry( const uint8_t * mac );
        
        bool FindOrAddConnectTokenEntry( const Address & address, const uint8_t * mac );

        int FindClientId( uint64_t clientId ) const;

        int FindAddressAndClientId( const Address & address, uint64_t clientId ) const;

        void ConnectClient( int clientIndex, const Address & clientAddress, uint64_t clientId );

        void SendPacket( const Address & address, Packet * packet, bool immediate = false );

        void SendPacketToConnectedClient( int clientIndex, Packet * packet, bool immediate = false );

        void ProcessConnectionRequest( const ConnectionRequestPacket & packet, const Address & address );

        void ProcessConnectionResponse( const ConnectionResponsePacket & packet, const Address & address );

        void ProcessConnectionHeartBeat( const ConnectionHeartBeatPacket & /*packet*/, const Address & address );

        void ProcessConnectionDisconnect( const ConnectionDisconnectPacket & /*packet*/, const Address & address );

#if YOJIMBO_INSECURE_CONNECT
        void ProcessInsecureConnect( const InsecureConnectPacket & /*packet*/, const Address & address );
#endif // #if YOJIMBO_INSECURE_CONNECT

        void ProcessConnectionPacket( ConnectionPacket & packet, const Address & address );

        void ProcessPacket( Packet * packet, const Address & address, uint64_t sequence );

        ConnectionHeartBeatPacket * CreateHeartBeatPacket( int clientIndex );

        Transport * GetTransport() { return m_transport; }

    private:

        void Defaults();

        ClientServerConfig m_config;                                        // client/server configuration.

        Allocator * m_allocator;                                            // allocator used for creating connections per-client.

        Allocator * m_globalStreamAllocator;                                // stream allocator for global packets. eg. packets not corresponding to an active client slot.

        Allocator * m_clientStreamAllocator[MaxClients];                    // stream allocator for per-client packets. this allocator is used once a connection is established.

        Transport * m_transport;                                            // transport interface for sending and receiving packets.

        ClientServerContext * m_globalContext;                              // global serialization context for client/server packets. used prior to connection.

        ClientServerContext * m_clientContext[MaxClients];                  // per-client serialization context for client/server packets once connected.

        MessageFactory * m_clientMessageFactory[MaxClients];                // message factory for creating and destroying messages. per-client and optional.

        PacketFactory * m_clientPacketFactory[MaxClients];                  // packet factory for creating and destroying packets. per-client. required.

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

        uint64_t m_counters[SERVER_COUNTER_NUM_COUNTERS];

    private:

        Server( const Server & other );
        
        const Server & operator = ( const Server & other );
    };
}

#endif // #ifndef YOJIMBO_CLIENT_SERVER_H
