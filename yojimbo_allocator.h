/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
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
	#define YOJIMBO_DELETE( a, T, p ) do { if (p) { (p)->~T(); (a).Free(p); } } while (0)	

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
	#define YOJIMBO_DELETE_ARRAY( a, array, count ) DeleteArray( a, array, count )
}

#endif
