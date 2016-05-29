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

#ifndef YOJIMBO_QUEUE_H
#define YOJIMBO_QUEUE_H

#include "yojimbo_array.h"
#include "yojimbo_types.h"

namespace yojimbo
{
    template<typename T> void queue_internal_increase_capacity( Queue<T> & q, uint32_t new_capacity )
    {
        uint32_t end = array_size( q.m_data );
        array_resize( q.m_data, new_capacity );
        if ( q.m_offset + q.m_size > end ) 
        {
            uint32_t end_items = end - q.m_offset;
            memmove( array_begin( q.m_data ) + new_capacity - end_items, array_begin( q.m_data ) + q.m_offset, end_items * sizeof(T) );
            q.m_offset += new_capacity - end;
        }
    }

    template<typename T> void queue_internal_grow( Queue<T> & q, uint32_t min_capacity = 0 )
    {
        uint32_t new_capacity = array_size( q.m_data ) * 2 + 8;
        if ( new_capacity < min_capacity )
            new_capacity = min_capacity;
        queue_internal_increase_capacity( q, new_capacity );
    }

    template<typename T> inline uint32_t queue_size( const Queue<T> & q )
    {
        return q.m_size;
    }

    template<typename T> inline uint32_t queue_space( const Queue<T> & q )
    {
        return array_size( q.m_data ) - q.m_size;
    }

    template<typename T> void queue_reserve( Queue<T> & q, uint32_t size )
    {
        if ( size > q.m_size )
            queue_internal_increase_capacity( q, size );
    }

    template<typename T> inline void queue_push_back( Queue<T> & q, const T & item )
    {
        if ( !queue_space( q ) )
            queue_internal_grow( q );
        q[q.m_size++] = item;
    }

    template<typename T> inline void queue_pop_back( Queue<T> & q )
    {
        --q.m_size;
    }
    
    template<typename T> inline void queue_push_front( Queue<T> & q, const T & item )
    {
        if ( !space( q ) )
            queue_internal_grow( q );
        q.m_offset = ( q.m_offset - 1 + array_size( q.m_data ) ) % array_size( q.m_data );
        ++q.m_size;
        q[0] = item;
    }
    
    template<typename T> inline void queue_pop_front( Queue<T> & q )
    {
        q.m_offset = ( q.m_offset + 1 ) % array_size( q.m_data );
        --q.m_size;
    }

    template<typename T> inline void queue_consume( Queue<T> & q, uint32_t n )
    {
        q.m_offset = ( q.m_offset + n) % array_size( q.m_data );
        q.m_size -= n;
    }

    template<typename T> void queue_clear( Queue<T> & q )
    {
        queue_consume( q, queue_size(q) );
    }

    template<typename T> void queue_push( Queue<T> & q, const T * items, uint32_t n )
    {
        if ( space(q) < n )
            queue_internal_grow( q, size( q ) + n );
        const uint32_t size = array_size( q.m_data );
        const uint32_t insert = ( q.m_offset + q.m_size ) % size;
        uint32_t to_insert = n;
        if ( insert + to_insert > size )
            to_insert = size - insert;
        memcpy( array_begin(q.m_data) + insert, items, to_insert * sizeof(T) );
        q.m_size += to_insert;
        items += to_insert;
        n -= to_insert;
        memcpy( array_begin(q.m_data), items, n * sizeof(T) );
        q.m_size += n;
    }

    template<typename T> inline T * queue_begin_front( Queue<T> & q )
    {
        return array_begin( q.m_data ) + q.m_offset;
    }

    template<typename T> inline const T * queue_begin_front( const Queue<T> & q )
    {
        return array_begin( q.m_data ) + q.m_offset;
    }

    template<typename T> T * queue_end_front( Queue<T> & q )
    {
        uint32_t end = q.m_offset + q.m_size;
        return end > array_size( q.m_data ) ? array_end( q.m_data ) : array_begin( q.m_data ) + end;
    }
    template<typename T> const T * queue_end_front(const Queue<T> &q)
    {
        uint32_t end = q.m_offset + q.m_size;
        return end > array_size( q.m_data ) ? array_end( q.m_data ) : array_begin( q.m_data ) + end;
    }

    template <typename T> inline Queue<T>::Queue( Allocator & allocator) : m_data( allocator ), m_size(0), m_offset(0) {}

    template <typename T> inline T & Queue<T>::operator [] ( uint32_t i )
    {
        return m_data[ ( i + m_offset ) % array_size( m_data ) ];
    }

    template <typename T> inline const T & Queue<T>::operator [] ( uint32_t i ) const
    {
        return m_data[ ( i + m_offset ) % array_size( m_data ) ];
    }
}

#endif // #ifndef YOJIMBO_QUEUE
