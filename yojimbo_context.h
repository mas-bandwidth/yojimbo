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

#ifndef YOJIMBO_CONTEXT_H
#define YOJIMBO_CONTEXT_H

#include "yojimbo_config.h"
#include "yojimbo_network.h"

#include <stdint.h>

namespace yojimbo
{
    // todo: this should go away and be replaced with a dynamic allocation on transport driven by # of connections expected.
    const int MaxContextMappings = 1024;

    class Allocator;
    class MessageFactory;

    struct Context
    {
        Allocator * streamAllocator;
        MessageFactory * messageFactory;

        Context()
        {
            streamAllocator = NULL;
            messageFactory = NULL;
        }
    };

    class ContextManager
    {
    public:

        ContextManager();

        bool AddContextMapping( const Address & address, Allocator & streamAllocator, MessageFactory * messageFactory );

        bool RemoveContextMapping( const Address & address );

        void ResetContextMappings();

        Context * GetContext( const Address & address );

    private:

        int m_numContextMappings;
        Address m_address[MaxContextMappings];
        Context m_context[MaxContextMappings];
    };
}

#endif // #ifndef YOJIMBO_CONTEXT_H
