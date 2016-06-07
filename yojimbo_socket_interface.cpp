/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo_socket_interface.h"
#include "yojimbo_array.h"
#include "yojimbo_queue.h"
#include "yojimbo_common.h"
#include <stdint.h>
#include <inttypes.h>

namespace yojimbo
{
    typedef Socket NetworkSocket;

    SocketInterface::SocketInterface( Allocator & allocator, 
                                      PacketFactory & packetFactory, 
                                      uint32_t protocolId,
                                      uint16_t socketPort, 
                                      SocketType socketType, 
                                      int maxPacketSize, 
                                      int sendQueueSize, 
                                      int receiveQueueSize )
        : m_sendQueue( allocator ),
          m_receiveQueue( allocator )
    {
        assert( protocolId != 0 );
        assert( sendQueueSize > 0 );
        assert( receiveQueueSize > 0 );

        m_time = 0.0;

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

    Packet * SocketInterface::CreatePacket( int type )
    {
        assert( m_packetFactory );
        return m_packetFactory->CreatePacket( type );
    }

    void SocketInterface::DestroyPacket( Packet * packet )
    {
        assert( m_packetFactory );
        m_packetFactory->DestroyPacket( packet );
    }

    void SocketInterface::SendPacket( const Address & address, Packet * packet, uint64_t sequence, bool immediate )
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

        if ( immediate )
        {
            WriteAndFlushPacket( address, packet, sequence );

            m_packetFactory->DestroyPacket( packet );
        }
        else
        {
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
        }

        m_counters[SOCKET_INTERFACE_COUNTER_PACKETS_SENT]++;
    }

    Packet * SocketInterface::ReceivePacket( Address & from, uint64_t * sequence )
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

        if ( sequence )
            *sequence = entry.sequence;

        m_counters[SOCKET_INTERFACE_COUNTER_PACKETS_RECEIVED]++;

        return entry.packet;
    }

    void SocketInterface::WritePackets()
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

            WriteAndFlushPacket( entry.address, entry.packet, entry.sequence );

            m_packetFactory->DestroyPacket( entry.packet );
        }
    }

    void SocketInterface::WriteAndFlushPacket( const Address & address, Packet * packet, uint64_t sequence )
    {
        const double time = GetTime();

        int packetBytes;

        const bool encrypt = IsEncryptedPacketType( packet->GetType() );

        const uint8_t * key = encrypt ? m_encryptionManager.GetSendKey( address, time ) : NULL;

        const uint8_t * packetData = m_packetProcessor->WritePacket( packet, sequence, packetBytes, encrypt, key );

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

            return;
        }

        m_socket->SendPacket( address, packetData, packetBytes );

        m_counters[SOCKET_INTERFACE_COUNTER_PACKETS_WRITTEN]++;

        if ( encrypt )
            m_counters[SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_WRITTEN]++;
        else
            m_counters[SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_WRITTEN]++;
    }

    void SocketInterface::ReadPackets()
    {
        assert( m_allocator );
        assert( m_socket );
        assert( m_packetFactory );
        assert( m_packetProcessor );

        const int maxPacketSize = GetMaxPacketSize();

        uint8_t * packetBuffer = (uint8_t*) alloca( maxPacketSize );

        while ( true )
        {
            Address address;
            int packetBytes = m_socket->ReceivePacket( address, packetBuffer, maxPacketSize );
            if ( !packetBytes )
                break;

            assert( packetBytes > 0 );

            if ( queue_size( m_receiveQueue ) == (size_t) m_receiveQueueSize )
            {
                m_counters[SOCKET_INTERFACE_COUNTER_RECEIVE_QUEUE_OVERFLOW]++;
                break;
            }

            uint64_t sequence = 0;

            const uint8_t * key = m_encryptionManager.GetReceiveKey( address, GetTime() );

            bool encrypted = false;

            Packet * packet = m_packetProcessor->ReadPacket( packetBuffer, sequence, packetBytes, encrypted, key, m_packetTypeIsEncrypted, m_packetTypeIsUnencrypted );

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
            entry.sequence = sequence;
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

    bool SocketInterface::AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey )
    {
        return m_encryptionManager.AddEncryptionMapping( address, sendKey, receiveKey, GetTime() );
    }

    bool SocketInterface::RemoveEncryptionMapping( const Address & address )
    {
        return m_encryptionManager.RemoveEncryptionMapping( address, GetTime() );
    }

    void SocketInterface::ResetEncryptionMappings()
    {
        m_encryptionManager.ResetEncryptionMappings();
    }

    void SocketInterface::AdvanceTime( double time )
    {
        assert( time >= m_time );
        m_time = time;
    }

    double SocketInterface::GetTime() const
    {
        return m_time;
    }

    uint64_t SocketInterface::GetCounter( int index ) const
    {
        assert( index >= 0 );
        assert( index < SOCKET_INTERFACE_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }
}
