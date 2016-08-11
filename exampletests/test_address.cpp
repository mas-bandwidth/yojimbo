#include "yojimbo.h"
#include "gtest/gtest.h"
#include "asserts.h"

using namespace yojimbo;


TEST(Address, CreateIpv4Direct)
{
    Address address( 127, 0, 0, 1, 1000 );

    ASSERT_TRUE( address.IsValid() );
    ASSERT_EQ( address.GetType(), ADDRESS_IPV4 );
    ASSERT_EQ( address.GetPort(), 1000 );
    ASSERT_EQ( address.GetAddress4(), (uint32_t)0x100007f );
}

TEST(Address, CreateIpv6Direct)
{
    const uint16_t addr[] = { 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329 };

    Address address( addr[0], addr[1], addr[2], addr[3],
                     addr[4], addr[5], addr[6], addr[7] );

    ASSERT_TRUE( address.IsValid() );
    ASSERT_EQ( address.GetType(), ADDRESS_IPV6 );
    ASSERT_EQ( address.GetPort(), 0 );
    ASSERT_TRUE( isAddress6Match( address.GetAddress6(), addr));
}

TEST(Address, CreateFromString)
{
    Address address( "127.0.0.1" );

    ASSERT_TRUE( address.IsValid() );
    ASSERT_EQ( address.GetType(), ADDRESS_IPV4 );
    ASSERT_EQ( address.GetPort(), 0 );
    ASSERT_EQ( address.GetAddress4(), (uint32_t)0x100007f );
}


TEST(Address, Clear)
{
    Address address( 127, 0, 0, 1, 1000 );
    address.Clear();

    ASSERT_FALSE( address.IsValid() );
    ASSERT_EQ( address.GetType(), ADDRESS_NONE );
    ASSERT_EQ( address.GetPort(), 0 );
    // ASSERT_EQ( address.GetAddress4(), (uint32_t)0x0 );
}

TEST(Address, ToString)
{
    char buffer[MaxAddressLength];

    Address address ( 127, 0, 0, 1, 1000 );
    ASSERT_STREQ( address.ToString( buffer, MaxAddressLength ), "127.0.0.1:1000" );

    address = Address( 127, 0, 0, 1 );
    ASSERT_STREQ( address.ToString( buffer, MaxAddressLength ), "127.0.0.1" );

    address = Address( 255, 255, 255, 255, 65535 );
    ASSERT_STREQ( address.ToString( buffer, MaxAddressLength ), "255.255.255.255:65535" );

    address = Address( 10, 24, 168, 192, 3000 );
    ASSERT_STREQ( address.ToString( buffer, MaxAddressLength ), "10.24.168.192:3000" );

    address = Address( 0, 0, 0, 0, 0, 0, 0, 1 );
    ASSERT_STREQ( address.ToString( buffer, MaxAddressLength ), "::1" );

    address = Address( 0xFE80, 0x0000, 0x0000, 0x0000, 0x0202, 0xB3FF, 0xFE1E, 0x8329, 0xFE80 );
    ASSERT_STREQ( address.ToString( buffer, MaxAddressLength ), "[fe80::202:b3ff:fe1e:8329]:65152" );
}

TEST(Address, IsLinkLocal)
{
    Address address( 0xfe80, 0, 0, 0, 0, 0, 0, 0);
    ASSERT_TRUE( address.IsLinkLocal() );

    address = Address( 0, 0, 0, 0, 0, 0, 0, 0);
    ASSERT_FALSE( address.IsLinkLocal() );
}

TEST(Address, IsSiteLocal)
{
    Address address( 0xFEC0, 0, 0, 0, 0, 0, 0, 0, 0);
    ASSERT_TRUE( address.IsSiteLocal() );

    address = Address( 0, 0, 0, 0, 0, 0, 0, 0);
    ASSERT_FALSE( address.IsSiteLocal() );
}

TEST(Address, IsMulticast)
{
    Address address( 0xFF00, 0, 0, 0, 0, 0, 0, 0, 0);
    ASSERT_TRUE( address.IsMulticast() );

    address = Address( 0, 0, 0, 0, 0, 0, 0, 0);
    ASSERT_FALSE( address.IsMulticast() );
}

TEST(Address, IsLoopback)
{
    Address address( 127, 0, 0, 1, 1000 );
    ASSERT_TRUE( address.IsLoopback() );

    address = Address( 0, 0, 0, 0, 0, 0, 0, 1 );
    ASSERT_TRUE( address.IsLoopback() );

    address = Address( 10, 0, 0, 1, 1000 );
    ASSERT_FALSE( address.IsLoopback() );
}
