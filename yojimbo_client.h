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

#ifndef YOJIMBO_CLIENT_H
#define YOJIMBO_CLIENT_H

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
    /**
        The set of state in the client state machine.

        A client starts in the default state CLIENT_STATE_DISCONNECTED (0). 

        Positive states lower than CLIENT_STATE_CONNECTED represent states where the client is negotiating connection to a server following a call to Client::Connect or Client::InsecureConnect. While in these states, Client::IsConnecting returns true.
        
        When a client state connects to a server it transitions to CLIENT_STATE_CONNECTED. While in this state, Client::IsConnected returns true and Client::IsConnecting returns false.

        Negative states correspond to client-side errors. There is one client state per error condition that the client can experience. If one of these errors occur, the client disconnects and transitions to that error state. When in one of these error state, Client::IsConnected returns false and Client::ConnectionFailed returns true.
        
        @see Client::GetClientState
        @see Client::IsConnecting
        @see Client::IsConnected
        @see Client::ConnectedFailed
     */

    enum ClientState
    {
#if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_INSECURE_CONNECT_TIMEOUT = -9,                             ///< The client tried to connect to a server via Client::InsecureConnect, but the connection timed out.
#endif // #if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_PACKET_FACTORY_ERROR = -8,                                 ///< The client disconnected because the packet factory got into an error state.
        CLIENT_STATE_MESSAGE_FACTORY_ERROR = -7,                                ///< The client disconnected because the message factory got into an error state.
        CLIENT_STATE_ALLOCATOR_ERROR = -6,                                      ///< The client disconnected because the allocator got into an error state. This happens when the allocator runs out of memory.
        CLIENT_STATE_CONNECTION_ERROR = -5,                                     ///< The client disconnected because the connection got into an error state. This happens when something goes wrong when reading and writing messages.
        CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT = -4,                           ///< The client timed out while sending connection request packets to the server.
        CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT = -3,                           ///< The client timed out while sending challenge response packets to the server.
        CLIENT_STATE_CONNECTION_TIMEOUT = -2,                                   ///< The client timed out after successfully connecting to the server.
        CLIENT_STATE_CONNECTION_DENIED = -1,                                    ///< The server denied the connection request. This happens when the server is full.
        CLIENT_STATE_DISCONNECTED = 0,                                          ///< The client is disconnected. This is the default state. Also, this is the state the client transitions to if it cleanly disconnects from the server (client side), or via disconnect packet sent from the server.
#if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_SENDING_INSECURE_CONNECT,                                  ///< The client is sending insecure connect packets to the server. This state immediately follows Client::InsecureConnect and transitions directly to CLIENT_STATE_CONNECTED when an insecure connection is established.
#endif // #if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_SENDING_CONNECTION_REQUEST,                                ///< The client is sending connection request packets to the server. This state immediately follows Client::Connect. It transitions to CLIENT_STATE_SENDING_CHALLENGE_RESPONSE and then to CLIENT_STATE_CONNECTED.
        CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,                                ///< The client is sending challenge response packets to the server. Challenge/response during connect filters out clients trying to connection with a spoofed packet source address.
        CLIENT_STATE_CONNECTED                                                  ///< The client is connected to the server.
    };

    /** 
        Helper function to convert client state enum value to a string. Useful for logging and debugging.
        @param clientState The client state to convert to a string.
        @returns The client state in a user friendly string, eg. "connected".
     */

    inline const char * GetClientStateName( ClientState clientState )
    {
        switch ( clientState )
        {
#if !YOJIMBO_SECURE_MODE
            case CLIENT_STATE_INSECURE_CONNECT_TIMEOUT:         return "insecure connect timeout";
#endif // #if !YOJIMBO_SECURE_MODE
            case CLIENT_STATE_PACKET_FACTORY_ERROR:             return "packet factory error";
            case CLIENT_STATE_MESSAGE_FACTORY_ERROR:            return "message factory error";
            case CLIENT_STATE_ALLOCATOR_ERROR:                  return "allocator error";
            case CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT:       return "connection request timeout";
            case CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT:       return "challenge response timeout";
            case CLIENT_STATE_CONNECTION_TIMEOUT:               return "connection timeout";
            case CLIENT_STATE_CONNECTION_ERROR:                 return "connection error";
            case CLIENT_STATE_CONNECTION_DENIED:                return "connection denied";
            case CLIENT_STATE_DISCONNECTED:                     return "disconnected";
#if !YOJIMBO_SECURE_MODE
            case CLIENT_STATE_SENDING_INSECURE_CONNECT:         return "sending insecure connect";
#endif // #if !YOJIMBO_SECURE_MODE
            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:       return "sending connection request";
            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:       return "sending challenge response";
            case CLIENT_STATE_CONNECTED:                        return "connected";
            default:
                assert( false );
                return "???";
        }
    }

    /// Client counters. These are useful for debugging and telemetry.

    enum ClientCounters
    {
        CLIENT_COUNTER_TODO,                                                    ///< TODO: Add some client counters.

        NUM_CLIENT_COUNTERS                                                     
    };

    /** 
        A client that connects to a server.

        This class is designed to be inherited from to create your own client class.
     */

    class Client : public ConnectionListener
    {
    public:

        /**
            The client constructor.

            @param allocator The allocator for all memory used by the client.
            @param transport The transport for sending and receiving packets.
            @param config The client/server configuration.
            @param time The current time in seconds. See Client::AdvanceTime
         */

        explicit Client( Allocator & allocator, Transport & transport, const ClientServerConfig & config, double time );

        /**
            The client destructor.

            IMPORTANT: Please call Client::Disconnect before destroying the client. This is necessary because Disconnect is virtual and calling virtual methods from destructors does not give the expected behavior when you override that method.
         */

        virtual ~Client();

#if !YOJIMBO_SECURE_MODE

        /** 
            Connect to a server (insecure).

            IMPORTANT: Insecure connections are not encrypted and do not provide authentication. 

            They are provided for convienence in development only, and should not be used in production code!

            You can completely disable insecure connections in your retail build by defining YOJIMBO_SECURE_MODE 1

            @param clientId The client id. Typically used to identify clients when your server talks to a web backend. If you don't have your own concept of unique client id, roll a random 64bit integer.
            @param serverAddress The address of the server to connect to.
         */

        void InsecureConnect( uint64_t clientId, const Address & serverAddress );
        
        /** 
            Connect to a list of servers (insecure).

            The client tries to connect to each server in the list, in turn, until one of the servers is connected to, or it reaches the end of the server address list.

            IMPORTANT: Insecure connections are not encrypted and do not provide authentication. 

            They are provided for convienence in development only, and should not be used in production code!

            You can completely disable insecure connections in your retail build by defining YOJIMBO_SECURE_MODE 1

            @param clientId The client id. Typically used to identify clients when your server talks to a web backend. If you don't have your own concept of unique client id, roll a random 64bit integer.
            @param serverAddresses The list of server addresses to connect to, in order of first to last.
            @param numServerAddresses Number of server addresses in [1,yojimbo::MaxServersPerConnect].
         */

        void InsecureConnect( uint64_t clientId, const Address serverAddresses[], int numServerAddresses );

#endif // #if !YOJIMBO_SECURE_MODE

        /** 
            Connect to a server (secure).

            This function takes a connect token generated by matcher.go and passes it to the server to establish a secure connection.

            Secure connections are encrypted and authenticated. If the server runs in secure mode, it will only accept connections from clients with a connect token, thus stopping unauthenticated clients from connecting to your server.

            @param clientId The client id. Typically used to identify clients when your server talks to a web backend. If you don't have your own concept of unique client id, roll a random 64bit integer.
            @param serverAddress The address of the server to connect to.
            @param connectTokenData Pointer to the connect token data from the matcher.
            @param connectTokenNonce Pointer to the connect token nonce from the matcher.
            @param clientToServerKey The encryption key for client to server packets.
            @param clientToServerKey The encryption key for server to client packets.
            @param connectTokenExpireTimestamp The timestamp for when the connect token expires. Used by the server to quickly reject stale connect tokens without decrypting them.
         */

        void Connect( uint64_t clientId,
                      const Address & serverAddress, 
                      const uint8_t * connectTokenData, 
                      const uint8_t * connectTokenNonce,
                      const uint8_t * clientToServerKey,
                      const uint8_t * serverToClientKey,
                      uint64_t connectTokenExpireTimestamp );

        /** 
            Connect to a list of servers (secure).

            The client tries to connect to each server in the list, in turn, until one of the servers is connected to, or it reaches the end of the server address list.

            This is designed for use with the matcher, which can return a list of up to yojimbo::MaxServersPerConnect with the connect token, to work around race conditions where the server sends multiple clients to fill the same slot on a server.

            Secure connections are encrypted and authenticated. If the server runs in secure mode, it will only accept connections from clients with a connect token, thus stopping unauthenticated from connecting to your server.

            @param clientId The client id. Typically used to identify clients when your server talks to a web backend. If you don't have your own concept of unique client id, roll a random 64bit integer.
            @param serverAddresses The list of server addresses to connect to, in order of first to last.
            @param numServerAddresses Number of server addresses in [1,yojimbo::MaxServersPerConnect].
            @param connectTokenData Pointer to the connect token data from the matcher.
            @param connectTokenNonce Pointer to the connect token nonce from the matcher.
            @param clientToServerKey The encryption key for client to server packets.
            @param clientToServerKey The encryption key for server to client packets.
            @param connectTokenExpireTimestamp The timestamp for when the connect token expires. Used by the server to quickly reject stale connect tokens without decrypting them.
         */

        void Connect( uint64_t clientId, 
                      const Address serverAddresses[], int numServerAddresses,
                      const uint8_t * connectTokenData, 
                      const uint8_t * connectTokenNonce,
                      const uint8_t * clientToServerKey,
                      const uint8_t * serverToClientKey,
                      uint64_t connectTokenExpireTimestamp );

        bool IsConnecting() const;

        bool IsConnected() const;

        bool IsDisconnected() const;

        bool ConnectionFailed() const;

        ClientState GetClientState() const;

        void Disconnect( ClientState clientState = CLIENT_STATE_DISCONNECTED, bool sendDisconnectPacket = true );

        Message * CreateMsg( int type );

        bool CanSendMsg( int channelId = 0 );

        void SendMsg( Message * message, int channelId = 0 );

        Message * ReceiveMsg( int channelId = 0 );

        void ReleaseMsg( Message * message );

        MessageFactory & GetMsgFactory();

        Packet * CreatePacket( int type );

        void SendPackets();

        void ReceivePackets();

        void CheckForTimeOut();

        void AdvanceTime( double time );

        double GetTime() const;

        uint64_t GetClientId() const;

        int GetClientIndex() const;

        uint64_t GetCounter( int index ) const;

        Allocator & GetClientAllocator();

        void SetUserContext( void * context );

    protected:

        virtual void OnConnect( const Address & /*address*/ ) {}

        virtual void OnClientStateChange( ClientState /*previousState*/, ClientState /*currentState*/ ) {}

        virtual void OnDisconnect() {}

        virtual void OnPacketSent( int /*packetType*/, const Address & /*to*/, bool /*immediate*/ ) {}

        virtual void OnPacketReceived( int /*packetType*/, const Address & /*from*/ ) {}

        virtual void OnConnectionPacketSent( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketAcked( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketReceived( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionFragmentReceived( Connection * /*connection*/, int /*channelId*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/, int /*numFragmentsReceived*/, int /*numFragmentsInBlock*/ ) {}

        virtual bool ProcessUserPacket( Packet * /*packet*/ ) { return false; }

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

#define YOJIMBO_CLIENT_ALLOCATOR( allocator_class )                                                     \
    Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes )                   \
    {                                                                                                   \
        return YOJIMBO_NEW( allocator, allocator_class, memory, bytes );                                \
    }

#define YOJIMBO_CLIENT_PACKET_FACTORY( packet_factory_class )                                           \
    PacketFactory * CreatePacketFactory( Allocator & allocator )                                        \
    {                                                                                                   \
        return YOJIMBO_NEW( allocator, packet_factory_class, allocator );                               \
    }

#define YOJIMBO_CLIENT_MESSAGE_FACTORY( message_factory_class )                                         \
    MessageFactory * CreateMessageFactory( Allocator & allocator )                                      \
    {                                                                                                   \
        return YOJIMBO_NEW( allocator, message_factory_class, allocator );                              \
    }

#endif // #ifndef YOJIMBO_CLIENT_H
