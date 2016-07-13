/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

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
#include <assert.h>

#ifdef _MSC_VER
#define SODIUM_STATIC
#endif // #ifdef _MSC_VER

#include <sodium.h>

#if YOJIMBO_DEBUG_MEMORY_LEAKS
#include <unordered_map>
#endif // YOJIMBO_DEBUG_MEMORY_LEAKS

static yojimbo::Allocator * g_defaultAllocator = NULL;

namespace yojimbo
{
    class DefaultAllocator : public Allocator
    {
#if YOJIMBO_DEBUG_MEMORY_LEAKS
        std::unordered_map<void*,uint32_t> m_alloc_map;
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

    public:

        DefaultAllocator() 
        {
            // ...
        }

        ~DefaultAllocator()
        {
#if YOJIMBO_DEBUG_MEMORY_LEAKS
            if ( m_alloc_map.size() )
            {
                printf( "you leaked memory!\n" );
                typedef std::unordered_map<void*,uint32_t>::iterator itor_type;
                for ( itor_type i = m_alloc_map.begin(); i != m_alloc_map.end(); ++i ) 
                {
                    void *p = i->first;
                    printf( "leaked block %p (%d bytes)\n", p, i->second );
                }
                exit(1);
            }
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
        }

        void * Allocate( uint32_t size )
        {
            void * p = malloc( size );
            if ( !p )
                return NULL;
#if YOJIMBO_DEBUG_MEMORY_LEAKS
            m_alloc_map[p] = size;
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
            return p;
        }

        virtual void Free( void * p ) 
        {
            if ( !p )
                return;
#if YOJIMBO_DEBUG_MEMORY_LEAKS
            assert( m_alloc_map.find( p ) != m_alloc_map.end() );
            m_alloc_map.erase( p );
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
            free( p );
        }
    };

    Allocator & GetDefaultAllocator()
    {
        assert( g_defaultAllocator );
        return *g_defaultAllocator;
    }
}

bool InitializeYojimbo()
{
    g_defaultAllocator = new yojimbo::DefaultAllocator();

    assert( yojimbo::NonceBytes == crypto_aead_chacha20poly1305_NPUBBYTES );
    assert( yojimbo::KeyBytes == crypto_aead_chacha20poly1305_KEYBYTES );
    assert( yojimbo::AuthBytes == crypto_aead_chacha20poly1305_ABYTES );
    assert( yojimbo::KeyBytes == crypto_secretbox_KEYBYTES );
    assert( yojimbo::MacBytes == crypto_secretbox_MACBYTES );

    if ( !yojimbo::InitializeNetwork() )
        return false;

    return sodium_init() != -1;
}

void ShutdownYojimbo()
{
    yojimbo::ShutdownNetwork();

    assert( g_defaultAllocator );
    delete g_defaultAllocator;
    g_defaultAllocator = NULL;
}
