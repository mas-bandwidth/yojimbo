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
#include "yojimbo_allocator.h"
#include "yojimbo_encryption.h"
#include "yojimbo_packet_processor.h"

namespace yojimbo
{
    /// Transport flags are used to enable and disable features on a transport.

    enum TransportFlags
    {
        TRANSPORT_FLAG_INSECURE_MODE = (1<<0)                                       ///< When insecure secure mode is enabled on a transport, it supports receiving unencrypted packet types that would normally be rejected if they weren't encrypted. Don't turn this on in production!
    };

    /**
        Transport counters provide insight into the number of times an action was performed by the transport.

        They are intended for use in a telemetry system, eg. the server would report these counters to some backend logging system to track behavior in a production environment.

        They're also pretty useful for debugging and seeing what happened after the fact, and functional testing because counters let tests verify that expected codepaths were hit.
     */

    enum TransportCounters
    {
        TRANSPORT_COUNTER_PACKETS_SENT,                                             ///< Number of packets sent by the transport. Corresponds to the number of times Transport::SendPacket was called by the user.
        TRANSPORT_COUNTER_PACKETS_RECEIVED,                                         ///< Number of packets received by the transport. Corresponds to the number of times Transport::ReceivePacket returned a non-NULL packet pointer to the user.
        TRANSPORT_COUNTER_PACKETS_READ,                                             ///< Number of packets read from the network.
        TRANSPORT_COUNTER_PACKETS_WRITTEN,                                          ///< Number of packets written to the network.
        TRANSPORT_COUNTER_SEND_QUEUE_OVERFLOW,                                      ///< Number of times the packet send queue has overflowed. When this is non-zero, it means that packets are being dropped because the transport send queue is too small.
        TRANSPORT_COUNTER_RECEIVE_QUEUE_OVERFLOW,                                   ///< Number of times the packet receive queue has overflowed. When this is non-zero, packets read from the network aren't able to be queued up and are dropped because there is no room in the receive queue.
        TRANSPORT_COUNTER_READ_PACKET_FAILURES,                                     ///< Number of times a failure has occured while reading a packet. This usually corresponds to the number of corrupt packet that return false from their serialize read function, but it's also possible it is user error causing packets to fail to deserialize properly.
        TRANSPORT_COUNTER_WRITE_PACKET_FAILURES,                                    ///< Number of times a packet failed to serialize write. This is not common. If it does fail, it's probably you're fault. You're the one who wrote the packet serialize function after all :)
        TRANSPORT_COUNTER_ENCRYPT_PACKET_FAILURES,                                  ///< Number of times packet encryption failed. This usually only fails if there is something wrong with libsodium installed on your system.
        TRANSPORT_COUNTER_DECRYPT_PACKET_FAILURES,                                  ///< Number of times libsodium failed to decrypt a packet. Non-zero indicates that corrupt packets were received and could not be decrypted.
        TRANSPORT_COUNTER_ENCRYPTED_PACKETS_READ,                                   ///< Number of encrypted packet read from the network.
        TRANSPORT_COUNTER_ENCRYPTED_PACKETS_WRITTEN,                                ///< Number of encrypted packets written to the network.
        TRANSPORT_COUNTER_UNENCRYPTED_PACKETS_READ,                                 ///< Number of unencrypted packets read from the network.
        TRANSPORT_COUNTER_UNENCRYPTED_PACKETS_WRITTEN,                              ///< Number of unencrypted packets written to the network.
        TRANSPORT_COUNTER_ENCRYPTION_MAPPING_FAILURES,                              ///< Number of encryption mapping failures. This is when an encrypted packet is sent to us, but we don't can't find any key to decrypt that packet corresponding to it's source address. See Transport::AddEncryptionMapping.
        TRANSPORT_COUNTER_NUM_COUNTERS                                              ///< The number of transport counters.
    };

    /** 
        Gives the transport access to objects it needs to read and write packets.

        Each transport has a default context set by Transport::SetContext and cleared by Transport::ClearContext. 

        Each transport also provides a context mapping that can be used to associate specific addresses with their own context.

        The server uses per-client contexts to setup a mapping between connected clients and the resources for that client like allocators, packet factories, message factories and replay protection.

        The benefit is that each client is silo'd to their own set of resources and cannot launch an attack to deplete resources shared with other clients.

        @see TransportContext
        @see Transport::SetContext
        @see Transport::ClearContext
        @see Transport::AddContextMapping
        @see Transport::RemoveContextMapping
        @see Transport::ResetContextMappings
     */

    struct TransportContext
    {
        TransportContext()
        {
            Clear();
        }

        TransportContext( Allocator & _allocator, PacketFactory & _packetFactory )
        {
            Clear();
            allocator = &_allocator;
            packetFactory = &_packetFactory;
        }

        void Clear()
        {
            allocator = NULL;
            packetFactory = NULL;
            replayProtection = NULL;
            connectionContext = NULL;
            userContext = NULL;
            encryptionIndex = -1;
        }

        Allocator * allocator;                                                      ///< The allocator set on the stream. See Stream::GetAllocator. This lets packets to allocate and free memory as they read and write packets.
        PacketFactory * packetFactory;                                              ///< The packet factory used to create packets.
        ReplayProtection * replayProtection;                                        ///< The replay protection object used to filter out old and duplicate encrypted packets being replayed. Protects against packet replay attacks.
        struct ConnectionContext * connectionContext;                               ///< The connection context. This provides information needed by the ConnectionPacket to read and write messages and data block fragments.
        void * userContext;                                                         ///< The user context. This lets the client pass a pointer to data so it is accessible when reading and writing packets. See Stream::GetUserContext.
        int encryptionIndex;                                                        ///< The encryption index. This is an optimization that avoids repeatedly searching for the encryption mapping by index. If a context is setup for that address, the encryption index is cached in the context.
    };

    /** 
        Maps addresses to transport contexts so each client on the server has its own set of resources.

        Typically, one context mapping is added to the transport per-connected client, allowing the server to silo each client to its own set of resources, eliminating the risk of malicious clients depleting shared resources on the server.

        This is the implementation class for the Transport::AddContextMapping, Transport::RemoveContextMapping and Transport::ResetContextMappings set of functions. It allows multiple transport implementations to reuse the same context mappping implementation.

        @see TransportContext
        @see Transport::AddContextMapping
        @see Transport::RemoveContextMapping
        @see Transport::ResetContextMappings
     */

    class TransportContextManager
    {
    public:

        TransportContextManager();

        /**
            Add a context mapping.

            Maps the specified address to the context mapping. If a context mapping already exists for this address, it is updated with the new context.

            IMPORTANT: The context data is copied across by value and not stored by pointer. All pointers and data inside the context are expected to stay valid while set on the transport.

            @param address The address that maps to the context.
            @param context A reference to the context data to be copied across.
            @returns True if the context mapping was added successfully. False if the context is already added context mapping slots are avalable. See yojimbo::MaxContextMappings.
         */

        bool AddContextMapping( const Address & address, const TransportContext & context );

        /**
            Remove a context mapping.

            Searches for a context mapping for the address passed in, and if it finds one, removes it.

            @returns True if a context mapping was found and removed, false otherwise.
         */

        bool RemoveContextMapping( const Address & address );

        /**
            Reset all context mappings.

            After this function all context mappings are removed.

            Call this if you want to completely reset the context mappings and start fresh.
         */

        void ResetContextMappings();

        /** 
            Find a context by address.

            This is called by the transport when a packet is sent or received, to find the appropriate context to use given the address the packet is being sent to, or received from.

            @param address The address corresponding to the context we want.
            @returns A const pointer to the context if one exists for the address passed in, NULL otherwise.
         */

        const TransportContext * GetContext( const Address & address ) const;

    private:

        int m_numContextMappings;                                                   ///< The current number of context mappings in [0,MaxContextMappings-1]
        int m_allocated[MaxContextMappings];                                        ///< Array of allocated flags per-context entry. True if a context mapping is allocated at this index.
        Address m_address[MaxContextMappings];                                      ///< Array of addresses. Used by the O(n) for a context by address. OK because n is typically small.
        TransportContext m_context[MaxContextMappings];                             ///< Array of context data corresponding to the address at the same index in the address array.
    };

    /** 
        Interface for sending and receiving packets.

        This is the common interface shared by all transport implementations. 

        Transports provide send and receive queues for high level packets objects, taking care of the details of reading and writing binary packets to the network.

        It is intended to allow mixing and matching of different transport implementations with protocol classes like Client, Server and Connection. This way a new transport implementation can be swapped in without any change to protocol level code.
     */

    class Transport
    {
    public:

        virtual ~Transport() {}

        /**
            Reset the transport.

            This function completely resets the transport, clearing all packet send and receive queues and resetting all contexts and encryption mappings.

            This gets the transport back into a pristine state, ready to be used for some other purpose, without needing to destroy and recreate the transport object.
         */

        virtual void Reset() = 0;

        /**
            Set the transport context.

            This is the default context to be used for reading and writing packets. See TransportContext.

            The user can also call Transport::AddContextMapping to associate up to yojimbo::MaxContextMappings contexts with particular addresses. When a context is associated with an address, that context is when reading/writing packets from/to that address instead of this one.

            @param context The default context for reading and writing packets.
         */

        virtual void SetContext( const TransportContext & context ) = 0;

        /**
            Clear context.

            This function clears the default context set on the transport.

            This returns the transport to the state before a context was set on it. While in this state, the transport cannot read or write packets and will early out of Transport::ReadPackets and Transport::WritePackets.

            If the transport is set on a client, it is returned to this state after the client disconnects. On the server, the transport is returned to this state when the server stops.

            @see Client::Disconnect
            @see Server::Stop
         */

        virtual void ClearContext() = 0;

        /**
            Queue a packet to be sent.

            IMPORTANT: The packet will be sent over a UDP-equivalent network. It may arrive out of order, in duplicate or not at all.

            @param address The address the packet should be sent to.
            @param packet The packet that is being sent.
            @param sequence The 64 bit sequence number of the packet used as a nonce for packet encryption. Should increase with each packet sent per-encryption mapping, but this is up to the caller.
            @param immediate If true the the packet is written and flushed to the network immediately, rather than in the next call to Transport::WritePackets.
         */

        virtual void SendPacket( const Address & address, Packet * packet, uint64_t sequence = 0, bool immediate = false ) = 0;

        /**
            Receive a packet.

            This function pops a packet off the receive queue, if any are available to be received.

            To make sure packet latency is minimized call Transport::ReceivePackets just before looping and calling this function until it returns NULL.

            @param from The address that the packet was received from.
            @param sequence Pointer to an unsigned 64bit sequence number (optional). If the pointer is not NULL, it will be dereferenced to store the sequence number of the received packet. Only encrypted packets have these sequence numbers.
            @returns The next packet in the receive queue, or NULL if no packets are left in the receive queue.
         */

        virtual Packet * ReceivePacket( Address & from, uint64_t * sequence = NULL ) = 0;

        /**
            Iterates across all packets in the send queue and writes them to the network.

            To minimize packet latency, call this function shortly after you have finished queueing up packets to send via Transport::SendPacket.
         */

        virtual void WritePackets() = 0;

        /**
            Reads packets from the network and adds them to the packet receive queue.

            To minimize packet latency call this function right before you loop calling Transport::ReceivePacket until it returns NULL.
        */

        virtual void ReadPackets() = 0;

        /**
            Returns the maximum packet size supported by this transport.

            This is typically configured in the constructor of the transport implementation.

            It's added here so callers using the transport interface know the maximum packet size that can be sent over the transport.

            @returns The maximum packet size that can be sent over this transport (bytes).
         */

        virtual int GetMaxPacketSize() const = 0;

        /** 
            Add simulated network conditions on top of this transport.

            By default no latency, jitter, packet loss, or duplicates are added on top of the network. 

            However, during development you often want to simulate some latency and packet loss while the game is being played over the LAN,
            so people developing your game do so under conditions that you can expect to encounter when it's being played over the Internet.

            I recommend adding around 125ms of latency and somewhere between 2-5% packet loss, and +/- one frame of jitter @ 60HZ (eg. 20ms or so). 

            IMPORTANT: Take care when sitting simulated network conditions because they are implemented on packet send only, so you usually want to set HALF of the latency you want on client, and HALF of the latency on the server transport. So for 100ms of latency in total, you'd set 50ms latency on the client transport, and 50ms latency on the server transport, which adds up to of 100ms extra round trip delay.

            The network simulator allocated here can take up a significant amount of memory. If you want to save memory, you might want to disable the network simulator. You can do this in the constructor of your transport implementation. See NetworkTransport::NetworkTransport for details.

            @param latency The amount of latency to add to each packet sent (milliseconds).
            @param jitter The amount of jitter to add to each packet sent (milliseconds). The packet delivery time is adjusted by some random amount within +/- jitter. This is best used in combination with some amount of latency, otherwise the jitter is not truly +/-.
            @param packetLoss The percentage of packets to drop on send. 100% drops all packets. 0% drops no packets.
            @param duplicate The percentage of packets to be duplicated. Duplicate packets are scheduled to be sent at some random time up to 1 second in the future, to grossly approximate duplicate packets that occur from IP route changes.
         */

        virtual void SetNetworkConditions( float latency, float jitter, float packetLoss, float duplicate ) = 0;

        /**
            Clear network conditions back to defaults.

            After this function latency, jitter, packet loss and duplicate packets are all set back to zero.
         */

        virtual void ClearNetworkConditions() = 0;

        /** 
            Turns on packet encryption and enables it for all packet types.

            When a packet is sent the transport checks if that packet type is encrypted. If it's an encrypted, the transport looks for an encryption mapping for the destination address, and encrypts the packet with that key. Otherwise, the packet is sent out unencrypted.

            When a packet is received, the transport checks if that packetet type should be encrypted. If it should be, but the packet itself is not encrypted, the packet is discarded. Otherwise, the server looks for a decryption key for the packet sender address and decrypts the packet before adding it to the packet receive queue.

            The exception to this rule is when you enable TRANSPORT_FLAG_INSECURE_MODE via Transport::SetFlags, which makes encryption optional for encrypted packet types. 

            This allows a mixture of secure and insecure client connections, which is convenient for development over a LAN, but should NEVER be enabled in production. 

            Therefore, the insecure mode flag is disabled #if YOJIMBO_SECURE_MODE 1. 

            Please make sure you #define YOJIMBO_SECURE_MODE 1 in your production build!

            @see Transport::DisablePacketEncryption
            @see Transport::DisableEncryptionForPacketType
         */

        virtual void EnablePacketEncryption() = 0;

        /** 
            Disables encryption for all packet types.

            @see Transport::DisablePacketEncryption
            @see Transport::DisableEncryptionForPacketType
         */

        virtual void DisablePacketEncryption() = 0;

        /**
            Disables encryption for a specific packet type.

            Typical usage is to enable packet encryption (for all types) via Transport::EnablePacketEncryption, and then selectively disable it for packet types that you don't want to be encrypted.

            For example, the client/server protocol sends connection request packets as unencrypted, because they contain connect token data which is already encrypted, and every other packet sent is encrypted.

            @param type The packet type that should be set as not encrypted.
         */

        virtual void DisableEncryptionForPacketType( int type ) = 0;

        /**
            Is a packet type encrypted?

            @returns True if the packet type is an encrypted packet, false otherwise.

            @see EnablePacketEncryption
            @see DisablePacketEncryption
            @see DisableEncryptionForPacketType
         */

        virtual bool IsEncryptedPacketType( int type ) const = 0;

        /**
            Associates an address with keys for packet encryption.

            This mapping is used by the transport on packet send and receive determine what key should be used when sending a packet to an address, and what key should be used to decrypt a packet received from an address.

            For example, the server adds an encryption mapping for clients when it receives a connection request packet with a valid connect token, enabling encrypted packets to be exchanged between the server and that client past that point. The encryption mapping is removed when the client disconnects from the server.

            Encryption mappings also time out, making them ideal for pending clients, who may never reply back and complete the connection negotiation. As a result, there are more encryption mappings than client slots. See yojimbo::MaxEncryptionMappings.

            See EncryptionManager for further details.

            @param address The address to associate with encryption keys.
            @param sendKey The key used to encrypt packets sent to this address.
            @param receiveKey The key used to decrypt packets received from this address.
         */

        virtual bool AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey ) = 0;

        /**
            Remove the encryption mapping for an address.

            @param address The address of the encryption mapping to remove.

            @returns True if an encryption mapping for the address exists and was removed, false if no encryption mapping could be found for the address.
         */

        virtual bool RemoveEncryptionMapping( const Address & address ) = 0;

        /**
            Find the index of an encryption mapping by address.

            @returns The index of the encryption mapping in the range [0,MaxEncryptionMappings-1], or -1 if no encryption mapping exists for the specified address.
         */

        virtual int FindEncryptionMapping( const Address & address ) = 0;

        /**
            Reset all encryption mappings.

            All encryption mappings set on the transport are removed.

            @see Transport::Reset
         */

        virtual void ResetEncryptionMappings() = 0;

        /**
            Add a transport context that is specific for an address.

            Context mappings are used to give each connected client on the server its own set of resources, so malicious clients can't exhaust resources shared with other clients.

            When a client establishes connection with the server, a context mapping is added for that client. When the client disconnects from the server, the context mapping is removed.

            @param address The address to associate with a transport context.
            @param context The transport context to be copied across. All data in the context must remain valid while it is set on the transport.
            
            @see TransportContextManager
            @see yojimbo::MaxContextMappings
         */

        virtual bool AddContextMapping( const Address & address, const TransportContext & context ) = 0;

        /**
            Remove a context mapping.

            Context mappings are used to give each connected client on the server its own set of resources, so malicious clients can't exhaust resources shared with other clients.

            When a client establishes connection with the server, a context mapping is added for that client. When the client disconnects from the server, the context mapping is removed.

            @param address The address of the context to remove.
            @returns True if a context mapping was found at the address and removed, false if no context mapping could be found for the address.
         */

        virtual bool RemoveContextMapping( const Address & address ) = 0;

        /** 
            Reset all context mappings.

            After this function is called all context mappings are removed.
         */

        virtual void ResetContextMappings() = 0;

        /**
            Advance transport time.

            Call this at the end of each frame to advance the transport time forward. 

            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Gets the current transport time.

            @see Transport::AdvanceTime
         */

        virtual double GetTime() const = 0;

        /**
            Get a counter value.

            Counters are used to track event and actions performed by the transport. They are useful for debugging, testing and telemetry.

            @returns The counter value. See yojimbo::TransportCounters for the set of transport counters.
         */

        virtual uint64_t GetCounter( int index ) const = 0;

        /** 
            Reset all counters to zero.

            This is typically used with a telemetry application after uploading the current set of counters to the telemetry backend. 

            This way you can continue to accumulate events, and upload them at some frequency, like every 5 minutes to the telemetry backend, without double counting events.
         */

        virtual void ResetCounters();

        /**
            Set transport flags.

            Flags are used to enable and disable transport functionality.

            @param flags The transport flags to set. See yojimbo::TransportFlags for the set of transport flags that can be passed in.

            @see Transport::GetFlags
         */

        virtual void SetFlags( uint64_t flags ) = 0;

        /**
            Get the current transport flags.

            @returns The transport flags. See yojimbo::TransportFlags for the set of transport flags.
    
            @see Transport::SetFlags
         */

        virtual uint64_t GetFlags() const = 0;

        /**
            Get the address of the transport.

            This is the address that packet should be sent to to be received by this transport.

            @returns The transport address.
         */

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

        int FindEncryptionMapping( const Address & address );

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

#if !YOJIMBO_SECURE_MODE
        uint8_t * m_allPacketTypes;
#endif // #if !YOJIMBO_SECURE_MODE
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
