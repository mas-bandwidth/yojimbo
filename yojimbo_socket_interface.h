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

#ifndef YOJIMBO_SOCKET_INTERFACE_H
#define YOJIMBO_SOCKET_INTERFACE_H

#include "network2.h"
#include "protocol2.h"
#include "yojimbo_config.h"
#include "yojimbo_util.h"
#include "yojimbo_types.h"
#include "yojimbo_memory.h"
#include "yojimbo_crypto.h"
#include "yojimbo_allocator.h"
#include "yojimbo_network_interface.h"
#include "yojimbo_packet_processor.h"

namespace yojimbo
{
    enum SocketInterfaceCounter
    {
        SOCKET_INTERFACE_COUNTER_PACKETS_SENT,
        SOCKET_INTERFACE_COUNTER_PACKETS_RECEIVED,
        SOCKET_INTERFACE_COUNTER_PACKETS_READ,
        SOCKET_INTERFACE_COUNTER_PACKETS_WRITTEN,
        SOCKET_INTERFACE_COUNTER_SEND_QUEUE_OVERFLOW,
        SOCKET_INTERFACE_COUNTER_RECEIVE_QUEUE_OVERFLOW,
        SOCKET_INTERFACE_COUNTER_READ_PACKET_FAILURES,
        SOCKET_INTERFACE_COUNTER_WRITE_PACKET_FAILURES,
        SOCKET_INTERFACE_COUNTER_ENCRYPT_PACKET_FAILURES,
        SOCKET_INTERFACE_COUNTER_DECRYPT_PACKET_FAILURES,
        SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_READ,
        SOCKET_INTERFACE_COUNTER_ENCRYPTED_PACKETS_WRITTEN,
        SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_READ,
        SOCKET_INTERFACE_COUNTER_UNENCRYPTED_PACKETS_WRITTEN,
        SOCKET_INTERFACE_COUNTER_ENCRYPTION_MAPPING_FAILURES,
        SOCKET_INTERFACE_COUNTER_NUM_COUNTERS
    };

    class SocketInterface : public NetworkInterface
    {
        void * m_context;

        uint32_t m_protocolId;
        int m_sendQueueSize;
        int m_receiveQueueSize;

        Allocator * m_allocator;
        network2::Socket * m_socket;
        protocol2::PacketFactory * m_packetFactory;
        PacketProcessor * m_packetProcessor;

        struct PacketEntry
        {
            uint64_t sequence;
            network2::Address address;
            protocol2::Packet *packet;
        };

        Queue<PacketEntry> m_sendQueue;
        Queue<PacketEntry> m_receiveQueue;

        uint8_t * m_packetTypeIsEncrypted;
        uint8_t * m_packetTypeIsUnencrypted;

        struct EncryptionMapping
        {
            network2::Address address;
            uint8_t sendKey[KeyBytes];
            uint8_t receiveKey[KeyBytes];
        };

        enum { MaxEncryptionMappings = 1024 };
        int m_numEncryptionMappings;
        EncryptionMapping m_encryptionMappings[MaxEncryptionMappings];

        uint64_t m_counters[SOCKET_INTERFACE_COUNTER_NUM_COUNTERS];

    protected:

        void ClearSendQueue();

        void ClearReceiveQueue();

        EncryptionMapping * FindEncryptionMapping( const network2::Address & address )
        {
            for ( int i = 0; i < m_numEncryptionMappings; ++i )
            {
                if ( m_encryptionMappings[i].address == address )
                    return &m_encryptionMappings[i];
            }
            return NULL;
        }

    public:

        SocketInterface( Allocator & allocator,
                         protocol2::PacketFactory & packetFactory, 
                         uint32_t protocolId,
                         uint16_t socketPort, 
                         network2::SocketType socketType = network2::SOCKET_TYPE_IPV6, 
                         int maxPacketSize = 4 * 1024,
                         int sendQueueSize = 1024,
                         int receiveQueueSize = 1024 );

        ~SocketInterface();

        bool IsError() const;

        int GetError() const;

        protocol2::Packet * CreatePacket( int type );

        void DestroyPacket( protocol2::Packet * packet );

        void SendPacket( const network2::Address & address, protocol2::Packet * packet, uint64_t sequence = 0 );

        protocol2::Packet * ReceivePacket( network2::Address & from, uint64_t *sequence = NULL );

        void WritePackets( double time );

        void ReadPackets( double time );

        int GetMaxPacketSize() const;

        void SetContext( void * context );

        void EnablePacketEncryption();

        void DisableEncryptionForPacketType( int type );

        bool IsEncryptedPacketType( int type ) const;

        bool AddEncryptionMapping( const network2::Address & address, const uint8_t * sendKey, const uint8_t * receiveKey );

        bool RemoveEncryptionMapping( const network2::Address & address );

        void ResetEncryptionMappings();

        uint64_t GetCounter( int index ) const;
    };
}

#endif // #ifndef YOJIMBO_SOCKET_INTERFACE_H
