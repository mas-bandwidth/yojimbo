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

#include "yojimbo_packet_processor.h"
#include "yojimbo_encryption.h"
#include "yojimbo_allocator.h"
#include "yojimbo_packet.h"
#include "yojimbo_common.h"
#include <stdio.h>
#include <sodium.h>

namespace yojimbo
{
    const int MaxPrefixBytes = 9;

    PacketProcessor::PacketProcessor( Allocator & allocator, uint32_t protocolId, int maxPacketSize )
    {
        m_allocator = &allocator;

        m_protocolId = protocolId;

        m_error = PACKET_PROCESSOR_ERROR_NONE;

        m_maxPacketSize = maxPacketSize + ( ( maxPacketSize % 4 ) ? ( 4 - ( maxPacketSize % 4 ) ) : 0 );
        
        assert( m_maxPacketSize % 4 == 0 );
        assert( m_maxPacketSize >= maxPacketSize );

        m_absoluteMaxPacketSize = maxPacketSize + MaxPrefixBytes + MacBytes;

        m_context = NULL;

        m_packetBuffer = (uint8_t*) allocator.Allocate( m_absoluteMaxPacketSize );

        m_scratchBuffer = (uint8_t*) allocator.Allocate( m_absoluteMaxPacketSize );
    }

    PacketProcessor::~PacketProcessor()
    {
        m_allocator->Free( m_packetBuffer );
        m_allocator->Free( m_scratchBuffer );

        m_packetBuffer = NULL;
        m_scratchBuffer = NULL;
    }

    void PacketProcessor::SetContext( void * context )
    {
        m_context = context;
    }

    static const int ENCRYPTED_PACKET_FLAG = (1<<7);

    const uint8_t * PacketProcessor::WritePacket( Packet * packet, uint64_t sequence, int & packetBytes, bool encrypt, const uint8_t * key, Allocator & streamAllocator, PacketFactory & packetFactory )
    {
        m_error = PACKET_PROCESSOR_ERROR_NONE;

        if ( encrypt )
        {
            if ( !key )
            {
                debug_printf( "packet processor (write packet): key is null\n" );
                m_error = PACKET_PROCESSOR_ERROR_KEY_IS_NULL;
                return NULL;
            }

            int prefixBytes;
            compress_packet_sequence( sequence, m_scratchBuffer[0], prefixBytes, m_scratchBuffer+1 );
            m_scratchBuffer[0] |= ENCRYPTED_PACKET_FLAG;
            prefixBytes++;

            PacketReadWriteInfo info;
            info.context = m_context;
            info.protocolId = m_protocolId;
            info.packetFactory = &packetFactory;
            info.streamAllocator = &streamAllocator;
            info.rawFormat = 1;

            packetBytes = yojimbo::WritePacket( info, packet, m_packetBuffer, m_maxPacketSize );
            if ( packetBytes <= 0 )
            {
                debug_printf( "packet processor (write packet): write packet failed\n" );
                m_error = PACKET_PROCESSOR_ERROR_WRITE_PACKET_FAILED;
                return NULL;
            }

            assert( packetBytes <= m_maxPacketSize );

            int encryptedPacketSize;

            if ( !Encrypt( m_packetBuffer,
                           packetBytes,
                           m_scratchBuffer + prefixBytes,
                           encryptedPacketSize, 
                           (uint8_t*) &sequence, key ) )
            {
                debug_printf( "packet processor (write packet): encrypt packet failed\n" );
                m_error = PACKET_PROCESSOR_ERROR_ENCRYPT_FAILED;
                return NULL;
            }
            
            packetBytes = prefixBytes + encryptedPacketSize;

            assert( packetBytes <= m_absoluteMaxPacketSize );

            return m_scratchBuffer;
        }
        else
        {
            PacketReadWriteInfo info;
            info.context = m_context;
            info.protocolId = m_protocolId;
            info.packetFactory = &packetFactory;
            info.streamAllocator = &streamAllocator;
            info.prefixBytes = 1;

            packetBytes = yojimbo::WritePacket( info, packet, m_packetBuffer, m_maxPacketSize );

            if ( packetBytes <= 0 )
            {
                debug_printf( "packet processor (write packet): write packet failed (unencrypted)\n" );
                m_error = PACKET_PROCESSOR_ERROR_WRITE_PACKET_FAILED;
                return NULL;
            }

            assert( packetBytes <= m_maxPacketSize );

            return m_packetBuffer;
        }
    }

    Packet * PacketProcessor::ReadPacket( const uint8_t * packetData, 
                                          uint64_t & sequence, 
                                          int packetBytes, 
                                          bool & encrypted,  
                                          const uint8_t * key, 
                                          const uint8_t * encryptedPacketTypes, 
                                          const uint8_t * unencryptedPacketTypes,
                                          Allocator & streamAllocator,
                                          PacketFactory & packetFactory )
    {
        m_error = PACKET_PROCESSOR_ERROR_NONE;

        const uint8_t prefixByte = packetData[0];

        encrypted = ( prefixByte & ENCRYPTED_PACKET_FLAG ) != 0;

        if ( encrypted )
        {
            if ( !key )
            {
                debug_printf( "packet processor (read packet): key is null\n" );
                m_error = PACKET_PROCESSOR_ERROR_KEY_IS_NULL;
                return NULL;
            }

            const int sequenceBytes = get_packet_sequence_bytes( prefixByte );

            const int prefixBytes = 1 + sequenceBytes;

            if ( packetBytes <= prefixBytes + MacBytes )
            {
                debug_printf( "packet processor (read packet): packet is too small\n" );
                m_error = PACKET_PROCESSOR_ERROR_PACKET_TOO_SMALL;
                return NULL;
            }

            sequence = decompress_packet_sequence( prefixByte, packetData + 1 );

            int decryptedPacketBytes;

            if ( !Decrypt( packetData + prefixBytes, packetBytes - prefixBytes, m_scratchBuffer, decryptedPacketBytes, (uint8_t*)&sequence, key ) )
            {
                debug_printf( "packet processor (read packet): decrypt failed\n" );
                m_error = PACKET_PROCESSOR_ERROR_DECRYPT_FAILED;
                return NULL;
            }

            PacketReadWriteInfo info;
            info.context = m_context;
            info.protocolId = m_protocolId;
            info.packetFactory = &packetFactory;
            info.streamAllocator = &streamAllocator;
            info.allowedPacketTypes = encryptedPacketTypes;
            info.rawFormat = 1;

            int readError;
            
            Packet * packet = yojimbo::ReadPacket( info, m_scratchBuffer, decryptedPacketBytes, &readError );

            if ( !packet )
            {
                debug_printf( "packet processor (read packet): read packet failed\n" );
                m_error = PACKET_PROCESSOR_ERROR_READ_PACKET_FAILED;
                return NULL;
            }

            return packet;
        }
        else
        {
            PacketReadWriteInfo info;
            info.context = m_context;
            info.protocolId = m_protocolId;
            info.packetFactory = &packetFactory;
            info.streamAllocator = &streamAllocator;
            info.allowedPacketTypes = unencryptedPacketTypes;
            info.prefixBytes = 1;

            sequence = 0;
            
            int readError;

            Packet * packet = yojimbo::ReadPacket( info, packetData, packetBytes, &readError );

            if ( !packet )
            {
                debug_printf( "packet processor (read packet): read packet failed (unencrypted)\n" );
                m_error = PACKET_PROCESSOR_ERROR_READ_PACKET_FAILED;
                return NULL;
            }
            
            return packet;
        }
    }
}
