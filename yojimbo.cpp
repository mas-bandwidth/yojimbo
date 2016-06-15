/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.

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
#include <map>
#endif // YOJIMBO_DEBUG_MEMORY_LEAKS

static yojimbo::Allocator * g_defaultAllocator = NULL;

namespace yojimbo
{
    struct Header 
    {
        uint32_t size;
    };

    const uint32_t HEADER_PAD_VALUE = 0xffffffff;

    inline void * align_forward( void * p, uint32_t align )
    {
        uintptr_t pi = uintptr_t( p );
        const uint32_t mod = pi % align;
        if ( mod )
            pi += align - mod;
        return (void*) pi;
    }

    inline void * data_pointer( Header * header, uint32_t align )
    {
        void * p = header + 1;
        return align_forward( p, align );
    }

    inline Header * header( void * data )
    {
        uint32_t * p = (uint32_t*) data;
        while ( p[-1] == HEADER_PAD_VALUE )
            --p;
        return (Header*)p - 1;
    }

    inline void fill( Header * header, void * data, uint32_t size )
    {
        header->size = size;
        uint32_t * p = (uint32_t*) ( header + 1 );
        while ( p < data )
            *p++ = HEADER_PAD_VALUE;
    }

    class MallocAllocator : public Allocator
    {
        int64_t m_total_allocated;

#if YOJIMBO_DEBUG_MEMORY_LEAKS
        std::map<void*,int> m_alloc_map;
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

        static inline uint32_t size_with_padding( uint32_t size, uint32_t align ) 
        {
            return size + align + sizeof( Header );
        }

    public:

        MallocAllocator() : m_total_allocated(0) {}

        ~MallocAllocator()
        {
#if YOJIMBO_DEBUG_MEMORY_LEAKS
            if ( m_alloc_map.size() )
            {
                printf( "you leaked memory!\n" );
                printf( "%d blocks still allocated\n", (int) m_alloc_map.size() );
                printf( "%d bytes still allocated\n", (uint32_t) m_total_allocated );
                typedef std::map<void*,int>::iterator itor_type;
                for ( itor_type i = m_alloc_map.begin(); i != m_alloc_map.end(); ++i ) 
                {
                    void *p = i->first;
                    printf( "leaked block %p\n", p );
                }
                exit(1);
            }
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

            if ( m_total_allocated != 0 )
            {
                printf( "you leaked memory! %d bytes still allocated\n", (uint32_t) m_total_allocated );
                assert( !"leaked memory" );
            }

            assert( m_total_allocated == 0 );
        }

        void * Allocate( uint32_t size, uint32_t align )
        {
            const uint32_t ts = size_with_padding( size, align );
            Header * h = (Header*) malloc( ts );
            void * p = data_pointer( h, align );
            fill( h, p, ts );
            m_total_allocated += ts;
#if YOJIMBO_DEBUG_MEMORY_LEAKS
            m_alloc_map[p] = 1;
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
            return p;
        }

        virtual void Free( void * p ) 
        {
            if ( !p )
                return;
#if YOJIMBO_DEBUG_MEMORY_LEAKS
            typedef std::map<void*,int>::iterator itor_type;
            itor_type itor = m_alloc_map.find( p );
            assert( itor != m_alloc_map.end() );
            m_alloc_map.erase( p );
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
            Header * h = header( p );
            m_total_allocated -= h->size;
            assert( m_total_allocated >= 0 );
            free( h );
        }

        virtual uint32_t GetAllocatedSize( void * p )
        {
            return header(p)->size;
        }

        virtual uint32_t GetTotalAllocated() 
        {
            return (uint32_t) m_total_allocated;
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
    g_defaultAllocator = new yojimbo::MallocAllocator();

    assert( yojimbo::NonceBytes == crypto_aead_chacha20poly1305_NPUBBYTES );
    assert( yojimbo::KeyBytes == crypto_aead_chacha20poly1305_KEYBYTES );
    assert( yojimbo::KeyBytes == crypto_secretbox_KEYBYTES );
    assert( yojimbo::AuthBytes == crypto_aead_chacha20poly1305_ABYTES );
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
