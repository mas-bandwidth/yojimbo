/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo_encryption.h"

#ifdef _MSC_VER
#define SODIUM_STATIC
#endif // #ifdef _MSC_VER

#include <sodium.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>

namespace yojimbo
{
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

    EncryptionManager::EncryptionManager()
    {
        m_encryptionMappingTimeout = DefaultEncryptionMappingTimeout;

        ResetEncryptionMappings();
    }

    inline void PrintBytes( const char * label, const uint8_t * data, int data_bytes )
    {
        printf( "%s: ", label );
        for ( int i = 0; i < data_bytes; ++i )
        {
            printf( "%02x", (int) data[i] );
            if ( i != data_bytes - 1 )
                printf( "-" );
        }
        printf( " (%d bytes)\n", data_bytes );
    }

    bool EncryptionManager::AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey, double time )
    {
        char addressString[64];
        address.ToString( addressString, sizeof( addressString ) );
        printf( "add encryption mapping: %s\n", addressString );

        PrintBytes( "receiveKey", receiveKey, KeyBytes );

        for ( int i = 0; i < m_numEncryptionMappings; ++i )
        {
            if ( m_address[i] == address && m_lastAccessTime[i] + m_encryptionMappingTimeout >= time )
            {
                m_lastAccessTime[i] = time;
                memcpy( m_sendKey + i*KeyBytes, sendKey, KeyBytes );
                memcpy( m_receiveKey + i*KeyBytes, receiveKey, KeyBytes );
                return true;
            }
        }

        for ( int i = 0; i < MaxEncryptionMappings; ++i )
        {
            if ( m_lastAccessTime[i] + m_encryptionMappingTimeout < time )
            {
                m_address[i] = address;
                m_lastAccessTime[i] = time;
                memcpy( m_sendKey + i*KeyBytes, sendKey, KeyBytes );
                memcpy( m_receiveKey + i*KeyBytes, receiveKey, KeyBytes );
                if ( i + 1 > m_numEncryptionMappings )
                    m_numEncryptionMappings = i + 1;
                return true;
            }
        }

        return false;
    }

    bool EncryptionManager::RemoveEncryptionMapping( const Address & address, double time )
    {
        char addressString[64];
        address.ToString( addressString, sizeof( addressString ) );
        printf( "remove encryption mapping: %s\n", addressString );

        for ( int i = 0; i < m_numEncryptionMappings; ++i )
        {
            if ( m_address[i] == address )
            {
                printf( "actually removed encryption mapping\n" );

                m_address[i] = Address();
                m_lastAccessTime[i] = -1000.0;

                memset( m_sendKey + i*KeyBytes, 0, KeyBytes );
                memset( m_receiveKey + i*KeyBytes, 0, KeyBytes );

                if ( i + 1 == m_numEncryptionMappings )
                {
                    int index = i - 1;
                    while ( index >= 0 )
                    {
                        if ( m_lastAccessTime[index] + m_encryptionMappingTimeout >= time )
                            break;
                        index--;
                    }
                    m_numEncryptionMappings = index + 1;
                }

                return true;
            }
        }

        printf( "could not remove encryption mapping\n" );

        return false;
    }

    void EncryptionManager::ResetEncryptionMappings()
    {
        printf( "reset encryption mappings\n" );

        m_numEncryptionMappings = 0;
        
        for ( int i = 0; i < MaxEncryptionMappings; ++i )
        {
            m_lastAccessTime[i] = -1000.0;
            m_address[i] = Address();
        }
        
        memset( m_sendKey, 0, sizeof( m_sendKey ) );
        memset( m_receiveKey, 0, sizeof( m_receiveKey ) );
    }

    const uint8_t * EncryptionManager::GetSendKey( const Address & address, double time )
    {
        for ( int i = 0; i < m_numEncryptionMappings; ++i )
        {
            if ( m_address[i] == address && m_lastAccessTime[i] + m_encryptionMappingTimeout >= time )
            {
                m_lastAccessTime[i] = time;
                return m_sendKey + i*KeyBytes;
            }
        }
        return NULL;
    }

    const uint8_t * EncryptionManager::GetReceiveKey( const Address & address, double time )
    {
        for ( int i = 0; i < m_numEncryptionMappings; ++i )
        {
            if ( m_address[i] == address && m_lastAccessTime[i] + m_encryptionMappingTimeout >= time )
            {
                m_lastAccessTime[i] = time;
                return m_receiveKey + i*KeyBytes;
            }
        }
        return NULL;
    }
}
