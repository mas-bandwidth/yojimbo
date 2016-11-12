/*
    Profile Testbed

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
#define QUIET 1

#include "shared.h"
#include <signal.h>

static const int MaxBlockSize = 21 * 1024;

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

// NOTE: This is not a pretty file. I'm just trying to profile some stuff and I'm doing it in the most straightforward, hamfisted way possible!

struct ServerData
{
    GameServer * server;

    Transport * transport;

    Address address;

    uint64_t numMessagesSent[MaxClients];
    uint64_t numMessagesReceived[MaxClients];

    ServerData()
    {
        memset( this, 0, sizeof( ServerData ) );
    }

    ~ServerData()
    {
        if ( server )
        {
            server->Stop();
            delete server;
            server = NULL;
        }

        if ( transport )
        {
            delete transport;
            transport = NULL;
        }
    }   
};

struct ClientData
{
    GameClient * client;

    Transport * transport;

    Address address;

    uint64_t clientId;

    uint8_t connectTokenData[ConnectTokenBytes];
    uint8_t connectTokenNonce[NonceBytes];

    uint8_t clientToServerKey[KeyBytes];
    uint8_t serverToClientKey[KeyBytes];

    uint64_t connectTokenExpireTimestamp;

    int numServerAddresses;
    Address serverAddresses[MaxServersPerConnect];

    uint64_t numMessagesSent;
    uint64_t numMessagesReceived;

    ClientData()
    {
        memset( this, 0, sizeof( ClientData ) );
    }

    ~ClientData()
    {
        if ( client )
        {
            client->Disconnect();
            delete client;
            client = NULL;
        }

        if ( transport )
        {
            delete transport;
            transport = NULL;
        }
    }
};

Message * CreateRandomMessage( MessageFactory & messageFactory, uint64_t numMessagesSent )
{
    if ( rand() % 100 )
    {
        TestMessage * message = (TestMessage*) messageFactory.Create( TEST_MESSAGE );
        
        if ( message )
        {
            message->sequence = (uint16_t) numMessagesSent;
            
            return message;
        }
    }
    else
    {
        TestBlockMessage * blockMessage = (TestBlockMessage*) messageFactory.Create( TEST_BLOCK_MESSAGE );

        if ( blockMessage )
        {
            blockMessage->sequence = (uint16_t) numMessagesSent;

            const int blockSize = 1 + ( int( numMessagesSent ) * 33 ) % MaxBlockSize;

            uint8_t * blockData = (uint8_t*) messageFactory.GetAllocator().Allocate( blockSize );

            if ( blockData )
            {
                for ( int j = 0; j < blockSize; ++j )
                    blockData[j] = uint8_t( numMessagesSent + j );

                blockMessage->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );

                return blockMessage;
            }
        }
    }

    return NULL;
}

int ProfileMain()
{
    srand( (unsigned int) time( NULL ) );

    ServerData * serverData = new ServerData();
    serverData->address = Address( "::1", ServerPort );

	ClientData * clientData = new ClientData[MaxClients];

    LocalMatcher matcher;

    for ( int i = 0; i < MaxClients; ++i )
    {
        clientData[i].address = Address( "::1", ClientPort + i );

        clientData[i].clientId = 1 + i;

        if ( !matcher.RequestMatch( clientData[i].clientId, 
                                    clientData[i].connectTokenData, 
                                    clientData[i].connectTokenNonce, 
                                    clientData[i].clientToServerKey, 
                                    clientData[i].serverToClientKey,
                                    clientData[i].connectTokenExpireTimestamp, 
                                    clientData[i].numServerAddresses, 
                                    clientData[i].serverAddresses ) )
        {
            printf( "error: client %d request match failed\n", i );
            delete serverData;
            delete [] clientData;
            return 1;
        }
    }

    double time = 100.0;

    serverData->transport = new NetworkTransport( GetDefaultAllocator(), serverData->address, ProtocolId, time );

    for ( int i = 0; i < MaxClients; ++i )
    {
        clientData[i].transport = new NetworkTransport( GetDefaultAllocator(), clientData[i].address, ProtocolId, time );
    }

    serverData->server = new GameServer( GetDefaultAllocator(), *serverData->transport, ClientServerConfig(), time );

    for ( int i = 0; i < MaxClients; ++i )
        clientData[i].client = new GameClient( GetDefaultAllocator(), *clientData[i].transport, ClientServerConfig(), time );

    serverData->server->SetServerAddress( serverData->address );

    serverData->server->Start();

    for ( int i = 0; i < MaxClients; ++i )
    {
        clientData[i].client->Connect( serverData->address, 
                                       clientData[i].connectTokenData, 
                                       clientData[i].connectTokenNonce, 
                                       clientData[i].clientToServerKey, 
                                       clientData[i].serverToClientKey,
                                       clientData[i].connectTokenExpireTimestamp );
    }

    signal( SIGINT, interrupt_handler ); 

    while ( !quit )
    {
        serverData->server->SendPackets();

        for ( int i = 0; i < MaxClients; ++i )
            clientData[i].client->SendPackets();

        serverData->transport->WritePackets();

        for ( int i = 0; i < MaxClients; ++i )
            clientData[i].transport->WritePackets();

        serverData->transport->ReadPackets();

        for ( int i = 0; i < MaxClients; ++i )
            clientData[i].transport->ReadPackets();

        serverData->server->ReceivePackets();

        for ( int i = 0; i < MaxClients; ++i )
            clientData[i].client->ReceivePackets();

        for ( int i = 0; i < MaxClients; ++i )
        {
            if ( clientData[i].client->ConnectionFailed() )
            {
                printf( "\nerror: client %d connect failed!", i );
                quit = true;
                break;
            }
        }

        time += 0.1;

        for ( int i = 0; i < MaxClients; ++i )
        {
            const int messagesToSend = random_int( 0, 64 );

            for ( int j = 0; j < messagesToSend; ++j )
            {
                if ( !clientData[i].client->CanSendMsg() )
                    break;

                Message * message = CreateRandomMessage( clientData[i].client->GetMsgFactory(), clientData[i].numMessagesSent );

                if ( message )
                {
                    clientData[i].client->SendMsg( message );

                    clientData[i].numMessagesSent++;
                }
            }
        }            

        for ( int i = 0; i < MaxClients; ++i )
        {
            const int messagesToSend = random_int( 0, 64 );

            for ( int j = 0; j < messagesToSend; ++j )
            {
                if ( !serverData->server->CanSendMsg( i ) )
                    break;

                Message * message = CreateRandomMessage( serverData->server->GetMsgFactory( i ), serverData->numMessagesSent[i] );

                if ( message )
                {
                    serverData->server->SendMsg( i, message );

                    serverData->numMessagesSent[i]++;
                }
            } 
        }            

        for ( int i = 0; i < MaxClients; ++i )
        {
            while ( true )
            {
                Message * message = serverData->server->ReceiveMsg( i );

                if ( !message )
                    break;

                serverData->numMessagesReceived[i]++;

//                printf( "server received message %d from client %d\n", message->GetId(), i );

				serverData->server->ReleaseMsg( i, message );
            }
        }

        for ( int i = 0; i < MaxClients; ++i )
        {
            while ( true )
            {
                Message * message = clientData[i].client->ReceiveMsg();
                
                if ( !message )
                    break;

                clientData[i].numMessagesReceived++;

//                printf( "client received message %d from client %d\n", message->GetId(), i );

				clientData[i].client->ReleaseMsg( message );
            }
        }

        serverData->server->CheckForTimeOut();

        for ( int i = 0; i < MaxClients; ++i )
            clientData[i].client->CheckForTimeOut();

        serverData->server->AdvanceTime( time );

        for ( int i = 0; i < MaxClients; ++i )
            clientData[i].client->AdvanceTime( time );

        serverData->transport->AdvanceTime( time );

        for ( int i = 0; i < MaxClients; ++i )
            clientData[i].transport->AdvanceTime( time );

        platform_sleep( 0.1 );
    }

    if ( quit )
    {
        printf( "\n\nstopped\n" );
    }

	delete [] clientData;

	delete serverData;

    return 0;
}

int main()
{
    printf( "\nprofile test\n\n" );

    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize Yojimbo!\n" );
        return 1;
    }

    srand( (unsigned int) time( NULL ) );

    int result = ProfileMain();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
