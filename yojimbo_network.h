/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#ifndef YOJIMBO_NETWORK_H
#define YOJIMBO_NETWORK_H

#include "yojimbo_config.h"

#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct addrinfo;
struct sockaddr_in6;
struct sockaddr_storage;

namespace yojimbo
{
    bool InitializeNetwork();

    void ShutdownNetwork();

    bool IsNetworkInitialized();

    enum AddressType
    {
        ADDRESS_UNDEFINED,
        ADDRESS_IPV4,
        ADDRESS_IPV6
    };

    inline int random_int( int a, int b )
    {
        assert( a < b );
        int result = a + rand() % ( b - a + 1 );
        assert( result >= a );
        assert( result <= b );
        return result;
    }

    inline float random_float( float a, float b )
    {
        assert( a < b );
		float random = ( (float) rand() ) / (float) RAND_MAX;
		float diff = b - a;
		float r = random * diff;
		return a + r;
	}

    class Address
    {
        AddressType m_type;

        union
        {
            uint32_t m_address_ipv4;
            uint16_t m_address_ipv6[8];
        };

        uint16_t m_port;

   public:

        Address();

        Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port = 0 );

        explicit Address( uint32_t address, int16_t port = 0 );

        explicit Address( uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                          uint16_t e, uint16_t f, uint16_t g, uint16_t h,
                          uint16_t port = 0 );

        explicit Address( const uint16_t address[], uint16_t port = 0 );

        explicit Address( const sockaddr_storage & addr );

        explicit Address( const sockaddr_in6 & addr_ipv6 );

        explicit Address( addrinfo * p );

        explicit Address( const char * address );

        explicit Address( const char * address, uint16_t port );

        void Clear();

        uint32_t GetAddress4() const;

        const uint16_t * GetAddress6() const;

        void SetPort( uint16_t port );

        uint16_t GetPort() const;

        AddressType GetType() const;

        const char * ToString( char buffer[], int bufferSize ) const;

        bool IsValid() const;

        bool operator ==( const Address & other ) const;

        bool operator !=( const Address & other ) const;

    protected:

        void Parse( const char * address );
    };

#if YOJIMBO_SOCKETS

    enum SocketType
    {
        SOCKET_TYPE_IPV4,
        SOCKET_TYPE_IPV6
    };

    enum SocketError
    {
        SOCKET_ERROR_NONE,
        SOCKET_ERROR_CREATE_FAILED,
        SOCKET_ERROR_SET_NON_BLOCKING_FAILED,
        SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED,
        SOCKET_ERROR_BIND_IPV4_FAILED,
        SOCKET_ERROR_BIND_IPV6_FAILED
    };

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
	typedef uint64_t SocketHandle;
#else // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
	typedef int SocketHandle;
#endif // #if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS
						   
    class Socket
    {
    public:

        Socket( uint16_t port, SocketType type = SOCKET_TYPE_IPV6 );

        ~Socket();

        bool IsError() const;

        int GetError() const;

        bool SendPacket( const Address & address, const void * packetData, size_t packetBytes );
    
        int ReceivePacket( Address & from, void * packetData, int maxPacketSize );

    private:

        int m_error;
        uint16_t m_port;
        SocketHandle m_socket;
    };

#endif // #if YOJIMBO_SOCKETS

#if YOJIMBO_NETWORK_SIMULATOR

    // todo: rename to "NetworkSimulator"

    class Simulator
    {
        float m_latency;                                // latency in milliseconds
        float m_jitter;                                 // jitter in milliseconds +/-
        float m_packetLoss;                             // packet loss percentage
        float m_duplicates;                             // duplicate packet percentage

        int m_numEntries;                               // number of elements in the packet entry array.
        int m_currentIndex;                             // current index in the packet entry array. new packets are inserted here.

        struct Entry
        {
            Entry()
            {
                deliveryTime = 0.0;
                packetData = NULL;
                packetSize = 0;
            }

            Address from;                               // address this packet is from
            Address to;                                 // address this packet is sent to
            double deliveryTime;                        // delivery time for this packet
            uint8_t *packetData;                        // packet data (owns pointer)
            int packetSize;                             // size of packet in bytes
        };

        Entry * m_entries;                              // pointer to dynamically allocated packet entries. this is where buffered packets are stored.

        double m_currentTime;                           // current time from last call to update. initially 0.0

    public:

        Simulator( int numPackets = 1024 );
        ~Simulator();

        void SetLatency( float milliseconds );
        void SetJitter( float milliseconds );
        void SetPacketLoss( float percent );
        void SetDuplicates( float percent );
        
        void SendPacket( const Address & from, const Address & to, uint8_t *packetData, int packetSize );

        uint8_t * ReceivePacket( Address & from, Address & to, int & packetSize );

        void Update( double t );
    };

#endif // #if YOJIMBO_NETWORK_SIMULATOR
}

#endif // #ifndef YOJIMBO_NETWORK_H
