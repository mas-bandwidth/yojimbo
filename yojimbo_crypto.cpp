/*
    Yojimbo Client/Server Network Library.

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

#include "yojimbo_crypto.h"

#ifdef _MSC_VER
#define SODIUM_STATIC
#endif // #ifdef _MSC_VER

#include <sodium.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>

namespace yojimbo
{
    bool InitializeCrypto()
    {
        assert( NonceBytes == crypto_aead_chacha20poly1305_NPUBBYTES );
        assert( KeyBytes == crypto_aead_chacha20poly1305_KEYBYTES );
        assert( AuthBytes == crypto_aead_chacha20poly1305_ABYTES );
        assert( MacBytes == crypto_secretbox_MACBYTES );
        return sodium_init() == 0;
    }

    void GenerateKey( uint8_t * key )
    {
        assert( key );
        randombytes_buf( key, KeyBytes );
    }
    
    void RandomBytes( uint8_t * data, int bytes )
    {
        assert( data );
        randombytes_buf( data, bytes );
    }

    bool Encrypt( const uint8_t * message, int messageLength, 
                  uint8_t * encryptedMessage, int & encryptedMessageLength, 
                  const uint8_t * nonce, const uint8_t * key )
    {
        uint8_t actual_nonce[crypto_secretbox_NONCEBYTES];
        memset( actual_nonce, 0, sizeof( actual_nonce ) );
        memcpy( actual_nonce, nonce, NonceBytes );

        if ( crypto_secretbox_easy( encryptedMessage, message, messageLength, actual_nonce, key ) != 0 )
            return false;

        encryptedMessageLength = messageLength + MacBytes;

        return true;
    }
 
    bool Decrypt( const uint8_t * encryptedMessage, int encryptedMessageLength, 
                  uint8_t * decryptedMessage, int & decryptedMessageLength, 
                  const uint8_t * nonce, const uint8_t * key )
    {
        uint8_t actual_nonce[crypto_secretbox_NONCEBYTES];
        memset( actual_nonce, 0, sizeof( actual_nonce ) );
        memcpy( actual_nonce, nonce, NonceBytes );

        if ( crypto_secretbox_open_easy( decryptedMessage, encryptedMessage, encryptedMessageLength, actual_nonce, key ) != 0 )
            return false;

        decryptedMessageLength = encryptedMessageLength - MacBytes;

        return true;
    }

    bool Encrypt_AEAD( const uint8_t * message, uint64_t messageLength, 
                       uint8_t * encryptedMessage, uint64_t &  encryptedMessageLength,
                       const uint8_t * additional, uint64_t additionalLength,
                       const uint8_t * nonce,
                       const uint8_t * key )
    {
        unsigned long long encryptedLength;

        int result = crypto_aead_chacha20poly1305_encrypt( encryptedMessage, &encryptedLength,
                                                           message, (unsigned long long) messageLength,
                                                           additional, (unsigned long long) additionalLength,
                                                           NULL, nonce, key );

        encryptedMessageLength = (uint64_t) encryptedLength;

        return result == 0;
    }

    bool Decrypt_AEAD( const uint8_t * encryptedMessage, uint64_t encryptedMessageLength, 
                       uint8_t * decryptedMessage, uint64_t & decryptedMessageLength,
                       const uint8_t * additional, uint64_t additionalLength,
                       const uint8_t * nonce,
                       const uint8_t * key )
    {
        unsigned long long decryptedLength;

        int result = crypto_aead_chacha20poly1305_decrypt( decryptedMessage, &decryptedLength,
                                                           NULL,
                                                           encryptedMessage, (unsigned long long) encryptedMessageLength,
                                                           additional, (unsigned long long) additionalLength,
                                                           nonce, key );

        decryptedMessageLength = (uint64_t) decryptedLength;

        return result == 0;
    }
}
