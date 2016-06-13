/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#include "yojimbo_packet.h"

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <alloca.h>
#endif

namespace yojimbo
{
    int WritePacket( const PacketInfo & info, Packet * packet, uint8_t * buffer, int bufferSize, PacketHeader * header )
    {
        assert( packet );
        assert( buffer );
        assert( bufferSize > 0 );
        assert( info.protocolId );
        assert( info.packetFactory );

        const int numPacketTypes = info.packetFactory->GetNumPacketTypes();

        WriteStream stream( buffer, bufferSize );

        stream.SetContext( info.context );

        for ( int i = 0; i < info.prefixBytes; ++i )
        {
            uint8_t zero = 0;
            stream.SerializeBits( zero, 8 );
        }

        uint32_t crc32 = 0;

        if ( !info.rawFormat )
            stream.SerializeBits( crc32, 32 );

        if ( header )
        {
            if ( !header->SerializeWrite( stream ) )
                return 0;
        }

        int packetType = packet->GetType();

        assert( numPacketTypes > 0 );

        if ( numPacketTypes > 1 )
        {
            stream.SerializeInteger( packetType, 0, numPacketTypes - 1 );
        }

        if ( !packet->SerializeWrite( stream ) )
            return 0;

        stream.SerializeCheck( "end of packet" );

        stream.Flush();

        if ( !info.rawFormat )
        {
            uint32_t network_protocolId = host_to_network( info.protocolId );
            crc32 = calculate_crc32( (uint8_t*) &network_protocolId, 4 );
            crc32 = calculate_crc32( buffer + info.prefixBytes, stream.GetBytesProcessed() - info.prefixBytes, crc32 );
            *((uint32_t*)(buffer+info.prefixBytes)) = host_to_network( crc32 );
        }

        if ( stream.GetError() )
            return 0;

        return stream.GetBytesProcessed();
    }

    Packet * ReadPacket( const PacketInfo & info, const uint8_t * buffer, int bufferSize, PacketHeader * header, int * errorCode )
    {
        assert( buffer );
        assert( bufferSize > 0 );
        assert( info.protocolId != 0 );
        assert( info.packetFactory );

        if ( errorCode )
            *errorCode = YOJIMBO_PROTOCOL_ERROR_NONE;

        ReadStream stream( buffer, bufferSize );

        stream.SetContext( info.context );

        for ( int i = 0; i < info.prefixBytes; ++i )
        {
            uint32_t dummy = 0;
            stream.SerializeBits( dummy, 8 );
        }

        uint32_t read_crc32 = 0;
        if ( !info.rawFormat )
        {
            stream.SerializeBits( read_crc32, 32 );

            uint32_t network_protocolId = host_to_network( info.protocolId );
            uint32_t crc32 = calculate_crc32( (const uint8_t*) &network_protocolId, 4 );
            uint32_t zero = 0;
            crc32 = calculate_crc32( (const uint8_t*) &zero, 4, crc32 );
            crc32 = calculate_crc32( buffer + info.prefixBytes + 4, bufferSize - 4 - info.prefixBytes, crc32 );

            if ( crc32 != read_crc32 )
            {
#ifdef DEBUG
                printf( "corrupt packet. expected crc32 %x, got %x\n", crc32, read_crc32 );
#endif // #ifdef DEBUG
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_CRC32_MISMATCH;
                return NULL;
            }
        }

        if ( header )
        {
            if ( !header->SerializeRead( stream ) )
            {
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_SERIALIZE_HEADER_FAILED;
                return NULL;
            }
        }

        int packetType = 0;

        const int numPacketTypes = info.packetFactory->GetNumPacketTypes();

        assert( numPacketTypes > 0 );

        if ( numPacketTypes > 1 )
        {
            if ( !stream.SerializeInteger( packetType, 0, numPacketTypes - 1 ) )
            {
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_INVALID_PACKET_TYPE;
                return NULL;
            }
        }

        if ( info.allowedPacketTypes )
        {
            if ( !info.allowedPacketTypes[packetType] )
            {
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_PACKET_TYPE_NOT_ALLOWED;
                return NULL;
            }
        }

        Packet *packet = info.packetFactory->CreatePacket( packetType );
        if ( !packet )
        {
            if ( errorCode )
                *errorCode = YOJIMBO_PROTOCOL_ERROR_CREATE_PACKET_FAILED;
            return NULL;
        }

        if ( !packet->SerializeRead( stream ) )
        {
            if ( errorCode )
                *errorCode = YOJIMBO_PROTOCOL_ERROR_SERIALIZE_PACKET_FAILED;
            goto cleanup;
        }

#if YOJIMBO_SERIALIZE_CHECKS
        if ( !stream.SerializeCheck( "end of packet" ) )
        {
            if ( errorCode )
                *errorCode = YOJIMBO_PROTOCOL_ERROR_SERIALIZE_CHECK_FAILED;
            goto cleanup;
        }
#endif // #if YOJIMBO_SERIALIZE_CHECKS

        if ( stream.GetError() )
        {
            if ( errorCode )
                *errorCode = stream.GetError();
            goto cleanup;
        }

        return packet;

cleanup:
        info.packetFactory->DestroyPacket( packet );
        return NULL;
    }

#if YOJIMBO_PACKET_AGGREGATION

    int WriteAggregatePacket( const PacketInfo & info, 
                              int numPackets, 
                              Packet **packets, 
                              uint8_t *buffer, 
                              int bufferSize, 
                              int & numPacketsWritten, 
                              PacketHeader * aggregatePacketHeader, 
                              PacketHeader ** packetHeaders )
    {
        assert( numPackets >= 0 );
        assert( packets );
        assert( buffer );
        assert( bufferSize > 0 );
        assert( info.protocolId != 0 );
        assert( info.packetFactory );

        const int numPacketTypes = info.packetFactory->GetNumPacketTypes();

        assert( numPacketTypes > 0 );
        assert( numPacketTypes + 1 <= 65535 );

        const int packetTypeBytes = ( numPacketTypes > 255 ) ? 2 : 1;

        numPacketsWritten = 0;

        int aggregatePacketBytes = 0;

        if ( info.prefixBytes > 0 )
        {
            memset( buffer, 0, info.prefixBytes );
            aggregatePacketBytes += info.prefixBytes;
        }

        if ( !info.rawFormat )
        {
            // reserve space for crc32
            memset( buffer + aggregatePacketBytes, 0, 4 );
            aggregatePacketBytes += 4;
        }

        // write the optional aggregate packet header

        if ( aggregatePacketHeader )
        {
            uint8_t *scratch = (uint8_t*) alloca( bufferSize );

            typedef WriteStream Stream;

            Stream stream( scratch, bufferSize );

            stream.SetContext( info.context );

            if ( !aggregatePacketHeader->SerializeWrite( stream ) )
                return 0;

            stream.SerializeCheck( "aggregate packet header" );

            stream.SerializeAlign();

            stream.Flush();

            if ( stream.GetError() )
                return 0;

            int packetSize = stream.GetBytesProcessed();

            memcpy( buffer + aggregatePacketBytes, scratch, packetSize );

            aggregatePacketBytes += packetSize;
        }

        // write packet type, packet header (optional) and packet data for each packet passed in

        for ( int i = 0; i < numPackets; ++i )
        {
            uint8_t *scratch = (uint8_t*) alloca( bufferSize );

            typedef WriteStream Stream;

            Stream stream( scratch, bufferSize );

            Packet *packet = packets[i];

            int packetTypePlusOne = packet->GetType() + 1;

            assert( numPacketTypes > 0 );

            assert( stream.GetAlignBits() == 0 );       // must be byte aligned at this point

            stream.SerializeInteger( packetTypePlusOne, 0, numPacketTypes );

            if ( packetHeaders )
            {
                assert( packetHeaders[i] );

                if ( !packetHeaders[i]->SerializeWrite( stream ) )
                    return 0;
            }

            if ( !packet->SerializeWrite( stream ) )
                return 0;

            stream.SerializeCheck( "end of packet" );

            stream.SerializeAlign();

            stream.Flush();

            if ( stream.GetError() )
                return 0;

            int packetSize = stream.GetBytesProcessed();

            if ( aggregatePacketBytes + packetSize >= bufferSize + packetTypeBytes )
                break;

            memcpy( buffer + aggregatePacketBytes, scratch, packetSize );

            aggregatePacketBytes += packetSize;

            numPacketsWritten++;
        }

        // write END marker packet type (0)

        memset( buffer + aggregatePacketBytes, 0, packetTypeBytes );

        aggregatePacketBytes += packetTypeBytes;

        assert( aggregatePacketBytes > 0 );
        assert( aggregatePacketBytes <= bufferSize );

        if ( !info.rawFormat )
        {
            // calculate header crc32 for aggregate packet as a whole

            uint32_t network_protocolId = host_to_network( info.protocolId );
            uint32_t crc32 = calculate_crc32( (uint8_t*) &network_protocolId, 4 );
            crc32 = calculate_crc32( buffer, aggregatePacketBytes, crc32 );

            *((uint32_t*)(buffer+info.prefixBytes)) = host_to_network( crc32 );
        }

        return aggregatePacketBytes;
    }

    void ReadAggregatePacket( const PacketInfo & info,
                              int maxPacketsToRead, 
                              Packet ** packets, 
                              const uint8_t * buffer, 
                              int bufferSize, 
                              int & numPacketsRead, 
                              PacketHeader * aggregatePacketHeader, 
                              PacketHeader ** packetHeaders, 
                              int * errorCode )
    {
        assert( info.protocolId );
        assert( info.packetFactory );

        numPacketsRead = 0;

        for ( int i = 0; i < maxPacketsToRead; ++i )
            packets[i] = NULL;

        typedef ReadStream Stream;

        Stream stream( buffer, bufferSize );

        stream.SetContext( info.context );

        for ( int i = 0; i < info.prefixBytes; ++i )
        {
            uint32_t dummy = 0;
            stream.SerializeBits( dummy, 8 );
        }

        if ( !info.rawFormat )
        {
            // verify crc32 for packet matches, otherwise discard with error

            uint32_t read_crc32 = 0;
            stream.SerializeBits( read_crc32, 32 );

            uint32_t network_protocolId = host_to_network( info.protocolId );
            uint32_t crc32 = calculate_crc32( (const uint8_t*) &network_protocolId, 4 );
            uint32_t zero = 0;
            crc32 = calculate_crc32( (const uint8_t*) &zero, 4, crc32 );
            crc32 = calculate_crc32( buffer + 4, bufferSize - 4, crc32 );

            if ( crc32 != read_crc32 )
            {
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_CRC32_MISMATCH;
                return;
            }
        }

        // read optional aggregate packet header

        if ( aggregatePacketHeader )
        {
            if ( !aggregatePacketHeader->SerializeRead( stream ) )
            {
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_SERIALIZE_HEADER_FAILED;
                return;
            }

            stream.SerializeCheck( "aggregate packet header" );

            stream.SerializeAlign();
        }

        // serialize read packets in the aggregate packet until we hit an end packet marker

        while ( numPacketsRead < maxPacketsToRead )
        {
            assert( info.packetFactory->GetNumPacketTypes() > 0 );

            assert( stream.GetAlignBits() == 0 );       // must be byte aligned at this point

            int packetTypePlusOne = 0;

            stream.SerializeInteger( packetTypePlusOne, 0, info.packetFactory->GetNumPacketTypes() );

            if ( packetTypePlusOne == 0 )               // end of packet marker
                break;

            const int packetType = packetTypePlusOne - 1;

            if ( info.allowedPacketTypes )
            {
                if ( !info.allowedPacketTypes[packetType] )
                {
                    if ( errorCode )
                        *errorCode = YOJIMBO_PROTOCOL_ERROR_PACKET_TYPE_NOT_ALLOWED;
                    goto cleanup;
                }
            }

            if ( packetHeaders )
            {
                assert( packetHeaders[numPacketsRead] );

                if ( !packetHeaders[numPacketsRead]->SerializeRead( stream ) )
                {
                    if ( errorCode )
                        *errorCode = YOJIMBO_PROTOCOL_ERROR_SERIALIZE_HEADER_FAILED;
                    goto cleanup;
                }
            }

            packets[numPacketsRead] = info.packetFactory->CreatePacket( packetType );

            if ( !packets[numPacketsRead] )
            {
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_CREATE_PACKET_FAILED;
                goto cleanup;
            }

            if ( !packets[numPacketsRead]->SerializeRead( stream ) )
            {
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_SERIALIZE_PACKET_FAILED;
                goto cleanup;
            }

            if ( !stream.SerializeCheck( "end of packet" ) )
            {
                if ( errorCode )
                    *errorCode = YOJIMBO_PROTOCOL_ERROR_SERIALIZE_CHECK_FAILED;
                goto cleanup;
            }

            stream.SerializeAlign();

            if ( stream.GetError() )
            {
                if ( errorCode )
                    *errorCode = stream.GetError();
                goto cleanup;
            }

            ++numPacketsRead;
        }

        return;

    cleanup:

        for ( int j = 0; j < numPacketsRead; ++j )
        {
            info.packetFactory->DestroyPacket( packets[j] );
            packets[j] = NULL;
        }

        numPacketsRead = 0;
    }

#endif // #if PROTOCOL2_PACKET_AGGREGATION

    PacketFactory::PacketFactory( Allocator & allocator, int numPacketTypes )
    {
        m_numPacketTypes = numPacketTypes;
        m_numAllocatedPackets = 0;
        m_allocator = &allocator;
    }

    PacketFactory::~PacketFactory()
    {
#if PROTOCOL2_DEBUG_PACKET_LEAKS
        if ( allocated_packets.size() )
        {
            printf( "you leaked packets!\n" );
            printf( "%d packets leaked\n", m_numAllocatedPackets );
            for ( auto itor : allocated_packets )
            {
                auto p = itor.first;
                printf( "leaked packet %p\n", p );
            }
        }
#endif // #if PROTOCOL2_DEBUG_PACKET_LEAKS

        assert( m_numAllocatedPackets == 0 );
    }

    Packet* PacketFactory::CreatePacket( int type )
    {
        assert( type >= 0 );
        assert( type < m_numPacketTypes );

        Packet * packet = Create( type );
        if ( !packet )
            return NULL;
        
#if PROTOCOL2_DEBUG_PACKET_LEAKS
        printf( "create packet %p\n", packet );
        allocated_packets[packet] = 1;
        auto itor = allocated_packets.find( packet );
        assert( itor != allocated_packets.end() );
#endif // #if PROTOCOL2_DEBUG_PACKET_LEAKS
        
        m_numAllocatedPackets++;

        return packet;
    }

    void PacketFactory::DestroyPacket( Packet* packet )
    {
        if ( !packet )
            return;

#if PROTOCOL2_DEBUG_PACKET_LEAKS
        printf( "destroy packet %p\n", packet );
        auto itor = allocated_packets.find( packet );
        assert( itor != allocated_packets.end() );
        allocated_packets.erase( packet );
#endif // #if PROTOCOL2_DEBUG_PACKET_LEAKS

        assert( m_numAllocatedPackets > 0 );

        m_numAllocatedPackets--;

        Destroy( packet );
    }

    int PacketFactory::GetNumPacketTypes() const
    {
        return m_numPacketTypes;
    }

    Allocator & PacketFactory::GetAllocator()
    {
        assert( m_allocator );
        return *m_allocator;
    }
}
