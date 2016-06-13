/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
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
