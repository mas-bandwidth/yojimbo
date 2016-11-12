/*
    Yojimbo Client/Server Network Protocol Library.
    
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

#ifndef YOJIMBO_TOKENS_H
#define YOJIMBO_TOKENS_H

#include "yojimbo_config.h"
#include "yojimbo_address.h"
#include "yojimbo_serialize.h"

#include <inttypes.h>

namespace yojimbo
{
    struct ConnectToken
    {
        uint32_t protocolId;                                                // the protocol id this connect token corresponds to
     
        uint64_t clientId;                                                  // the unique client id. max one connection per-client id, per-server
     
        uint64_t expireTimestamp;                                           // timestamp of when this connect token expires
     
        int numServerAddresses;                                             // the number of server addresses in the connect token whitelist
    
        Address serverAddresses[MaxServersPerConnect];                      // connect token only allows connection to these server addresses
     
        uint8_t clientToServerKey[KeyBytes];                                // the key for encrypted communication from client -> server
     
        uint8_t serverToClientKey[KeyBytes];                                // the key for encrypted communication from server -> client

        uint8_t random[KeyBytes];                                           // random data the client cannot possibly know

        ConnectToken()
        {
            protocolId = 0;
            clientId = 0;
            expireTimestamp = 0;
            numServerAddresses = 0;
            memset( clientToServerKey, 0, KeyBytes );
            memset( serverToClientKey, 0, KeyBytes );
            memset( random, 0, KeyBytes );
        }

        template <typename Stream> bool Serialize( Stream & stream )
        {
            serialize_uint32( stream, protocolId );

            serialize_uint64( stream, clientId );
            
            serialize_uint64( stream, expireTimestamp );
            
            serialize_int( stream, numServerAddresses, 0, MaxServersPerConnect - 1 );
            
            for ( int i = 0; i < numServerAddresses; ++i )
                serialize_address( stream, serverAddresses[i] );

            serialize_bytes( stream, clientToServerKey, KeyBytes );

            serialize_bytes( stream, serverToClientKey, KeyBytes );

            serialize_bytes( stream, random, KeyBytes );

            return true;
        }

        bool operator == ( const ConnectToken & other ) const;
        bool operator != ( const ConnectToken & other ) const;
    };

    struct ChallengeToken
    {
        uint64_t clientId;                                                  // the unique client id. max one connection per-client id, per-server.

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

            serialize_bytes( stream, connectTokenMac, MacBytes );

            serialize_bytes( stream, clientToServerKey, KeyBytes );

            serialize_bytes( stream, serverToClientKey, KeyBytes );

            serialize_bytes( stream, random, KeyBytes );

            return true;
        }
    };

    void GenerateConnectToken( ConnectToken & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses, uint32_t protocolId, int expirySeconds );

    bool EncryptConnectToken( const ConnectToken & token, uint8_t * encryptedMessage, const uint8_t * nonce, const uint8_t * key );

    bool DecryptConnectToken( const uint8_t * encryptedMessage, ConnectToken & decryptedToken, const uint8_t * nonce, const uint8_t * key, uint64_t expireTimestamp );

    bool WriteConnectTokenToJSON( const ConnectToken & connectToken, char * output, int outputSize );

    bool ReadConnectTokenFromJSON( const char * json, ConnectToken & connectToken );

    bool GenerateChallengeToken( const ConnectToken & connectToken, const uint8_t * connectTokenMac, ChallengeToken & challengeToken );

    bool EncryptChallengeToken( ChallengeToken & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );

    bool DecryptChallengeToken( const uint8_t * encryptedMessage, ChallengeToken & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key );
}

#endif // #ifndef YOJIMBO_TOKENS_H
