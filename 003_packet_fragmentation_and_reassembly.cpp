/*
    Example source code for "Packet Fragmentation and Reassembly"

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

const int PacketBufferSize = 256;                       // size of packet buffer, eg. number of historical packets for which we can buffer fragments
const int MaxFragmentSize = 1024;                       // maximum size of a packet fragment
const int MaxFragmentsPerPacket = 256;                  // maximum number of fragments per-packet

const int MaxPacketSize = MaxFragmentSize * MaxFragmentsPerPacket;

//#define SOAK_TEST 1                // uncomment this line to loop forever and soak

#if SOAK_TEST
const int NumIterations = -1;
#else // #if SOAK_TEST
const int NumIterations = 32;
#endif // #if SOAK_TEST

const uint32_t ProtocolId = 0x55667788;

const int PacketFragmentHeaderBytes = 16;

enum TestPacketTypes
{
    PACKET_FRAGMENT = 0,                                // IMPORTANT: packet type 0 indicates a packet fragment

    TEST_PACKET_A,
    TEST_PACKET_B,
    TEST_PACKET_C,

    TEST_PACKET_NUM_TYPES
};

// fragment packet on-the-wire format:
// [crc32] (32 bits) | [sequence] (16 bits) | [packet type 0] (# of bits depends on number of packet types) 
// [fragment id] (8 bits) | [num fragments] (8 bits) | (pad zero bits to nearest byte) | <fragment data>

struct FragmentPacket : public protocol2::Object
{
    // input/output

    int fragmentSize;                   // set as input on serialize write. output on serialize read (inferred from size of packet)

    // serialized data

    uint32_t crc32;
    uint16_t sequence;
    int packetType;
    uint8_t fragmentId;
    uint8_t numFragments;
    uint8_t fragmentData[MaxFragmentSize];

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bits( stream, crc32, 32 );
        serialize_bits( stream, sequence, 16 );

        packetType = 0;
        serialize_int( stream, packetType, 0, TEST_PACKET_NUM_TYPES - 1 );
        if ( packetType != 0 )
            return true;

        serialize_bits( stream, fragmentId, 8 );
        serialize_bits( stream, numFragments, 8 );

        serialize_align( stream );

        if ( Stream::IsReading )
        {
            assert( ( stream.GetBitsRemaining() % 8 ) == 0 );
            fragmentSize = stream.GetBitsRemaining() / 8;
            if ( fragmentSize <= 0 || fragmentSize > MaxFragmentSize )
            {
                printf( "packet fragment size is out of bounds (%d)\n", fragmentSize );
                return false;
            }
        }

        assert( fragmentSize > 0 );
        assert( fragmentSize <= MaxFragmentSize );

        serialize_bytes( stream, fragmentData, fragmentSize );

        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct PacketData
{
    int size;
    uint8_t *data;
};

struct PacketBufferEntry
{
    uint32_t sequence : 16;                             // packet sequence number
    uint32_t numFragments : 8;                          // number of fragments for this packet
    uint32_t receivedFragments : 8;                     // number of received fragments so far
    int fragmentSize[MaxFragmentsPerPacket];            // size of fragment n in bytes
    uint8_t *fragmentData[MaxFragmentsPerPacket];       // pointer to data for fragment n 
};

struct PacketBuffer
{
    PacketBuffer() { memset( this, 0, sizeof( PacketBuffer ) ); }

    uint16_t currentSequence;                           // sequence number of most recent packet in buffer

    int numBufferedFragments;                           // total number of fragments stored in the packet buffer (across *all* packets)

    bool valid[PacketBufferSize];                       // true if there is a valid buffered packet entry at this index

    PacketBufferEntry entries[PacketBufferSize];        // buffered packets in range [ current_sequence - PacketBufferSize + 1, current_sequence ] (modulo 65536)

    /*
        Advance the current sequence for the packet buffer forward.
        This function removes old packet entries and frees their fragments.
    */

    void Advance( uint16_t sequence )
    {
        if ( !protocol2::sequence_greater_than( sequence, currentSequence ) )
            return;

        const uint16_t oldestSequence = sequence - PacketBufferSize + 1;

        for ( int i = 0; i < PacketBufferSize; ++i )
        {
            if ( valid[i] )
            {
                if ( protocol2::sequence_less_than( entries[i].sequence, oldestSequence ) )
                {
                    printf( "remove old packet entry %d\n", entries[i].sequence );

                    for ( int j = 0; j < (int) entries[i].numFragments; ++j )
                    {
                        if ( entries[i].fragmentData[j] )
                        {
                            delete [] entries[i].fragmentData[j];
                            assert( numBufferedFragments > 0 );
                            numBufferedFragments--;
                        }
                    }
                }

                memset( &entries[i], 0, sizeof( PacketBufferEntry ) );

                valid[i] = false;
            }
        }

        currentSequence = sequence;
    }

    /*
        Process packet fragment on receiver side.

        Stores each fragment ready to receive the whole packet once all fragments for that packet are received.

        If any fragment is dropped, fragments are not resent, the whole packet is dropped.

        NOTE: This function is fairly complicated because it must handle all possible cases
        of maliciously constructed packets attempting to overflow and corrupt the packet buffer!
    */

    bool ProcessFragment( const uint8_t *fragmentData, int fragmentSize, uint16_t packetSequence, int fragmentId, int numFragmentsInPacket )
    {
        assert( fragmentData );

        // fragment size is <= zero? discard the fragment.

        if ( fragmentSize <= 0 )
            return false;

        // fragment size exceeds max fragment size? discard the fragment.

        if ( fragmentSize > MaxFragmentSize )
            return false;

        // num fragments outside of range? discard the fragment

        if ( numFragmentsInPacket <= 0 || numFragmentsInPacket > MaxFragmentsPerPacket )
            return false;

        // fragment index out of range? discard the fragment

        if ( fragmentId < 0 || fragmentId >= numFragmentsInPacket )
            return false;

        // if this is not the last fragment in the packet and fragment size is not equal to MaxFragmentSize, discard the fragment

        if ( fragmentId != numFragmentsInPacket - 1 && fragmentSize != MaxFragmentSize )
            return false;

        // packet sequence number wildly out of range from the current sequence? discard the fragment

        if ( protocol2::sequence_difference( packetSequence, currentSequence ) > 1024 )
            return false;

        // if the entry exists, but has a different sequence number, discard the fragment

        const int index = packetSequence % PacketBufferSize;

        if ( valid[index] && entries[index].sequence != packetSequence )
            return false;

        // if the entry does not exist, add an entry for this sequence # and set total fragments

        if ( !valid[index] )
        {
            Advance( packetSequence );
            entries[index].sequence = packetSequence;
            entries[index].numFragments = numFragmentsInPacket;
            assert( entries[index].receivedFragments == 0 );            // IMPORTANT: Should have already been cleared to zeros in "Advance"
            valid[index] = true;
        }

        // at this point the entry must exist and have the same sequence number as the fragment

        assert( valid[index] );
        assert( entries[index].sequence == packetSequence );

        // if the total number fragments is different for this packet vs. the entry, discard the fragment

        if ( numFragmentsInPacket != (int) entries[index].numFragments )
            return false;

        // if this fragment has already been received, ignore it because it must have come from a duplicate packet

        assert( fragmentId < numFragmentsInPacket );
        assert( fragmentId < MaxFragmentsPerPacket );
        assert( numFragmentsInPacket <= MaxFragmentsPerPacket );

        if ( entries[index].fragmentSize[fragmentId] )
            return false;

        // add the fragment to the packet buffer

        printf( "added fragment %d of packet %d to buffer\n", fragmentId, packetSequence );

        assert( fragmentSize > 0 );
        assert( fragmentSize <= MaxFragmentSize );

        entries[index].fragmentSize[fragmentId] = fragmentSize;
        entries[index].fragmentData[fragmentId] = new uint8_t[fragmentSize];
        memcpy( entries[index].fragmentData[fragmentId], fragmentData, fragmentSize );
        entries[index].receivedFragments++;

        assert( entries[index].receivedFragments <= entries[index].numFragments );

        numBufferedFragments++;

        return true;
    }

    bool ProcessPacket( const uint8_t *data, int size )
    {
        protocol2::ReadStream stream( data, size );

        FragmentPacket fragmentPacket;
        
        if ( !fragmentPacket.SerializeRead( stream ) )
        {
            printf( "error: fragment packet failed to serialize\n" );
            return false;
        }

        uint32_t protocolId = protocol2::host_to_network( ProtocolId );
        uint32_t crc32 = protocol2::calculate_crc32( (const uint8_t*) &protocolId, 4 );
        uint32_t zero = 0;
        crc32 = protocol2::calculate_crc32( (const uint8_t*) &zero, 4, crc32 );
        crc32 = protocol2::calculate_crc32( data + 4, size - 4, crc32 );

        if ( crc32 != fragmentPacket.crc32 )
        {
            printf( "corrupt packet: expected crc32 %x, got %x\n", crc32, fragmentPacket.crc32 );
            return false;
        }

        if ( fragmentPacket.packetType == 0 )
        {
            return ProcessFragment( data + PacketFragmentHeaderBytes, fragmentPacket.fragmentSize, fragmentPacket.sequence, fragmentPacket.fragmentId, fragmentPacket.numFragments );
        }
        else
        {
            return ProcessFragment( data, size, fragmentPacket.sequence, 0, 1 );
        }

        return true;
    }

    void ReceivePackets( int & numPackets, PacketData packetData[] )
    {
        numPackets = 0;

        const uint16_t oldestSequence = currentSequence - PacketBufferSize + 1;

        for ( int i = 0; i < PacketBufferSize; ++i )
        {
            const uint16_t sequence = uint16_t( ( oldestSequence + i ) & 0xFF );

            const int index = sequence % PacketBufferSize;

            if ( valid[index] && entries[index].sequence == sequence )
            {
                // have all fragments arrived for this packet?

                if ( entries[index].receivedFragments != entries[index].numFragments )
                    continue;

                printf( "received all fragments for packet %d [%d]\n", sequence, entries[index].numFragments );

                // what's the total size of this packet?

                int packetSize = 0;
                for ( int j = 0; j < (int) entries[index].numFragments; ++j )
                {
                    packetSize += entries[index].fragmentSize[j];
                }

                assert( packetSize > 0 );
                assert( packetSize <= MaxPacketSize );

                // allocate a packet to return to the caller

                PacketData & packet = packetData[numPackets++];

                packet.size = packetSize;
                packet.data = new uint8_t[packetSize];

                // reconstruct the packet from the fragments

                printf( "reassembling packet %d from fragments (%d bytes)\n", sequence, packetSize );

                uint8_t *dst = packet.data;
                for ( int j = 0; j < (int) entries[index].numFragments; ++j )
                {
                    memcpy( dst, entries[index].fragmentData[i], entries[index].fragmentSize[i] );
                    dst += entries[index].fragmentSize[i];
                }

                // free all fragments

                for ( int j = 0; j < (int) entries[index].numFragments; ++j )
                {
                    delete [] entries[index].fragmentData[j];
                    numBufferedFragments--;
                }

                // clear the packet buffer entry

                memset( &entries[index], 0, sizeof( PacketBufferEntry ) );

                valid[index] = false;
            }
        }
    }
};

bool SplitPacketIntoFragments( uint16_t sequence, const uint8_t *packetData, int packetSize, int & numFragments, PacketData fragmentPackets[] )
{
    numFragments = 0;

    assert( packetData );
    assert( packetSize > 0 );
    assert( packetSize < MaxPacketSize );

    numFragments = ( packetSize / MaxFragmentSize ) + ( ( packetSize % MaxFragmentSize ) != 0 ? 1 : 0 );

    assert( numFragments > 0 );
    assert( numFragments <= MaxFragmentsPerPacket );

    const uint8_t *src = packetData;

    printf( "splitting packet into %d fragments\n", numFragments );

    for ( int i = 0; i < numFragments; ++i )
    {
        const int fragmentSize = ( i == numFragments - 1 ) ? ( (int) ( intptr_t( packetData + packetSize ) - intptr_t( src ) ) ) : MaxFragmentSize;

        static const int MaxFragmentPacketSize = MaxFragmentSize + PacketFragmentHeaderBytes;

        fragmentPackets[i].data = new uint8_t[MaxFragmentPacketSize];

        protocol2::WriteStream stream( fragmentPackets[i].data, MaxFragmentPacketSize );

        FragmentPacket fragmentPacket;
        fragmentPacket.fragmentSize = fragmentSize;
        fragmentPacket.crc32 = 0;
        fragmentPacket.sequence = sequence;
        fragmentPacket.fragmentId = (uint8_t) i;
        fragmentPacket.numFragments = (uint8_t) numFragments;
        memcpy( fragmentPacket.fragmentData, src, fragmentSize );

        if ( !fragmentPacket.SerializeWrite( stream ) )
        {
            numFragments = 0;
            for ( int j = 0; j < i; ++j )
            {
                delete fragmentPackets[i].data;
                fragmentPackets[i].data = NULL;
                fragmentPackets[i].size = 0;
            }
            return false;
        }

        stream.Flush();

        uint32_t protocolId = protocol2::host_to_network( ProtocolId );
        uint32_t crc32 = protocol2::calculate_crc32( (uint8_t*) &protocolId, 4 );
        crc32 = protocol2::calculate_crc32( fragmentPackets[i].data, stream.GetBytesProcessed(), crc32 );

        *((uint32_t*)fragmentPackets[i].data) = protocol2::host_to_network( crc32 );

        printf( "fragment packet %d: %d bytes\n", i, stream.GetBytesProcessed() );

        fragmentPackets[i].size = stream.GetBytesProcessed();

        src += fragmentSize;
    }

    assert( src == packetData + packetSize );

    return true;
}

static PacketBuffer packetBuffer;

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

static const int MaxItems = 4096 * 4;

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

struct TestPacketHeader : public protocol2::Object
{
    uint16_t sequence;

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bits( stream, sequence, 16 );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();

    bool operator == ( const TestPacketHeader & other )
    {
        return sequence == other.sequence;
    }

    bool operator != ( const TestPacketHeader & other )
    {
        return !(*this == other );
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

bool CheckPacketsAreIdentical( protocol2::Packet *p1, protocol2::Packet *p2, TestPacketHeader & h1, TestPacketHeader & h2 )
{
    assert( p1 );
    assert( p2 );

    if ( h1 != h2 )
        return false;

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

int main()
{
    printf( "\npacket fragmentation and reassembly\n\n" );

    srand( (unsigned int) time( NULL ) );

    TestPacketFactory packetFactory;

    uint16_t sequence = 0;

    for ( int i = 0; ( i < NumIterations || NumIterations == -1 ); ++i )
    {
        const int packetType = 1 + rand() % ( TEST_PACKET_NUM_TYPES - 1 );              // because packet type 0 indicate a packet fragment

        protocol2::Packet *writePacket = packetFactory.CreatePacket( packetType );

        assert( writePacket );
        assert( writePacket->GetType() == packetType );

        uint8_t buffer[MaxPacketSize];

        bool error = false;

        TestPacketHeader writePacketHeader;
        writePacketHeader.sequence = sequence;

        protocol2::PacketInfo info;
        info.protocolId = ProtocolId;
        info.packetFactory = &packetFactory;

        const int bytesWritten = protocol2::WritePacket( info, writePacket, buffer, MaxPacketSize, &writePacketHeader );

        printf( "===================================================\n" );

        printf( "writing packet %d\n", sequence );

        if ( bytesWritten > 0 )
        {
            printf( "wrote packet type %d (%d bytes)\n", writePacket->GetType(), bytesWritten );
        }
        else
        {
            printf( "write packet error\n" );

            error = true;
        }

        if ( bytesWritten > MaxFragmentSize )
        {
            int numFragments;
            PacketData fragmentPackets[MaxFragmentsPerPacket];
            SplitPacketIntoFragments( sequence, buffer, bytesWritten, numFragments, fragmentPackets );

            for ( int j = 0; j < numFragments; ++j )
                packetBuffer.ProcessPacket( fragmentPackets[j].data, fragmentPackets[j].size );
        }
        else
        {
            printf( "sending packet %d as a regular packet\n", sequence );

            packetBuffer.ProcessPacket( buffer, bytesWritten );
        }

        int numPackets = 0;
        PacketData packets[PacketBufferSize];
        packetBuffer.ReceivePackets( numPackets, packets );

        for ( int j = 0; j < numPackets; ++j )
        {
            int readError;
            TestPacketHeader readPacketHeader;
            protocol2::Packet *readPacket = protocol2::ReadPacket( info, buffer, bytesWritten, &readPacketHeader, &readError );

            if ( readPacket )
            {
                printf( "read packet type %d (%d bytes)\n", readPacket->GetType(), bytesWritten );

                if ( !CheckPacketsAreIdentical( readPacket, writePacket, readPacketHeader, writePacketHeader ) )
                {
                    printf( "failure: read packet is not the same as written packet. something wrong with serialize function?\n" );
                    error = true;
                }
                else
                {
                    printf( "success: read packet %d matches written packet %d\n", readPacketHeader.sequence, writePacketHeader.sequence );
                }
            }
            else
            {
                printf( "read packet error: %s\n", protocol2::GetErrorString( readError ) );

                error = true;
            }

            packetFactory.DestroyPacket( readPacket );

            if ( error )
                break;
    
            printf( "===================================================\n" );
        }

        packetFactory.DestroyPacket( writePacket );

        if ( error )
            return 1;

        sequence++;

        printf( "\n" );
    }

    return 0;
}
