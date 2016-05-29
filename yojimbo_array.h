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

#ifndef YOJIMBO_ARRAY_H
#define YOJIMBO_ARRAY_H

#include <stddef.h>
#include "yojimbo_allocator.h"
#include "yojimbo_types.h"

namespace yojimbo
{
	template<typename T> inline uint32_t array_size( const Array<T> & a ) 		{ return a.m_size; }
	template<typename T> inline bool array_empty( const Array<T> & a ) 			{ return a.m_size == 0; }
	
	template<typename T> inline T * array_begin( Array<T> & a ) 				{ return a.m_data; }
	template<typename T> inline const T * array_begin( const Array<T> & a ) 	{ return a.m_data; }
	template<typename T> inline T * array_end( Array<T> & a ) 				    { return a.m_data + a.m_size; }
	template<typename T> inline const T * array_end( const Array<T> & a ) 	    { return a.m_data + a.m_size; }
	
	template<typename T> inline T & array_front( Array<T> & a ) 				{ return a.m_data[0]; }
	template<typename T> inline const T & array_front( const Array<T> & a ) 	{ return a.m_data[0]; }
	template<typename T> inline T & array_back( Array<T> & a ) 					{ return a.m_data[a.m_size-1]; }
	template<typename T> inline const T & arary_back(const Array<T> & a ) 		{ return a.m_data[a.m_size-1]; }

	template<typename T> inline void array_resize( Array<T> & a, uint32_t new_size );

	template<typename T> inline void array_clear( Array<T> & a ) { resize( a, 0 ); }
	template<typename T> inline void array_trim( Array<T> & a ) { set_capacity( a, a.m_size); }

	template<typename T> inline void array_set_capacity( Array<T> & a, uint32_t new_capacity );

	template<typename T> inline void array_reserve( Array<T> & a, uint32_t new_capacity )
	{
		if ( new_capacity > a.m_capacity )
			set_capacity( a, new_capacity );
	}

	template<typename T> void array_grow( Array<T> & a, uint32_t min_capacity )
	{
		uint32_t new_capacity = a.m_capacity * 2 + 8;
		if ( new_capacity < min_capacity )
			 new_capacity = min_capacity ;
		array_set_capacity( a, new_capacity );
	}

	template<typename T> void array_resize( Array<T> & a, uint32_t new_size )
	{
		if ( new_size > a.m_capacity )
			array_grow( a, new_size );
		a.m_size = new_size;
	}

	template<typename T> void array_set_capacity( Array<T> & a, uint32_t new_capacity )
	{
		if ( new_capacity == a.m_capacity )
			return;

		if ( new_capacity < a.m_size )
			array_resize( a, new_capacity );

		T * new_data = NULL;

		if ( new_capacity > 0 )
		{
			new_data = (T*) a.m_allocator->Allocate( sizeof(T)*new_capacity, alignof(T) );
			memcpy( new_data, a.m_data, sizeof(T) * a.m_size );
		}

		a.m_allocator->Free( a.m_data );
		a.m_data = new_data;
		a.m_capacity = new_capacity;
	}

	template<typename T> inline void array_push_back( Array<T> & a, const T &item )
	{
		if ( a.m_size + 1 > a.m_capacity )
			array_grow( a, a.m_size + 1 );
		a.m_data[a.m_size++] = item;
	}

	template<typename T> inline void array_pop_back( Array<T> & a )
	{
		a.m_size--;
	}

	template<typename T> inline Array<T>::Array( Allocator & allocator ) 
		: m_allocator( &allocator ), m_size(0), m_capacity(0), m_data(0) 
	{
		// ...
	}

	template<typename T> inline Array<T>::~Array()
	{
		m_allocator->Free( m_data );
	}

	template<typename T> Array<T>::Array( const Array<T> & other ) 
		: m_allocator( other.m_allocator ), m_size(0), m_capacity(0), m_data(0)
	{
		const uint32_t n = other.m_size;
		array_set_capacity( *this, n );
		memcpy( m_data, other.m_data, sizeof(T) * n );
		m_size = n;
	}

	template<typename T> Array<T> & Array<T>::operator = ( const Array<T> & other )
	{
		const uint32_t n = other.m_size;
		array_resize( *this, n );
		memcpy( m_data, other.m_data, sizeof(T) * n );
		return *this;
	}

	template<typename T> inline T & Array<T>::operator [] ( uint32_t i )
	{
		return m_data[i];
	}

	template<typename T> inline const T & Array<T>::operator [] ( uint32_t i ) const
	{
		return m_data[i];
	}
}

#endif // #ifndef YOJIMBO_ARRAY
