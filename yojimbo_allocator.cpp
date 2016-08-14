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

#include "yojimbo_allocator.h"
#include <assert.h>
#include <stdlib.h>

namespace yojimbo
{
    DefaultAllocator::DefaultAllocator() 
    {
        m_error = ALLOCATOR_ERROR_NONE;
    }

    DefaultAllocator::~DefaultAllocator()
    {
#if YOJIMBO_DEBUG_MEMORY_LEAKS
        if ( m_alloc_map.size() )
        {
            printf( "you leaked memory!\n" );
            typedef std::map<void*,uint32_t>::iterator itor_type;
            for ( itor_type i = m_alloc_map.begin(); i != m_alloc_map.end(); ++i ) 
            {
                void *p = i->first;
                printf( "leaked block %p (%d bytes)\n", p, i->second );
            }
            exit(1);
        }
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
    }

    void * DefaultAllocator::Allocate( size_t size )
    {
        void * p = malloc( size );

        if ( !p )
        {
            m_error = ALLOCATOR_ERROR_FAILED_TO_ALLOCATE;
            return NULL;
        }

#if YOJIMBO_DEBUG_MEMORY_LEAKS
        m_alloc_map[p] = size;
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

        return p;
    }

    void DefaultAllocator::Free( void * p ) 
    {
        if ( !p )
            return;

#if YOJIMBO_DEBUG_MEMORY_LEAKS
        assert( m_alloc_map.find( p ) != m_alloc_map.end() );
        m_alloc_map.erase( p );
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

        free( p );
    }

    int DefaultAllocator::GetError() const
    {
        return m_error;
    }

    void DefaultAllocator::ClearError()
    {
        m_error = ALLOCATOR_ERROR_NONE;
    }

    // =============================================

    TLSFAllocator::TLSFAllocator( void * memory, size_t size ) 
    {
        m_error = ALLOCATOR_ERROR_NONE;

        m_tlsf = tlsf_create_with_pool( memory, size );
    }

    TLSFAllocator::~TLSFAllocator()
    {
#if YOJIMBO_DEBUG_MEMORY_LEAKS
        if ( m_alloc_map.size() )
        {
            printf( "you leaked memory!\n" );
            typedef std::map<void*,uint32_t>::iterator itor_type;
            for ( itor_type i = m_alloc_map.begin(); i != m_alloc_map.end(); ++i ) 
            {
                void *p = i->first;
                printf( "leaked block %p (%d bytes)\n", p, i->second );
            }
            exit(1);
        }
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

        tlsf_destroy( m_tlsf );
    }

    void * TLSFAllocator::Allocate( size_t size )
    {
        void * p = tlsf_malloc( m_tlsf, size );

        if ( !p )
        {
            m_error = ALLOCATOR_ERROR_FAILED_TO_ALLOCATE;
            return NULL;
        }

#if YOJIMBO_DEBUG_MEMORY_LEAKS
        m_alloc_map[p] = size;
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
        
        return p;
    }

    void TLSFAllocator::Free( void * p ) 
    {
        if ( !p )
            return;

#if YOJIMBO_DEBUG_MEMORY_LEAKS
        assert( m_alloc_map.find( p ) != m_alloc_map.end() );
        m_alloc_map.erase( p );
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

        tlsf_free( m_tlsf, p );
    }

    int TLSFAllocator::GetError() const
    {
        return m_error;
    }

    void TLSFAllocator::ClearError()
    {
        m_error = ALLOCATOR_ERROR_NONE;
    }
}
