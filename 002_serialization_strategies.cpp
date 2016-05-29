/*
    Example source code for "Serialization Strategies"

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

#include "vectorial/vec3f.h"
#include "vectorial/vec4f.h"
#include "vectorial/quat4f.h"
#include "vectorial/mat4f.h"

using namespace vectorial;

//#define SOAK_TEST 1                // uncomment this line to loop forever and soak

#if SOAK_TEST
const int NumIterations = -1;
#else // #if SOAK_TEST
const int NumIterations = 16;
#endif // #if SOAK_TEST

const uint32_t MaxPacketSize = 256 * 1024;

const uint32_t ProtocolId = 0x44551177;

template <typename Stream> bool serialize_vector_internal( Stream & stream, vec3f & vector )
{
    float values[3];
    if ( Stream::IsWriting )
        vector.store( values );
    serialize_float( stream, values[0] );
    serialize_float( stream, values[1] );
    serialize_float( stream, values[2] );
    if ( Stream::IsReading )
        vector.load( values );
    return true;
}

#define serialize_vector( stream, value )                                       \
    do                                                                          \
    {                                                                           \
        if ( !serialize_vector_internal( stream, value ) )                      \
            return false;                                                       \
    } while ( 0 )

template <typename Stream> bool serialize_quaternion_internal( Stream & stream, quat4f & quaternion )
{
    float values[4];
    if ( Stream::IsWriting )
        quaternion.store( values );
    serialize_float( stream, values[0] );
    serialize_float( stream, values[1] );
    serialize_float( stream, values[2] );
    serialize_float( stream, values[3] );
    if ( Stream::IsReading )
        quaternion.load( values );
    return true;
}

#define serialize_quaternion( stream, value )                                   \
    do                                                                          \
    {                                                                           \
        if ( !serialize_quaternion_internal( stream, value ) )                  \
            return false;                                                       \
    } while ( 0 )

template <typename Stream> bool serialize_compressed_float_internal( Stream & stream, float & value, float min, float max, float res )
{
    const float delta = max - min;
    const float values = delta / res;
    const uint32_t maxIntegerValue = (uint32_t) ceil( values );
    const int bits = protocol2::bits_required( 0, maxIntegerValue );
    
    uint32_t integerValue = 0;
    
    if ( Stream::IsWriting )
    {
        float normalizedValue = protocol2::clamp( ( value - min ) / delta, 0.0f, 1.0f );
        integerValue = (uint32_t) floor( normalizedValue * maxIntegerValue + 0.5f );
    }
    
    if ( !stream.SerializeBits( integerValue, bits ) )
        return false;

    if ( Stream::IsReading )
    {
        const float normalizedValue = integerValue / float( maxIntegerValue );
        value = normalizedValue * delta + min;
    }

    return true;
}

#define serialize_compressed_float( stream, value, min, max, res )                              \
do                                                                                              \
{                                                                                               \
    if ( !serialize_compressed_float_internal( stream, value, min, max, res ) )                 \
        return false;                                                                           \
}                                                                                               \
while(0)

template <typename Stream> bool serialize_compressed_vector_internal( Stream & stream, vec3f & vector, float min, float max, float res )
{
    float values[3];
    if ( Stream::IsWriting )
        vector.store( values );
    serialize_compressed_float( stream, values[0], min, max, res );
    serialize_compressed_float( stream, values[1], min, max, res );
    serialize_compressed_float( stream, values[2], min, max, res );
    if ( Stream::IsReading )
        vector.load( values );
    return true;
}

#define serialize_compressed_vector( stream, value, min, max, res )                             \
do                                                                                              \
{                                                                                               \
    if ( !serialize_compressed_vector_internal( stream, value, min, max, res ) )                \
        return false;                                                                           \
}                                                                                               \
while(0)

template <int bits> struct compressed_quaternion
{
    enum { max_value = (1<<bits)-1 };

    uint32_t largest : 2;
    uint32_t integer_a : bits;
    uint32_t integer_b : bits;
    uint32_t integer_c : bits;

    void Load( float x, float y, float z, float w )
    {
        assert( bits > 1 );
        assert( bits <= 10 );

        const float minimum = - 1.0f / 1.414214f;       // 1.0f / sqrt(2)
        const float maximum = + 1.0f / 1.414214f;

        const float scale = float( ( 1 << bits ) - 1 );

        const float abs_x = fabs( x );
        const float abs_y = fabs( y );
        const float abs_z = fabs( z );
        const float abs_w = fabs( w );

        largest = 0;
        float largest_value = abs_x;

        if ( abs_y > largest_value )
        {
            largest = 1;
            largest_value = abs_y;
        }

        if ( abs_z > largest_value )
        {
            largest = 2;
            largest_value = abs_z;
        }

        if ( abs_w > largest_value )
        {
            largest = 3;
            largest_value = abs_w;
        }

        float a = 0;
        float b = 0;
        float c = 0;

        switch ( largest )
        {
            case 0:
                if ( x >= 0 )
                {
                    a = y;
                    b = z;
                    c = w;
                }
                else
                {
                    a = -y;
                    b = -z;
                    c = -w;
                }
                break;

            case 1:
                if ( y >= 0 )
                {
                    a = x;
                    b = z;
                    c = w;
                }
                else
                {
                    a = -x;
                    b = -z;
                    c = -w;
                }
                break;

            case 2:
                if ( z >= 0 )
                {
                    a = x;
                    b = y;
                    c = w;
                }
                else
                {
                    a = -x;
                    b = -y;
                    c = -w;
                }
                break;

            case 3:
                if ( w >= 0 )
                {
                    a = x;
                    b = y;
                    c = z;
                }
                else
                {
                    a = -x;
                    b = -y;
                    c = -z;
                }
                break;

            default:
                assert( false );
        }

        const float normal_a = ( a - minimum ) / ( maximum - minimum ); 
        const float normal_b = ( b - minimum ) / ( maximum - minimum );
        const float normal_c = ( c - minimum ) / ( maximum - minimum );

        integer_a = floor( normal_a * scale + 0.5f );
        integer_b = floor( normal_b * scale + 0.5f );
        integer_c = floor( normal_c * scale + 0.5f );
    }

    void Save( float & x, float & y, float & z, float & w ) const
    {
        assert( bits > 1 );
        assert( bits <= 10 );

        const float minimum = - 1.0f / 1.414214f;       // 1.0f / sqrt(2)
        const float maximum = + 1.0f / 1.414214f;

        const float scale = float( ( 1 << bits ) - 1 );

        const float inverse_scale = 1.0f / scale;

        const float a = integer_a * inverse_scale * ( maximum - minimum ) + minimum;
        const float b = integer_b * inverse_scale * ( maximum - minimum ) + minimum;
        const float c = integer_c * inverse_scale * ( maximum - minimum ) + minimum;

        switch ( largest )
        {
            case 0:
            {
                x = sqrtf( 1 - a*a - b*b - c*c );
                y = a;
                z = b;
                w = c;
            }
            break;

            case 1:
            {
                x = a;
                y = sqrtf( 1 - a*a - b*b - c*c );
                z = b;
                w = c;
            }
            break;

            case 2:
            {
                x = a;
                y = b;
                z = sqrtf( 1 - a*a - b*b - c*c );
                w = c;
            }
            break;

            case 3:
            {
                x = a;
                y = b;
                z = c;
                w = sqrtf( 1 - a*a - b*b - c*c );
            }
            break;

            default:
            {
                assert( false );
                x = 0;
                y = 0;
                z = 0;
                w = 1;
            }
        }
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bits( stream, largest, 2 );
        serialize_bits( stream, integer_a, bits );
        serialize_bits( stream, integer_b, bits );
        serialize_bits( stream, integer_c, bits );
        return true;
    }
};

template <typename Stream> bool serialize_compressed_quaternion_internal( Stream & stream, quat4f & quat )
{
    compressed_quaternion<10> compressed_quat;
    
    if ( Stream::IsWriting )
        compressed_quat.Load( quat.x(), quat.y(), quat.z(), quat.w() );
    
    serialize_object( stream, compressed_quat );

    if ( Stream::IsReading )
    {
        float x,y,z,w;
        compressed_quat.Save( x, y, z, w );
        quat = normalize( quat4f( x, y, z, w ) );
    }

    return true;
}

#define serialize_compressed_quaternion( stream, value )                        \
    do                                                                          \
    {                                                                           \
        if ( !serialize_compressed_quaternion_internal( stream, value ) )       \
            return false;                                                       \
    } while ( 0 )

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

const float PositionMin = -1000.0f;
const float PositionMax = +1000.0f;
const float PositionRes = 0.001f;

const float LinearVelocityMin = -10.0f;
const float LinearVelocityMax = +10.0f;
const float LinearVelocityRes = 0.01f;

const float AngularVelocityMin = -20.0f;
const float AngularVelocityMax = +20.0f;
const float AngularVelocityRes = 0.01f;

inline vec3f random_position()
{
    return vec3f( random_float( PositionMin, PositionMax ), 
                  random_float( PositionMin, PositionMax ), 
                  random_float( PositionMin, PositionMax ) );
}

inline float random_angle()
{
    const float pi = 3.1415926;
    return random_float( 0, 2*pi );
}

inline vec3f random_axis()
{
    while ( true )
    {
        const float v1 = random_float( -1, +1 );
        const float v2 = random_float( -1, +1 );

        const float s = v1*v1 + v2*v2;

        if ( s >= 1 )
            continue;

        return vec3f( 2 * v1 * (float) sqrt( 1 - s ), 
                      2 * v2 * (float) sqrt( 1 - s ), 
                      1 - 2 * s );
    }
}

inline quat4f random_orientation()
{
    const vec3f axis = random_axis();
    const float angle = random_angle();
    return quat4f::axis_rotation( angle, axis );
}

inline vec3f random_linear_velocity()
{
    return vec3f( random_float( LinearVelocityMin, LinearVelocityMax ), 
                  random_float( LinearVelocityMin, LinearVelocityMax ), 
                  random_float( LinearVelocityMin, LinearVelocityMax ) );
}

inline vec3f random_angular_velocity()
{
    return vec3f( random_float( AngularVelocityMin, AngularVelocityMax ), 
                  random_float( AngularVelocityMin, AngularVelocityMax ), 
                  random_float( AngularVelocityMin, AngularVelocityMax ) );
}

#define serialize_position( stream, value )             serialize_compressed_vector( stream, value, PositionMin, PositionMax, PositionRes );
#define serialize_orientation( stream, value )          serialize_compressed_quaternion( stream, value );
#define serialize_linear_velocity( stream, value )      serialize_compressed_vector( stream, value, LinearVelocityMin, LinearVelocityMax, LinearVelocityRes );
#define serialize_angular_velocity( stream, value )     serialize_compressed_vector( stream, value, AngularVelocityMin, AngularVelocityMax, AngularVelocityRes );

struct Object
{
    bool send;
    vec3f position;
    quat4f orientation;
    vec3f linear_velocity;
    vec3f angular_velocity;

    void Randomize()
    {
		send = random_int( 0, 1 ) ? true : false;

        if ( send )
        {
            position = random_position();
            orientation = random_orientation();
            linear_velocity = random_linear_velocity();
            angular_velocity = random_angular_velocity();
        }
        else
        {
            memset( this, 0, sizeof( Object ) );
        }
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_position( stream, position );

        serialize_orientation( stream, orientation );

        bool at_rest = Stream::IsWriting ? ( length( linear_velocity ) == 0.0f && length( angular_velocity ) == 0.0f ) : false;

        serialize_bool( stream, at_rest );

        if ( !at_rest )
        {
            serialize_linear_velocity( stream, linear_velocity );

            serialize_angular_velocity( stream, angular_velocity );
        }

        return true;
    }
};

const int MaxObjects = 1024;

struct Scene
{
    Object objects[MaxObjects];
};

void RandomizeScene( Scene & scene )
{
    for ( int i = 0; i < MaxObjects; ++i )
        scene.objects[i].Randomize();
}

template <typename Stream> bool serialize_scene_a( Stream & stream, Scene & scene )
{
    for ( int i = 0; i < MaxObjects; ++i )
    {
        serialize_bool( stream, scene.objects[i].send );
        
        if ( !scene.objects[i].send )
        {
            if ( Stream::IsReading )
                memset( &scene.objects[i], 0, sizeof( Object ) );
            continue;
        }

        serialize_object( stream, scene.objects[i] );
    }

    return true;
}

bool write_scene_b( protocol2::WriteStream & stream, Scene & scene )
{
    int num_objects_sent = 0;

    for ( int i = 0; i < MaxObjects; ++i )
    {
        if ( scene.objects[i].send )
            num_objects_sent++;
    }

    write_int( stream, num_objects_sent, 0, MaxObjects );

    for ( int i = 0; i < MaxObjects; ++i )
    {
        if ( !scene.objects[i].send )
            continue;

        write_int( stream, i, 0, MaxObjects - 1 );
        
        write_object( stream, scene.objects[i] );
    }

    return true;
}

bool read_scene_b( protocol2::ReadStream & stream, Scene & scene )
{
    memset( &scene, 0, sizeof( scene ) );

    int num_objects_sent; read_int( stream, num_objects_sent, 0, MaxObjects );

    for ( int i = 0; i < num_objects_sent; ++i )
    {
        int index; read_int( stream, index, 0, MaxObjects - 1 );
        
        read_object( stream, scene.objects[index] );
    }

    return true;
}

bool write_scene_c( protocol2::WriteStream & stream, Scene & scene )
{
    for ( int i = 0; i < MaxObjects; ++i )
    {
        if ( !scene.objects[i].send )
            continue;

        write_int( stream, i, 0, MaxObjects );

        write_object( stream, scene.objects[i] );
    }

    write_int( stream, MaxObjects, 0, MaxObjects );

    return true;
}

bool read_scene_c( protocol2::ReadStream & stream, Scene & scene )
{
    memset( &scene, 0, sizeof( scene ) );

    while ( true )
    {
        int index; read_int( stream, index, 0, MaxObjects );

        if ( index == MaxObjects )
            break;

        read_object( stream, scene.objects[index] );
    }

    return true;
}

template <typename Stream> bool serialize_object_index_internal( Stream & stream, int & previous, int & current )
{
    uint32_t difference;
    if ( Stream::IsWriting )
    {
        assert( previous < current );
        difference = current - previous;
        assert( difference > 0 );
    }

    // +1 (1 bit)

    bool plusOne;
    if ( Stream::IsWriting )
        plusOne = difference == 1;
    serialize_bool( stream, plusOne );
    if ( plusOne )
    {
        if ( Stream::IsReading )
            current = previous + 1;
        previous = current;
        return true;
    }

    // [+2,5] -> [0,3] (2 bits)

    bool twoBits;
    if ( Stream::IsWriting )
        twoBits = difference <= 5;
    serialize_bool( stream, twoBits );
    if ( twoBits )
    {
        serialize_int( stream, difference, 2, 5 );
        if ( Stream::IsReading )
            current = previous + difference;
        previous = current;
        return true;
    }

    // [6,13] -> [0,7] (3 bits)

    bool threeBits;
    if ( Stream::IsWriting )
        threeBits = difference <= 13;
    serialize_bool( stream, threeBits );
    if ( threeBits )
    {
        serialize_int( stream, difference, 6, 13 );
        if ( Stream::IsReading )
            current = previous + difference;
        previous = current;
        return true;
    }

    // [14,29] -> [0,15] (4 bits)

    bool fourBits;
    if ( Stream::IsWriting )
        fourBits = difference <= 29;
    serialize_bool( stream, fourBits );
    if ( fourBits )
    {
        serialize_int( stream, difference, 14, 29 );
        if ( Stream::IsReading )
            current = previous + difference;
        previous = current;
        return true;
    }

    // [30,61] -> [0,31] (5 bits)

    bool fiveBits;
    if ( Stream::IsWriting )
        fiveBits = difference <= 61;
    serialize_bool( stream, fiveBits );
    if ( fiveBits )
    {
        serialize_int( stream, difference, 30, 61 );
        if ( Stream::IsReading )
            current = previous + difference;
        previous = current;
        return true;
    }

    // [62,125] -> [0,63] (6 bits)

    bool sixBits;
    if ( Stream::IsWriting )
        sixBits = difference <= 125;
    serialize_bool( stream, sixBits );
    if ( sixBits )
    {
        serialize_int( stream, difference, 62, 125 );
        if ( Stream::IsReading )
            current = previous + difference;
        previous = current;
        return true;
    }

    // [126,MaxObjects+1] (required in case we go from -1 directly to MaxObjects, eg. no objects to send...)

    serialize_int( stream, difference, 126, MaxObjects + 1 );
    if ( Stream::IsReading )
    {
        current = previous + difference;
        if ( current > MaxObjects )
            current = MaxObjects;
    }

    previous = current;

    return true;
}

#define read_object_index( stream, previous, current )                          \
    do                                                                          \
    {                                                                           \
        if ( !serialize_object_index_internal( stream, previous, current ) )    \
            return false;                                                       \
    } while ( 0 )

#define write_object_index( stream, previous, current )                         \
    do                                                                          \
    {                                                                           \
        int tmp = current;                                                      \
        if ( !serialize_object_index_internal( stream, previous, tmp ) )        \
            return false;                                                       \
    } while ( 0 )

template <typename Stream> bool serialize_scene_d( Stream & stream, Scene & scene )
{
    int previous_index = -1;

    if ( Stream::IsWriting )
    {
        for ( int i = 0; i < MaxObjects; ++i )
        {
            if ( !scene.objects[i].send )
                continue;

            write_object_index( stream, previous_index, i );

            write_object( stream, scene.objects[i] );
        }

        write_object_index( stream, previous_index, MaxObjects );
    }
    else
    {
        while ( true )
        {
            int index; read_object_index( stream, previous_index, index );
            if ( index == MaxObjects )
                break;

            assert( index >= 0 );
            assert( index < MaxObjects );

            read_object( stream, scene.objects[index] );
        }
    }

    return true;
}

enum TestPacketTypes
{
    TEST_PACKET_A,
    TEST_PACKET_B,
    TEST_PACKET_C,
    TEST_PACKET_D,
    TEST_PACKET_NUM_TYPES
};

struct TestPacketA : public protocol2::Packet
{
    Scene scene;

    TestPacketA() : Packet( TEST_PACKET_A )
    {
        RandomizeScene( scene );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        return serialize_scene_a( stream, scene );
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

struct TestPacketB : public protocol2::Packet
{
    Scene scene;

    TestPacketB() : Packet( TEST_PACKET_B )
    {
        RandomizeScene( scene );
    }

    bool SerializeRead( protocol2::ReadStream & stream )
    {
        return read_scene_b( stream, scene );
    }

    bool SerializeWrite( protocol2::WriteStream & stream )
    {
        return write_scene_b( stream, scene );
    }

    bool SerializeMeasure( protocol2::MeasureStream & /*stream*/ )
    {
        return false;
    }
};

struct TestPacketC : public protocol2::Packet
{
    Scene scene;

    TestPacketC() : Packet( TEST_PACKET_C )
    {
        RandomizeScene( scene );
    }

    bool SerializeRead( protocol2::ReadStream & stream )
    {
        return read_scene_c( stream, scene );
    }

    bool SerializeWrite( protocol2::WriteStream & stream )
    {
        return write_scene_c( stream, scene );
    }

    bool SerializeMeasure( protocol2::MeasureStream & /*stream*/ )
    {
        return false;
    }
};

struct TestPacketD : public protocol2::Packet
{
    Scene scene;

    TestPacketD() : Packet( TEST_PACKET_D )
    {
        RandomizeScene( scene );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        return serialize_scene_d( stream, scene );
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
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
            case TEST_PACKET_D: return new TestPacketD();
        }
        return NULL;
    }

    void Destroy( protocol2::Packet *packet )
    {
        delete packet;
    }
};

int main()
{
    printf( "\nserialization strategies\n\n" );

    srand( (unsigned int) time( NULL ) );

    TestPacketFactory packetFactory;

    for ( int i = 0; ( i < NumIterations || NumIterations < 0 ); ++i )
    {
        const int packetType = rand() % TEST_PACKET_NUM_TYPES;

        protocol2::Packet *writePacket = packetFactory.CreatePacket( packetType );

        assert( writePacket );
        assert( writePacket->GetType() == packetType );

        uint8_t readBuffer[MaxPacketSize];
        uint8_t writeBuffer[MaxPacketSize];

        bool error = false;

        protocol2::PacketInfo info;
        info.protocolId = ProtocolId;
        info.packetFactory = &packetFactory;

        const int bytesWritten = protocol2::WritePacket( info, writePacket, writeBuffer, MaxPacketSize );

        if ( bytesWritten > 0 )
        {
            printf( "wrote packet type %d (%d bytes)\n", writePacket->GetType(), bytesWritten );
        }
        else
        {
            printf( "write packet error\n" );
            
            error = true;
        }

        memset( readBuffer, 0, sizeof( readBuffer ) );
        memcpy( readBuffer, writeBuffer, bytesWritten );

        int readError;
        protocol2::Packet *readPacket = protocol2::ReadPacket( info, readBuffer, bytesWritten, NULL, &readError );
        
        if ( readPacket )
        {
            printf( "read packet type %d (%d bytes)\n", readPacket->GetType(), bytesWritten );
        }
        else
        {
            printf( "read packet error: %s\n", protocol2::GetErrorString( readError ) );

            error = true;
        }

        packetFactory.DestroyPacket( readPacket );
        packetFactory.DestroyPacket( writePacket );

        if ( error )
            return 1;
    }

    printf( "\n" );

    return 0;
}
