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

namespace yojimbo
{
    // -----------------------------------------------------------------------------------------------------

    BaseServer::BaseServer( Allocator & allocator, const BaseClientServerConfig & config, double time )
    {
        m_config = config;
        m_allocator = &allocator;
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
        m_globalAllocator = CreateAllocator( *m_allocator, m_globalMemory, m_config.serverGlobalMemory );
        for ( int i = 0; i < m_maxClients; ++i )
        {
            assert( !m_clientMemory[i] );
            assert( !m_clientAllocator[i] );
            m_clientMemory[i] = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverPerClientMemory );
            m_clientAllocator[i] = CreateAllocator( *m_allocator, m_clientMemory[i], m_config.serverPerClientMemory );
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
    }

    Allocator * BaseServer::CreateAllocator( Allocator & allocator, void * memory, size_t bytes )
    {
        return YOJIMBO_NEW( allocator, TLSF_Allocator, memory, bytes );
    }

    // -----------------------------------------------------------------------------------------------------

    Server::Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, double time ) : BaseServer( allocator, config, time )
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
            // todo
        }
    }

    void Server::ReceivePackets()
    {
        if ( m_server )
        {
            // todo
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

    // -----------------------------------------------------------------------------------------------------
}
