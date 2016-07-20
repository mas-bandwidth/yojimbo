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

const uint32_t ProtocolId = 0x12345678;

const int MaxPacketSize = 4 * 1024;
const int MaxBlockSize = 10 * 1024;

using namespace yojimbo;

class MemoryTransport : public BaseTransport
{
public:

    MemoryTransport( Allocator & allocator, PacketFactory & packetFactory, const Address & address )
        : BaseTransport( allocator, packetFactory, address, ProtocolId, MaxPacketSize, 32, 32 )
    {
        m_sentPacketSize = 0;
        m_receivePacketSize = 0;
        m_receivePacketData = NULL;
    }

    void SetReceivePacketData( const Address & from, const uint8_t * packetData, int packetSize )
    {
        m_receivePacketFrom = from;
        m_receivePacketData = packetData;
        m_receivePacketSize = packetSize;
    }

    uint8_t * GetSentPacketData()
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
    }

protected:

    virtual bool InternalSendPacket( const Address & to, const void * packetData, int packetBytes )
    {
        (void)to;
        (void)packetData;
        (void)packetBytes;
        return true;
    }

    virtual int InternalReceivePacket( Address & from, void * packetData, int maxPacketSize )
    {
        (void)maxPacketSize;

        if ( m_receivePacketSize == 0 )
            return 0;

        const int packetSize = m_receivePacketSize;

        from = m_receivePacketFrom;

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
    uint8_t m_sentPacketData[MaxPacketSize];
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

struct TestMessage : public Message
{
    uint16_t sequence;

    TestMessage()
    {
        sequence = 0;
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );

        int numBits = GetNumBitsForMessage( sequence );
        int numWords = numBits / 32;
        uint32_t dummy = 0;
        for ( int i = 0; i < numWords; ++i )
            serialize_bits( stream, dummy, 32 );
        int numRemainderBits = numBits - numWords * 32;
        if ( numRemainderBits > 0 )
            serialize_bits( stream, dummy, numRemainderBits );

        return true;
    }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestBlockMessage : public BlockMessage
{
    uint16_t sequence;

    TestBlockMessage()
    {
        sequence = 0;
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );
        return true;
    }

    YOJIMBO_ADD_VIRTUAL_SERIALIZE_FUNCTIONS();
};

enum MessageType
{
    TEST_MESSAGE,
    TEST_BLOCK_MESSAGE,
    NUM_MESSAGE_TYPES
};

YOJIMBO_MESSAGE_FACTORY_START( TestMessageFactory, MessageFactory, NUM_MESSAGE_TYPES );
    YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_MESSAGE, TestMessage );
    YOJIMBO_DECLARE_MESSAGE_TYPE( TEST_BLOCK_MESSAGE, TestBlockMessage );
YOJIMBO_MESSAGE_FACTORY_FINISH();

Message * GenerateRandomMessage( MessageFactory & messageFactory, uint64_t numMessagesSent )
{
    if ( rand() % 100 )
    {
        TestMessage * message = (TestMessage*) messageFactory.Create( TEST_MESSAGE );
        
        if ( message )
        {
            message->sequence = (uint16_t) numMessagesSent;
        }

        return message;
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
            }
        }

        return blockMessage;
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

void ProcessMessage( TestMessageFactory & messageFactory, Message * message, uint64_t numMessagesReceived )
{
    check( message );

    check( message->GetId() == (uint16_t) numMessagesReceived );

    switch ( message->GetType() )
    {
        case TEST_MESSAGE:
        {
            TestMessage * testMessage = (TestMessage*) message;

            check( testMessage->sequence == uint16_t( numMessagesReceived ) );

            printf( "received message %d\n", testMessage->sequence );
        }
        break;

        case TEST_BLOCK_MESSAGE:
        {
            TestBlockMessage * blockMessage = (TestBlockMessage*) message;

            check( blockMessage->sequence == uint16_t( numMessagesReceived ) );

            const int blockSize = blockMessage->GetBlockSize();

            const int expectedBlockSize = 1 + ( int( numMessagesReceived ) * 33 ) % MaxBlockSize;

            check( blockSize == expectedBlockSize );

            const uint8_t * blockData = blockMessage->GetBlockData();

            check( blockData );

            for ( int i = 0; i < blockSize; ++i )
            {
                check( blockData[i] == uint8_t( numMessagesReceived + i ) );
            }

            printf( "received block %d\n", uint16_t( numMessagesReceived ) );
        }
        break;
    }

    messageFactory.Release( message );
}

int WriteConnectionPacket( Connection & connection, MemoryTransport & transport, uint64_t sequence, uint8_t * packetBuffer, const Address & toAddress )
{
    // todo: GeneratePacket is actually a much better name. this function does not "write" the packet...
    ConnectionPacket * connectionPacket = connection.WritePacket();         
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

bool ReadConnectionPacket( Connection & connection, const uint8_t * packetBuffer, int packetSize, const Address & fromAddress )
{
    (void)connection;
    (void)packetBuffer;
    (void)packetSize;
    (void)fromAddress;
    return true;
}

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

int MessagesMain()
{
    printf( "messages\n\n" );

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

    Connection sender( GetDefaultAllocator(), packetFactory, messageFactory, connectionConfig );
    Connection receiver( GetDefaultAllocator(), packetFactory, messageFactory, connectionConfig );

    ConnectionContext context;
    context.connectionConfig = &connectionConfig;
    context.messageFactory = &messageFactory;

    senderTransport.SetContext( &context );
    receiverTransport.SetContext( &context );

    signal( SIGINT, interrupt_handler );    

    uint64_t numIterations = 0;
    uint64_t numMessagesSent = 0;
    uint64_t numMessagesReceived = 0;

    uint8_t senderPacketData[MaxPacketSize];
    uint8_t receiverPacketData[MaxPacketSize];

    while ( !quit )
    {
        const int messagesToSend = random_int( 0, 64 );

        for ( int i = 0; i < messagesToSend; ++i )
        {
            if ( !sender.CanSendMessage() )
                break;

            Message * message = GenerateRandomMessage( messageFactory, numMessagesSent );

            if ( message )
            {
                sender.SendMessage( message );
                numMessagesSent++;
            }
        }

        const int senderPacketSize = WriteConnectionPacket( sender, senderTransport, numIterations, senderPacketData, receiverAddress );
        if ( !senderPacketSize )
        {
            printf( "error: failed to write connection packet (sender)\n" );
            return 1;
        }

        if ( !ReadConnectionPacket( receiver, senderPacketData, senderPacketSize, senderAddress ) )
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

        if ( !ReadConnectionPacket( sender, receiverPacketData, receiverPacketSize, receiverAddress ) )
        {
            printf( "error: failed to read connection packet (sender)\n" );
            return 1;
        }

        while ( true )
        {
            Message * message = receiver.ReceiveMessage();
            if ( !message )
                break;

            ProcessMessage( messageFactory, message, numMessagesReceived );

            messageFactory.Release( message );

            numMessagesReceived++;
        }

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
