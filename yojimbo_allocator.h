/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#ifndef YOJIMBO_ALLOCATOR_H
#define YOJIMBO_ALLOCATOR_H

#include <stdint.h>

namespace yojimbo
{
	// allocator base class

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
}

#endif
