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
#if YOJIMBO_INSECURE_CONNECT
        CLIENT_STATE_INSECURE_CONNECT_TIMEOUT = -9,
#endif // #if YOJIMBO_INSECURE_CONNECT
        CLIENT_STATE_PACKET_FACTORY_ERROR = -8,
        CLIENT_STATE_MESSAGE_FACTORY_ERROR = -7,
        CLIENT_STATE_ALLOCATOR_ERROR = -6,
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

        void SendPackets();

        void ReceivePackets();

        void CheckForTimeOut();

        void AdvanceTime( double time );

        double GetTime() const;

        int GetClientIndex() const;

        Allocator & GetClientAllocator();

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

        virtual void ShutdownConnection();

        virtual void SetEncryptedPacketTypes();

        virtual PacketFactory * CreatePacketFactory( Allocator & allocator );

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator );

        virtual ClientServerContext * CreateContext( Allocator & allocator );

        void SetClientState( int clientState );

        void ResetConnectionData( int clientState = CLIENT_STATE_DISCONNECTED );

        void SendPacketToServer( Packet * packet );

    private:

        void SendPacketToServer_Internal( Packet * packet, bool immediate = false );

    protected:

        void ProcessConnectionDenied( const ConnectionDeniedPacket & packet, const Address & address );

        void ProcessChallenge( const ChallengePacket & packet, const Address & address );

        void ProcessHeartBeat( const HeartBeatPacket & packet, const Address & address );

        void ProcessDisconnect( const DisconnectPacket & packet, const Address & address );

        void ProcessConnectionPacket( ConnectionPacket & packet, const Address & address );

        void ProcessPacket( Packet * packet, const Address & address, uint64_t sequence );

        bool IsPendingConnect();

        void CompletePendingConnect( int clientIndex );

    protected:

        void Defaults();

        virtual void CreateAllocator();

        virtual void DestroyAllocator();

        ClientServerConfig m_config;                                        // client/server configuration.

        Allocator * m_allocator;                                            // the allocator passed in to the client on creation.

        uint8_t * m_clientMemory;                                           // memory backing the client allocator.

        Allocator * m_clientAllocator;                                      // client allocator. everything after connect is allocated via this allocated.

        PacketFactory * m_packetFactory;                                    // packet factory for creating and destroying messages. created via CreatePacketFactory.

        MessageFactory * m_messageFactory;                                  // message factory for creating and destroying messages. optional.

        bool m_allocateConnection;                                          // true if we should allocate a connection.

        Connection * m_connection;                                          // the connection object for exchanging messages with the server. optional.

        int m_clientIndex;                                                  // the client index on the server [0,maxClients-1]. -1 if not connected.

        ClientServerContext * m_context;                                    // serialization context for client/server packets.

        ClientState m_clientState;                                          // current client state

        uint64_t m_connectTokenExpireTimestamp;                             // expire timestamp for connect token used in secure connect.

        Address m_serverAddress;                                            // server address we are connecting or connected to.

        double m_lastPacketSendTime;                                        // time we last sent a packet to the server.

        double m_lastPacketReceiveTime;                                     // time we last received a packet from the server (used for timeouts).

        Transport * m_transport;                                            // transport for sending and receiving packets

        bool m_shouldDisconnect;                                            // set to true when we receive a disconnect packet from the server

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
