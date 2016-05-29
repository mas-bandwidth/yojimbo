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

#ifndef YOJIMBO_HASH_H
#define YOJIMBO_HASH_H

#include "yojimbo_array.h"

namespace yojimbo
{
    template<typename T> bool hash_has( const Hash<T> & h, uint64_t key );

    template<typename T> const T & hash_get( const Hash<T> & h, uint64_t key, const T & default_value );

    template<typename T> void hash_set( Hash<T> & h, uint64_t key, const T & value );

    template<typename T> void hash_remove( Hash<T> & h, uint64_t key );

    template<typename T> void hash_reserve( Hash<T> & h, uint32_t size );

    template<typename T> void hash_clear( Hash<T> & h );

    template<typename T> const typename Hash<T>::Entry * hash_begin( const Hash<T> & h );

    template<typename T> const typename Hash<T>::Entry * hash_end( const Hash<T> & h );

    template<typename T> const typename Hash<T>::Entry * multi_hash_find_first( const Hash<T> & h, uint64_t key );

    template<typename T> const typename Hash<T>::Entry * multi_hash_find_next( const Hash<T> & h, const typename Hash<T>::Entry * e );

    template<typename T> uint32_t multi_hash_count( const Hash<T> & h, uint64_t key );

    template<typename T> void get( const Hash<T> & h, uint64_t key, Array<T> & items );

    template<typename T> void insert( Hash<T> & h, uint64_t key, const T & value );

    template<typename T> void remove( Hash<T> & h, const typename Hash<T>::Entry * e );

    template<typename T> void remove_all( Hash<T> & h, uint64_t key );

    const uint32_t HASH_END_OF_LIST = 0xffffffffu;
    
    struct HashFindResult
    {
        uint32_t hash_i;
        uint32_t data_prev;
        uint32_t data_i;
    };  

    template<typename T> uint32_t hash_internal_add_entry( Hash<T> & h, uint64_t key )
    {
        typename Hash<T>::Entry e;
        e.key = key;
        e.next = HASH_END_OF_LIST;
        uint32_t ei = array_size( h._data );
        array_push_back( h._data, e );
        return ei;
    }

    template<typename T> HashFindResult hash_internal_find( const Hash<T> & h, uint64_t key )
    {
        HashFindResult fr;
        fr.hash_i = HASH_END_OF_LIST;
        fr.data_prev = HASH_END_OF_LIST;
        fr.data_i = HASH_END_OF_LIST;

        if ( array_size( h._hash ) == 0 )
            return fr;

        fr.hash_i = key % array_size( h._hash );
        fr.data_i = h._hash[fr.hash_i];
        while ( fr.data_i != HASH_END_OF_LIST )
        {
            if ( h._data[fr.data_i].key == key )
                return fr;
            fr.data_prev = fr.data_i;
            fr.data_i = h._data[fr.data_i].next;
        }
        return fr;
    }

    template<typename T> HashFindResult hash_internal_find( const Hash<T> &h, const typename Hash<T>::Entry * e )
    {
        HashFindResult fr;
        fr.hash_i = HASH_END_OF_LIST;
        fr.data_prev = HASH_END_OF_LIST;
        fr.data_i = HASH_END_OF_LIST;

        if ( array_size( h._hash ) == 0 )
            return fr;

        fr.hash_i = e->key % array_size( h._hash );
        fr.data_i = h._hash[fr.hash_i];
        while ( fr.data_i != HASH_END_OF_LIST ) 
        {
            if ( &h._data[fr.data_i] == e )
                return fr;
            fr.data_prev = fr.data_i;
            fr.data_i = h._data[fr.data_i].next;
        }
        return fr;
    }

    template<typename T> void hash_internal_erase( Hash<T> & h, const HashFindResult & fr )
    {
        if ( fr.data_prev == HASH_END_OF_LIST )
            h._hash[fr.hash_i] = h._data[fr.data_i].next;
        else
            h._data[fr.data_prev].next = h._data[fr.data_i].next;

        if ( fr.data_i == array_size( h._data ) - 1 ) 
        {
            array_pop_back( h._data );
            return;
        }

        h._data[fr.data_i] = h._data[array_size(h._data) - 1];
        HashFindResult last = hash_internal_find( h, h._data[fr.data_i].key );

        if ( last.data_prev != HASH_END_OF_LIST )
            h._data[last.data_prev].next = fr.data_i;
        else
            h._hash[last.hash_i] = fr.data_i;
    }

    template<typename T> uint32_t hash_internal_find_or_fail( const Hash<T> & h, uint64_t key )
    {
        return hash_internal_find( h, key ).data_i;
    }

    template<typename T> uint32_t hash_internal_find_or_make( Hash<T> & h, uint64_t key )
    {
        const HashFindResult fr = find( h, key );
        if (fr.data_i != HASH_END_OF_LIST)
            return fr.data_i;

        uint32_t i = add_entry(h, key);
        if (fr.data_prev == HASH_END_OF_LIST)
            h._hash[fr.hash_i] = i;
        else
            h._data[fr.data_prev].next = i;
        return i;
    }

    template<typename T> uint32_t hash_internal_make( Hash<T> & h, uint64_t key )
    {
        const HashFindResult fr = find( h, key );
        const uint32_t i = hash_internal_add_entry( h, key );

        if ( fr.data_prev == HASH_END_OF_LIST )
            h._hash[fr.hash_i] = i;
        else
            h._data[fr.data_prev].next = i;

        h._data[i].next = fr.data_i;

        return i;
    }   

    template<typename T> void hash_internal_find_and_erase( Hash<T> &h, uint64_t key )
    {
        const HashFindResult fr = find( h, key );
        if ( fr.data_i != HASH_END_OF_LIST )
            erase(h, fr);
    }

    template<typename T> void hash_internal_rehash( Hash<T> & h, uint32_t new_size )
    {
        Hash<T> nh( *h._hash.m_allocator );
        array_resize( nh._hash, new_size );
        array_reserve( nh._data, array_size(h._data) );
        for ( uint32_t i = 0; i < new_size; ++i )
            nh._hash[i] = HASH_END_OF_LIST;
        for ( uint32_t i = 0; i < array_size(h._data); ++i ) 
        {
            const typename Hash<T>::Entry &e = h._data[i];
            multi_hash_insert( nh, e.key, e.value );
        }

        Hash<T> empty( *h._hash.m_allocator );
        h.~Hash<T>();
        memcpy( &h, &nh, sizeof( Hash<T> ) );
        memcpy( &nh, &empty, sizeof( Hash<T> ) );
    }

    template<typename T> bool hash_internal_full( const Hash<T> & h )
    {
        const float max_load_factor = 0.7f;
        return array_size( h._data ) >= array_size( h._hash ) * max_load_factor;
    }

    template<typename T> void hash_internal_grow( Hash<T> & h )
    {
        const uint32_t new_size = array_size(h._data) * 2 + 10;
        hash_internal_rehash( h, new_size );
    }

    template<typename T> bool hash_has( const Hash<T> & h, uint64_t key )
    {
        return hash_internal_find_or_fail( h, key ) != HASH_END_OF_LIST;
    }

    template<typename T> const T & hash_get( const Hash<T> & h, uint64_t key, const T & default_value )
    {
        const uint32_t i = hash_internal_find_or_fail(h, key);
        return i == HASH_END_OF_LIST ? default_value : h._data[i].value;
    }

    template<typename T> void hash_set( Hash<T> & h, uint64_t key, const T & value )
    {
        if ( array_size( h._hash ) == 0 )
            hash_internal_grow( h );

        const uint32_t i = hash_internal_find_or_make( h, key );
        h._data[i].value = value;
        if ( hash_internal_full( h ) )
            hash_internal_grow( h );
    }

    template<typename T> void hash_remove( Hash<T> & h, uint64_t key )
    {
        hash_internal_find_and_erase( h, key );
    }

    template<typename T> void hash_reserve( Hash<T> & h, uint32_t size )
    {
        hash_internal_rehash( h, size );
    }

    template<typename T> void hash_clear( Hash<T> & h )
    {
        array_clear( h._data );
        array_clear( h._hash );
    }

    template<typename T> const typename Hash<T>::Entry * hash_begin( const Hash<T> & h )
    {
        return array_begin( h._data );
    }

    template<typename T> const typename Hash<T>::Entry * hash_end( const Hash<T> & h )
    {
        return array_end( h._data );
    }

    template<typename T> const typename Hash<T>::Entry * multi_hash_find_first( const Hash<T> &h, uint64_t key )
    {
        const uint32_t i = hash_internal_find_or_fail(h, key);
        return i == HASH_END_OF_LIST ? 0 : &h._data[i];
    }

    template<typename T> const typename Hash<T>::Entry * multi_hash_find_next( const Hash<T> & h, const typename Hash<T>::Entry * e )
    {
        uint32_t i = e->next;
        while ( i != HASH_END_OF_LIST ) 
        {
            if ( h._data[i].key == e->key )
                return &h._data[i];
            i = h._data[i].next;
        }
        return 0;
    }

    template<typename T> uint32_t multi_hash_count( const Hash<T> & h, uint64_t key )
    {
        uint32_t i = 0;
        const typename Hash<T>::Entry * e = multi_hash_find_first( h, key );
        while ( e ) 
        {
            ++i;
            e = multi_hash_find_next( h, e );
        }
        return i;
    }

    template<typename T> void multi_hash_get( const Hash<T> & h, uint64_t key, Array<T> & items )
    {
        const typename Hash<T>::Entry * e = multi_hash_find_first( h, key );
        while ( e ) 
        {
            array_push_back( items, e->value );
            e = find_next( h, e );
        }
    }

    template<typename T> void multi_hash_insert( Hash<T> & h, uint64_t key, const T & value )
    {
        if ( array_size( h._hash ) == 0 )
            hash_internal_grow( h );

        const uint32_t i = hash_internal_make( h, key );
        h._data[i].value = value;
        if ( hash_internal_full( h ) )
            hash_internal_grow( h );
    }

    template<typename T> void multi_hash_remove( Hash<T> & h, const typename Hash<T>::Entry * e )
    {
        const HashFindResult fr = hash_internal_find( h, e );
        if ( fr.data_i != HASH_END_OF_LIST )
            hash_internal_erase( h, fr );
    }

    template<typename T> void multi_hash_remove_all( Hash<T> & h, uint64_t key )
    {
        while ( hash_has( h, key ) )
            hash_remove( h, key );
    }

    template <typename T> Hash<T>::Hash( Allocator & a ) : _hash(a), _data(a) {}
}

#endif // #ifndef YOJIMBO_HASH
