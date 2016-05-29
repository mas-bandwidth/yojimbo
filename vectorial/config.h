/*
  Vectorial
  Copyright (c) 2010 Mikko Lehtonen
  Licensed under the terms of the two-clause BSD License (see LICENSE)
*/
#ifndef VECTORIAL_CONFIG_H
#define VECTORIAL_CONFIG_H


#ifndef VECTORIAL_FORCED
    #if defined(__SSE__)

        #define VECTORIAL_SSE

    #elif defined(__ARM_NEON__) 

        #define VECTORIAL_NEON

    // Don't use gnu extension for arm, buggy with some gccs with armv6 and -Os,
    // Also doesn't seem perform as well
    #elif defined(__GNUC__) && !defined(__arm__)

        #define VECTORIAL_GNU

    #else

        #define VECTORIAL_SCALAR

    #endif
#endif



#ifdef VECTORIAL_SCALAR
    #define VECTORIAL_SIMD_TYPE "scalar"
#endif

#ifdef VECTORIAL_SSE
    #define VECTORIAL_SIMD_TYPE "sse"
#endif

#ifdef VECTORIAL_NEON
    #define VECTORIAL_SIMD_TYPE "neon"
#endif

#ifdef VECTORIAL_GNU
    #define VECTORIAL_SIMD_TYPE "gnu"
#endif



#if defined(VECTORIAL_FORCED) && !defined(VECTORIAL_SIMD_TYPE)
    #error VECTORIAL_FORCED set but no simd-type found, try f.ex. VECTORIAL_SCALAR
#endif


#define vectorial_inline    static inline

#if defined(__GNUC__) 
  #if defined(__cplusplus)
    #define vectorial_restrict  __restrict
  #endif
#elif defined(_WIN32)
  #define vectorial_restrict  
#else
  #define vectorial_restrict  restrict
#endif
// #define vectorial_restrict

#ifdef __GNUC__
    #define vectorial_pure __attribute__((pure))
#else
    #define vectorial_pure
#endif


#ifdef __cplusplus
    // Hack around msvc badness
    #define SIMD_PARAM(t, p) const t& p
#else
    #define SIMD_PARAM(t, p) t p
#endif
                    
#define VECTORIAL_PI      3.14159265f
#define VECTORIAL_HALFPI  1.57079633f



#endif
