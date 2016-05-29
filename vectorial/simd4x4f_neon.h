/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_SIMD4X4F_NEON_H
#define VECTORIAL_SIMD4X4F_NEON_H


vectorial_inline void simd4x4f_transpose_inplace(simd4x4f* s) {
    const _simd4f_union sx = { s->x };
    const _simd4f_union sy = { s->y };
    const _simd4f_union sz = { s->z };
    const _simd4f_union sw = { s->w };
    
    const simd4f dx = simd4f_create( sx.f[0], sy.f[0], sz.f[0], sw.f[0] );
    const simd4f dy = simd4f_create( sx.f[1], sy.f[1], sz.f[1], sw.f[1] );
    const simd4f dz = simd4f_create( sx.f[2], sy.f[2], sz.f[2], sw.f[2] );
    const simd4f dw = simd4f_create( sx.f[3], sy.f[3], sz.f[3], sw.f[3] );

    s->x = dx;
    s->y = dy;
    s->z = dz;
    s->w = dw;

}

vectorial_inline void simd4x4f_transpose(const simd4x4f *s, simd4x4f *out) {
    *out=*s;
    simd4x4f_transpose_inplace(out);
}



#endif
