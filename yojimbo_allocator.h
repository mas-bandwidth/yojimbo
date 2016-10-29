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

#ifndef YOJIMBO_ALLOCATOR_H
#define YOJIMBO_ALLOCATOR_H

#include <stdint.h>
#include <new>
#if YOJIMBO_DEBUG_MEMORY_LEAKS
#include <map>
#endif // YOJIMBO_DEBUG_MEMORY_LEAKS

#include "yojimbo_tlsf.h"

namespace yojimbo
{
    enum
    {
        ALLOCATOR_ERROR_NONE = 0,
        ALLOCATOR_ERROR_FAILED_TO_ALLOCATE
    };

    class Allocator
    {
    public:

        Allocator() {}

        virtual ~Allocator() {}
        
        virtual void * Allocate( size_t size ) = 0;

        virtual void Free( void * p ) = 0;

        virtual int GetError() const = 0;

        virtual void ClearError() = 0;

    private:

        Allocator( const Allocator & other );

        Allocator & operator = ( const Allocator & other );
    };

    Allocator & GetDefaultAllocator();

    #define YOJIMBO_NEW( a, T, ... ) ( new ((a).Allocate(sizeof(T))) T(__VA_ARGS__) )

    #define YOJIMBO_DELETE( a, T, p ) do { if (p) { (p)->~T(); (a).Free(p); p = NULL; } } while (0)    

    class DefaultAllocator : public Allocator
    {
#if YOJIMBO_DEBUG_MEMORY_LEAKS
        std::map<void*,uint32_t> m_alloc_map;
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

        int m_error;

    public:

        DefaultAllocator();

        ~DefaultAllocator();

        void * Allocate( size_t size );

        void Free( void * p );

        int GetError() const;

        void ClearError();
    };

    class TLSFAllocator : public Allocator
    {
#if YOJIMBO_DEBUG_MEMORY_LEAKS
        std::map<void*,uint32_t> m_alloc_map;
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

        int m_error;
        tlsf_t m_tlsf;

    public:

        TLSFAllocator( void * memory, size_t bytes );

        ~TLSFAllocator();

        void * Allocate( size_t size );

        void Free( void * p );

        int GetError() const;

        void ClearError();
    };
}

#endif
