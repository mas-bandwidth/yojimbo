#include "yojimbo_client.h"
#include "yojimbo_connection.h"
#include "yojimbo_network_simulator.h"
#include "yojimbo_adapter.h"
#include "yojimbo_utils.h"
#include "netcode.h"
#include "reliable.h"

namespace yojimbo
{
    // Map a netcode client error state to the disconnect reason we record for this client.
    // This is where "server is full" vs "update your client" vs "network problem" comes from.
    static int ClientDisconnectReasonForNetcodeState( int netcodeState )
    {
        switch ( netcodeState )
        {
            case NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED:            return YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECT_TOKEN_EXPIRED;
            case NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN:            return YOJIMBO_CLIENT_DISCONNECT_REASON_INVALID_CONNECT_TOKEN;
            case NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT:             return YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_TIMED_OUT;
            case NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT:    return YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_RESPONSE_TIMED_OUT;
            case NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT:     return YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_REQUEST_TIMED_OUT;
            case NETCODE_CLIENT_STATE_CONNECTION_DENIED:                return YOJIMBO_CLIENT_DISCONNECT_REASON_CONNECTION_DENIED;
            default:                                                    return YOJIMBO_CLIENT_DISCONNECT_REASON_DISCONNECTED_BY_SERVER;
        }
    }

    Client::Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time )
        : BaseClient( allocator, config, adapter, time ), m_config( config ), m_address( address )
    {
        m_clientId = 0;
        m_client = NULL;
        m_boundAddress = m_address;
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
        SetDisconnectReason( YOJIMBO_CLIENT_DISCONNECT_REASON_NONE );       // new connect attempt: clear the reason from any previous disconnect
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
            SetDisconnectReason( YOJIMBO_CLIENT_DISCONNECT_REASON_INVALID_CONNECT_TOKEN );
            SetClientState( CLIENT_STATE_ERROR );
            return;
        }
        netcode_client_connect( m_client, connectToken );
        SetClientState( CLIENT_STATE_CONNECTING );
    }

    bool Client::GenerateInsecureConnectToken( uint8_t * connectToken, 
                                               const uint8_t privateKey[], 
                                               uint64_t clientId, 
                                               const Address serverAddresses[], 
                                               int numServerAddresses )
    {
        char serverAddressStrings[NETCODE_MAX_SERVERS_PER_CONNECT][MaxAddressLength];
        const char * serverAddressStringPointers[NETCODE_MAX_SERVERS_PER_CONNECT];
        for ( int i = 0; i < numServerAddresses; ++i ) 
        {
            serverAddresses[i].ToString( serverAddressStrings[i], MaxAddressLength );
            serverAddressStringPointers[i] = serverAddressStrings[i];
        }

        uint8_t userData[256];
        memset( &userData, 0, sizeof(userData) );

        // Give the insecure connect token an expiry well above the connection timeout, so a
        // failed insecure connect reports "connection request timed out" just like a secure
        // connect token from a matchmaker would. With expiry == timeout, netcode checks token
        // expiry first and every failed connect reports "connect token expired" instead.
        const int expireSeconds = yojimbo_max( InsecureConnectTokenExpirySeconds, m_config.timeout * 2 );

        return netcode_generate_connect_token( numServerAddresses,
                                               serverAddressStringPointers,
                                               serverAddressStringPointers,
                                               expireSeconds,
                                               m_config.timeout,
                                               clientId,
                                               m_config.protocolId,
                                               (uint8_t*)privateKey,
                                               &userData[0],
                                               connectToken ) == NETCODE_OK;
    }

    void Client::Connect( uint64_t clientId, uint8_t * connectToken )
    {
        yojimbo_assert( connectToken );
        Disconnect();
        CreateInternal();
        SetDisconnectReason( YOJIMBO_CLIENT_DISCONNECT_REASON_NONE );       // new connect attempt: clear the reason from any previous disconnect
        m_clientId = clientId;
        CreateClient( m_address );
        if ( !m_client )
        {
            // Socket creation/bind failed (e.g. port in use, invalid bind address). Bail before
            // calling into netcode, which dereferences the client pointer without checking.
            Disconnect();
            return;
        }
        netcode_client_connect( m_client, connectToken );
        if ( netcode_client_state( m_client ) > NETCODE_CLIENT_STATE_DISCONNECTED )
        {
            SetClientState( CLIENT_STATE_CONNECTING );
        }
        else
        {
            // The connect failed immediately, eg. an invalid connect token.
            SetDisconnectReason( ClientDisconnectReasonForNetcodeState( netcode_client_state( m_client ) ) );
            Disconnect();
        }
    }

    void Client::Disconnect()
    {
        // Record a deliberate local disconnect, but only if no more specific reason was already
        // recorded (connection error, netcode error state) — the first reason recorded wins.
        // Checked before BaseClient::Disconnect below, because that resets the client state.
        if ( ( IsConnecting() || IsConnected() ) && GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE )
        {
            SetDisconnectReason( YOJIMBO_CLIENT_DISCONNECT_REASON_DISCONNECTED );
        }
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
        uint8_t * packetData = GetPacketBuffer();
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
                // Record why netcode failed (denied, token expired/invalid, timed out) before the
                // disconnect, so the game can tell the player what actually happened.
                if ( GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE )
                {
                    SetDisconnectReason( ClientDisconnectReasonForNetcodeState( state ) );
                }
                Disconnect();
                SetClientState( CLIENT_STATE_ERROR );
            }
            else if ( state == NETCODE_CLIENT_STATE_DISCONNECTED )
            {
                // netcode dropped to disconnected while we thought we were connecting/connected:
                // the server disconnected us (server stopped, or kicked this client).
                if ( GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE )
                {
                    SetDisconnectReason( YOJIMBO_CLIENT_DISCONNECT_REASON_DISCONNECTED_BY_SERVER );
                }
                Disconnect();
                SetClientState( CLIENT_STATE_DISCONNECTED );
            }
            else if ( state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST || state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE )
            {
                SetClientState( CLIENT_STATE_CONNECTING );
            }
            else
            {
                SetClientState( CLIENT_STATE_CONNECTED );
            }
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator && networkSimulator->IsActive() )
            {
                // Drain the simulator in fixed size batches, so stack usage here doesn't scale
                // with maxSimulatorPackets. Each batch scans the simulator ring again, but the
                // simulator is a development tool, not a production path.
                const int MaxBatchPackets = 64;
                uint8_t * packetData[MaxBatchPackets];
                int packetBytes[MaxBatchPackets];
                while ( true )
                {
                    const int numPackets = networkSimulator->ReceivePackets( MaxBatchPackets, packetData, packetBytes, NULL );
                    if ( numPackets == 0 )
                        break;
                    for ( int i = 0; i < numPackets; ++i )
                    {
                        netcode_client_send_packet( m_client, packetData[i], packetBytes[i] );
                        YOJIMBO_FREE( networkSimulator->GetAllocator(), packetData[i] );
                    }
                }
            }
        }
    }

    int Client::GetClientIndex() const
    {
        return m_client ? netcode_client_index( m_client ) : -1;
    }

    void Client::ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients )
    {
        Disconnect();
        CreateInternal();
        SetDisconnectReason( YOJIMBO_CLIENT_DISCONNECT_REASON_NONE );       // new connect attempt: clear the reason from any previous disconnect
        m_clientId = clientId;
        CreateClient( m_address );
        if ( !m_client )
        {
            // Socket creation/bind failed. Bail before netcode dereferences the client pointer.
            Disconnect();
            return;
        }
        netcode_client_connect_loopback( m_client, clientIndex, maxClients );
        SetClientState( CLIENT_STATE_CONNECTED );
    }

    void Client::DisconnectLoopback()
    {
        // Same recording rule as Client::Disconnect: a deliberate local disconnect, unless a more
        // specific reason was already recorded.
        if ( ( IsConnecting() || IsConnected() ) && GetDisconnectReason() == YOJIMBO_CLIENT_DISCONNECT_REASON_NONE )
        {
            SetDisconnectReason( YOJIMBO_CLIENT_DISCONNECT_REASON_DISCONNECTED );
        }
        netcode_client_disconnect_loopback( m_client );
        BaseClient::Disconnect();
        DestroyClient();
        DestroyInternal();
        m_clientId = 0;
    }

    bool Client::IsLoopback() const
    {
        // Safe to query in any state: when not connected there is no netcode client, so this is
        // not a loopback client. netcode_client_loopback() would dereference a NULL m_client.
        return m_client ? ( netcode_client_loopback( m_client ) != 0 ) : false;
    }

    void Client::ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        netcode_client_process_loopback_packet( m_client, packetData, packetBytes, packetSequence );
    }

    void Client::CreateClient( const Address & address )
    {
        DestroyClient();
        char addressString[MaxAddressLength];
        address.ToString( addressString, MaxAddressLength );

        struct netcode_client_config_t netcodeConfig;
        netcode_default_client_config(&netcodeConfig);
        netcodeConfig.allocator_context             = &GetClientAllocator();
        netcodeConfig.allocate_function             = StaticAllocateFunction;
        netcodeConfig.free_function                 = StaticFreeFunction;
        netcodeConfig.callback_context              = this;
        netcodeConfig.state_change_callback         = StaticStateChangeCallbackFunction;
        netcodeConfig.send_loopback_packet_callback = StaticSendLoopbackPacketCallbackFunction;
        m_client = netcode_client_create(addressString, &netcodeConfig, GetTime());
        
        if ( m_client )
        {
            m_boundAddress.SetPort( netcode_client_get_port( m_client ) );
        }
    }

    void Client::DestroyClient()
    {
        if ( m_client )
        {
            m_boundAddress = m_address;
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

    void Client::SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        GetAdapter().ClientSendLoopbackPacket( clientIndex, packetData, packetBytes, packetSequence );
    }

    void Client::StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        Client * client = (Client*) context;
        client->SendLoopbackPacketCallbackFunction( clientIndex, packetData, packetBytes, packetSequence );
    }
}
