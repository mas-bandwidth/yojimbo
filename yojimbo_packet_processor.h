/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#ifndef YOJIMBO_PACKET_PROCESSOR_H
#define YOJIMBO_PACKET_PROCESSOR_H

#include "yojimbo_config.h"
#include "yojimbo_packet.h"

namespace yojimbo
{
    enum PacketProcessErrors
    {
        PACKET_PROCESSOR_ERROR_NONE,                        // everything is fine
        PACKET_PROCESSOR_ERROR_KEY_IS_NULL,                 // we needed an encryption/decryption key but it was passed in as NULL
        PACKET_PROCESSOR_ERROR_PACKET_TOO_SMALL,            // an encrypted packet was discarded because it was too short to possibly contain valid data
        PACKET_PROCESSOR_ERROR_WRITE_PACKET_FAILED,         // failed to write packet
        PACKET_PROCESSOR_ERROR_READ_PACKET_FAILED,          // failed to read packet
        PACKET_PROCESSOR_ERROR_ENCRYPT_FAILED,              // encrypt packet failed
        PACKET_PROCESSOR_ERROR_DECRYPT_FAILED,              // decrypt packet failed
    };

    class PacketProcessor
    {
    public:

        PacketProcessor( Allocator & allocator, PacketFactory & packetFactory, uint32_t protocolId, int maxPacketSize, void * context = NULL );

        ~PacketProcessor();

        const uint8_t * WritePacket( Packet * packet, uint64_t sequence, int & packetBytes, bool encrypt = false, const uint8_t * key = NULL );

        Packet * ReadPacket( const uint8_t * packetData,  uint64_t & sequence, int packetBytes, bool & encrypted,
                             const uint8_t * key = NULL, const uint8_t * encryptedPacketTypes = NULL, const uint8_t * unencryptedPacketTypes = NULL );

        int GetMaxPacketSize() const { return m_maxPacketSize; }

        int GetError() const { return m_error; }

    private:

        Allocator * m_allocator;

        uint32_t m_protocolId;

        int m_error;
        int m_maxPacketSize;
        int m_absoluteMaxPacketSize;
        
        uint8_t * m_packetBuffer;
        uint8_t * m_scratchBuffer;

        void * m_context;

        PacketFactory * m_packetFactory;
    };
}

#endif // #ifndef YOJIMBO_PACKET_PROCESSOR
