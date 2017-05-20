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
#include "yojimbo_server.h"
#include "netcode.h"
#include "reliable.h"

namespace yojimbo
{
    // -----------------------------------------------------------------------------------------------------

    BaseServer::BaseServer( Allocator & allocator, const BaseClientServerConfig & config, Adapter & adapter, double time )
    {
        m_config = config;
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
    }

    BaseServer::~BaseServer()
    {
        // IMPORTANT: Please stop the server before destroying it!
        assert( !IsRunning () );
        m_allocator = NULL;
    }

    void BaseServer::SetContext( void * context )
    {
        assert( !IsRunning() );
        m_context = context;
    }

    void BaseServer::Start( int maxClients )
    {
        Stop();
        m_running = true;
        m_maxClients = maxClients;
        assert( !m_globalMemory );
        assert( !m_globalAllocator );
        m_globalMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverGlobalMemory );
        m_globalAllocator = m_adapter->CreateAllocator( *m_allocator, m_globalMemory, m_config.serverGlobalMemory );
        for ( int i = 0; i < m_maxClients; ++i )
        {
            assert( !m_clientMemory[i] );
            assert( !m_clientAllocator[i] );
            m_clientMemory[i] = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverPerClientMemory );
            m_clientAllocator[i] = m_adapter->CreateAllocator( *m_allocator, m_clientMemory[i], m_config.serverPerClientMemory );
            m_clientMessageFactory[i] = m_adapter->CreateMessageFactory( *m_clientAllocator[i] );
            m_clientConnection[i] = YOJIMBO_NEW( *m_clientAllocator[i], Connection, *m_clientAllocator[i], *m_clientMessageFactory[i], m_config );
            // todo: need to build reliable endpoint config from yojimbo config.
            reliable_config_t config;
            reliable_default_config( &config );
            config.context = (void*) this;
            config.index = i;
            config.transmit_packet_function = BaseServer::StaticTransmitPacketFunction;
            config.process_packet_function = BaseServer::StaticProcessPacketFunction;
            m_clientEndpoint[i] = reliable_endpoint_create( &config );
        }
    }

    void BaseServer::Stop()
    {
        if ( IsRunning() )
        {
            assert( m_globalMemory );
            assert( m_globalAllocator );
            for ( int i = 0; i < m_maxClients; ++i )
            {
                assert( m_clientMemory[i] );
                assert( m_clientAllocator[i] );
                assert( m_clientMessageFactory[i] );
                assert( m_clientEndpoint[i] );
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
        if ( !IsRunning() )
        {
            for ( int i = 0; i < m_maxClients; ++i )
            {
                reliable_endpoint_update( m_clientEndpoint[i] );
                int numAcks;
                const uint16_t * acks = reliable_endpoint_get_acks( m_clientEndpoint[i], &numAcks );
                m_clientConnection[i]->ProcessAcks( acks, numAcks );
                reliable_endpoint_clear_acks( m_clientEndpoint[i] );
            }
        }
    }

    MessageFactory & BaseServer::GetClientMessageFactory( int clientIndex ) 
    { 
        assert( IsRunning() ); 
        assert( clientIndex >= 0 ); 
        assert( clientIndex < m_maxClients );
        return *m_clientMessageFactory[clientIndex];
    }

    reliable_endpoint_t * BaseServer::GetClientEndpoint( int clientIndex )
    {
        assert( IsRunning() ); 
        assert( clientIndex >= 0 ); 
        assert( clientIndex < m_maxClients );
        return m_clientEndpoint[clientIndex];
    }

    Connection & BaseServer::GetClientConnection( int clientIndex )
    {
        assert( IsRunning() ); 
        assert( clientIndex >= 0 ); 
        assert( clientIndex < m_maxClients );
        assert( m_clientConnection[clientIndex] );
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

    // -----------------------------------------------------------------------------------------------------

    Server::Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, Adapter & adapter, double time ) : BaseServer( allocator, config, adapter, time )
    {
        assert( KeyBytes == NETCODE_KEY_BYTES );
        memcpy( m_privateKey, privateKey, NETCODE_KEY_BYTES );
        m_address = address;
        m_config = config;
        m_server = NULL;
    }

    Server::~Server()
    {
        // IMPORTANT: Please stop the server before destroying it!
        assert( !m_server );
    }

    void Server::Start( int maxClients )
    {
        if ( IsRunning() )
            Stop();
        BaseServer::Start( maxClients );
        char addressString[MaxAddressLength];
        m_address.ToString( addressString, MaxAddressLength );
        m_server = netcode_server_create( addressString, m_config.protocolId, m_privateKey, GetTime() );
        assert( m_server );
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
        assert( m_server );
        netcode_server_disconnect_client( m_server, clientIndex );
    }

    void Server::DisconnectAllClients()
    {
        assert( m_server );
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
                    GetClientConnection(i).GeneratePacket( packetData, m_config.maxPacketSize, packetBytes );
                    reliable_endpoint_send_packet( GetClientEndpoint( i ), packetData, packetBytes );
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
        netcode_server_send_packet( m_server, clientIndex, packetData, packetBytes );
    }

    int Server::ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        return (int) GetClientConnection(clientIndex).ProcessPacket( packetSequence, packetData, packetBytes );
    }

    // -----------------------------------------------------------------------------------------------------
}
