/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
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
            m_entries = AllocateArray( allocator, size, m_entries );
        }

        ~Queue()
        {
            assert( m_allocator );
            assert( m_entries );
            DeleteArray( *m_allocator, m_entries, m_arraySize );
            m_arraySize = 0;
            m_startIndex = 0;
            m_numEntries = 0;
            m_entries = NULL;
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
            m_entries[m_startIndex+m_numEntries] = entry;
            m_numEntries++;
        }

        T & operator [] ( int index )
        {
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        const T & operator [] ( int index ) const
        {
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        int GetSize() const
        {
            return m_numEntries;
        }

        bool IsFull() const
        {
            return m_numEntries == m_arraySize;
        }

        bool IsEmpty() const
        {
            return m_numEntries == 0;
        }
    };
}

#endif // #ifndef YOJIMBO_BITPACK_H
