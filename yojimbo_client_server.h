/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#ifndef YOJIMBO_CLIENT_SERVER_H
#define YOJIMBO_CLIENT_SERVER_H

#include "yojimbo_config.h"
#include "yojimbo_packet.h"
#include "yojimbo_crypto.h"
#include "yojimbo_network.h"
#include "yojimbo_packet_processor.h"

namespace yojimbo
{
    const int ConnectTokenBytes = 1024;
    const int ChallengeTokenBytes = 256;
    const int MaxServersPerConnectToken = 8;
    const int ConnectTokenExpirySeconds = 10;

    template <typename Stream> bool serialize_address_internal( Stream & stream, Address & address )
    {
        char buffer[64];

        if ( Stream::IsWriting )
        {
            assert( address.IsValid() );
            address.ToString( buffer, sizeof( buffer ) );
        }

        // todo: serialize the address as binary instead.
        serialize_string( stream, buffer, sizeof( buffer ) );

        if ( Stream::IsReading )
        {
            address = Address( buffer );
            if ( !address.IsValid() )
                return false;
        }

        return true;
    }

    #define serialize_address( stream, value )                                              \
        do                                                                                  \
        {                                                                                   \
            if ( !yojimbo::serialize_address_internal( stream, value ) )                    \
                return false;                                                               \
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

    void GenerateConnectToken( ConnectToken & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses, uint32_t protocolId );

    bool EncryptConnectToken( ConnectToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool DecryptConnectToken( const uint8_t * encryptedMessage, ConnectToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool GenerateChallengeToken( const ConnectToken & connectToken, const Address & clientAddress, const Address & serverAddress, const uint8_t * connectTokenMac, ChallengeToken & challengeToken );

    bool EncryptChallengeToken( ChallengeToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool DecryptChallengeToken( const uint8_t * encryptedMessage, ChallengeToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );
}

#endif // #ifndef YOJIMBO_SERIALIZE_H
