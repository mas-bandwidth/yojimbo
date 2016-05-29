/*
    Example source code for "Packet Aggregation"

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

#define PROTOCOL2_IMPLEMENTATION

#include "protocol2.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#define SOAK_TEST 1                // uncomment this line to loop forever and soak

#if !SOAK_TEST
const int NumIterations = 16;
#endif // #if !SOAK_TEST

const int MaxPacketsPerIteration = 8;

const uint32_t MaxPacketSize = 1024;

const uint32_t ProtocolId = 0x22446688;

enum TestPacketTypes
{
    TEST_PACKET_A,
    TEST_PACKET_B,
    TEST_PACKET_C,
    TEST_PACKET_NUM_TYPES
};

struct Vector
{
    float x,y,z;
};

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

struct TestPacketA : public protocol2::Packet
{
    int a,b,c;

    TestPacketA() : Packet( TEST_PACKET_A )
    {
        a = random_int( -10, +10 );
        b = random_int( -20, +20 );
        c = random_int( -30, +30 );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, a, -10, 10 );
        serialize_int( stream, b, -20, 20 );
        serialize_int( stream, c, -30, 30 );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();

    bool operator == ( const TestPacketA & other ) const
    {
        return a == other.a && b == other.b && c == other.c;
    }

    bool operator != ( const TestPacketA & other ) const
    {
        return ! ( *this == other );
    }
};

static const int MaxItems = 16;

struct TestPacketB : public protocol2::Packet
{
    int numItems;
    int items[MaxItems];

    TestPacketB() : Packet( TEST_PACKET_B )
    {
        numItems = random_int( 0, MaxItems );
        for ( int i = 0; i < numItems; ++i )
            items[i] = random_int( -100, +100 );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_int( stream, numItems, 0, MaxItems );
        for ( int i = 0; i < numItems; ++i )
            serialize_int( stream, items[i], -100, +100 );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();

    bool operator == ( const TestPacketB & other ) const
    {
        if ( numItems != other.numItems )
            return false;
        for ( int i = 0; i < numItems; ++i )
        {
            if ( items[i] != other.items[i] )
                return false;
        }
        return true;
    }

    bool operator != ( const TestPacketB & other ) const
    {
        return ! ( *this == other );
    }
};

struct TestPacketC : public protocol2::Packet
{
    Vector position;
    Vector velocity;

    TestPacketC() : Packet( TEST_PACKET_C )
    {
        position.x = random_float( -1000, +1000 );
        position.y = random_float( -1000, +1000 );
        position.z = random_float( -1000, +1000 );

        if ( rand() % 2 )
        {
            velocity.x = random_float( -100, +100 );
            velocity.y = random_float( -100, +100 );
            velocity.z = random_float( -100, +100 );
        }
        else
        {
            velocity.x = 0.0f;
            velocity.y = 0.0f;
            velocity.z = 0.0f;
        }
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_float( stream, position.x );
        serialize_float( stream, position.y );
        serialize_float( stream, position.z );

        bool at_rest = Stream::IsWriting && velocity.x == 0.0f && velocity.y == 0.0f && velocity.z == 0.0f;

        serialize_bool( stream, at_rest );

        if ( !at_rest )
        {
            serialize_float( stream, velocity.x );
            serialize_float( stream, velocity.y );
            serialize_float( stream, velocity.z );
        }
        else
        {
            if ( Stream::IsReading )
            {
                velocity.x = 0.0f;
                velocity.y = 0.0f;
                velocity.z = 0.0f;
            }
        }

        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();

    bool operator == ( const TestPacketC & other )
    {
        return position.x == other.position.x &&
               position.y == other.position.y &&
               position.z == other.position.z &&
               velocity.x == other.velocity.x &&
               velocity.y == other.velocity.y &&
               velocity.z == other.velocity.z;
    }
};

struct TestPacketFactory : public protocol2::PacketFactory
{
    TestPacketFactory() : PacketFactory( TEST_PACKET_NUM_TYPES ) {}

    protocol2::Packet* Create( int type )
    {
        switch ( type )
        {
            case TEST_PACKET_A: return new TestPacketA();
            case TEST_PACKET_B: return new TestPacketB();
            case TEST_PACKET_C: return new TestPacketC();
        }
        return NULL;
    }

    void Destroy( protocol2::Packet *packet )
    {
        delete packet;
    }
};

bool CheckPacketsAreIdentical( protocol2::Packet *p1, protocol2::Packet *p2 )
{
    assert( p1 );
    assert( p2 );

    if ( p1->GetType() != p2->GetType() )
        return false;

    switch ( p1->GetType() )
    {
        case TEST_PACKET_A:     return *((TestPacketA*)p1) == *((TestPacketA*)p2);
        case TEST_PACKET_B:     return *((TestPacketB*)p1) == *((TestPacketB*)p2);
        case TEST_PACKET_C:     return *((TestPacketC*)p1) == *((TestPacketC*)p2);
        default:
            return false;
    }
}

struct TestPacketHeader : public protocol2::Object
{
    uint16_t sequence;

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bits( stream, sequence, 16 );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

int main()
{
    srand( (unsigned int) time( NULL ) );

    TestPacketFactory packetFactory;

    uint16_t sequence = 0;

    TestPacketHeader *readPacketHeaders[MaxPacketsPerIteration];
    TestPacketHeader *writePacketHeaders[MaxPacketsPerIteration];

    for ( int i = 0; i < MaxPacketsPerIteration; ++i )
    {
        readPacketHeaders[i] = new TestPacketHeader();
        writePacketHeaders[i] = new TestPacketHeader();
    }

#if !SOAK_TEST
    for ( int i = 0; i < NumIterations; ++i )
#else // #if !SOAK_TEST
    for ( uint32_t i = 0; ; ++i )
#endif // #if !SOAK_TEST
    {
        int numReadPackets = 0;
        int numWritePackets = 0;
        
        protocol2::Packet *readPackets[MaxPacketsPerIteration];
        protocol2::Packet *writePackets[MaxPacketsPerIteration];

        printf( "==============================================================\n" );
        printf( "iteration %d\n", i );

        // create an array of different packets (may be zero length)

        numWritePackets = random_int( 0, MaxPacketsPerIteration );

        printf( "creating %d packets\n", numWritePackets );

        for ( int j = 0; j < numWritePackets; ++j )
        {
            const int packetType = rand() % TEST_PACKET_NUM_TYPES;

            printf( "%d: created packet %d [%d]\n", j, sequence, packetType );

            writePackets[j] = packetFactory.CreatePacket( packetType );

            assert( writePackets[j] );

            writePacketHeaders[j]->sequence = sequence++;
        }

        // combine packets together into one aggregate on-the-wire packet

        uint8_t writeBuffer[MaxPacketSize];

        int numPacketsActuallyWritten = 0;

        protocol2::PacketInfo info;
        info.protocolId = ProtocolId;
        info.packetFactory = &packetFactory;

        const int bytesWritten = protocol2::WriteAggregatePacket( info, 
                                                                  numWritePackets,
                                                                  writePackets, 
                                                                  writeBuffer, 
                                                                  MaxPacketSize, 
                                                                  numPacketsActuallyWritten,
                                                                  NULL,
                                                                  (protocol2::Object**) writePacketHeaders );

        bool error = false;

        if ( bytesWritten > 0 )
        {
            printf( "wrote aggregate packet (%d bytes)\n", bytesWritten );

            assert( numPacketsActuallyWritten == numWritePackets );
        }
        else
        {
            printf( "write aggregate packet failed\n" );
            
            error = true;

            goto cleanup;
        }

        // read individual packets from the aggregate on-the-wire packet

        {
            int bytesToRead = bytesWritten;

            uint8_t readBuffer[MaxPacketSize];

            memset( readBuffer, 0, MaxPacketSize );
            memcpy( readBuffer, writeBuffer, bytesWritten );

            int readError = 0;

            printf( "reading aggregate packet (%d bytes)\n", bytesToRead );

            info.protocolId = ProtocolId;
            info.packetFactory = &packetFactory;

            ReadAggregatePacket( info, MaxPacketsPerIteration, readPackets, readBuffer, bytesWritten, numReadPackets, NULL, (protocol2::Object**) readPacketHeaders, &readError );

            if ( readError != PROTOCOL2_ERROR_NONE )
            {
                printf( "read packet error: %s\n", protocol2::GetErrorString( readError ) );
                error = true;
                goto cleanup;
            }

            printf( "num packets read: %d\n", numReadPackets );

            // verify that packets read from the aggregate packet exactly match the packets written to it

            assert( numReadPackets == numWritePackets );

            for ( int j = 0; j < numReadPackets; ++j )
            {
                assert( readPackets[j] );

                printf( "%d: read packet %d [%d]\n", j, readPacketHeaders[j]->sequence, readPackets[j]->GetType() );

                if ( readPacketHeaders[j]->sequence != writePacketHeaders[j]->sequence )
                {
                    printf( "read packet header is not the same as written packet header. something wrong with serialize function?\n" );
                    error = true;
                    goto cleanup;
                }

                if ( !CheckPacketsAreIdentical( readPackets[j], writePackets[j] ) )
                {
                    printf( "read packet is not the same as written packet. something wrong with serialize function?\n" );
                    error = true;
                    goto cleanup;
                }
            }

            if ( numReadPackets > 0 )
                printf( "read packets match written packets\n" );
        }

cleanup:

        // clean up packets for this iteration

        for ( int j = 0; j < numWritePackets; ++j )
        {
            packetFactory.DestroyPacket( writePackets[j] );
        }

        for ( int j = 0; j < numReadPackets; ++j )
        {
            packetFactory.DestroyPacket( readPackets[j] );
        }

        printf( "==============================================================\n\n" );

        // has there been an error? stop.

        if ( error )
            break;
    }

    for ( int i = 0; i < MaxPacketsPerIteration; ++i )
    {
        delete readPacketHeaders[i];
        delete writePacketHeaders[i];
    }

    return 0;
}
