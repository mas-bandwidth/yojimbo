/*
    Yojimbo Client/Server Network Protocol Library.
    
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

#include "yojimbo_config.h"
#include "yojimbo_allocator.h"
#include <assert.h>
#include <stdlib.h>

#if YOJIMBO_DEBUG_MEMORY_LEAKS
#include <stdio.h>
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

namespace yojimbo
{
    Allocator::Allocator() 
    {
        SetError( ALLOCATOR_ERROR_NONE );
    }

    Allocator::~Allocator()
    {
#if YOJIMBO_DEBUG_MEMORY_LEAKS
        if ( m_alloc_map.size() )
        {
            printf( "you leaked memory!\n\n" );
            typedef std::map<void*,AllocatorEntry>::iterator itor_type;
            for ( itor_type i = m_alloc_map.begin(); i != m_alloc_map.end(); ++i )
            {
                void * p = i->first;
                AllocatorEntry entry = i->second;
                printf( "leaked block %p (%d bytes) - %s:%d\n", p, (int) entry.size, entry.file, entry.line );
            }
            printf( "\n" );
            exit(1);
        }
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
    }

    void Allocator::TrackAlloc( void * p, size_t size, const char * file, int line )
    {
#if YOJIMBO_DEBUG_MEMORY_LEAKS

        assert( m_alloc_map.find( p ) == m_alloc_map.end() );

        AllocatorEntry entry;
        entry.size = size;
        entry.file = file;
        entry.line = line;
        m_alloc_map[p] = entry;

#else // #if YOJIMBO_DEBUG_MEMORY_LEAKS

        (void) p;
        (void) size;
        (void) file;
        (void) line;

#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
    }

    void Allocator::TrackFree( void * p, const char * file, int line )
    {
        (void) p;
        (void) file;
        (void) line;
#if YOJIMBO_DEBUG_MEMORY_LEAKS
        assert( m_alloc_map.find( p ) != m_alloc_map.end() );
        m_alloc_map.erase( p );
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS
    }

    // =============================================

    void * DefaultAllocator::Allocate( size_t size, const char * file, int line )
    {
        void * p = malloc( size );

        if ( !p )
        {
            SetError( ALLOCATOR_ERROR_FAILED_TO_ALLOCATE );
            return NULL;
        }

        TrackAlloc( p, size, file, line );

        return p;
    }

    void DefaultAllocator::Free( void * p, const char * file, int line ) 
    {
        if ( !p )
            return;

        TrackFree( p, file, line );

        free( p );
    }

    // =============================================

    TLSF_Allocator::TLSF_Allocator( void * memory, size_t size ) 
    {
        m_error = ALLOCATOR_ERROR_NONE;

        m_tlsf = tlsf_create_with_pool( memory, size );
    }

    TLSF_Allocator::~TLSF_Allocator()
    {
        tlsf_destroy( m_tlsf );
    }

    void * TLSF_Allocator::Allocate( size_t size, const char * file, int line )
    {
        void * p = tlsf_malloc( m_tlsf, size );

        if ( !p )
        {
            SetError( ALLOCATOR_ERROR_FAILED_TO_ALLOCATE );
            return NULL;
        }

        TrackAlloc( p, size, file, line );
        
        return p;
    }

    void TLSF_Allocator::Free( void * p, const char * file, int line ) 
    {
        if ( !p )
            return;

        TrackFree( p, file, line );

        tlsf_free( m_tlsf, p );
    }
}
