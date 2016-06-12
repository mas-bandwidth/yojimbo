/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo_socket_interface.h"
#include "yojimbo_array.h"
#include "yojimbo_queue.h"
#include "yojimbo_common.h"
#include <stdint.h>
#include <inttypes.h>

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
        m_socket = new Socket( address );
    }

    SocketInterface::~SocketInterface()
    {
        assert( m_socket );

        delete m_socket;

        m_socket = NULL;
    }

    bool SocketInterface::IsError() const
    {
        assert( m_socket );

        return m_socket->IsError();
    }

    int SocketInterface::GetError() const
    {
        assert( m_socket );

        return m_socket->GetError();
    }

    bool SocketInterface::InternalSendPacket( const Address & to, const void * packetData, size_t packetBytes )
    {
        assert( m_socket );

        return m_socket->SendPacket( to, packetData, packetBytes );
    }

    int SocketInterface::InternalReceivePacket( Address & from, void * packetData, int maxPacketSize )
    {
        assert( m_socket );
        
        return m_socket->ReceivePacket( from, packetData, maxPacketSize );
    }
}
