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
