/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2026, Más Bandwidth LLC.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "yojimbo.h"
#include "yojimbo_utils.h"
#include "netcode.h"
#include "reliable.h"

// Note: libsodium is initialized by netcode_init() (which calls sodium_init and fails if it
// can't), so this translation unit no longer calls sodium directly and does not include sodium.h.

static yojimbo::Allocator * g_defaultAllocator;

namespace yojimbo
{
    Allocator & GetDefaultAllocator()
    {
        yojimbo_assert( g_defaultAllocator );
        return *g_defaultAllocator;
    }
}

bool InitializeYojimbo()
{
    // Bring subsystems up in order and unwind anything already initialized if a later step
    // fails. Callers do not call ShutdownYojimbo when InitializeYojimbo returns false, so on
    // every failure path we must leave the process exactly as we found it: no subsystem left
    // initialized, and no default allocator leaked.
    if ( netcode_init() != NETCODE_OK )
        return false;

    // netcode_init() already calls sodium_init() (and fails if it can't), so we don't repeat it
    // here: netcode owns that call and takes priority. There is nothing extra to initialize for
    // libsodium at the yojimbo layer.

    if ( reliable_init() != RELIABLE_OK )
    {
        netcode_term();
        return false;
    }

    // Create the default allocator last, so an earlier failure can't leak it. (Throwing new:
    // it never returns NULL, so there is no allocator-failure path to unwind here.)
    yojimbo_assert( g_defaultAllocator == NULL );
    g_defaultAllocator = new yojimbo::DefaultAllocator();

    return true;
}

void EnablePacketTagging()
{
    netcode_enable_packet_tagging();
}

void ShutdownYojimbo()
{
    reliable_term();

    netcode_term();

    yojimbo_assert( g_defaultAllocator );
    delete g_defaultAllocator;
    g_defaultAllocator = NULL;
}
