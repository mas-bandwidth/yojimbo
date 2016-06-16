/*
    Yojimbo Client/Server Network Library.

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

#ifndef YOJIMBO_BASE_INTERFACE_H
#define YOJIMBO_BASE_INTERFACE_H

#include "yojimbo_config.h"
#include "yojimbo_queue.h"
#include "yojimbo_common.h"
#include "yojimbo_packet.h"
#include "yojimbo_network.h"
#include "yojimbo_allocator.h"
#include "yojimbo_encryption.h"
#include "yojimbo_packet_processor.h"
#include "yojimbo_network_interface.h"

namespace yojimbo
{
    class BaseInterface : public NetworkInterface
    {
    public:

        BaseInterface( Allocator & allocator,
                       PacketFactory & packetFactory, 
                       const Address & address,
                       uint32_t protocolId,
                       int maxPacketSize = 4 * 1024,
                       int sendQueueSize = 1024,
                       int receiveQueueSize = 1024 );

        ~BaseInterface();

        Packet * CreatePacket( int type );

        void DestroyPacket( Packet * packet );

        void SendPacket( const Address & address, Packet * packet, uint64_t sequence, bool immediate );

        Packet * ReceivePacket( Address & from, uint64_t * sequence );

        void WritePackets();

        void ReadPackets();

        int GetMaxPacketSize() const;

        void SetContext( void * context );

        void EnablePacketEncryption();

        void DisableEncryptionForPacketType( int type );

        bool IsEncryptedPacketType( int type ) const;

        bool AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey );

        bool RemoveEncryptionMapping( const Address & address );

        void ResetEncryptionMappings();

        void AdvanceTime( double time );

        double GetTime() const;

        uint64_t GetCounter( int index ) const;

        void SetFlags( uint64_t flags );

        uint64_t GetFlags() const;

        const Address & GetAddress() const;

    protected:

        void ClearSendQueue();

        void ClearReceiveQueue();

        void WriteAndFlushPacket( const Address & address, Packet * packet, uint64_t sequence );

    protected:

        virtual bool InternalSendPacket( const Address & to, const void * packetData, int packetBytes ) = 0;
    
        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize ) = 0;

        Allocator & GetAllocator() { assert( m_allocator ); return *m_allocator; }

    private:

        Address m_address;

        double m_time;

        uint64_t m_flags;

        void * m_context;

        uint32_t m_protocolId;

        Allocator * m_allocator;
        PacketFactory * m_packetFactory;
        PacketProcessor * m_packetProcessor;

        struct PacketEntry
        {
            uint64_t sequence;
            Address address;
            Packet * packet;
        };

        Queue<PacketEntry> m_sendQueue;
        Queue<PacketEntry> m_receiveQueue;

#if YOJIMBO_INSECURE_CONNECT
        uint8_t * m_allPacketTypes;
#endif // #if YOJIMBO_INSECURE_CONNECT
        uint8_t * m_packetTypeIsEncrypted;
        uint8_t * m_packetTypeIsUnencrypted;

        uint64_t m_counters[NETWORK_INTERFACE_COUNTER_NUM_COUNTERS];

        EncryptionManager m_encryptionManager;
    };
}

#endif // #ifndef YOJIMBO_NETWORK_IMPLEMENTATION_H
