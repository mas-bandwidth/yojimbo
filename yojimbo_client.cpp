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
#include "yojimbo_connection.h"
#include "netcode.h"
#include "reliable.h"
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

namespace yojimbo
{
    // ------------------------------------------------------------------------------------------------------------------

    BaseClient::BaseClient( Allocator & allocator, const BaseClientServerConfig & config, Adapter & adapter, double time )
    {
        m_allocator = &allocator;
        m_adapter = &adapter;
        m_config = config;
        m_time = time;
        m_context = NULL;
        m_clientMemory = NULL;
        m_clientAllocator = NULL;
        m_endpoint = NULL;
        m_connection = NULL;
        m_messageFactory = NULL;
        m_clientState = CLIENT_STATE_DISCONNECTED;
        m_clientIndex = -1;
    }

    BaseClient::~BaseClient()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        assert( m_clientState <= CLIENT_STATE_DISCONNECTED );
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
            reliable_endpoint_update( m_endpoint );
            int numAcks;
            const uint16_t * acks = reliable_endpoint_get_acks( m_endpoint, &numAcks );
            m_connection->ProcessAcks( acks, numAcks );
            reliable_endpoint_clear_acks( m_endpoint );
        }
    }

    void BaseClient::SetClientState( ClientState clientState )
    {
        m_clientState = clientState;
    }

    void BaseClient::CreateInternal()
    {
        assert( m_allocator );
        assert( m_adapter );
        assert( m_clientMemory == NULL );
        assert( m_clientAllocator == NULL );
        assert( m_messageFactory == NULL );
        m_clientMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.clientMemory );
        m_clientAllocator = m_adapter->CreateAllocator( *m_allocator, m_clientMemory, m_config.clientMemory );
        m_messageFactory = m_adapter->CreateMessageFactory( *m_clientAllocator );
        m_connection = YOJIMBO_NEW( *m_clientAllocator, Connection, *m_clientAllocator, *m_messageFactory, m_config );
        // todo: need to build reliable endpoint config from yojimbo config.
        reliable_config_t config;
        reliable_default_config( &config );
        config.context = (void*) this;
        config.transmit_packet_function = BaseClient::StaticTransmitPacketFunction;
        config.process_packet_function = BaseClient::StaticProcessPacketFunction;
        m_endpoint = reliable_endpoint_create( &config );
    }

    void BaseClient::DestroyInternal()
    {
        assert( m_allocator );
        if ( m_endpoint )
        {
            reliable_endpoint_destroy( m_endpoint ); 
            m_endpoint = NULL;
        }
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

    // ------------------------------------------------------------------------------------------------------------------

    Client::Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time ) : BaseClient( allocator, config, adapter, time )
    {
        m_config = config;
        m_address = address;
        m_client = NULL;
    }

    Client::~Client()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        assert( m_client == NULL );
    }

#if !YOJIMBO_SECURE_MODE

    void Client::InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address & address )
    {
        InsecureConnect( privateKey, clientId, &address, 1 );
    }

    void Client::InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses )
    {
        assert( serverAddresses );
        assert( numServerAddresses > 0 );
        assert( numServerAddresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        uint8_t connectToken[NETCODE_CONNECT_TOKEN_BYTES];
        if ( !GenerateInsecureConnectToken( connectToken, privateKey, clientId, serverAddresses, numServerAddresses ) )
        {
            printf( "failed to generate insecure connect token\n" );
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

#endif // #if !YOJIMBO_SECURE_MODE

    void Client::Connect( uint64_t clientId, uint8_t * connectToken )
    {
        assert( connectToken );
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
        assert( m_client );
        // todo: we don't want to allocate this on the stack, as packet size can be larger than that now
        uint8_t * packetData = (uint8_t*) alloca( m_config.maxPacketSize );
        int packetBytes;
        GetConnection().GeneratePacket( packetData, m_config.maxPacketSize, packetBytes );
        reliable_endpoint_send_packet( GetEndpoint(), packetData, packetBytes );
    }

    void Client::ReceivePackets()
    {
        if ( !IsConnected() )
            return;
        assert( m_client );
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
                SetClientState( CLIENT_STATE_ERROR );
            }
            else if ( state == NETCODE_CLIENT_STATE_DISCONNECTED )
            {
                SetClientState( CLIENT_STATE_DISCONNECTED );
            }
            else
            {
                SetClientState( CLIENT_STATE_CONNECTED );
            }
        }
    }

    void Client::CreateClient( const Address & address )
    {
        DestroyClient();
        char addressString[MaxAddressLength];
        address.ToString( addressString, MaxAddressLength );
        m_client = netcode_client_create( addressString, GetTime() );
    }

    void Client::DestroyClient()
    {
        if ( m_client )
        {
            netcode_client_destroy( m_client );
            m_client = NULL;
        }
    }

    void Client::TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) packetSequence;
        netcode_client_send_packet( m_client, packetData, packetBytes );
    }

    int Client::ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        return (int) GetConnection().ProcessPacket( packetSequence, packetData, packetBytes );
    }

    // ------------------------------------------------------------------------------------------------------------------
}
