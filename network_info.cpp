/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#include "yojimbo.h"
#include <stdio.h>
#include <stdlib.h>

using namespace yojimbo;

int main()
{
    memory_initialize();
    
    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize yojimbo\n" );
        exit( 1 );
    }

    if ( !InitializeNetwork() )
    {
        printf( "error: failed to initialize network\n" );
        exit( 1 );
    }

    const int MaxInterfaces = 64;

    int numInterfaces;
    NetworkInterfaceInfo interfaceInfo[MaxInterfaces];
    GetNetworkInterfaceInfo( interfaceInfo, numInterfaces, MaxInterfaces );    

    printf( "\navailable network interfaces:\n" );

    for ( int i = 0; i < numInterfaces; ++i )
    {
        char addressString[64];
        interfaceInfo[i].address.ToString( addressString, sizeof( addressString) );
        printf( " + %s => %s\n", interfaceInfo[i].name, addressString );
    }

    {
        Address address = GetFirstLocalAddress_IPV4();
        char addressString[64];
        address.ToString( addressString, sizeof( addressString) );
        printf( "\nfirst local IPV4 address: %s\n", addressString );
    }

    {
        Address address = GetFirstLocalAddress_IPV6();
        if ( address.IsValid() )
        {
            char addressString[64];
            address.ToString( addressString, sizeof( addressString) );
            printf( "\nfirst local IPV6 address: %s\n", addressString );
        }
    }

    {
        Address address = GetFirstLocalAddress();
        char addressString[64];
        address.ToString( addressString, sizeof( addressString) );
        printf( "\nfirst local address (prefer IPV6): %s\n", addressString );
    }

    printf( "\n" );

    ShutdownNetwork();

    ShutdownYojimbo();

    memory_shutdown();

    return 0;
}
