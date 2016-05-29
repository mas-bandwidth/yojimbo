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

#include "yojimbo_socket_interface.h"
#include "yojimbo_array.h"
#include "yojimbo_queue.h"
#include "yojimbo_util.h"
#include <stdint.h>
#include <inttypes.h>

namespace yojimbo
{
    typedef network2::Socket NetworkSocket;

    SocketInterface::SocketInterface( Allocator & allocator, 
                                      protocol2::PacketFactory & packetFactory, 
                                      uint32_t protocolId,
                                      uint16_t socketPort, 
                                      network2::SocketType socketType, 
                                      int maxPacketSize, 
                                      int sendQueueSize, 
                                      int receiveQueueSize )
        : m_sendQueue( allocator ),
          m_receiveQueue( allocator )
    {
        assert( protocolId != 0 );
        assert( sendQueueSize > 0 );
        assert( receiveQueueSize > 0 );

        m_context = NULL;

        m_allocator = &allocator;
        
        m_socket = YOJIMBO_NEW( *m_allocator, NetworkSocket, socketPort, socketType );
        
        m_protocolId = protocolId;

        m_sendQueueSize = sendQueueSize;

        m_receiveQueueSize = receiveQueueSize;

        m_packetFactory = &packetFactory;
        
        m_packetProcessor = new PacketProcessor( packetFactory, m_protocolId, maxPacketSize );
        
        queue_reserve( m_sendQueue, sendQueueSize );
        queue_reserve( m_receiveQueue, receiveQueueSize );

        const int numPacketTypes = m_packetFactory->GetNumPacketTypes();

		assert( numPacketTypes > 0 );

        m_packetTypeIsEncrypted = (uint8_t*) m_allocator->Allocate( numPacketTypes );
        m_packetTypeIsUnencrypted = (uint8_t*) m_allocator->Allocate( numPacketTypes );

        memset( m_packetTypeIsEncrypted, 0, m_packetFactory->GetNumPacketTypes() );
        memset( m_packetTypeIsUnencrypted, 1, m_packetFactory->GetNumPacketTypes() );

        m_numEncryptionMappings = 0;

        memset( m_counters, 0, sizeof( m_counters ) );
    }

    SocketInterface::~SocketInterface()
    {
        assert( m_socket );

        ClearSendQueue();
        ClearReceiveQueue();

        YOJIMBO_DELETE( *m_allocator, NetworkSocket, m_socket );

        m_allocator->Free( m_packetTypeIsEncrypted );
        m_allocator->Free( m_packetTypeIsUnencrypted );

        delete m_packetProcessor;

        m_socket = NULL;
        m_packetFactory = NULL;
        m_packetTypeIsEncrypted = NULL;
        m_packetTypeIsUnencrypted = NULL;

        m_packetProcessor = NULL;

        m_allocator = NULL;
    }

    void SocketInterface::ClearSendQueue()
    {
        for ( int i = 0; i < (int) queue_size( m_sendQueue ); ++i )
        {
            PacketEntry & entry = m_sendQueue[i];
            assert( entry.packet );
            assert( entry.address.IsValid() );
            m_packetFactory->DestroyPacket( entry.packet );
        }

        queue_clear( m_sendQueue );
    }

    void SocketInterface::ClearReceiveQueue()
    {
        for ( int i = 0; i < (int) queue_size( m_receiveQueue ); ++i )
        {
            PacketEntry & entry = m_receiveQueue[i];
            assert( entry.packet );
            assert( entry.address.IsValid() );
            m_packetFactory->DestroyPacket( entry.packet );
        }

        queue_clear( m_receiveQueue );
    }

    bool SocketInterface::IsError() const
    {
        assert( m_socket );
        return m_socket->IsError();
    }

    int SocketInterface::GetError() const
    {
        assert( m_socket );
        return m_socket->GetError();
    }

    protocol2::Packet * SocketInterface::CreatePacket( int type )
    {
        assert( m_packetFactory );
        return m_packetFactory->CreatePacket( type );
    }

    void SocketInterface::DestroyPacket( protocol2::Packet * packet )
    {
        assert( m_packetFactory );
        m_packetFactory->DestroyPacket( packet );
    }

    void SocketInterface::SendPacket( const network2::Address & address, protocol2::Packet * packet, uint64_t sequence )
    {
        assert( m_allocator );
        assert( m_packetFactory );

        assert( packet );
        assert( address.IsValid() );

        if ( IsError() )
        {
            m_packetFactory->DestroyPacket( packet );
            return;
        }

        PacketEntry entry;
        entry.sequence = sequence;
        entry.address = address;
        entry.packet = packet;

        if ( queue_size( m_sendQueue ) >= (size_t)m_sendQueueSize )
        {
            m_counters[SOCKET_INTERFACE_COUNTER_SEND_QUEUE_OVERFLOW]++;
            m_packetFactory->DestroyPacket( packet );
            return;
        }

        queue_push_back( m_sendQueue, entry );

        m_counters[SOCKET_INTERFACE_COUNTER_PACKETS_SENT]++;
    }

    protocol2::Packet * SocketInterface::ReceivePacket( network2::Address & from, uint64_t * /*sequence*/ )
    {
        assert( m_allocator );
        assert( m_packetFactory );

        if ( IsError() )
            return NULL;

        if ( queue_size( m_receiveQueue ) == 0 )
            return NULL;

        const PacketEntry & entry = m_receiveQueue[0];

        queue_consume( m_receiveQueue, 1 );

        assert( entry.packet );
        assert( entry.address.IsValid() );

        from = entry.address;

        m_counters[SOCKET_INTERFACE_COUNTER_PACKETS_RECEIVED]++;

        return entry.packet;
    }

    void SocketInterface::WritePackets( double /*time*/ )
    {
        assert( m_allocator );
        assert( m_socket );
        assert( m_packetFactory );
        assert( m_packetProcessor );

        while ( queue_size( m_sendQueue ) )
        {
            const PacketEntry & entry = m_sendQueue[0];

            assert( entry.packet );
            assert( entry.address.IsValid() );

            queue_consume( m_sendQueue, 1 );

            int packetBytes;

            const bool encrypt = IsEncryptedPacketType( entry.packet->GetType() );

            const uint8_t * key = NULL;

            if ( encrypt)
            {
                EncryptionMapping * encryptionMapping = FindEncryptionMapping( entry.address );
                if ( encryptionMapping )
                    key = encryptionMapping->sendKey;
            }

            const uint8_t * packetData = m_packetProcessor->WritePacket( entry.packet, entry.sequence, packetBytes, encrypt, key );

            if ( !packetData )
            {
                switch ( m_packetProcessor->GetError() )
                {
                    case PACKET_PROCESSOR_ERROR_KEY_IS_NULL:                m_counters[SOCKET_INTERFACE_COUNTER_ENCRYPTION_MAPPING_FAILURES]++;         break;
                    case PACKET_PROCESSOR_ERROR_ENCRYPT_FAILED:             m_counters[SOCKET_INTERFACE_COUNTER_ENCRYPT_PACKET_FAILURES]++;             break;
                    case PACKET_PROCESSOR_ERROR_WRITE_PACKET_FAILED:        m_counters[SOCKET_INTERFACE_COUNTER_WRITE_PACKET_FAILURES]++;               break;

                    default:
                        break;
                }

                continue;
            }

            m_socket->SendPacket( entry.address, packetData, packetBytes );

            m_packetFactory->DestroyPacket( entry.packet );

            m_counters[SOCKET_INTERFACE_COUNTER_PACKETS_WRITTEN]++;

            if ( encrypt )
                m_counters[SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_WRITTEN]++;
            else
                m_counters[SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_WRITTEN]++;
        }
    }

    void SocketInterface::ReadPackets( double /*time*/ )
    {
        assert( m_allocator );
        assert( m_socket );
        assert( m_packetFactory );
        assert( m_packetProcessor );

        const int maxPacketSize = GetMaxPacketSize();

        uint8_t * packetBuffer = (uint8_t*) alloca( maxPacketSize );

        while ( true )
        {
            network2::Address address;
            int packetBytes = m_socket->ReceivePacket( address, packetBuffer, maxPacketSize );
            if ( !packetBytes )
                break;

            assert( packetBytes > 0 );

            if ( queue_size( m_receiveQueue ) == (size_t) m_receiveQueueSize )
            {
                m_counters[SOCKET_INTERFACE_COUNTER_RECEIVE_QUEUE_OVERFLOW]++;
                break;
            }

            uint64_t sequence;

            const uint8_t * key = NULL;

            EncryptionMapping * encryptionMapping = FindEncryptionMapping( address );
            if ( encryptionMapping )
                key = encryptionMapping->receiveKey;

            bool encrypted = false;

            protocol2::Packet * packet = m_packetProcessor->ReadPacket( packetBuffer, sequence, packetBytes, encrypted, key, m_packetTypeIsEncrypted, m_packetTypeIsUnencrypted );

            if ( !packet )
            {
                switch ( m_packetProcessor->GetError() )
                {
                    case PACKET_PROCESSOR_ERROR_KEY_IS_NULL:                m_counters[SOCKET_INTERFACE_COUNTER_ENCRYPTION_MAPPING_FAILURES]++;        break;
                    case PACKET_PROCESSOR_ERROR_DECRYPT_FAILED:             m_counters[SOCKET_INTERFACE_COUNTER_ENCRYPT_PACKET_FAILURES]++;            break;
                    case PACKET_PROCESSOR_ERROR_PACKET_TOO_SMALL:           m_counters[SOCKET_INTERFACE_COUNTER_DECRYPT_PACKET_FAILURES]++;            break;
                    case PACKET_PROCESSOR_ERROR_READ_PACKET_FAILED:         m_counters[SOCKET_INTERFACE_COUNTER_READ_PACKET_FAILURES]++;               break;

                    default:
                        break;
                }

                continue;
            }

            PacketEntry entry;
            entry.sequence = 0;
            entry.packet = packet;
            entry.address = address;

            queue_push_back( m_receiveQueue, entry );

            m_counters[SOCKET_INTERFACE_COUNTER_PACKETS_READ]++;

            if ( encrypted )
                m_counters[SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_READ]++;
            else
                m_counters[SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_READ]++;
        }
    }

    int SocketInterface::GetMaxPacketSize() const 
    {
        return m_packetProcessor->GetMaxPacketSize();
    }

    void SocketInterface::SetContext( void * context )
    {
        m_context = context;
    }

    void SocketInterface::EnablePacketEncryption()
    {
        memset( m_packetTypeIsEncrypted, 1, m_packetFactory->GetNumPacketTypes() );
        memset( m_packetTypeIsUnencrypted, 0, m_packetFactory->GetNumPacketTypes() );
    }

    void SocketInterface::DisableEncryptionForPacketType( int type )
    {
        assert( type >= 0 );
        assert( type < m_packetFactory->GetNumPacketTypes() );
        m_packetTypeIsEncrypted[type] = 0;
        m_packetTypeIsUnencrypted[type] = 1;
    }

    bool SocketInterface::IsEncryptedPacketType( int type ) const
    {
        assert( type >= 0 );
        assert( type < m_packetFactory->GetNumPacketTypes() );
        return m_packetTypeIsEncrypted[type] != 0;
    }

    bool SocketInterface::AddEncryptionMapping( const network2::Address & address, const uint8_t * sendKey, const uint8_t * receiveKey )
    {
        EncryptionMapping *encryptionMapping = FindEncryptionMapping( address );
        if ( encryptionMapping )
        {
            encryptionMapping->address = address;
            memcpy( encryptionMapping->sendKey, sendKey, KeyBytes );
            memcpy( encryptionMapping->receiveKey, receiveKey, KeyBytes );
            return true;
        }

        assert( m_numEncryptionMappings >= 0 );
        assert( m_numEncryptionMappings <= MaxEncryptionMappings );

        if ( m_numEncryptionMappings == MaxEncryptionMappings )
            return false;

        encryptionMapping = &m_encryptionMappings[m_numEncryptionMappings++];
        encryptionMapping->address = address;
        memcpy( encryptionMapping->sendKey, sendKey, KeyBytes );
        memcpy( encryptionMapping->receiveKey, receiveKey, KeyBytes );

        return true;
    }

    bool SocketInterface::RemoveEncryptionMapping( const network2::Address & /*address*/ )
    {
        // todo: implement this and consider a different data structure. this is not great.
        assert( !"not implemented yet" );
        return false;
    }

    void SocketInterface::ResetEncryptionMappings()
    {
        m_numEncryptionMappings = 0;
    }

    uint64_t SocketInterface::GetCounter( int index ) const
    {
        assert( index >= 0 );
        assert( index < SOCKET_INTERFACE_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }
}
