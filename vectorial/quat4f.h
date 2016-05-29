#ifndef VECTORIAL_QUAT4F_H
#define VECTORIAL_QUAT4F_H

#include <math.h>
#include "vectorial/vec3f.h"
#include "vectorial/vec4f.h"

namespace vectorial 
{
    struct quat4f : public vec4f
    {
        quat4f() {}

        quat4f( float x, float y, float z, float w )
            : vec4f( x, y, z, w ) {}

        quat4f( vec4f v )
            : vec4f( v ) {}

        static quat4f identity()
        {
            return quat4f( 0, 0, 0, 1 );
        }

        static quat4f axis_rotation( float angle_radians, vec3f axis )
        {
            const float a = angle_radians * 0.5f;
            const float s = (float) sin( a );
            const float c = (float) cos( a );
            return quat4f( axis.x() * s, axis.y() * s, axis.z() * s, c );
        }

        void to_axis_angle( vec3f & axis, float & angle, const float epsilonSquared = 0.0001f * 0.0001f ) const
        {
            const float squareLength = length_squared( vec3f( x(), y(), z() ) );

            const float _x = x();
            const float _y = y();
            const float _z = z();
            const float _w = w();

            if ( squareLength > epsilonSquared )
            {
                angle = 2.0f * (float) acos( _w );
                const float inverseLength = 1.0f / (float) sqrt( squareLength );
                axis = vec3f( _x, _y, _z ) * inverseLength;
            }
            else
            {
                axis = vec3f( 1, 0, 0 );
                angle = 0.0f;
            }
        }
    };

    static inline float norm( const quat4f & q )
    {
        return length_squared( q );
    }

    static inline quat4f conjugate( const quat4f & q )
    {
        return quat4f( -q.x(), -q.y(), -q.z(), q.w() );
    }

    static inline quat4f exp( const quat4f & q ) 
    {
        float a[4];
        q.store( a );
        float r  = (float) sqrt( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );
        float et = (float) ::exp( a[3] );
        float s  = ( r >= 0.00001f ) ? et * (float) sin(r) / r : 0;
        return quat4f( s*a[0],s *a[1], s*a[2], et*(float)cos(r) );
    }

    static inline quat4f ln( const quat4f & q ) 
    {
        float a[4];
        q.store( a );
        float r = (float) sqrt( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );
        float t = ( r > 0.00001f ) ? float( atan2( r, a[3] ) ) / r : 0;
        return quat4f( t*a[0], t*a[1], t*a[2], 0.5f*(float)log( norm(q) ) );
    }

    static inline quat4f nlerp( float t, const quat4f & a, const quat4f & b ) 
    {
        return ( dot( a, b ) >= 0 ) ? normalize( a + ( b - a ) * t ) : normalize( a + ( -b - a ) * t );
    }

    static inline quat4f slerp( float t, const quat4f & a, const quat4f & b ) 
    {
        float flip = 1;

        float cosine = dot( a, b );

        if ( cosine < 0 )
        { 
            cosine = -cosine; 
            flip = -1; 
        }

        const float epsilon = 0.001f;   

        if ( ( 1 - cosine ) < epsilon ) 
            return a * ( 1 - t ) + b * ( t * flip ); 

        float theta = (float) acos( cosine ); 
        float sine = (float) sin( theta ); 
        float beta = (float) sin( ( 1 - t ) * theta ) / sine; 
        float alpha = (float) sin( t * theta ) / sine;

        return a * beta + b * alpha * flip;
    }

    static inline quat4f multiply( const quat4f & a, const quat4f & b )
    {
        const float a_x = a.x();
        const float a_y = a.y();
        const float a_z = a.z();
        const float a_w = a.w();

        const float b_x = b.x();
        const float b_y = b.y();
        const float b_z = b.z();
        const float b_w = b.w();

        return quat4f( a_w * b_x + a_x * b_w + a_y * b_z - a_z * b_y, 
                       a_w * b_y - a_x * b_z + a_y * b_w + a_z * b_x,
                       a_w * b_z + a_x * b_y - a_y * b_x + a_z * b_w,
                       a_w * b_w - a_x * b_x - a_y * b_y - a_z * b_z );
    }

    static inline quat4f operator * ( const quat4f & lhs, const quat4f & rhs )
    {
        return multiply( lhs, rhs );
    }

    static inline vec3f transform( const quat4f & quat, const vec3f & input )
    {
        quat4f quat_inv = conjugate( quat );
        quat4f a( input.x(), input.y(), input.z(), 0 );
        quat4f r = multiply( multiply( quat, a ), quat_inv );
        return vec3f( r.x(), r.y(), r.z() );
    }
}

#endif
