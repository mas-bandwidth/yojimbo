/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_SIMD4F_SCALAR_H
#define VECTORIAL_SIMD4F_SCALAR_H

#include <math.h>
#include <string.h>  // memcpy

#ifdef __cplusplus
extern "C" {
#endif


typedef struct { 
    float x;
    float y; 
    float z; 
    float w;
} simd4f;



vectorial_inline simd4f simd4f_create(float x, float y, float z, float w) {
    simd4f s = { x, y, z, w };
    return s;
}

vectorial_inline simd4f simd4f_zero() { return simd4f_create(0.0f, 0.0f, 0.0f, 0.0f); }

vectorial_inline simd4f simd4f_uload4(const float *ary) {
    simd4f s = { ary[0], ary[1], ary[2], ary[3] };
    return s;
}

vectorial_inline simd4f simd4f_uload3(const float *ary) {
    simd4f s = { ary[0], ary[1], ary[2], 0 };
    return s;
}

vectorial_inline simd4f simd4f_uload2(const float *ary) {
    simd4f s = { ary[0], ary[1], 0, 0 };
    return s;
}


vectorial_inline void simd4f_ustore4(const simd4f val, float *ary) {
    memcpy(ary, &val, sizeof(float) * 4);
}

vectorial_inline void simd4f_ustore3(const simd4f val, float *ary) {
    memcpy(ary, &val, sizeof(float) * 3);
}

vectorial_inline void simd4f_ustore2(const simd4f val, float *ary) {
    memcpy(ary, &val, sizeof(float) * 2);
}



// utilities
vectorial_inline simd4f simd4f_splat(float v) { 
    simd4f s = { v, v, v, v }; 
    return s;
}

vectorial_inline simd4f simd4f_splat_x(simd4f v) { 
    simd4f s = { v.x, v.x, v.x, v.x }; 
    return s;
}

vectorial_inline simd4f simd4f_splat_y(simd4f v) { 
    simd4f s = { v.y, v.y, v.y, v.y }; 
    return s;
}

vectorial_inline simd4f simd4f_splat_z(simd4f v) { 
    simd4f s = { v.z, v.z, v.z, v.z }; 
    return s;
}

vectorial_inline simd4f simd4f_splat_w(simd4f v) { 
    simd4f s = { v.w, v.w, v.w, v.w }; 
    return s;
}

vectorial_inline simd4f simd4f_reciprocal(simd4f v) { 
    simd4f s = { 1.0f/v.x, 1.0f/v.y, 1.0f/v.z, 1.0f/v.w }; 
    return s;
}

vectorial_inline simd4f simd4f_sqrt(simd4f v) { 
    simd4f s = { sqrtf(v.x), sqrtf(v.y), sqrtf(v.z), sqrtf(v.w) }; 
    return s;
}

vectorial_inline simd4f simd4f_rsqrt(simd4f v) { 
    simd4f s = { 1.0f/sqrtf(v.x), 1.0f/sqrtf(v.y), 1.0f/sqrtf(v.z), 1.0f/sqrtf(v.w) }; 
    return s;
}


// arithmetic

vectorial_inline simd4f simd4f_add(simd4f lhs, simd4f rhs) {
    simd4f ret = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
    return ret;
}

vectorial_inline simd4f simd4f_sub(simd4f lhs, simd4f rhs) {
    simd4f ret = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
    return ret;
}

vectorial_inline simd4f simd4f_mul(simd4f lhs, simd4f rhs) {
    simd4f ret = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
    return ret;
}

vectorial_inline simd4f simd4f_div(simd4f lhs, simd4f rhs) {
    simd4f ret = { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w };
    return ret;
}


vectorial_inline simd4f simd4f_cross3(simd4f lhs, simd4f rhs) {
    return simd4f_create( lhs.y * rhs.z - lhs.z * rhs.y,
                          lhs.z * rhs.x - lhs.x * rhs.z,
                          lhs.x * rhs.y - lhs.y * rhs.x, 0);
}


vectorial_inline float simd4f_get_x(simd4f s) { return s.x; }
vectorial_inline float simd4f_get_y(simd4f s) { return s.y; }
vectorial_inline float simd4f_get_z(simd4f s) { return s.z; }
vectorial_inline float simd4f_get_w(simd4f s) { return s.w; }


#ifdef __cplusplus
}
#endif


#endif

