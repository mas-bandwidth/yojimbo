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
#if !YOJIMBO_SECURE_MODE
        TRANSPORT_FLAG_INSECURE_MODE = (1<<0)                                       ///< When insecure secure mode is enabled on a transport, it supports receiving unencrypted packets that would normally be rejected if they weren't encrypted. This allows a mix of secure and insecure clients on the same server. Don't turn this on in production!
#endif // #if !YOJIMBO_SECURE_MODE
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
        Gives the transport access to resources it needs to read and write packets.

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
        Maps addresses to transport contexts so each client on the server can have its own set of resources.

        Typically, one context mapping is added to the transport per-connected client, allowing the server to silo each client to its own set of resources, eliminating the risk of malicious clients depleting shared resources on the server.

        This is the implementation class for the Transport::AddContextMapping, Transport::RemoveContextMapping and Transport::ResetContextMappings set of functions. It allows different transport implementations to reuse the same context mappping implementation.

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

            Ownership of the packet object is transferred to the transport. Don't call Packet::Destroy on the packet after you send it with this function, the transport will do that for you automatically.

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

            This function transfers ownership of the packet pointer to the caller. It is the callers responsibility to call Packet::Destroy after processing the packet.

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

            Please make sure you \#define YOJIMBO_SECURE_MODE 1 in your production build!

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

        virtual void ResetCounters() = 0;

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

        /**
            Get the protocol id for the transport.

            Protocol id is used to enable multiple versions of your protocol at the same time, by excluding communication between protocols with different ids.
         */

        virtual uint32_t GetProtocolId() const = 0;
    };

    /**
        Common functionality shared between multiple transport implementations.
     */

    class BaseTransport : public Transport
    {
    public:

        /**
            Base transport constructor.

            @param allocator The allocator used for transport allocations.
            @param address The address of the packet. This is how other other transports would send packets to this transport.
            @param protocolId The protocol id for this transport. Protocol id is included in the packet header, packets received with a different protocol id are discarded. This allows multiple versions of your protocol to exist on the same network.
            @param time The current time value in seconds.
            @param maxPacketSize The maximum packet size that can be sent across this transport.
            @param sendQueueSize The size of the packet send queue (number of packets).
            @param receiveQueueSize The size of the packet receive queue (number of packets).
            @param allocateNetworkSimulator If true then a network simulator is allocated, otherwise a network simulator is not allocated. You can use this to disable the network simulator if you are not using it, to save memory.
         */

        BaseTransport( Allocator & allocator,
                       const Address & address,
                       uint32_t protocolId,
                       double time,
                       int maxPacketSize = DefaultMaxPacketSize,
                       int sendQueueSize = DefaultPacketSendQueueSize,
                       int receiveQueueSize = DefaultPacketReceiveQueueSize,
                       bool allocateNetworkSimulator = true );

        ~BaseTransport();

        void SetContext( const TransportContext & context );

        void ClearContext();

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

        void ResetCounters();

        void SetFlags( uint64_t flags );

        uint64_t GetFlags() const;

        const Address & GetAddress() const;

        uint32_t GetProtocolId() const;

    protected:

        /// Clear the packet send queue.

        void ClearSendQueue();

        /// Clear the packet receive queue.

        void ClearReceiveQueue();

        /**
            Writes a packet to a scratch buffer and returns a pointer to the packet data.

            If the packet is an encrypted packet type, this function also performs encryption.

            @param address The address of the packet. This is how other other transports would send packets to this transport.
            @param packet The packet object to be serialized (written).
            @param sequence The sequence number of the packet being written. If this is an encrypted packet, this serves as the nonce, and it is the users responsibility to increase this value with each packet sent per-encryption context. Not used for unencrypted packets (pass in zero).
            @param packetBytes The number of packet bytes written to the buffer [out]. This is the wire size of the packet to be sent via sendto or equivalent.

            @returns A const pointer to the packet data written to a scratch buffer. Don't hold on to this pointer and don't free it. The scratch buffer is internal and managed by the transport.
         */

        const uint8_t * WritePacket( const Address & address, Packet * packet, uint64_t sequence, int & packetBytes );

        /**
            Write a packet and queue it up in the network simulator.

            This is used when network simulation is enabled via Transport::SetNetworkConditions to add latency, packet loss and so on to sent packets.

            Packets are first queued up in the network simulation, and then when they pop off the network simulator they are flushed to the network.

            This codepath is not called when network simulation is disabled. Packets are written and flushed directly to the network in that case.

            @param address The address the packet is being sent to.
            @param packet The packet object to be serialized (written).
            @param sequence The sequence number of the packet being written. If this is an encrypted packet, this serves as the nonce, and it is the users responsibility to increase this value with each packet sent per-encryption context. Not used for unencrypted packets (pass in zero).

            @see BaseTransport::ShouldPacketGoThroughSimulator
         */

        void WritePacketToSimulator( const Address & address, Packet * packet, uint64_t sequence );

        /**
            Write a packet and flush it to the network.

            This codepath is used when network simulation is disabled (eg. when no latency or packet loss is simulated).

            @param address The address the packet is being sent to.
            @param packet The packet object to be serialized (written).
            @param sequence The sequence number of the packet being written. If this is an encrypted packet, this serves as the nonce, and it is the users responsibility to increase this value with each packet sent per-encryption context. Not used for unencrypted packets (pass in zero).

            @see BaseTransport::InternalSendPacket
            @see BaseTransport::ShouldPacketGoThroughSimulator
         */

        void WriteAndFlushPacket( const Address & address, Packet * packet, uint64_t sequence );

        /**
            Read a packet buffer that arrived from the network and deserialize it into a newly created packet object.

            @param address The address that sent the packet.
            @param packetBuffer The byte buffer containing the packet data received from the network.
            @param packetBytes The size of the packet data being read in bytes.
            @param sequence Reference to the sequence number for the packet. If this is an encrypted packet, it will be filled with the sequence number from the encrypted packet header (eg. the nonce). If this is an unencrypted packet, it will be set to zero.

            @returns The packet object if the packet buffer was successfully read (and decrypted), NULL otherwise. The caller owns the packet pointer and is responsible for destroying it. See Packet::Destroy.
         */

        Packet * ReadPacket( const Address & address, uint8_t * packetBuffer, int packetBytes, uint64_t & sequence );

        /**
            Should sent packets go through the simulator first before they are flushed to the network?

            This is true in the case where network conditions are set on the simulator, which is how latency and packet loss is added on top of the transport.

            Returning false means packets are written and flushed directly to the network via BaseTransport::InternalSendPacket.

            @returns True if packets should be directed through the simulator (see BaseTransport::WritePacketToSimulator) and false if packets should be flushed directly to the network (see BaseTransport::WriteAndFlushPacket).

            @see Transport::SetNetworkConditions
         */

        virtual bool ShouldPacketsGoThroughSimulator();

        /**
            Internal function to send a packet over the network.

            IMPORTANT: Override this to implement your own packet send function for derived transport classes.

            @param to The address the packet should be sent to.
            @param packetData The serialized, and potentially encrypted packet data generated by BaseTransport::WritePacket.
            @param packetBytes The size of the packet data in bytes.
         */

        virtual void InternalSendPacket( const Address & to, const void * packetData, int packetBytes ) = 0;

        /**
            Internal function to receive a packet from the network.

            IMPORTANT: Override this to implement your own packet receive function for derived transport classes. 

            IMPORTANT: This call must be non-blocking.

            @param from The address that sent the packet [out].
            @param packetData The buffer to which will receive packet data read from the network.
            @param maxPacketSize The size of your packet data buffer. Packets received from the network that are larger than this are discarded.

            @returns The number of packet bytes read from the network, 0 if no packet data was read.
         */
    
        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize ) = 0;

    protected:

        TransportContext m_context;                                     ///< The default transport context used if no context can be found for the specific address.

        Address m_address;                                              ///< The address of the transport. This is the address to send packets to so that this transport would receive them.

        double m_time;                                                  ///< The current transport time. See Transport::AdvanceTime.

        uint64_t m_flags;                                               ///< The transport flags. See Transport::SetFlags.

        uint32_t m_protocolId;                                          ///< The protocol id. You set it when you create the transport and it's used to filter out any packets sent that have different protocols ids. Lets you implement basic versioning (eg. ignore packets sent by other versions of your protocol on the network).

        Allocator * m_allocator;                                        ///< The allocator passed in to the transport constructor. Used for all allocations inside the transport class, except for packets, messages and stream allocations, which are determined by the factories and allocators passed in to the context.

        PacketProcessor * m_packetProcessor;                            ///< The packet processor. This is the engine for reading and writing packets, as well as packet encryption and decryption.

        struct PacketEntry
        {
            PacketEntry()
            {
                sequence = 0;
                packet = NULL;
            }

            uint64_t sequence;                                          ///< The sequence number of the packet. Always 0 if the packet is not encrypted.
            Address address;                                            ///< The address of the packet. Depending on the queue, this is the address the packet should be sent to, or the address that sent the packet.
            Packet * packet;                                            ///< The packet object. While the packet in a queue, it is owned by that queue, and the queue is responsible for destroying that packet, or handing ownership off to somebody else (eg. after ReceivePacket).
        };

        Queue<PacketEntry> m_sendQueue;                                 ///< The packet send queue. Packets sent via Transport::SendPacket are added to this queue (unless the immediate flag is true). This queue is flushed to the network each time Transport::WritePackets is called.

        Queue<PacketEntry> m_receiveQueue;                              ///< The packet receive queue. As packets are read from the network in Transport::ReadPackets they are added to this queue. Packets are popped off the receive queue by Transport::ReadPacket.

#if !YOJIMBO_SECURE_MODE
        uint8_t * m_allPacketTypes;                                     ///< An array with each entry for valid packet types from the PacketFactory set to 1.
#endif // #if !YOJIMBO_SECURE_MODE
        
        uint8_t * m_packetTypeIsEncrypted;                              ///< An array with each entry set to 1 if that packet type is encrypted. See Transport::EnablePacketEncryption.

        uint8_t * m_packetTypeIsUnencrypted;                            ///< An array with each entry set to 1 if that packet type is NOT encrypted.

        EncryptionManager * m_encryptionManager;                        ///< The encryption manager. Manages encryption contexts and lets the transport look up send and receive keys for encrypting and decrypting packets.

        TransportContextManager * m_contextManager;                     ///< The context manager. Manages the set of contexts on the transport, which allows certain addresses to be assigned to their own set of resources (like packet factories, message factories and allocators).

        bool m_allocateNetworkSimulator;                                ///< True if the network simulator was allocated in the constructor, and must be freed in the destructor.

        class NetworkSimulator * m_networkSimulator;                    ///< The network simulator. May be NULL.

        uint64_t m_counters[TRANSPORT_COUNTER_NUM_COUNTERS];            ///< The array of transport counters. Used for stats, debugging and telemetry.
    };

    /**
        Implements a local transport built on top of a network simulator.

        This transport does not provide any networking sockets, and is used in unit tests and loopback.
     */

    class LocalTransport : public BaseTransport
    {
    public:

        /**
            Local transport constructor.

            @param allocator The allocator used for transport allocations.
            @param networkSimulator The network simulator to use. Typically, you use one network simulator shared across multiple local transports. See test.cpp and client_server.cpp for examples.
            @param address The address of the packet. This is how other other transports would send packets to this transport.
            @param protocolId The protocol id for this transport. Protocol id is included in the packet header, packets received with a different protocol id are discarded. This allows multiple versions of your protocol to exist on the same network.
            @param time The current time value in seconds.
            @param maxPacketSize The maximum packet size that can be sent across this transport.
            @param sendQueueSize The size of the packet send queue (number of packets).
            @param receiveQueueSize The size of the packet receive queue (number of packets).
         */

        LocalTransport( Allocator & allocator,
                        class NetworkSimulator & networkSimulator,
                        const Address & address,
                        uint32_t protocolId,
                        double time,
                        int maxPacketSize = DefaultMaxPacketSize,
                        int sendQueueSize = DefaultPacketSendQueueSize,
                        int receiveQueueSize = DefaultPacketReceiveQueueSize );

        ~LocalTransport();

        /// The local transport guarantees that any packets sent to this transport prior to a call to reset will not cross this boundary.

        void Reset();

        /// As an optimization, the local transport receives packets from the simulator inside this function. This avoids O(n^2) performance, where n is the maximum number of packets that can be stored in the simulator.

        void AdvanceTime( double time );

    protected:

        void DiscardReceivePackets();

        void PumpReceivePacketsFromSimulator();

        void InternalSendPacket( const Address & to, const void * packetData, int packetBytes );
    
        int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        int m_receivePacketIndex;                           ///< Current index into receive packet (for InternalReceivePacket)

        int m_numReceivePackets;                            ///< Number of receive packets from last AdvanceTime update
        
        int m_maxReceivePackets;                            ///< Size of receive packet buffer (matches size of packet receive queue)
        
        uint8_t ** m_receivePacketData;                     ///< Array of packet data to be received. pointers are owned and must be freed with simulator allocator.
        
        int * m_receivePacketBytes;                         ///< Array of packet sizes in bytes.
        
        Address * m_receiveFrom;                            ///< Array of packet from addresses.
    };

#if YOJIMBO_SOCKETS

    /**
        Implements a network transport built on top of non-blocking sendto and recvfrom socket APIs.
     */

    class NetworkTransport : public BaseTransport
    {
    public:

        /**
            Network transport constructor.

            @param allocator The allocator used for transport allocations.
            @param address The address to send packets to that would be received by this transport.
            @param protocolId The protocol id for this transport. Protocol id is included in the packet header, packets received with a different protocol id are discarded. This allows multiple versions of your protocol to exist on the same network.
            @param time The current time value in seconds.
            @param maxPacketSize The maximum packet size that can be sent across this transport.
            @param sendQueueSize The size of the packet send queue (number of packets).
            @param receiveQueueSize The size of the packet receive queue (number of packets).
            @param socketSendBufferSize The size of the send buffers to set on the socket (SO_SNDBUF).
            @param socketReceiveBufferSize The size of the send buffers to set on the socket (SO_RCVBUF).
         */

        NetworkTransport( Allocator & allocator,
                          const Address & address,
                          uint32_t protocolId,
                          double time,
                          int maxPacketSize = DefaultMaxPacketSize,
                          int sendQueueSize = DefaultPacketSendQueueSize,
                          int receiveQueueSize = DefaultPacketReceiveQueueSize,
                          int socketSendBufferSize = DefaultSocketSendBufferSize,
						  int socketReceiveBufferSize = DefaultSocketReceiveBufferSize );

        ~NetworkTransport();

        /**

            You should call this after creating a network transport, to make sure the socket was created successfully.

            @returns True if the socket is in error state.

            @see NetworkTransport::GetError
         */

        bool IsError() const;

        /** 
            Get the socket error code. 

            @returns The socket error code. One of the values in yojimbo::SocketError enum.

            @see yojimbo::SocketError
            @see NetworkTransport::IsError
         */

        int GetError() const;

    protected:

        /// Overridden internal packet send function. Effectively just calls through to sendto.

        virtual void InternalSendPacket( const Address & to, const void * packetData, int packetBytes );
    
        /// Overriden internal packet receive function. Effectively just wraps recvfrom.

        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        class Socket * m_socket;                                ///< The socket used for sending and receiving UDP packets.
    };

#endif // #if YOJIMBO_SOCKETS
}

#endif // #ifndef YOJIMBO_INTERFACE_H
