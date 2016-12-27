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

#ifndef YOJIMBO_PACKET_H
#define YOJIMBO_PACKET_H

#include "yojimbo_config.h"
#include "yojimbo_common.h"
#include "yojimbo_bitpack.h"
#include "yojimbo_stream.h"
#include "yojimbo_serialize.h"

#if YOJIMBO_DEBUG_PACKET_LEAKS
#include <map>
#endif // #if YOJIMBO_DEBUG_PACKET_LEAKS

namespace yojimbo
{
    class PacketFactory;

    /**
        A packet that can be serialized to a bit stream.
     */

    class Packet : public Serializable
    {
    public:        
        
        Packet() : m_packetFactory( NULL ), m_type( 0 ) {}

        /**
            Destroy the packet.

            Unlike messages, packets are not reference counted. Call this method to destroy the packet. It takes care to call through to the message factory that created the packet for you.

            IMPORTANT: If the packet lives longer than the packet that created it, the packet factory will assert that you have a packet leak its destructor, as a safety feature. Make sure you destroy all packets before cleaning up the packet factory!
         */

        void Destroy();

        /**
            Checks if the packet is valid. 

            Packet type is clearedh to -1 when a packet is destroyed, to aid with tracking down pointers to already deleted packets.

            @returns True if the packet is valid, false otherwise.
         */

        bool IsValid() const { return m_type >= 0; }

        /**
            Get the type of the packet.

            Corresponds to the type value passed in to the packet factory when this packet object was created.

            @returns The packet type in [0,numTypes-1] as defined by the packet factory.

            @see PacketFactory::Create
         */

        int GetType() const { return m_type; }

        /**
            Get the packet factory that was used to create this packet.

            IMPORTANT: The packet factory must remain valid while packets created with it still exist. Make sure you destroy all packets before destroying the message factory that created them.

            @returns The packet factory.
         */

        PacketFactory & GetPacketFactory() { return *m_packetFactory; }

    protected:

        friend class PacketFactory;

        /**
            Set the type of the packet factory.

            Used internally by the packet factory to set the packet type on the packet object on creation.

            Protected because a bunch of stuff would break if the user were to change the packet type dynamically.

            @param type The packet type.
         */

        void SetType( int type) { m_type = type; }

        /**
            Set the packet factory.

            Used internally by the packet factory to set its own pointer on packets it creates, so those packets remember this and call back to the packet factory that created them when they are destroyed.

            Protected because everything would break if the user were to modify the packet factory on an object after creation.

            @param packetFactory The packet factory that created this packet.
         */

        void SetPacketFactory( PacketFactory & packetFactory ) { m_packetFactory = &packetFactory; }

        /**
            Packet destructor.

            Protected because you need to call in to Packet::Destroy to destroy this packet, instead of just deleting it, to make sure it gets cleaned up with the packet factory and allocator that created it.
         */

        virtual ~Packet() { m_packetFactory = NULL; m_type = -1; }

    private:

        PacketFactory * m_packetFactory;                                    ///< The factory that was used to create this packet. Used by Packet::Destroy to ensure that the packet is cleaned up by the factory that created it.

        int m_type;                                                         ///< The packet type, as defined by the packet factory.

        Packet( const Packet & other );

        Packet & operator = ( const Packet & other );
    };

    /**
        The packet factory error level.

        Any error level set other than PACKET_FACTORY_ERROR_NONE results in the connection being torn down and the client being disconnected.

        @see Connection
        @see Client
        @see Server
     */

    enum PacketFactoryError
    {
        PACKET_FACTORY_ERROR_NONE,                                          ///< No error. All is well.
        PACKET_FACTORY_ERROR_FAILED_TO_ALLOCATE_PACKET,                     ///< Tried to allocate a packet but failed. The allocator backing the packet factory is probably out of memory.
    };

    /**
        Defines the set of packet types and a function to create packets.

        Packets are not reference counted. They are typically added to send/receive queues, dequeued, processed and then destroyed.

        @see Packet::Destroy
     */

    class PacketFactory
    {        
    public:

        /**
            The packet factory constructor.

            @param allocator The allocator used to create packets.
            @param numTypes The number of packet types that can be created with this factory.
         */

        PacketFactory( class Allocator & allocator, int numTypes );

        /**
            Packet factory destructor.

            IMPORTANT: You must destroy all packets created by this factory before you destroy it. 

            As a safety check, in debug builds the packet factory track packets created and will assert if you don't destroy them all before destroying the factory.
         */

        virtual ~PacketFactory();

        /**
            Create a packet by type.

            IMPORTANT: Check the packet pointer returned by this call. It can be NULL if there is no memory to create a packet!

            @param type The type of packet to create in [0,numTypes-1].

            @returns The packet object created. NULL if packet could not be created. You are responsible for destroying non-NULL packets via Packet::Destroy, or to pass ownership of the packte to some other function like Transport::SendPacket.
         */

        Packet * Create( int type );

        /**
            Get the number of packet types that can be created with this factory.

            Packet types that can be created are in [0,numTypes-1].

            @returns The number of packet types.
         */

        int GetNumPacketTypes() const;

        /**
            Get the error level of the packet factory.

            If any packet fails to allocate, the error level is set to yojimbo::PACKET_FACTORY_ERROR_FAILED_TO_ALLOCATE_PACKET.

            @see PacketFactory::ClearError
		 */

        PacketFactoryError GetError() const;

        /**
            Clears the error level back to none.
         */

        void ClearError();

    protected:

        friend class Packet;

        /**
            Internal method to destroy a packet called by Packet::Destroy.

            This is done so packets can be destroyed unilaterally, without the user needing to remember which packet factory they need to be destroyed with.

            This is important because the server has one global packet factory (for connection negotiation packets) and one packet factory per-client for security reasons. 

            It would be too much of a burden and too error prone to require the user to look-up the packet factory by client index for packets belonging to client connections.

            @param packet The packet to destroy.
         */

        void DestroyPacket( Packet * packet );

        /**
            Set the packet type on a packet.

            Called by the packet factory to set the type of packets it has just created in PacketFactory::Create.

            @param packet The packet to set the type on.
            @param type The packet type to be set.
         */

        void SetPacketType( Packet * packet, int type );

        /**
            Set the packet factory on a packet.

            Called by packet factory to set the packet factory on packets it has created in PacketFactory::Create, so those packets know how to destroy themselves.

            @param packet The packet to set the this packet factory on.
         */

        void SetPacketFactory( Packet * packet );

        /**
            Get the allocator used to create packets.

            @returns The allocator.
         */

        Allocator & GetAllocator();

    protected:

        /**
            Internal function used to create packets.

            This is typically overridden using helper macros instead of doing it manually.

            See:

                YOJIMBO_PACKET_FACTORY_START
                YOJIMBO_DECLARE_PACKET_TYPE
                YOJIMBO_PACKET_FACTORY_FINISH

            See tests/shared.h for an example of usage.

            @param type The type of packet to create.

            @returns The packet created, or NULL if no packet could be created (eg. the allocator is out of memory).
         */

        virtual Packet * CreatePacket( int type ) { (void) type; return NULL; }

    private:

#if YOJIMBO_DEBUG_PACKET_LEAKS
        std::map<void*,int> allocated_packets;                                          ///< Tracks packets created by this packet factory. Used in debug builds to check that all packets created by the factory are destroyed before the packet factory is destroyed.
#endif // #if YOJIMBO_DEBUG_PACKET_LEAKS

        Allocator * m_allocator;                                                        ///< The allocator used to create and destroy packet objects.

        PacketFactoryError m_error;                                                     ///< The error level. Used to track failed packet allocations and take action.

        int m_numPacketTypes;                                                           ///< The number of packet types that can be created with this factory. Valid packet types are in range [0,m_numPacketTypes-1].
        
        PacketFactory( const PacketFactory & other );
        
        PacketFactory & operator = ( const PacketFactory & other );
    };

    /**
        Information passed into low-level functions to read and write packets.
     */

    struct PacketReadWriteInfo
    {
        bool rawFormat;                                                                 ///< If true then packets are written in "raw" format without crc32 (useful for encrypted packets which have packet signature elsewhere).

        int prefixBytes;                                                                ///< Prefix this number of bytes when reading and writing packets. Used for the variable length sequence number at the start of encrypted packets.

        uint32_t protocolId;                                                            ///< Protocol id that distinguishes your protocol from other packets sent over UDP.

        Allocator * streamAllocator;                                                    ///< This allocator is passed in to the stream and used for dynamic allocations while reading and writing packets.

        PacketFactory * packetFactory;                                                  ///< Packet factory defines the set of packets that can be read. Also called to create packet objects when a packet is read from the network.

        const uint8_t * allowedPacketTypes;                                             ///< Array of allowed packet types. One entry per-packet type in [0,numPacketTypes-1] as defined by the packet factory. If a packet type is not allowed then serialization of that packet will fail. Allows the caller to disable certain packet types dynamically.

        void * context;                                                                 ///< Context for packet serialization. Optional. Pass in NULL if not using it. Set on the stream and accessible during packet serialization via ReadStream::GetContext, WriteStream::GetContext, etc.

        void * userContext;                                                             ///< User context for packet serialization. Optional. Pass in NULL if not using it. Set on the stream and accessible during packet serialization via ReadStream::GetUserContext, WriteStream::GetUserContext, etc.

        PacketReadWriteInfo()
        {
            rawFormat = false;
            prefixBytes = 0;
            protocolId = 0;
            packetFactory = NULL;
            streamAllocator = NULL;
            allowedPacketTypes = NULL;
            context = NULL;
            userContext = NULL;
        }
    };

    /**
        Low-level function to write a packet to a byte buffer.

        The packet is written to the byte buffer in wire format, with CRC32 prefixed to the packet, unless the packet is configured to write in raw format without CRC32.

        Packet encryption is done elsewhere. See PacketProcessor for details.

        @param info Describes how to write the packet.
        @param packet The packet object to write to the buffer.
        @param buffer The buffer to write to.
        @param bufferSize The size of the buffer to write to (bytes). This effectively sets the maximum packet size that can be written.

        @returns The number of bytes written to the buffer.

        @see yojimbo::ReadPacket
     */

    int WritePacket( const PacketReadWriteInfo & info, Packet * packet, uint8_t * buffer, int bufferSize );

    /**
        Error codes for read packet function.

        @see yojimbo::ReadPacket
     */

    enum ReadPacketError
    {
        READ_PACKET_ERROR_NONE,                                     ///< Packet read OK.
        READ_PACKET_ERROR_CRC32_MISMATCH,                           ///< Packet CRC32 check failed.
        READ_PACKET_ERROR_CREATE_PACKET_FAILED,                     ///< Tried to create a packet but failed. The allocator backing the packet factory is probably out of memory.
        READ_PACKET_ERROR_PACKET_TYPE_NOT_ALLOWED,                  ///< Packet type is not one we are allowed to read. See PacketReadWriteInfo::allowedPacketTypes.
        READ_PACKET_ERROR_SERIALIZE_PACKET_HEADER,                  ///< Failed to serialize the packet header. One of the packet header elements returned false when serialized.
        READ_PACKET_ERROR_SERIALIZE_PACKET_BODY,	                ///< Failed to serialize the packet body. The packet serialize read function returned false.
        READ_PACKET_ERROR_SERIALIZE_PACKET_FOOTER,					///< Failed to serialize the packet footer. The serialize check at the end of the packet failed. The packet data is probably truncated.
    };

    /**
        Read a packet from a byte buffer.

        Lots of checking is performed. If the packet fails to read for some reason, this function returns NULL. Check the errorCode parameter for the reason why.

        @param info Describing how to read the packet. Should be the same data as when the packet was written, otherwise the packet serialize can desync.
        @param buffer The packet data read from the network (eg. from recvfrom).
        @param bufferSize The number of bytes of packet data to read.
        @param errorCode The error code describing the reason the packet failed to serialize.

        @see yojimbo::WritePacket
     */

    Packet * ReadPacket( const PacketReadWriteInfo & info, const uint8_t * buffer, int bufferSize, ReadPacketError * errorCode = NULL );
}

// todo: can I document macros? That would be really helpful. Help me doxygen.

#define YOJIMBO_PACKET_FACTORY_START( factory_class, base_factory_class, num_packet_types )                                         \
                                                                                                                                    \
    class factory_class : public base_factory_class                                                                                 \
    {                                                                                                                               \
    public:                                                                                                                         \
        factory_class( yojimbo::Allocator & allocator = yojimbo::GetDefaultAllocator(), int numPacketTypes = num_packet_types )     \
         : base_factory_class( allocator, numPacketTypes ) {}                                                                       \
        yojimbo::Packet * CreatePacket( int type )																					\
        {                                                                                                                           \
            yojimbo::Packet * packet = base_factory_class::CreatePacket( type );													\
            if ( packet )                                                                                                           \
                return packet;                                                                                                      \
            yojimbo::Allocator & allocator = GetAllocator();                                                                        \
            (void)allocator;                                                                                                        \
            switch ( type )                                                                                                         \
            {                                                                                                       


#define YOJIMBO_DECLARE_PACKET_TYPE( packet_type, packet_class )                                                    \
                                                                                                                    \
                case packet_type:                                                                                   \
                    packet = YOJIMBO_NEW( allocator, packet_class );                                                \
                    if ( !packet )                                                                                  \
                        return NULL;                                                                                \
                    SetPacketType( packet, packet_type );                                                           \
                    SetPacketFactory( packet );                                                                     \
                    return packet;        

#define YOJIMBO_PACKET_FACTORY_FINISH()                                                                             \
                                                                                                                    \
                default: return NULL;                                                                               \
            }                                                                                                       \
        }                                                                                                           \
    };

#endif // #ifndef YOJIMBO_PACKET_H
