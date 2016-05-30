/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#ifndef YOJIMBO_CLIENT_SERVER_H
#define YOJIMBO_CLIENT_SERVER_H

#include "yojimbo_config.h"
#include "yojimbo_packet.h"
#include "yojimbo_crypto.h"
#include "yojimbo_network.h"
#include "yojimbo_packet_processor.h"
#include "yojimbo_network_interface.h"
#include "yojimbo_socket_interface.h"

namespace yojimbo
{
    const int MaxClients = 64;
    const int MaxConnectTokenEntries = MaxClients * 16;
    const int ConnectTokenBytes = 1024;
    const int ChallengeTokenBytes = 256;
    const int MaxServersPerConnectToken = 8;
    const int ConnectTokenExpirySeconds = 10;
    const float ConnectionRequestSendRate = 0.1f;
    const float ConnectionResponseSendRate = 0.1f;
    const float ConnectionConfirmSendRate = 0.1f;
    const float ConnectionHeartBeatRate = 1.0f;
    const float ConnectionRequestTimeOut = 5.0f;
    const float ChallengeResponseTimeOut = 5.0f;
    const float ConnectionTimeOut = 10.0f;

    template <typename Stream> bool serialize_address_internal( Stream & stream, Address & address )
    {
        char buffer[64];

        if ( Stream::IsWriting )
        {
            assert( address.IsValid() );
            address.ToString( buffer, sizeof( buffer ) );
        }

        // todo: serialize the address as binary instead.
        serialize_string( stream, buffer, sizeof( buffer ) );

        if ( Stream::IsReading )
        {
            address = Address( buffer );
            if ( !address.IsValid() )
                return false;
        }

        return true;
    }

    #define serialize_address( stream, value )                                              \
        do                                                                                  \
        {                                                                                   \
            if ( !yojimbo::serialize_address_internal( stream, value ) )                    \
                return false;                                                               \
        } while (0)

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
    };

    struct ChallengeToken
    {
        uint64_t clientId;                                                  // the unique client id. max one connection per-client id, per-server.

        Address clientAddress;                                              // client address corresponding to the initial connection request.

        Address serverAddress;                                              // client address corresponding to the initial connection request.

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
            
            serialize_address( stream, clientAddress );

            serialize_address( stream, serverAddress );

            serialize_bytes( stream, connectTokenMac, MacBytes );

            serialize_bytes( stream, clientToServerKey, KeyBytes );

            serialize_bytes( stream, serverToClientKey, KeyBytes );

            serialize_bytes( stream, random, KeyBytes );

            return true;
        }
    };

    void GenerateConnectToken( ConnectToken & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses, uint32_t protocolId );

    bool EncryptConnectToken( ConnectToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool DecryptConnectToken( const uint8_t * encryptedMessage, ConnectToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool GenerateChallengeToken( const ConnectToken & connectToken, const Address & clientAddress, const Address & serverAddress, const uint8_t * connectTokenMac, ChallengeToken & challengeToken );

    bool EncryptChallengeToken( ChallengeToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool DecryptChallengeToken( const uint8_t * encryptedMessage, ChallengeToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    enum PacketTypes
    {
        PACKET_CONNECTION_REQUEST,                      // client requests a connection.
        PACKET_CONNECTION_DENIED,                       // server denies client connection request.
        PACKET_CONNECTION_CHALLENGE,                    // server response to client connection request.
        PACKET_CONNECTION_RESPONSE,                     // client response to server connection challenge.
        PACKET_CONNECTION_HEARTBEAT,                    // heartbeat packet sent at some low rate (once per-second) to keep the connection alive.
        PACKET_CONNECTION_DISCONNECT,                   // courtesy packet to indicate that the other side has disconnected. better than a timeout.
        CLIENT_SERVER_NUM_PACKETS
    };

    struct ConnectionRequestPacket : public Packet
    {
        uint8_t connectTokenData[ConnectTokenBytes];                        // encrypted connect token data generated by matchmaker
        uint8_t connectTokenNonce[NonceBytes];                              // nonce required to decrypt the connect token on the server

        ConnectionRequestPacket() : Packet( PACKET_CONNECTION_REQUEST )
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

        YOJIMBO_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionDeniedPacket : public Packet
    {
        ConnectionDeniedPacket() : Packet( PACKET_CONNECTION_DENIED ) {}

        template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

        YOJIMBO_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionChallengePacket : public Packet
    {
        uint8_t challengeTokenData[ChallengeTokenBytes];                      // encrypted challenge token data generated by matchmaker
        uint8_t challengeTokenNonce[NonceBytes];                              // nonce required to decrypt the challenge token on the server

        ConnectionChallengePacket() : Packet( PACKET_CONNECTION_CHALLENGE )
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

        YOJIMBO_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionResponsePacket : public Packet
    {
        uint8_t challengeTokenData[ChallengeTokenBytes];                      // encrypted challenge token data generated by matchmaker
        uint8_t challengeTokenNonce[NonceBytes];                              // nonce required to decrypt the challenge token on the server

        ConnectionResponsePacket() : Packet( PACKET_CONNECTION_RESPONSE )
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

        YOJIMBO_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionHeartBeatPacket : public Packet
    {
        ConnectionHeartBeatPacket() : Packet( PACKET_CONNECTION_HEARTBEAT ) {}

        template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

        YOJIMBO_SERIALIZE_FUNCTIONS();
    };

    struct ConnectionDisconnectPacket : public Packet
    {
        ConnectionDisconnectPacket() : Packet( PACKET_CONNECTION_DISCONNECT ) {}

        template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

        YOJIMBO_SERIALIZE_FUNCTIONS();
    };

    struct ClientServerPacketFactory : public PacketFactory
    {
        ClientServerPacketFactory() : PacketFactory( CLIENT_SERVER_NUM_PACKETS ) {}

        Packet * Create( int type )
        {
            switch ( type )
            {
                case PACKET_CONNECTION_REQUEST:         return new ConnectionRequestPacket();
                case PACKET_CONNECTION_DENIED:          return new ConnectionDeniedPacket();
                case PACKET_CONNECTION_CHALLENGE:       return new ConnectionChallengePacket();
                case PACKET_CONNECTION_RESPONSE:        return new ConnectionResponsePacket();
                case PACKET_CONNECTION_HEARTBEAT:       return new ConnectionHeartBeatPacket();
                case PACKET_CONNECTION_DISCONNECT:      return new ConnectionDisconnectPacket();
                default:
                    return NULL;
            }
        }

        void Destroy( Packet * packet )
        {
            delete packet;
        }
    };

    struct ServerClientData
    {
        Address address;
        uint64_t clientId;
        double connectTime;
        double lastPacketSendTime;
        double lastPacketReceiveTime;

        ServerClientData()
        {
            clientId = 0;
            connectTime = 0.0;
            lastPacketSendTime = 0.0;
            lastPacketReceiveTime = 0.0;
        }
    };

    struct ConnectTokenEntry
    {
        double time;                                                       // time for this entry. used to replace the oldest entries once the connect token array fills up.
        Address address;                                                   // address of the client that sent the connect token. binds a connect token to a particular address so it can't be exploited.
        uint8_t mac[MacBytes];                                             // hmac of connect token. we use this to avoid replay attacks where the same token is sent repeatedly for different addresses.

        ConnectTokenEntry()
        {
            time = 0.0;
            memset( mac, 0, MacBytes );
        }
    };

    enum ServerCounters
    {
        SERVER_COUNTER_CONNECTION_REQUEST_PACKETS_RECEIVED,
        SERVER_COUNTER_FAILED_TO_DECRYPT_CONNECT_TOKEN,
        SERVER_COUNTER_SERVER_ADDRESS_NOT_IN_CONNECT_TOKEN_WHITELIST,
        SERVER_COUNTER_CONNECT_TOKEN_CLIENT_ID_IS_ZERO,
        SERVER_COUNTER_CONNECT_TOKEN_CLIENT_ID_ALREADY_CONNECTED,
        SERVER_COUNTER_CONNECT_TOKEN_EXPIRED,
        SERVER_COUNTER_ADD_ENCRYPTION_MAPPING_FAILURES,
        SERVER_COUNTER_CONNECTION_DENIED_SERVER_IS_FULL,
        SERVER_COUNTER_CONNECT_TOKEN_ALREADY_USED,
        SERVER_COUNTER_FAILED_TO_GENERATE_CHALLENGE_TOKEN,
        SERVER_COUNTER_FAILED_TO_ENCRYPT_CHALLENGE_TOKEN,
        SERVER_COUNTER_CHALLENGE_PACKETS_SENT,

        SERVER_COUNTER_CHALLENGE_RESPONSE_PACKETS_RECEIVED,
        SERVER_COUNTER_FAILED_TO_DECRYPT_CHALLENGE_TOKEN,
        SERVER_COUNTER_CHALLENGE_TOKEN_CLIENT_ADDRESS_DOES_NOT_MATCH,
        SERVER_COUNTER_CHALLENGE_TOKEN_SERVER_ADDRESS_DOES_NOT_MATCH,
        SERVER_COUNTER_CHALLENGE_CLIENT_ALREADY_CONNECTED_REPLY_WITH_HEARTBEAT,

        SERVER_COUNTER_CLIENT_CONNECTS,
        SERVER_COUNTER_CLIENT_DISCONNECTS,
        SERVER_COUNTER_CLIENT_CLEAN_DISCONNECTS,
        SERVER_COUNTER_CLIENT_TIMEOUT_DISCONNECTS,

        SERVER_COUNTER_NUM_COUNTERS
    };

    class Server
    {
        NetworkInterface * m_networkInterface;                              // network interface for sending and receiving packets.

        uint8_t m_privateKey[KeyBytes];                                     // private key used for encrypting and decrypting tokens.

        uint64_t m_challengeTokenNonce;                                     // nonce used for encoding challenge tokens

        int m_numConnectedClients;                                          // number of connected clients
        
        bool m_clientConnected[MaxClients];                                 // true if client n is connected
        
        uint64_t m_clientId[MaxClients];                                    // array of client id values per-client
        
        Address m_serverAddress;                                            // the external IP address of this server (what clients will be sending packets to)

        Address m_clientAddress[MaxClients];                                // array of client address values per-client
        
        ServerClientData m_clientData[MaxClients];                          // heavier weight data per-client, eg. not for fast lookup

        ConnectTokenEntry m_connectTokenEntries[MaxConnectTokenEntries];    // array of connect tokens entries. used to avoid replay attacks of the same connect token for different addresses.

        uint64_t m_counters[SERVER_COUNTER_NUM_COUNTERS];

    public:

        Server( NetworkInterface & networkInterface );

        virtual ~Server();

        void SetPrivateKey( const uint8_t * privateKey )
        {
            memcpy( m_privateKey, privateKey, KeyBytes );
        }

        void SetServerAddress( const Address & address )
        {
            m_serverAddress = address;
        }

        const Address & GetServerAddress() const
        {
            return m_serverAddress;
        }

        uint64_t GetClientId( int clientIndex ) const
        {
            assert( clientIndex >= 0 );
            assert( clientIndex < MaxClients );
            return m_clientId[clientIndex];
        }

        const Address & GetClientAddress( int clientIndex ) const
        {
            assert( clientIndex >= 0 );
            assert( clientIndex < MaxClients );
            return m_clientAddress[clientIndex];
        }

        int GetNumConnectedClients() 
        {
            return m_numConnectedClients;
        }

        uint64_t GetCounter( int index ) const 
        {
            assert( index >= 0 );
            assert( index < SERVER_COUNTER_NUM_COUNTERS );
            return m_counters[index];
        }

        void SendPackets( double time );

        void ReceivePackets( double time );

        void CheckForTimeOut( double time );

    protected:

        virtual void OnClientConnect( int /*clientIndex*/ ) {}

        virtual void OnClientDisconnect( int /*clientIndex*/ ) {}

        virtual void OnClientTimedOut( int /*clientIndex*/ ) {}

        virtual void OnPacketSent( int /*packetType*/, const Address & /*to*/ ) {}

        virtual void OnPacketReceived( int /*packetType*/, const Address & /*from*/ ) {}

    protected:

        void ResetClientState( int clientIndex );

        int FindFreeClientIndex() const;

        int FindExistingClientIndex( const Address & address ) const;

        int FindExistingClientIndex( const Address & address, uint64_t clientId ) const;

        bool FindOrAddConnectTokenEntry( const Address & address, const uint8_t * mac, double time );

        void ConnectClient( int clientIndex, const ChallengeToken & challengeToken, double time );

        void DisconnectClient( int clientIndex, double time );

        bool IsConnected( uint64_t clientId ) const;

        bool IsConnected( const Address & address, uint64_t clientId ) const;

        void SendPacket( const Address & address, Packet * packet );

        void SendPacketToConnectedClient( int clientIndex, Packet * packet, double time );

        void ProcessConnectionRequest( const ConnectionRequestPacket & packet, const Address & address, double time );

        void ProcessConnectionResponse( const ConnectionResponsePacket & packet, const Address & address, double time );

        void ProcessConnectionHeartBeat( const ConnectionHeartBeatPacket & /*packet*/, const Address & address, double time );

        void ProcessConnectionDisconnect( const ConnectionDisconnectPacket & /*packet*/, const Address & address, double time );
    };

    enum ClientState
    {
        CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT = -4,
        CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT = -3,
        CLIENT_STATE_CONNECTION_TIMED_OUT = -2,
        CLIENT_STATE_CONNECTION_DENIED = -1,
        CLIENT_STATE_DISCONNECTED = 0,
        CLIENT_STATE_SENDING_CONNECTION_REQUEST = 1,
        CLIENT_STATE_SENDING_CHALLENGE_RESPONSE = 2,
        CLIENT_STATE_CONNECTED = 3
    };

    inline const char * GetClientStateName( int clientState )
    {
        switch ( clientState )
        {
            case CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT: return "connection request timed out";
            case CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT: return "challenge response timed out";
            case CLIENT_STATE_CONNECTION_TIMED_OUT: return "connection timed out";
            case CLIENT_STATE_CONNECTION_DENIED: return "connection denied";
            case CLIENT_STATE_DISCONNECTED: return "disconnected";
            case CLIENT_STATE_SENDING_CONNECTION_REQUEST: return "sending connection request";
            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE: return "sending challenge response";
            case CLIENT_STATE_CONNECTED: return "connected";
            default:
                assert( false );
                return "???";
        }
    }

    class Client
    {
        ClientState m_clientState;                                          // current client state

        Address m_serverAddress;                                            // server address we are connecting or connected to.

        double m_lastPacketSendTime;                                        // time we last sent a packet to the server.

        double m_lastPacketReceiveTime;                                     // time we last received a packet from the server (used for timeouts).

        NetworkInterface * m_networkInterface;                              // network interface the client uses to send and receive packets.

        uint64_t m_clientId;                                                // client id as per-connect call

        uint8_t m_connectTokenData[ConnectTokenBytes];                      // encrypted connect token data for connection request packet

        uint8_t m_connectTokenNonce[NonceBytes];                            // nonce required to send to server so it can decrypt connect token

        uint8_t m_challengeTokenData[ChallengeTokenBytes];                  // encrypted challenge token data for challenge response packet

        uint8_t m_challengeTokenNonce[NonceBytes];                          // nonce required to send to server so it can decrypt challenge token

    public:

        Client( NetworkInterface & networkInterface );

        virtual ~Client();

        void Connect( const Address & address, 
                      double time, 
                      uint64_t clientId,
                      const uint8_t * connectTokenData, 
                      const uint8_t * connectTokenNonce,
                      const uint8_t * clientToServerKey,
                      const uint8_t * serverToClientKey );

        bool IsConnecting() const
        {
            return m_clientState == CLIENT_STATE_SENDING_CONNECTION_REQUEST || m_clientState == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE;
        }

        bool IsConnected() const
        {
            return m_clientState == CLIENT_STATE_CONNECTED;
        }

        bool ConnectionFailed() const
        {
            return m_clientState < CLIENT_STATE_DISCONNECTED;
        }

        void Disconnect( double time, int clientState = CLIENT_STATE_DISCONNECTED );

        void SendPackets( double time );

        void ReceivePackets( double time );

        void CheckForTimeOut( double time );

    protected:

        virtual void OnConnect( const Address & /*address*/ ) {}

        virtual void OnClientStateChange( int /*previousState*/, int /*currentState*/ ) {}

        virtual void OnDisconnect() {};

        virtual void OnPacketSent( int /*packetType*/, const Address & /*to*/ ) {}

        virtual void OnPacketReceived( int /*packetType*/, const Address & /*from*/ ) {}

    protected:

        void SetClientState( int clientState );

        void ResetConnectionData();

        void SendPacketToServer( Packet *packet, double time );

        void ProcessConnectionDenied( const ConnectionDeniedPacket & /*packet*/, const Address & address, double /*time*/ );

        void ProcessConnectionChallenge( const ConnectionChallengePacket & packet, const Address & address, double time );

        void ProcessConnectionHeartBeat( const ConnectionHeartBeatPacket & /*packet*/, const Address & address, double time );

        void ProcessConnectionDisconnect( const ConnectionDisconnectPacket & /*packet*/, const Address & address, double time );
    };
}

#endif // #ifndef YOJIMBO_CLIENT_SERVER_H
