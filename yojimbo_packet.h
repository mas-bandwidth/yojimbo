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
    class Packet : public Serializable
    {
    public:        
        
#if YOJIMBO_PACKET_MAGIC

        explicit Packet( int type ) : m_type( type ), m_magic( 0 ) {}

		virtual ~Packet() { m_type = -1; m_magic = 0; }

		uint64_t GetMagic() const { return m_magic; }
    
        bool IsValid() const { return m_magic != 0 && m_type >= 0; }

#else // #if YOJIMBO_PACKET_MAGIC

        explicit Packet( int type ) : m_type( type ) {}

		virtual ~Packet() { m_type = -1; }

        bool IsValid() const { return m_type >= 0; }

#endif // #if YOJIMBO_PACKET_MAGIC

        int GetType() const { return m_type; }

    protected:

        friend class PacketFactory;

        void SetType( int type) { m_type = type; }

        int m_type;

#if YOJIMBO_PACKET_MAGIC
        uint64_t m_magic;
#endif // #if YOJIMBO_PACKET_MAGIC

        Packet( const Packet & other );
        Packet & operator = ( const Packet & other );
    };

    class PacketHeader : public Serializable
    {
        // ...
    };

    class PacketFactory
    {        
    public:

        PacketFactory( class Allocator & allocator, int numTypes );

        ~PacketFactory();

        Packet * CreatePacket( int type );

        void DestroyPacket( Packet * packet );

        int GetNumPacketTypes() const;

#if YOJIMBO_PACKET_MAGIC
        uint64_t GetMagic() const { return m_magic; }
#endif // #if YOJIMBO_PACKET_MAGIC

    protected:

        void OverridePacketType( Packet * packet, int type );

        Allocator & GetAllocator();

    protected:

        virtual Packet * CreateInternal( int type ) = 0;

        virtual void DestroyInternal( Packet * packet ) = 0;

    private:

#if YOJIMBO_PACKET_MAGIC
        uint64_t m_magic;
#endif // #if DEBUG

#if YOJIMBO_DEBUG_PACKET_LEAKS
        std::map<void*,int> allocated_packets;
#endif // #if YOJIMBO_DEBUG_PACKET_LEAKS

        Allocator * m_allocator;

        int m_numPacketTypes;
        int m_numAllocatedPackets;  
    };

    struct PacketInfo
    {
        bool rawFormat;                             // if true packets are written in "raw" format without crc32 (useful for encrypted packets).

        int prefixBytes;                            // prefix this number of bytes when reading and writing packets. stick your own data there.

        uint32_t protocolId;                        // protocol id that distinguishes your protocol from other packets sent over UDP.

        PacketFactory * packetFactory;              // create packets and determine information about packet types. required.

        const uint8_t * allowedPacketTypes;         // array of allowed packet types. if a packet type is not allowed the serialize read or write will fail.

        void * context;                             // context for the packet serialization (optional, pass in NULL)

        PacketInfo()
        {
            rawFormat = false;
            prefixBytes = 0;
            protocolId = 0;
            packetFactory = NULL;
            allowedPacketTypes = NULL;
            context = NULL;
        }
    };

    int WritePacket( const PacketInfo & info, Packet * packet, uint8_t * buffer, int bufferSize, PacketHeader * header = NULL );

    Packet * ReadPacket( const PacketInfo & info, const uint8_t * buffer, int bufferSize, PacketHeader * header = NULL, int * errorCode = NULL );

#if YOJIMBO_PACKET_AGGREGATION

    int WriteAggregatePacket( const PacketInfo & info, 
                              int numPackets, 
                              Packet ** packets, 
                              uint8_t * buffer, 
                              int bufferSize, 
                              int & numPacketsWritten, 
                              PacketHeader * aggregatePacketHeader = NULL, 
                              PacketHeader ** packetHeaders = NULL );

    void ReadAggregatePacket( const PacketInfo & info, 
                              int maxPacketsToRead, 
                              Packet ** packets, 
                              const uint8_t * buffer, 
                              int bufferSize, 
                              int & numPacketsRead, 
                              PacketHeader * aggregatePacketHeader = NULL, 
                              PacketHeader ** packetHeaders = NULL, 
                              int * errorCode = NULL );

#endif // #if YOJIMBO_PACKET_AGGREGATION
}

#endif // #ifndef YOJIMBO_PACKET_H
