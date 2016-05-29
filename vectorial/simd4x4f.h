/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_SIMD4X4F_H
#define VECTORIAL_SIMD4X4F_H


#include "simd4f.h"

#include <math.h>

/*
  Note, x,y,z,w are conceptually columns with matrix math.
*/

typedef struct {
    simd4f x,y,z,w;
} simd4x4f;



vectorial_inline simd4x4f simd4x4f_create(simd4f x, simd4f y, simd4f z, SIMD_PARAM(simd4f, w)) {
    simd4x4f s = { x, y, z, w };
    return s;
}


vectorial_inline void simd4x4f_identity(simd4x4f* m) {
    *m = simd4x4f_create( simd4f_create(1.0f, 0.0f, 0.0f, 0.0f),
                          simd4f_create(0.0f, 1.0f, 0.0f, 0.0f),
                          simd4f_create(0.0f, 0.0f, 1.0f, 0.0f),
                          simd4f_create(0.0f, 0.0f, 0.0f, 1.0f));
}


vectorial_inline void simd4x4f_scale(simd4x4f* m, float s) {
    *m = simd4x4f_create( simd4f_create(   s, 0.0f, 0.0f, 0.0f),
                          simd4f_create(0.0f,    s, 0.0f, 0.0f),
                          simd4f_create(0.0f, 0.0f,    s, 0.0f),
                          simd4f_create(0.0f, 0.0f, 0.0f, 1.0f));
}


vectorial_inline void simd4x4f_uload(simd4x4f* m, float *f) {

    m->x = simd4f_uload4(f + 0);
    m->y = simd4f_uload4(f + 4);
    m->z = simd4f_uload4(f + 8);
    m->w = simd4f_uload4(f + 12);

}





#ifdef VECTORIAL_SCALAR
    #include "simd4x4f_scalar.h"
#elif defined(VECTORIAL_SSE)
    #include "simd4x4f_sse.h"
#elif defined(VECTORIAL_GNU)
    #include "simd4x4f_gnu.h"
#elif defined(VECTORIAL_NEON)
    #include "simd4x4f_neon.h"
#else
    #error No implementation defined
#endif

vectorial_inline void simd4x4f_sum(const simd4x4f* a, simd4f* out) {
    simd4f t;
    t = simd4f_add(a->x, a->y);
    t = simd4f_add(t, a->z);
    t = simd4f_add(t, a->w);
    *out = t;
}

vectorial_inline void simd4x4f_matrix_vector_mul(const simd4x4f* a, const simd4f * b, simd4f* out) {

    simd4x4f bbbb = simd4x4f_create( simd4f_splat_x(*b),
                                     simd4f_splat_y(*b),
                                     simd4f_splat_z(*b),
                                     simd4f_splat_w(*b) );

    simd4x4f ab = simd4x4f_create( simd4f_mul(a->x, bbbb.x),
                                   simd4f_mul(a->y, bbbb.y),
                                   simd4f_mul(a->z, bbbb.z),
                                   simd4f_mul(a->w, bbbb.w) );

    simd4x4f_sum(&ab, out);

}

vectorial_inline void simd4x4f_matrix_vector3_mul(const simd4x4f* a, const simd4f * b, simd4f* out) {

    *out = simd4f_add( simd4f_add( simd4f_mul(a->x, simd4f_splat_x(*b)), 
                                   simd4f_mul(a->y, simd4f_splat_y(*b)) ),
                       simd4f_mul(a->z, simd4f_splat_z(*b)) );

}

vectorial_inline void simd4x4f_matrix_point3_mul(const simd4x4f* a, const simd4f * b, simd4f* out) {

    *out = simd4f_add( simd4f_add( simd4f_mul(a->x, simd4f_splat_x(*b)), 
                                   simd4f_mul(a->y, simd4f_splat_y(*b)) ),
                       simd4f_add( simd4f_mul(a->z, simd4f_splat_z(*b)), 
                                   a->w) );

}


vectorial_inline void simd4x4f_matrix_mul(const simd4x4f* a, const simd4x4f* b, simd4x4f* out) {

    simd4x4f_matrix_vector_mul(a, &b->x, &out->x);
    simd4x4f_matrix_vector_mul(a, &b->y, &out->y);
    simd4x4f_matrix_vector_mul(a, &b->z, &out->z);
    simd4x4f_matrix_vector_mul(a, &b->w, &out->w);

}




vectorial_inline void simd4x4f_perspective(simd4x4f *m, float fovy, float aspect, float znear, float zfar) {
    
    float radians = fovy * VECTORIAL_HALFPI / 180.0f;
    float deltaz = zfar - znear;
    float sine = sinf(radians);
    float cotangent = cosf(radians) / sine;
    
    float a = cotangent / aspect;
    float b = cotangent;
    float c = -(zfar + znear) / deltaz;
    float d = -2 * znear * zfar / deltaz;
    
    m->x = simd4f_create( a, 0, 0,  0);
    m->y = simd4f_create( 0, b, 0,  0);
    m->z = simd4f_create( 0, 0, c, -1);
    m->w = simd4f_create( 0, 0, d,  0);

}

vectorial_inline void simd4x4f_ortho(simd4x4f *m, float left, float right, float bottom, float top, float znear, float zfar) {
    
    float deltax = right - left;
    float deltay = top - bottom;
    float deltaz = zfar - znear;

    float a = 2.0f / deltax;
    float b = -(right + left) / deltax;
    float c = 2.0f / deltay;
    float d = -(top + bottom) / deltay;
    float e =  -2.0f / deltaz;
    float f = -(zfar + znear) / deltaz;
    
    m->x = simd4f_create( a, 0, 0, 0);
    m->y = simd4f_create( 0, c, 0, 0);
    m->z = simd4f_create( 0, 0, e, 0);
    m->w = simd4f_create( b, d, f, 1);
    
}


vectorial_inline void simd4x4f_lookat(simd4x4f *m, simd4f eye, simd4f center, simd4f up) {
    
    simd4f zaxis = simd4f_normalize3( simd4f_sub(center, eye) );
    simd4f xaxis = simd4f_normalize3( simd4f_cross3( zaxis, up ) );
    simd4f yaxis = simd4f_cross3(xaxis, zaxis);

    zaxis = simd4f_sub( simd4f_zero(), zaxis);

    float x = -simd4f_get_x( simd4f_dot3(xaxis, eye) );
    float y = -simd4f_get_x( simd4f_dot3(yaxis, eye) );
    float z = -simd4f_get_x( simd4f_dot3(zaxis, eye) );

    m->x = xaxis;
    m->y = yaxis;
    m->z = zaxis;

    m->w = simd4f_create( 0,0,0, 1);
    simd4x4f_transpose_inplace(m);
    m->w = simd4f_create( x,y,z,1);

}


vectorial_inline void simd4x4f_translation(simd4x4f* m, float x, float y, float z) {
    *m = simd4x4f_create( simd4f_create(1.0f, 0.0f, 0.0f, 0.0f),
                          simd4f_create(0.0f, 1.0f, 0.0f, 0.0f),
                          simd4f_create(0.0f, 0.0f, 1.0f, 0.0f),
                          simd4f_create(   x,    y,    z, 1.0f));
}


vectorial_inline void simd4x4f_rotation(simd4x4f* m, float x, float y, float z, float w) {

    const float fTx  = 2.0f * x;
    const float fTy  = 2.0f * y;
    const float fTz  = 2.0f * z;

    const float fTwx = fTx * w;
    const float fTwy = fTy * w;
    const float fTwz = fTz * w;

    const float fTxx = fTx * x;
    const float fTxy = fTy * x;
    const float fTxz = fTz * x;

    const float fTyy = fTy * y;
    const float fTyz = fTz * y;
    const float fTzz = fTz * z;

    *m = simd4x4f_create( simd4f_create( 1.0f - ( fTyy + fTzz ), fTxy + fTwz, fTxz - fTwy, 0 ),
                          simd4f_create( fTxy - fTwz, 1.0f - ( fTxx + fTzz ), fTyz + fTwx, 0 ),
                          simd4f_create( fTxz + fTwy, fTyz - fTwx, 1.0f - ( fTxx + fTyy ), 0 ),
                          simd4f_create( 0, 0, 0, 1 ));
}


vectorial_inline void simd4x4f_axis_rotation(simd4x4f* m, float angle, simd4f axis) {

    angle = -angle;

    axis = simd4f_normalize3(axis);

	const float pi = 3.14159265358979323846f;

    const float radians = angle * pi / 180;
    const float sine = sinf(radians);
    const float cosine = cosf(radians);

    const float x = simd4f_get_x(axis);
    const float y = simd4f_get_y(axis);
    const float z = simd4f_get_z(axis);

    const float ab = x * y * (1 - cosine);
    const float bc = y * z * (1 - cosine);
    const float ca = z * x * (1 - cosine);

    const float tx = x * x;
    const float ty = y * y;
    const float tz = z * z;

    const simd4f i = simd4f_create( tx + cosine * (1 - tx), ab - z * sine,          ca + y * sine,          0);
    const simd4f j = simd4f_create( ab + z * sine,          ty + cosine * (1 - ty), bc - x * sine,          0);
    const simd4f k = simd4f_create( ca - y * sine,          bc + x * sine,          tz + cosine * (1 - tz), 0);
    
    *m = simd4x4f_create( i,j,k, simd4f_create(0, 0, 0, 1) );
        
}



vectorial_inline void simd4x4f_add(simd4x4f* a, simd4x4f* b, simd4x4f* out) {
    
    out->x = simd4f_add(a->x, b->x);
    out->y = simd4f_add(a->y, b->y);
    out->z = simd4f_add(a->z, b->z);
    out->w = simd4f_add(a->w, b->w);
    
}

vectorial_inline void simd4x4f_sub(simd4x4f* a, simd4x4f* b, simd4x4f* out) {
    
    out->x = simd4f_sub(a->x, b->x);
    out->y = simd4f_sub(a->y, b->y);
    out->z = simd4f_sub(a->z, b->z);
    out->w = simd4f_sub(a->w, b->w);
    
}

vectorial_inline void simd4x4f_mul(simd4x4f* a, simd4x4f* b, simd4x4f* out) {
    
    out->x = simd4f_mul(a->x, b->x);
    out->y = simd4f_mul(a->y, b->y);
    out->z = simd4f_mul(a->z, b->z);
    out->w = simd4f_mul(a->w, b->w);
    
}

vectorial_inline void simd4x4f_div(simd4x4f* a, simd4x4f* b, simd4x4f* out) {
    
    out->x = simd4f_div(a->x, b->x);
    out->y = simd4f_div(a->y, b->y);
    out->z = simd4f_div(a->z, b->z);
    out->w = simd4f_div(a->w, b->w);
    
}


#ifdef __cplusplus

    #ifdef VECTORIAL_OSTREAM
        #include <ostream>

        vectorial_inline std::ostream& operator<<(std::ostream& os, const simd4x4f& v) {
            os << "simd4x4f(simd4f(" << simd4f_get_x(v.x) << ", "
                       << simd4f_get_y(v.x) << ", "
                       << simd4f_get_z(v.x) << ", "
                       << simd4f_get_w(v.x) << "),\n"
                       << "         simd4f(" << simd4f_get_x(v.y) << ", "
                       << simd4f_get_y(v.y) << ", "
                       << simd4f_get_z(v.y) << ", "
                       << simd4f_get_w(v.y) << "),\n"
                       << "         simd4f(" << simd4f_get_x(v.z) << ", "
                       << simd4f_get_y(v.z) << ", "
                       << simd4f_get_z(v.z) << ", "
                       << simd4f_get_w(v.z) << "),\n"
                       << "         simd4f(" << simd4f_get_x(v.w) << ", "
                       << simd4f_get_y(v.w) << ", "
                       << simd4f_get_z(v.w) << ", "
                       << simd4f_get_w(v.w) << "))";
            return os;
        }
    #endif

#endif





#endif 
