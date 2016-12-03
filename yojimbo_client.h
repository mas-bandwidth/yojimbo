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
    enum ClientState
    {
#if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_INSECURE_CONNECT_TIMEOUT = -9,
#endif // #if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_PACKET_FACTORY_ERROR = -8,
        CLIENT_STATE_MESSAGE_FACTORY_ERROR = -7,
        CLIENT_STATE_ALLOCATOR_ERROR = -6,
        CLIENT_STATE_CONNECTION_ERROR = -5,
        CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT = -4,
        CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT = -3,
        CLIENT_STATE_CONNECTION_TIMEOUT = -2,
        CLIENT_STATE_CONNECTION_DENIED = -1,
        CLIENT_STATE_DISCONNECTED = 0,
#if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_SENDING_INSECURE_CONNECT,
#endif // #if !YOJIMBO_SECURE_MODE
        CLIENT_STATE_SENDING_CONNECTION_REQUEST,
        CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,
        CLIENT_STATE_CONNECTED
    };

    inline const char * GetClientStateName( int clientState )
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

    enum ClientCounters
    {
        CLIENT_COUNTER_TODO,
        
        NUM_CLIENT_COUNTERS
    };

    const char * GetClientStateName( int clientState );

    /** 
        A client that connects to a server.

        This class is designed to be inherited from to create your own client class.
     */

    class Client : public ConnectionListener
    {
    public:

        explicit Client( Allocator & allocator, Transport & transport, const ClientServerConfig & config, double time );

        virtual ~Client();

#if !YOJIMBO_SECURE_MODE
        void InsecureConnect( uint64_t clientId, const Address & serverAddress );
        void InsecureConnect( uint64_t clientId, const Address serverAddresses[], int numServerAddresses );
#endif // #if !YOJIMBO_SECURE_MODE

        void Connect( uint64_t clientId,
                      const Address & serverAddress, 
                      const uint8_t * connectTokenData, 
                      const uint8_t * connectTokenNonce,
                      const uint8_t * clientToServerKey,
                      const uint8_t * serverToClientKey,
                      uint64_t connectTokenExpireTimestamp );

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

        void Disconnect( int clientState = CLIENT_STATE_DISCONNECTED, bool sendDisconnectPacket = true );

        Message * CreateMsg( int type );

        bool CanSendMsg();

        void SendMsg( Message * message );

        Message * ReceiveMsg();

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

        virtual void OnClientStateChange( int /*previousState*/, int /*currentState*/ ) {}

        virtual void OnDisconnect() {}

        virtual void OnPacketSent( int /*packetType*/, const Address & /*to*/, bool /*immediate*/ ) {}

        virtual void OnPacketReceived( int /*packetType*/, const Address & /*from*/ ) {}

        virtual void OnConnectionPacketSent( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketAcked( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionPacketReceived( Connection * /*connection*/, uint16_t /*sequence*/ ) {}

        virtual void OnConnectionFragmentReceived( Connection * /*connection*/, uint16_t /*messageId*/, uint16_t /*fragmentId*/, int /*fragmentBytes*/, int /*channelId*/ ) {}

        virtual bool ProcessUserPacket( Packet * /*packet*/ ) { return false; }

    protected:

        virtual void InitializeConnection( uint64_t clientId );

        virtual void ShutdownConnection();

        virtual void SetEncryptedPacketTypes();

        virtual PacketFactory * CreatePacketFactory( Allocator & allocator );

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator );

        virtual void SetClientState( int clientState );

        virtual void ResetConnectionData( int clientState = CLIENT_STATE_DISCONNECTED );

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

        ClientServerConfig m_config;                                        // client/server configuration.

        Allocator * m_allocator;                                            // the allocator passed in to the client on creation.

        uint8_t * m_clientMemory;                                           // memory backing the client allocator.

        Allocator * m_clientAllocator;                                      // client allocator. everything after connect is allocated via this allocated.

        PacketFactory * m_packetFactory;                                    // packet factory for creating and destroying messages. created via CreatePacketFactory.

        MessageFactory * m_messageFactory;                                  // message factory for creating and destroying messages. optional.

        ReplayProtection * m_replayProtection;                              // replay protection eliminates old and duplicate packets. protects against replay attacks.

        bool m_allocateConnection;                                          // true if we should allocate a connection.

        Connection * m_connection;                                          // the connection object for exchanging messages with the server. optional.

        uint64_t m_clientId;                                                // the globally unique client id (set on each call to connect).

        int m_clientIndex;                                                  // the client index on the server [0,maxClients-1]. -1 if not connected.

        TransportContext m_transportContext;                                // transport context for reading and writing packets.

        ConnectionContext m_connectionContext;                              // connection context for serializing connection packets.

        ClientState m_clientState;                                          // current client state

        uint64_t m_connectTokenExpireTimestamp;                             // expire timestamp for connect token used in secure connect.

        int m_serverAddressIndex;                                           // current index in the server address array. this is the server we are currently connecting to.

        int m_numServerAddresses;                                           // number of server addresses in the array.

        Address m_serverAddresses[MaxServersPerConnect];                    // server addresses we are connecting or connected to.

        Address m_serverAddress;                                            // the current server address we are connecting to.

        double m_lastPacketSendTime;                                        // time we last sent a packet to the server.

        double m_lastPacketReceiveTime;                                     // time we last received a packet from the server (used for timeouts).

        Transport * m_transport;                                            // transport for sending and receiving packets

        bool m_shouldDisconnect;                                            // set to true when we receive a disconnect packet from the server

        ClientState m_shouldDisconnectState;                                // client state to set on disconnect via m_shouldDisconnect

        double m_time;                                                      // current client time (see "AdvanceTime")

#if !YOJIMBO_SECURE_MODE
        uint64_t m_clientSalt;                                              // client salt for insecure connect
#endif // #if !YOJIMBO_SECURE_MODE

        uint64_t m_sequence;                                                // packet sequence # for packets sent to the server

        uint64_t m_counters[NUM_CLIENT_COUNTERS];                           // counters to aid with debugging and stats

        uint8_t m_connectTokenData[ConnectTokenBytes];                      // encrypted connect token data for connection request packet

        uint8_t m_connectTokenNonce[NonceBytes];                            // nonce required to send to server so it can decrypt connect token

        uint8_t m_challengeTokenData[ChallengeTokenBytes];                  // encrypted challenge token data for challenge response packet

        uint8_t m_challengeTokenNonce[NonceBytes];                          // nonce required to send to server so it can decrypt challenge token

        uint8_t m_clientToServerKey[KeyBytes];                              // client to server encryption key

        uint8_t m_serverToClientKey[KeyBytes];                              // server to client encryption key

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
