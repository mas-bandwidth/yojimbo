/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2026, Más Bandwidth LLC.

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

#ifndef YOJIMBO_ADDRESS_CONVERSION_H
#define YOJIMBO_ADDRESS_CONVERSION_H

// Internal header, shared by yojimbo_client.cpp and yojimbo_server.cpp. Not installed:
// it names netcode types, and netcode.h is not part of the yojimbo public interface.

#include "yojimbo_address.h"
#include "netcode.h"
#include <string.h>

namespace yojimbo
{
    inline Address AddressFromNetcode( const netcode_address_t & address )
    {
        if ( address.type == NETCODE_ADDRESS_IPV4 )
            return Address( address.data.ipv4, address.port );
        if ( address.type == NETCODE_ADDRESS_IPV6 )
            return Address( address.data.ipv6, address.port );
        return Address();
    }

    inline bool AddressToNetcode( const Address & address, netcode_address_t & result )
    {
        memset( &result, 0, sizeof( result ) );
        result.port = address.GetPort();
        if ( address.GetType() == ADDRESS_IPV4 )
        {
            result.type = NETCODE_ADDRESS_IPV4;
            memcpy( result.data.ipv4, address.GetAddress4(), sizeof( result.data.ipv4 ) );
            return true;
        }
        if ( address.GetType() == ADDRESS_IPV6 )
        {
            result.type = NETCODE_ADDRESS_IPV6;
            memcpy( result.data.ipv6, address.GetAddress6(), sizeof( result.data.ipv6 ) );
            return true;
        }
        return false;
    }
}

#endif // #ifndef YOJIMBO_ADDRESS_CONVERSION_H
