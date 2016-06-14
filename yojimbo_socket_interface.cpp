/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo_socket_interface.h"

namespace yojimbo
{
    SocketInterface::SocketInterface( Allocator & allocator, 
                                      PacketFactory & packetFactory, 
                                      const Address & address,
                                      uint32_t protocolId,
                                      int maxPacketSize, 
                                      int sendQueueSize, 
                                      int receiveQueueSize )
        : BaseInterface( allocator, 
                         packetFactory, 
                         address,
                         protocolId,
                         maxPacketSize,
                         sendQueueSize,
                         receiveQueueSize )
    {
        m_socket = YOJIMBO_NEW( allocator, Socket, address );
    }

    SocketInterface::~SocketInterface()
    {
        YOJIMBO_DELETE( GetAllocator(), Socket, m_socket );
    }

    bool SocketInterface::IsError() const
    {
        return m_socket->IsError();
    }

    int SocketInterface::GetError() const
    {
        return m_socket->GetError();
    }

    bool SocketInterface::InternalSendPacket( const Address & to, const void * packetData, int packetBytes )
    {
        return m_socket->SendPacket( to, packetData, packetBytes );
    }

    int SocketInterface::InternalReceivePacket( Address & from, void * packetData, int maxPacketSize )
    {
        return m_socket->ReceivePacket( from, packetData, maxPacketSize );
    }
}
