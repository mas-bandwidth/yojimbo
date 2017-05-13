/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.

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

#include "yojimbo_config.h"
#include "yojimbo_client.h"
#include "netcode.io/c/netcode.h"
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

namespace yojimbo
{
    // ------------------------------------------------------------------------------------------------------------------

    BaseClient::BaseClient( Allocator & allocator, const BaseClientServerConfig & config, double time )
    {
        m_allocator = &allocator;
        m_config = config;
        m_time = time;
        m_allocator = NULL;
        m_context = NULL;
        m_clientMemory = NULL;
        m_clientAllocator = NULL;
        m_clientIndex = -1;
        m_clientState = CLIENT_STATE_DISCONNECTED;
    }

    BaseClient::~BaseClient()
    {
        // IMPORTANT: You must disconnect the client before destroying it!
        assert( m_clientState <= CLIENT_STATE_DISCONNECTED );
    }

    void BaseClient::Disconnect()
    {
        // todo
    }

    void BaseClient::SendPackets()
    {
        // todo
    }

    void BaseClient::ReceivePackets()
    {
        // todo
    }

    void BaseClient::AdvanceTime( double time )
    {
        m_time = time;
    }

    void BaseClient::CreateAllocators()
    {
        assert( m_allocator );
        assert( m_clientMemory == NULL );
        m_clientMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.clientMemory );
        m_clientAllocator = YOJIMBO_NEW( *m_allocator, TLSF_Allocator, m_clientMemory, m_config.clientMemory );
    }

    void BaseClient::DestroyAllocators()
    {
        assert( m_allocator );
        YOJIMBO_DELETE( *m_allocator, Allocator, m_clientAllocator );
        YOJIMBO_FREE( *m_allocator, m_clientMemory );
    }

    Allocator * BaseClient::CreateAllocator( Allocator & allocator, void * memory, size_t bytes )
    {
        return YOJIMBO_NEW( allocator, TLSF_Allocator, memory, bytes );
    }

    // ------------------------------------------------------------------------------------------------------------------

    Client::Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, double time ) : BaseClient( allocator, config, time )
    {
        m_address = address;
        m_client = NULL;
    }

    Client::~Client()
    {
        // IMPORTANT: You must disconnect the client before destroying it!
        assert( m_client == NULL );
    }

#ifndef YOJIMBO_SECURE_MODE

    void Client::InsecureConnect( uint64_t clientId, const Address & address )
    {
        // todo
    }

    void Client::InsecureConnect( uint64_t clientId, const Address serverAddresses[], int numServerAddresses )
    {
        // todo
    }

#endif // #ifndef YOJIMBO_SECURE_MODE

    void Client::Connect( uint8_t * connectToken )
    {
        assert( connectToken );

        Disconnect();

        CreateClient( m_address );

        netcode_client_connect( m_client, connectToken );
    }

    void Client::CreateClient( const Address & address )
    {
        assert( m_client == NULL );
        char addressString[MaxAddressLength];
        address.ToString( addressString, MaxAddressLength );
        m_client = netcode_client_create( addressString, GetTime() );
    }

    void Client::DestroyClient()
    {
        assert( m_client != NULL );
        netcode_client_destroy( m_client );
        m_client = NULL;
    }

    // ------------------------------------------------------------------------------------------------------------------
}














#if 0 // todo

namespace yojimbo
{
    void Client::Defaults()
    {
        m_allocator = NULL;
        m_clientMemory = NULL;
        m_clientAllocator = NULL;
        m_transport = NULL;
        m_packetFactory = NULL;
        m_messageFactory = NULL;
        m_replayProtection = NULL;
        m_allocateConnection = false;
        m_connection = NULL;
        m_time = 0.0;
        m_clientState = CLIENT_STATE_DISCONNECTED;
        m_clientId = 0;
        m_clientIndex = -1;
        m_lastPacketSendTime = 0.0;
        m_lastPacketReceiveTime = 0.0;
#if !YOJIMBO_SECURE_MODE
        m_clientSalt = 0;
#endif // #if !YOJIMBO_SECURE_MODE
        m_sequence = 0;
        m_connectTokenExpireTimestamp = 0;
        m_shouldDisconnect = false;
        m_shouldDisconnectState = CLIENT_STATE_DISCONNECTED;
        m_serverAddressIndex = 0;
        m_numServerAddresses = 0;
        memset( m_counters, 0, sizeof( m_counters ) );
        memset( m_connectTokenData, 0, sizeof( m_connectTokenData ) );
        memset( m_connectTokenNonce, 0, sizeof( m_connectTokenNonce ) );
        memset( m_challengeTokenData, 0, sizeof( m_challengeTokenData ) );
        memset( m_challengeTokenNonce, 0, sizeof( m_challengeTokenNonce ) );
        memset( m_clientToServerKey, 0, sizeof( m_clientToServerKey ) );
        memset( m_serverToClientKey, 0, sizeof( m_serverToClientKey ) );
    }

    void Client::CreateAllocators()
    {
        assert( m_allocator );

        assert( m_clientMemory == NULL );

        m_clientMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.clientMemory );

        m_clientAllocator = YOJIMBO_NEW( *m_allocator, TLSF_Allocator, m_clientMemory, m_config.clientMemory );
    }

    void Client::DestroyAllocators()
    {
        assert( m_allocator );

        YOJIMBO_DELETE( *m_allocator, Allocator, m_clientAllocator );

        YOJIMBO_FREE( *m_allocator, m_clientMemory );
    }

    Allocator * Client::CreateAllocator( Allocator & allocator, void * memory, size_t bytes )
    {
        return YOJIMBO_NEW( allocator, TLSF_Allocator, memory, bytes );
    }

    Client::Client( Allocator & allocator, Transport & transport, const ClientServerConfig & config, double time )
    {
        Defaults();
        m_allocator = &allocator;
        m_transport = &transport;
        m_config = config;
        m_config.connectionConfig.connectionPacketType = CLIENT_SERVER_PACKET_CONNECTION;
        m_allocateConnection = m_config.enableMessages;
        m_userContext = NULL;
        m_time = time;
    }

    Client::~Client()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        assert( !IsConnected() );

        m_transport = NULL;
        m_allocator = NULL;
    }

    void Client::SetUserContext( void * context )
    {
        assert( m_clientState == CLIENT_STATE_DISCONNECTED );
        m_userContext = context;
    }

#if !YOJIMBO_SECURE_MODE

    void Client::InsecureConnect( uint64_t clientId, const Address & serverAddress )
    {
        InsecureConnect( clientId, &serverAddress, 1 );
    }

    void Client::InsecureConnect( uint64_t clientId, const Address serverAddresses[], int numServerAddresses )
    {
        assert( numServerAddresses >= 1 );
        assert( numServerAddresses <= MaxServersPerConnect );

        Disconnect();

        InitializeConnection( clientId );

        m_serverAddressIndex = 0;
        m_numServerAddresses = numServerAddresses;
        for ( int i = 0; i < numServerAddresses; ++i )
            m_serverAddresses[i] = serverAddresses[i];

        char addressString[MaxAddressLength];
        m_serverAddresses[m_serverAddressIndex].ToString( addressString, sizeof( addressString ) );
        debug_printf( "connect to insecure server: %s (%d/%d)\n", addressString, m_serverAddressIndex + 1, m_numServerAddresses );

        m_transport->DisablePacketEncryption();

        InternalInsecureConnect( serverAddresses[0] );
    }

#endif // #if !YOJIMBO_SECURE_MODE

    void Client::Connect( uint64_t clientId,
                          const Address & serverAddress,
                          const uint8_t * connectTokenData, 
                          const uint8_t * connectTokenNonce,
                          const uint8_t * clientToServerKey,
                          const uint8_t * serverToClientKey,
                          uint64_t connectTokenExpireTimestamp )
    {
        Connect( clientId, &serverAddress, 1, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, connectTokenExpireTimestamp );
    }

    void Client::Connect( uint64_t clientId, 
                          const Address serverAddresses[], int numServerAddresses,
                          const uint8_t * connectTokenData, 
                          const uint8_t * connectTokenNonce,
                          const uint8_t * clientToServerKey,
                          const uint8_t * serverToClientKey,
                          uint64_t connectTokenExpireTimestamp )
    {
        assert( numServerAddresses > 0 );
        assert( numServerAddresses <= MaxServersPerConnect );

        Disconnect();

        InitializeConnection( clientId );

        m_serverAddressIndex = 0;
        m_numServerAddresses = numServerAddresses;
        for ( int i = 0; i < numServerAddresses; ++i )
            m_serverAddresses[i] = serverAddresses[i];

        char addressString[MaxAddressLength];
        m_serverAddresses[m_serverAddressIndex].ToString( addressString, sizeof( addressString ) );
        debug_printf( "connect to secure server: %s (%d/%d)\n", addressString, m_serverAddressIndex + 1, m_numServerAddresses );

        memcpy( m_connectTokenData, connectTokenData, ConnectTokenBytes );
        memcpy( m_connectTokenNonce, connectTokenNonce, NonceBytes );
        memcpy( m_clientToServerKey, clientToServerKey, KeyBytes );
        memcpy( m_serverToClientKey, serverToClientKey, KeyBytes );

        m_connectTokenExpireTimestamp = connectTokenExpireTimestamp;

        SetEncryptedPacketTypes();

        InternalSecureConnect( m_serverAddresses[0] );
    }

    void Client::Disconnect( ClientState clientState, bool sendDisconnectPacket )
    {
        assert( clientState <= CLIENT_STATE_DISCONNECTED );

        if ( m_clientState <= CLIENT_STATE_DISCONNECTED || m_clientState == clientState )
            return;

        OnDisconnect();

        if ( sendDisconnectPacket && m_clientState > CLIENT_STATE_DISCONNECTED )
        {
            for ( int i = 0; i < m_config.numDisconnectPackets; ++i )
            {
                DisconnectPacket * packet = (DisconnectPacket*) CreatePacket( CLIENT_SERVER_PACKET_DISCONNECT );            

                if ( packet )
                {
                    SendPacketToServer_Internal( packet, true );
                }
            }
        }

        ResetConnectionData( clientState );

        ShutdownConnection();
    }

    Message * Client::CreateMsg( int type )
    {
        assert( m_messageFactory );
        return m_messageFactory->Create( type );
    }

    bool Client::CanSendMsg( int channelId )
    {
        if ( !IsConnected() )
            return false;

        assert( m_messageFactory );
        assert( m_connection );
        
        return m_connection->CanSendMsg( channelId );
    }

    void Client::SendMsg( Message * message, int channelId )
    {
        assert( IsConnected() );
        assert( m_messageFactory );
        assert( m_connection );
        m_connection->SendMsg( message, channelId );
    }

    Message * Client::ReceiveMsg( int channelId )
    {
        assert( m_messageFactory );

        if ( !IsConnected() )
            return NULL;

        assert( m_connection );

        return m_connection->ReceiveMsg( channelId );
    }

    void Client::ReleaseMsg( Message * message )
    {
        assert( message );
        assert( m_messageFactory );
        m_messageFactory->Release( message );
    }

    MessageFactory & Client::GetMsgFactory()
    {
        assert( m_messageFactory );
        return *m_messageFactory;
    }

    bool Client::IsConnecting() const
    {
        return m_clientState > CLIENT_STATE_DISCONNECTED && m_clientState < CLIENT_STATE_CONNECTED;
    }

    bool Client::IsConnected() const
    {
        return m_clientState == CLIENT_STATE_CONNECTED;
    }

    bool Client::IsDisconnected() const
    {
        return m_clientState <= CLIENT_STATE_DISCONNECTED;
    }

    bool Client::ConnectionFailed() const
    {
        return m_clientState < CLIENT_STATE_DISCONNECTED;
    }

    ClientState Client::GetClientState() const
    { 
        return m_clientState;
    }

    Packet * Client::CreatePacket( int type )
    {
        assert( m_packetFactory );
        return m_packetFactory ? m_packetFactory->Create( type ) : NULL;
    }

    void Client::SendPackets()
    {
        const double time = GetTime();

        switch ( m_clientState )
        {
#if !YOJIMBO_SECURE_MODE

            case CLIENT_STATE_SENDING_INSECURE_CONNECT:
            {
                if ( m_lastPacketSendTime + ( 1.0f / m_config.connectionNegotiationSendRate ) > time )
                    return;

                InsecureConnectPacket * packet = (InsecureConnectPacket*) CreatePacket( CLIENT_SERVER_PACKET_INSECURE_CONNECT );
                if ( packet )
                {
                    packet->clientId = m_clientId;
                    packet->clientSalt = m_clientSalt;
                    SendPacketToServer_Internal( packet );
                }
            }
            break;

#endif // #if !YOJIMBO_SECURE_MODE

            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
            {
                if ( m_lastPacketSendTime + ( 1.0f / m_config.connectionNegotiationSendRate ) > time )
                    return;

                ConnectionRequestPacket * packet = (ConnectionRequestPacket*) CreatePacket( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );

                if ( packet )
                {
                    packet->connectTokenExpireTimestamp = m_connectTokenExpireTimestamp;
                    memcpy( packet->connectTokenData, m_connectTokenData, ConnectTokenBytes );
                    memcpy( packet->connectTokenNonce, m_connectTokenNonce, NonceBytes );

                    SendPacketToServer_Internal( packet );
                }
            }
            break;

            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
            {
                if ( m_lastPacketSendTime + ( 1.0f / m_config.connectionNegotiationSendRate ) > time )
                    return;

                ChallengeResponsePacket * packet = (ChallengeResponsePacket*) CreatePacket( CLIENT_SERVER_PACKET_CHALLENGE_RESPONSE );
                
                if ( packet )
                {
                    memcpy( packet->challengeTokenData, m_challengeTokenData, ChallengeTokenBytes );
                    memcpy( packet->challengeTokenNonce, m_challengeTokenNonce, NonceBytes );
                    
                    SendPacketToServer_Internal( packet );
                }
            }
            break;

            case CLIENT_STATE_CONNECTED:
            {
                if ( m_connection )
                {
                    ConnectionPacket * packet = m_connection->GeneratePacket();

                    if ( packet )
                    {
                        SendPacketToServer( packet );
                    }
                }

                if ( m_lastPacketSendTime + ( 1.0f / m_config.connectionKeepAliveSendRate ) <= time )
                {
                    KeepAlivePacket * packet = (KeepAlivePacket*) CreatePacket( CLIENT_SERVER_PACKET_KEEPALIVE );

                    if ( packet )
                    {
                        SendPacketToServer( packet );
                    }
                }
            }
            break;

            default:
                break;
        }
    }

    void Client::ReceivePackets()
    {
        while ( true )
        {
            Address address;
            uint64_t sequence;
            Packet * packet = m_transport->ReceivePacket( address, &sequence );
            if ( !packet )
                break;

            ProcessPacket( packet, address, sequence );

            packet->Destroy();
        }
    }

    void Client::CheckForTimeOut()
    {
        const double time = GetTime();

        if ( m_shouldDisconnect )
        {
            debug_printf( "m_shouldDisconnect -> %s\n", GetClientStateName( m_shouldDisconnectState ) );
            if ( ConnectToNextServer() )
                return;
            Disconnect( m_shouldDisconnectState, false );
            return;
        }

        switch ( m_clientState )
        {
#if !YOJIMBO_SECURE_MODE

            case CLIENT_STATE_SENDING_INSECURE_CONNECT:
            {
                if ( m_lastPacketReceiveTime + m_config.connectionNegotiationTimeOut < time )
                {
                    debug_printf( "insecure connect timed out\n" );
                    if ( ConnectToNextServer() )
                        return;
                    Disconnect( CLIENT_STATE_INSECURE_CONNECT_TIMEOUT, false );
                    return;
                }
            }
            break;

#endif // #if !YOJIMBO_SECURE_MODE

            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
            {
                if ( m_lastPacketReceiveTime + m_config.connectionNegotiationTimeOut < time )
                {
                    debug_printf( "connection request timed out\n" );
                    if ( ConnectToNextServer() )
                        return;
                    Disconnect( CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT, false );
                    return;
                }
            }
            break;

            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
            {
                if ( m_lastPacketReceiveTime + m_config.connectionNegotiationTimeOut < time )
                {
                    debug_printf( "challenge response timed out\n" );
                    if ( ConnectToNextServer() )
                        return;
                    Disconnect( CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT, false );
                    return;
                }
            }
            break;

            case CLIENT_STATE_CONNECTED:
            {
                if ( m_lastPacketReceiveTime + m_config.connectionTimeOut < time )
                {
                    debug_printf( "connection timed out (%f<%f)\n", m_lastPacketReceiveTime, time );
                    Disconnect( CLIENT_STATE_CONNECTION_TIMEOUT, false );
                    return;
                }
            }
            break;

            default:
                break;
        }
    }

    void Client::AdvanceTime( double time )
    {
        m_time = time;

        if ( m_clientAllocator && m_clientAllocator->GetError() )
        {
            Disconnect( CLIENT_STATE_ALLOCATOR_ERROR, true );
            return;
        }

        if ( m_messageFactory && m_messageFactory->GetError() )
        {
            Disconnect( CLIENT_STATE_MESSAGE_FACTORY_ERROR, true );
            return;
        }

        if ( m_packetFactory && m_packetFactory->GetError() )
        {
            Disconnect( CLIENT_STATE_PACKET_FACTORY_ERROR, true );
            return;
        }

        if ( m_connection )
        {
            if ( m_connection->GetError() )
            {
                Disconnect( CLIENT_STATE_CONNECTION_ERROR, true );
                return;
            }

            m_connection->AdvanceTime( time );
        }
    }

    double Client::GetTime() const
    {
        return m_time;
    }

    uint64_t Client::GetClientId() const
    {
        return m_clientId;
    }

    int Client::GetClientIndex() const
    {
        return m_clientIndex;
    }

    uint64_t Client::GetCounter( int index ) const 
    {
        assert( index >= 0 );
        assert( index < NUM_CLIENT_COUNTERS );
        return m_counters[index];
    }

    Allocator & Client::GetClientAllocator()
    {
        assert( m_clientAllocator );
        return *m_clientAllocator;
    }

    void Client::InitializeConnection( uint64_t clientId )
    {
        debug_printf( "Client::InitializeConnection (%p), clientId = %" PRIx64 "\n", this, clientId );

        m_clientId = clientId;

        m_lastPacketSendTime = m_time - 1.0f;
        m_lastPacketReceiveTime = m_time;

        CreateAllocators();

        assert( m_clientAllocator );

        assert( !m_packetFactory );

        m_packetFactory = CreatePacketFactory( *m_clientAllocator );

        assert( m_packetFactory );

        m_replayProtection = YOJIMBO_NEW( *m_clientAllocator, ReplayProtection );

        if ( m_config.enableMessages )
        {
            if ( m_allocateConnection )
            {
                assert( !m_connection );
                assert( !m_messageFactory );

                m_messageFactory = CreateMessageFactory( *m_clientAllocator );
                
                assert( m_messageFactory );
                
                m_connection = YOJIMBO_NEW( *m_clientAllocator, Connection, *m_clientAllocator, *m_packetFactory, *m_messageFactory, m_config.connectionConfig );
                
                assert( m_connection );

                m_connection->SetListener( this );
            }
        }

        m_transportContext = TransportContext();

        m_transportContext.allocator = m_clientAllocator;
        m_transportContext.userContext = m_userContext;
        m_transportContext.packetFactory = m_packetFactory;
        m_transportContext.replayProtection = m_replayProtection;
        m_transportContext.encryptionIndex = m_transport->FindEncryptionMapping( m_serverAddress );

        if ( m_allocateConnection )
        {
            m_connectionContext.messageFactory = m_messageFactory;
            m_connectionContext.connectionConfig = &m_config.connectionConfig;
            m_transportContext.connectionContext = &m_connectionContext;
        }

        m_transport->SetContext( m_transportContext );
    }

    void Client::ShutdownConnection()
    {
        debug_printf( "Client::ShutdownConnection (%p)\n", this );

        m_transport->ClearContext();

        m_transport->Reset();

        YOJIMBO_DELETE( *m_clientAllocator, Connection, m_connection );

        YOJIMBO_DELETE( *m_clientAllocator, PacketFactory, m_packetFactory );

        YOJIMBO_DELETE( *m_clientAllocator, MessageFactory, m_messageFactory );

        YOJIMBO_DELETE( *m_clientAllocator, ReplayProtection, m_replayProtection );

        DestroyAllocators();
    }

    void Client::SetEncryptedPacketTypes()
    {
        m_transport->EnablePacketEncryption();

        m_transport->DisableEncryptionForPacketType( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );
    }

    PacketFactory * Client::CreatePacketFactory( Allocator & allocator )
    {
        return YOJIMBO_NEW( allocator, ClientServerPacketFactory, allocator );
    }

    MessageFactory * Client::CreateMessageFactory( Allocator & /*allocator*/ )
    {
        assert( !"you need to override Client::CreateMessageFactory if you want to use messages" );
        return NULL;
    }    

    void Client::SetClientState( ClientState clientState )
    {
        const ClientState previous = m_clientState;

        m_clientState = (ClientState) clientState;

        if ( clientState != previous )
        {
            OnClientStateChange( previous, clientState );
        }
    }

    void Client::ResetConnectionData( ClientState clientState )
    {
        assert( m_transport );

        m_clientId = 0;
        m_clientIndex = -1;
        m_serverAddress = Address();
        m_serverAddressIndex = 0;
        m_numServerAddresses = 0;

        SetClientState( clientState );

        m_lastPacketSendTime = -1000.0;
        m_lastPacketReceiveTime = -1000.0;

        memset( m_connectTokenData, 0, ConnectTokenBytes );
        memset( m_connectTokenNonce, 0, NonceBytes );
        memset( m_challengeTokenData, 0, ChallengeTokenBytes );
        memset( m_challengeTokenNonce, 0, NonceBytes );

#if !YOJIMBO_SECURE_MODE
        m_clientSalt = 0;
#endif // #if !YOJIMBO_SECURE_MODE

        m_sequence = 0;

        ResetBeforeNextConnect();
    }

    void Client::ResetBeforeNextConnect()
    {
        m_lastPacketSendTime = m_time - 1.0f;
        m_lastPacketReceiveTime = m_time;

        m_shouldDisconnect = false;
        m_shouldDisconnectState = CLIENT_STATE_DISCONNECTED;

        m_transport->ResetEncryptionMappings();

        if ( m_clientAllocator )
            m_clientAllocator->ClearError();

        if ( m_packetFactory )
            m_packetFactory->ClearError();

        if ( m_messageFactory )
            m_messageFactory->ClearError();

        if ( m_replayProtection )
            m_replayProtection->Reset();

        if ( m_connection )
            m_connection->Reset();
    }

    bool Client::ConnectToNextServer()
    {
        if ( m_serverAddressIndex + 1 >= m_numServerAddresses )
            return false;

        m_serverAddressIndex++;

        ResetBeforeNextConnect();

#if !YOJIMBO_SECURE_MODE

        if ( m_clientState == CLIENT_STATE_SENDING_INSECURE_CONNECT )
        {
            char addressString[MaxAddressLength];
            m_serverAddresses[m_serverAddressIndex].ToString( addressString, sizeof( addressString ) );
            debug_printf( "connect to next insecure server: %s (%d/%d)\n", addressString, m_serverAddressIndex + 1, m_numServerAddresses );

            InternalInsecureConnect( m_serverAddresses[m_serverAddressIndex] );

            return true;
        }

#endif // #if !YOJIMBO_SECURE_MODE

        char addressString[MaxAddressLength];
        m_serverAddresses[m_serverAddressIndex].ToString( addressString, sizeof( addressString ) );
        debug_printf( "connect to next secure server: %s (%d/%d)\n", addressString, m_serverAddressIndex + 1, m_numServerAddresses );

        InternalSecureConnect( m_serverAddresses[m_serverAddressIndex] );

        return true;
    }

#if !YOJIMBO_SECURE_MODE

    void Client::InternalInsecureConnect( const Address & serverAddress )
    {
        m_serverAddress = serverAddress;

        OnConnect( serverAddress );

        SetClientState( CLIENT_STATE_SENDING_INSECURE_CONNECT );

        RandomBytes( (uint8_t*) &m_clientSalt, sizeof( m_clientSalt ) );

        debug_printf( "Client::InternalInsecureConnection - m_clientSalt = %" PRIx64 "\n", m_clientSalt );
    }

#endif // #if !YOJIMBO_SECURE_MODE

    void Client::InternalSecureConnect( const Address & serverAddress )
    {
        m_serverAddress = serverAddress;

        OnConnect( serverAddress );

        SetClientState( CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        m_transport->ResetEncryptionMappings();

        m_transport->AddEncryptionMapping( serverAddress, m_clientToServerKey, m_serverToClientKey, m_config.connectionTimeOut );
    }

    void Client::SendPacketToServer( Packet * packet )
    {
        assert( packet );
        assert( m_serverAddress.IsValid() );

        if ( !IsConnected() )
        {
            packet->Destroy();
            return;
        }

        SendPacketToServer_Internal( packet, false );
    }

    void Client::SendPacketToServer_Internal( Packet * packet, bool immediate )
    {
        assert( packet );
        assert( m_clientState > CLIENT_STATE_DISCONNECTED );
        assert( m_serverAddress.IsValid() );

        m_transport->SendPacket( m_serverAddress, packet, ++m_sequence, immediate );

        OnPacketSent( packet->GetType(), m_serverAddress, immediate );

        m_lastPacketSendTime = GetTime();
    }

    void Client::ProcessConnectionDenied( const ConnectionDeniedPacket & /*packet*/, const Address & address )
    {
        if ( m_clientState != CLIENT_STATE_SENDING_CONNECTION_REQUEST )
            return;

        if ( address != m_serverAddress )
            return;

        m_shouldDisconnect = true;
        m_shouldDisconnectState = CLIENT_STATE_CONNECTION_DENIED;
    }

    void Client::ProcessChallenge( const ChallengePacket & packet, const Address & address )
    {
        if ( m_clientState != CLIENT_STATE_SENDING_CONNECTION_REQUEST )
            return;

        if ( address != m_serverAddress )
            return;

        memcpy( m_challengeTokenData, packet.challengeTokenData, ChallengeTokenBytes );
        memcpy( m_challengeTokenNonce, packet.challengeTokenNonce, NonceBytes );

        SetClientState( CLIENT_STATE_SENDING_CHALLENGE_RESPONSE );

        const double time = GetTime();

        m_lastPacketReceiveTime = time;
    }

    bool Client::IsPendingConnect()
    {
#if !YOJIMBO_SECURE_MODE
        return m_clientState == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE || m_clientState == CLIENT_STATE_SENDING_INSECURE_CONNECT;
#else // #if !YOJIMBO_SECURE_MODE
        return m_clientState == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE;
#endif // #if !YOJIMBO_SECURE_MODE
    }

    void Client::CompletePendingConnect( int clientIndex )
    {
        if ( m_clientState == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE )
        {
            m_clientIndex = clientIndex;

            memset( m_connectTokenData, 0, ConnectTokenBytes );
            memset( m_connectTokenNonce, 0, NonceBytes );
            memset( m_challengeTokenData, 0, ChallengeTokenBytes );
            memset( m_challengeTokenNonce, 0, NonceBytes );

            SetClientState( CLIENT_STATE_CONNECTED );
        }

#if !YOJIMBO_SECURE_MODE

        if ( m_clientState == CLIENT_STATE_SENDING_INSECURE_CONNECT )
        {
            m_clientIndex = clientIndex;

            SetClientState( CLIENT_STATE_CONNECTED );
        }

#endif // #if !YOJIMBO_SECURE_MODE
    }

    void Client::ProcessKeepAlive( const KeepAlivePacket & packet, const Address & address )
    {
        debug_printf( "Client::ProcessKeepAlive\n" );

        if ( address != m_serverAddress )
            return;

#if !YOJIMBO_SECURE_MODE
        if ( m_clientState == CLIENT_STATE_SENDING_INSECURE_CONNECT && packet.clientSalt != m_clientSalt )
        {
            debug_printf( "client salt mismatch: expected %" PRIx64 ", got %" PRIx64 "\n", m_clientSalt, packet.clientSalt );
            return;
        }
#endif // #if !YOJIMBO_SECURE_MODE

        if ( IsPendingConnect() )
            CompletePendingConnect( packet.clientIndex );

        m_lastPacketReceiveTime = GetTime();
    }

    void Client::ProcessDisconnect( const DisconnectPacket & /*packet*/, const Address & address )
    {
        if ( m_clientState != CLIENT_STATE_CONNECTED )
            return;

        if ( address != m_serverAddress )
            return;

        m_shouldDisconnect = true;
        m_shouldDisconnectState = CLIENT_STATE_DISCONNECTED;
    }

    void Client::ProcessConnectionPacket( ConnectionPacket & packet, const Address & address )
    {
        if ( !IsConnected() )
            return;

        if ( address != m_serverAddress )
            return;

        if ( m_connection )
            m_connection->ProcessPacket( &packet );

        m_lastPacketReceiveTime = GetTime();
    }

    void Client::ProcessPacket( Packet * packet, const Address & address, uint64_t /*sequence*/ )
    {
        OnPacketReceived( packet->GetType(), address );
        
        switch ( packet->GetType() )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:
                ProcessConnectionDenied( *(ConnectionDeniedPacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_CHALLENGE:
                ProcessChallenge( *(ChallengePacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_KEEPALIVE:
                ProcessKeepAlive( *(KeepAlivePacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_DISCONNECT:
                ProcessDisconnect( *(DisconnectPacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_CONNECTION:
                ProcessConnectionPacket( *(ConnectionPacket*)packet, address );
                return;

            default:
                break;
        }

        if ( !IsConnected() )
            return;

        if ( address != m_serverAddress )
            return;

        if ( !ProcessUserPacket( packet ) )
            return;

        m_lastPacketReceiveTime = GetTime();
    }
}

#endif
