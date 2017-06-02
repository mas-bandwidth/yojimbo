/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#include "yojimbo_config.h"
#include "yojimbo_client.h"
#include "yojimbo_connection.h"
#include "yojimbo_simulator.h"
#include "netcode.h"
#include "reliable.h"
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

namespace yojimbo
{
    // ------------------------------------------------------------------------------------------------------------------

    BaseClient::BaseClient( Allocator & allocator, const BaseClientServerConfig & config, Adapter & adapter, double time ) : m_config( config )
    {
        m_allocator = &allocator;
        m_adapter = &adapter;
        m_time = time;
        m_context = NULL;
        m_clientMemory = NULL;
        m_clientAllocator = NULL;
        m_endpoint = NULL;
        m_connection = NULL;
        m_messageFactory = NULL;
        m_networkSimulator = NULL;
        m_clientState = CLIENT_STATE_DISCONNECTED;
        m_clientIndex = -1;
    }

    BaseClient::~BaseClient()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        yojimbo_assert( m_clientState <= CLIENT_STATE_DISCONNECTED );
        m_allocator = NULL;
    }

    void BaseClient::Disconnect()
    {
        SetClientState( CLIENT_STATE_DISCONNECTED );
    }

    void BaseClient::AdvanceTime( double time )
    {
        m_time = time;
        if ( m_endpoint )
        {
            m_connection->AdvanceTime( time );
            if ( m_connection->GetErrorLevel() != CONNECTION_ERROR_NONE )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_DEBUG, "connection error. disconnecting client\n" );
                Disconnect();
                return;
            }
            reliable_endpoint_update( m_endpoint );
            int numAcks;
            const uint16_t * acks = reliable_endpoint_get_acks( m_endpoint, &numAcks );
            m_connection->ProcessAcks( acks, numAcks );
            reliable_endpoint_clear_acks( m_endpoint );
        }
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator )
        {
            networkSimulator->AdvanceTime( time );
        }
    }

    void BaseClient::SetLatency( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetLatency( milliseconds );
        }
    }

    void BaseClient::SetJitter( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetJitter( milliseconds );
        }
    }

    void BaseClient::SetPacketLoss( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetPacketLoss( percent );
        }
    }

    void BaseClient::SetDuplicates( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetDuplicates( percent );
        }
    }

    void BaseClient::SetClientState( ClientState clientState )
    {
        m_clientState = clientState;
    }

    void BaseClient::CreateInternal()
    {
        yojimbo_assert( m_allocator );
        yojimbo_assert( m_adapter );
        yojimbo_assert( m_clientMemory == NULL );
        yojimbo_assert( m_clientAllocator == NULL );
        yojimbo_assert( m_messageFactory == NULL );
        m_clientMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.clientMemory );
        m_clientAllocator = m_adapter->CreateAllocator( *m_allocator, m_clientMemory, m_config.clientMemory );
        m_messageFactory = m_adapter->CreateMessageFactory( *m_clientAllocator );
        m_connection = YOJIMBO_NEW( *m_clientAllocator, Connection, *m_clientAllocator, *m_messageFactory, m_config, m_time );
        yojimbo_assert( m_connection );
        if ( m_config.networkSimulator )
        {
            m_networkSimulator = YOJIMBO_NEW( *m_clientAllocator, NetworkSimulator, *m_clientAllocator, m_config.maxSimulatorPackets, m_time );
        }
        // todo: fully setup endpoint config from client/server config
        reliable_config_t config;
        reliable_default_config( &config );
        strcpy( config.name, "client endpoint" );
        config.context = (void*) this;
        config.transmit_packet_function = BaseClient::StaticTransmitPacketFunction;
        config.process_packet_function = BaseClient::StaticProcessPacketFunction;
        config.allocator_context = m_clientAllocator;
        config.allocate_function = BaseClient::StaticAllocateFunction;
        config.free_function = BaseClient::StaticFreeFunction;
        m_endpoint = reliable_endpoint_create( &config );
        reliable_endpoint_reset( m_endpoint );
    }

    void BaseClient::DestroyInternal()
    {
        yojimbo_assert( m_allocator );
        if ( m_endpoint )
        {
            reliable_endpoint_destroy( m_endpoint ); 
            m_endpoint = NULL;
        }
        YOJIMBO_DELETE( *m_clientAllocator, NetworkSimulator, m_networkSimulator );
        YOJIMBO_DELETE( *m_clientAllocator, Connection, m_connection );
        YOJIMBO_DELETE( *m_clientAllocator, MessageFactory, m_messageFactory );
        YOJIMBO_DELETE( *m_allocator, Allocator, m_clientAllocator );
        YOJIMBO_FREE( *m_allocator, m_clientMemory );
    }

    void BaseClient::StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) index;
        BaseClient * client = (BaseClient*) context;
        client->TransmitPacketFunction( packetSequence, packetData, packetBytes );
    }
    
    int BaseClient::StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) index;
        BaseClient * client = (BaseClient*) context;
        return client->ProcessPacketFunction( packetSequence, packetData, packetBytes );
    }

    void * BaseClient::StaticAllocateFunction( void * context, uint64_t bytes )
    {
        yojimbo_assert( context );
        Allocator * allocator = (Allocator*) context;
        return YOJIMBO_ALLOCATE( *allocator, bytes );
    }
    
    void BaseClient::StaticFreeFunction( void * context, void * pointer )
    {
        yojimbo_assert( context );
        yojimbo_assert( pointer );
        Allocator * allocator = (Allocator*) context;
        YOJIMBO_FREE( *allocator, pointer );
    }

    Message * BaseClient::CreateMessage( int type )
    {
        yojimbo_assert( m_messageFactory );
        return m_messageFactory->CreateMessage( type );
    }

    uint8_t * BaseClient::AllocateBlock( int bytes )
    {
        return (uint8_t*) YOJIMBO_ALLOCATE( *m_clientAllocator, bytes );
    }

    void BaseClient::AttachBlockToMessage( Message * message, uint8_t * block, int bytes )
    {
        yojimbo_assert( message );
        yojimbo_assert( block );
        yojimbo_assert( bytes > 0 );
        yojimbo_assert( message->IsBlockMessage() );
        BlockMessage * blockMessage = (BlockMessage*) message;
        blockMessage->AttachBlock( *m_clientAllocator, block, bytes );
    }

    void BaseClient::FreeBlock( uint8_t * block )
    {
        YOJIMBO_FREE( *m_clientAllocator, block );
    }

    bool BaseClient::CanSendMessage( int channelIndex ) const
    {
        yojimbo_assert( m_connection );
        return m_connection->CanSendMessage( channelIndex );
    }

    void BaseClient::SendMessage( int channelIndex, Message * message )
    {
        yojimbo_assert( m_connection );
        m_connection->SendMessage( channelIndex, message );
    }

    Message * BaseClient::ReceiveMessage( int channelIndex )
    {
        yojimbo_assert( m_connection );
        return m_connection->ReceiveMessage( channelIndex );
    }

    void BaseClient::ReleaseMessage( Message * message )
    {
        yojimbo_assert( m_connection );
        m_connection->ReleaseMessage( message );
    }

    // ------------------------------------------------------------------------------------------------------------------

    Client::Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time ) : BaseClient( allocator, config, adapter, time ), m_config( config ), m_address( address )
    {
        m_clientId = 0;
        m_client = NULL;
    }

    Client::~Client()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        yojimbo_assert( m_client == NULL );
    }

    void Client::InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address & address )
    {
        InsecureConnect( privateKey, clientId, &address, 1 );
    }

    void Client::InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses )
    {
        yojimbo_assert( serverAddresses );
        yojimbo_assert( numServerAddresses > 0 );
        yojimbo_assert( numServerAddresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        if ( !m_client )
        {
            Disconnect();
            return;
        }
        uint8_t connectToken[NETCODE_CONNECT_TOKEN_BYTES];
        if ( !GenerateInsecureConnectToken( connectToken, privateKey, clientId, serverAddresses, numServerAddresses ) )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to generate insecure connect token\n" );
            SetClientState( CLIENT_STATE_ERROR );
            return;
        }
        netcode_client_connect( m_client, connectToken );
        SetClientState( CLIENT_STATE_CONNECTING );
    }

    bool Client::GenerateInsecureConnectToken( uint8_t * connectToken, const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses, int timeout )
    {
        char serverAddressStrings[NETCODE_MAX_SERVERS_PER_CONNECT][MaxAddressLength];
        char * serverAddressStringPointers[NETCODE_MAX_SERVERS_PER_CONNECT];
        for ( int i = 0; i < numServerAddresses; ++i ) 
        {
            serverAddresses[i].ToString( serverAddressStrings[i], MaxAddressLength );
            serverAddressStringPointers[i] = serverAddressStrings[i];
        }
        return netcode_generate_connect_token( numServerAddresses, serverAddressStringPointers, timeout, clientId, m_config.protocolId, 0, (uint8_t*)privateKey, connectToken ) == NETCODE_OK;
    }

    void Client::Connect( uint64_t clientId, uint8_t * connectToken )
    {
        yojimbo_assert( connectToken );
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        netcode_client_connect( m_client, connectToken );
        SetClientState( CLIENT_STATE_CONNECTING );
    }

    void Client::Disconnect()
    {
        BaseClient::Disconnect();
        DestroyClient();
        DestroyInternal();
        m_clientId = 0;
    }

    void Client::SendPackets()
    {
        if ( !IsConnected() )
            return;
        yojimbo_assert( m_client );
        // todo: we don't want to allocate this on the stack, as packet size can be larger than that now
        uint8_t * packetData = (uint8_t*) alloca( m_config.maxPacketSize );
        int packetBytes;
        uint16_t packetSequence = reliable_endpoint_next_packet_sequence( GetEndpoint() );
        if ( GetConnection().GeneratePacket( GetContext(), packetSequence, packetData, m_config.maxPacketSize, packetBytes ) )
        {
            reliable_endpoint_send_packet( GetEndpoint(), packetData, packetBytes );
        }
    }

    void Client::ReceivePackets()
    {
        if ( !IsConnected() )
            return;
        yojimbo_assert( m_client );
        while ( true )
        {
            int packetBytes;
            uint64_t packetSequence;
            uint8_t * packetData = netcode_client_receive_packet( m_client, &packetBytes, &packetSequence );
            if ( !packetData )
                break;
            reliable_endpoint_receive_packet( GetEndpoint(), packetData, packetBytes );
            netcode_client_free_packet( m_client, packetData );
        }
    }

    void Client::AdvanceTime( double time )
    {
        BaseClient::AdvanceTime( time );
        if ( m_client )
        {
            netcode_client_update( m_client, time );
            const int state = netcode_client_state( m_client );
            if ( state < NETCODE_CLIENT_STATE_DISCONNECTED )
            {
                Disconnect();
                SetClientState( CLIENT_STATE_ERROR );
            }
            else if ( state == NETCODE_CLIENT_STATE_DISCONNECTED )
            {
                Disconnect();
                SetClientState( CLIENT_STATE_DISCONNECTED );
            }
            else
            {
                SetClientState( CLIENT_STATE_CONNECTED );
            }
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator && networkSimulator->IsActive() )
            {
                uint8_t ** packetData = (uint8_t**) alloca( sizeof( uint8_t*) * m_config.maxSimulatorPackets );
                int * packetBytes = (int*) alloca( sizeof(int) * m_config.maxSimulatorPackets );
                int numPackets = networkSimulator->ReceivePackets( m_config.maxSimulatorPackets, packetData, packetBytes, NULL );
                for ( int i = 0; i < numPackets; ++i )
                {
                    netcode_client_send_packet( m_client, (uint8_t*) packetData[i], packetBytes[i] );
                    YOJIMBO_FREE( networkSimulator->GetAllocator(), packetData[i] );
                }
            }
        }
    }

    int Client::GetClientIndex() const
    {
        return m_client ? netcode_client_index( m_client ) : -1;
    }

    void Client::CreateClient( const Address & address )
    {
        DestroyClient();
        char addressString[MaxAddressLength];
        address.ToString( addressString, MaxAddressLength );
        m_client = netcode_client_create_with_allocator( addressString, GetTime(), &GetClientAllocator(), StaticAllocateFunction, StaticFreeFunction );
        if ( m_client )
        {
            netcode_client_state_change_callback( m_client, this, StaticStateChangeCallbackFunction );
        }
    }

    void Client::DestroyClient()
    {
        if ( m_client )
        {
            netcode_client_destroy( m_client );
            m_client = NULL;
        }
    }

    void Client::StateChangeCallbackFunction( int previous, int current )
    {
        (void) previous;
        (void) current;
    }

    void Client::StaticStateChangeCallbackFunction( void * context, int previous, int current )
    {
        Client * client = (Client*) context;
        client->StateChangeCallbackFunction( previous, current );
    }

    void Client::TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) packetSequence;
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator && networkSimulator->IsActive() )
        {
            networkSimulator->SendPacket( 0, packetData, packetBytes );
        }
        else
        {
            netcode_client_send_packet( m_client, packetData, packetBytes );
        }
    }

    int Client::ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        return (int) GetConnection().ProcessPacket( GetContext(), packetSequence, packetData, packetBytes );
    }

    // ------------------------------------------------------------------------------------------------------------------
}
