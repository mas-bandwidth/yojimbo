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

#ifndef YOJIMBO_NETWORK_H
#define YOJIMBO_NETWORK_H

#include "yojimbo_config.h"
#include "yojimbo_address.h"
#include "yojimbo_packet.h"

#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace yojimbo
{
    /** 
        Initialize the network layer.

        On windows this calls in to WSAStartup.

        IMPORTANT: You must call this before you create any sockets.

        @see Socket
        @see IsNetworkInitialized
        @see ShutdownNetwork
     */

    bool InitializeNetwork();

    /**
        Is the network layer initialized?

        @returns True if the network layer is initialized, false otherwise.

        @see InitializeNetwork
        @see ShutdownNetwork
     */

    bool IsNetworkInitialized();

    /**
        Shut down the network layer.

        Call this after you have destroyed all sockets to shut down the network layer.
     */

    void ShutdownNetwork();

    /**
        Address filter for GetNetworkAddresses function.
     */

    enum AddressFilter
    {
        ADDRESS_FILTER_BOTH,                                    ///< Get both IPv4 and IPv6 addresses.
        ADDRESS_FILTER_IPV4_ONLY,                               ///< IPv4 addresses only.
        ADDRESS_FILTER_IPV6_ONLY                                ///< IPv6 addresses only.
    };

    /**
        Get the list of network interface addresses available on the system.

        IMPORTANT: Loopback and link local addresses are filtered out.

        @param addresses Pointer to an array of addresses to be filled with addresses on the system [out].
        @param numAddresses The number of addresses that were written to the array [out].
        @param maxAddresses The maximum number of addresses that can be written to the array.
        @param filter The filter specifying the type of addresses to retrieve.
     */

    void GetNetworkAddresses( Address * addresses, int & numAddresses, int maxAddresses, AddressFilter filter = ADDRESS_FILTER_BOTH );
}

#endif // #ifndef YOJIMBO_NETWORK_H
