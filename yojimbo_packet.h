/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#ifndef YOJIMBO_PACKET_H
#define YOJIMBO_PACKET_H

#include "yojimbo_config.h"
#include "yojimbo_common.h"
#include "yojimbo_bitpack.h"
#include "yojimbo_stream.h"
#include "yojimbo_serialize.h"

namespace yojimbo
{
    class Packet : public Serializable
    {
    public:
        
        Packet( int _type ) : type(_type) {}

        virtual ~Packet() {}

        int GetType() const { return type; }

    private:

        int type;

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

        PacketFactory( int numTypes );

        ~PacketFactory();

        Packet * CreatePacket( int type );

        void DestroyPacket( Packet * packet );

        int GetNumPacketTypes() const;

    protected:

        virtual Packet * Create( int type ) = 0;

        virtual void Destroy( Packet * packet ) = 0;

    private:

        int m_numPacketTypes;
        int m_numAllocatedPackets;  
#if YOJIMBO_DEBUG_PACKET_LEAKS
        std::map<void*,int> allocated_packets;
#endif // #if YOJIMBO_DEBUG_PACKET_LEAKS
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
