/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_SIMD4F_SSE_H
#define VECTORIAL_SIMD4F_SSE_H

#include <xmmintrin.h>
#include <string.h>  // memcpy

#ifdef __cplusplus
extern "C" {
#endif


typedef __m128 simd4f; 

typedef union {
    simd4f s ;
    float f[4];
} _simd4f_union;

// creating

vectorial_inline simd4f simd4f_create(float x, float y, float z, float w) {
    simd4f s = { x, y, z, w };
    return s;
}

vectorial_inline simd4f simd4f_zero() { return _mm_setzero_ps(); }

vectorial_inline simd4f simd4f_uload4(const float *ary) {
    simd4f s = _mm_loadu_ps(ary);
    return s;
}

vectorial_inline simd4f simd4f_uload3(const float *ary) {
    simd4f s = simd4f_create(ary[0], ary[1], ary[2], 0);
    return s;
}

vectorial_inline simd4f simd4f_uload2(const float *ary) {
    simd4f s = simd4f_create(ary[0], ary[1], 0, 0);
    return s;
}


vectorial_inline void simd4f_ustore4(const simd4f val, float *ary) {
    _mm_storeu_ps(ary, val);
}

vectorial_inline void simd4f_ustore3(const simd4f val, float *ary) {
    memcpy(ary, &val, sizeof(float) * 3);
}

vectorial_inline void simd4f_ustore2(const simd4f val, float *ary) {
    memcpy(ary, &val, sizeof(float) * 2);
}


// utilites

vectorial_inline simd4f simd4f_splat(float v) { 
    simd4f s = _mm_set1_ps(v); 
    return s;
}

vectorial_inline simd4f simd4f_splat_x(simd4f v) { 
    simd4f s = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0,0,0,0)); 
    return s;
}

vectorial_inline simd4f simd4f_splat_y(simd4f v) { 
    simd4f s = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1,1,1,1)); 
    return s;
}

vectorial_inline simd4f simd4f_splat_z(simd4f v) { 
    simd4f s = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,2,2,2)); 
    return s;
}

vectorial_inline simd4f simd4f_splat_w(simd4f v) { 
    simd4f s = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3,3,3,3)); 
    return s;
}

vectorial_inline simd4f simd4f_reciprocal(simd4f v) { 
    simd4f s = _mm_rcp_ps(v); 
    return s;
}

vectorial_inline simd4f simd4f_sqrt(simd4f v) { 
    simd4f s = _mm_sqrt_ps(v); 
    return s;
}

vectorial_inline simd4f simd4f_rsqrt(simd4f v) { 
    simd4f s = _mm_rsqrt_ps(v); 
    return s;
}


// arithmetic

vectorial_inline simd4f simd4f_add(simd4f lhs, simd4f rhs) {
    simd4f ret = _mm_add_ps(lhs, rhs);
    return ret;
}

vectorial_inline simd4f simd4f_sub(simd4f lhs, simd4f rhs) {
    simd4f ret = _mm_sub_ps(lhs, rhs);
    return ret;
}

vectorial_inline simd4f simd4f_mul(simd4f lhs, simd4f rhs) {
    simd4f ret = _mm_mul_ps(lhs, rhs);
    return ret;
}

vectorial_inline simd4f simd4f_div(simd4f lhs, simd4f rhs) {
    simd4f ret = _mm_div_ps(lhs, rhs);
    return ret;
}


vectorial_inline simd4f simd4f_cross3(simd4f lhs, simd4f rhs) {
    
    const simd4f lyzx = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(3,0,2,1));
    const simd4f lzxy = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(3,1,0,2));

    const simd4f ryzx = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(3,0,2,1));
    const simd4f rzxy = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(3,1,0,2));

    return _mm_sub_ps(_mm_mul_ps(lyzx, rzxy), _mm_mul_ps(lzxy, ryzx));

}



vectorial_inline float simd4f_get_x(simd4f s) { _simd4f_union u={s}; return u.f[0]; }
vectorial_inline float simd4f_get_y(simd4f s) { _simd4f_union u={s}; return u.f[1]; }
vectorial_inline float simd4f_get_z(simd4f s) { _simd4f_union u={s}; return u.f[2]; }
vectorial_inline float simd4f_get_w(simd4f s) { _simd4f_union u={s}; return u.f[3]; }


#ifdef __cplusplus
}
#endif


#endif

