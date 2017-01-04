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

#ifndef YOJIMBO_PACKET_PROCESSOR_H
#define YOJIMBO_PACKET_PROCESSOR_H

#include "yojimbo_config.h"
#include "yojimbo_packet.h"

/** @file */

namespace yojimbo
{
    class ReplayProtection;

    /**
        Packet processor error codes.
     */

    enum PacketProcessErrors
    {
        PACKET_PROCESSOR_ERROR_NONE,                            ///< No error. All is well.
        PACKET_PROCESSOR_ERROR_KEY_IS_NULL,                     ///< Needed an encryption/decryption key but it was passed in as NULL.
        PACKET_PROCESSOR_ERROR_PACKET_TOO_SMALL,                ///< An encrypted packet was discarded because it was too short to possibly contain valid data.
        PACKET_PROCESSOR_ERROR_PACKET_ALREADY_RECEIVED,         ///< An encrypted packet was discarded because it has already been received (replay protection).
        PACKET_PROCESSOR_ERROR_WRITE_PACKET_FAILED,             ///< Failed to write packet. See yojimbo::WritePacket.
        PACKET_PROCESSOR_ERROR_READ_PACKET_FAILED,              ///< Failed to read packet. See yojimbo::ReadPacket.
        PACKET_PROCESSOR_ERROR_ENCRYPT_FAILED,                  ///< Encrypt packet failed.
        PACKET_PROCESSOR_ERROR_DECRYPT_FAILED,                  ///< Decrypt packet failed.
    };

    /**
        Adds packet encryption and decryption on top of low-level read and write packet functions.

        @see yojimbo::WritePacket
        @see yojimbo::ReadPacket
     */

    class PacketProcessor
    {
    public:

        /** 
            Packet processor constructor.

            @param allocator The allocator to use.
            @param protocolId The protocol id that identifies your protocol. Typically a hash of your data and a protocol version number.
            @param maxPacketSize The maximum packet size that can be written (bytes).
         */

        PacketProcessor( Allocator & allocator, uint64_t protocolId, int maxPacketSize );

        /**
            Packet processor destructor.
         */

        ~PacketProcessor();

        /**
            Set a context to pass to the stream.

            @see BaseStream::SetContext
         */

        void SetContext( void * context );

        /** 
            Set a context to pass to the stream.

            @see BaseStream::SetUserContext
         */

        void SetUserContext( void * context );

        /**
            Write a packet.

            @param packet The packet to write.
            @param sequence The sequence number of the packet. Used as the nonce for encrypted packets. Ignored for unencrypted packets.
            @param packetBytes The number of bytes of packet data written [out].
            @param encrypt Should this packet be encrypted?
            @param key The key used for packet encryption.
            @param streamAllocator The allocator to set on the stream. See BaseStream::GetAllocator.
            @param packetFactory The packet factory so we know the range of packet types supported.

            @returns A pointer to the packet data written. NULL if the packet write failed. This is an internal scratch buffer. Do not cache it and do not free it.
         */

        const uint8_t * WritePacket( Packet * packet, uint64_t sequence, int & packetBytes, bool encrypt, const uint8_t * key, Allocator & streamAllocator, PacketFactory & packetFactory );

        /**
            Read a packet.

            @param packetData The packet data to read.
            @param sequence The packet sequence number [out]. Only set for encrypted packets. Set to 0 for unencrypted packets.
            @param packetBytes The number of bytes of packet data to read.
            @param encrypted Set to true if the packet is encrypted [out].
            @param key The key used to decrypt the packet, if it is encrypted.
            @param encryptedPacketTypes Entry n is 1 if packet type n is encrypted. Passed into the low-level packet read as the set of allowed packet types, if the packet is encrypted.
            @param unencryptedPacketTypes Entry n is 1 if packet type n is unencrypted. Passed into the low-level packet read as the set of allowed packet types, if the packet is not encrypted.
            @param streamAllocator The allocator to set on the steram. See BaseStream::GetAllocator.
            @param packetFactory The packet factory used to create the packet.
            @param replayProtection The replay protection buffer. Optional. Pass in NULL if not used.

            @returns The packet object if it was sucessfully read, NULL otherwise. You are responsible for destroying the packet created by this function.
         */

        Packet * ReadPacket( const uint8_t * packetData, uint64_t & sequence, int packetBytes, bool & encrypted, const uint8_t * key, const uint8_t * encryptedPacketTypes, const uint8_t * unencryptedPacketTypes, Allocator & streamAllocator, PacketFactory & packetFactory, ReplayProtection * replayProtection );

        /**
            Gets the maximum packet size to be generated.

            @returns The maximum packet size in bytes.
         */

        int GetMaxPacketSize() const { return m_maxPacketSize; }

        /**
            Get the packet processor error level.

            Use this to work out why read or write packet functions failed.

            @returns The packet processor error code.

            @see PacketProcessor::ReadPacket
            @see PacketProcessor::WritePacket
         */

        int GetError() const { return m_error; }

    private:

        Allocator * m_allocator;                            ///< The allocator passed in to the constructor.

        uint64_t m_protocolId;                              ///< The protocol id. This is used as part of the CRC32 for unencrypted packets.

        int m_error;                                        ///< The error level. 

        int m_maxPacketSize;                                ///< The maximum packet size (as in, serialized packet).

        int m_absoluteMaxPacketSize;                        ///< The absolute maximum packet size, considering header and encryption overhead.
        
        uint8_t * m_packetBuffer;                           ///< The packet buffer for reading and writing packets.
        
        uint8_t * m_scratchBuffer;                          ///< Scratch buffer used when one packet buffer is just not enough.

        void * m_context;                                   ///< Context to set on stream.

        void * m_userContext;                               ///< User context to set on stream.
    };
}

#endif // #ifndef YOJIMBO_PACKET_PROCESSOR
