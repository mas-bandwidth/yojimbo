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

#include "yojimbo.h"
#include <stdio.h>
#include <stdlib.h>

using namespace yojimbo;

int NetworkInfoMain()
{
    const int MaxAddresses = 64;

    int numAddresses;
    Address networkAddresses[MaxAddresses];
    GetNetworkAddresses( networkAddresses, numAddresses, MaxAddresses );    

    printf( "\nnetwork addresses:\n" );

    for ( int i = 0; i < numAddresses; ++i )
    {
        char addressString[64];
        networkAddresses[i].ToString( addressString, sizeof( addressString) );
        printf( " + %s\n", addressString );
    }

    {
        Address address = GetFirstNetworkAddress_IPV4();
        char addressString[64];
        address.ToString( addressString, sizeof( addressString) );
        printf( "\nfirst IPV4 network address: %s\n", addressString );
    }

    {
        Address address = GetFirstNetworkAddress_IPV6();
        char addressString[64];
        address.ToString( addressString, sizeof( addressString) );
        printf( "\nfirst IPV6 network address: %s\n", addressString );
    }

    printf( "\n" );

    return 0;
}

int main()
{
    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize yojimbo\n" );
        exit( 1 );
    }

    int result = NetworkInfoMain();

    ShutdownYojimbo();

    return result;
}
