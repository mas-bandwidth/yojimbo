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

#ifndef YOJIMBO_ENCRYPTION_H
#define YOJIMBO_ENCRYPTION_H

#include "yojimbo_config.h"
#include "yojimbo_network.h"
#include "yojimbo_replay_protection.h"

#include <stdint.h>

namespace yojimbo
{
    /**
        Generate a cryptographically secure random key.

        See https://download.libsodium.org/doc/ for implementation details.

        @param key Pointer to the byte buffer where the generated key will be written. Buffer must be at least yojimbo::KeyBytes large.
     */
  
    extern void GenerateKey( uint8_t * key );

    /**
        Generate a cryptographically secure random number.

        See https://download.libsodium.org/doc/ for implementation details.

        @param data Pointer to the byte buffer where the generated random number will be written.
        @param bytes The number of bytes of random data to generate.
     */

    extern void RandomBytes( uint8_t * data, int bytes );

    /**
        Encrypt a message with a symmetric cipher.

        This is used for encrypted UDP packets sent between the client and server.

        See https://download.libsodium.org/doc/ for implementation details.

        @param message The message to encrypt.
        @param messageLength The length of the message to encrypt (bytes).
        @param encryptedMessage Buffer where the encrypted message will be written. 
        @param encryptedMessageLength The number of bytes of encrypted message data that was written [out]. Will be equal to messageLength + yojimbo::MacBytes on successful encryption.
        @param nonce The nonce to use to encrypt the message. This should be a sequence number that increases with each call to encrypt. Never pass in a nonce value that has already been used with this key!
        @param key The key used for encryption.

        @returns True if the message was encrypted successfully, false otherwise.

        @see Decrypt
     */

    extern bool Encrypt( const uint8_t * message, int messageLength, uint8_t * encryptedMessage, int & encryptedMessageLength, const uint8_t * nonce, const uint8_t * key );
 
    /**
        Decrypt a message that was encrypted with a symmetric cipher.

        This is used for encrypted UDP packets sent between the client and server.

        See https://download.libsodium.org/doc/ for implementation details.

        @param encryptedMessage The encrypted message.
        @param encryptedMessageLength The length of the encrypted message (bytes).
        @param decryptedMessage The buffer where the decrypted message will be written. Must be at least encryptedMessageLength - yojimbo::MacBytes large.
        @param decryptedMessageLength The length of the decrypted message (bytes). On success will be equal to encryptedMessageLength - yojimbo::MacBytes.
        @param nonce The nonce used to encrypt the message.
        @param key The key used to encrypt the message.

        @returns True if the message was sucessfully decrypted, false otherwise.
     */

    extern bool Decrypt( const uint8_t * encryptedMessage, int encryptedMessageLength, uint8_t * decryptedMessage, int & decryptedMessageLength, const uint8_t * nonce, const uint8_t * key );

    /**
        Encrypt a message with an AEAD privitive (authenticated encryption with associated data).

        This is used to encrypt connect tokens and challenge tokens. Connect tokens set the associated data to the connect token expiry timestap for quick rejection of stale tokens.

        See https://en.wikipedia.org/wiki/Authenticated_encryption for an overview of AEAD.

        See https://download.libsodium.org/doc/ for implementation details.

        @param message The message to encrypt.
        @param messageLength The length of the message to encrypt (bytes).
        @param encryptedMessage The buffer where the encrypted message will be written.
        @param encryptedMessageLength The length of the encrypted message (bytes) [out].
        @param additional The additional data to consider when encrypting the data.
        @param additionalLength The length of the additional data (bytes).
        @param nonce The nonce to use to encrypt the message. This should be a sequence number that increases with each call to encrypt. Never pass in a nonce value that has already been used with this key!
        @param key The key used for encryption.

        @returns True if the message was successfully encrypted. False otherwise.
     */

    extern bool Encrypt_AEAD( const uint8_t * message, uint64_t messageLength, uint8_t * encryptedMessage, uint64_t & encryptedMessageLength, const uint8_t * additional, uint64_t additionalLength, const uint8_t * nonce, const uint8_t * key );

    /**
        Decrypt a message encrypted an AEAD privitive (authenticated encryption with associated data).

        This is used to encrypt connect tokens and challenge tokens. Connect tokens set the associated data to the connect token expiry timestap for quick rejection of stale tokens.

        See https://en.wikipedia.org/wiki/Authenticated_encryption for an overview of AEAD.

        See https://download.libsodium.org/doc/ for implementation details.

        @param encryptedMessage The encrypted message.
        @param encryptedMessageLength The length of the encrypted message (bytes).
        @param decryptedMessage The buffer where the decrypted message will be written. Must be at least encryptedMessageLength - yojimbo::MacBytes large.
        @param decryptedMessageLength The number of bytes of decrypted message written [out]. Will be encryptedMessageLength - yojimbo::MacBytes on success.
        @param additional The additional data to consider when decrypting the data. Must match the additional data used when encrypting the data or the decrypt will fail.
        @param additionalLength The length of the additional data (bytes).
        @param nonce The nonce used to encrypt the message.
        @param key The key used to encrypt the message.

        @returns True if the message was successfully decrypted. False otherwise.
     */

    extern bool Decrypt_AEAD( const uint8_t * encryptedMessage, uint64_t encryptedMessageLength, uint8_t * decryptedMessage, uint64_t & decryptedMessageLength, const uint8_t * additional, uint64_t additionalLength, const uint8_t * nonce, const uint8_t * key );

    // todo: document the encryption manager

    /// Associates addresses with encryption keys so each client gets their own keys for packet encryption.

    class EncryptionManager
    {
    public:

        EncryptionManager();

        bool AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey, double time, double timeout );

        bool RemoveEncryptionMapping( const Address & address, double time );

        void ResetEncryptionMappings();

        int FindEncryptionMapping( const Address & address, double time );

        const uint8_t * GetSendKey( int index ) const ;

        const uint8_t * GetReceiveKey( int index ) const;

    private:

        int m_numEncryptionMappings;

        double m_lastAccessTime[MaxEncryptionMappings];

        double m_timeout[MaxEncryptionMappings];
        
        Address m_address[MaxEncryptionMappings];
        
        uint8_t m_sendKey[KeyBytes*MaxEncryptionMappings];
        
        uint8_t m_receiveKey[KeyBytes*MaxEncryptionMappings];
    };
}

#endif // #ifndef YOJIMBO_ENCRYPTION_H
