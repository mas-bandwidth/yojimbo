 /*
    Yojimbo Client/Server Network Library.
    
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

#include "yojimbo_client_server.h"
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;    

namespace yojimbo
{
    bool ConnectToken::operator == ( const ConnectToken & other ) const
    {
        if ( protocolId != other.protocolId )
            return false;
        
        if ( clientId != other.clientId )
            return false;
            
        if ( expiryTimestamp != other.expiryTimestamp )
            return false;
            
        if ( numServerAddresses != other.numServerAddresses )
            return false;
            
        for ( int i = 0; i < numServerAddresses; ++i )
        {
            if ( serverAddresses[i] != other.serverAddresses[i] )
                return false;
        }

        if ( memcmp( clientToServerKey, other.clientToServerKey, KeyBytes ) != 0 )
            return false;

        if ( memcmp( serverToClientKey, other.serverToClientKey, KeyBytes ) != 0 )
            return false;

        if ( memcmp( random, other.random, KeyBytes ) != 0 )
            return false;

        return true;
    }

    bool ConnectToken::operator != ( const ConnectToken & other ) const
    {
        return ! ( (*this) == other );
    }

    void GenerateConnectToken( ConnectToken & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses, uint32_t protocolId, int expirySeconds )
    {
        uint64_t timestamp = (uint64_t) time( NULL );
        
        token.protocolId = protocolId;
        token.clientId = clientId;
        token.expiryTimestamp = timestamp + expirySeconds;
        
        assert( numServerAddresses > 0 );
        assert( numServerAddresses <= MaxServersPerConnectToken );
        token.numServerAddresses = numServerAddresses;
        for ( int i = 0; i < numServerAddresses; ++i )
            token.serverAddresses[i] = serverAddresses[i];

        GenerateKey( token.clientToServerKey );    

        GenerateKey( token.serverToClientKey );

        GenerateKey( token.random );
    }

    bool EncryptConnectToken( const ConnectToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key )
    {
        char message[ConnectTokenBytes-AuthBytes];
        memset( message, 0, ConnectTokenBytes - AuthBytes );
        if ( !WriteConnectTokenToJSON( token, message, ConnectTokenBytes - AuthBytes ) )
            return false;

        uint64_t encryptedLength;

        if ( !Encrypt_AEAD( (const uint8_t*)message, ConnectTokenBytes - AuthBytes, encryptedMessage, encryptedLength, additional, additionalLength, nonce, key ) )
            return false;

        assert( encryptedLength == ConnectTokenBytes );

        return true;
    }

    bool DecryptConnectToken( const uint8_t * encryptedMessage, ConnectToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key )
    {
        const int encryptedMessageLength = ConnectTokenBytes;

        uint64_t decryptedMessageLength;
        uint8_t decryptedMessage[ConnectTokenBytes];

        if ( !Decrypt_AEAD( encryptedMessage, encryptedMessageLength, decryptedMessage, decryptedMessageLength, additional, additionalLength, nonce, key ) )
            return false;

        assert( decryptedMessageLength == ConnectTokenBytes - AuthBytes );

        return ReadConnectTokenFromJSON( (const char*) decryptedMessage, decryptedToken );
    }

    static void insert_number_as_string( Writer<StringBuffer> & writer, const char * key, uint64_t number )
    {
        char buffer[256];
        sprintf( buffer, "%" PRId64, number );
        writer.Key( key ); writer.String( buffer );
    }

    static void insert_data_as_base64_string( Writer<StringBuffer> & writer, const char * key, const uint8_t * data, int data_length )
    {
        char * buffer = (char*) alloca( data_length * 2 );
        base64_encode_data( data, data_length, buffer, data_length * 2 );
        writer.Key( key ); writer.String( buffer );
    }

    bool WriteConnectTokenToJSON( const ConnectToken & connectToken, char * output, int outputSize )
    {
        StringBuffer s;
        Writer<StringBuffer> writer(s);
    
        writer.StartObject();
    
        insert_number_as_string( writer, "protocolId", connectToken.protocolId );

        insert_number_as_string( writer, "clientId", connectToken.clientId );

        insert_number_as_string( writer, "expiryTimestamp", connectToken.expiryTimestamp );

        insert_number_as_string( writer, "numServerAddresses", connectToken.numServerAddresses );

        writer.Key( "serverAddresses" );

        writer.StartArray();

        for ( int i = 0; i < connectToken.numServerAddresses; ++i )
        {
            char serverAddress[MaxAddressLength];
            assert( connectToken.serverAddresses[i].IsValid() );
            connectToken.serverAddresses[i].ToString( serverAddress, MaxAddressLength );

            char serverAddressBase64[MaxAddressLength*2];
            base64_encode_string( serverAddress, serverAddressBase64, sizeof( serverAddressBase64 ) );

            writer.String( serverAddressBase64 );
        }

        writer.EndArray();

        insert_data_as_base64_string( writer, "clientToServerKey", connectToken.clientToServerKey, KeyBytes );

        insert_data_as_base64_string( writer, "serverToClientKey", connectToken.serverToClientKey, KeyBytes );

        insert_data_as_base64_string( writer, "random", connectToken.random, KeyBytes );
    
        writer.EndObject();

        const char * json_output = s.GetString();

        int json_bytes = (int) strlen( json_output ) + 1;

        if ( json_bytes > outputSize )
            return false;

        memcpy( output, json_output, json_bytes );

        return true;
    }

    static bool read_int_from_string( Document & doc, const char * key, int & value )
    {
        if ( !doc.HasMember( key ) )
            return false;

        if ( !doc[key].IsString() )
            return false;

        value = atoi( doc[key].GetString() );

        return true;
    }

    static bool read_uint32_from_string( Document & doc, const char * key, uint32_t & value )
    {
        if ( !doc.HasMember( key ) )
            return false;

        if ( !doc[key].IsString() )
            return false;

        value = (uint32_t) strtoull( doc[key].GetString(), NULL, 10 );

        return true;
    }

    static bool read_uint64_from_string( Document & doc, const char * key, uint64_t & value )
    {
        if ( !doc.HasMember( key ) )
            return false;

        if ( !doc[key].IsString() )
            return false;

        value = (uint64_t) strtoull( doc[key].GetString(), NULL, 10 );

        return true;
    }

    static bool read_data_from_base64_string( Document & doc, const char * key, uint8_t * data, int data_bytes )
    {
        if ( !doc.HasMember( key ) )
            return false;

        if ( !doc[key].IsString() )
            return false;

        const char * string = doc[key].GetString();

        const int string_length = (int) strlen( string );

        int read_data_bytes = base64_decode_data( string, data, string_length );

        return read_data_bytes == data_bytes;
    }

    bool ReadConnectTokenFromJSON( const char * json, ConnectToken & connectToken )
    {
        assert( json );

        Document doc;
        doc.Parse( json );
        if ( doc.HasParseError() )
            return false;

        if ( !read_uint32_from_string( doc, "protocolId", connectToken.protocolId ) )
            return false;

        if ( !read_uint64_from_string( doc, "clientId", connectToken.clientId ) )
            return false;

        if ( !read_uint64_from_string( doc, "expiryTimestamp", connectToken.expiryTimestamp ) )
            return false;

        if ( !read_int_from_string( doc, "numServerAddresses", connectToken.numServerAddresses ) )
            return false;

        if ( connectToken.numServerAddresses < 0 || connectToken.numServerAddresses > MaxServersPerConnectToken )
            return false;

        const Value & serverAddresses = doc["serverAddresses"];

        if ( !serverAddresses.IsArray() )
            return false;

        if ( (int) serverAddresses.Size() != connectToken.numServerAddresses )
            return false;

        for ( SizeType i = 0; i < serverAddresses.Size(); ++i )
        {
            if ( !serverAddresses[i].IsString() )
                return false;

            const char * string = serverAddresses[i].GetString();

            const int string_length = (int) strlen( string );

            const int MaxStringLength = 128;

            if ( string_length > 128 )
                return false;

            char buffer[MaxStringLength*2];

            base64_decode_string( string, buffer, sizeof( buffer ) );

            connectToken.serverAddresses[i] = Address( buffer );

            if ( !connectToken.serverAddresses[i].IsValid() )
                return false;
        }

        if ( !read_data_from_base64_string( doc, "clientToServerKey", connectToken.clientToServerKey, KeyBytes ) )
            return false;

        if ( !read_data_from_base64_string( doc, "serverToClientKey", connectToken.serverToClientKey, KeyBytes ) )
            return false;

        if ( !read_data_from_base64_string( doc, "random", connectToken.random, KeyBytes ) )
            return false;

        return true;
    }

    bool GenerateChallengeToken( const ConnectToken & connectToken, const uint8_t * connectTokenMac, ChallengeToken & challengeToken )
    {
        if ( connectToken.clientId == 0 )
            return false;

        challengeToken.clientId = connectToken.clientId;

        memcpy( challengeToken.connectTokenMac, connectTokenMac, MacBytes );

        memcpy( challengeToken.clientToServerKey, connectToken.clientToServerKey, KeyBytes );

        memcpy( challengeToken.serverToClientKey, connectToken.serverToClientKey, KeyBytes );

        GenerateKey( challengeToken.random );

        return true;
    }

    bool EncryptChallengeToken( ChallengeToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key )
    {
        uint8_t message[ChallengeTokenBytes - AuthBytes];
        memset( message, 0, ChallengeTokenBytes - AuthBytes );
        WriteStream stream( message, ChallengeTokenBytes - AuthBytes );
        if ( !token.Serialize( stream ) )
            return false;

        stream.Flush();
        
        if ( stream.GetError() )
            return false;

        uint64_t encryptedLength;

        if ( !Encrypt_AEAD( message, ChallengeTokenBytes - AuthBytes, encryptedMessage, encryptedLength, additional, additionalLength, nonce, key ) )
            return false;

        assert( encryptedLength == ChallengeTokenBytes );

        return true;
    }

    bool DecryptChallengeToken( const uint8_t * encryptedMessage, ChallengeToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key )
    {
        const int encryptedMessageLength = ChallengeTokenBytes;

        uint64_t decryptedMessageLength;
        uint8_t decryptedMessage[ChallengeTokenBytes];

        if ( !Decrypt_AEAD( encryptedMessage, encryptedMessageLength, decryptedMessage, decryptedMessageLength, additional, additionalLength, nonce, key ) )
            return false;

        assert( decryptedMessageLength == ChallengeTokenBytes - AuthBytes );

        ReadStream stream( decryptedMessage, ChallengeTokenBytes - AuthBytes );
        if ( !decryptedToken.Serialize( stream ) )
            return false;

        if ( stream.GetError() )
            return false;

        return true;
    }

    // =============================================================

    const char * GetClientStateName( int clientState )
    {
        switch ( clientState )
        {
#if YOJIMBO_INSECURE_CONNECT
            case CLIENT_STATE_INSECURE_CONNECT_TIMEOUT:         return "insecure connect timeout";
#endif // #if YOJIMBO_INSECURE_CONNECT
            case CLIENT_STATE_PACKET_FACTORY_ERROR:             return "packet factory error";
            case CLIENT_STATE_MESSAGE_FACTORY_ERROR:            return "message factory error";
            case CLIENT_STATE_STREAM_ALLOCATOR_ERROR:           return "stream allocator error";
            case CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT:       return "connection request timeout";
            case CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT:       return "challenge response timeout";
            case CLIENT_STATE_CONNECTION_TIMEOUT:               return "connection timeout";
            case CLIENT_STATE_CONNECTION_ERROR:                 return "connection error";
            case CLIENT_STATE_CONNECTION_DENIED:                return "connection denied";
            case CLIENT_STATE_DISCONNECTED:                     return "disconnected";
#if YOJIMBO_INSECURE_CONNECT
            case CLIENT_STATE_SENDING_INSECURE_CONNECT:         return "sending insecure connect";
#endif // #if YOJIMBO_INSECURE_CONNECT
            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:       return "sending connection request";
            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:       return "sending challenge response";
            case CLIENT_STATE_CONNECTED:                        return "connected";
            default:
                assert( false );
                return "???";
        }
    }

    void Client::Defaults()
    {
        m_allocator = NULL;
        m_streamAllocator = NULL;
        m_transport = NULL;
        m_messageFactory = NULL;
        m_allocateConnection = false;
        m_connection = NULL;
        m_time = 0.0;
        m_clientState = CLIENT_STATE_DISCONNECTED;
        m_clientIndex = -1;
        m_lastPacketSendTime = 0.0;
        m_lastPacketReceiveTime = 0.0;
        m_clientSalt = 0;
        m_sequence = 0;
    }

    Client::Client( Allocator & allocator, Transport & transport, const ClientServerConfig & config )
    {
        Defaults();
        m_allocator = &allocator;
        m_transport = &transport;
        m_config = config;
        m_config.connectionConfig.connectionPacketType = CLIENT_SERVER_PACKET_CONNECTION;
        m_allocateConnection = m_config.enableConnection;
    }

    Client::~Client()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        assert( !IsConnected() );

        assert( m_allocator );

        YOJIMBO_DELETE( *m_allocator, Connection, m_connection );

        YOJIMBO_DELETE( *m_allocator, MessageFactory, m_messageFactory );

        YOJIMBO_DELETE( *m_allocator, Allocator, m_streamAllocator );

        m_messageFactory = NULL;
        m_transport = NULL;
        m_allocator = NULL;
    }

#if YOJIMBO_INSECURE_CONNECT

    void Client::InsecureConnect( const Address & address )
    {
        Disconnect();

        InitializeConnection();

        m_serverAddress = address;

        OnConnect( address );

        SetClientState( CLIENT_STATE_SENDING_INSECURE_CONNECT );

        const double time = GetTime();        

        m_lastPacketSendTime = time - 1.0f;
        m_lastPacketReceiveTime = time;

        RandomBytes( (uint8_t*) &m_clientSalt, sizeof( m_clientSalt ) );

        m_transport->ResetEncryptionMappings();
    }

#endif // #if YOJIMBO_INSECURE_CONNECT

    void Client::Connect( const Address & address, 
                          const uint8_t * connectTokenData, 
                          const uint8_t * connectTokenNonce,
                          const uint8_t * clientToServerKey,
                          const uint8_t * serverToClientKey )
    {
        Disconnect();

        InitializeConnection();

        m_serverAddress = address;

        SetEncryptedPacketTypes();

        OnConnect( address );

        SetClientState( CLIENT_STATE_SENDING_CONNECTION_REQUEST );

        const double time = GetTime();        

        m_lastPacketSendTime = time - 1.0f;
        m_lastPacketReceiveTime = time;
        memcpy( m_connectTokenData, connectTokenData, ConnectTokenBytes );
        memcpy( m_connectTokenNonce, connectTokenNonce, NonceBytes );

        m_transport->AddEncryptionMapping( m_serverAddress, clientToServerKey, serverToClientKey );
    }

    void Client::Disconnect( int clientState, bool sendDisconnectPacket )
    {
        assert( clientState <= CLIENT_STATE_DISCONNECTED );

        if ( m_clientState != clientState )
            OnDisconnect();

        if ( sendDisconnectPacket && m_clientState > CLIENT_STATE_DISCONNECTED )
        {
            for ( int i = 0; i < m_config.numDisconnectPackets; ++i )
            {
                ConnectionDisconnectPacket * packet = (ConnectionDisconnectPacket*) m_transport->CreatePacket( CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT );            

                if ( packet )
                {
                    SendPacketToServer_Internal( packet, true );
                }
            }
        }

        ResetConnectionData( clientState );

        m_transport->ResetEncryptionMappings();
    }

    Message * Client::CreateMessage( int type )
    {
        assert( m_messageFactory );
        return m_messageFactory->Create( type );
    }

    bool Client::CanSendMessage()
    {
        if ( !IsConnected() )
            return false;

        assert( m_messageFactory );
        assert( m_connection );
        
        return m_connection->CanSendMessage();
    }

    void Client::SendMessage( Message * message )
    {
        assert( IsConnected() );
        assert( m_messageFactory );
        assert( m_connection );
        m_connection->SendMessage( message );
    }

    Message * Client::ReceiveMessage()
    {
        assert( m_messageFactory );

        if ( !IsConnected() )
            return NULL;

        assert( m_connection );

        return m_connection->ReceiveMessage();
    }

    void Client::ReleaseMessage( Message * message )
    {
        assert( message );
        assert( m_messageFactory );
        m_messageFactory->Release( message );
    }

    MessageFactory & Client::GetMessageFactory()
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

    void Client::SendPackets()
    {
        const double time = GetTime();

        switch ( m_clientState )
        {
#if YOJIMBO_INSECURE_CONNECT

            case CLIENT_STATE_SENDING_INSECURE_CONNECT:
            {
                if ( m_lastPacketSendTime + m_config.insecureConnectSendRate > time )
                    return;

                InsecureConnectPacket * packet = (InsecureConnectPacket*) m_transport->CreatePacket( CLIENT_SERVER_PACKET_INSECURE_CONNECT );
                if ( packet )
                {
                    packet->clientSalt = m_clientSalt;
                    SendPacketToServer_Internal( packet );
                }
            }
            break;

#endif // #if YOJIMBO_INSECURE_CONNECT

            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
            {
                if ( m_lastPacketSendTime + m_config.connectionRequestSendRate > time )
                    return;

                ConnectionRequestPacket * packet = (ConnectionRequestPacket*) m_transport->CreatePacket( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );
                if ( packet )
                {
                    memcpy( packet->connectTokenData, m_connectTokenData, ConnectTokenBytes );
                    memcpy( packet->connectTokenNonce, m_connectTokenNonce, NonceBytes );

                    SendPacketToServer_Internal( packet );
                }
            }
            break;

            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
            {
                if ( m_lastPacketSendTime + m_config.connectionResponseSendRate > time )
                    return;

                ConnectionResponsePacket * packet = (ConnectionResponsePacket*) m_transport->CreatePacket( CLIENT_SERVER_PACKET_CONNECTION_RESPONSE );
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

                if ( m_lastPacketSendTime + m_config.connectionHeartBeatRate <= time )
                {
                    ConnectionHeartBeatPacket * packet = (ConnectionHeartBeatPacket*) m_transport->CreatePacket( CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT );

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

        switch ( m_clientState )
        {
#if YOJIMBO_INSECURE_CONNECT

            case CLIENT_STATE_SENDING_INSECURE_CONNECT:
            {
                if ( m_lastPacketReceiveTime + m_config.insecureConnectTimeOut < time )
                {
                    Disconnect( CLIENT_STATE_INSECURE_CONNECT_TIMEOUT, false );
                    return;
                }
            }
            break;

#endif // #if YOJIMBO_INSECURE_CONNECT

            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
            {
                if ( m_lastPacketReceiveTime + m_config.connectionRequestTimeOut < time )
                {
                    Disconnect( CLIENT_STATE_CONNECTION_REQUEST_TIMEOUT, false );
                    return;
                }
            }
            break;

            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
            {
                if ( m_lastPacketReceiveTime + m_config.challengeResponseTimeOut < time )
                {
                    Disconnect( CLIENT_STATE_CHALLENGE_RESPONSE_TIMEOUT, false );
                    return;
                }
            }
            break;

            case CLIENT_STATE_CONNECTED:
            {
                if ( m_lastPacketReceiveTime + m_config.connectionTimeOut < time )
                {
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
        assert( time >= m_time );

        m_time = time;

        if ( m_streamAllocator && m_streamAllocator->GetError() )
        {
            Disconnect( CLIENT_STATE_STREAM_ALLOCATOR_ERROR, true );
            m_streamAllocator->ClearError();
            return;
        }

        if ( m_messageFactory && m_messageFactory->GetError() )
        {
            Disconnect( CLIENT_STATE_MESSAGE_FACTORY_ERROR, true );
            m_messageFactory->ClearError();
            return;
        }

        PacketFactory * packetFactory = m_transport->GetPacketFactory();
        if ( packetFactory && packetFactory->GetError() )
        {
            Disconnect( CLIENT_STATE_PACKET_FACTORY_ERROR, true );
            packetFactory->ClearError();
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

    int Client::GetClientIndex() const
    {
        return m_clientIndex;
    }

    void Client::InitializeConnection()
    {
        // todo: this is a bit rough. maybe clean up, or make this non-virtual. if the client wants to create their own context, they have no way to do it.

        if ( !m_streamAllocator )
        {
            m_streamAllocator = CreateStreamAllocator();
            m_transport->SetStreamAllocator( *m_streamAllocator );
        }

        if ( m_config.enableConnection )
        {
            if ( m_allocateConnection && !m_connection )
            {
                m_messageFactory = CreateMessageFactory();
                m_connection = YOJIMBO_NEW( *m_allocator, Connection, *m_allocator, *m_transport->GetPacketFactory(), *m_messageFactory, m_config.connectionConfig );
                m_connection->SetListener( this );
            }

            m_context.messageFactory = m_messageFactory;
            m_context.connectionConfig = &m_config.connectionConfig;
            
            m_transport->SetContext( &m_context );
        }
        else
        {
            m_transport->SetContext( NULL );
        }
    }

    void Client::SetEncryptedPacketTypes()
    {
        m_transport->EnablePacketEncryption();
        m_transport->DisableEncryptionForPacketType( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );
    }

    Allocator * Client::CreateStreamAllocator()
    {
        return YOJIMBO_NEW( *m_allocator, DefaultAllocator );
    }

    PacketFactory * Client::CreatePacketFactory()
    {
        return YOJIMBO_NEW( *m_allocator, ClientServerPacketFactory, *m_allocator );
    }

    MessageFactory * Client::CreateMessageFactory()
    {
        assert( !"you need to override Client::CreateMessageFactory if you want to use messages" );
        return NULL;
    }

    void Client::SetClientState( int clientState )
    {
        const int previous = m_clientState;
        m_clientState = (ClientState) clientState;
        if ( clientState != previous )
            OnClientStateChange( previous, clientState );
    }

    void Client::ResetConnectionData( int clientState )
    {
        assert( m_transport );
        m_clientIndex = -1;
        m_serverAddress = Address();
        SetClientState( clientState );
        m_lastPacketSendTime = -1000.0;
        m_lastPacketReceiveTime = -1000.0;
        memset( m_connectTokenData, 0, ConnectTokenBytes );
        memset( m_connectTokenNonce, 0, NonceBytes );
        memset( m_challengeTokenData, 0, ChallengeTokenBytes );
        memset( m_challengeTokenNonce, 0, NonceBytes );
        m_transport->ResetEncryptionMappings();
        m_sequence = 0;
#if YOJIMBO_INSECURE_CONNECT
        m_clientSalt = 0;
#endif // #if YOJIMBO_INSECURE_CONNECT
        if ( m_connection )
        {
            m_connection->Reset();
        }
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

        SetClientState( CLIENT_STATE_CONNECTION_DENIED );
    }

    void Client::ProcessConnectionChallenge( const ConnectionChallengePacket & packet, const Address & address )
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
#if YOJIMBO_INSECURE_CONNECT
        return m_clientState == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE || m_clientState == CLIENT_STATE_SENDING_INSECURE_CONNECT;
#else // #if YOJIMBO_INSECURE_CONNECT
        return m_clientState == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE;
#endif // #if YOJIMBO_INSECURE_CONNECT
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

#if YOJIMBO_INSECURE_CONNECT

        if ( m_clientState == CLIENT_STATE_SENDING_INSECURE_CONNECT )
        {
            m_clientIndex = clientIndex;

            SetClientState( CLIENT_STATE_CONNECTED );
        }

#endif // #if YOJIMBO_INSECURE_CONNECT
    }

    void Client::ProcessConnectionHeartBeat( const ConnectionHeartBeatPacket & packet, const Address & address )
    {
        if ( !IsPendingConnect() && !IsConnected() )
            return;

        if ( address != m_serverAddress )
            return;

        if ( IsPendingConnect() )
            CompletePendingConnect( packet.clientIndex );

        m_lastPacketReceiveTime = GetTime();
    }

    void Client::ProcessConnectionDisconnect( const ConnectionDisconnectPacket & /*packet*/, const Address & address )
    {
        if ( m_clientState != CLIENT_STATE_CONNECTED )
            return;

        if ( address != m_serverAddress )
            return;

        Disconnect( CLIENT_STATE_DISCONNECTED, false );
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

    void Client::ProcessPacket( Packet * packet, const Address & address, uint64_t sequence )
    {
        OnPacketReceived( packet->GetType(), address, sequence );
        
        switch ( packet->GetType() )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_DENIED:
                ProcessConnectionDenied( *(ConnectionDeniedPacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE:
                ProcessConnectionChallenge( *(ConnectionChallengePacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT:
                ProcessConnectionHeartBeat( *(ConnectionHeartBeatPacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT:
                ProcessConnectionDisconnect( *(ConnectionDisconnectPacket*)packet, address );
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

        if ( !ProcessGamePacket( packet, sequence ) )
            return;

        m_lastPacketReceiveTime = GetTime();
    }

    // =============================================================

    void Server::Defaults()
    {
        m_allocator = NULL;
        m_globalStreamAllocator = NULL;
        m_transport = NULL;
        m_allocateConnections = false;
        m_time = 0.0;
        m_flags = 0;
        m_maxClients = -1;
        m_numConnectedClients = 0;
        m_challengeTokenNonce = 0;
        m_globalSequence = 0;
        m_globalContext = NULL;
        memset( m_privateKey, 0, KeyBytes );
        memset( m_clientStreamAllocator, 0, sizeof( m_clientStreamAllocator ) );
        memset( m_clientContext, 0, sizeof( m_clientContext ) );
        memset( m_clientMessageFactory, 0, sizeof( m_clientMessageFactory ) );
        memset( m_clientPacketFactory, 0, sizeof( m_clientPacketFactory ) );
        memset( m_connection, 0, sizeof( m_connection ) );
        memset( m_clientSequence, 0, sizeof( m_clientSequence ) );
        memset( m_counters, 0, sizeof( m_counters ) );
        for ( int i = 0; i < MaxClients; ++i )
            ResetClientState( i );
    }

    Server::Server( Allocator & allocator, Transport & transport, const ClientServerConfig & config )
    {
        Defaults();
        m_allocator = &allocator;
        m_transport = &transport;
        m_config = config;
        m_config.connectionConfig.connectionPacketType = CLIENT_SERVER_PACKET_CONNECTION;
        m_allocateConnections = m_config.enableConnection;
    }

    Server::~Server()
    {
		// IMPORTANT: Please call stop on the server before you destroy it. 
		assert( !IsRunning() );

        YOJIMBO_DELETE( *m_allocator, Allocator, m_globalStreamAllocator );

        assert( m_transport );

        m_transport = NULL;
    }

    void Server::SetPrivateKey( const uint8_t * privateKey )
    {
        memcpy( m_privateKey, privateKey, KeyBytes );
    }

    void Server::SetServerAddress( const Address & address )
    {
        m_serverAddress = address;
    }

    void Server::Start( int maxClients )
    {
        assert( maxClients > 0 );
        assert( maxClients <= MaxClients );

        Stop();

        m_maxClients = maxClients;

        if ( !m_globalStreamAllocator )
        {
            m_globalStreamAllocator = CreateStreamAllocator( SERVER_RESOURCE_GLOBAL, -1 );

            m_transport->SetStreamAllocator( *m_globalStreamAllocator );
        }

        for ( int clientIndex = 0; clientIndex < m_maxClients; ++clientIndex )
        {
            m_clientPacketFactory[clientIndex] = CreatePacketFactory( clientIndex );

            assert( m_clientPacketFactory[clientIndex] );
            assert( m_clientPacketFactory[clientIndex]->GetNumPacketTypes() == m_transport->GetPacketFactory()->GetNumPacketTypes() );

            m_clientStreamAllocator[clientIndex] = CreateStreamAllocator( SERVER_RESOURCE_PER_CLIENT, clientIndex );
        }

        if ( m_allocateConnections )
        {
            for ( int clientIndex = 0; clientIndex < m_maxClients; ++clientIndex )
            {
                m_clientMessageFactory[clientIndex] = CreateMessageFactory( clientIndex );
                
                m_connection[clientIndex] = YOJIMBO_NEW( *m_allocator, Connection, *m_allocator, *m_clientPacketFactory[clientIndex], *m_clientMessageFactory[clientIndex], m_config.connectionConfig );
               
                m_connection[clientIndex]->SetListener( this );

                m_connection[clientIndex]->SetClientIndex( clientIndex );

                m_clientContext[clientIndex] = CreateContext( SERVER_RESOURCE_PER_CLIENT, clientIndex );

                assert( m_clientContext[clientIndex]->magic == ConnectionContextMagic );
            }
        }

        SetEncryptedPacketTypes();

        InitializeGlobalContext();

        OnStart( maxClients );
    }

    void Server::Stop()
    {
        if ( !IsRunning() )
            return;

        OnStop();

        DisconnectAllClients();

        m_transport->Reset();

        for ( int clientIndex = 0; clientIndex < m_maxClients; ++clientIndex )
        {
            YOJIMBO_DELETE( *m_allocator, ClientServerContext, m_clientContext[clientIndex] );

            YOJIMBO_DELETE( *m_allocator, Connection, m_connection[clientIndex] );

            YOJIMBO_DELETE( *m_allocator, PacketFactory, m_clientPacketFactory[clientIndex] );

            YOJIMBO_DELETE( *m_allocator, MessageFactory, m_clientMessageFactory[clientIndex] );

            YOJIMBO_DELETE( *m_allocator, Allocator, m_clientStreamAllocator[clientIndex] );
        }

        m_maxClients = -1;
    }

    void Server::DisconnectClient( int clientIndex, bool sendDisconnectPacket )
    {
        assert( IsRunning() );
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( m_numConnectedClients > 0 );
        assert( m_clientConnected[clientIndex] );

        OnClientDisconnect( clientIndex );

        if ( sendDisconnectPacket )
        {
            for ( int i = 0; i < m_config.numDisconnectPackets; ++i )
            {
                ConnectionDisconnectPacket * packet = (ConnectionDisconnectPacket*) CreateGlobalPacket( CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT );

                if ( packet )
                {
                    SendPacketToConnectedClient( clientIndex, packet, true );
                }
            }
        }

        m_transport->RemoveContextMapping( m_clientData[clientIndex].address );

        m_transport->RemoveEncryptionMapping( m_clientData[clientIndex].address );

        ResetClientState( clientIndex );

        m_counters[SERVER_COUNTER_CLIENT_DISCONNECTS]++;

        m_numConnectedClients--;
    }

    void Server::DisconnectAllClients( bool sendDisconnectPacket )
    {
        assert( IsRunning() );

        for ( int i = 0; i < m_maxClients; ++i )
        {
            if ( m_clientConnected[i] )
                DisconnectClient( i, sendDisconnectPacket );
        }
    }

    Message * Server::CreateMessage( int clientIndex, int type )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( m_clientMessageFactory[clientIndex] );
        return m_clientMessageFactory[clientIndex]->Create( type );
    }

    bool Server::CanSendMessage( int clientIndex ) const
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );

        if ( !IsRunning() )
            return false;

        if ( !IsClientConnected( clientIndex ) )
            return false;

        assert( m_clientMessageFactory );
        assert( m_connection[clientIndex] );

        return m_connection[clientIndex]->CanSendMessage();
    }

    void Server::SendMessage( int clientIndex, Message * message )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( m_clientMessageFactory[clientIndex] );

        if ( !m_clientConnected[clientIndex] )
        {
            m_clientMessageFactory[clientIndex]->Release( message );
            return;
        }

        assert( m_connection[clientIndex] );

        m_connection[clientIndex]->SendMessage( message );
    }

    Message * Server::ReceiveMessage( int clientIndex )
    {
        assert( m_clientMessageFactory );

        if ( !m_clientConnected[clientIndex] )
            return NULL;

        assert( m_connection[clientIndex] );

        return m_connection[clientIndex]->ReceiveMessage();
    }

    void Server::ReleaseMessage( int clientIndex, Message * message )
    {
        assert( message );
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( m_clientMessageFactory[clientIndex] );
        m_clientMessageFactory[clientIndex]->Release( message );
    }

    MessageFactory & Server::GetMessageFactory( int clientIndex )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( m_clientMessageFactory[clientIndex] );
        return *m_clientMessageFactory[clientIndex];
    }

    Packet * Server::CreateGlobalPacket( int type )
    {
        return m_transport->CreatePacket( type );
    }

    Packet * Server::CreateClientPacket( int clientIndex, int type )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( m_clientPacketFactory[clientIndex] );
        return m_clientPacketFactory[clientIndex]->CreatePacket( type );
    }

    void Server::SendPackets()
    {
        if ( !IsRunning() )
            return;

        const double time = GetTime();

        for ( int i = 0; i < m_maxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                continue;

            if ( m_clientData[i].fullyConnected )
            {
                if ( m_connection[i] )
                {
                    ConnectionPacket * packet = m_connection[i]->GeneratePacket();

                    if ( packet )
                    {
                        SendPacketToConnectedClient( i, packet );
                    }
                }

                if ( m_clientData[i].lastPacketSendTime + m_config.connectionHeartBeatRate <= time )
                {
                    ConnectionHeartBeatPacket * packet = CreateHeartBeatPacket( i );

                    if ( packet )
                    {
                        SendPacketToConnectedClient( i, packet );

                        m_clientData[i].lastHeartBeatSendTime = GetTime();
                    }
                }
            }
            else
            {
                if ( m_clientData[i].lastHeartBeatSendTime + m_config.connectionHeartBeatRate <= time )
                {
                    ConnectionHeartBeatPacket * packet = CreateHeartBeatPacket( i );

                    if ( packet )
                    {
                        SendPacketToConnectedClient( i, packet );

                        m_clientData[i].lastHeartBeatSendTime = GetTime();
                    }
                }
            }
        }
    }

    void Server::ReceivePackets()
    {
        while ( true )
        {
            Address address;
            uint64_t sequence;
            Packet * packet = m_transport->ReceivePacket( address, &sequence );

            if ( !packet )
                break;

            if ( IsRunning() )
                ProcessPacket( packet, address, sequence );

            packet->Destroy();
        }
    }

    void Server::CheckForTimeOut()
    {
        if ( !IsRunning() )
            return;

        const double time = GetTime();

        for ( int clientIndex = 0; clientIndex < m_maxClients; ++clientIndex )
        {
            if ( !m_clientConnected[clientIndex] )
                continue;

            if ( m_clientData[clientIndex].lastPacketReceiveTime + m_config.connectionTimeOut < time )
            {
                printf( "client timed out\n" );

                OnClientError( clientIndex, SERVER_CLIENT_ERROR_TIMEOUT );

                m_counters[SERVER_COUNTER_CLIENT_TIMEOUTS]++;

                DisconnectClient( clientIndex, false );
            }
        }
    }

    void Server::AdvanceTime( double time )
    {
        assert( time >= m_time );

        m_time = time;

        // check for global stream allocator error, increase counter and clear error. nothing we can do but take note.

        if ( m_globalStreamAllocator->GetError() )
        {
            m_counters[SERVER_COUNTER_GLOBAL_STREAM_ALLOCATOR_ERRORS]++;

            m_globalStreamAllocator->ClearError();
        }

        // check for global packet factory error, increase counter and clear error. nothing we can do but take note.

		PacketFactory * globalPacketFactory = m_transport->GetPacketFactory();

        if ( globalPacketFactory->GetError() )
        {
            m_counters[SERVER_COUNTER_GLOBAL_PACKET_FACTORY_ERRORS]++;

            globalPacketFactory->ClearError();
        }

        for ( int clientIndex = 0; clientIndex < m_maxClients; ++clientIndex )
        {
            if ( IsClientConnected( clientIndex ) )
            {
                // check for stream allocator error

                if ( m_clientStreamAllocator[clientIndex]->GetError() )
                {
                    OnClientError( clientIndex, SERVER_CLIENT_ERROR_STREAM_ALLOCATOR );

                    m_counters[SERVER_COUNTER_CLIENT_STREAM_ALLOCATOR_ERRORS]++;

                    DisconnectClient( clientIndex, true );
                }

                // check for message factory error

                if ( m_clientMessageFactory[clientIndex] )
                {
                    if ( m_clientMessageFactory[clientIndex]->GetError() )
                    {
                        OnClientError( clientIndex, SERVER_CLIENT_ERROR_MESSAGE_FACTORY );

                        m_counters[SERVER_COUNTER_CLIENT_MESSAGE_FACTORY_ERRORS]++;

                        DisconnectClient( clientIndex, true );
                    }
                }

                // check for packet factory error

                if ( m_clientPacketFactory[clientIndex]->GetError() )
                {
                    OnClientError( clientIndex, SERVER_CLIENT_ERROR_PACKET_FACTORY );

                    m_counters[SERVER_COUNTER_CLIENT_PACKET_FACTORY_ERRORS]++;

                    DisconnectClient( clientIndex, true );
                }

                // check for connection error

                if ( m_connection[clientIndex] )
                {
                    m_connection[clientIndex]->AdvanceTime( time );

                    if ( m_connection[clientIndex]->GetError() )
                    {
                        OnClientError( clientIndex, SERVER_CLIENT_ERROR_CONNECTION );

                        m_counters[SERVER_COUNTER_CLIENT_CONNECTION_ERRORS]++;

                        DisconnectClient( clientIndex, true );
                    }
                }
            }
        }
    }

    void Server::SetFlags( uint64_t flags )
    {
        m_flags = flags;
    }

    bool Server::IsRunning() const
    {
        return m_maxClients > 0;
    }

    int Server::GetMaxClients() const
    {
        return m_maxClients;
    }

    bool Server::IsClientConnected( int clientIndex ) const
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        return m_clientConnected[clientIndex];
    }

    const Address & Server::GetServerAddress() const
    {
        return m_serverAddress;
    }

    int Server::FindClientIndex( const Address & address ) const
    {
        if ( !address.IsValid() )
            return -1;

        for ( int i = 0; i < m_maxClients; ++i )
        {   
            if ( m_clientConnected[i] && m_clientAddress[i] == address )
                return i;
        }

        return -1;
    }

    uint64_t Server::GetClientId( int clientIndex ) const
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        return m_clientId[clientIndex];
    }

    const Address & Server::GetClientAddress( int clientIndex ) const
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        return m_clientAddress[clientIndex];
    }

    int Server::GetNumConnectedClients() const
    {
        return m_numConnectedClients;
    }

    uint64_t Server::GetCounter( int index ) const 
    {
        assert( index >= 0 );
        assert( index < SERVER_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }

    double Server::GetTime() const
    {
        return m_time;
    }

    uint64_t Server::GetFlags() const
    {
        return m_flags;
    }

    void Server::InitializeGlobalContext()
    {
        m_globalContext = CreateContext( SERVER_RESOURCE_GLOBAL, -1 );
        assert( m_globalContext );
        assert( m_globalContext->magic == ConnectionContextMagic );
        m_transport->SetContext( m_globalContext );
    }

    void Server::SetEncryptedPacketTypes()
    {
        m_transport->EnablePacketEncryption();
        m_transport->DisableEncryptionForPacketType( CLIENT_SERVER_PACKET_CONNECTION_REQUEST );
    }

    Allocator * Server::CreateStreamAllocator( ServerResourceType /*type*/, int /*clientIndex*/ )
    {
        return YOJIMBO_NEW( *m_allocator, DefaultAllocator );
    }

    PacketFactory * Server::CreatePacketFactory( int /*clientIndex*/ )
    {
        return YOJIMBO_NEW( *m_allocator, ClientServerPacketFactory, *m_allocator );
    }

    MessageFactory * Server::CreateMessageFactory( int /*clientIndex*/ )
    {
        assert( !"you need to override Server::CreateMessageFactory if you want to use messages" );
        return NULL;
    }

    ClientServerContext * Server::CreateContext( ServerResourceType type, int clientIndex )
    {
        ClientServerContext * context = YOJIMBO_NEW( *m_allocator, ClientServerContext );
        
        assert( context );
        assert( context->magic == ConnectionContextMagic );

        context->connectionConfig = &GetConnectionConfig();
        
        if ( type == SERVER_RESOURCE_PER_CLIENT )
        {
            assert( clientIndex >= 0 );
            assert( clientIndex < m_maxClients );
            context->messageFactory = &GetMessageFactory( clientIndex );
        }
        
        return context;
    }

    void Server::ResetClientState( int clientIndex )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < MaxClients );
        m_clientConnected[clientIndex] = false;
        m_clientId[clientIndex] = 0;
        m_clientAddress[clientIndex] = Address();
        m_clientData[clientIndex] = ServerClientData();
        m_clientSequence[clientIndex] = 0;
    }

    int Server::FindFreeClientIndex() const
    {
        for ( int i = 0; i < m_maxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                return i;
        }
        return -1;
    }

    int Server::FindExistingClientIndex( const Address & address ) const
    {
        for ( int i = 0; i < m_maxClients; ++i )
        {
            if ( m_clientConnected[i] && m_clientAddress[i] == address )
                return i;
        }
        return -1;
    }

    int Server::FindExistingClientIndex( const Address & address, uint64_t clientId ) const
    {
        for ( int i = 0; i < m_maxClients; ++i )
        {
            if ( m_clientId[i] == clientId && m_clientConnected[i] && m_clientAddress[i] == address )
                return i;
        }
        return -1;
    }

    bool Server::FindConnectTokenEntry( const uint8_t * mac )
    {
        for ( int i = 0; i < MaxConnectTokenEntries; ++i )
        {
            if ( memcmp( mac, m_connectTokenEntries[i].mac, MacBytes ) == 0 )
                return true;
        }

        return false;
    }

    bool Server::FindOrAddConnectTokenEntry( const Address & address, const uint8_t * mac )
    {
        // find the matching entry for the token mac, and the oldest token. constant time worst case O(1) at all times. This is intentional!

        const double time = GetTime();

        assert( address.IsValid() );

        assert( mac );

        int matchingTokenIndex = -1;
        int oldestTokenIndex = -1;
        double oldestTokenTime = 0.0;
        for ( int i = 0; i < MaxConnectTokenEntries; ++i )
        {
            if ( memcmp( mac, m_connectTokenEntries[i].mac, MacBytes ) == 0 )
            {
                matchingTokenIndex = i;
            }

            if ( oldestTokenIndex == -1 || m_connectTokenEntries[i].time < oldestTokenTime )
            {
                oldestTokenTime = m_connectTokenEntries[i].time;
                oldestTokenIndex = i;
            }
        }

        // if no entry is found with the mac, replace the oldest entry with this (mac,address,time) and return true

        assert( oldestTokenIndex != -1 );

        if ( matchingTokenIndex == -1 )
        {
            m_connectTokenEntries[oldestTokenIndex].time = time;
            m_connectTokenEntries[oldestTokenIndex].address = address;
            memcpy( m_connectTokenEntries[oldestTokenIndex].mac, mac, MacBytes );
            return true;
        }

        // if an entry is found with the same mac *and* it has the same address, return true

        assert( matchingTokenIndex >= 0 );
        assert( matchingTokenIndex < MaxConnectTokenEntries );

        if ( m_connectTokenEntries[matchingTokenIndex].address == address )
            return true;

        // otherwise an entry exists with the same mac but a different address, somebody is trying to reuse the connect token as a replay attack!

        return false;
    }

    void Server::ConnectClient( int clientIndex, const Address & clientAddress, uint64_t clientId )
    {
        assert( IsRunning() );
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( m_numConnectedClients >= 0 );
        assert( m_numConnectedClients < m_maxClients );
        assert( !m_clientConnected[clientIndex] );

        const double time = GetTime();

        m_counters[SERVER_COUNTER_CLIENT_CONNECTS]++;

        m_numConnectedClients++;

        m_clientConnected[clientIndex] = true;
        m_clientId[clientIndex] = clientId;
        m_clientAddress[clientIndex] = clientAddress;

        m_clientData[clientIndex].address = clientAddress;
        m_clientData[clientIndex].clientId = clientId;
        m_clientData[clientIndex].connectTime = time;
        m_clientData[clientIndex].lastPacketSendTime = time;
        m_clientData[clientIndex].lastPacketReceiveTime = time;
        m_clientData[clientIndex].fullyConnected = false;

        assert( m_clientPacketFactory[clientIndex] );
        assert( m_clientStreamAllocator[clientIndex] );

        m_transport->AddContextMapping( clientAddress, *m_clientStreamAllocator[clientIndex], *m_clientPacketFactory[clientIndex], m_clientContext[clientIndex] );

        OnClientConnect( clientIndex );

        ConnectionHeartBeatPacket * connectionHeartBeatPacket = CreateHeartBeatPacket( clientIndex );

        if ( connectionHeartBeatPacket )
        {
            SendPacketToConnectedClient( clientIndex, connectionHeartBeatPacket );
        }
    }

    int Server::FindClientId( uint64_t clientId ) const
    {
        for ( int i = 0; i < m_maxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                continue;
            if ( m_clientId[i] == clientId )
                return i;
        }
        return -1;
    }

    int Server::FindAddressAndClientId( const Address & address, uint64_t clientId ) const
    {
        for ( int i = 0; i < m_maxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                continue;
            if ( m_clientAddress[i] == address && m_clientId[i] == clientId )
                return i;
        }
        return -1;
    }

    void Server::SendPacket( const Address & address, Packet * packet, bool immediate )
    {
        assert( IsRunning() );

        m_transport->SendPacket( address, packet, ++m_globalSequence, immediate );

        OnPacketSent( packet->GetType(), address, immediate );
    }

    void Server::SendPacketToConnectedClient( int clientIndex, Packet * packet, bool immediate )
    {
        assert( IsRunning() );
        assert( packet );
        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );
        assert( m_clientConnected[clientIndex] );

        const double time = GetTime();
        
        m_clientData[clientIndex].lastPacketSendTime = time;
        
        m_transport->SendPacket( m_clientAddress[clientIndex], packet, ++m_clientSequence[clientIndex], immediate );
        
        OnPacketSent( packet->GetType(), m_clientAddress[clientIndex], immediate );
    }

    void Server::ProcessConnectionRequest( const ConnectionRequestPacket & packet, const Address & address )
    {
        assert( IsRunning() );

        if ( m_flags & SERVER_FLAG_IGNORE_CONNECTION_REQUESTS )
        {
            debug_printf( "ignored connection request because SERVER_FLAG_IGNORE_CONNECTION_REQUESTS is set on the server\n" );
            return;
        }

        m_counters[SERVER_COUNTER_CONNECTION_REQUEST_PACKETS_RECEIVED]++;

        ConnectToken connectToken;
        if ( !DecryptConnectToken( packet.connectTokenData, connectToken, NULL, 0, packet.connectTokenNonce, m_privateKey ) )
        {
            debug_printf( "failed to decrypt connection token\n" );
            m_counters[SERVER_COUNTER_CONNECT_TOKEN_FAILED_TO_DECRYPT]++;
            return;
        }

        bool serverAddressInConnectTokenWhiteList = false;

        for ( int i = 0; i < connectToken.numServerAddresses; ++i )
        {
            if ( m_serverAddress == connectToken.serverAddresses[i] )
            {
                serverAddressInConnectTokenWhiteList = true;
                break;
            }
        }

        if ( !serverAddressInConnectTokenWhiteList )
        {
            debug_printf( "server address not in connect token whitelist\n" );
            m_counters[SERVER_COUNTER_CONNECT_TOKEN_SERVER_ADDRESS_NOT_IN_WHITELIST]++;
            return;
        }

        if ( connectToken.clientId == 0 )
        {
            debug_printf( "client id is zero\n" );
            m_counters[SERVER_COUNTER_CONNECT_TOKEN_CLIENT_ID_IS_ZERO]++;
            return;
        }

        if ( FindAddressAndClientId( address, connectToken.clientId ) >= 0 )
        {
            debug_printf( "client id already connected: %" PRIx64 " (connection request)\n", connectToken.clientId );
            m_counters[SERVER_COUNTER_CONNECT_TOKEN_CLIENT_ID_ALREADY_CONNECTED]++;
            return;
        }

        uint64_t timestamp = (uint64_t) ::time( NULL );

        if ( connectToken.expiryTimestamp <= timestamp )
        {
            debug_printf( "connect token expired\n" );
            m_counters[SERVER_COUNTER_CONNECT_TOKEN_EXPIRED]++;
            return;
        }

        if ( !FindConnectTokenEntry( packet.connectTokenData ) )
        {
            if ( !m_transport->AddEncryptionMapping( address, connectToken.serverToClientKey, connectToken.clientToServerKey ) )
            {
                debug_printf( "failed to add encryption mapping\n" );
                m_counters[SERVER_COUNTER_ENCRYPTION_MAPPING_CANNOT_ADD]++;
                return;
            }
        }

        assert( m_numConnectedClients >= 0 );
        assert( m_numConnectedClients <= m_maxClients );

        if ( m_numConnectedClients == m_maxClients )
        {
            debug_printf( "server is full\n" );
            m_counters[SERVER_COUNTER_CONNECTION_DENIED_SERVER_IS_FULL]++;
            ConnectionDeniedPacket * connectionDeniedPacket = (ConnectionDeniedPacket*) m_transport->CreatePacket( CLIENT_SERVER_PACKET_CONNECTION_DENIED );
            if ( connectionDeniedPacket )
            {
                SendPacket( address, connectionDeniedPacket );
            }
            return;
        }

        if ( !FindOrAddConnectTokenEntry( address, packet.connectTokenData ) )
        {
            debug_printf( "find or add connect token entry failed\n" );
            m_counters[SERVER_COUNTER_CONNECT_TOKEN_ALREADY_USED]++;
            return;
        }

        ChallengeToken challengeToken;
        if ( !GenerateChallengeToken( connectToken, packet.connectTokenData, challengeToken ) )
        {
            debug_printf( "failed to generate challenge token\n" );
            m_counters[SERVER_COUNTER_CHALLENGE_TOKEN_FAILED_TO_GENERATE]++;
            return;
        }

        ConnectionChallengePacket * connectionChallengePacket = (ConnectionChallengePacket*) CreateGlobalPacket( CLIENT_SERVER_PACKET_CONNECTION_CHALLENGE );
        if ( !connectionChallengePacket )
        {
            debug_printf( "null connection challenge packet\n" );
            return;
        }

        memcpy( connectionChallengePacket->challengeTokenNonce, (uint8_t*) &m_challengeTokenNonce, NonceBytes );

        if ( !EncryptChallengeToken( challengeToken, connectionChallengePacket->challengeTokenData, NULL, 0, connectionChallengePacket->challengeTokenNonce, m_privateKey ) )
        {
            debug_printf( "failed to encrypt challenge token\n" );
            m_counters[SERVER_COUNTER_CHALLENGE_TOKEN_FAILED_TO_ENCRYPT]++;
            return;
        }

        m_counters[SERVER_COUNTER_CHALLENGE_PACKETS_SENT]++;

        SendPacket( address, connectionChallengePacket );
    }

    void Server::ProcessConnectionResponse( const ConnectionResponsePacket & packet, const Address & address )
    {
        assert( IsRunning() );

        if ( m_flags & SERVER_FLAG_IGNORE_CHALLENGE_RESPONSES )
            return;

        const double time = GetTime();

        m_counters[SERVER_COUNTER_CHALLENGE_RESPONSE_PACKETS_RECEIVED]++;

        ChallengeToken challengeToken;
        if ( !DecryptChallengeToken( packet.challengeTokenData, challengeToken, NULL, 0, packet.challengeTokenNonce, m_privateKey ) )
        {
            debug_printf( "failed to decrypt challenge token\n" );
            m_counters[SERVER_COUNTER_CHALLENGE_TOKEN_FAILED_TO_DECRYPT]++;
            return;
        }

        const int existingClientIndex = FindExistingClientIndex( address, challengeToken.clientId );
        if ( existingClientIndex != -1 )
        {
            assert( existingClientIndex >= 0 );
            assert( existingClientIndex < m_maxClients );

            if ( m_clientData[existingClientIndex].lastPacketSendTime + m_config.connectionConfirmSendRate < time )
            {
                ConnectionHeartBeatPacket * connectionHeartBeatPacket = CreateHeartBeatPacket( existingClientIndex );

                if ( connectionHeartBeatPacket )
                {
                    SendPacketToConnectedClient( existingClientIndex, connectionHeartBeatPacket );
                }
            }

            debug_printf( "client is already connected: %" PRIx64 " (challenge response)\n", challengeToken.clientId );

            return;
        }

        if ( m_numConnectedClients == m_maxClients )
        {
            m_counters[SERVER_COUNTER_CONNECTION_DENIED_SERVER_IS_FULL]++;
            ConnectionDeniedPacket * connectionDeniedPacket = (ConnectionDeniedPacket*) CreateGlobalPacket( CLIENT_SERVER_PACKET_CONNECTION_DENIED );
            if ( connectionDeniedPacket )
            {
                SendPacket( address, connectionDeniedPacket );
            }
            return;
        }

        const int clientIndex = FindFreeClientIndex();

        assert( clientIndex != -1 );
        if ( clientIndex == -1 )
        {
            debug_printf( "ignoring challenge response because there is no free client slot\n" );
            return;
        }

        ConnectClient( clientIndex, address, challengeToken.clientId );
    }

    void Server::ProcessConnectionHeartBeat( const ConnectionHeartBeatPacket & /*packet*/, const Address & address )
    {
        assert( IsRunning() );

        const int clientIndex = FindExistingClientIndex( address );
        if ( clientIndex == -1 )
            return;

        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );

        const double time = GetTime();
        
        m_clientData[clientIndex].lastPacketReceiveTime = time;

        m_clientData[clientIndex].fullyConnected = true;
    }

    void Server::ProcessConnectionDisconnect( const ConnectionDisconnectPacket & /*packet*/, const Address & address )
    {
        assert( IsRunning() );

        const int clientIndex = FindExistingClientIndex( address );
        if ( clientIndex == -1 )
            return;

        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );

        m_counters[SERVER_COUNTER_CLIENT_CLEAN_DISCONNECTS]++;

        DisconnectClient( clientIndex, false );
    }

#if YOJIMBO_INSECURE_CONNECT
    void Server::ProcessInsecureConnect( const InsecureConnectPacket & packet, const Address & address )
    {
        assert( IsRunning() );

        if ( ( GetFlags() & SERVER_FLAG_ALLOW_INSECURE_CONNECT ) == 0 )
            return;

        if ( m_numConnectedClients == m_maxClients )
        {
            m_counters[SERVER_COUNTER_CONNECTION_DENIED_SERVER_IS_FULL]++;
            ConnectionDeniedPacket * connectionDeniedPacket = (ConnectionDeniedPacket*) CreateGlobalPacket( CLIENT_SERVER_PACKET_CONNECTION_DENIED );
            if ( connectionDeniedPacket )
            {
                SendPacket( address, connectionDeniedPacket );
            }
            return;
        }

        const uint64_t clientSalt = packet.clientSalt;

        int clientIndex = FindExistingClientIndex( address );

        if ( clientIndex != -1 )
        {
            if ( m_clientData[clientIndex].clientSalt == clientSalt )
            {
                ConnectionHeartBeatPacket * connectionHeartBeatPacket = CreateHeartBeatPacket( clientIndex );

                if ( connectionHeartBeatPacket )
                {
                    SendPacketToConnectedClient( clientIndex, connectionHeartBeatPacket );
                }
            }

            return;            
        }

        clientIndex = FindFreeClientIndex();

        assert( clientIndex != -1 );
        if ( clientIndex == -1 )
            return;

        ConnectClient( clientIndex, address, 0 );

        m_clientData[clientIndex].clientSalt = clientSalt;
    }
#endif // #if YOJIMBO_INSECURE_CONNECT

    void Server::ProcessConnectionPacket( ConnectionPacket & packet, const Address & address )
    {
        const int clientIndex = FindExistingClientIndex( address );
        if ( clientIndex == -1 )
            return;

        assert( clientIndex >= 0 );
        assert( clientIndex < m_maxClients );

        if ( m_connection[clientIndex] )
            m_connection[clientIndex]->ProcessPacket( &packet );

        m_clientData[clientIndex].lastPacketReceiveTime = GetTime();

        m_clientData[clientIndex].fullyConnected = true;
    }

    void Server::ProcessPacket( Packet * packet, const Address & address, uint64_t sequence )
    {
        OnPacketReceived( packet->GetType(), address, sequence );
        
        switch ( packet->GetType() )
        {
            case CLIENT_SERVER_PACKET_CONNECTION_REQUEST:
                ProcessConnectionRequest( *(ConnectionRequestPacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_CONNECTION_RESPONSE:
                ProcessConnectionResponse( *(ConnectionResponsePacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT:
                ProcessConnectionHeartBeat( *(ConnectionHeartBeatPacket*)packet, address );
                return;

            case CLIENT_SERVER_PACKET_CONNECTION_DISCONNECT:
                ProcessConnectionDisconnect( *(ConnectionDisconnectPacket*)packet, address );
                return;

#if YOJIMBO_INSECURE_CONNECT
            case CLIENT_SERVER_PACKET_INSECURE_CONNECT:
                ProcessInsecureConnect( *(InsecureConnectPacket*)packet, address );
                return;
#endif // #if YOJIMBO_INSECURE_CONNECT

            case CLIENT_SERVER_PACKET_CONNECTION:
                ProcessConnectionPacket( *(ConnectionPacket*)packet, address );
                return;

            default:
                break;
        }

        const int clientIndex = FindClientIndex( address );

        if ( clientIndex == -1 )
            return;

        m_clientData[clientIndex].fullyConnected = true;

        if ( !ProcessGamePacket( clientIndex, packet, sequence ) )
            return;

        m_clientData[clientIndex].lastPacketReceiveTime = GetTime();
    }

    ConnectionHeartBeatPacket * Server::CreateHeartBeatPacket( int clientIndex )
    {
        ConnectionHeartBeatPacket * packet = (ConnectionHeartBeatPacket*) CreateClientPacket( clientIndex, CLIENT_SERVER_PACKET_CONNECTION_HEARTBEAT );

        if ( packet )
        {
            packet->clientIndex = clientIndex;
        }

        return packet;
    }
}
