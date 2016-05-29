/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_SIMD4X4F_SCALAR_H
#define VECTORIAL_SIMD4X4F_SCALAR_H


vectorial_inline void simd4x4f_transpose_inplace(simd4x4f *s) {
    simd4x4f d=*s;
    s->x.x = d.x.x;
    s->x.y = d.y.x;
    s->x.z = d.z.x;
    s->x.w = d.w.x;

    s->y.x = d.x.y;
    s->y.y = d.y.y;
    s->y.z = d.z.y;
    s->y.w = d.w.y;

    s->z.x = d.x.z;
    s->z.y = d.y.z;
    s->z.z = d.z.z;
    s->z.w = d.w.z;

    s->w.x = d.x.w;
    s->w.y = d.y.w;
    s->w.z = d.z.w;
    s->w.w = d.w.w;

}

vectorial_inline void simd4x4f_transpose(const simd4x4f *s, simd4x4f *out) {
    *out=*s;
    simd4x4f_transpose_inplace(out);
}



#endif
