/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_SIMD4F_COMMON_H
#define VECTORIAL_SIMD4F_COMMON_H


vectorial_inline simd4f simd4f_sum(simd4f v) { 
    const simd4f s1 = simd4f_add(simd4f_splat_x(v), simd4f_splat_y(v));
    const simd4f s2 = simd4f_add(s1, simd4f_splat_z(v));
    const simd4f s3 = simd4f_add(s2, simd4f_splat_w(v));
    return s3;
}

vectorial_inline simd4f simd4f_dot4(simd4f lhs, simd4f rhs) {
    return simd4f_sum( simd4f_mul(lhs, rhs) );
}

vectorial_inline simd4f simd4f_dot3(simd4f lhs, simd4f rhs) {
    const simd4f m = simd4f_mul(lhs, rhs);
    const simd4f s1 = simd4f_add(simd4f_splat_x(m), simd4f_splat_y(m));
    const simd4f s2 = simd4f_add(s1, simd4f_splat_z(m));
    return s2;
}

vectorial_inline simd4f simd4f_dot2(simd4f lhs, simd4f rhs) {
    const simd4f m = simd4f_mul(lhs, rhs);
    const simd4f s1 = simd4f_add(simd4f_splat_x(m), simd4f_splat_y(m));
    return s1;
}


vectorial_inline simd4f simd4f_length4(simd4f v) {
    return simd4f_sqrt( simd4f_dot4(v,v) );
}

vectorial_inline simd4f simd4f_length3(simd4f v) {
    return simd4f_sqrt( simd4f_dot3(v,v) );
}

vectorial_inline simd4f simd4f_length2(simd4f v) {
    return simd4f_sqrt( simd4f_dot2(v,v) );
}

vectorial_inline simd4f simd4f_length4_squared(simd4f v) {
    return simd4f_dot4(v,v);
}

vectorial_inline simd4f simd4f_length3_squared(simd4f v) {
    return simd4f_dot3(v,v);
}

vectorial_inline simd4f simd4f_length2_squared(simd4f v) {
    return simd4f_dot2(v,v);
}


vectorial_inline simd4f simd4f_normalize4(simd4f a) {
    simd4f invlen = simd4f_rsqrt( simd4f_dot4(a,a) );
    return simd4f_mul(a, invlen);    
}

vectorial_inline simd4f simd4f_normalize3(simd4f a) {
    simd4f invlen = simd4f_rsqrt( simd4f_dot3(a,a) );
    return simd4f_mul(a, invlen);    
}

vectorial_inline simd4f simd4f_normalize2(simd4f a) {
    simd4f invlen = simd4f_rsqrt( simd4f_dot2(a,a) );
    return simd4f_mul(a, invlen);    
}


#endif
