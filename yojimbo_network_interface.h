/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#ifndef YOJIMBO_NETWORK_INTERFACE_H
#define YOJIMBO_NETWORK_INTERFACE_H

#include "yojimbo_config.h"

namespace yojimbo
{
    enum NetworkInterfaceFlags
    {
        NETWORK_INTERFACE_FLAG_INSECURE_MODE = (1<<0)
    };

    class NetworkInterface
    {
    public:

        virtual ~NetworkInterface() {}

        virtual Packet * CreatePacket( int type ) = 0;

        virtual void DestroyPacket( Packet * packet ) = 0;

        virtual void SendPacket( const Address & address, Packet * packet, uint64_t sequence = 0, bool immediate = false ) = 0;

        virtual Packet * ReceivePacket( Address & from, uint64_t * sequence = NULL ) = 0;

        virtual void WritePackets() = 0;

        virtual void ReadPackets() = 0;

        virtual int GetMaxPacketSize() const = 0;

        virtual void SetContext( void * context ) = 0;

        virtual void EnablePacketEncryption() = 0;

        virtual void DisableEncryptionForPacketType( int type ) = 0;

        virtual bool IsEncryptedPacketType( int type ) const = 0;

        virtual bool AddEncryptionMapping( const Address & address, const uint8_t * sendKey, const uint8_t * receiveKey ) = 0;

        virtual bool RemoveEncryptionMapping( const Address & address ) = 0;

        virtual void ResetEncryptionMappings() = 0;

        virtual void AdvanceTime( double time ) = 0;

        virtual double GetTime() const = 0;

        virtual void SetFlags( uint64_t flags ) = 0;

        virtual uint64_t GetFlags() const = 0;

        virtual const Address & GetAddress() const = 0;
    };
}

#endif // #ifndef YOJIMBO_H
