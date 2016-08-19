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

    class Packet : public Serializable
    {
    public:        
        
        Packet() : m_packetFactory( NULL ), m_type( 0 ) {}

        void Destroy();

        bool IsValid() const { return m_type >= 0; }

        int GetType() const { return m_type; }

        PacketFactory & GetPacketFactory() { return *m_packetFactory; }

    protected:

        friend class PacketFactory;

        void SetType( int type) { m_type = type; }

        void SetPacketFactory( PacketFactory & packetFactory ) { m_packetFactory = &packetFactory; }

        virtual ~Packet() { m_packetFactory = NULL; m_type = -1; }

    private:

        PacketFactory * m_packetFactory;

        int m_type;

        Packet( const Packet & other );

        Packet & operator = ( const Packet & other );
    };

    enum PacketFactoryError
    {
        PACKET_FACTORY_ERROR_NONE,
        PACKET_FACTORY_ERROR_FAILED_TO_ALLOCATE_PACKET,
        PACKET_FACTORY_ERROR_ALLOCATOR_IS_EXHAUSTED
    };

    class PacketFactory
    {        
    public:

        PacketFactory( class Allocator & allocator, int numTypes );

        ~PacketFactory();

        Packet * CreatePacket( int type );

        int GetNumPacketTypes() const;

        int GetError() const;

        void ClearError();

    protected:

        friend class Packet;

        void DestroyPacket( Packet * packet );

        void SetPacketType( Packet * packet, int type );

        void SetPacketFactory( Packet * packet );

        Allocator & GetAllocator();

    protected:

        virtual Packet * CreateInternal( int /*type*/ ) { return NULL; }

    private:

#if YOJIMBO_DEBUG_PACKET_LEAKS
        std::map<void*,int> allocated_packets;
#endif // #if YOJIMBO_DEBUG_PACKET_LEAKS

        Allocator * m_allocator;

        int m_error;
        int m_numPacketTypes;
        int m_numAllocatedPackets;

        PacketFactory( const PacketFactory & other );
        
        PacketFactory & operator = ( const PacketFactory & other );
    };

    struct PacketReadWriteInfo
    {
        bool rawFormat;                             // if true packets are written in "raw" format without crc32 (useful for encrypted packets).

        int prefixBytes;                            // prefix this number of bytes when reading and writing packets. stick your own data there.

        uint32_t protocolId;                        // protocol id that distinguishes your protocol from other packets sent over UDP.

        Allocator * streamAllocator;                // this allocator is passed in to the stream and used for dynamic allocations, typically in serialize read. required.

        PacketFactory * packetFactory;              // create packets and determine information about packet types. required.

        const uint8_t * allowedPacketTypes;         // array of allowed packet types. if a packet type is not allowed the serialize read or write will fail.

        void * context;                             // context for the packet serialization (optional, you may pass in NULL)

        PacketReadWriteInfo()
        {
            rawFormat = false;
            prefixBytes = 0;
            protocolId = 0;
            packetFactory = NULL;
            streamAllocator = NULL;
            allowedPacketTypes = NULL;
            context = NULL;
        }
    };

    int WritePacket( const PacketReadWriteInfo & info, Packet * packet, uint8_t * buffer, int bufferSize );

    Packet * ReadPacket( const PacketReadWriteInfo & info, const uint8_t * buffer, int bufferSize, int * errorCode = NULL );
}

#define YOJIMBO_PACKET_FACTORY_START( factory_class, base_factory_class, num_packet_types )                                         \
                                                                                                                                    \
    class factory_class : public base_factory_class                                                                                 \
    {                                                                                                                               \
    public:                                                                                                                         \
        factory_class( yojimbo::Allocator & allocator = yojimbo::GetDefaultAllocator(), int numPacketTypes = num_packet_types )     \
         : base_factory_class( allocator, numPacketTypes ) {}                                                                       \
        yojimbo::Packet * CreateInternal( int type )                                                                                \
        {                                                                                                                           \
            yojimbo::Packet * packet = base_factory_class::CreateInternal( type );                                                  \
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
