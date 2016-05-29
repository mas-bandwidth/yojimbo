/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#ifndef YOJIMBO_TYPES_H
#define YOJIMBO_TYPES_H

#include <stdint.h>

namespace yojimbo
{
    class Allocator;

    template<typename T> struct Array
    {
        Array( Allocator & a );
        ~Array();

        Array( const Array & other );
        Array & operator = ( const Array & other );
        
        T & operator [] ( uint32_t i );
        const T & operator [] ( uint32_t i ) const;

        Allocator * m_allocator;
        uint32_t m_size;
        uint32_t m_capacity;
        T * m_data;
    };

    template <typename T> struct Queue
    {
        Queue( Allocator & a );

        T & operator [] ( uint32_t i );
        const T & operator [] ( uint32_t i ) const;

        Array<T> m_data;
        uint32_t m_size;
        uint32_t m_offset;
    };

    template<typename T> struct Hash
    {
        Hash( Allocator & a );
        
        struct Entry 
        {
            uint64_t key;
            uint32_t next;
            T value;
        };

        Array<uint32_t> _hash;
        Array<Entry> _data;
    };
}

#endif // #ifndef YOJIMBO_TYPES
