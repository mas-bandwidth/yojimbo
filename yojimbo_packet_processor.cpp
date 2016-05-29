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

#include "yojimbo_packet_processor.h"
#include "yojimbo_crypto.h"
#include "yojimbo_util.h"
#include "protocol2.h"
#include <stdio.h>
#include <sodium.h>

namespace yojimbo
{
    PacketProcessor::PacketProcessor( protocol2::PacketFactory & packetFactory, uint32_t protocolId, int maxPacketSize, void * context )
    {
        m_packetFactory = &packetFactory;

        m_protocolId = protocolId;

        m_error = PACKET_PROCESSOR_ERROR_NONE;

        m_maxPacketSize = maxPacketSize + ( ( maxPacketSize % 4 ) ? ( 4 - ( maxPacketSize % 4 ) ) : 0 );
        
        assert( m_maxPacketSize % 4 == 0 );
        assert( m_maxPacketSize >= maxPacketSize );

        const int MaxPrefixBytes = 9;
        const int CryptoOverhead = MacBytes;

        m_absoluteMaxPacketSize = maxPacketSize + MaxPrefixBytes + CryptoOverhead;

        m_context = context;

        m_packetBuffer = new uint8_t[m_absoluteMaxPacketSize];
        m_scratchBuffer = new uint8_t[m_absoluteMaxPacketSize];
    }

    PacketProcessor::~PacketProcessor()
    {
        delete [] m_packetBuffer;
        delete [] m_scratchBuffer;

        m_packetBuffer = NULL;
        m_scratchBuffer = NULL;
    }

    static const int ENCRYPTED_PACKET_FLAG = (1<<7);

    const uint8_t * PacketProcessor::WritePacket( protocol2::Packet * packet, uint64_t sequence, int & packetBytes, bool encrypt, const uint8_t * key )
    {
        m_error = PACKET_PROCESSOR_ERROR_NONE;

        if ( encrypt )
        {
            if ( !key )
            {
                m_error = PACKET_PROCESSOR_ERROR_KEY_IS_NULL;
                return NULL;
            }

            int prefixBytes;
            uint8_t prefix[16];
            CompressPacketSequence( sequence, prefix[0], prefixBytes, prefix+1 );
            prefix[0] |= ENCRYPTED_PACKET_FLAG;
            prefixBytes++;

            protocol2::PacketInfo info;

            info.context = m_context;
            info.protocolId = m_protocolId;
            info.packetFactory = m_packetFactory;
//            info.rawFormat = 1;

            packetBytes = protocol2::WritePacket( info, packet, m_scratchBuffer, m_maxPacketSize );

            if ( packetBytes <= 0 )
            {
                m_error = PACKET_PROCESSOR_ERROR_WRITE_PACKET_FAILED;
                return NULL;
            }

            assert( packetBytes <= m_maxPacketSize );

            int encryptedPacketSize;

            if ( !Encrypt( m_scratchBuffer,
                          packetBytes,
                          m_scratchBuffer,
                          encryptedPacketSize, 
                          (uint8_t*) &sequence, key ) )
            {
                m_error = PACKET_PROCESSOR_ERROR_ENCRYPT_FAILED;
                return NULL;
            }

            memcpy( m_packetBuffer, prefix, prefixBytes );
            memcpy( m_packetBuffer + prefixBytes, m_scratchBuffer, encryptedPacketSize );

            packetBytes = prefixBytes + encryptedPacketSize;

            assert( packetBytes <= m_absoluteMaxPacketSize );

            return m_packetBuffer;
        }
        else
        {
            protocol2::PacketInfo info;

            info.context = m_context;
            info.protocolId = m_protocolId;
            info.packetFactory = m_packetFactory;
            info.prefixBytes = 1;

            packetBytes = protocol2::WritePacket( info, packet, m_packetBuffer, m_maxPacketSize );

            if ( packetBytes <= 0 )
            {
                m_error = PACKET_PROCESSOR_ERROR_WRITE_PACKET_FAILED;
                return NULL;
            }

            assert( packetBytes <= m_maxPacketSize );

            return m_packetBuffer;
        }
    }

    protocol2::Packet * PacketProcessor::ReadPacket( const uint8_t * packetData, uint64_t & sequence, int packetBytes, bool & encrypted,  
                                                     const uint8_t * key, const uint8_t * encryptedPacketTypes, const uint8_t * unencryptedPacketTypes )
    {
        m_error = PACKET_PROCESSOR_ERROR_NONE;

        const uint8_t prefixByte = packetData[0];

        encrypted = ( prefixByte & ENCRYPTED_PACKET_FLAG ) != 0;

        if ( encrypted )
        {
            if ( !key )
            {
                m_error = PACKET_PROCESSOR_ERROR_KEY_IS_NULL;
                return NULL;
            }

            const int sequenceBytes = GetPacketSequenceBytes( prefixByte );

            const int prefixBytes = 1 + sequenceBytes;

            if ( packetBytes <= prefixBytes + MacBytes )
            {
                m_error = PACKET_PROCESSOR_ERROR_PACKET_TOO_SMALL;
                return NULL;
            }

            sequence = DecompressPacketSequence( prefixByte, packetData + 1 );

            int decryptedPacketBytes;

            memcpy( m_scratchBuffer, packetData + prefixBytes, packetBytes - prefixBytes );

            if ( !Decrypt( m_scratchBuffer, packetBytes - prefixBytes, m_scratchBuffer, decryptedPacketBytes, (uint8_t*)&sequence, key ) )
            {
                m_error = PACKET_PROCESSOR_ERROR_DECRYPT_FAILED;
                return NULL;
            }

            protocol2::PacketInfo info;

            info.context = m_context;
            info.protocolId = m_protocolId;
            info.packetFactory = m_packetFactory;
            info.allowedPacketTypes = encryptedPacketTypes;

            int readError;
            
            protocol2::Packet * packet = protocol2::ReadPacket( info, m_scratchBuffer, decryptedPacketBytes, NULL, &readError );

            if ( !packet )
            {
                m_error = PACKET_PROCESSOR_ERROR_READ_PACKET_FAILED;
                return NULL;
            }
            
            return packet;
        }
        else
        {
            protocol2::PacketInfo info;
            
            info.context = m_context;
            info.protocolId = m_protocolId;
            info.packetFactory = m_packetFactory;
            info.allowedPacketTypes = unencryptedPacketTypes;
            info.prefixBytes = 1;

            sequence = 0;
            
            int readError;

            protocol2::Packet * packet = protocol2::ReadPacket( info, packetData, packetBytes, NULL, &readError );

            if ( !packet )
            {
                m_error = PACKET_PROCESSOR_ERROR_READ_PACKET_FAILED;
                return NULL;
            }
            
            return packet;
        }
    }
}
