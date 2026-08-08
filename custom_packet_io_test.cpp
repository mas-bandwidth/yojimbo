/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2026, Más Bandwidth LLC.

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

#include "yojimbo.h"
#include "netcode.h"
#include "shared.h"

#include <deque>
#include <stdio.h>
#include <string.h>

using namespace yojimbo;

struct MemoryPacket
{
    Address from;
    int bytes;
    uint8_t data[NETCODE_MAX_PACKET_SIZE];
};

class MemoryPacketAdapter : public TestAdapter
{
public:
    explicit MemoryPacketAdapter( const Address & localAddress )
        : m_localAddress( localAddress ), m_peer( NULL ), m_sentPackets( 0 ), m_receivedPackets( 0 )
    {
    }

    void Connect( MemoryPacketAdapter & peer )
    {
        m_peer = &peer;
    }

    bool UseCustomPacketIO() const
    {
        return true;
    }

    void SendPacket( const Address & to, const uint8_t * packetData, int packetBytes )
    {
        if ( !m_peer || to != m_peer->m_localAddress || packetBytes <= 0 || packetBytes > NETCODE_MAX_PACKET_SIZE )
            return;

        MemoryPacket packet;
        packet.from = m_localAddress;
        packet.bytes = packetBytes;
        memcpy( packet.data, packetData, packetBytes );
        m_peer->m_packets.push_back( packet );
        ++m_sentPackets;
    }

    int ReceivePacket( Address & from, uint8_t * packetData, int maxPacketBytes )
    {
        if ( m_packets.empty() )
            return 0;

        const MemoryPacket packet = m_packets.front();
        m_packets.pop_front();
        if ( packet.bytes > maxPacketBytes )
            return 0;

        from = packet.from;
        memcpy( packetData, packet.data, packet.bytes );
        ++m_receivedPackets;
        return packet.bytes;
    }

    int SentPackets() const { return m_sentPackets; }
    int ReceivedPackets() const { return m_receivedPackets; }

private:
    Address m_localAddress;
    MemoryPacketAdapter * m_peer;
    std::deque<MemoryPacket> m_packets;
    int m_sentPackets;
    int m_receivedPackets;
};

static bool Require( bool condition, const char * message )
{
    if ( condition )
        return true;
    printf( "FAIL: %s\n", message );
    return false;
}

int main()
{
    if ( !InitializeYojimbo() )
        return 1;

    bool ok = true;
    {
        const Address clientAddress( "203.0.113.2", 41230 );
        const Address serverAddress( "203.0.113.1", 41230 );
        MemoryPacketAdapter clientAdapter( clientAddress );
        MemoryPacketAdapter serverAdapter( serverAddress );
        clientAdapter.Connect( serverAdapter );
        serverAdapter.Connect( clientAdapter );

        ClientServerConfig config;
        config.numChannels = 2;
        config.channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
        config.channel[1].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;

        uint8_t privateKey[KeyBytes];
        memset( privateKey, 0, sizeof( privateKey ) );
        double time = 100.0;

        Server server( GetDefaultAllocator(), privateKey, serverAddress, config, serverAdapter, time );
        server.Start( 1 );
        Client client( GetDefaultAllocator(), clientAddress, config, clientAdapter, time );
        client.InsecureConnect( privateKey, 1, serverAddress );

        for ( int i = 0; i < 256 && !client.IsConnected(); ++i )
        {
            client.SendPackets();
            server.SendPackets();
            client.ReceivePackets();
            server.ReceivePackets();
            time += 0.1;
            client.AdvanceTime( time );
            server.AdvanceTime( time );
        }

        ok &= Require( client.GetAddress() == clientAddress, "client synthetic local address changed" );
        ok &= Require( server.GetAddress() == serverAddress, "server synthetic local address changed" );
        ok &= Require( client.IsConnected(), "client did not connect over custom packet I/O" );
        ok &= Require( server.GetNumConnectedClients() == 1, "server did not admit the custom-I/O client" );
        ok &= Require( clientAdapter.SentPackets() > 0 && clientAdapter.ReceivedPackets() > 0,
                       "client adapter did not send and receive packets" );
        ok &= Require( serverAdapter.SentPackets() > 0 && serverAdapter.ReceivedPackets() > 0,
                       "server adapter did not send and receive packets" );

        TestMessage * toServer = (TestMessage*) client.CreateMessage( TEST_MESSAGE );
        TestMessage * toClient = (TestMessage*) server.CreateMessage( 0, TEST_MESSAGE );
        ok &= Require( toServer != NULL && toClient != NULL, "could not allocate channel test messages" );
        if ( toServer && toClient )
        {
            toServer->sequence = 11;
            toClient->sequence = 22;
            client.SendMessage( 0, toServer );
            server.SendMessage( 0, 1, toClient );

            Message * fromClient = NULL;
            Message * fromServer = NULL;
            for ( int i = 0; i < 256 && ( !fromClient || !fromServer ); ++i )
            {
                client.SendPackets();
                server.SendPackets();
                client.ReceivePackets();
                server.ReceivePackets();
                if ( !fromClient )
                    fromClient = server.ReceiveMessage( 0, 0 );
                if ( !fromServer )
                    fromServer = client.ReceiveMessage( 1 );
                time += 0.1;
                client.AdvanceTime( time );
                server.AdvanceTime( time );
            }

            ok &= Require( fromClient && ((TestMessage*) fromClient)->sequence == 11,
                           "reliable client message did not cross custom packet I/O" );
            ok &= Require( fromServer && ((TestMessage*) fromServer)->sequence == 22,
                           "unreliable server message did not cross custom packet I/O" );
            if ( fromClient )
                server.ReleaseMessage( 0, fromClient );
            if ( fromServer )
                client.ReleaseMessage( fromServer );
        }

        client.Disconnect();
        server.Stop();
    }
    ShutdownYojimbo();

    if ( !ok )
        return 1;

    printf( "OK: custom packet I/O handshake passed without native sockets\n" );
    return 0;
}
