/*
    Yojimbo Network Library. Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#include "yojimbo.h"

#ifdef _MSC_VER
#define SODIUM_STATIC
#endif // #ifdef _MSC_VER

#include <sodium.h>

#if YOJIMBO_DEBUG_MEMORY_LEAKS
#include <map>
#endif // YOJIMBO_DEBUG_MEMORY_LEAKS

static yojimbo::Allocator * g_defaultAllocator = NULL;

namespace yojimbo
{
    Allocator & GetDefaultAllocator()
    {
        yojimbo_assert( g_defaultAllocator );
        return *g_defaultAllocator;
    }
}

extern "C" int netcode_init();
extern "C" int reliable_init();
extern "C" void netcode_term();
extern "C" void reliable_term();

#define NETCODE_OK 1
#define RELIABLE_OK 1

bool InitializeYojimbo()
{
    g_defaultAllocator = new yojimbo::DefaultAllocator();

    if ( netcode_init() != NETCODE_OK )
        return false;

    if ( reliable_init() != RELIABLE_OK )
        return false;

    return sodium_init() != -1;
}

void ShutdownYojimbo()
{
    reliable_term();

    netcode_term();

    yojimbo_assert( g_defaultAllocator );
    delete g_defaultAllocator;
    g_defaultAllocator = NULL;
}
