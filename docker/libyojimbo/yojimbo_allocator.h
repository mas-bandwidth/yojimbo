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

#ifndef YOJIMBO_ALLOCATOR_H
#define YOJIMBO_ALLOCATOR_H

#include <stdint.h>
#include <new>

namespace yojimbo
{
	class Allocator
	{
	public:

		static const uint32_t DEFAULT_ALIGN = 16;
		static const uint32_t SIZE_NOT_TRACKED = 0xffffffff;

		Allocator() {}

		virtual ~Allocator() {}
		
		virtual void * Allocate( uint32_t size, uint32_t align = DEFAULT_ALIGN ) = 0;

		virtual void Free( void * p ) = 0;

		virtual uint32_t GetAllocatedSize( void * p ) = 0;

		virtual uint32_t GetTotalAllocated() = 0;

	private:

	    Allocator( const Allocator & other );

	    Allocator & operator = ( const Allocator & other );
	};

#if defined( _MSC_VER )
	#define _ALLOW_KEYWORD_MACROS
#endif // #if defined( _MSC_VER )
	
#if !defined( alignof )
	#define alignof(x) __alignof(x)
#endif // #if !defined( alignof )

	#define YOJIMBO_NEW( a, T, ... ) ( new ((a).Allocate(sizeof(T), alignof(T))) T(__VA_ARGS__) )
	#define YOJIMBO_DELETE( a, T, p ) do { assert(p); (p)->~T(); (a).Free(p); p = NULL; } while (0)	

	template <typename T> T * AllocateArray( Allocator & allocator, int arraySize, T * /*dummy*/ )
	{
		T * array = (T*) allocator.Allocate( sizeof(T) * arraySize, alignof(T) );
		for ( int i = 0; i < arraySize; ++i )
			new( &array[i] ) T();
		return array;
	}

	template <typename T> void DeleteArray( Allocator & allocator, T * array, int arraySize )
	{
		for ( int i = 0; i < arraySize; ++i )
			(&array[i])->~T();
		allocator.Free( array );
	}

	#define YOJIMBO_NEW_ARRAY( a, T, count ) AllocateArray( a, count, (T*)NULL )
	#define YOJIMBO_DELETE_ARRAY( a, array, count ) do { DeleteArray( a, array, count ); array = NULL; } while (0)
}

#endif
