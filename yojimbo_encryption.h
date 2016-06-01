/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#ifndef YOJIMBO_ENCRYPTION_H
#define YOJIMBO_ENCRYPTION_H

#include "yojimbo_config.h"
#include "yojimbo_network.h"

#include <stdint.h>

namespace yojimbo
{
    const int NonceBytes = 8;
    const int KeyBytes = 32;
    const int AuthBytes = 16;
    const int MacBytes = 16;

    extern void GenerateKey( uint8_t * key );

    extern void RandomBytes( uint8_t * data, int bytes );

    extern bool Encrypt( const uint8_t * message, int messageLength, 
                         uint8_t * encryptedMessage, int & encryptedMessageLength, 
                         const uint8_t * nonce, const uint8_t * key );
 
    extern bool Decrypt( const uint8_t * encryptedMessage, int encryptedMessageLength, 
                         uint8_t * decryptedMessage, int & decryptedMessageLength, 
                         const uint8_t * nonce, const uint8_t * key );

    extern bool Encrypt_AEAD( const uint8_t * message, uint64_t messageLength, 
                              uint8_t * encryptedMessage, uint64_t & encryptedMessageLength,
                              const uint8_t * additional, uint64_t additionalLength,
                              const uint8_t * nonce,
                              const uint8_t * key );

    extern bool Decrypt_AEAD( const uint8_t * encryptedMessage, uint64_t encryptedMessageLength, 
                              uint8_t * decryptedMessage, uint64_t & decryptedMessageLength,
                              const uint8_t * additional, uint64_t additionalLength,
                              const uint8_t * nonce,
                              const uint8_t * key );

    const int MaxEncryptionMappings = 1024;

    const double DefaultEncryptionMappingTimeout = 10;

    class EncryptionManager
    {
    public:

        EncryptionManager();

        bool AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey, double time );

        bool RemoveEncryptionMapping( const Address & address, double time );

        void ResetEncryptionMappings();

        const uint8_t * GetSendKey( const Address & address, double time );

        const uint8_t * GetReceiveKey( const Address & address, double time );

        void SetTimeout( double timeout )
        {
            m_encryptionMappingTimeout = timeout;
        }

    private:

        int m_numEncryptionMappings;
        double m_encryptionMappingTimeout;
        double m_lastAccessTime[MaxEncryptionMappings];
        Address m_address[MaxEncryptionMappings];
        uint8_t m_sendKey[KeyBytes*MaxEncryptionMappings];
        uint8_t m_receiveKey[KeyBytes*MaxEncryptionMappings];
    };
}

#endif // #ifndef YOJIMBO_ENCRYPTION_H
