/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#ifndef YOJIMBO_SOCKET_INTERFACE_H
#define YOJIMBO_SOCKET_INTERFACE_H

#include "yojimbo_config.h"
#include "yojimbo_network.h"
#include "yojimbo_base_interface.h"

namespace yojimbo
{
    class SocketInterface : public BaseInterface
    {
    public:

        SocketInterface( Allocator & allocator,
                         PacketFactory & packetFactory, 
                         const Address & address,
                         uint32_t protocolId,
                         int maxPacketSize = 4 * 1024,
                         int sendQueueSize = 1024,
                         int receiveQueueSize = 1024 );

        ~SocketInterface();

        bool IsError() const;

        int GetError() const;

    protected:

        virtual bool InternalSendPacket( const Address & to, const void * packetData, size_t packetBytes );
    
        virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        Socket * m_socket;
    };
}

#endif // #ifndef YOJIMBO_SOCKET_INTERFACE_H
