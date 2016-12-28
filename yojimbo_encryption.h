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

    /** 
        Associates addresses with encryption keys so each client gets their own keys for packet encryption.

        Separate keys are used for packets sent to an address vs. packets received from this address. 

        This was done to allow the client/server to use the sequence numbers of packets as a nonce in both directions. An alternative would have been to set the high bit of the packet sequence number in one of the directions, but I felt this was cleaner.
     */


    class EncryptionManager
    {
    public:

        /**
            Encryption manager constructor.
         */

        EncryptionManager();

        /**
            Associates an address with send and receive keys for packet encryption.

            @param address The address to associate with encryption keys.
            @param sendKey The key used to encrypt packets sent to this address.
            @param receiveKey The key used to decrypt packets received from this address.
            @param time The current time (seconds).
            @param timeout The timeout value in seconds for this encryption mapping (seconds). Encyrption mapping times out if no packets are sent to or received from this address in the timeout period.

            @returns True if the encryption mapping was added successfully, false otherwise.
         */

        bool AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey, double time, double timeout );

        /**
            Remove the encryption mapping for an address.

            @param address The address of the encryption mapping to remove.
            @param time The current time (seconds).

            @returns True if an encryption mapping for the address exists and was removed, false if no encryption mapping could be found for the address.
         */

        bool RemoveEncryptionMapping( const Address & address, double time );

        /**
            Reset all encryption mappings.

            Any encryption mappings that were added are cleared and the encryption manager is reset back to default state.
         */

        void ResetEncryptionMappings();

        /**
            Find an encryption mapping (index) for the specified address.

            IMPORTANT: This function "touches" the encryption mapping and resets its last access time to the current time. As long as this is called regularly for an encryption mapping it won't time out.

            @param address The address of the encryption mapping to search for.

            @returns The index of the encryption mapping if one exists for the address. -1 if no encryption mapping was found.

            @see EncryptionManager::GetSendKey
            @see EncryptionManager::GetReceiveKey
         */

        int FindEncryptionMapping( const Address & address, double time );

        /**
            Get the send key for an encryption mapping (by index).

            @see EncryptionManager::FindEncryptionMapping

            @param The encryption mapping index. See EncryptionMapping::FindEncryptionMapping

            @returns The key to encrypt sent packets.
         */

        const uint8_t * GetSendKey( int index ) const ;

        /**
            Get the receive key for an encryption mapping (by index).

            @see EncryptionManager::FindEncryptionMapping

            @param The encryption mapping index. See EncryptionMapping::FindEncryptionMapping

            @returns The key to decrypt received packets.
         */

        const uint8_t * GetReceiveKey( int index ) const;

    private:

        int m_numEncryptionMappings;                                                    ///< The number of encryption mappings in the array. This is how far we search from left to right starting at index 0. It's updated as entries are removed from the right.

        double m_lastAccessTime[MaxEncryptionMappings];                                 ///< Array of last access times used to time out encryption mappings.

        double m_timeout[MaxEncryptionMappings];                                        ///< Array of timeout values (seconds) for each encryption mapping. Allows each encryption mapping to potentially have its own timeout value.
        
        Address m_address[MaxEncryptionMappings];                                       ///< The address associated with each encryption mapping index. If no encryption mapping exists at this index, this is set to an invalid address. See Address::IsValid.
        
        uint8_t m_sendKey[KeyBytes*MaxEncryptionMappings];                              ///< Array containing all send keys. The send key for an encryption mapping index n starts at offset KeyBytes * n.
        
        uint8_t m_receiveKey[KeyBytes*MaxEncryptionMappings];                           ///< Array containing all receive keys. The receive key for an encryption mapping index n starts at offset KeyBytes * n.
    };
}

#endif // #ifndef YOJIMBO_ENCRYPTION_H
