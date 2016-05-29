/*
    Example source code for "Sending Large Blocks of Data"

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.

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

#define NETWORK2_IMPLEMENTATION
#define PROTOCOL2_IMPLEMENTATION

#include "network2.h"
#include "protocol2.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const int MaxPacketSize = 1200;
const int SliceSize = 1024;
const int MaxSlicesPerChunk = 32;
const int MaxChunkSize = SliceSize * MaxSlicesPerChunk;

const float SliceMinimumResendTime = 0.1f;
const float MinimumTimeBetweenAcks = 0.1f;

//#define SOAK_TEST 1                // uncomment this line to loop forever and soak

#if SOAK_TEST
const int NumChunksToSend = -1;
#else // #if SOAK_TEST
const int NumChunksToSend = 32;
#endif // #if SOAK_TEST

const uint32_t ProtocolId = 0x11223344;

enum PacketTypes
{
    SLICE_PACKET,                    // this packet contains slice x out of y that makes up chunk n
    ACK_PACKET,                      // this packet acks slices of chunk n that have been received
    NUM_PACKET_TYPES
};

struct SlicePacket : public protocol2::Packet
{
    uint16_t chunkId;
    int sliceId;
    int numSlices;
    int sliceBytes;
    uint8_t data[SliceSize];

    SlicePacket() : Packet( SLICE_PACKET )
    {
        chunkId = 0;
        sliceId = 0;
        numSlices = 0;
        sliceBytes = 0;
        memset( data, 0, sizeof( data ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bits( stream, chunkId, 16 );
        serialize_int( stream, sliceId, 0, MaxSlicesPerChunk - 1 );
        serialize_int( stream, numSlices, 1, MaxSlicesPerChunk );
        if ( sliceId == numSlices - 1 )
        {
            serialize_int( stream, sliceBytes, 1, SliceSize );
        }
        else if ( Stream::IsReading )
        {
            sliceBytes = SliceSize;
        }
        serialize_bytes( stream, data, sliceBytes );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct AckPacket : public protocol2::Packet
{
    uint16_t chunkId;
    int numSlices;
    bool acked[MaxSlicesPerChunk];

    AckPacket() : Packet( ACK_PACKET )
    {
        chunkId = 0;
        numSlices = 0;
        memset( acked, 0, sizeof( acked ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bits( stream, chunkId, 16 );
        serialize_int( stream, numSlices, 1, MaxSlicesPerChunk );
        for ( int i = 0; i < numSlices; ++i )
            serialize_bool( stream, acked[i] );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct PacketFactory : public protocol2::PacketFactory
{
    PacketFactory() : protocol2::PacketFactory( NUM_PACKET_TYPES ) {}

    protocol2::Packet* Create( int type )
    {
        switch ( type )
        {
            case SLICE_PACKET:   return new SlicePacket();
            case ACK_PACKET:     return new AckPacket();
        }
        return NULL;
    }

    void Destroy( protocol2::Packet *packet )
    {
        delete packet;
    }
};

static PacketFactory packetFactory;

class ChunkSender
{
    bool sending;                                               // true if we are currently sending a chunk. can only send one chunk at a time
    uint16_t chunkId;                                           // the chunk id. starts at 0 and increases as each chunk is successfully sent and acked.
    int chunkSize;                                              // the size of the chunk that is being sent in bytes
    int numSlices;                                              // the number of slices in the current chunk being sent
    int currentSliceId;                                         // the current slice id to be considered next time we send a slice packet. iteration starts here.
    int numAckedSlices;                                         // number of slices acked by the receiver. when num slices acked = num slices, the send is completed.
    bool acked[MaxSlicesPerChunk];                              // acked flag for each slice of the chunk. chunk send completes when all slices are acked. acked slices are skipped when iterating for next slice to send.
    double timeLastSent[MaxSlicesPerChunk];                     // time the slice of the chunk was last sent. avoids redundant behavior
    uint8_t chunkData[MaxChunkSize];                            // chunk data being sent.

public:

    ChunkSender()
    {
        memset( this, 0, sizeof( ChunkSender ) );
    }

    void SendChunk( const uint8_t *data, int size )
    {
        assert( data );
        assert( size > 0 );
        assert( size <= MaxChunkSize );
        assert( !IsSending() );

        sending = true;
        chunkSize = size;
        currentSliceId = 0;
        numAckedSlices = 0;

        numSlices = ( size + SliceSize - 1 ) / SliceSize;

        assert( numSlices > 0 );
        assert( numSlices <= MaxSlicesPerChunk );
        assert( ( numSlices - 1 ) * SliceSize < chunkSize );
        assert( numSlices * SliceSize >= chunkSize );

        memset( acked, 0, sizeof( acked ) );
        memset( timeLastSent, 0, sizeof( timeLastSent ) );
        memcpy( chunkData, data, size );

        printf( "sending chunk %d in %d slices (%d bytes)\n", chunkId, numSlices, chunkSize );
    }

    bool IsSending()
    {
        return sending;
    }

    SlicePacket* GenerateSlicePacket( double t )
    {
        if ( !sending ) 
            return NULL;

        SlicePacket *packet = NULL;

        for ( int i = 0; i < numSlices; ++i )
        {
            const int sliceId = ( currentSliceId + i ) % numSlices;

            if ( acked[sliceId] )
                continue;

            if ( timeLastSent[sliceId] + SliceMinimumResendTime < t )
            {
                packet = (SlicePacket*) packetFactory.CreatePacket( SLICE_PACKET );
                packet->chunkId = chunkId;
                packet->sliceId = sliceId;
                packet->numSlices = numSlices;
                packet->sliceBytes = ( sliceId == numSlices - 1 ) ? ( SliceSize - ( SliceSize * numSlices - chunkSize ) ) : SliceSize;
                memcpy( packet->data, chunkData + sliceId * SliceSize, packet->sliceBytes );
                printf( "sent slice %d of chunk %d (%d bytes)\n", sliceId, chunkId, packet->sliceBytes );
                break;
            }
        }

        currentSliceId = ( currentSliceId + 1 ) % numSlices;

        return packet;
    }

    bool ProcessAckPacket( AckPacket *packet )
    {
        assert( packet );

        if ( !sending )
            return false;

        if ( packet->chunkId != chunkId )
            return false;

        if ( packet->numSlices != numSlices )
            return false;

        for ( int i = 0; i < numSlices; ++i )
        {
            if ( acked[i] == false && packet->acked[i] )
            {
                acked[i] = true;
                numAckedSlices++;
                assert( numAckedSlices >= 0 );
                assert( numAckedSlices <= numSlices );
                printf( "acked slice %d of chunk %d [%d/%d]\n", i, chunkId, numAckedSlices, numSlices );
                if ( numAckedSlices == numSlices )
                {
                    printf( "all slices of chunk %d acked, send completed\n", chunkId );
                    sending = false;
                    chunkId++;
                }
            }
        }

        return true;
    }
};

class ChunkReceiver
{
    bool receiving;                                             // true if we are currently receiving a chunk.
    bool readyToRead;                                           // true if a chunk has been received and is ready for the caller to read.
    bool forceAckPreviousChunk;                                 // if this flag is set then we need to send a complete ack for the previous chunk id (sender has not yet received an ack with all slices received)
    int previousChunkNumSlices;                                 // number of slices in the previous chunk received. used for force ack of previous chunk.
    uint16_t chunkId;                                           // id of the chunk that is currently being received, or
    int chunkSize;                                              // the size of the chunk that has been received. only known once the last slice has been received!
    int numSlices;                                              // the number of slices in the current chunk being sent
    int numReceivedSlices;                                      // number of slices received for the current chunk. when num slices receive = num slices, the receive is complete.
    double timeLastAckSent;                                     // time last ack was sent. used to rate limit acks to some maximum number of acks per-second. 
    bool received[MaxSlicesPerChunk];                           // received flag for each slice of the chunk. chunk receive completes when all slices are received.
    uint8_t chunkData[MaxChunkSize];                            // chunk data being received.

public:

    ChunkReceiver()
    {
        memset( this, 0, sizeof( ChunkReceiver ) );
    }

    bool ProcessSlicePacket( SlicePacket *packet )
    {
        assert( packet );

        // caller has to ead the chunk out of the recieve buffer before we can receive the next one
        if ( readyToRead )
            return false;

        if ( !receiving && packet->chunkId == uint16_t( chunkId - 1 ) && previousChunkNumSlices != 0 )
        {
            // otherwise the sender gets stuck if the last ack packet is dropped due to packet loss
            forceAckPreviousChunk = true;
        }

        if ( !receiving && packet->chunkId == chunkId )
        {
            printf( "started receiving chunk %d\n", chunkId );

            assert( !readyToRead );
            
            receiving = true;
            forceAckPreviousChunk = false;
            numReceivedSlices = 0;
            chunkSize = 0;
            
            numSlices = packet->numSlices;
            assert( numSlices > 0 );
            assert( numSlices <= MaxSlicesPerChunk );

            memset( received, 0, sizeof( received ) );
        }

        if ( packet->chunkId != chunkId )
            return false;

        if ( packet->numSlices != numSlices )
            return false;

        assert( packet->sliceId >= 0 );
        assert( packet->sliceId <= numSlices );

        if ( !received[packet->sliceId] )
        {
            received[packet->sliceId] = true;

            assert( packet->sliceBytes > 0 );
            assert( packet->sliceBytes <= SliceSize );

            memcpy( chunkData + packet->sliceId * SliceSize, packet->data, packet->sliceBytes );

            numReceivedSlices++;

            assert( numReceivedSlices > 0 );
            assert( numReceivedSlices <= numSlices );

            printf( "received slice %d of chunk %d [%d/%d]\n", packet->sliceId, chunkId, numReceivedSlices, numSlices );

            if ( packet->sliceId == numSlices - 1 )
            {
                chunkSize = ( numSlices - 1 ) * SliceSize + packet->sliceBytes;
                printf( "received chunk size is %d\n", chunkSize );
            }

            if ( numReceivedSlices == numSlices )
            {
                printf( "received all slices for chunk %d\n", chunkId );
                receiving = false;
                readyToRead = true;
                previousChunkNumSlices = numSlices;
                chunkId++;
            }
        }

        return true;
    }

    AckPacket* GenerateAckPacket( double t )
    {
        if ( timeLastAckSent + MinimumTimeBetweenAcks > t )
            return NULL;

        if ( forceAckPreviousChunk && previousChunkNumSlices != 0 )
        {
            timeLastAckSent = t;
            forceAckPreviousChunk = false;

            AckPacket *packet = (AckPacket*) packetFactory.CreatePacket( ACK_PACKET );
            packet->chunkId = uint16_t( chunkId - 1 );
            packet->numSlices = previousChunkNumSlices;
            assert( previousChunkNumSlices > 0 );
            assert( previousChunkNumSlices <= MaxSlicesPerChunk );
            for ( int i = 0; i < previousChunkNumSlices; ++i )
                packet->acked[i] = true;

            return packet;
        }

        if ( receiving )
        {
            timeLastAckSent = t;

            AckPacket *packet = (AckPacket*) packetFactory.CreatePacket( ACK_PACKET );
            packet->chunkId = chunkId;
            packet->numSlices = numSlices;
            for ( int i = 0; i < numSlices; ++i )
                packet->acked[i] = received[i];

            return packet;
        }

        return NULL;
    }

    const uint8_t* ReadChunk( int & resultChunkSize )
    {
        if ( !readyToRead )
            return NULL;
        readyToRead = false;
        resultChunkSize = chunkSize;
        return chunkData;
    }
};

static network2::Simulator simulator;

void SendPacket( const network2::Address & from, const network2::Address & to, protocol2::Packet *packet )
{
    assert( packet );

    uint8_t *packetData = new uint8_t[MaxPacketSize];

    protocol2::PacketInfo info;
    info.protocolId = ProtocolId;
    info.packetFactory = &packetFactory;

    const int packetSize = protocol2::WritePacket( info, packet, packetData, MaxPacketSize );

    if ( packetSize > 0 )
    {
        simulator.SendPacket( from, to, packetData, packetSize );
    }
    else
    {
        delete [] packetData;
    }

    packetFactory.DestroyPacket( packet );
}

protocol2::Packet* ReceivePacket( network2::Address & from, network2::Address & to )
{
    int packetSize;
    uint8_t* packetData = simulator.ReceivePacket( from, to, packetSize );
    if ( !packetData )
        return NULL;

    protocol2::PacketInfo info;
    info.protocolId = ProtocolId;
    info.packetFactory = &packetFactory;

    int error = 0;
    protocol2::Packet *packet = protocol2::ReadPacket( info, packetData, packetSize, NULL, &error );
    if ( error != PROTOCOL2_ERROR_NONE )
        printf( "read packet error: %s\n", protocol2::GetErrorString( error ) );

    return packet;
}

inline int random_int( int a, int b )
{
    assert( a < b );
    int result = a + rand() % ( b - a + 1 );
    assert( result >= a );
    assert( result <= b );
    return result;
}

inline float random_float( float a, float b )
{
    assert( a < b );
    float random = ( (float) rand() ) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

int main()
{
    printf( "\nsending large blocks of data\n\n" );

    srand( (unsigned int) time( NULL ) );

    ChunkSender sender;
    ChunkReceiver receiver;

    network2::Address senderAddress( "::1", 20000 );
    network2::Address receiverAddress( "::1", 20001 );

    int numChunksSent = 0;
    bool sendingChunk = false;
    int sendChunkSize = 0;
    uint8_t sendChunkData[MaxChunkSize];

    double t = 0.0;
    double dt = 1.0 / 60.0;

    simulator.SetJitter( 100 );
    simulator.SetLatency( 250 );
    simulator.SetPacketLoss( 50 );
    simulator.SetDuplicates( 20 );

    while ( numChunksSent < NumChunksToSend || NumChunksToSend < 0 )
    {
        if ( !sendingChunk )
        {
            printf( "=======================================================\n" );
            sendChunkSize = random_int( 1, MaxChunkSize );
            for ( int i = 0; i < sendChunkSize; ++i )
                sendChunkData[i] = (uint8_t) random_int( 0, 255 );
            sender.SendChunk( sendChunkData, sendChunkSize );
            sendingChunk = true;
        }

        SlicePacket *slicePacket = sender.GenerateSlicePacket( t );
        if ( slicePacket )
            SendPacket( senderAddress, receiverAddress, slicePacket );

        AckPacket *ackPacket = receiver.GenerateAckPacket( t );
        if ( ackPacket )
            SendPacket( receiverAddress, senderAddress, ackPacket );

        while ( true )
        {
            network2::Address from, to;
            protocol2::Packet *packet = ReceivePacket( from, to );
            if ( !packet )
                break;

            switch ( packet->GetType() )
            {
                case SLICE_PACKET:
                {
                    if ( to == receiverAddress )
                    {
                        SlicePacket *p = (SlicePacket*) packet;
                        receiver.ProcessSlicePacket( p );
                    }
                }
                break;

                case ACK_PACKET:
                {
                    if ( to == senderAddress )
                    {
                        AckPacket *p = (AckPacket*) packet;
                        sender.ProcessAckPacket( p );
                    }
                }
                break;
            }

            packetFactory.DestroyPacket( packet );
        }

        simulator.Update( t );

        int chunkSize;
        const uint8_t *chunkData = receiver.ReadChunk( chunkSize );
        if ( chunkData )
        {
            if ( chunkSize != sendChunkSize )
            {
                printf( "chunk size mismatch: expected %d, got %d\n", sendChunkSize, chunkSize );
            }
            assert( chunkSize == sendChunkSize );
            assert( memcmp( chunkData, sendChunkData, chunkSize ) == 0 );
            printf( "chunk size and data match what was sent\n" );
        }

        if ( sendingChunk && !sender.IsSending() )
        {
            printf( "=======================================================\n\n" );
            sendingChunk = false;
            numChunksSent++;
        }
        
        t += dt;
    }

    return 0;
}
