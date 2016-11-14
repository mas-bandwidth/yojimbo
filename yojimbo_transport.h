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

#ifndef YOJIMBO_TRANSPORT_H
#define YOJIMBO_TRANSPORT_H

#include "yojimbo_config.h"
#include "yojimbo_queue.h"
#include "yojimbo_common.h"
#include "yojimbo_packet.h"
#include "yojimbo_network.h"
#include "yojimbo_context.h"
#include "yojimbo_allocator.h"
#include "yojimbo_encryption.h"
#include "yojimbo_packet_processor.h"

namespace yojimbo
{
    enum TransportFlags
    {
        TRANSPORT_FLAG_INSECURE_MODE = (1<<0)
    };

    enum TransportCounters
    {
        TRANSPORT_COUNTER_PACKETS_SENT,
        TRANSPORT_COUNTER_PACKETS_RECEIVED,
        TRANSPORT_COUNTER_PACKETS_READ,
        TRANSPORT_COUNTER_PACKETS_WRITTEN,
        TRANSPORT_COUNTER_SEND_QUEUE_OVERFLOW,
        TRANSPORT_COUNTER_RECEIVE_QUEUE_OVERFLOW,
        TRANSPORT_COUNTER_READ_PACKET_FAILURES,
        TRANSPORT_COUNTER_WRITE_PACKET_FAILURES,
        TRANSPORT_COUNTER_ENCRYPT_PACKET_FAILURES,
        TRANSPORT_COUNTER_DECRYPT_PACKET_FAILURES,
        TRANSPORT_COUNTER_ENCRYPTED_PACKETS_READ,
        TRANSPORT_COUNTER_ENCRYPTED_PACKETS_WRITTEN,
        TRANSPORT_COUNTER_UNENCRYPTED_PACKETS_READ,
        TRANSPORT_COUNTER_UNENCRYPTED_PACKETS_WRITTEN,
        TRANSPORT_COUNTER_ENCRYPTION_MAPPING_FAILURES,
        TRANSPORT_COUNTER_NUM_COUNTERS
    };

    struct TransportContext
    {
        TransportContext()
        {
            allocator = NULL;
            packetFactory = NULL;
            replayProtection = NULL;
            connectionContext = NULL;
            userContext = NULL;
        }

        TransportContext( Allocator & _allocator, PacketFactory & _packetFactory )
        {
            allocator = &_allocator;
            packetFactory = &_packetFactory;
            replayProtection = NULL;
            connectionContext = NULL;
            userContext = NULL;
        }

        Allocator * allocator;
        PacketFactory * packetFactory;
        ReplayProtection * replayProtection;
        struct ConnectionContext * connectionContext;
        void * userContext;
    };

    class TransportContextManager
    {
    public:

        TransportContextManager();

        bool AddContextMapping( const Address & address, const TransportContext & context );

        bool RemoveContextMapping( const Address & address );

        void ResetContextMappings();

        const TransportContext * GetContext( const Address & address ) const;

    private:

        int m_numContextMappings;
        int m_allocated[MaxContextMappings];
        Address m_address[MaxContextMappings];
        TransportContext m_context[MaxContextMappings];
    };

    class Transport
    {
    public:

        virtual ~Transport() {}

        virtual void Reset() = 0;

        virtual void SetContext( const TransportContext & context ) = 0;

        virtual void ClearContext() = 0;

        virtual void SendPacket( const Address & address, Packet * packet, uint64_t sequence = 0, bool immediate = false ) = 0;

        virtual Packet * ReceivePacket( Address & from, uint64_t * sequence = NULL ) = 0;

        virtual void WritePackets() = 0;

        virtual void ReadPackets() = 0;

        virtual int GetMaxPacketSize() const = 0;

        virtual void SetNetworkConditions( float latency, float jitter, float packetLoss, float duplicate ) = 0;

        virtual void ClearNetworkConditions() = 0;

        virtual void EnablePacketEncryption() = 0;

        virtual void DisablePacketEncryption() = 0;

        virtual void DisableEncryptionForPacketType( int type ) = 0;

        virtual bool IsEncryptedPacketType( int type ) const = 0;

        virtual bool AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey ) = 0;

        virtual bool RemoveEncryptionMapping( const Address & address ) = 0;

        virtual void ResetEncryptionMappings() = 0;

        virtual bool AddContextMapping( const Address & address, const TransportContext & context ) = 0;

        virtual bool RemoveContextMapping( const Address & address ) = 0;

        virtual void ResetContextMappings() = 0;

        virtual void AdvanceTime( double time ) = 0;

        virtual double GetTime() const = 0;

        virtual uint64_t GetCounter( int index ) const = 0;

        virtual void SetFlags( uint64_t flags ) = 0;

        virtual uint64_t GetFlags() const = 0;

        virtual const Address & GetAddress() const = 0;
    };

    class BaseTransport : public Transport
    {
    public:

        BaseTransport( Allocator & allocator,
                       const Address & address,
                       uint32_t protocolId,
                       double time,
                       int maxPacketSize = DefaultMaxPacketSize,
                       int sendQueueSize = DefaultPacketSendQueueSize,
                       int receiveQueueSize = DefaultPacketReceiveQueueSize,
                       bool allocateNetworkSimulator = true );

        void SetContext( const TransportContext & context );

        void ClearContext();

        ~BaseTransport();

        void Reset();

        void SendPacket( const Address & address, Packet * packet, uint64_t sequence, bool immediate );

        Packet * ReceivePacket( Address & from, uint64_t * sequence );

        void WritePackets();

        void ReadPackets();

        int GetMaxPacketSize() const;

        void SetNetworkConditions( float latency, float jitter, float packetLoss, float duplicate );

        void ClearNetworkConditions();

        void EnablePacketEncryption();

        void DisablePacketEncryption();

        void DisableEncryptionForPacketType( int type );

        bool IsEncryptedPacketType( int type ) const;

        bool AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey );

        bool RemoveEncryptionMapping( const Address & address );

        void ResetEncryptionMappings();

        bool AddContextMapping( const Address & address, const TransportContext & context );

        bool RemoveContextMapping( const Address & address );

        void ResetContextMappings();

        void AdvanceTime( double time );

        double GetTime() const;

        uint64_t GetCounter( int index ) const;

        void SetFlags( uint64_t flags );

        uint64_t GetFlags() const;

        const Address & GetAddress() const;

    protected:

        void ClearSendQueue();

        void ClearReceiveQueue();

        const uint8_t * WritePacket( const Address & address, Packet * packet, uint64_t sequence, int & packetBytes );

        void WritePacketToSimulator( const Address & address, Packet * packet, uint64_t sequence );

        void WriteAndFlushPacket( const Address & address, Packet * packet, uint64_t sequence );

        Packet * ReadPacket( const Address & address, uint8_t * packetBuffer, int packetBytes, uint64_t & sequence );

        virtual bool ShouldPacketGoThroughSimulator();

        virtual void InternalSendPacket( const Address & to, const void * packetData, int packetBytes ) = 0;
    
        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize ) = 0;

        Allocator & GetAllocator() { assert( m_allocator ); return *m_allocator; }

    protected:

        TransportContext m_context;

        Address m_address;

        double m_time;

        uint64_t m_flags;

        void * m_userContext;

        uint32_t m_protocolId;

        Allocator * m_allocator;

        PacketProcessor * m_packetProcessor;

        struct PacketEntry
        {
            PacketEntry()
            {
                sequence = 0;
                packet = NULL;
            }

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

        EncryptionManager * m_encryptionManager;

        TransportContextManager * m_contextManager;

        bool m_allocateNetworkSimulator;

        class NetworkSimulator * m_networkSimulator;

        uint64_t m_counters[TRANSPORT_COUNTER_NUM_COUNTERS];
    };

    class LocalTransport : public BaseTransport
    {
    public:

        LocalTransport( Allocator & allocator,
                        class NetworkSimulator & networkSimulator,
                        const Address & address,
                        uint32_t protocolId,
                        double time,
                        int maxPacketSize = DefaultMaxPacketSize,
                        int sendQueueSize = DefaultPacketSendQueueSize,
                        int receiveQueueSize = DefaultPacketReceiveQueueSize );

        ~LocalTransport();

        void Reset();

        void AdvanceTime( double time );

    protected:

        void DiscardReceivePackets();

        void PumpReceivePacketsFromSimulator();

        void InternalSendPacket( const Address & to, const void * packetData, int packetBytes );
    
        int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        int m_receivePacketIndex;                           // current index into receive packet (for InternalReceivePacket)
        int m_numReceivePackets;                            // number of receive packets from last AdvanceTime update
        int m_maxReceivePackets;                            // size of receive packet buffer (matches size of packet receive queue)
        uint8_t ** m_receivePacketData;                     // array of packet data to be received. pointers are owned and must be freed with simulator allocator.
        int * m_receivePacketBytes;                         // array of packet sizes in bytes.
        Address * m_receiveFrom;                            // array of packet from addresses.
    };

#if YOJIMBO_SOCKETS

    class NetworkTransport : public BaseTransport
    {
    public:

        NetworkTransport( Allocator & allocator,
                          const Address & address,
                          uint32_t protocolId,
                          double time,
                          int maxPacketSize = DefaultMaxPacketSize,
                          int sendQueueSize = DefaultPacketSendQueueSize,
                          int receiveQueueSize = DefaultPacketReceiveQueueSize,
                          int socketBufferSize = DefaultSocketBufferSize );

        ~NetworkTransport();

        bool IsError() const;

        int GetError() const;

    protected:

        virtual void InternalSendPacket( const Address & to, const void * packetData, int packetBytes );
    
        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        class Socket * m_socket;
    };

#endif // #if YOJIMBO_SOCKETS
}

#endif // #ifndef YOJIMBO_INTERFACE_H
