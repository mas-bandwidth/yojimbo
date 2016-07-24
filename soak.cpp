/*
    Soak Test

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

#define SERVER 1
#define CLIENT 1
#define MATCHER 1

#include "shared.h"
#include <signal.h>

static const int MaxBlockSize = 25 * 1024;

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

int SoakMain()
{
    srand( (unsigned int) time( NULL ) );

    LocalMatcher matcher;

    uint64_t clientId = 1;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnectToken];

    memset( connectTokenNonce, 0, NonceBytes );

    if ( !matcher.RequestMatch( clientId, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey, numServerAddresses, serverAddresses ) )
    {
        printf( "error: request match failed\n" );
        return 1;
    }

    GamePacketFactory packetFactory;

    NetworkSimulator networkSimulator( GetDefaultAllocator() );

    networkSimulator.SetJitter( 250 );
    networkSimulator.SetLatency( 250 );
    networkSimulator.SetDuplicates( 10 );
    networkSimulator.SetPacketLoss( 50 );

    Address clientAddress( "::1", ClientPort );
    Address serverAddress( "::1", ServerPort );

    SimulatorTransport clientTransport( GetDefaultAllocator(), networkSimulator, packetFactory, clientAddress, ProtocolId );
    SimulatorTransport serverTransport( GetDefaultAllocator(), networkSimulator, packetFactory, serverAddress, ProtocolId );

    ConnectionConfig connectionConfig;
    connectionConfig.maxPacketSize = 1100;
    connectionConfig.numChannels = 1;
    connectionConfig.channelConfig[0].messagePacketBudget = 256;
    connectionConfig.channelConfig[0].maxMessagesPerPacket = 256;

    GameClient client( GetDefaultAllocator(), clientTransport, connectionConfig );

    GameServer server( GetDefaultAllocator(), serverTransport, connectionConfig );

    server.SetServerAddress( serverAddress );

    server.Start();
    
    client.Connect( serverAddress, connectTokenData, connectTokenNonce, clientToServerKey, serverToClientKey );

    uint64_t numMessagesSentToServer = 0;
    uint64_t numMessagesReceivedFromClient = 0;

    signal( SIGINT, interrupt_handler );    

    double time = 0.0;

    while ( !quit )
    {
        client.SendPackets();
        server.SendPackets();

        clientTransport.WritePackets();
        serverTransport.WritePackets();

        clientTransport.ReadPackets();
        serverTransport.ReadPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            break;
        }

        time += 0.1f;

        if ( client.IsConnected() )
        {
            const int messagesToSend = random_int( 0, 64 );

            for ( int i = 0; i < messagesToSend; ++i )
            {
                if ( !client.CanSendMessage() )
                    break;

                if ( rand() % 100 )
                {
                    GameMessage * message = (GameMessage*) client.CreateMessage( GAME_MESSAGE );
                    
                    if ( message )
                    {
                        message->sequence = (uint16_t) numMessagesSentToServer;
                        
                        client.SendMessage( message );

                        numMessagesSentToServer++;
                    }
                }
                else
                {
                    GameBlockMessage * blockMessage = (GameBlockMessage*) client.CreateMessage( GAME_BLOCK_MESSAGE );

                    if ( blockMessage )
                    {
                        blockMessage->sequence = (uint16_t) numMessagesSentToServer;

                        const int blockSize = 1 + ( int( numMessagesSentToServer ) * 33 ) % MaxBlockSize;

                        Allocator & messageAllocator = client.GetMessageFactory().GetAllocator();

                        uint8_t * blockData = (uint8_t*) messageAllocator.Allocate( blockSize );

                        if ( blockData )
                        {
                            for ( int j = 0; j < blockSize; ++j )
                                blockData[j] = uint8_t( numMessagesSentToServer + j );

                            blockMessage->AttachBlock( messageAllocator, blockData, blockSize );

                            client.SendMessage( blockMessage );

                            numMessagesSentToServer++;
                        }
                    }
                }
            }
        
            const int clientIndex = client.GetClientIndex();

            while ( true )
            {
                Message * message = server.ReceiveMessage( clientIndex );

                if ( !message )
                    break;

                assert( message->GetId() == (uint16_t) numMessagesReceivedFromClient );

                switch ( message->GetType() )
                {
                    case GAME_MESSAGE:
                    {
                        GameMessage * gameMessage = (GameMessage*) message;

                        assert( gameMessage->sequence == uint16_t( numMessagesReceivedFromClient ) );

                        printf( "received message %d\n", gameMessage->sequence );

                        server.ReleaseMessage( clientIndex, message );

                        numMessagesReceivedFromClient++;
                    }
                    break;

                    case GAME_BLOCK_MESSAGE:
                    {
                        GameBlockMessage * blockMessage = (GameBlockMessage*) message;

                        assert( blockMessage->sequence == uint16_t( numMessagesReceivedFromClient ) );

                        const int blockSize = blockMessage->GetBlockSize();

                        const int expectedBlockSize = 1 + ( int( numMessagesReceivedFromClient ) * 33 ) % MaxBlockSize;

                        if ( blockSize  != expectedBlockSize )
                        {
                            printf( "error: block size mismatch. expected %d, got %d\n", expectedBlockSize, blockSize );
                            return 1;
                        }

                        const uint8_t * blockData = blockMessage->GetBlockData();

                        assert( blockData );

                        for ( int i = 0; i < blockSize; ++i )
                        {
                            if ( blockData[i] != uint8_t( numMessagesReceivedFromClient + i ) )
                            {
                                printf( "error: block data mismatch. expected %d, but blockData[%d] = %d\n", uint8_t( numMessagesReceivedFromClient + i ), i, blockData[i] );
                                return 1;
                            }
                        }

                        printf( "received block %d\n", uint16_t( numMessagesReceivedFromClient ) );

                        server.ReleaseMessage( clientIndex, message );

                        numMessagesReceivedFromClient++;
                    }
                    break;
                }
            }
        }

        client.AdvanceTime( time );
        server.AdvanceTime( time );

        clientTransport.AdvanceTime( time );
        serverTransport.AdvanceTime( time );
    }

    if ( quit )
    {
        printf( "\nstopped\n" );
    }

    return 0;
}

int main()
{
    printf( "\nsoak test\n\n" );

    verbose_logging = true;

    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize Yojimbo!\n" );
        return 1;
    }

    srand( (unsigned int) time( NULL ) );

    int result = SoakMain();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
