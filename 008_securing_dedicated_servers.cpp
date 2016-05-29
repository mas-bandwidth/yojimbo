/*
    Example source code for "Securing Dedicated Servers"

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.

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

#include "yojimbo.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <map>

using namespace std;
using namespace yojimbo;
using namespace protocol2;
using namespace network2;

const uint32_t ProtocolId = 0x12341651;

const int MaxClients = 64;
const int ClientPort = 40000;
const int ServerPort = 50000;
const float ConnectionRequestSendRate = 0.1f;
const float ConnectionResponseSendRate = 0.1f;
const float ConnectionConfirmSendRate = 0.1f;
const float ConnectionHeartBeatRate = 1.0f;
const float ConnectionRequestTimeOut = 5.0f;
const float ChallengeResponseTimeOut = 5.0f;
const float ConnectionTimeOut = 10.0f;

const int ConnectTokenBytes = 1024;
const int ChallengeTokenBytes = 256;
const int MaxServersPerConnectToken = 8;
const int ConnectTokenExpirySeconds = 10;
const int MaxConnectTokenEntries = MaxClients * 16;

template <typename Stream> bool serialize_address_internal( Stream & stream, Address & address )
{
    char buffer[64];

    if ( Stream::IsWriting )
    {
        assert( address.IsValid() );
        address.ToString( buffer, sizeof( buffer ) );
    }

    serialize_string( stream, buffer, sizeof( buffer ) );

    if ( Stream::IsReading )
    {
        address = Address( buffer );
        if ( !address.IsValid() )
            return false;
    }

    return true;
}

#define serialize_address( stream, value )                          \
    do                                                              \
    {                                                               \
        if ( !serialize_address_internal( stream, value ) )         \
            return false;                                           \
    } while (0)

struct ConnectToken
{
    uint32_t protocolId;                                                // the protocol id this connect token corresponds to.
 
    uint64_t clientId;                                                  // the unique client id. max one connection per-client id, per-server.
 
    uint64_t expiryTimestamp;                                           // timestamp the connect token expires (eg. ~10 seconds after token creation)
 
    int numServerAddresses;                                             // the number of server addresses in the connect token whitelist.
 
    Address serverAddresses[MaxServersPerConnectToken];                 // connect token only allows connection to these server addresses.
 
    uint8_t clientToServerKey[KeyBytes];                                // the key for encrypted communication from client -> server.
 
    uint8_t serverToClientKey[KeyBytes];                                // the key for encrypted communication from server -> client.

    uint8_t random[KeyBytes];                                           // random data the client cannot possibly know.

    ConnectToken()
    {
        protocolId = 0;
        clientId = 0;
        expiryTimestamp = 0;
        numServerAddresses = 0;
        memset( clientToServerKey, 0, KeyBytes );
        memset( serverToClientKey, 0, KeyBytes );
        memset( random, 0, KeyBytes );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_uint32( stream, protocolId );

        serialize_uint64( stream, clientId );
        
        serialize_uint64( stream, expiryTimestamp );
        
        serialize_int( stream, numServerAddresses, 0, MaxServersPerConnectToken - 1 );
        
        for ( int i = 0; i < numServerAddresses; ++i )
            serialize_address( stream, serverAddresses[i] );

        serialize_bytes( stream, clientToServerKey, KeyBytes );

        serialize_bytes( stream, serverToClientKey, KeyBytes );

        serialize_bytes( stream, random, KeyBytes );

        return true;
    }
};

void GenerateConnectToken( ConnectToken & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses )
{
    uint64_t timestamp = (uint64_t) time( NULL );

    token.protocolId = ProtocolId;
    token.clientId = clientId;
    token.expiryTimestamp = timestamp + ConnectTokenExpirySeconds;
    
    assert( numServerAddresses > 0 );
    assert( numServerAddresses <= MaxServersPerConnectToken );
    token.numServerAddresses = numServerAddresses;
    for ( int i = 0; i < numServerAddresses; ++i )
        token.serverAddresses[i] = serverAddresses[i];

    GenerateKey( token.clientToServerKey );    

    GenerateKey( token.serverToClientKey );

    GenerateKey( token.random );
}

bool EncryptConnectToken( ConnectToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key )
{
    uint8_t message[ConnectTokenBytes];
    memset( message, 0, ConnectTokenBytes );
    WriteStream stream( message, ConnectTokenBytes );
    if ( !token.Serialize( stream ) )
        return false;

    stream.Flush();
    
    if ( stream.GetError() )
        return false;

    uint64_t encryptedLength;

    if ( !Encrypt_AEAD( message, ConnectTokenBytes - AuthBytes, encryptedMessage, encryptedLength, additional, additionalLength, nonce, key ) )
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

    ReadStream stream( decryptedMessage, ConnectTokenBytes - AuthBytes );
    if ( !decryptedToken.Serialize( stream ) )
        return false;

    if ( stream.GetError() )
        return false;

    return true;
}

struct ChallengeToken
{
    uint64_t clientId;                                                  // the unique client id. max one connection per-client id, per-server.

    Address clientAddress;                                              // client address corresponding to the initial connection request.

    Address serverAddress;                                              // client address corresponding to the initial connection request.

    uint8_t connectTokenMac[MacBytes];                                  // mac of the initial connect token this challenge corresponds to.
 
    uint8_t clientToServerKey[KeyBytes];                                // the key for encrypted communication from client -> server.
 
    uint8_t serverToClientKey[KeyBytes];                                // the key for encrypted communication from server -> client.

    uint8_t random[KeyBytes];                                           // random bytes the client cannot possibly know.

    ChallengeToken()
    {
        clientId = 0;
        memset( connectTokenMac, 0, MacBytes );
        memset( clientToServerKey, 0, KeyBytes );
        memset( serverToClientKey, 0, KeyBytes );
        memset( random, 0, KeyBytes );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_uint64( stream, clientId );
        
        serialize_address( stream, clientAddress );

        serialize_address( stream, serverAddress );

        serialize_bytes( stream, connectTokenMac, MacBytes );

        serialize_bytes( stream, clientToServerKey, KeyBytes );

        serialize_bytes( stream, serverToClientKey, KeyBytes );

        serialize_bytes( stream, random, KeyBytes );

        return true;
    }
};

bool GenerateChallengeToken( const ConnectToken & connectToken, const Address & clientAddress, const Address & serverAddress, const uint8_t * connectTokenMac, ChallengeToken & challengeToken )
{
    if ( connectToken.clientId == 0 )
        return false;

    if ( !clientAddress.IsValid() )
        return false;

    challengeToken.clientId = connectToken.clientId;

    challengeToken.clientAddress = clientAddress;
    
    challengeToken.serverAddress = serverAddress;

    memcpy( challengeToken.connectTokenMac, connectTokenMac, MacBytes );

    memcpy( challengeToken.clientToServerKey, connectToken.clientToServerKey, KeyBytes );

    memcpy( challengeToken.serverToClientKey, connectToken.serverToClientKey, KeyBytes );

    GenerateKey( challengeToken.random );

    return true;
}

bool EncryptChallengeToken( ChallengeToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key )
{
    uint8_t message[ChallengeTokenBytes];
    memset( message, 0, ChallengeTokenBytes );
    WriteStream stream( message, ChallengeTokenBytes );
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

static uint8_t private_key[KeyBytes];

struct MatcherServerData
{
    Address serverAddress;                              // IP address of this server

    int numConnectedClients;                            // number of connected clients on this dedi

    uint64_t connectedClients[MaxClients];              // client ids connected to this server (tight array)
};

class Matcher
{
    uint64_t m_nonce;                                   // increments with each match request

    map<Address,MatcherServerData*> m_serverMap;        // maps network address to data for that server

public:

    Matcher()
    {
        m_nonce = 0;
    }

    bool RequestMatch( uint64_t clientId, uint8_t * tokenData, uint8_t * tokenNonce, uint8_t * clientToServerKey, uint8_t * serverToClientKey, int & numServerAddresses, Address * serverAddresses )
    {
        if ( clientId == 0 )
            return false;

        numServerAddresses = 1;
        serverAddresses[0] = Address( "::1", ServerPort );

        ConnectToken token;
        GenerateConnectToken( token, clientId, numServerAddresses, serverAddresses );

        memcpy( clientToServerKey, token.clientToServerKey, KeyBytes );
        memcpy( serverToClientKey, token.serverToClientKey, KeyBytes );

        if ( !EncryptConnectToken( token, tokenData, NULL, 0, (const uint8_t*) &m_nonce, private_key ) )
            return false;

        assert( NonceBytes == 8 );

        memcpy( tokenNonce, &m_nonce, NonceBytes );

        m_nonce++;

        return true;
    }
};

enum PacketTypes
{
    PACKET_CONNECTION_REQUEST,                      // client requests a connection.
    PACKET_CONNECTION_DENIED,                       // server denies client connection request.
    PACKET_CONNECTION_CHALLENGE,                    // server response to client connection request.
    PACKET_CONNECTION_RESPONSE,                     // client response to server connection challenge.
    PACKET_CONNECTION_HEARTBEAT,                    // heartbeat packet sent at some low rate (once per-second) to keep the connection alive.
    PACKET_CONNECTION_DISCONNECT,                   // courtesy packet to indicate that the other side has disconnected. better than a timeout.
    CLIENT_SERVER_NUM_PACKETS
};

struct ConnectionRequestPacket : public Packet
{
    uint8_t connectTokenData[ConnectTokenBytes];                        // encrypted connect token data generated by matchmaker
    uint8_t connectTokenNonce[NonceBytes];                              // nonce required to decrypt the connect token on the server

    ConnectionRequestPacket() : Packet( PACKET_CONNECTION_REQUEST )
    {
        memset( connectTokenData, 0, sizeof( connectTokenData ) );
        memset( connectTokenNonce, 0, sizeof( connectTokenNonce ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bytes( stream, connectTokenData, sizeof( connectTokenData ) );
        serialize_bytes( stream, connectTokenNonce, sizeof( connectTokenNonce ) );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct ConnectionDeniedPacket : public Packet
{
    ConnectionDeniedPacket() : Packet( PACKET_CONNECTION_DENIED ) {}

    template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct ConnectionChallengePacket : public Packet
{
    uint8_t challengeTokenData[ChallengeTokenBytes];                      // encrypted challenge token data generated by matchmaker
    uint8_t challengeTokenNonce[NonceBytes];                              // nonce required to decrypt the challenge token on the server

    ConnectionChallengePacket() : Packet( PACKET_CONNECTION_CHALLENGE )
    {
        memset( challengeTokenData, 0, sizeof( challengeTokenData ) );
        memset( challengeTokenNonce, 0, sizeof( challengeTokenNonce ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bytes( stream, challengeTokenData, sizeof( challengeTokenData ) );
        serialize_bytes( stream, challengeTokenNonce, sizeof( challengeTokenNonce ) );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct ConnectionResponsePacket : public Packet
{
    uint8_t challengeTokenData[ChallengeTokenBytes];                      // encrypted challenge token data generated by matchmaker
    uint8_t challengeTokenNonce[NonceBytes];                              // nonce required to decrypt the challenge token on the server

    ConnectionResponsePacket() : Packet( PACKET_CONNECTION_RESPONSE )
    {
        memset( challengeTokenData, 0, sizeof( challengeTokenData ) );
        memset( challengeTokenNonce, 0, sizeof( challengeTokenNonce ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bytes( stream, challengeTokenData, sizeof( challengeTokenData ) );
        serialize_bytes( stream, challengeTokenNonce, sizeof( challengeTokenNonce ) );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct ConnectionHeartBeatPacket : public Packet
{
    ConnectionHeartBeatPacket() : Packet( PACKET_CONNECTION_HEARTBEAT ) {}

    template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct ConnectionDisconnectPacket : public Packet
{
    ConnectionDisconnectPacket() : Packet( PACKET_CONNECTION_DISCONNECT ) {}

    template <typename Stream> bool Serialize( Stream & /*stream*/ ) { return true; }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct ClientServerPacketFactory : public PacketFactory
{
    ClientServerPacketFactory() : PacketFactory( CLIENT_SERVER_NUM_PACKETS ) {}

    Packet* Create( int type )
    {
        switch ( type )
        {
            case PACKET_CONNECTION_REQUEST:         return new ConnectionRequestPacket();
            case PACKET_CONNECTION_DENIED:          return new ConnectionDeniedPacket();
            case PACKET_CONNECTION_CHALLENGE:       return new ConnectionChallengePacket();
            case PACKET_CONNECTION_RESPONSE:        return new ConnectionResponsePacket();
            case PACKET_CONNECTION_HEARTBEAT:       return new ConnectionHeartBeatPacket();
            case PACKET_CONNECTION_DISCONNECT:      return new ConnectionDisconnectPacket();
            default:
                return NULL;
        }
    }

    void Destroy( Packet *packet )
    {
        delete packet;
    }
};

void PrintBytes( const uint8_t * data, int data_bytes )
{
    for ( int i = 0; i < data_bytes; ++i )
    {
        printf( "%02x", (int) data[i] );
        if ( i != data_bytes - 1 )
            printf( "-" );
    }
    printf( " (%d bytes)", data_bytes );
}

struct ServerClientData
{
    Address address;
    uint64_t clientId;
    double connectTime;
    double lastPacketSendTime;
    double lastPacketReceiveTime;

    ServerClientData()
    {
        clientId = 0;
        connectTime = 0.0;
        lastPacketSendTime = 0.0;
        lastPacketReceiveTime = 0.0;
    }
};

struct ConnectTokenEntry
{
    double time;                                                       // time for this entry. used to replace the oldest entries once the connect token array fills up.
    Address address;                                                   // address of the client that sent the connect token. binds a connect token to a particular address so it can't be exploited.
    uint8_t mac[MacBytes];                                             // hmac of connect token. we use this to avoid replay attacks where the same token is sent repeatedly for different addresses.

    ConnectTokenEntry()
    {
        time = 0.0;
        memset( mac, 0, MacBytes );
    }
};

class Server
{
    NetworkInterface * m_networkInterface;                              // network interface for sending and receiving packets.

    uint64_t m_challengeTokenNonce;                                     // nonce used for encoding challenge tokens

    int m_numConnectedClients;                                          // number of connected clients
    
    bool m_clientConnected[MaxClients];                                 // true if client n is connected
    
    uint64_t m_clientId[MaxClients];                                    // array of client id values per-client
    
    Address m_serverAddress;                                            // the external IP address of this server (what clients will be sending packets to)

    Address m_clientAddress[MaxClients];                                // array of client address values per-client
    
    ServerClientData m_clientData[MaxClients];                          // heavier weight data per-client, eg. not for fast lookup

    ConnectTokenEntry m_connectTokenEntries[MaxConnectTokenEntries];    // array of connect tokens entries. used to avoid replay attacks of the same connect token for different addresses.

public:

    Server( NetworkInterface & networkInterface )
    {
        m_networkInterface = &networkInterface;
        m_numConnectedClients = 0;
        m_challengeTokenNonce = 0;
        for ( int i = 0; i < MaxClients; ++i )
            ResetClientState( i );
    }

    ~Server()
    {
        assert( m_networkInterface );
        m_networkInterface = NULL;
    }

    void SendPackets( double time )
    {
        for ( int i = 0; i < MaxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                continue;

            if ( m_clientData[i].lastPacketSendTime + ConnectionHeartBeatRate > time )
                return;

            ConnectionHeartBeatPacket * packet = (ConnectionHeartBeatPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_HEARTBEAT );

            SendPacketToConnectedClient( i, packet, time );
        }
    }

    void ReceivePackets( double time )
    {
        while ( true )
        {
            Address address;
            Packet *packet = m_networkInterface->ReceivePacket( address );
            if ( !packet )
                break;
            
            switch ( packet->GetType() )
            {
                case PACKET_CONNECTION_REQUEST:
                    ProcessConnectionRequest( *(ConnectionRequestPacket*)packet, address, time );
                    break;

                case PACKET_CONNECTION_RESPONSE:
                    ProcessConnectionResponse( *(ConnectionResponsePacket*)packet, address, time );
                    break;

                case PACKET_CONNECTION_HEARTBEAT:
                    ProcessConnectionHeartBeat( *(ConnectionHeartBeatPacket*)packet, address, time );
                    break;

                case PACKET_CONNECTION_DISCONNECT:
                    ProcessConnectionDisconnect( *(ConnectionDisconnectPacket*)packet, address, time );
                    break;

                default:
                    break;
            }

            m_networkInterface->DestroyPacket( packet );
        }
    }

    void CheckForTimeOut( double time )
    {
        for ( int i = 0; i < MaxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                continue;

            if ( m_clientData[i].lastPacketReceiveTime + ConnectionTimeOut < time )
            {
                char buffer[256];
                const char *addressString = m_clientAddress[i].ToString( buffer, sizeof( buffer ) );
                printf( "client %d timed out (client address = %s, client id = %" PRIx64 ")\n", i, addressString, m_clientId[i] );
                DisconnectClient( i, time );
            }
        }
    }

    void SetServerAddress( const Address & address )
    {
        m_serverAddress = address;
    }

    const Address & GetServerAddress() const
    {
        return m_serverAddress;
    }

    int GetNumConnectedClients() 
    {
        return m_numConnectedClients;
    }

protected:

    void ResetClientState( int clientIndex )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < MaxClients );
        m_clientConnected[clientIndex] = false;
        m_clientId[clientIndex] = 0;
        m_clientAddress[clientIndex] = Address();
        m_clientData[clientIndex] = ServerClientData();
    }

    int FindFreeClientIndex() const
    {
        for ( int i = 0; i < MaxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                return i;
        }
        return -1;
    }

    int FindExistingClientIndex( const Address & address ) const
    {
        for ( int i = 0; i < MaxClients; ++i )
        {
            if ( m_clientConnected[i] && m_clientAddress[i] == address )
                return i;
        }
        return -1;
    }

    int FindExistingClientIndex( const Address & address, uint64_t clientId ) const
    {
        for ( int i = 0; i < MaxClients; ++i )
        {
            if ( m_clientId[i] == clientId && m_clientConnected[i] && m_clientAddress[i] == address )
                return i;
        }
        return -1;
    }

    bool FindOrAddConnectTokenEntry( const Address & address, const uint8_t * mac, double time )
    {
        // find the matching entry for the token mac, and the oldest token. constant time worst case O(1) at all times. This is intentional!

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

            if ( oldestTokenIndex == -1 || oldestTokenTime < m_connectTokenEntries[i].time )
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

    void ConnectClient( int clientIndex, const ChallengeToken & challengeToken, double time )
    {
        assert( m_numConnectedClients >= 0 );
        assert( m_numConnectedClients < MaxClients - 1 );
        assert( !m_clientConnected[clientIndex] );

        m_numConnectedClients++;

        m_clientConnected[clientIndex] = true;
        m_clientId[clientIndex] = challengeToken.clientId;
        m_clientAddress[clientIndex] = challengeToken.clientAddress;

        m_clientData[clientIndex].address = challengeToken.clientAddress;
        m_clientData[clientIndex].clientId = challengeToken.clientId;
        m_clientData[clientIndex].connectTime = time;
        m_clientData[clientIndex].lastPacketSendTime = time;
        m_clientData[clientIndex].lastPacketReceiveTime = time;

        char buffer[256];
        const char *addressString = challengeToken.clientAddress.ToString( buffer, sizeof( buffer ) );
        printf( "client %d connected (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, challengeToken.clientId );

        ConnectionHeartBeatPacket * connectionHeartBeatPacket = (ConnectionHeartBeatPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_HEARTBEAT );

        if ( connectionHeartBeatPacket )
        {
            SendPacketToConnectedClient( clientIndex, connectionHeartBeatPacket, time );
        }
    }

    void DisconnectClient( int clientIndex, double time )
    {
        assert( clientIndex >= 0 );
        assert( clientIndex < MaxClients );
        assert( m_numConnectedClients > 0 );
        assert( m_clientConnected[clientIndex] );

        char buffer[256];
        const char *addressString = m_clientAddress[clientIndex].ToString( buffer, sizeof( buffer ) );
        printf( "client %d disconnected: (client address = %s, client id = %" PRIx64 ")\n", clientIndex, addressString, m_clientId[clientIndex] );

        ConnectionDisconnectPacket * packet = (ConnectionDisconnectPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_DISCONNECT );

        SendPacketToConnectedClient( clientIndex, packet, time );

        ResetClientState( clientIndex );

        m_numConnectedClients--;
    }

    bool IsConnected( uint64_t clientId ) const
    {
        for ( int i = 0; i < MaxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                continue;
            if ( m_clientId[i] == clientId )
                return true;
        }
        return false;
    }

    bool IsConnected( const Address & address, uint64_t clientId ) const
    {
        for ( int i = 0; i < MaxClients; ++i )
        {
            if ( !m_clientConnected[i] )
                continue;
            if ( m_clientAddress[i] == address && m_clientId[i] == clientId )
                return true;
        }
        return false;
    }

    void SendPacketToConnectedClient( int clientIndex, Packet * packet, double time )
    {
        assert( packet );
        assert( clientIndex >= 0 );
        assert( clientIndex < MaxClients );
        assert( m_clientConnected[clientIndex] );
        m_clientData[clientIndex].lastPacketSendTime = time;
        m_networkInterface->SendPacket( m_clientAddress[clientIndex], packet );
    }

    void ProcessConnectionRequest( const ConnectionRequestPacket & packet, const Address & address, double time )
    {
        char buffer[256];
        const char * addressString = address.ToString( buffer, sizeof( buffer ) );        
        printf( "processing connection request packet from: %s\n", addressString );

        ConnectToken connectToken;
        if ( !DecryptConnectToken( packet.connectTokenData, connectToken, NULL, 0, packet.connectTokenNonce, private_key ) )
        {
            printf( "failed to decrypt connect token\n" );
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
            printf( "server address is not in connect token whitelist\n" );
            return;
        }

        if ( connectToken.clientId == 0 )
        {
            printf( "connect token client id is 0\n" );
            return;
        }

        if ( IsConnected( address, connectToken.clientId ) )
        {
            printf( "client is already connected\n" );
            return;
        }

        uint64_t timestamp = (uint64_t) ::time( NULL );

        if ( connectToken.expiryTimestamp <= timestamp )
        {
            printf( "connect token has expired\n" );
            return;
        }

        if ( !m_networkInterface->AddEncryptionMapping( address, connectToken.serverToClientKey, connectToken.clientToServerKey ) )
        {
            printf( "failed to add encryption mapping\n" );
            return;
        }

        if ( m_numConnectedClients == MaxClients )
        {
            printf( "connection denied: server is full\n" );
            ConnectionDeniedPacket * connectionDeniedPacket = (ConnectionDeniedPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_DENIED );
            m_networkInterface->SendPacket( address, connectionDeniedPacket );
            return;
        }

        if ( !FindOrAddConnectTokenEntry( address, packet.connectTokenData, time ) )
        {
            printf( "connect token has already been used\n" );
            return;
        }

        ChallengeToken challengeToken;
        if ( !GenerateChallengeToken( connectToken, address, m_serverAddress, packet.connectTokenData, challengeToken ) )
        {
            printf( "failed to generate challenge token\n" );
            return;
        }

        ConnectionChallengePacket * connectionChallengePacket = (ConnectionChallengePacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_CHALLENGE );
        if ( !connectionChallengePacket )
            return;

        memcpy( connectionChallengePacket->challengeTokenNonce, (uint8_t*) &m_challengeTokenNonce, NonceBytes );

        if ( !EncryptChallengeToken( challengeToken, connectionChallengePacket->challengeTokenData, NULL, 0, connectionChallengePacket->challengeTokenNonce, private_key ) )
        {
            printf( "failed to encrypt challenge token\n" );
            return;
        }

        char clientAddressString[64];
        address.ToString( clientAddressString, sizeof( clientAddressString ) );
        printf( "server sent challenge to client %s\n", clientAddressString );

        m_networkInterface->SendPacket( address, connectionChallengePacket );
    }

    void ProcessConnectionResponse( const ConnectionResponsePacket & packet, const Address & address, double time )
    {
        ChallengeToken challengeToken;
        if ( !DecryptChallengeToken( packet.challengeTokenData, challengeToken, NULL, 0, packet.challengeTokenNonce, private_key ) )
        {
            printf( "failed to decrypt challenge token\n" );
            return;
        }

        if ( challengeToken.clientAddress != address )
        {
            printf( "challenge token client address does not match\n" );
            return;
        }

        if ( challengeToken.serverAddress != m_serverAddress )
        {
            printf( "challenge token server address does not match\n" );
            return;
        }

        const int existingClientIndex = FindExistingClientIndex( address, challengeToken.clientId );
        if ( existingClientIndex != -1 )
        {
            assert( existingClientIndex >= 0 );
            assert( existingClientIndex < MaxClients );

            if ( m_clientData[existingClientIndex].lastPacketSendTime + ConnectionConfirmSendRate < time )
            {
                ConnectionHeartBeatPacket * connectionHeartBeatPacket = (ConnectionHeartBeatPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_HEARTBEAT );

                if ( connectionHeartBeatPacket )
                {
                    SendPacketToConnectedClient( existingClientIndex, connectionHeartBeatPacket, time );
                }
            }

            return;
        }

        char buffer[256];
        const char * addressString = address.ToString( buffer, sizeof( buffer ) );
        printf( "processing connection response from client %s (client id = %" PRIx64 ")\n", addressString, challengeToken.clientId );

        if ( m_numConnectedClients == MaxClients )
        {
            printf( "connection denied: server is full\n" );
            ConnectionDeniedPacket * connectionDeniedPacket = (ConnectionDeniedPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_DENIED );
            if ( connectionDeniedPacket )
            {
                m_networkInterface->SendPacket( address, connectionDeniedPacket );
            }
            return;
        }

        const int clientIndex = FindFreeClientIndex();

        assert( clientIndex != -1 );
        if ( clientIndex == -1 )
            return;

        ConnectClient( clientIndex, challengeToken, time );
    }

    void ProcessConnectionHeartBeat( const ConnectionHeartBeatPacket & /*packet*/, const Address & address, double time )
    {
        const int clientIndex = FindExistingClientIndex( address );
        if ( clientIndex == -1 )
            return;

        assert( clientIndex >= 0 );
        assert( clientIndex < MaxClients );

        m_clientData[clientIndex].lastPacketReceiveTime = time;
    }

    void ProcessConnectionDisconnect( const ConnectionDisconnectPacket & /*packet*/, const Address & address, double time )
    {
        const int clientIndex = FindExistingClientIndex( address );
        if ( clientIndex == -1 )
            return;

        assert( clientIndex >= 0 );
        assert( clientIndex < MaxClients );

        DisconnectClient( clientIndex, time );
    }
};

class ClientServerNetworkInterface : public SocketInterface
{   
public:

    ClientServerNetworkInterface( ClientServerPacketFactory & packetFactory, uint16_t port ) : SocketInterface( memory_default_allocator(), packetFactory, ProtocolId, port )
    {
        EnablePacketEncryption();

        DisableEncryptionForPacketType( PACKET_CONNECTION_REQUEST );

        assert( IsEncryptedPacketType( PACKET_CONNECTION_REQUEST ) == false );
        assert( IsEncryptedPacketType( PACKET_CONNECTION_DENIED ) == true );
        assert( IsEncryptedPacketType( PACKET_CONNECTION_CHALLENGE ) == true );
        assert( IsEncryptedPacketType( PACKET_CONNECTION_RESPONSE ) == true );
        assert( IsEncryptedPacketType( PACKET_CONNECTION_HEARTBEAT ) == true );
        assert( IsEncryptedPacketType( PACKET_CONNECTION_DISCONNECT ) == true );
    }

    ~ClientServerNetworkInterface()
    {
        ClearSendQueue();
        ClearReceiveQueue();
    }
};

enum ClientState
{
    CLIENT_STATE_DISCONNECTED,
    CLIENT_STATE_SENDING_CONNECTION_REQUEST,
    CLIENT_STATE_SENDING_CHALLENGE_RESPONSE,
    CLIENT_STATE_CONNECTED,
    CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT,
    CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT,
    CLIENT_STATE_CONNECTION_TIMED_OUT,
    CLIENT_STATE_CONNECTION_DENIED
};

class Client
{
    ClientState m_clientState;                                          // current client state

    Address m_serverAddress;                                            // server address we are connecting or connected to.

    double m_lastPacketSendTime;                                        // time we last sent a packet to the server.

    double m_lastPacketReceiveTime;                                     // time we last received a packet from the server (used for timeouts).

    NetworkInterface * m_networkInterface;                              // network interface the client uses to send and receive packets.

    uint64_t m_clientId;                                                // client id as per-connect call

    uint8_t m_connectTokenData[ConnectTokenBytes];                      // encrypted connect token data for connection request packet

    uint8_t m_connectTokenNonce[NonceBytes];                            // nonce required to send to server so it can decrypt connect token

    uint8_t m_challengeTokenData[ChallengeTokenBytes];                  // encrypted challenge token data for challenge response packet

    uint8_t m_challengeTokenNonce[NonceBytes];                          // nonce required to send to server so it can decrypt challenge token

public:

    Client( NetworkInterface & networkInterface )
    {
        m_networkInterface = &networkInterface;
        ResetConnectionData();
    }

    ~Client()
    {
        m_networkInterface = NULL;
    }

    void Connect( const Address & address, 
                  double time, 
                  uint64_t clientId,
                  const uint8_t * connectTokenData, 
                  const uint8_t * connectTokenNonce,
                  const uint8_t * clientToServerKey,
                  const uint8_t * serverToClientKey )
    {
        Disconnect( time );
        m_serverAddress = address;
        m_clientState = CLIENT_STATE_SENDING_CONNECTION_REQUEST;
        m_lastPacketSendTime = time - 1.0f;
        m_lastPacketReceiveTime = time;
        m_clientId = clientId;
        memcpy( m_connectTokenData, connectTokenData, ConnectTokenBytes );
        memcpy( m_connectTokenNonce, connectTokenNonce, NonceBytes );
        m_networkInterface->ResetEncryptionMappings();
        m_networkInterface->AddEncryptionMapping( m_serverAddress, clientToServerKey, serverToClientKey );
    }

    bool IsConnecting() const
    {
        return m_clientState == CLIENT_STATE_SENDING_CONNECTION_REQUEST || m_clientState == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE;
    }

    bool IsConnected() const
    {
        return m_clientState == CLIENT_STATE_CONNECTED;
    }

    bool ConnectionFailed() const
    {
        return m_clientState > CLIENT_STATE_CONNECTED;
    }

    void Disconnect( double time )
    {
        if ( m_clientState == CLIENT_STATE_CONNECTED )
        {
            printf( "client-side disconnect: (client id = %" PRIx64 ")\n", m_clientId );
            ConnectionDisconnectPacket * packet = (ConnectionDisconnectPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_DISCONNECT );
            SendPacketToServer( packet, time );
        }

        ResetConnectionData();
    }

    void SendPackets( double time )
    {
        switch ( m_clientState )
        {
            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
            {
                if ( m_lastPacketSendTime + ConnectionRequestSendRate > time )
                    return;

                char buffer[256];
                const char *addressString = m_serverAddress.ToString( buffer, sizeof( buffer ) );
                printf( "client sending connection request to server %s\n", addressString );

                ConnectionRequestPacket * packet = (ConnectionRequestPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_REQUEST );

                if ( packet )
                {
                    memcpy( packet->connectTokenData, m_connectTokenData, ConnectTokenBytes );
                    memcpy( packet->connectTokenNonce, m_connectTokenNonce, NonceBytes );

                    SendPacketToServer( packet, time );
                }
            }
            break;

            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
            {
                if ( m_lastPacketSendTime + ConnectionResponseSendRate > time )
                    return;

                char buffer[256];
                const char *addressString = m_serverAddress.ToString( buffer, sizeof( buffer ) );
                printf( "client sending challenge response to server %s\n", addressString );

                ConnectionResponsePacket * packet = (ConnectionResponsePacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_RESPONSE );

                if ( packet )
                {
                    memcpy( packet->challengeTokenData, m_challengeTokenData, ChallengeTokenBytes );
                    memcpy( packet->challengeTokenNonce, m_challengeTokenNonce, NonceBytes );
                    
                    SendPacketToServer( packet, time );
                }
            }
            break;

            case CLIENT_STATE_CONNECTED:
            {
                if ( m_lastPacketSendTime + ConnectionHeartBeatRate > time )
                    return;

                ConnectionHeartBeatPacket * packet = (ConnectionHeartBeatPacket*) m_networkInterface->CreatePacket( PACKET_CONNECTION_HEARTBEAT );

                if ( packet )
                {
                    SendPacketToServer( packet, time );
                }
            }
            break;

            default:
                break;
        }
    }

    void ReceivePackets( double time )
    {
        while ( true )
        {
            Address address;
            Packet *packet = m_networkInterface->ReceivePacket( address );
            if ( !packet )
                break;
            
            switch ( packet->GetType() )
            {
                case PACKET_CONNECTION_DENIED:
                    ProcessConnectionDenied( *(ConnectionDeniedPacket*)packet, address, time );
                    break;

                case PACKET_CONNECTION_CHALLENGE:
                    ProcessConnectionChallenge( *(ConnectionChallengePacket*)packet, address, time );
                    break;

                case PACKET_CONNECTION_HEARTBEAT:
                    ProcessConnectionHeartBeat( *(ConnectionHeartBeatPacket*)packet, address, time );
                    break;

                case PACKET_CONNECTION_DISCONNECT:
                    ProcessConnectionDisconnect( *(ConnectionDisconnectPacket*)packet, address, time );
                    break;

                default:
                    break;
            }

            m_networkInterface->DestroyPacket( packet );
        }
    }

    void CheckForTimeOut( double time )
    {
        switch ( m_clientState )
        {
            case CLIENT_STATE_SENDING_CONNECTION_REQUEST:
            {
                if ( m_lastPacketReceiveTime + ConnectionRequestTimeOut < time )
                {
                    printf( "connection request to server timed out\n" );
                    m_clientState = CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT;
                    return;
                }
            }
            break;

            case CLIENT_STATE_SENDING_CHALLENGE_RESPONSE:
            {
                if ( m_lastPacketReceiveTime + ChallengeResponseTimeOut < time )
                {
                    printf( "challenge response to server timed out\n" );
                    m_clientState = CLIENT_STATE_CHALLENGE_RESPONSE_TIMED_OUT;
                    return;
                }
            }
            break;

            case CLIENT_STATE_CONNECTED:
            {
                if ( m_lastPacketReceiveTime + ConnectionTimeOut < time )
                {
                    printf( "keep alive timed out\n" );
                    m_clientState = CLIENT_STATE_CONNECTION_TIMED_OUT;
                    Disconnect( time );
                    return;
                }
            }
            break;

            default:
                break;
        }
    }

protected:

    void ResetConnectionData()
    {
        assert( m_networkInterface );
        m_serverAddress = Address();
        m_clientState = CLIENT_STATE_DISCONNECTED;
        m_lastPacketSendTime = -1000.0;
        m_lastPacketReceiveTime = -1000.0;
        m_clientId = 0;
        memset( m_connectTokenData, 0, ConnectTokenBytes );
        memset( m_connectTokenNonce, 0, NonceBytes );
        memset( m_challengeTokenData, 0, ChallengeTokenBytes );
        memset( m_challengeTokenNonce, 0, NonceBytes );
        m_networkInterface->ResetEncryptionMappings();
    }

    void SendPacketToServer( Packet *packet, double time )
    {
        assert( m_clientState != CLIENT_STATE_DISCONNECTED );
        assert( m_serverAddress.IsValid() );

        m_networkInterface->SendPacket( m_serverAddress, packet );

        m_lastPacketSendTime = time;
    }

    void ProcessConnectionDenied( const ConnectionDeniedPacket & /*packet*/, const Address & address, double /*time*/ )
    {
        if ( m_clientState != CLIENT_STATE_SENDING_CONNECTION_REQUEST )
            return;

        if ( address != m_serverAddress )
            return;

        char buffer[256];
        const char * addressString = address.ToString( buffer, sizeof( buffer ) );
        printf( "client received connection denied from server: %s\n", addressString );
        m_clientState = CLIENT_STATE_CONNECTION_DENIED;
    }

    void ProcessConnectionChallenge( const ConnectionChallengePacket & packet, const Address & address, double time )
    {
        if ( m_clientState != CLIENT_STATE_SENDING_CONNECTION_REQUEST )
            return;

        if ( address != m_serverAddress )
            return;

        char buffer[256];
        const char * addressString = address.ToString( buffer, sizeof( buffer ) );
        printf( "client received connection challenge from server: %s\n", addressString );

        memcpy( m_challengeTokenData, packet.challengeTokenData, ChallengeTokenBytes );
        memcpy( m_challengeTokenNonce, packet.challengeTokenNonce, NonceBytes );

        m_clientState = CLIENT_STATE_SENDING_CHALLENGE_RESPONSE;

        m_lastPacketReceiveTime = time;
    }

    void ProcessConnectionHeartBeat( const ConnectionHeartBeatPacket & /*packet*/, const Address & address, double time )
    {
        if ( m_clientState < CLIENT_STATE_SENDING_CHALLENGE_RESPONSE )
            return;

        if ( address != m_serverAddress )
            return;

        if ( m_clientState == CLIENT_STATE_SENDING_CHALLENGE_RESPONSE )
        {
            char buffer[256];
            const char * addressString = address.ToString( buffer, sizeof( buffer ) );
            printf( "client is now connected to server: %s\n", addressString );

            memset( m_connectTokenData, 0, ConnectTokenBytes );
            memset( m_connectTokenNonce, 0, NonceBytes );
            memset( m_challengeTokenData, 0, ChallengeTokenBytes );
            memset( m_challengeTokenNonce, 0, NonceBytes );

            m_clientState = CLIENT_STATE_CONNECTED;        
        }

        m_lastPacketReceiveTime = time;
    }

    void ProcessConnectionDisconnect( const ConnectionDisconnectPacket & /*packet*/, const Address & address, double time )
    {
        if ( m_clientState != CLIENT_STATE_CONNECTED )
            return;

        if ( address != m_serverAddress )
            return;

        Disconnect( time );
    }
};

int main()
{
    printf( "\nsecuring dedicated servers\n\n" );

    Matcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t challengeTokenData[ChallengeTokenBytes];
    
    uint8_t connectTokenNonce[NonceBytes];
    uint8_t challengeTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );
    memset( challengeTokenNonce, 0, NonceBytes );

    printf( "requesting match\n\n" );

    GenerateKey( private_key );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        return 1;
    }

    printf( "connect token: " );
    PrintBytes( connectTokenData, ConnectTokenBytes );
    printf( "\n" );

    ConnectToken connectToken;
    if ( !DecryptConnectToken( connectTokenData, connectToken, NULL, 0, connectTokenNonce, private_key ) )
    {
        printf( "error: failed to decrypt connect token\n" );
        return 1;
    }

    assert( connectToken.clientId == 1 );
    assert( connectToken.numServerAddresses == 1 );
    assert( connectToken.serverAddresses[0] == Address( "::1", ServerPort ) );
    assert( memcmp( connectToken.clientToServerKey, clientToServerKey, KeyBytes ) == 0 );
    assert( memcmp( connectToken.serverToClientKey, serverToClientKey, KeyBytes ) == 0 );

    char serverAddressString[64];
    connectToken.serverAddresses[0].ToString( serverAddressString, sizeof( serverAddressString ) );
    printf( "\nsuccess: connect token is valid for client %" PRIx64 " connection to %s\n\n", connectToken.clientId, serverAddressString );

    Address clientAddress( "::1", ClientPort );

    ChallengeToken challengeToken;
    if ( !GenerateChallengeToken( connectToken, clientAddress, serverAddresses[0], connectTokenData, challengeToken ) )
    {
        printf( "error: failed to generate challenge token\n" );
        return 1;
    }

    if ( !EncryptChallengeToken( challengeToken, challengeTokenData, NULL, 0, challengeTokenNonce, private_key ) )
    {
        printf( "error: failed to encrypt challenge token\n" );
        return 1;
    }

    printf( "challenge token: " );
    PrintBytes( challengeTokenData, ChallengeTokenBytes );
    printf( "\n" );

    ChallengeToken decryptedChallengeToken;
    if ( !DecryptChallengeToken( challengeTokenData, decryptedChallengeToken, NULL, 0, challengeTokenNonce, private_key ) )
    {
        printf( "error: failed to decrypt challenge token\n" );
        return 1;
    }

    assert( challengeToken.clientId == 1 );
    assert( challengeToken.clientAddress == clientAddress );
    assert( challengeToken.serverAddress == serverAddresses[0] );
    assert( memcmp( challengeToken.connectTokenMac, connectTokenData, MacBytes ) == 0 );
    assert( memcmp( challengeToken.clientToServerKey, clientToServerKey, KeyBytes ) == 0 );
    assert( memcmp( challengeToken.serverToClientKey, serverToClientKey, KeyBytes ) == 0 );

    char clientAddressString[64];
    challengeToken.clientAddress.ToString( clientAddressString, sizeof( clientAddressString ) );
    challengeToken.serverAddress.ToString( serverAddressString, sizeof( serverAddressString ) );
    printf( "\nsuccess: challenge token is valid for client %" PRIx64 " connection from %s to %s\n\n", challengeToken.clientId, clientAddressString, serverAddressString );

    memory_initialize();
    {
        srand( (unsigned int) time( NULL ) );

        InitializeNetwork();

        ClientServerPacketFactory packetFactory;

        Address clientAddress( "::1", ClientPort );
        Address serverAddress( "::1", ServerPort );

        ClientServerNetworkInterface clientInterface( packetFactory, ClientPort );
        ClientServerNetworkInterface serverInterface( packetFactory, ServerPort );

        if ( clientInterface.GetError() != SOCKET_ERROR_NONE || serverInterface.GetError() != SOCKET_ERROR_NONE )
        {
            printf( "error: failed to initialize sockets\n" );
            return 1;
        }
        
        const int NumIterations = 20;

        double time = 0.0;

        Client client( clientInterface );

        Server server( serverInterface );

        server.SetServerAddress( serverAddress );
        
        client.Connect( serverAddress, time, clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

        printf( "----------------------------------------------------------\n" );

        for ( int i = 0; i < NumIterations; ++i )
        {
            printf( "t = %f\n", time );

            client.SendPackets( time );
            server.SendPackets( time );

            clientInterface.WritePackets( time );
            serverInterface.WritePackets( time );

            clientInterface.ReadPackets( time );
            serverInterface.ReadPackets( time );

            client.ReceivePackets( time );
            server.ReceivePackets( time );

            client.CheckForTimeOut( time );
            server.CheckForTimeOut( time );

            if ( client.ConnectionFailed() )
            {
                printf( "error: client connect failed!\n" );
                break;
            }

            if ( client.IsConnected() )
                client.Disconnect( time );

            time += 0.1f;

            printf( "----------------------------------------------------------\n" );

            if ( !client.IsConnecting() && !client.IsConnected() && server.GetNumConnectedClients() )
                break;
        }

        ShutdownNetwork();
    }

    memory_shutdown();

    printf( "\n" );

    return 0;
}
