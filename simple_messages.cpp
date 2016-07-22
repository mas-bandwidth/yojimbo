/*
    Yojimbo Client/Server Network Library.

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

#include "yojimbo.h"
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>
#include <time.h>

using namespace yojimbo;

const uint32_t ProtocolId = 0x12345678;
const int MaxPacketSize = 5 * 1024;
const int MaxBlockSize = 32 * 1024;
const int MaxSmallMessageSize = 256;

class MemoryTransport : public BaseTransport
{
public:

    MemoryTransport( Allocator & allocator, PacketFactory & packetFactory, const Address & address )
        : BaseTransport( allocator, packetFactory, address, ProtocolId, MaxPacketSize, 32, 32 )
    {
        m_sentPacketSize = 0;
        m_sentPacketData = NULL;
        m_receivePacketSize = 0;
        m_receivePacketData = NULL;
    }

    void SetReceivePacketData( const uint8_t * packetData, int packetSize, const Address & from )
    {
        m_receivePacketFrom = from;
        m_receivePacketData = packetData;
        m_receivePacketSize = packetSize;
    }

    const uint8_t * GetSentPacketData()
    {
        return m_sentPacketData;
    }

    int GetSentPacketSize()
    {
        return m_sentPacketSize;
    }

    void ClearSentPacketData()
    {
        m_sentPacketSize = 0;
        m_sentPacketData = NULL;
    }

protected:

    virtual bool InternalSendPacket( const Address & to, const void * packetData, int packetBytes )
    {
        (void)to;
        m_sentPacketSize = packetBytes;
        m_sentPacketData = (const uint8_t*) packetData;
        return true;
    }

    virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize )
    {
        (void)maxPacketSize;

        if ( m_receivePacketSize == 0 )
            return 0;

        from = m_receivePacketFrom;

        const int packetSize = m_receivePacketSize;

        memcpy( packetData, m_receivePacketData, packetSize );

        m_receivePacketSize = 0;
        m_receivePacketData = NULL;
        m_receivePacketFrom = Address();

        return packetSize;
    }

private:

    int m_sentPacketSize;
    int m_receivePacketSize;
    Address m_receivePacketFrom;
    const uint8_t * m_sentPacketData;
    const uint8_t * m_receivePacketData;
};

enum TestPacketTypes
{
    CONNECTION_PACKET,
    NUM_PACKET_TYPES
};

YOJIMBO_PACKET_FACTORY_START( TestPacketFactory, PacketFactory, NUM_PACKET_TYPES );
    YOJIMBO_DECLARE_PACKET_TYPE( CONNECTION_PACKET, ConnectionPacket );
YOJIMBO_PACKET_FACTORY_FINISH();

inline int GetNumBitsForMessage( uint16_t sequence )
{
    static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 5, 3, 7, 50 };
    const int modulus = sizeof( messageBitsArray ) / sizeof( int );
    const int index = sequence % modulus;
    return messageBitsArray[index];
}

struct SmallMessage : public Message
{
    SmallMessage()
    {
        messageSize = 0;
        memset( messageData, 0, MaxSmallMessageSize );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {        
        serialize_int( stream, messageSize, 1, MaxSmallMessageSize );
        serialize_bytes( stream, messageData, messageSize );
        return true;
    }

    int messageSize;
    uint8_t messageData[MaxSmallMessageSize];

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct LargeMessage : public BlockMessage
{
    LargeMessage() {}

    template <typename Stream> bool Serialize( Stream & stream ) { (void)stream; return true; }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum MessageType
{
    SMALL_MESSAGE,
    LARGE_MESSAGE,
    NUM_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START( TestMessageFactory, MessageFactory, NUM_MESSAGE_TYPES );
    YOJIMBO_DECLARE_MESSAGE_TYPE( SMALL_MESSAGE, SmallMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( LARGE_MESSAGE, LargeMessage );
YOJIMBO_MESSAGE_FACTORY_FINISH();

enum Channels
{
    UNRELIABLE_CHANNEL,
    RELIABLE_CHANNEL,
    NUM_CHANNELS
};

Message * GenerateRandomMessage( MessageFactory & messageFactory, uint64_t numMessagesSent, int channelId )
{
    if ( rand() % 100 )
    {
        SmallMessage * message = (SmallMessage*) messageFactory.Create( SMALL_MESSAGE );
        
        if ( message )
        {
            message->messageSize = random_int( 1, MaxSmallMessageSize );
            for ( int i = 0; i < message->messageSize; ++i )
                message->messageData[i] = uint8_t( numMessagesSent + i );
        }

        return message;
    }
    else
    {
        LargeMessage * largeMessage = (LargeMessage*) messageFactory.Create( LARGE_MESSAGE );

        if ( largeMessage )
        {
            const int blockSize = ( channelId == RELIABLE_CHANNEL ) ? ( 1024 + ( int( numMessagesSent ) * 33 ) % MaxBlockSize ) : ( 1024 + ( int( numMessagesSent ) * 33 ) % 1024 );

            uint8_t * blockData = (uint8_t*) messageFactory.GetAllocator().Allocate( blockSize );

            if ( blockData )
            {
                for ( int i = 0; i < blockSize; ++i )
                    blockData[i] = uint8_t( numMessagesSent + i );

                largeMessage->AttachBlock( messageFactory.GetAllocator(), blockData, blockSize );
            }
        }

        return largeMessage;
    }
}

static void CheckHandler( const char * condition, 
                          const char * function,
                          const char * file,
                          int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifndef NDEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define check( condition )                                                     \
do                                                                             \
{                                                                              \
    if ( !(condition) )                                                        \
    {                                                                          \
        CheckHandler( #condition, __FUNCTION__, __FILE__, __LINE__ );          \
    }                                                                          \
} while(0)

void ProcessMessage( Message * message, uint64_t numMessagesReceived, int channelId )
{
    check( message );

    if ( channelId == RELIABLE_CHANNEL )
    {
        check( message->GetId() == (uint16_t) numMessagesReceived );
    }

    switch ( message->GetType() )
    {
        case SMALL_MESSAGE:
        {
            SmallMessage * smallMessage = (SmallMessage*) message;

            if ( channelId == RELIABLE_CHANNEL )
            {
                for ( int i = 0; i < smallMessage->messageSize; ++i )
                {
                    check( smallMessage->messageData[i] == uint8_t( numMessagesReceived + i ) );
                }
            }

            printf( "channel %d: received small message (%d bytes)\n", channelId, smallMessage->messageSize );
        }
        break;

        case LARGE_MESSAGE:
        {
            LargeMessage * largeMessage = (LargeMessage*) message;

            const int blockSize = largeMessage->GetBlockSize();

            if ( channelId == RELIABLE_CHANNEL )
            {
                const int expectedBlockSize = ( channelId == RELIABLE_CHANNEL ) ? ( 1024 + ( int( numMessagesReceived ) * 33 ) % MaxBlockSize ) : ( 1024 + ( int( numMessagesReceived ) * 33 ) % 1024 );

                check( blockSize == expectedBlockSize );

                const uint8_t * blockData = largeMessage->GetBlockData();

                check( blockData );

                for ( int i = 0; i < blockSize; ++i )
                {
                    check( blockData[i] == uint8_t( numMessagesReceived + i ) );
                }
            }

            printf( "channel %d: received large message (%d bytes)\n", channelId, blockSize );
        }
        break;
    }
}

int WriteConnectionPacket( Connection & connection, MemoryTransport & transport, uint64_t sequence, uint8_t * packetBuffer, const Address & toAddress )
{
    ConnectionPacket * connectionPacket = connection.GeneratePacket();
    if ( !connectionPacket )
        return 0;

    transport.SendPacket( toAddress, connectionPacket, sequence, true );

    const int sentPacketSize = transport.GetSentPacketSize();
    if ( sentPacketSize == 0 )
        return 0;

    const uint8_t * sentPacketData = transport.GetSentPacketData();

    memcpy( packetBuffer, sentPacketData, sentPacketSize );

    transport.ClearSentPacketData();

    return sentPacketSize;
}

bool ReadConnectionPacket( Connection & connection, MemoryTransport & transport, const uint8_t * packetBuffer, int packetSize, const Address & fromAddress )
{
    transport.SetReceivePacketData( packetBuffer, packetSize, fromAddress );

    transport.ReadPackets();

    Address from;
    uint64_t sequence;
    Packet * packet = transport.ReceivePacket( from, &sequence );
    if ( !packet )
        return false;

    if ( packet->GetType() != CONNECTION_PACKET )
    {
        transport.DestroyPacket( packet );
        return false;
    }

    ConnectionPacket * connectionPacket = (ConnectionPacket*) packet;

    connection.ProcessPacket( connectionPacket );

    transport.DestroyPacket( packet );

    return true;
}

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

int MessagesMain()
{
    printf( "simple messages\n\n" );

    TestPacketFactory packetFactory;

    const uint16_t SenderPort = 30000;
    const uint16_t ReceiverPort = 40000;

    Address senderAddress( "::1", SenderPort );
    Address receiverAddress( "::1", ReceiverPort );

    MemoryTransport senderTransport( GetDefaultAllocator(), packetFactory, senderAddress );
    MemoryTransport receiverTransport( GetDefaultAllocator(), packetFactory, receiverAddress );

    senderTransport.EnablePacketEncryption();
    receiverTransport.EnablePacketEncryption();

    uint8_t senderToReceiverPrivateKey[KeyBytes];
    uint8_t receiverToSenderPrivateKey[KeyBytes];

    GenerateKey( senderToReceiverPrivateKey );
    GenerateKey( receiverToSenderPrivateKey );

    senderTransport.AddEncryptionMapping( receiverAddress, senderToReceiverPrivateKey, receiverToSenderPrivateKey );
    receiverTransport.AddEncryptionMapping( senderAddress, receiverToSenderPrivateKey, senderToReceiverPrivateKey );

    TestMessageFactory messageFactory;

    ConnectionConfig connectionConfig;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.numChannels = NUM_CHANNELS;
    connectionConfig.channelConfig[UNRELIABLE_CHANNEL].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    connectionConfig.channelConfig[UNRELIABLE_CHANNEL].messagePacketBudget = 2 * 1024;
    connectionConfig.channelConfig[RELIABLE_CHANNEL].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    connectionConfig.channelConfig[RELIABLE_CHANNEL].messagePacketBudget = 2 * 1024;

    Connection sender( GetDefaultAllocator(), packetFactory, messageFactory, connectionConfig );
    Connection receiver( GetDefaultAllocator(), packetFactory, messageFactory, connectionConfig );

    ConnectionContext context;
    context.connectionConfig = &connectionConfig;
    context.messageFactory = &messageFactory;

    senderTransport.SetContext( &context );
    receiverTransport.SetContext( &context );

    signal( SIGINT, interrupt_handler );    

    uint64_t numIterations = 0;
    uint64_t numMessagesSent[NUM_CHANNELS];
    uint64_t numMessagesReceived[NUM_CHANNELS];

    memset( numMessagesSent, 0, sizeof( numMessagesSent ) );
    memset( numMessagesReceived, 0, sizeof( numMessagesReceived ) );

    uint8_t senderPacketData[MaxPacketSize];
    uint8_t receiverPacketData[MaxPacketSize];

    double time = 0.0;

    while ( !quit )
    {
        for ( int channelId = 0; channelId < NUM_CHANNELS; ++channelId )
        {
            const int messagesToSend = random_int( 0, 64 );

            for ( int i = 0; i < messagesToSend; ++i )
            {
                if ( !sender.CanSendMessage( channelId ) )
                    break;

                Message * message = GenerateRandomMessage( messageFactory, numMessagesSent[channelId], channelId );

                if ( message )
                {
                    sender.SendMessage( message, channelId );
                    numMessagesSent[channelId]++;
                }
            }
        }

        const int senderPacketSize = WriteConnectionPacket( sender, senderTransport, numIterations, senderPacketData, receiverAddress );
        if ( !senderPacketSize )
        {
            printf( "error: failed to write connection packet (sender)\n" );
            return 1;
        }

        if ( !ReadConnectionPacket( receiver, receiverTransport, senderPacketData, senderPacketSize, senderAddress ) )
        {
            printf( "error: failed to read connection packet (receiver)\n" );
            return 1;
        }

        const int receiverPacketSize = WriteConnectionPacket( receiver, receiverTransport, numIterations, receiverPacketData, senderAddress );
        if ( !receiverPacketSize )
        {
            printf( "error: failed to write connection packet (receiver)\n" );
            return 1;
        }

        if ( !ReadConnectionPacket( sender, senderTransport, receiverPacketData, receiverPacketSize, receiverAddress ) )
        {
            printf( "error: failed to read connection packet (sender)\n" );
            return 1;
        }

        for ( int channelId = 0; channelId < NUM_CHANNELS; ++channelId )
        {
            while ( true )
            {
                Message * message = receiver.ReceiveMessage( channelId );
                if ( !message )
                    break;

                ProcessMessage( message, numMessagesReceived[channelId], channelId );

                messageFactory.Release( message );

                numMessagesReceived[channelId]++;
            }
        }

        sender.AdvanceTime( time );
        receiver.AdvanceTime( time );

        senderTransport.AdvanceTime( time );
        receiverTransport.AdvanceTime( time );

        numIterations++;
    }

    printf( "\nstopped\n" );

    return 0;
}

int main()
{
    printf( "\n" );

    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize Yojimbo!\n" );
        return 1;
    }

    srand( (unsigned int) time( NULL ) );

    int result = MessagesMain();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
