/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo_memory.h"

namespace yojimbo
{
	struct MemoryGlobals 
	{
#if YOJIMBO_SCRATCH_ALLOCATOR
		static const int ALLOCATOR_MEMORY = sizeof( MallocAllocator ) + sizeof( ScratchAllocator );
#else // #if YOJIMBO_SCRATCH_ALLOCATOR
		static const int ALLOCATOR_MEMORY = sizeof( MallocAllocator ) + sizeof( MallocAllocator );
#endif // #if YOJIMBO_SCRATCH_ALLOCATOR

		uint8_t buffer[ALLOCATOR_MEMORY];

		MallocAllocator * default_allocator;

#if YOJIMBO_SCRATCH_ALLOCATOR
		ScratchAllocator * scratch_allocator;
#else // #if YOJIMBO_SCRATCH_ALLOCATOR
		MallocAllocator * scratch_allocator;
#endif // #if YOJIMBO_SCRATCH_ALLOCATOR

		MemoryGlobals() : default_allocator( NULL ), scratch_allocator( NULL ) {}
	};

	MemoryGlobals memory_globals;

#if YOJIMBO_SCRATCH_ALLOCATOR
	void memory_initialize( uint32_t temporary_memory ) 
#else // #if YOJIMBO_SCRATCH_ALLOCATOR
    void memory_initialize( uint32_t /*temporary_memory*/ ) 
#endif // #if YOJIMBO_SCRATCH_ALLOCATOR
	{
		uint8_t * p = memory_globals.buffer;
		memory_globals.default_allocator = new (p) MallocAllocator();
		p += sizeof( MallocAllocator );
#if YOJIMBO_SCRATCH_ALLOCATOR
		memory_globals.scratch_allocator = new (p) ScratchAllocator( *memory_globals.default_allocator, temporary_memory );
#else // #if YOJIMBO_SCRATCH_ALLOCATOR
		memory_globals.scratch_allocator = new (p) MallocAllocator();
#endif // #if YOJIMBO_SCRATCH_ALLOCATOR
	}

	Allocator & memory_default_allocator() 
	{
		assert( memory_globals.default_allocator );
		return *memory_globals.default_allocator;
	}

	Allocator & memory_scratch_allocator() 
	{
		assert( memory_globals.scratch_allocator );
		return *memory_globals.scratch_allocator;
	}

	void memory_shutdown() 
	{
#if YOJIMBO_SCRATCH_ALLOCATOR
		memory_globals.scratch_allocator->~ScratchAllocator();
#else // #if YOJIMBO_SCRATCH_ALLOCATOR
		memory_globals.scratch_allocator->~MallocAllocator();
#endif // #if YOJIMBO_SCRATCH_ALLOCATOR
		memory_globals.default_allocator->~MallocAllocator();
		memory_globals = MemoryGlobals();
	}
}
