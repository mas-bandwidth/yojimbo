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
    /**
        Connect token passed from the client to server when it requests a secure connection.

        How do connect tokens work?

        First, matcher.go generates a connect token equivalent structure in golang, encodes it to JSON, encrypts it, base64 encodes it and transmits it to the client over HTTPS.

        The client receives this encrypted connect token over HTTPS when it requests a match, along with information about which servers to connect to, and the keys for packet encryption (hence the need for HTTPS).

        The client takes the connect token and passes it to each server it tries to connect to inside connection request packets. The server inspects these connection request packets and only allows connections from clients with a valid connect token.

        This creates a system where servers only allow connections from authenticated clients, effectively transmitting whatever authentication the backend has performed to the dedicated servers, without the dedicated servers and matcher needing to have any direct communication.

        @see ConnectionRequestPacket
     */

    struct ConnectToken
    {
        uint32_t protocolId;                                                ///< The protocol id the connect token corresponds to.
     
        uint64_t clientId;                                                  ///< The unique client id. Only one connection per-client id is allowed at a time on a server.
     
        uint64_t expireTimestamp;                                           ///< Timestamp when this connect token expires.
     
        int numServerAddresses;                                             ///< The number of server addresses in the connect token whitelist. Connect tokens are only valid to connect to the server addresses in this list.
    
        Address serverAddresses[MaxServersPerConnect];                      ///< This connect token only allows connection to these server addresses.
     
        uint8_t clientToServerKey[KeyBytes];                                ///< The key for encrypted communication from client -> server.
     
        uint8_t serverToClientKey[KeyBytes];                                ///< The key for encrypted communication from server -> client.

        uint8_t random[KeyBytes];                                           ///< Random data. Security experts: Is there any point to doing this or am I being overly paranoid here?

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

        bool operator == ( const ConnectToken & other ) const;
        bool operator != ( const ConnectToken & other ) const;
    };

    /**
        Data stored inside the encrypted challenge token.

        Sent from server to client in response to a connection request with a valid connect token.

        The purpose of this challenge/response does is ensure that a potential client is able to receive packets on the source packet address for the connection request packet. 

        This stops clients from connecting with spoofed packet source addresses.

        @see ConnectionRequestPacket
        @see ChallengeResponsePacket
     */

    struct ChallengeToken
    {
        uint64_t clientId;                                                  ///< The unique client id. Maximum of one connection per-client id, per-server at any time.

        uint8_t connectTokenMac[MacBytes];                                  ///< Mac of the initial connect token this challenge corresponds to. Used to quickly map the challenge response from a client to a pending connection entry on the server.
     
        uint8_t clientToServerKey[KeyBytes];                                ///< The key for encrypted communication from client -> server.
     
        uint8_t serverToClientKey[KeyBytes];                                ///< The key for encrypted communication from server -> client.

        uint8_t random[KeyBytes];                                           ///< Random bytes the client cannot possibly know. Security experts: am I being overly paranoid here?

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

    /**
        Generates a connect token. 

        I use this function for testing purposes. The real connect tokens are generated in matcher.go. 

        I don't expect you'll want to code your web backend in C++, but if you really want to, feel free :)
        
        @param token The connect token to generate [out]
        @param clientId The client id (must be globally unique).
        @param numServerAddresses The number of server address to store in the connect token.
        @param serverAddresses Array of server addresses to store in the connect token. The client can only use the connect token to connect to these addresses.
        @param protocolId The protocol id of the connection to be established. The server will reject any connections from clients with a different protocol id.
        @param expirySeconds The number of seconds in the future until this connect token expires. This should long enough for a client to try all servers in the list in turn if they are busy. I recommend 45 seconds. 

        @see EncryptConnectToken
        @see DecryptConnectToken
     */

    void GenerateConnectToken( ConnectToken & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses, uint32_t protocolId, int expirySeconds );

    /**
        Encrypt a connect token. 

        Writes a connect token to JSON and encrypts that JSON with the key and nonce supplied.

        The encryption is performed with libsodium's AEAD primitive, with the expiry timestamp as the additional data, so we can quickly reject stale connect tokens without needing to do a full decrypt.

        SUPER FUCKING IMPORTANT: The nonce *must* increase after each connect token is generated. If you don't do this, it's possible for attackers to recover your private key and completely break the security model!

        @param token The connect token data to encrypt.
        @param encryptedMessage The buffer to receive the encrypted message. Must be at least yojimbo::ConnectTokenBytes bytes large.
        @param nonce The nonce for the encryption. Treat this as a sequence number and increase it each time you generate a new connect token. This is incredibly important. You must use each nonce value only once!
        @param key The private key used to encrypt the connect token. This must be known by the matcher and your dedicated servers instances. The client must *not* know this private key, or the security model breaks down!

        @see GenerateConnectToken
        @see DecryptConnectToken
     */

    bool EncryptConnectToken( const ConnectToken & token, uint8_t * encryptedMessage, const uint8_t * nonce, const uint8_t * key );

    /**
        Decrypt a connect token.

        Used by the server to decrypt and validate connect tokens sent to it. 

        Uses libsodium's AEAD construction with the token expire timestamp as the additional data for quick rejection of expired tokens.

        @param encryptedMessage The encrypted connect token that should be decrypted. The buffer must be of length yojimbo::ConnectTokenBytes.
        @param decryptedToken The decrypted connect token [out]. Valid only if this function returns true.
        @param nonce The nonce used to encrypt the connect token.
        @param key The private key used to encrypt the connect token.
        @param expireTimestamp The timestamp when this connect token expires. Passed into the AEAD primitive as the addition data.

        @returns True if the connect token decrypted successfully. False otherwise.
     */

    bool DecryptConnectToken( const uint8_t * encryptedMessage, ConnectToken & decryptedToken, const uint8_t * nonce, const uint8_t * key, uint64_t expireTimestamp );

    /**
        Write a connect token to JSON.

        @param connectToken The connect token to write to JSON.
        @param output The output buffer to write the connect token to.
        @param outputSize The size of the output buffer in bytes. 

        @returns True if the connect token was written to JSON successfully, false otherwise.
     */

    bool WriteConnectTokenToJSON( const ConnectToken & connectToken, char * output, int outputSize );

    /**
        Read a connect token from JSON.

        @param json The JSON string (null terminated).
        @param connectToken The connect token read in from the JSON [out]. Only valid if this function returns true.

        @returns True if the connect token was successfully read from JSON, false otherwise.
     */

    bool ReadConnectTokenFromJSON( const char * json, ConnectToken & connectToken );

    /**
        Generate a challenge token.

        This is used by the server to generate a challenge token in response to a valid connection request packet sent from a potential client.

        @param connectToken The connect token sent from the client.
        @param connectTokenMac The MAC fo the connect token. Used for quick lookup from challenge response to the pending client entry on the server.
        @param challengeToken The challenge token to generate [out]. Only valid if this function returns true.

        @returns True if a challange token was successfully generated. False otherwise.
     */

    bool GenerateChallengeToken( const ConnectToken & connectToken, const uint8_t * connectTokenMac, ChallengeToken & challengeToken );

    /**
        Encrypt a challenge token.

        Writes the challenge token to a binary format and encrypts that buffer using libsodium's AEAD privitive.

        @param token The challenge token to encrypt.
        @param encryptedMessage The buffer that the encrypted message should be written to. Should be at least yojimbo::ChallengeTokenBytes large.
        @param nonce The nonce to encrypt the challenge token with. Treat this as a sequence number and increase it with each challenge token encrypted. Take care to never use each nonce only once!
        @param key The private key used to encrypt the challenge token. This key must not be known by clients.
     */

    bool EncryptChallengeToken( ChallengeToken & token, uint8_t * encryptedMessage, const uint8_t * nonce, const uint8_t * key );

    /**
        Decrypt a challenge token.

        Used by the server to decrypt the challenge token contained in a challenge response packet sent from the client.

        @param encryptedMessage The encrypted challenge token to decrypt.
        @param decryptedToken The decrypted challenge token data [out]. Only valid if this function returns true.
        @param nonce The nonce used to encrypt the challenge token.
        @param key The key used to encrypt the challenge token.

        @returns True if the challenge token was successfully decrypted, false otherwise.
     */

    bool DecryptChallengeToken( const uint8_t * encryptedMessage, ChallengeToken & decryptedToken, const uint8_t * nonce, const uint8_t * key );
}

#endif // #ifndef YOJIMBO_TOKENS_H
