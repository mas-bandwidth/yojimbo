/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#include "yojimbo_config.h"
#include "yojimbo_server.h"
#include "yojimbo_simulator.h"
#include "netcode.h"
#include "reliable.h"

namespace yojimbo
{
    // -----------------------------------------------------------------------------------------------------

    BaseServer::BaseServer( Allocator & allocator, const BaseClientServerConfig & config, Adapter & adapter, double time ) : m_config( config )
    {
        m_allocator = &allocator;
        m_adapter = &adapter;
        m_context = NULL;
        m_time = time;
        m_running = false;
        m_maxClients = 0;
        m_globalMemory = NULL;
        m_globalAllocator = NULL;
        for ( int i = 0; i < MaxClients; ++i )
        {
            m_clientMemory[i] = NULL;
            m_clientAllocator[i] = NULL;
            m_clientMessageFactory[i] = NULL;
            m_clientConnection[i] = NULL;
            m_clientEndpoint[i] = NULL;
        }
        m_networkSimulator = NULL;
    }

    BaseServer::~BaseServer()
    {
        // IMPORTANT: Please stop the server before destroying it!
        yojimbo_assert( !IsRunning () );
        m_allocator = NULL;
    }

    void BaseServer::SetContext( void * context )
    {
        yojimbo_assert( !IsRunning() );
        m_context = context;
    }

    void BaseServer::Start( int maxClients )
    {
        Stop();
        m_running = true;
        m_maxClients = maxClients;
        yojimbo_assert( !m_globalMemory );
        yojimbo_assert( !m_globalAllocator );
        m_globalMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverGlobalMemory );
        m_globalAllocator = m_adapter->CreateAllocator( *m_allocator, m_globalMemory, m_config.serverGlobalMemory );
        yojimbo_assert( m_globalAllocator );
        if ( m_config.networkSimulator )
        {
            m_networkSimulator = YOJIMBO_NEW( *m_globalAllocator, NetworkSimulator, *m_globalAllocator, m_config.maxSimulatorPackets, m_time );
        }
        for ( int i = 0; i < m_maxClients; ++i )
        {
            yojimbo_assert( !m_clientMemory[i] );
            yojimbo_assert( !m_clientAllocator[i] );
            m_clientMemory[i] = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverPerClientMemory );
            m_clientAllocator[i] = m_adapter->CreateAllocator( *m_allocator, m_clientMemory[i], m_config.serverPerClientMemory );
            yojimbo_assert( m_clientAllocator[i] );
            m_clientMessageFactory[i] = m_adapter->CreateMessageFactory( *m_clientAllocator[i] );
            yojimbo_assert( m_clientMessageFactory[i] );
            m_clientConnection[i] = YOJIMBO_NEW( *m_clientAllocator[i], Connection, *m_clientAllocator[i], *m_clientMessageFactory[i], m_config, m_time );
            yojimbo_assert( m_clientConnection[i] );
            // todo: fully setup endpoint config from client/server config
            reliable_config_t config;
            reliable_default_config( &config );
            sprintf( config.name, "server endpoint" );
            config.context = (void*) this;
            config.index = i;
            config.transmit_packet_function = BaseServer::StaticTransmitPacketFunction;
            config.process_packet_function = BaseServer::StaticProcessPacketFunction;
            config.allocator_context = &GetGlobalAllocator();
            config.allocate_function = BaseServer::StaticAllocateFunction;
            config.free_function = BaseServer::StaticFreeFunction;
            m_clientEndpoint[i] = reliable_endpoint_create( &config );
            reliable_endpoint_reset( m_clientEndpoint[i] );
        }
    }

    void BaseServer::Stop()
    {
        if ( IsRunning() )
        {
            yojimbo_assert( m_globalMemory );
            yojimbo_assert( m_globalAllocator );
            YOJIMBO_DELETE( *m_globalAllocator, NetworkSimulator, m_networkSimulator );
            for ( int i = 0; i < m_maxClients; ++i )
            {
                yojimbo_assert( m_clientMemory[i] );
                yojimbo_assert( m_clientAllocator[i] );
                yojimbo_assert( m_clientMessageFactory[i] );
                yojimbo_assert( m_clientEndpoint[i] );
                reliable_endpoint_destroy( m_clientEndpoint[i] ); m_clientEndpoint[i] = NULL;
                YOJIMBO_DELETE( *m_clientAllocator[i], Connection, m_clientConnection[i] );
                YOJIMBO_DELETE( *m_clientAllocator[i], MessageFactory, m_clientMessageFactory[i] );
                YOJIMBO_DELETE( *m_allocator, Allocator, m_clientAllocator[i] );
                YOJIMBO_FREE( *m_allocator, m_clientMemory[i] );
            }
            YOJIMBO_DELETE( *m_allocator, Allocator, m_globalAllocator );
            YOJIMBO_FREE( *m_allocator, m_globalMemory );
        }
        m_running = false;
        m_maxClients = 0;
    }

    void BaseServer::AdvanceTime( double time )
    {
        m_time = time;
        if ( IsRunning() )
        {
            for ( int i = 0; i < m_maxClients; ++i )
            {
                m_clientConnection[i]->AdvanceTime( time );
                if ( m_clientConnection[i]->GetErrorLevel() != CONNECTION_ERROR_NONE )
                {
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "client %d connection is in error state. disconnecting client\n", m_clientConnection[i]->GetErrorLevel() );
                    DisconnectClient( i );
                    continue;
                }
                reliable_endpoint_update( m_clientEndpoint[i] );
                int numAcks;
                const uint16_t * acks = reliable_endpoint_get_acks( m_clientEndpoint[i], &numAcks );
                m_clientConnection[i]->ProcessAcks( acks, numAcks );
                reliable_endpoint_clear_acks( m_clientEndpoint[i] );
            }
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator )
            {
                networkSimulator->AdvanceTime( time );
            }        
        }
    }

    void BaseServer::SetLatency( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetLatency( milliseconds );
        }
    }

    void BaseServer::SetJitter( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetJitter( milliseconds );
        }
    }

    void BaseServer::SetPacketLoss( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetPacketLoss( percent );
        }
    }

    void BaseServer::SetDuplicates( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetDuplicates( percent );
        }
    }

    Message * BaseServer::CreateMessage( int clientIndex, int type )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientMessageFactory[clientIndex] );
        return m_clientMessageFactory[clientIndex]->CreateMessage( type );
    }

    uint8_t * BaseServer::AllocateBlock( int clientIndex, int bytes )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientAllocator[clientIndex] );
        return (uint8_t*) YOJIMBO_ALLOCATE( *m_clientAllocator[clientIndex], bytes );
    }

    void BaseServer::AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( message );
        yojimbo_assert( block );
        yojimbo_assert( bytes > 0 );
        yojimbo_assert( message->IsBlockMessage() );
        BlockMessage * blockMessage = (BlockMessage*) message;
        blockMessage->AttachBlock( *m_clientAllocator[clientIndex], block, bytes );
    }

    void BaseServer::FreeBlock( int clientIndex, uint8_t * block )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        YOJIMBO_FREE( *m_clientAllocator[clientIndex], block );
    }

    bool BaseServer::CanSendMessage( int clientIndex, int channelIndex ) const
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->CanSendMessage( channelIndex );
    }

    void BaseServer::SendMessage( int clientIndex, int channelIndex, Message * message )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->SendMessage( channelIndex, message );
    }

    Message * BaseServer::ReceiveMessage( int clientIndex, int channelIndex )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->ReceiveMessage( channelIndex );
    }

    void BaseServer::ReleaseMessage( int clientIndex, Message * message )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        m_clientConnection[clientIndex]->ReleaseMessage( message );
    }

    MessageFactory & BaseServer::GetClientMessageFactory( int clientIndex ) 
    { 
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        return *m_clientMessageFactory[clientIndex];
    }

    reliable_endpoint_t * BaseServer::GetClientEndpoint( int clientIndex )
    {
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        return m_clientEndpoint[clientIndex];
    }

    Connection & BaseServer::GetClientConnection( int clientIndex )
    {
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return *m_clientConnection[clientIndex];
    }

    void BaseServer::StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        BaseServer * server = (BaseServer*) context;
        server->TransmitPacketFunction( index, packetSequence, packetData, packetBytes );
    }
    
    int BaseServer::StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        BaseServer * server = (BaseServer*) context;
        return server->ProcessPacketFunction( index, packetSequence, packetData, packetBytes );
    }

    void * BaseServer::StaticAllocateFunction( void * context, uint64_t bytes )
    {
        yojimbo_assert( context );
        Allocator * allocator = (Allocator*) context;
        return YOJIMBO_ALLOCATE( *allocator, bytes );
    }
    
    void BaseServer::StaticFreeFunction( void * context, void * pointer )
    {
        yojimbo_assert( context );
        yojimbo_assert( pointer );
        Allocator * allocator = (Allocator*) context;
        YOJIMBO_FREE( *allocator, pointer );
    }

    // -----------------------------------------------------------------------------------------------------

    Server::Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, Adapter & adapter, double time ) 
        : BaseServer( allocator, config, adapter, time )
    {
        yojimbo_assert( KeyBytes == NETCODE_KEY_BYTES );
        memcpy( m_privateKey, privateKey, NETCODE_KEY_BYTES );
        m_address = address;
        m_config = config;
        m_server = NULL;
    }

    Server::~Server()
    {
        // IMPORTANT: Please stop the server before destroying it!
        yojimbo_assert( !m_server );
    }

    void Server::Start( int maxClients )
    {
        if ( IsRunning() )
            Stop();
        
        BaseServer::Start( maxClients );
        
        char addressString[MaxAddressLength];
        m_address.ToString( addressString, MaxAddressLength );
        m_server = netcode_server_create_with_allocator( addressString, 
                                                         m_config.protocolId, 
                                                         m_privateKey, 
                                                         GetTime(), 
                                                         &GetGlobalAllocator(), 
                                                         StaticAllocateFunction, 
                                                         StaticFreeFunction );
        if ( !m_server )
        {
            Stop();
            return;
        }

        netcode_server_connect_disconnect_callback( m_server, this, StaticConnectDisconnectCallbackFunction );
        
        netcode_server_start( m_server, maxClients );
    }

    void Server::Stop()
    {
        if ( m_server )
        {
            netcode_server_stop( m_server );
            netcode_server_destroy( m_server );
            m_server = NULL;
        }
        BaseServer::Stop();
    }

    void Server::DisconnectClient( int clientIndex )
    {
        yojimbo_assert( m_server );
        netcode_server_disconnect_client( m_server, clientIndex );
    }

    void Server::DisconnectAllClients()
    {
        yojimbo_assert( m_server );
        netcode_server_disconnect_all_clients( m_server );
    }

    void Server::SendPackets()
    {
        if ( m_server )
        {
            const int maxClients = GetMaxClients();
            for ( int i = 0; i < maxClients; ++i )
            {
                if ( IsClientConnected( i ) )
                {
                    // todo: we don't want to allocate this on the stack, as packet size can be larger than that now
                    uint8_t * packetData = (uint8_t*) alloca( m_config.maxPacketSize );
                    int packetBytes;
                    uint16_t packetSequence = reliable_endpoint_next_packet_sequence( GetClientEndpoint(i) );
                    if ( GetClientConnection(i).GeneratePacket( GetContext(), packetSequence, packetData, m_config.maxPacketSize, packetBytes ) )
                    {
                        reliable_endpoint_send_packet( GetClientEndpoint(i), packetData, packetBytes );
                    }
                }
            }
        }
    }

    void Server::ReceivePackets()
    {
        if ( m_server )
        {
            const int maxClients = GetMaxClients();
            for ( int clientIndex = 0; clientIndex < maxClients; ++clientIndex )
            {
                while ( true )
                {
                    int packetBytes;
                    uint64_t packetSequence;
                    uint8_t * packetData = netcode_server_receive_packet( m_server, clientIndex, &packetBytes, &packetSequence );
                    if ( !packetData )
                        break;
                    reliable_endpoint_receive_packet( GetClientEndpoint( clientIndex ), packetData, packetBytes );
                    netcode_server_free_packet( m_server, packetData );
                }
            }
        }
    }

    void Server::AdvanceTime( double time )
    {
        if ( m_server )
        {
            netcode_server_update( m_server, time );
        }
        BaseServer::AdvanceTime( time );
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator && networkSimulator->IsActive() )
        {
            uint8_t ** packetData = (uint8_t**) alloca( sizeof( uint8_t*) * m_config.maxSimulatorPackets );
            int * packetBytes = (int*) alloca( sizeof(int) * m_config.maxSimulatorPackets );
            int * to = (int*) alloca( sizeof(int) * m_config.maxSimulatorPackets );
            int numPackets = networkSimulator->ReceivePackets( m_config.maxSimulatorPackets, packetData, packetBytes, to );
            for ( int i = 0; i < numPackets; ++i )
            {
                netcode_server_send_packet( m_server, to[i], (uint8_t*) packetData[i], packetBytes[i] );
                YOJIMBO_FREE( networkSimulator->GetAllocator(), packetData[i] );
            }
        }
    }

    bool Server::IsClientConnected( int clientIndex ) const
    {
        return netcode_server_client_connected( m_server, clientIndex ) != 0;
    }

    int Server::GetNumConnectedClients() const
    {
        return netcode_server_num_connected_clients( m_server );
    }

    void Server::TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) packetSequence;
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator && networkSimulator->IsActive() )
        {
            networkSimulator->SendPacket( clientIndex, packetData, packetBytes );
        }
        else
        {
            netcode_server_send_packet( m_server, clientIndex, packetData, packetBytes );
        }
    }

    int Server::ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        return (int) GetClientConnection(clientIndex).ProcessPacket( GetContext(), packetSequence, packetData, packetBytes );
    }

    void Server::ConnectDisconnectCallbackFunction( int clientIndex, int connected )
    {
        if ( connected == 0 )
        {
            reliable_endpoint_reset( GetClientEndpoint( clientIndex ) );
            GetClientConnection( clientIndex ).Reset();
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator && networkSimulator->IsActive() )
            {
                networkSimulator->DiscardClientPackets( clientIndex );
            }
        }
    }

    void Server::StaticConnectDisconnectCallbackFunction( void * context, int clientIndex, int connected )
    {
        Server * server = (Server*) context;
        server->ConnectDisconnectCallbackFunction( clientIndex, connected );
    }

    // -----------------------------------------------------------------------------------------------------
}
