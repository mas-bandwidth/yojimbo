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

#ifndef YOJIMBO_QUEUE_H
#define YOJIMBO_QUEUE_H

#include "yojimbo_config.h"
#include "yojimbo_allocator.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

namespace yojimbo
{
    // todo: document this file

    template <typename T> class Queue
    {
        Allocator * m_allocator;
        T * m_entries;
        int m_arraySize;
        int m_startIndex;
        int m_numEntries;

    public:

        Queue( Allocator & allocator, int size )
        {
            assert( size > 0 );
            m_arraySize = size;
            m_startIndex = 0;
            m_numEntries = 0;
            m_allocator = &allocator;
            m_entries = (T*) YOJIMBO_ALLOCATE( allocator, sizeof(T) * size );
            memset( m_entries, 0, sizeof(T) * size );
        }

        ~Queue()
        {
            assert( m_allocator );
            
            YOJIMBO_FREE( *m_allocator, m_entries );

            m_arraySize = 0;
            m_startIndex = 0;
            m_numEntries = 0;

            m_allocator = NULL;
        }

        void Clear()
        {
            m_numEntries = 0;
            m_startIndex = 0;
        }

        T Pop()
        {
            assert( !IsEmpty() );
            const T & entry = m_entries[m_startIndex];
            m_startIndex = ( m_startIndex + 1 ) % m_arraySize;
            m_numEntries--;
            return entry;
        }

        void Push( const T & entry )
        {
            assert( !IsFull() );
            const int index = ( m_startIndex + m_numEntries ) % m_arraySize;
            m_entries[index] = entry;
            m_numEntries++;
        }

        T & operator [] ( int index )
        {
            assert( !IsEmpty() );
            assert( index >= 0 );
            assert( index < m_numEntries );
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        const T & operator [] ( int index ) const
        {
            assert( !IsEmpty() );
            assert( index >= 0 );
            assert( index < m_numEntries );
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        int GetSize() const
        {
            return m_arraySize;
        }

        bool IsFull() const
        {
            return m_numEntries == m_arraySize;
        }

        bool IsEmpty() const
        {
            return m_numEntries == 0;
        }

        int GetNumEntries() const
        {
            return m_numEntries;
        }
    };
}

#endif // #ifndef YOJIMBO_BITPACK_H
