/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_MAT4F_H
#define VECTORIAL_MAT4F_H

#ifndef VECTORIAL_SIMD4X4F_H
  #include "simd4x4f.h"
#endif

#ifndef VECTORIAL_VEC4F_H
  #include "vectorial/vec4f.h"
#endif

#ifndef VECTORIAL_QUAT4F_H
  #include "vectorial/quat4f.h"
#endif

namespace vectorial {
    

    class mat4f {
    public:

        simd4x4f value;
    
        inline mat4f() {}
        inline mat4f(const simd4x4f& v) : value(v) {}
        inline mat4f(vec4f v0, vec4f v1, vec4f v2, vec4f v3) : value(simd4x4f_create(v0.value, v1.value, v2.value, v3.value)) {}

        inline void load(const float *ary) { 
            value.x = simd4f_uload4(ary);
            value.y = simd4f_uload4(ary+4); 
            value.z = simd4f_uload4(ary+8); 
            value.w = simd4f_uload4(ary+12); 
        }

        inline void store(float *ary) const { 
            simd4f_ustore4(value.x, ary);
            simd4f_ustore4(value.y, ary+4);
            simd4f_ustore4(value.z, ary+8);
            simd4f_ustore4(value.w, ary+12);
        }

        static mat4f identity() { mat4f m; simd4x4f_identity(&m.value); return m; }

        static mat4f scale( float s ) { mat4f m; simd4x4f_scale(&m.value, s); return m; }

        static mat4f perspective(float fovy, float aspect, float znear, float zfar) {
            simd4x4f m;
            simd4x4f_perspective(&m, fovy, aspect, znear, zfar);
            return m;
        }
        
        static mat4f ortho(float left, float right, float bottom, float top, float znear, float zfar) {
            simd4x4f m;
            simd4x4f_ortho(&m, left, right, bottom, top, znear, zfar);
            return m;
        }
        
        static mat4f lookAt(vec3f eye, vec3f center, vec3f up) {
            simd4x4f m;
            simd4x4f_lookat(&m, eye.value, center.value, up.value);
            return m;            
        }

        static mat4f translation(vec3f pos) {
            simd4x4f m;
            simd4x4f_translation(&m, pos.x(), pos.y(), pos.z());
            return m;            
        }

        static mat4f rotation(quat4f quat) {
            simd4x4f m;
            simd4x4f_rotation(&m, quat.x(), quat.y(), quat.z(), quat.w());
            return m;            
        }

        static mat4f axisRotation(float angle, vec3f axis) {
            simd4x4f m;
            simd4x4f_axis_rotation(&m, angle, axis.value);
            return m;            
        }

        static quat4f quaternion(mat4f matrix) {
            // From http://www.insomniacgames.com/converting-a-rotation-matrix-to-a-quaternion/
            float t;
            quat4f q;
            float m[4][4];
            matrix.store( &m[0][0] );
            if ( m[2][2] < 0 ) 
            {
                if ( m[0][0] > m[1][1] )
                {
                    t = 1 + m[0][0] - m[1][1] - m[2][2];
                    q = quat4f( t, m[0][1] + m[1][0], m[2][0] + m[0][2], m[1][2] - m[2][1] );
                }
                else 
                {
                    t = 1 - m[0][0] + m[1][1] - m[2][2];
                    q = quat4f( m[0][1] + m[1][0], t, m[1][2] + m[2][1], m[2][0] - m[0][2] );
                }
            } 
            else 
            {
                if ( m[0][0] < -m[1][1] )
                {
                    t = 1 - m[0][0] - m[1][1] + m[2][2];
                    q = quat4f( m[2][0] + m[0][2], m[1][2] + m[2][1], t, m[0][1] - m[1][0] );
                }
                else 
                {
                    t = 1 + m[0][0] + m[1][1] + m[2][2];
                    q = quat4f( m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0], t );
                }
            }
            q *= 0.5f / sqrtf( t );
            return q;
        }
    };
    
    
    vectorial_inline mat4f operator*(const mat4f& lhs, const mat4f& rhs) {
        mat4f ret;
        simd4x4f_matrix_mul(&lhs.value, &rhs.value, &ret.value);
        return ret;
    }

    vectorial_inline vec4f operator*(const mat4f& lhs, const vec4f& rhs) {
        vec4f ret;
        simd4x4f_matrix_vector_mul(&lhs.value, &rhs.value, &ret.value);
        return ret;
    }

    vectorial_inline vec3f transformVector(const mat4f& lhs, const vec3f& rhs) {
        vec3f ret;
        simd4x4f_matrix_vector3_mul(&lhs.value, &rhs.value, &ret.value);
        return ret;
    }

    vectorial_inline vec3f transformPoint(const mat4f& lhs, const vec3f& rhs) {
        vec3f ret;
        simd4x4f_matrix_point3_mul(&lhs.value, &rhs.value, &ret.value);
        return ret;
    }
    
    vectorial_inline mat4f transpose(const mat4f& m) {
        mat4f ret;
        simd4x4f_transpose(&m.value, &ret.value);
        return ret;
    }
    


}



#ifdef VECTORIAL_OSTREAM
//#include <ostream>

vectorial_inline std::ostream& operator<<(std::ostream& os, const vectorial::mat4f& v) {

    os << "[ ";
    os << simd4f_get_x(v.value.x) << ", ";
    os << simd4f_get_x(v.value.y) << ", ";
    os << simd4f_get_x(v.value.z) << ", ";
    os << simd4f_get_x(v.value.w) << " ; ";

    os << simd4f_get_y(v.value.x) << ", ";
    os << simd4f_get_y(v.value.y) << ", ";
    os << simd4f_get_y(v.value.z) << ", ";
    os << simd4f_get_y(v.value.w) << " ; ";

    os << simd4f_get_z(v.value.x) << ", ";
    os << simd4f_get_z(v.value.y) << ", ";
    os << simd4f_get_z(v.value.z) << ", ";
    os << simd4f_get_z(v.value.w) << " ; ";

    os << simd4f_get_w(v.value.x) << ", ";
    os << simd4f_get_w(v.value.y) << ", ";
    os << simd4f_get_w(v.value.z) << ", ";
    os << simd4f_get_w(v.value.w) << " ]";

    return os;
}
#endif




#endif
