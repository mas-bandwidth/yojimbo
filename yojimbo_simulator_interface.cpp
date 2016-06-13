/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo_simulator_interface.h"

namespace yojimbo
{
    SimulatorInterface::SimulatorInterface( Allocator & allocator, 
                                            NetworkSimulator & networkSimulator,
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
        assert( address.IsValid() );

        m_networkSimulator = &networkSimulator;
    }

    SimulatorInterface::~SimulatorInterface()
    {
        assert( m_networkSimulator );

        m_networkSimulator->DiscardPackets( GetAddress() );

        m_networkSimulator = NULL;
    }

    void SimulatorInterface::AdvanceTime( double time )
    {
        BaseInterface::AdvanceTime( time );

        m_networkSimulator->AdvanceTime( time );
    }

    bool SimulatorInterface::InternalSendPacket( const Address & to, const void * packetData, size_t packetBytes )
    {
        assert( m_networkSimulator );

        uint8_t * packetDataCopy = new uint8_t[packetBytes];

        if ( !packetDataCopy )
            return false;

        memcpy( packetDataCopy, packetData, packetBytes );

        m_networkSimulator->SendPacket( GetAddress(), to, packetDataCopy, packetBytes );

        return true;
    }

    int SimulatorInterface::InternalReceivePacket( Address & from, void * packetData, int maxPacketSize )
    {
        assert( m_networkSimulator );

        int packetSize = 0;

        uint8_t * simulatorPacketData = m_networkSimulator->ReceivePacket( from, GetAddress(), packetSize );

        if ( !simulatorPacketData )
            return 0;

        assert( packetSize > 0 );
        assert( packetSize <= maxPacketSize );

        memcpy( packetData, simulatorPacketData, maxPacketSize );

        delete [] simulatorPacketData;

        return packetSize;
    }
}
