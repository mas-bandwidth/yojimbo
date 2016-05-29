/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#ifndef YOJIMBO_CONFIG_H
#define YOJIMBO_CONFIG_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define YOJIMBO_MAJOR_VERSION 0
#define YOJIMBO_MINOR_VERSION 1

#if YOJIMBO_DEBUG_PACKET_LEAKS
#include <stdio.h>
#include <map>
#endif // #if YOJIMBO_DEBUG_PACKET_LEAKS

#if    defined(__386__) || defined(i386)    || defined(__i386__)  \
    || defined(__X86)   || defined(_M_IX86)                       \
    || defined(_M_X64)  || defined(__x86_64__)                    \
    || defined(alpha)   || defined(__alpha) || defined(__alpha__) \
    || defined(_M_ALPHA)                                          \
    || defined(ARM)     || defined(_ARM)    || defined(__arm__)   \
    || defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) \
    || defined(_WIN32_WCE) || defined(__NT__)                     \
    || defined(__MIPSEL__)
  #define YOJIMBO_LITTLE_ENDIAN 1
#else
  #define YOJIMBO_BIG_ENDIAN 1
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#endif // #ifdef _MSC_VER

#define YOJIMBO_PLATFORM_WINDOWS                1
#define YOJIMBO_PLATFORM_MAC                    2
#define YOJIMBO_PLATFORM_UNIX                   3

#if defined(_WIN32)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_MAC
#else
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_UNIX
#endif

#define YOJIMBO_DEBUG_PACKET_LEAKS              0

#define YOJIMBO_SCRATCH_ALLOCATOR               0

#define YOJIMBO_SERIALIZE_CHECKS                1

#define YOJIMBO_DEBUG_PACKET_LEAKS              0

#define YOJIMBO_PACKET_AGGREGATION              1

#define YOJIMBO_SOCKETS                         1
    
#define YOJIMBO_NETWORK_SIMULATOR               1

#endif // #ifndef YOJIMBO_CONFIG_H
