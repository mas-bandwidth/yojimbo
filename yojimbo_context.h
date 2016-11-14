/*
    Yojimbo Client/Server Network Protocol Library.

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

#ifndef YOJIMBO_CLIENT_SERVER_CONTEXT_H
#define YOJIMBO_CLIENT_SERVER_CONTEXT_H

#include "yojimbo_config.h"
#include "yojimbo_network.h"

#include <stdint.h>

// todo: rename to yojimbo_client_server_context.h

namespace yojimbo
{
    struct ClientServerContext : public ConnectionContext
    {
        class Allocator * allocator;
        class PacketFactory * packetFactory;
        class ReplayProtection * replayProtection;

        ClientServerContext()
        {
            allocator = NULL;
            packetFactory = NULL;
            replayProtection = NULL;
        }
    };

    class ClientServerContextManager
    {
    public:

        ClientServerContextManager();

        bool AddContextMapping( const Address & address, ClientServerContext * context );

        bool RemoveContextMapping( const Address & address );

        void ResetContextMappings();

        const ClientServerContext * GetContext( const Address & address ) const;

    private:

        int m_numContextMappings;
        Address m_address[MaxContextMappings];
        ClientServerContext * m_context[MaxContextMappings];
    };
}

#endif // #ifndef YOJIMBO_CLIENT_SERVER_CONTEXT_H
