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
