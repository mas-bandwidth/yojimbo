/*
    Yojimbo Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.
*/

#ifndef YOJIMBO_ADAPTER_H
#define YOJIMBO_ADAPTER_H

#include <assert.h>
#include "yojimbo_config.h"
#include "yojimbo_message.h"
#include "yojimbo_allocator.h"

/** @file */

namespace yojimbo
{
    /** 
        Adapter class
     */

    class Adapter
    {
    public:

        virtual ~Adapter() {}

        virtual Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes )
        {
            return YOJIMBO_NEW( allocator, TLSF_Allocator, memory, bytes );
        }

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator )
        {
            (void) allocator;
            assert( false );
            return NULL;
        }
    };
}

#endif // #ifndef YOJIMBO_ADAPTER_H
