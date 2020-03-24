/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2019, The Network Protocol Company, Inc.

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

#ifndef YOJIMBO_H
#define YOJIMBO_H

/** @file */

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define YOJIMBO_MAJOR_VERSION 1
#define YOJIMBO_MINOR_VERSION 0
#define YOJIMBO_PATCH_VERSION 0

#if !defined (YOJIMBO_LITTLE_ENDIAN ) && !defined( YOJIMBO_BIG_ENDIAN )

  #ifdef __BYTE_ORDER__
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      #define YOJIMBO_LITTLE_ENDIAN 1
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      #define YOJIMBO_BIG_ENDIAN 1
    #else
      #error Unknown machine endianess detected. User needs to define YOJIMBO_LITTLE_ENDIAN or YOJIMBO_BIG_ENDIAN.
    #endif // __BYTE_ORDER__
  
  // Detect with GLIBC's endian.h
  #elif defined(__GLIBC__)
    #include <endian.h>
    #if (__BYTE_ORDER == __LITTLE_ENDIAN)
      #define YOJIMBO_LITTLE_ENDIAN 1
    #elif (__BYTE_ORDER == __BIG_ENDIAN)
      #define YOJIMBO_BIG_ENDIAN 1
    #else
      #error Unknown machine endianess detected. User needs to define YOJIMBO_LITTLE_ENDIAN or YOJIMBO_BIG_ENDIAN.
    #endif // __BYTE_ORDER
  
  // Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
  #elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
    #define YOJIMBO_LITTLE_ENDIAN 1
  #elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
    #define YOJIMBO_BIG_ENDIAN 1
  
  // Detect with architecture macros
  #elif    defined(__sparc)     || defined(__sparc__)                           \
        || defined(_POWER)      || defined(__powerpc__)                         \
        || defined(__ppc__)     || defined(__hpux)      || defined(__hppa)      \
        || defined(_MIPSEB)     || defined(_POWER)      || defined(__s390__)
    #define YOJIMBO_BIG_ENDIAN 1
  #elif    defined(__i386__)    || defined(__alpha__)   || defined(__ia64)      \
        || defined(__ia64__)    || defined(_M_IX86)     || defined(_M_IA64)     \
        || defined(_M_ALPHA)    || defined(__amd64)     || defined(__amd64__)   \
        || defined(_M_AMD64)    || defined(__x86_64)    || defined(__x86_64__)  \
        || defined(_M_X64)      || defined(__bfin__)
    #define YOJIMBO_LITTLE_ENDIAN 1
  #elif defined(_MSC_VER) && defined(_M_ARM)
    #define YOJIMBO_LITTLE_ENDIAN 1
  #else
    #error Unknown machine endianess detected. User needs to define YOJIMBO_LITTLE_ENDIAN or YOJIMBO_BIG_ENDIAN. 
  #endif
#endif

#ifndef YOJIMBO_LITTLE_ENDIAN
#define YOJIMBO_LITTLE_ENDIAN 0
#endif

#ifndef YOJIMBO_BIG_ENDIAN
#define YOJIMBO_BIG_ENDIAN 0
#endif

#ifndef YOJIMBO_DEFAULT_TIMEOUT
#define YOJIMBO_DEFAULT_TIMEOUT 5
#endif

#if !defined( YOJIMBO_WITH_MBEDTLS )
#define YOJIMBO_WITH_MBEDTLS 1
#endif // #if !defined( YOJIMBO_WITH_MBEDTLS )

#ifdef _MSC_VER
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#endif // #ifdef _MSC_VER

#define YOJIMBO_PLATFORM_WINDOWS                    1
#define YOJIMBO_PLATFORM_MAC                        2
#define YOJIMBO_PLATFORM_UNIX                       3

#if defined(_WIN32)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_MAC
#else
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_UNIX
#endif

#define YOJIMBO_SERIALIZE_CHECKS                    1

#ifndef NDEBUG

#define YOJIMBO_DEBUG_MEMORY_LEAKS                  1
#define YOJIMBO_DEBUG_MESSAGE_LEAKS                 1
#define YOJIMBO_DEBUG_MESSAGE_BUDGET                1

#else // #ifndef NDEBUG

#define YOJIMBO_DEBUG_MEMORY_LEAKS                  0
#define YOJIMBO_DEBUG_MESSAGE_LEAKS                 0
#define YOJIMBO_DEBUG_MESSAGE_BUDGET                0

#endif // #ifndef NDEBUG

#define YOJIMBO_ENABLE_LOGGING                      1

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <inttypes.h>
#if YOJIMBO_DEBUG_MESSAGE_LEAKS
#include <map>
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS

// windows =p
#ifdef SendMessage
#undef SendMessage
#endif

struct netcode_server_t;
struct netcode_client_t;
struct reliable_endpoint_t;

/// The library namespace.

namespace yojimbo
{
    const int MaxClients = 64;                                      ///< The maximum number of clients supported by this library. You can increase this if you want, but this library is designed around patterns that work best for [2,64] player games. If your game has less than 64 clients, reducing this will save memory.
    const int MaxChannels = 64;                                     ///< The maximum number of message channels supported by this library. If you need less than 64 channels per-packet, reducing this will save memory.
    const int KeyBytes = 32;                                        ///< Size of encryption key for dedicated client/server in bytes. Must be equal to key size for libsodium encryption primitive. Do not change.
    const int ConnectTokenBytes = 2048;                             ///< Size of the encrypted connect token data return from the matchmaker. Must equal size of NETCODE_CONNECT_TOKEN_BYTE (2048).
    const uint32_t SerializeCheckValue = 0x12345678;                ///< The value written to the stream for serialize checks. See WriteStream::SerializeCheck and ReadStream::SerializeCheck.
    const int ConservativeMessageHeaderBits = 32;                   ///< Conservative number of bits per-message header.
    const int ConservativeFragmentHeaderBits = 64;                  ///< Conservative number of bits per-fragment header.
    const int ConservativeChannelHeaderBits = 32;                   ///< Conservative number of bits per-channel header.
    const int ConservativePacketHeaderBits = 16;                    ///< Conservative number of bits per-packet header.

    /// Determines the reliability and ordering guarantees for a channel.

    enum ChannelType
    {
        CHANNEL_TYPE_RELIABLE_ORDERED,                              ///< Messages are received reliably and in the same order they were sent. 
        CHANNEL_TYPE_UNRELIABLE_UNORDERED                           ///< Messages are sent unreliably. Messages may arrive out of order, or not at all.
    };

    /** 
        Configuration properties for a message channel.
     
        Channels let you specify different reliability and ordering guarantees for messages sent across a connection.
     
        They may be configured as one of two types: reliable-ordered or unreliable-unordered.
     
        Reliable ordered channels guarantee that messages (see Message) are received reliably and in the same order they were sent. 
        This channel type is designed for control messages and RPCs sent between the client and server.
    
        Unreliable unordered channels are like UDP. There is no guarantee that messages will arrive, and messages may arrive out of order.
        This channel type is designed for data that is time critical and should not be resent if dropped, like snapshots of world state sent rapidly 
        from server to client, or cosmetic events such as effects and sounds.
        
        Both channel types support blocks of data attached to messages (see BlockMessage), but their treatment of blocks is quite different.
        
        Reliable ordered channels are designed for blocks that must be received reliably and in-order with the rest of the messages sent over the channel. 
        Examples of these sort of blocks include the initial state of a level, or server configuration data sent down to a client on connect. These blocks 
        are sent by splitting them into fragments and resending each fragment until the other side has received the entire block. This allows for sending
        blocks of data larger that maximum packet size quickly and reliably even under packet loss.
        
        Unreliable-unordered channels send blocks as-is without splitting them up into fragments. The idea is that transport level packet fragmentation
        should be used on top of the generated packet to split it up into into smaller packets that can be sent across typical Internet MTU (<1500 bytes). 
        Because of this, you need to make sure that the maximum block size for an unreliable-unordered channel fits within the maximum packet size.
        
        Channels are typically configured as part of a ConnectionConfig, which is included inside the ClientServerConfig that is passed into the Client and Server constructors.
     */

    struct ChannelConfig
    {
        ChannelType type;                                           ///< Channel type: reliable-ordered or unreliable-unordered.
        bool disableBlocks;                                         ///< Disables blocks being sent across this channel.
        int sentPacketBufferSize;                                   ///< Number of packet entries in the sent packet sequence buffer. Please consider your packet send rate and make sure you have at least a few seconds worth of entries in this buffer.
        int messageSendQueueSize;                                   ///< Number of messages in the send queue for this channel.
        int messageReceiveQueueSize;                                ///< Number of messages in the receive queue for this channel.
        int maxMessagesPerPacket;                                   ///< Maximum number of messages to include in each packet. Will write up to this many messages, provided the messages fit into the channel packet budget and the number of bytes remaining in the packet.
        int packetBudget;                                           ///< Maximum amount of message data to write to the packet for this channel (bytes). Specifying -1 means the channel can use up to the rest of the bytes remaining in the packet.
        int maxBlockSize;                                           ///< The size of the largest block that can be sent across this channel (bytes).
        int blockFragmentSize;                                      ///< Blocks are split up into fragments of this size (bytes). Reliable-ordered channel only.
        float messageResendTime;                                    ///< Minimum delay between message resends (seconds). Avoids sending the same message too frequently. Reliable-ordered channel only.
        float blockFragmentResendTime;                              ///< Minimum delay between block fragment resends (seconds). Avoids sending the same fragment too frequently. Reliable-ordered channel only.

        ChannelConfig() : type ( CHANNEL_TYPE_RELIABLE_ORDERED )
        {
            disableBlocks = false;
            sentPacketBufferSize = 1024;
            messageSendQueueSize = 1024;
            messageReceiveQueueSize = 1024;
            maxMessagesPerPacket = 256;
            packetBudget = -1;
            maxBlockSize = 256 * 1024;
            blockFragmentSize = 1024;
            messageResendTime = 0.1f;
            blockFragmentResendTime = 0.25f;
        }

        int GetMaxFragmentsPerBlock() const
        {
            return maxBlockSize / blockFragmentSize;
        }
    };

    /** 
        Configures connection properties and the set of channels for sending and receiving messages.
        Specifies the maximum packet size to generate, and the number of message channels, and the per-channel configuration data. See ChannelConfig for details.
        Typically configured as part of a ClientServerConfig which is passed into Client and Server constructors.
     */

    struct ConnectionConfig
    {
        int numChannels;                                        ///< Number of message channels in [1,MaxChannels]. Each message channel must have a corresponding configuration below.
        int maxPacketSize;                                      ///< The maximum size of packets generated to transmit messages between client and server (bytes).
        ChannelConfig channel[MaxChannels];                     ///< Per-channel configuration. See ChannelConfig for details.

        ConnectionConfig()
        {
            numChannels = 1;
            maxPacketSize = 8 * 1024;
        }
    };

    /** 
        Configuration shared between client and server.
        Passed to Client and Server constructors to configure their behavior.
        Please make sure that the message configuration is identical between client and server.
     */

    struct ClientServerConfig : public ConnectionConfig
    {
        uint64_t protocolId;                                    ///< Clients can only connect to servers with the same protocol id. Use this for versioning.
        int timeout;                                            ///< Timeout value in seconds. Set to negative value to disable timeouts (for debugging only).
        int clientMemory;                                       ///< Memory allocated inside Client for packets, messages and stream allocations (bytes)
        int serverGlobalMemory;                                 ///< Memory allocated inside Server for global connection request and challenge response packets (bytes)
        int serverPerClientMemory;                              ///< Memory allocated inside Server for packets, messages and stream allocations per-client (bytes)
        bool networkSimulator;                                  ///< If true then a network simulator is created for simulating latency, jitter, packet loss and duplicates.
        int maxSimulatorPackets;                                ///< Maximum number of packets that can be stored in the network simulator. Additional packets are dropped.
        int fragmentPacketsAbove;                               ///< Packets above this size (bytes) are split apart into fragments and reassembled on the other side.
        int packetFragmentSize;                                 ///< Size of each packet fragment (bytes).
        int maxPacketFragments;                                 ///< Maximum number of fragments a packet can be split up into.
        int packetReassemblyBufferSize;                         ///< Number of packet entries in the fragmentation reassembly buffer.
        int ackedPacketsBufferSize;                             ///< Number of packet entries in the acked packet buffer. Consider your packet send rate and aim to have at least a few seconds worth of entries.
        int receivedPacketsBufferSize;                          ///< Number of packet entries in the received packet sequence buffer. Consider your packet send rate and aim to have at least a few seconds worth of entries.

        ClientServerConfig()
        {
            protocolId = 0;
            timeout = YOJIMBO_DEFAULT_TIMEOUT;
            clientMemory = 10 * 1024 * 1024;
            serverGlobalMemory = 10 * 1024 * 1024;
            serverPerClientMemory = 10 * 1024 * 1024;
            networkSimulator = true;
            maxSimulatorPackets = 4 * 1024;
            fragmentPacketsAbove = 1024;
            packetFragmentSize = 1024;
            maxPacketFragments = (int) ceil( maxPacketSize / packetFragmentSize );
            packetReassemblyBufferSize = 64;
            ackedPacketsBufferSize = 256;
            receivedPacketsBufferSize = 256;
        }
    };
}

/**
    Initialize the yojimbo library.
    Call this before calling any yojimbo library functions.
    @returns True if the library was successfully initialized, false otherwise.
 */

bool InitializeYojimbo();

/**
    Shutdown the yojimbo library.
    Call this after you finish using the library and it will run some checks for you (for example, checking for memory leaks in debug build).
 */

void ShutdownYojimbo();

/**
    Template function to get the minimum of two values.
    @param a The first value.
    @param b The second value.
    @returns The minimum of a and b.
 */

template <typename T> const T & yojimbo_min( const T & a, const T & b )
{
    return ( a < b ) ? a : b;
}

/**
    Template function to get the maximum of two values.
    @param a The first value.
    @param b The second value.
    @returns The maximum of a and b.
 */

template <typename T> const T & yojimbo_max( const T & a, const T & b )
{
    return ( a > b ) ? a : b;
}

/**
    Template function to clamp a value.
    @param value The value to be clamped.
    @param a The minimum value.
    @param b The minimum value.
    @returns The clamped value in [a,b].
 */

template <typename T> T yojimbo_clamp( const T & value, const T & a, const T & b )
{
    if ( value < a )
        return a;
    else if ( value > b )
        return b;
    else
        return value;
}

/**
    Swap two values.
    @param a First value.
    @param b Second value.
 */

template <typename T> void yojimbo_swap( T & a, T & b )
{
    T tmp = a;
    a = b;
    b = tmp;
};

/**
    Get the absolute value.

    @param value The input value.

    @returns The absolute value.
 */

template <typename T> T yojimbo_abs( const T & value )
{
    return ( value < 0 ) ? -value : value;
}

/**
    Sleep for approximately this number of seconds.
    @param time number of seconds to sleep for.
 */

void yojimbo_sleep( double time );

/**
    Get a high precision time in seconds since the application has started.
    Please store time in doubles so you retain sufficient precision as time increases.
    @returns Time value in seconds.
 */

double yojimbo_time();

#define YOJIMBO_LOG_LEVEL_NONE      0
#define YOJIMBO_LOG_LEVEL_ERROR     1
#define YOJIMBO_LOG_LEVEL_INFO      2
#define YOJIMBO_LOG_LEVEL_DEBUG     3

/**
    Set the yojimbo log level.
    Valid log levels are: YOJIMBO_LOG_LEVEL_NONE, YOJIMBO_LOG_LEVEL_ERROR, YOJIMBO_LOG_LEVEL_INFO and YOJIMBO_LOG_LEVEL_DEBUG
    @param level The log level to set. Initially set to YOJIMBO_LOG_LEVEL_NONE.
 */

void yojimbo_log_level( int level );

/**
    Printf function used by yojimbo to emit logs.
    This function internally calls the printf callback set by the user. 
    @see yojimbo_set_printf_function
 */

void yojimbo_printf( int level, const char * format, ... );

extern void (*yojimbo_assert_function)( const char *, const char *, const char * file, int line );

/**
    Assert function used by yojimbo.
    This assert function lets the user override the assert presentation.
    @see yojimbo_set_assert_functio
 */

#ifndef NDEBUG
#define yojimbo_assert( condition )                                                         \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
        yojimbo_assert_function( #condition, __FUNCTION__, __FILE__, __LINE__ );            \
        exit(1);                                                                            \
    }                                                                                       \
} while(0)
#else
#define yojimbo_assert( ignore ) ((void)0)
#endif

/**
    Call this to set the printf function to use for logging.
    @param function The printf callback function.
 */

void yojimbo_set_printf_function( int (*function)( const char * /*format*/, ... ) );

/**
    Call this to set the function to call when an assert triggers.
    @param function The assert callback function.
 */

void yojimbo_set_assert_function( void (*function)( const char * /*condition*/, const char * /*function*/, const char * /*file*/, int /*line*/ ) );

#include <stdint.h>
#include <new>
#if YOJIMBO_DEBUG_MEMORY_LEAKS
#include <map>
#endif // YOJIMBO_DEBUG_MEMORY_LEAKS

typedef void* tlsf_t;

namespace yojimbo
{
    /**
        Get the default allocator.
        Use this allocator when you just want to use malloc/free, but in the form of a yojimbo allocator.
        This allocator instance is created inside InitializeYojimbo and destroyed in ShutdownYojimbo.
        In debug build, it will automatically check for memory leaks and print them out for you when you shutdown the library.
        @returns The default allocator instances backed by malloc and free.
     */

    class Allocator & GetDefaultAllocator();

    /// Macro for creating a new object instance with a yojimbo allocator.
    #define YOJIMBO_NEW( a, T, ... ) ( new ( (a).Allocate( sizeof(T), __FILE__, __LINE__ ) ) T(__VA_ARGS__) )

    /// Macro for deleting an object created with a yojimbo allocator.
    #define YOJIMBO_DELETE( a, T, p ) do { if (p) { (p)->~T(); (a).Free( p, __FILE__, __LINE__ ); p = NULL; } } while (0)    

    /// Macro for allocating a block of memory with a yojimbo allocator. 
    #define YOJIMBO_ALLOCATE( a, bytes ) (a).Allocate( (bytes), __FILE__, __LINE__ )

    /// Macro for freeing a block of memory created with a yojimbo allocator.
    #define YOJIMBO_FREE( a, p ) do { if ( p ) { (a).Free( p, __FILE__, __LINE__ ); p = NULL; } } while(0)

    /// Allocator error level.
    enum AllocatorErrorLevel
    {
        ALLOCATOR_ERROR_NONE = 0,                               ///< No error. All is well.
        ALLOCATOR_ERROR_OUT_OF_MEMORY                           ///< The allocator is out of memory!
    };

    /// Helper function to convert an allocator error to a user friendly string.
    inline const char * GetAllocatorErrorString( AllocatorErrorLevel error )
    {
        switch ( error )
        {
            case ALLOCATOR_ERROR_NONE:                  return "none";
            case ALLOCATOR_ERROR_OUT_OF_MEMORY:         return "out of memory";
            default:
                yojimbo_assert( false );
                return "(unknown)";
        }
    }

#if YOJIMBO_DEBUG_MEMORY_LEAKS

    /**
        Debug structure used to track allocations and find memory leaks. 
        Active in debug build only. Disabled in release builds for performance reasons.
     */

    struct AllocatorEntry
    {
        size_t size;                        ///< The size of the allocation in bytes.
        const char * file;                  ///< Filename of the source code file that made the allocation.
        int line;                           ///< Line number in the source code where the allocation was made.
    };

#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

    /**
        Functionality common to all allocators.
        Extend this class to hook up your own allocator to yojimbo.
        IMPORTANT: This allocator is not yet thread safe. Only call it from one thread!
     */

    class Allocator
    {
    public:

        /**
            Allocator constructor.
            Sets the error level to ALLOCATOR_ERROR_NONE.
         */

        Allocator();

        /**
            Allocator destructor.
            Make sure all allocations made from this allocator are freed before you destroy this allocator.
            In debug build, validates this is true walks the map of allocator entries. Any outstanding entries are considered memory leaks and printed to stdout.
         */

        virtual ~Allocator();

        /**
            Allocate a block of memory.
            IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.
            @param size The size of the block of memory to allocate (bytes).
            @param file The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the allocation.
            @returns A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.
            @see Allocator::Free
            @see Allocator::GetErrorLevel
         */

        virtual void * Allocate( size_t size, const char * file, int line ) = 0;

        /**
            Free a block of memory.
            IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.
            @param p Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise.
            @param file The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the free.
            @see Allocator::Allocate
            @see Allocator::GetErrorLevel
         */

        virtual void Free( void * p, const char * file, int line ) = 0;

        /**
            Get the allocator error level.
            Use this function to check if an allocation has failed. This is used in the client/server to disconnect a client with a failed allocation.
            @returns The allocator error level.
         */

        AllocatorErrorLevel GetErrorLevel() const { return m_errorLevel; }

        /**
            Clear the allocator error level back to default.
         */

        void ClearError() { m_errorLevel = ALLOCATOR_ERROR_NONE; }

    protected:

        /**
            Set the error level.
            For correct client/server behavior when an allocation fails, please make sure you call this method to set the error level to ALLOCATOR_ERROR_FAILED_TO_ALLOCATE.
            @param error The allocator error level to set.
         */

        void SetErrorLevel( AllocatorErrorLevel errorLevel );

        /**
            Call this function to track an allocation made by your derived allocator class.
            In debug build, tracked allocations are automatically checked for leaks when the allocator is destroyed.
            @param p Pointer to the memory that was allocated.
            @param size The size of the allocation in bytes.
            @param file The source code file that performed the allocation.
            @param line The line number in the source file where the allocation was performed.
         */

        void TrackAlloc( void * p, size_t size, const char * file, int line );

        /**
            Call this function to track a free made by your derived allocator class.
            In debug build, any allocation tracked without a corresponding free is considered a memory leak when the allocator is destroyed.
            @param p Pointer to the memory that was allocated.
            @param file The source code file that is calling in to free the memory.
            @param line The line number in the source file where the free is being called from.
         */

        void TrackFree( void * p, const char * file, int line );

        AllocatorErrorLevel m_errorLevel;                                       ///< The allocator error level.

#if YOJIMBO_DEBUG_MEMORY_LEAKS
        std::map<void*,AllocatorEntry> m_alloc_map;                             ///< Debug only data structure used to find and report memory leaks.
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

    private:

        Allocator( const Allocator & other );

        Allocator & operator = ( const Allocator & other );
    };

    /**
        The default allocator implementation based around malloc and free.
     */

    class DefaultAllocator : public Allocator
    {
    public:

        /**
            Default constructor.
         */

        DefaultAllocator() {}

        /**
            Allocates a block of memory using "malloc".
            IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.
            @param size The size of the block of memory to allocate (bytes).
            @param file The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the allocation.
            @returns A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.
         */

        void * Allocate( size_t size, const char * file, int line );

        /**
            Free a block of memory by calling "free".
            IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.
            @param p Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise.
            @param file The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the free.
         */

        void Free( void * p, const char * file, int line );

    private:

        DefaultAllocator( const DefaultAllocator & other );

        DefaultAllocator & operator = ( const DefaultAllocator & other );
    };

    /**
        An allocator built on the TLSF allocator implementation by Matt Conte. Thanks Matt!
        This is a fast allocator that supports multiple heaps. It's used inside the yojimbo server to silo allocations for each client to their own heap.
        See https://github.com/mattconte/tlsf for details on this allocator implementation.
     */

    class TLSF_Allocator : public Allocator
    {
    public:

        /**
            TLSF allocator constructor.
            If you want to integrate your own allocator with yojimbo for use with the client and server, this class is a good template to start from. 
            Make sure your constructor has the same signature as this one, and it will work with the YOJIMBO_SERVER_ALLOCATOR and YOJIMBO_CLIENT_ALLOCATOR helper macros.
            @param memory Block of memory in which the allocator will work. This block must remain valid while this allocator exists. The allocator does not assume ownership of it, you must free it elsewhere, if necessary.
            @param bytes The size of the block of memory (bytes). The maximum amount of memory you can allocate will be less, due to allocator overhead.
         */

        TLSF_Allocator( void * memory, size_t bytes );

        /**
            TLSF allocator destructor.
            Checks for memory leaks in debug build. Free all memory allocated by this allocator before destroying.
         */

        ~TLSF_Allocator();

        /**
            Allocates a block of memory using TLSF.
            IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.
            @param size The size of the block of memory to allocate (bytes).
            @param file The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the allocation.
            @returns A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.
         */

        void * Allocate( size_t size, const char * file, int line );

        /**
            Free a block of memory using TLSF.
            IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.
            @param p Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise.
            @param file The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the free.
            @see Allocator::Allocate
            @see Allocator::GetError
         */

        void Free( void * p, const char * file, int line );

    private:

        tlsf_t m_tlsf;              ///< The TLSF allocator instance backing this allocator.

        TLSF_Allocator( const TLSF_Allocator & other );
        TLSF_Allocator & operator = ( const TLSF_Allocator & other );
    };

    /**
        Generate cryptographically secure random data.
        @param data The buffer to store the random data.
        @param bytes The number of bytes of random data to generate.
     */

    void random_bytes( uint8_t * data, int bytes );

    /**
        Generate a random integer between a and b (inclusive).
        IMPORTANT: This is not a cryptographically secure random. It's used only for test functions and in the network simulator.
        @param a The minimum integer value to generate.
        @param b The maximum integer value to generate.
        @returns A pseudo random integer value in [a,b].
     */

    inline int random_int( int a, int b )
    {
        yojimbo_assert( a < b );
        int result = a + rand() % ( b - a + 1 );
        yojimbo_assert( result >= a );
        yojimbo_assert( result <= b );
        return result;
    }

    /** 
        Generate a random float between a and b.
        IMPORTANT: This is not a cryptographically secure random. It's used only for test functions and in the network simulator.
        @param a The minimum integer value to generate.
        @param b The maximum integer value to generate.
        @returns A pseudo random float value in [a,b].
     */

    inline float random_float( float a, float b )
    {
        yojimbo_assert( a < b );
        float random = ( (float) rand() ) / (float) RAND_MAX;
        float diff = b - a;
        float r = random * diff;
        return a + r;
    }

    /**
        Calculates the population count of an unsigned 32 bit integer at compile time.
        Population count is the number of bits in the integer that set to 1.
        See "Hacker's Delight" and http://www.hackersdelight.org/hdcodetxt/popArrayHS.c.txt
        @see yojimbo::Log2
        @see yojimbo::BitsRequired
     */

    template <uint32_t x> struct PopCount
    {
        enum {   a = x - ( ( x >> 1 )       & 0x55555555 ),
                 b =   ( ( ( a >> 2 )       & 0x33333333 ) + ( a & 0x33333333 ) ),
                 c =   ( ( ( b >> 4 ) + b ) & 0x0f0f0f0f ),
                 d =   c + ( c >> 8 ),
                 e =   d + ( d >> 16 ),

            result = e & 0x0000003f 
        };
    };

    /**
        Calculates the log 2 of an unsigned 32 bit integer at compile time.
        @see yojimbo::Log2
        @see yojimbo::BitsRequired
     */

    template <uint32_t x> struct Log2
    {
        enum {   a = x | ( x >> 1 ),
                 b = a | ( a >> 2 ),
                 c = b | ( b >> 4 ),
                 d = c | ( c >> 8 ),
                 e = d | ( d >> 16 ),
                 f = e >> 1,

            result = PopCount<f>::result
        };
    };

    /**
        Calculates the number of bits required to serialize an integer value in [min,max] at compile time.
        @see Log2
        @see PopCount
     */

    template <int64_t min, int64_t max> struct BitsRequired
    {
        static const uint32_t result = ( min == max ) ? 0 : ( Log2<uint32_t(max-min)>::result + 1 );
    };

    /**
        Calculates the population count of an unsigned 32 bit integer.
        The population count is the number of bits in the integer set to 1.
        @param x The input integer value.
        @returns The number of bits set to 1 in the input value.
     */

    inline uint32_t popcount( uint32_t x )
    {
#ifdef __GNUC__
        return __builtin_popcount( x );
#else // #ifdef __GNUC__
        const uint32_t a = x - ( ( x >> 1 )       & 0x55555555 );
        const uint32_t b =   ( ( ( a >> 2 )       & 0x33333333 ) + ( a & 0x33333333 ) );
        const uint32_t c =   ( ( ( b >> 4 ) + b ) & 0x0f0f0f0f );
        const uint32_t d =   c + ( c >> 8 );
        const uint32_t e =   d + ( d >> 16 );
        const uint32_t result = e & 0x0000003f;
        return result;
#endif // #ifdef __GNUC__
    }

    /**
        Calculates the log base 2 of an unsigned 32 bit integer.
        @param x The input integer value.
        @returns The log base 2 of the input.
     */

    inline uint32_t log2( uint32_t x )
    {
        const uint32_t a = x | ( x >> 1 );
        const uint32_t b = a | ( a >> 2 );
        const uint32_t c = b | ( b >> 4 );
        const uint32_t d = c | ( c >> 8 );
        const uint32_t e = d | ( d >> 16 );
        const uint32_t f = e >> 1;
        return popcount( f );
    }

    /**
        Calculates the number of bits required to serialize an integer in range [min,max].
        @param min The minimum value.
        @param max The maximum value.
        @returns The number of bits required to serialize the integer.
     */

    inline int bits_required( uint32_t min, uint32_t max )
    {
#ifdef __GNUC__
        return ( min == max ) ? 0 : 32 - __builtin_clz( max - min );
#else // #ifdef __GNUC__
        return ( min == max ) ? 0 : log2( max - min ) + 1;
#endif // #ifdef __GNUC__
    }

    /**
        Reverse the order of bytes in a 64 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint64_t bswap( uint64_t value )
    {
#ifdef __GNUC__
        return __builtin_bswap64( value );
#else // #ifdef __GNUC__
        value = ( value & 0x00000000FFFFFFFF ) << 32 | ( value & 0xFFFFFFFF00000000 ) >> 32;
        value = ( value & 0x0000FFFF0000FFFF ) << 16 | ( value & 0xFFFF0000FFFF0000 ) >> 16;
        value = ( value & 0x00FF00FF00FF00FF ) << 8  | ( value & 0xFF00FF00FF00FF00 ) >> 8;
        return value;
#endif // #ifdef __GNUC__
    }

    /**
        Reverse the order of bytes in a 32 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint32_t bswap( uint32_t value )
    {
#ifdef __GNUC__
        return __builtin_bswap32( value );
#else // #ifdef __GNUC__
        return ( value & 0x000000ff ) << 24 | ( value & 0x0000ff00 ) << 8 | ( value & 0x00ff0000 ) >> 8 | ( value & 0xff000000 ) >> 24;
#endif // #ifdef __GNUC__
    }

    /**
        Reverse the order of bytes in a 16 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint16_t bswap( uint16_t value )
    {
        return ( value & 0x00ff ) << 8 | ( value & 0xff00 ) >> 8;
    }

    /**
        Template to convert an integer value from local byte order to network byte order.
        IMPORTANT: Because most machines running yojimbo are little endian, yojimbo defines network byte order to be little endian.
        @param value The input value in local byte order. Supported integer types: uint64_t, uint32_t, uint16_t.
        @returns The input value converted to network byte order. If this processor is little endian the output is the same as the input. If the processor is big endian, the output is the input byte swapped.
        @see yojimbo::bswap
     */

    template <typename T> T host_to_network( T value )
    {
#if YOJIMBO_BIG_ENDIAN
        return bswap( value );
#else // #if YOJIMBO_BIG_ENDIAN
        return value;
#endif // #if YOJIMBO_BIG_ENDIAN
    }

    /**
        Template to convert an integer value from network byte order to local byte order.
        IMPORTANT: Because most machines running yojimbo are little endian, yojimbo defines network byte order to be little endian.
        @param value The input value in network byte order. Supported integer types: uint64_t, uint32_t, uint16_t.
        @returns The input value converted to local byte order. If this processor is little endian the output is the same as the input. If the processor is big endian, the output is the input byte swapped.
        @see yojimbo::bswap
     */

    template <typename T> T network_to_host( T value )
    {
#if YOJIMBO_BIG_ENDIAN
        return bswap( value );
#else // #if YOJIMBO_BIG_ENDIAN
        return value;
#endif // #if YOJIMBO_BIG_ENDIAN
    }

    /** 
        Compares two 16 bit sequence numbers and returns true if the first one is greater than the second (considering wrapping).
        IMPORTANT: This is not the same as s1 > s2!
        Greater than is defined specially to handle wrapping sequence numbers. 
        If the two sequence numbers are close together, it is as normal, but they are far apart, it is assumed that they have wrapped around.
        Thus, sequence_greater_than( 1, 0 ) returns true, and so does sequence_greater_than( 0, 65535 )!
        @param s1 The first sequence number.
        @param s2 The second sequence number.
        @returns True if the s1 is greater than s2, with sequence number wrapping considered.
     */

    inline bool sequence_greater_than( uint16_t s1, uint16_t s2 )
    {
        return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
               ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
    }

    /** 
        Compares two 16 bit sequence numbers and returns true if the first one is less than the second (considering wrapping).
        IMPORTANT: This is not the same as s1 < s2!
        Greater than is defined specially to handle wrapping sequence numbers. 
        If the two sequence numbers are close together, it is as normal, but they are far apart, it is assumed that they have wrapped around.
        Thus, sequence_less_than( 0, 1 ) returns true, and so does sequence_greater_than( 65535, 0 )!
        @param s1 The first sequence number.
        @param s2 The second sequence number.
        @returns True if the s1 is less than s2, with sequence number wrapping considered.
     */

    inline bool sequence_less_than( uint16_t s1, uint16_t s2 )
    {
        return sequence_greater_than( s2, s1 );
    }

    /**
        Convert a signed integer to an unsigned integer with zig-zag encoding.
        0,-1,+1,-2,+2... becomes 0,1,2,3,4 ...
        @param n The input value.
        @returns The input value converted from signed to unsigned with zig-zag encoding.
     */

    inline int signed_to_unsigned( int n )
    {
        return ( n << 1 ) ^ ( n >> 31 );
    }

    /**
        Convert an unsigned integer to as signed integer with zig-zag encoding.
        0,1,2,3,4... becomes 0,-1,+1,-2,+2...
        @param n The input value.
        @returns The input value converted from unsigned to signed with zig-zag encoding.
     */

    inline int unsigned_to_signed( uint32_t n )
    {
        return ( n >> 1 ) ^ ( -int32_t( n & 1 ) );
    }

#if YOJIMBO_WITH_MBEDTLS

    /**
        Base 64 encode a string.
        @param input The input string value. Must be null terminated.
        @param output The output base64 encoded string. Will be null terminated.
        @param output_size The size of the output buffer (bytes). Must be large enough to store the base 64 encoded string.
        @returns The number of bytes in the base64 encoded string, including terminating null. -1 if the base64 encode failed because the output buffer was too small.
     */

    int base64_encode_string( const char * input, char * output, int output_size );

    /**
        Base 64 decode a string.
        @param input The base64 encoded string.
        @param output The decoded string. Guaranteed to be null terminated, even if the base64 is maliciously encoded.
        @param output_size The size of the output buffer (bytes).
        @returns The number of bytes in the decoded string, including terminating null. -1 if the base64 decode failed.
     */

    int base64_decode_string( const char * input, char * output, int output_size );

    /**
        Base 64 encode a block of data.
        @param input The data to encode.
        @param input_length The length of the input data (bytes).
        @param output The output base64 encoded string. Will be null terminated.
        @param output_size The size of the output buffer. Must be large enough to store the base 64 encoded string.
        @returns The number of bytes in the base64 encoded string, including terminating null. -1 if the base64 encode failed because the output buffer was too small.
     */

    int base64_encode_data( const uint8_t * input, int input_length, char * output, int output_size );

    /**
        Base 64 decode a block of data.
        @param input The base 64 data to decode. Must be a null terminated string.
        @param output The output data. Will *not* be null terminated.
        @param output_size The size of the output buffer.
        @returns The number of bytes of decoded data. -1 if the base64 decode failed.
     */

    int base64_decode_data( const char * input, uint8_t * output, int output_size );

    /**
        Print bytes with a label. 
        Useful for printing out packets, encryption keys, nonce etc.
        @param label The label to print out before the bytes.
        @param data The data to print out to stdout.
        @param data_bytes The number of bytes of data to print.
     */

    void print_bytes( const char * label, const uint8_t * data, int data_bytes );

#endif // #if YOJIMBO_WITH_MBEDTLS

    /**
        A simple bit array class.
        You can create a bit array with a number of bits, set, clear and test if each bit is set.
     */

    class BitArray
    {
    public:

        /**
            The bit array constructor.
            @param allocator The allocator to use.
            @param size The number of bits in the bit array.
            All bits are initially set to zero.
         */

        BitArray( Allocator & allocator, int size )
        {
            yojimbo_assert( size > 0 );
            m_allocator = &allocator;
            m_size = size;
            m_bytes = 8 * ( ( size / 64 ) + ( ( size % 64 ) ? 1 : 0 ) );
            yojimbo_assert( m_bytes > 0 );
            m_data = (uint64_t*) YOJIMBO_ALLOCATE( allocator, m_bytes );
            Clear();
        }

        /**
            The bit array destructor.
         */

        ~BitArray()
        {
            yojimbo_assert( m_data );
            yojimbo_assert( m_allocator );
            YOJIMBO_FREE( *m_allocator, m_data );
            m_allocator = NULL;
        }

        /**
            Clear all bit values to zero.
         */

        void Clear()
        {
            yojimbo_assert( m_data );
            memset( m_data, 0, m_bytes );
        }

        /**
            Set a bit to 1.
            @param index The index of the bit.
         */

        void SetBit( int index )
        {
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            yojimbo_assert( bit_index >= 0 );
            yojimbo_assert( bit_index < 64 );
            m_data[data_index] |= uint64_t(1) << bit_index;
        }

        /**
            Clear a bit to 0.
            @param index The index of the bit.
         */

        void ClearBit( int index )
        {
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            m_data[data_index] &= ~( uint64_t(1) << bit_index );
        }

        /**
            Get the value of the bit.
            Returns 1 if the bit is set, 0 if the bit is not set.
            @param index The index of the bit.
         */

        uint64_t GetBit( int index ) const
        {
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_size );
            const int data_index = index >> 6;
            const int bit_index = index & ( (1<<6) - 1 );
            yojimbo_assert( bit_index >= 0 );
            yojimbo_assert( bit_index < 64 );
            return ( m_data[data_index] >> bit_index ) & 1;
        }

        /**
            Gets the size of the bit array, in number of bits.
            @returns The number of bits.
         */

        int GetSize() const
        {
            return m_size;
        }

    private:

        Allocator * m_allocator;                            ///< Allocator passed in to the constructor.
        int m_size;                                         ///< The size of the bit array in bits.
        int m_bytes;                                        ///< The size of the bit array in bytes.
        uint64_t * m_data;                                  ///< The data backing the bit array is an array of 64 bit integer values.

        BitArray( const BitArray & other );
        BitArray & operator = ( const BitArray & other );
    };

    /**
        A simple templated queue.
        This is a FIFO queue. First entry in, first entry out.
     */

    template <typename T> class Queue
    {
    public:

        /**
            Queue constructor.
            @param allocator The allocator to use.
            @param size The maximum number of entries in the queue.
         */

        Queue( Allocator & allocator, int size )
        {
            yojimbo_assert( size > 0 );
            m_arraySize = size;
            m_startIndex = 0;
            m_numEntries = 0;
            m_allocator = &allocator;
            m_entries = (T*) YOJIMBO_ALLOCATE( allocator, sizeof(T) * size );
            memset( m_entries, 0, sizeof(T) * size );
        }

        /**
            Queue destructor.
         */

        ~Queue()
        {
            yojimbo_assert( m_allocator );
            
            YOJIMBO_FREE( *m_allocator, m_entries );

            m_arraySize = 0;
            m_startIndex = 0;
            m_numEntries = 0;

            m_allocator = NULL;
        }

        /**
            Clear all entries in the queue and reset back to default state.
         */

        void Clear()
        {
            m_numEntries = 0;
            m_startIndex = 0;
        }

        /**
            Pop a value off the queue.
            IMPORTANT: This will assert if the queue is empty. Check Queue::IsEmpty or Queue::GetNumEntries first!
            @returns The value popped off the queue.
         */

        T Pop()
        {
            yojimbo_assert( !IsEmpty() );
            const T & entry = m_entries[m_startIndex];
            m_startIndex = ( m_startIndex + 1 ) % m_arraySize;
            m_numEntries--;
            return entry;
        }

        /**
            Push a value on to the queue.
            @param value The value to push onto the queue.
            IMPORTANT: Will assert if the queue is already full. Check Queue::IsFull before calling this!   
         */

        void Push( const T & value )
        {
            yojimbo_assert( !IsFull() );
            const int index = ( m_startIndex + m_numEntries ) % m_arraySize;
            m_entries[index] = value;
            m_numEntries++;
        }

        /**
            Random access for entries in the queue.
            @param index The index into the queue. 0 is the oldest entry, Queue::GetNumEntries() - 1 is the newest.
            @returns The value in the queue at the index.
         */

        T & operator [] ( int index )
        {
            yojimbo_assert( !IsEmpty() );
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_numEntries );
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        /**
            Random access for entries in the queue (const version).
            @param index The index into the queue. 0 is the oldest entry, Queue::GetNumEntries() - 1 is the newest.
            @returns The value in the queue at the index.
         */

        const T & operator [] ( int index ) const
        {
            yojimbo_assert( !IsEmpty() );
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_numEntries );
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        /**
            Get the size of the queue.
            This is the maximum number of values that can be pushed on the queue.
            @returns The size of the queue.
         */

        int GetSize() const
        {
            return m_arraySize;
        }

        /**
            Is the queue currently full?
            @returns True if the queue is full. False otherwise.
         */

        bool IsFull() const
        {
            return m_numEntries == m_arraySize;
        }

        /**
            Is the queue currently empty?
            @returns True if there are no entries in the queue.
         */

        bool IsEmpty() const
        {
            return m_numEntries == 0;
        }

        /**
            Get the number of entries in the queue.
            @returns The number of entries in the queue in [0,GetSize()].
         */

        int GetNumEntries() const
        {
            return m_numEntries;
        }

    private:


        Allocator * m_allocator;                        ///< The allocator passed in to the constructor.
        T * m_entries;                                  ///< Array of entries backing the queue (circular buffer).
        int m_arraySize;                                ///< The size of the array, in number of entries. This is the "size" of the queue.
        int m_startIndex;                               ///< The start index for the queue. This is the next value that gets popped off.
        int m_numEntries;                               ///< The number of entries currently stored in the queue.
    };

    /**
        Data structure that stores data indexed by sequence number.
        Entries may or may not exist. If they don't exist the sequence value for the entry at that index is set to 0xFFFFFFFF. 
        This provides a constant time lookup for an entry by sequence number. If the entry at sequence modulo buffer size doesn't have the same sequence number, that sequence number is not stored.
        This is incredibly useful and is used as the foundation of the packet level ack system and the reliable message send and receive queues.
        @see Connection
     */

    template <typename T> class SequenceBuffer
    {
    public:

        /**
            Sequence buffer constructor.
            @param allocator The allocator to use.
            @param size The size of the sequence buffer.
         */

        SequenceBuffer( Allocator & allocator, int size )
        {
            yojimbo_assert( size > 0 );
            m_size = size;
            m_sequence = 0;
            m_allocator = &allocator;
            m_entry_sequence = (uint32_t*) YOJIMBO_ALLOCATE( allocator, sizeof( uint32_t ) * size );
            m_entries = (T*) YOJIMBO_ALLOCATE( allocator, sizeof(T) * size );
            Reset();
        }

        /**
            Sequence buffer destructor.
         */

        ~SequenceBuffer()
        {
            yojimbo_assert( m_allocator );
            YOJIMBO_FREE( *m_allocator, m_entries );
            YOJIMBO_FREE( *m_allocator, m_entry_sequence );
            m_allocator = NULL;
        }

        /**
            Reset the sequence buffer.
            Removes all entries from the sequence buffer and restores it to initial state.
         */

        void Reset()
        {
            m_sequence = 0;
            memset( m_entry_sequence, 0xFF, sizeof( uint32_t ) * m_size );
        }

        /**
            Insert an entry in the sequence buffer.
            IMPORTANT: If another entry exists at the sequence modulo buffer size, it is overwritten.
            @param sequence The sequence number.
            @returns The sequence buffer entry, which you must fill with your data. NULL if a sequence buffer entry could not be added for your sequence number (if the sequence number is too old for example).
         */

        T * Insert( uint16_t sequence )
        {
            if ( sequence_greater_than( sequence + 1, m_sequence ) )
            {
                RemoveEntries( m_sequence, sequence );
                m_sequence = sequence + 1;
            }
            else if ( sequence_less_than( sequence, m_sequence - m_size ) )
            {
                return NULL;
            }
            const int index = sequence % m_size;
            m_entry_sequence[index] = sequence;
            return &m_entries[index];
        }

        /**
            Remove an entry from the sequence buffer.
            @param sequence The sequence number of the entry to remove.
         */

        void Remove( uint16_t sequence )
        {
            m_entry_sequence[ sequence % m_size ] = 0xFFFFFFFF;
        }

        /**
            Is the entry corresponding to the sequence number available? eg. Currently unoccupied.
            This works because older entries are automatically set back to unoccupied state as the sequence buffer advances forward.
            @param sequence The sequence number.
            @returns True if the sequence buffer entry is available, false if it is already occupied.
         */

        bool Available( uint16_t sequence ) const
        {
            return m_entry_sequence[ sequence % m_size ] == 0xFFFFFFFF;
        }

        /**
            Does an entry exist for a sequence number?
            @param sequence The sequence number.
            @returns True if an entry exists for this sequence number.
         */

        bool Exists( uint16_t sequence ) const
        {
            return m_entry_sequence[ sequence % m_size ] == uint32_t( sequence );
        }

        /**
            Get the entry corresponding to a sequence number.
            @param sequence The sequence number.
            @returns The entry if it exists. NULL if no entry is in the buffer for this sequence number.
         */

        T * Find( uint16_t sequence )
        {
            const int index = sequence % m_size;
            if ( m_entry_sequence[index] == uint32_t( sequence ) )
                return &m_entries[index];
            else
                return NULL;
        }

        /**
            Get the entry corresponding to a sequence number (const version).
            @param sequence The sequence number.
            @returns The entry if it exists. NULL if no entry is in the buffer for this sequence number.
         */

        const T * Find( uint16_t sequence ) const
        {
            const int index = sequence % m_size;
            if ( m_entry_sequence[index] == uint32_t( sequence ) )
                return &m_entries[index];
            else
                return NULL;
        }

        /**
            Get the entry at the specified index.
            Use this to iterate across entries in the sequence buffer.
            @param index The entry index in [0,GetSize()-1].
            @returns The entry if it exists. NULL if no entry is in the buffer at the specified index.
         */

        T * GetAtIndex( int index )
        {
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_size );
            return m_entry_sequence[index] != 0xFFFFFFFF ? &m_entries[index] : NULL;
        }

        /**
            Get the entry at the specified index (const version).
            Use this to iterate across entries in the sequence buffer.
            @param index The entry index in [0,GetSize()-1].
            @returns The entry if it exists. NULL if no entry is in the buffer at the specified index.
         */

        const T * GetAtIndex( int index ) const
        {
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_size );
            return m_entry_sequence[index] != 0xFFFFFFFF ? &m_entries[index] : NULL;
        }

        /**
            Get the most recent sequence number added to the buffer.
            This sequence number can wrap around, so if you are at 65535 and add an entry for sequence 0, then 0 becomes the new "most recent" sequence number.
            @returns The most recent sequence number.
            @see yojimbo::sequence_greater_than
            @see yojimbo::sequence_less_than
         */

        uint16_t GetSequence() const 
        {
            return m_sequence;
        }

        /**
            Get the entry index for a sequence number.
            This is simply the sequence number modulo the sequence buffer size.
            @param sequence The sequence number.
            @returns The sequence buffer index corresponding of the sequence number.
         */

        int GetIndex( uint16_t sequence ) const
        {
            return sequence % m_size;
        }

        /** 
            Get the size of the sequence buffer.
            @returns The size of the sequence buffer (number of entries).
         */

        int GetSize() const
        {
            return m_size;
        }

    protected:

        /** 
            Helper function to remove entries.
            This is used to remove old entries as we advance the sequence buffer forward. 
            Otherwise, if when entries are added with holes (eg. receive buffer for packets or messages, where not all sequence numbers are added to the buffer because we have high packet loss), 
            and we are extremely unlucky, we can have old sequence buffer entries from the previous sequence # wrap around still in the buffer, which corrupts our internal connection state.
            This actually happened in the soak test at high packet loss levels (>90%). It took me days to track it down :)
         */

        void RemoveEntries( int start_sequence, int finish_sequence )
        {
            if ( finish_sequence < start_sequence ) 
                finish_sequence += 65535;
            yojimbo_assert( finish_sequence >= start_sequence );
            if ( finish_sequence - start_sequence < m_size )
            {
                for ( int sequence = start_sequence; sequence <= finish_sequence; ++sequence )
                    m_entry_sequence[sequence % m_size] = 0xFFFFFFFF;
            }
            else
            {
                for ( int i = 0; i < m_size; ++i )
                    m_entry_sequence[i] = 0xFFFFFFFF;
            }
        }

    private:

        Allocator * m_allocator;                   ///< The allocator passed in to the constructor.
        int m_size;                                ///< The size of the sequence buffer.
        uint16_t m_sequence;                       ///< The most recent sequence number added to the buffer.
        uint32_t * m_entry_sequence;               ///< Array of sequence numbers corresponding to each sequence buffer entry for fast lookup. Set to 0xFFFFFFFF if no entry exists at that index.
        T * m_entries;                             ///< The sequence buffer entries. This is where the data is stored per-entry. Separate from the sequence numbers for fast lookup (hot/cold split) when the data per-sequence number is relatively large.
        
        SequenceBuffer( const SequenceBuffer<T> & other );

        SequenceBuffer<T> & operator = ( const SequenceBuffer<T> & other );
    };

    /**
        Bitpacks unsigned integer values to a buffer.
        Integer bit values are written to a 64 bit scratch value from right to left.
        Once the low 32 bits of the scratch is filled with bits it is flushed to memory as a dword and the scratch value is shifted right by 32.
        The bit stream is written to memory in little endian order, which is considered network byte order for this library.
        @see BitReader
     */

    class BitWriter
    {
    public:

        /**
            Bit writer constructor.
            Creates a bit writer object to write to the specified buffer. 
            @param data The pointer to the buffer to fill with bitpacked data.
            @param bytes The size of the buffer in bytes. Must be a multiple of 4, because the bitpacker reads and writes memory as dwords, not bytes.
         */

        BitWriter( void * data, int bytes ) : m_data( (uint32_t*) data ), m_numWords( bytes / 4 )
        {
            yojimbo_assert( data );
            yojimbo_assert( ( bytes % 4 ) == 0 );
            m_numBits = m_numWords * 32;
            m_bitsWritten = 0;
            m_wordIndex = 0;
            m_scratch = 0;
            m_scratchBits = 0;
        }

        /**
            Write bits to the buffer.
            Bits are written to the buffer as-is, without padding to nearest byte. Will assert if you try to write past the end of the buffer.
            A boolean value writes just 1 bit to the buffer, a value in range [0,31] can be written with just 5 bits and so on.
            IMPORTANT: When you have finished writing to your buffer, take care to call BitWrite::FlushBits, otherwise the last dword of data will not get flushed to memory!
            @param value The integer value to write to the buffer. Must be in [0,(1<<bits)-1].
            @param bits The number of bits to encode in [1,32].
            @see BitReader::ReadBits
         */

        void WriteBits( uint32_t value, int bits )
        {
            yojimbo_assert( bits > 0 );
            yojimbo_assert( bits <= 32 );
            yojimbo_assert( m_bitsWritten + bits <= m_numBits );
            yojimbo_assert( uint64_t( value ) <= ( ( 1ULL << bits ) - 1 ) );

            m_scratch |= uint64_t( value ) << m_scratchBits;

            m_scratchBits += bits;

            if ( m_scratchBits >= 32 )
            {
                yojimbo_assert( m_wordIndex < m_numWords );
                m_data[m_wordIndex] = host_to_network( uint32_t( m_scratch & 0xFFFFFFFF ) );
                m_scratch >>= 32;
                m_scratchBits -= 32;
                m_wordIndex++;
            }

            m_bitsWritten += bits;
        }

        /**
            Write an alignment to the bit stream, padding zeros so the bit index becomes is a multiple of 8.
            This is useful if you want to write some data to a packet that should be byte aligned. For example, an array of bytes, or a string.
            IMPORTANT: If the current bit index is already a multiple of 8, nothing is written.
            @see BitReader::ReadAlign
         */

        void WriteAlign()
        {
            const int remainderBits = m_bitsWritten % 8;

            if ( remainderBits != 0 )
            {
                uint32_t zero = 0;
                WriteBits( zero, 8 - remainderBits );
                yojimbo_assert( ( m_bitsWritten % 8 ) == 0 );
            }
        }

        /**
            Write an array of bytes to the bit stream.
            Use this when you have to copy a large block of data into your bitstream.
            Faster than just writing each byte to the bit stream via BitWriter::WriteBits( value, 8 ), because it aligns to byte index and copies into the buffer without bitpacking.
            @param data The byte array data to write to the bit stream.
            @param bytes The number of bytes to write.
            @see BitReader::ReadBytes
         */

        void WriteBytes( const uint8_t * data, int bytes )
        {
            yojimbo_assert( GetAlignBits() == 0 );
            yojimbo_assert( m_bitsWritten + bytes * 8 <= m_numBits );
            yojimbo_assert( ( m_bitsWritten % 32 ) == 0 || ( m_bitsWritten % 32 ) == 8 || ( m_bitsWritten % 32 ) == 16 || ( m_bitsWritten % 32 ) == 24 );

            int headBytes = ( 4 - ( m_bitsWritten % 32 ) / 8 ) % 4;
            if ( headBytes > bytes )
                headBytes = bytes;
            for ( int i = 0; i < headBytes; ++i )
                WriteBits( data[i], 8 );
            if ( headBytes == bytes )
                return;

            FlushBits();

            yojimbo_assert( GetAlignBits() == 0 );

            int numWords = ( bytes - headBytes ) / 4;
            if ( numWords > 0 )
            {
                yojimbo_assert( ( m_bitsWritten % 32 ) == 0 );
                memcpy( &m_data[m_wordIndex], data + headBytes, numWords * 4 );
                m_bitsWritten += numWords * 32;
                m_wordIndex += numWords;
                m_scratch = 0;
            }

            yojimbo_assert( GetAlignBits() == 0 );

            int tailStart = headBytes + numWords * 4;
            int tailBytes = bytes - tailStart;
            yojimbo_assert( tailBytes >= 0 && tailBytes < 4 );
            for ( int i = 0; i < tailBytes; ++i )
                WriteBits( data[tailStart+i], 8 );

            yojimbo_assert( GetAlignBits() == 0 );

            yojimbo_assert( headBytes + numWords * 4 + tailBytes == bytes );
        }

        /**
            Flush any remaining bits to memory.
            Call this once after you've finished writing bits to flush the last dword of scratch to memory!
            @see BitWriter::WriteBits
         */

        void FlushBits()
        {
            if ( m_scratchBits != 0 )
            {
                yojimbo_assert( m_scratchBits <= 32 );
                yojimbo_assert( m_wordIndex < m_numWords );
                m_data[m_wordIndex] = host_to_network( uint32_t( m_scratch & 0xFFFFFFFF ) );
                m_scratch >>= 32;
                m_scratchBits = 0;
                m_wordIndex++;                
            }
        }

        /**
            How many align bits would be written, if we were to write an align right now?
            @returns Result in [0,7], where 0 is zero bits required to align (already aligned) and 7 is worst case.
         */

        int GetAlignBits() const
        {
            return ( 8 - ( m_bitsWritten % 8 ) ) % 8;
        }

        /** 
            How many bits have we written so far?
            @returns The number of bits written to the bit buffer.
         */

        int GetBitsWritten() const
        {
            return m_bitsWritten;
        }

        /**
            How many bits are still available to write?
            For example, if the buffer size is 4, we have 32 bits available to write, if we have already written 10 bytes then 22 are still available to write.
            @returns The number of bits available to write.
         */

        int GetBitsAvailable() const
        {
            return m_numBits - m_bitsWritten;
        }
        
        /**
            Get a pointer to the data written by the bit writer.
            Corresponds to the data block passed in to the constructor.
            @returns Pointer to the data written by the bit writer.
         */

        const uint8_t * GetData() const
        {
            return (uint8_t*) m_data;
        }

        /**
            The number of bytes flushed to memory.
            This is effectively the size of the packet that you should send after you have finished bitpacking values with this class.
            The returned value is not always a multiple of 4, even though we flush dwords to memory. You won't miss any data in this case because the order of bits written is designed to work with the little endian memory layout.
            IMPORTANT: Make sure you call BitWriter::FlushBits before calling this method, otherwise you risk missing the last dword of data.
         */

        int GetBytesWritten() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

    private:

        uint32_t * m_data;              ///< The buffer we are writing to, as a uint32_t * because we're writing dwords at a time.
        uint64_t m_scratch;             ///< The scratch value where we write bits to (right to left). 64 bit for overflow. Once # of bits in scratch is >= 32, the low 32 bits are flushed to memory.
        int m_numBits;                  ///< The number of bits in the buffer. This is equivalent to the size of the buffer in bytes multiplied by 8. Note that the buffer size must always be a multiple of 4.
        int m_numWords;                 ///< The number of words in the buffer. This is equivalent to the size of the buffer in bytes divided by 4. Note that the buffer size must always be a multiple of 4.
        int m_bitsWritten;              ///< The number of bits written so far.
        int m_wordIndex;                ///< The current word index. The next word flushed to memory will be at this index in m_data.
        int m_scratchBits;              ///< The number of bits in scratch. When this is >= 32, the low 32 bits of scratch is flushed to memory as a dword and scratch is shifted right by 32.
    };

    /**
        Reads bit packed integer values from a buffer.
        Relies on the user reconstructing the exact same set of bit reads as bit writes when the buffer was written. This is an unattributed bitpacked binary stream!
        Implementation: 32 bit dwords are read in from memory to the high bits of a scratch value as required. The user reads off bit values from the scratch value from the right, after which the scratch value is shifted by the same number of bits.
     */

    class BitReader
    {
    public:

        /**
            Bit reader constructor.
            Non-multiples of four buffer sizes are supported, as this naturally tends to occur when packets are read from the network.
            However, actual buffer allocated for the packet data must round up at least to the next 4 bytes in memory, because the bit reader reads dwords from memory not bytes.
            @param data Pointer to the bitpacked data to read.
            @param bytes The number of bytes of bitpacked data to read.
            @see BitWriter
         */

#ifndef NDEBUG
        BitReader( const void * data, int bytes ) : m_data( (const uint32_t*) data ), m_numBytes( bytes ), m_numWords( ( bytes + 3 ) / 4)
#else // #ifndef NDEBUG
        BitReader( const void * data, int bytes ) : m_data( (const uint32_t*) data ), m_numBytes( bytes )
#endif // #ifndef NDEBUG
        {
            yojimbo_assert( data );
            m_numBits = m_numBytes * 8;
            m_bitsRead = 0;
            m_scratch = 0;
            m_scratchBits = 0;
            m_wordIndex = 0;
        }

        /**
            Would the bit reader would read past the end of the buffer if it read this many bits?
            @param bits The number of bits that would be read.
            @returns True if reading the number of bits would read past the end of the buffer.
         */

        bool WouldReadPastEnd( int bits ) const
        {
            return m_bitsRead + bits > m_numBits;
        }

        /**
            Read bits from the bit buffer.
            This function will assert in debug builds if this read would read past the end of the buffer.
            In production situations, the higher level ReadStream takes care of checking all packet data and never calling this function if it would read past the end of the buffer.
            @param bits The number of bits to read in [1,32].
            @returns The integer value read in range [0,(1<<bits)-1].
            @see BitReader::WouldReadPastEnd
            @see BitWriter::WriteBits
         */

        uint32_t ReadBits( int bits )
        {
            yojimbo_assert( bits > 0 );
            yojimbo_assert( bits <= 32 );
            yojimbo_assert( m_bitsRead + bits <= m_numBits );

            m_bitsRead += bits;

            yojimbo_assert( m_scratchBits >= 0 && m_scratchBits <= 64 );

            if ( m_scratchBits < bits )
            {
                yojimbo_assert( m_wordIndex < m_numWords );
                m_scratch |= uint64_t( network_to_host( m_data[m_wordIndex] ) ) << m_scratchBits;
                m_scratchBits += 32;
                m_wordIndex++;
            }

            yojimbo_assert( m_scratchBits >= bits );

            const uint32_t output = m_scratch & ( (uint64_t(1)<<bits) - 1 );

            m_scratch >>= bits;
            m_scratchBits -= bits;

            return output;
        }

        /**
            Read an align.
            Call this on read to correspond to a WriteAlign call when the bitpacked buffer was written. 
            This makes sure we skip ahead to the next aligned byte index. As a safety check, we verify that the padding to next byte is zero bits and return false if that's not the case. 
            This will typically abort packet read. Just another safety measure...
            @returns True if we successfully read an align and skipped ahead past zero pad, false otherwise (probably means, no align was written to the stream).
            @see BitWriter::WriteAlign
         */

        bool ReadAlign()
        {
            const int remainderBits = m_bitsRead % 8;
            if ( remainderBits != 0 )
            {
                uint32_t value = ReadBits( 8 - remainderBits );
                yojimbo_assert( m_bitsRead % 8 == 0 );
                if ( value != 0 )
                    return false;
            }
            return true;
        }

        /**
            Read bytes from the bitpacked data.
            @see BitWriter::WriteBytes
         */

        void ReadBytes( uint8_t * data, int bytes )
        {
            yojimbo_assert( GetAlignBits() == 0 );
            yojimbo_assert( m_bitsRead + bytes * 8 <= m_numBits );
            yojimbo_assert( ( m_bitsRead % 32 ) == 0 || ( m_bitsRead % 32 ) == 8 || ( m_bitsRead % 32 ) == 16 || ( m_bitsRead % 32 ) == 24 );

            int headBytes = ( 4 - ( m_bitsRead % 32 ) / 8 ) % 4;
            if ( headBytes > bytes )
                headBytes = bytes;
            for ( int i = 0; i < headBytes; ++i )
                data[i] = (uint8_t) ReadBits( 8 );
            if ( headBytes == bytes )
                return;

            yojimbo_assert( GetAlignBits() == 0 );

            int numWords = ( bytes - headBytes ) / 4;
            if ( numWords > 0 )
            {
                yojimbo_assert( ( m_bitsRead % 32 ) == 0 );
                memcpy( data + headBytes, &m_data[m_wordIndex], numWords * 4 );
                m_bitsRead += numWords * 32;
                m_wordIndex += numWords;
                m_scratchBits = 0;
            }

            yojimbo_assert( GetAlignBits() == 0 );

            int tailStart = headBytes + numWords * 4;
            int tailBytes = bytes - tailStart;
            yojimbo_assert( tailBytes >= 0 && tailBytes < 4 );
            for ( int i = 0; i < tailBytes; ++i )
                data[tailStart+i] = (uint8_t) ReadBits( 8 );

            yojimbo_assert( GetAlignBits() == 0 );

            yojimbo_assert( headBytes + numWords * 4 + tailBytes == bytes );
        }

        /**
            How many align bits would be read, if we were to read an align right now?
            @returns Result in [0,7], where 0 is zero bits required to align (already aligned) and 7 is worst case.
         */

        int GetAlignBits() const
        {
            return ( 8 - m_bitsRead % 8 ) % 8;
        }

        /** 
            How many bits have we read so far?
            @returns The number of bits read from the bit buffer so far.
         */

        int GetBitsRead() const
        {
            return m_bitsRead;
        }

        /**
            How many bits are still available to read?
            For example, if the buffer size is 4, we have 32 bits available to read, if we have already written 10 bytes then 22 are still available.
            @returns The number of bits available to read.
         */

        int GetBitsRemaining() const
        {
            return m_numBits - m_bitsRead;
        }

    private:

        const uint32_t * m_data;            ///< The bitpacked data we're reading as a dword array.
        uint64_t m_scratch;                 ///< The scratch value. New data is read in 32 bits at a top to the left of this buffer, and data is read off to the right.
        int m_numBits;                      ///< Number of bits to read in the buffer. Of course, we can't *really* know this so it's actually m_numBytes * 8.
        int m_numBytes;                     ///< Number of bytes to read in the buffer. We know this, and this is the non-rounded up version.
#ifndef NDEBUG
        int m_numWords;                     ///< Number of words to read in the buffer. This is rounded up to the next word if necessary.
#endif // #ifndef NDEBUG
        int m_bitsRead;                     ///< Number of bits read from the buffer so far.
        int m_scratchBits;                  ///< Number of bits currently in the scratch value. If the user wants to read more bits than this, we have to go fetch another dword from memory.
        int m_wordIndex;                    ///< Index of the next word to read from memory.
    };


    /**
        Routines to read and write variable-length integers.
    */

    int yojimbo_put_varint(unsigned char *p, uint64_t v);
    uint8_t yojimbo_get_varint(const unsigned char *p, uint64_t *v);
    uint8_t yojimbo_get_varint32(const unsigned char *p, uint32_t *v);
    int yojimbo_measure_varint(uint64_t v);

    /**
        The common case is for a varint to be a single byte.  They following macros handle the common case without a procedure call, but then call the procedure for larger varints.
    */
    // #define yojimbo_getvarint32(A,B) (uint8_t)((*(A)<(uint8_t)0x80)?((B)=(uint32_t)*(A)),1:yojimbo_get_varint32((A),(uint32_t *)&(B)))
    // #define yojimbo_putvarint32(A,B) (uint8_t)(((uint32_t)(B)<(uint32_t)0x80)?(*(A)=(unsigned char)(B)),1:yojimbo_put_varint((A),(B)))
    // #define yojimbo_getvarint yojimbo_get_varint
    // #define yojimbo_putvarint yojimbo_put_varint

    /** 
        Functionality common to all stream classes.
     */

    class BaseStream
    {
    public:

        /**
            Base stream constructor.
            @param allocator The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.
         */

        explicit BaseStream( Allocator & allocator ) : m_allocator( &allocator ), m_context( NULL ) {}

        /**
            Set a context on the stream.
            Contexts are used by the library supply data that is needed to read and write packets.
            Specifically, this context is used by the connection to supply data needed to read and write connection packets.
            If you are using the yojimbo client/server or connection classes you should NOT set this manually. It's already taken!
            However, if you are using only the low-level parts of yojimbo, feel free to take this over and use it for whatever you want.
            @see ConnectionContext
            @see ConnectionPacket
         */

        void SetContext( void * context )
        {
            m_context = context;
        }

        /**
            Get the context pointer set on the stream.

            @returns The context pointer. May be NULL.
         */

        void * GetContext() const
        {
            return m_context;
        }

        /**
            Get the allocator set on the stream.
            You can use this allocator to dynamically allocate memory while reading and writing packets.
            @returns The stream allocator.
         */

        Allocator & GetAllocator()
        {
            return *m_allocator;
        }

    private:

        Allocator * m_allocator;                    ///< The allocator passed into the constructor.
        void * m_context;                           ///< The context pointer set on the stream. May be NULL.
    };

    /**
        Stream class for writing bitpacked data.
        This class is a wrapper around the bit writer class. Its purpose is to provide unified interface for reading and writing.
        You can determine if you are writing to a stream by calling Stream::IsWriting inside your templated serialize method.
        This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.
        IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead. See test/shared.h for some examples.
        @see BitWriter
     */

    class WriteStream : public BaseStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        /**
            Write stream constructor.
            @param buffer The buffer to write to.
            @param bytes The number of bytes in the buffer. Must be a multiple of four.
            @param allocator The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.
         */

        WriteStream( Allocator & allocator, uint8_t * buffer, int bytes ) : BaseStream( allocator ), m_writer( buffer, bytes ) {}

        /**
            Serialize an integer (write).
            @param value The integer value in [min,max].
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on write.
         */

        bool SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            yojimbo_assert( min < max );
            yojimbo_assert( value >= min );
            yojimbo_assert( value <= max );
            const int bits = bits_required( min, max );
            uint32_t unsigned_value = value - min;
            m_writer.WriteBits( unsigned_value, bits );
            return true;
        }

        /**
            Serialize an varint (write).
            @param value The integer value.
            @returns Always returns true. All checking is performed by debug asserts only on write.
         */

        bool SerializeVarint32( uint32_t value )
        {
            uint8_t data[5];
            const int bytes = yojimbo_put_varint( data, value );
            yojimbo_assert( bytes >= 0 );
            for ( int i = 0; i < bytes; ++i )
                m_writer.WriteBits( data[i], 8 );
            return true;
        }

        /**
            Serialize an varint64 (write).
            @param value The integer value.
            @returns Always returns true. All checking is performed by debug asserts only on write.
        */

        bool SerializeVarint64( uint64_t value )
        {
            uint8_t data[9];
            const int bytes = yojimbo_put_varint( data, value );
            yojimbo_assert( bytes >= 0 );
            for ( int i = 0; i < bytes; ++i )
                m_writer.WriteBits( data[i], 8 );
            return true;
        }

        /**
            Serialize a number of bits (write).
            @param value The unsigned integer value to serialize. Must be in range [0,(1<<bits)-1].
            @param bits The number of bits to write in [1,32].
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBits( uint32_t value, int bits )
        {
            yojimbo_assert( bits > 0 );
            yojimbo_assert( bits <= 32 );
            m_writer.WriteBits( value, bits );
            return true;
        }

        /**
            Serialize an array of bytes (write).
            @param data Array of bytes to be written.
            @param bytes The number of bytes to write.
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBytes( const uint8_t * data, int bytes )
        {
            yojimbo_assert( data );
            yojimbo_assert( bytes >= 0 );
            SerializeAlign();
            m_writer.WriteBytes( data, bytes );
            return true;
        }

        /**
            Serialize an align (write).
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeAlign()
        {
            m_writer.WriteAlign();
            return true;
        }

        /** 
            If we were to write an align right now, how many bits would be required?
            @returns The number of zero pad bits required to achieve byte alignment in [0,7].
         */

        int GetAlignBits() const
        {
            return m_writer.GetAlignBits();
        }

        /**
            Serialize a safety check to the stream (write).
            Safety checks help track down desyncs. A check is written to the stream, and on the other side if the check is not present it asserts and fails the serialize.
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeCheck()
        {
#if YOJIMBO_SERIALIZE_CHECKS
            SerializeAlign();
            SerializeBits( SerializeCheckValue, 32 );
#else // #if YOJIMBO_SERIALIZE_CHECKS
            (void)string;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
        }

        /**
            Flush the stream to memory after you finish writing.
            Always call this after you finish writing and before you call WriteStream::GetData, or you'll potentially truncate the last dword of data you wrote.
            @see BitWriter::FlushBits
         */

        void Flush()
        {
            m_writer.FlushBits();
        }

        /**
            Get a pointer to the data written by the stream.
            IMPORTANT: Call WriteStream::Flush before you call this function!
            @returns A pointer to the data written by the stream
         */

        const uint8_t * GetData() const
        {
            return m_writer.GetData();
        }

        /**
            How many bytes have been written so far?
            @returns Number of bytes written. This is effectively the packet size.
         */

        int GetBytesProcessed() const
        {
            return m_writer.GetBytesWritten();
        }

        /**
            Get number of bits written so far.
            @returns Number of bits written.
         */

        int GetBitsProcessed() const
        {
            return m_writer.GetBitsWritten();
        }

    private:

        BitWriter m_writer;                 ///< The bit writer used for all bitpacked write operations.
    };

    /**
        Stream class for reading bitpacked data.
        This class is a wrapper around the bit reader class. Its purpose is to provide unified interface for reading and writing.
        You can determine if you are reading from a stream by calling Stream::IsReading inside your templated serialize method.
        This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.
        IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead. See test/shared.h for some examples.
        @see BitReader
     */

    class ReadStream : public BaseStream
    {
    public:

        enum { IsWriting = 0 };
        enum { IsReading = 1 };

        /**
            Read stream constructor.
            @param buffer The buffer to read from.
            @param bytes The number of bytes in the buffer. May be a non-multiple of four, however if it is, the underlying buffer allocated should be large enough to read the any remainder bytes as a dword.
            @param allocator The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.
         */

        ReadStream( Allocator & allocator, const uint8_t * buffer, int bytes ) : BaseStream( allocator ), m_reader( buffer, bytes ) {}

        /**
            Serialize an integer (read).
            @param value The integer value read is stored here. It is guaranteed to be in [min,max] if this function succeeds.
            @param min The minimum allowed value.
            @param max The maximum allowed value.
            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
         */

        bool SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            yojimbo_assert( min < max );
            const int bits = bits_required( min, max );
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint32_t unsigned_value = m_reader.ReadBits( bits );
            value = (int32_t) unsigned_value + min;
            return true;
        }

        /**
            Serialize a varint32 (read).
            @param value The integer value read is stored here.
            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
        */

        bool SerializeVarint32( uint32_t & value )
        {
            int i = 0;
            uint8_t data[6];
            uint32_t read_value;
            do { 
                if ( m_reader.WouldReadPastEnd( 8 ) )
                    return false;
                read_value = m_reader.ReadBits( 8 );
                data[ i++ ] = read_value;
            } while (i < 5 && (read_value >> 7) != 0 );
            data[i] = 0;
            yojimbo_get_varint32( data, &value );
            return true;
        }

        /**
            Serialize a varint64 (read).
            @param value The integer value read is stored here.
            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
        */

        bool SerializeVarint64( uint64_t & value )
        {
            int i = 0;
            uint8_t data[10];
            uint32_t read_value;
            do { 
                if ( m_reader.WouldReadPastEnd( 8 ) )
                    return false;
                read_value = m_reader.ReadBits( 8 );
                data[ i++ ] = read_value;
            } while (i < 9 && (read_value >> 7) != 0 );
            data[i] = 0;
            yojimbo_get_varint( data, &value );
            return true;
        }

        /**
            Serialize a number of bits (read).
            @param value The integer value read is stored here. Will be in range [0,(1<<bits)-1].
            @param bits The number of bits to read in [1,32].
            @returns Returns true if the serialize read succeeded, false otherwise.
         */

        bool SerializeBits( uint32_t & value, int bits )
        {
            yojimbo_assert( bits > 0 );
            yojimbo_assert( bits <= 32 );
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint32_t read_value = m_reader.ReadBits( bits );
            value = read_value;
            return true;
        }

        /**
            Serialize an array of bytes (read).
            @param data Array of bytes to read.
            @param bytes The number of bytes to read.
            @returns Returns true if the serialize read succeeded. False otherwise.
         */

        bool SerializeBytes( uint8_t * data, int bytes )
        {
            if ( !SerializeAlign() )
                return false;
            if ( m_reader.WouldReadPastEnd( bytes * 8 ) )
                return false;
            m_reader.ReadBytes( data, bytes );
            return true;
        }

        /**
            Serialize an align (read).
            @returns Returns true if the serialize read succeeded. False otherwise.
         */

        bool SerializeAlign()
        {
            const int alignBits = m_reader.GetAlignBits();
            if ( m_reader.WouldReadPastEnd( alignBits ) )
                return false;
            if ( !m_reader.ReadAlign() )
                return false;
            return true;
        }

        /** 
            If we were to read an align right now, how many bits would we need to read?
            @returns The number of zero pad bits required to achieve byte alignment in [0,7].
         */

        int GetAlignBits() const
        {
            return m_reader.GetAlignBits();
        }

        /**
            Serialize a safety check from the stream (read).
            Safety checks help track down desyncs. A check is written to the stream, and on the other side if the check is not present it asserts and fails the serialize.
            @returns Returns true if the serialize check passed. False otherwise.
         */

        bool SerializeCheck()
        {
#if YOJIMBO_SERIALIZE_CHECKS            
            if ( !SerializeAlign() )
                return false;
            uint32_t value = 0;
            if ( !SerializeBits( value, 32 ) )
                return false;
            if ( value != SerializeCheckValue )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_DEBUG, "serialize check failed: expected %x, got %x\n", SerializeCheckValue, value );
            }
            return value == SerializeCheckValue;
#else // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
        }

        /**
            Get number of bits read so far.
            @returns Number of bits read.
         */

        int GetBitsProcessed() const
        {
            return m_reader.GetBitsRead();
        }

        /**
            How many bytes have been read so far?
            @returns Number of bytes read. Effectively this is the number of bits read, rounded up to the next byte where necessary.
         */

        int GetBytesProcessed() const
        {
            return ( m_reader.GetBitsRead() + 7 ) / 8;
        }

    private:

        BitReader m_reader;             ///< The bit reader used for all bitpacked read operations.
    };

    /**
        Stream class for estimating how many bits it would take to serialize something.
        This class acts like a bit writer (IsWriting is 1, IsReading is 0), but instead of writing data, it counts how many bits would be written.
        It's used by the connection channel classes to work out how many messages will fit in the channel packet budget.
        Note that when the serialization includes alignment to byte (see MeasureStream::SerializeAlign), this is an estimate and not an exact measurement. The estimate is guaranteed to be conservative. 
        @see BitWriter
        @see BitReader
     */

    class MeasureStream : public BaseStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        /**
            Measure stream constructor.
            @param allocator The allocator to use for stream allocations. This lets you dynamically allocate memory as you read and write packets.
         */

        explicit MeasureStream( Allocator & allocator ) : BaseStream( allocator ), m_bitsWritten(0) {}

        /**
            Serialize an integer (measure).
            @param value The integer value to write. Not actually used or checked.
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on measure.
         */

        bool SerializeInteger( int32_t value, int32_t min, int32_t max )
        {   
            (void) value;
            yojimbo_assert( min < max );
            yojimbo_assert( value >= min );
            yojimbo_assert( value <= max );
            const int bits = bits_required( min, max );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize an varint32 (measure).
            @param value The integer value to write. Not actually used or checked.
            @returns Always returns true. All checking is performed by debug asserts only on measure.
         */

        bool SerializeVarint32( int32_t value )
        {   
            const int bits = yojimbo_measure_varint( value ) * 8;
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize an varint64 (measure).
            @param value The integer value to write. Not actually used or checked.
            @returns Always returns true. All checking is performed by debug asserts only on measure.
        */

        bool SerializeVarint64( int64_t value )
        {   
            const int bits = yojimbo_measure_varint( value ) * 8;
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize a number of bits (write).
            @param value The unsigned integer value to serialize. Not actually used or checked.
            @param bits The number of bits to write in [1,32].
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBits( uint32_t value, int bits )
        {
            (void) value;
            yojimbo_assert( bits > 0 );
            yojimbo_assert( bits <= 32 );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize an array of bytes (measure).
            @param data Array of bytes to 'write'. Not actually used.
            @param bytes The number of bytes to 'write'.
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBytes( const uint8_t * data, int bytes )
        {
            (void) data;
            SerializeAlign();
            m_bitsWritten += bytes * 8;
            return true;
        }

        /**
            Serialize an align (measure).
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeAlign()
        {
            const int alignBits = GetAlignBits();
            m_bitsWritten += alignBits;
            return true;
        }

        /** 
            If we were to write an align right now, how many bits would be required?
            IMPORTANT: Since the number of bits required for alignment depends on where an object is written in the final bit stream, this measurement is conservative. 
            @returns Always returns worst case 7 bits.
         */

        int GetAlignBits() const
        {
            return 7;
        }

        /**
            Serialize a safety check to the stream (measure).
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeCheck()
        {
#if YOJIMBO_SERIALIZE_CHECKS
            SerializeAlign();
            m_bitsWritten += 32;
#endif // #if YOJIMBO_SERIALIZE_CHECKS
            return true;
        }

        /**
            Get number of bits written so far.
            @returns Number of bits written.
         */

        int GetBitsProcessed() const
        {
            return m_bitsWritten;
        }

        /**
            How many bytes have been written so far?
            @returns Number of bytes written.
         */

        int GetBytesProcessed() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

    private:

        int m_bitsWritten;              ///< Counts the number of bits written.
    };

    const int MaxAddressLength = 256;       ///< The maximum length of an address when converted to a string (includes terminating NULL). @see Address::ToString

    /** 
        Address type.
        @see Address::GetType.
     */

    enum AddressType
    {
        ADDRESS_NONE,                                                       ///< Not an address. Set by the default constructor.
        ADDRESS_IPV4,                                                       ///< An IPv4 address, eg: "146.95.129.237"
        ADDRESS_IPV6                                                        ///< An IPv6 address, eg: "48d9:4a08:b543:ae31:89d8:3226:b92c:cbba"
    };

    /** 
        An IP address and port number.
        Supports both IPv4 and IPv6 addresses.
        Identifies where a packet came from, and where a packet should be sent.
     */

    class Address
    {
        AddressType m_type;                                                 ///< The address type: IPv4 or IPv6.
        union
        {
            uint8_t ipv4[4];                                                ///< IPv4 address data. Valid if type is ADDRESS_IPV4.
            uint16_t ipv6[8];                                               ///< IPv6 address data. Valid if type is ADDRESS_IPV6.
        } m_address;
        uint16_t m_port;                                                    ///< The IP port. Valid for IPv4 and IPv6 address types.

   public:

        /**
            Address default constructor.
            Designed for convenience so you can have address members of classes and initialize them via assignment.
            An address created by the default constructor will have address type set to ADDRESS_NONE. Address::IsValid will return false.
            @see IsValid
         */

        Address();

        /**
            Create an IPv4 address.
            IMPORTANT: Pass in port in local byte order. The address class handles the conversion to network order for you.
            @param a The first field of the IPv4 address.
            @param b The second field of the IPv4 address.
            @param c The third field of the IPv4 address.
            @param d The fourth field of the IPv4 address.
            @param port The IPv4 port (local byte order).
         */

        Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port = 0 );

        /**
            Create an IPv4 address.
            @param address Array of four address fields for the IPv4 address.
            @param port The port number (local byte order).
         */

        Address( const uint8_t address[], uint16_t port = 0 );

        /**
            Create an IPv6 address.
            IMPORTANT: Pass in address fields and the port in local byte order. The address class handles the conversion to network order for you.
            @param a First field of the IPv6 address (local byte order).
            @param b Second field of the IPv6 address (local byte order).
            @param c Third field of the IPv6 address (local byte order).
            @param d Fourth field of the IPv6 address (local byte order).
            @param e Fifth field of the IPv6 address (local byte order).
            @param f Sixth field of the IPv6 address (local byte order).
            @param g Seventh field of the IPv6 address (local byte order).
            @param h Eighth field of the IPv6 address (local byte order).
            @param port The port number (local byte order).
         */

        Address( uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h, uint16_t port = 0 );

        /**
            Create an IPv6 address.
            IMPORTANT: Pass in address fields and the port in local byte order. The address class handles the conversion to network order for you.
            @param address Array of 8 16 bit address fields for the IPv6 address (local byte order).
            @param port The IPv6 port (local byte order).
         */

        Address( const uint16_t address[], uint16_t port = 0 );

        /**
            Parse a string to an address.
            This versions supports parsing a port included in the address string. For example, "127.0.0.1:4000" and "[::1]:40000". 
            Parsing is performed via inet_pton once the port # has been extracted from the string, so you may specify any IPv4 or IPv6 address formatted in any valid way, and it should work as you expect.
            Depending on the type of data in the string the address will become ADDRESS_TYPE_IPV4 or ADDRESS_TYPE_IPV6.
            If the string is not recognized as a valid address, the address type is set to ADDRESS_TYPE_NONE, causing Address::IsValid to return false. Please check that after creating an address from a string.
            @param address The string to parse to create the address.
            @see Address::IsValid
            @see Address::GetType
         */

        explicit Address( const char * address );

        /**
            Parse a string to an address.
            This versions overrides any port read in the address with the port parameter. This lets you parse "127.0.0.1" and "[::1]" and pass in the port you want programmatically.
            Parsing is performed via inet_pton once the port # has been extracted from the string, so you may specify any IPv4 or IPv6 address formatted in any valid way, and it should work as you expect.
            Depending on the type of data in the string the address will become ADDRESS_TYPE_IPV4 or ADDRESS_TYPE_IPV6.
            If the string is not recognized as a valid address, the address type is set to ADDRESS_TYPE_NONE, causing Address::IsValid to return false. Please check that after creating an address from a string.
            @param address The string to parse to create the address.
            @param port Overrides the port number read from the string (if any).
            @see Address::IsValid
            @see Address::GetType
         */

        explicit Address( const char * address, uint16_t port );

        /**
            Clear the address.
            The address type is set to ADDRESS_TYPE_NONE.
            After this function is called Address::IsValid will return false.
         */

        void Clear();

        /**
            Get the IPv4 address data.
            @returns The IPv4 address as an array of bytes.
         */

        const uint8_t * GetAddress4() const;

        /**
            Get the IPv6 address data.
            @returns the IPv6 address data as an array of uint16_t (local byte order).
         */

        const uint16_t * GetAddress6() const;

        /**
            Set the port.
            This is useful when you want to programmatically set a server port, eg. try to open a server on ports 40000, 40001, etc...
            @param port The port number (local byte order). Works for both IPv4 and IPv6 addresses.
         */

        void SetPort( uint16_t port );

        /**
            Get the port number.
            @returns The port number (local byte order).
         */

        uint16_t GetPort() const;

        /**
            Get the address type.
            @returns The address type: ADDRESS_NONE, ADDRESS_IPV4 or ADDRESS_IPV6.
         */

        AddressType GetType() const;

        /**
            Convert the address to a string.
            @param buffer The buffer the address will be written to.
            @param bufferSize The size of the buffer in bytes. Must be at least MaxAddressLength.
         */

        const char * ToString( char buffer[], int bufferSize ) const;

        /**
            True if the address is valid.
            A valid address is any address with a type other than ADDRESS_TYPE_NONE.
            @returns True if the address is valid, false otherwise.
         */

        bool IsValid() const;

        /**
            Is this a loopback address?
            Corresponds to an IPv4 address of "127.0.0.1", or an IPv6 address of "::1".
            @returns True if this is the loopback address.
         */

        bool IsLoopback() const;

        /**
            Is this an IPv6 link local address?
            Corresponds to the first field of the address being 0xfe80
            @returns True if this address is a link local IPv6 address.
         */

        bool IsLinkLocal() const;

        /**
            Is this an IPv6 site local address?
            Corresponds to the first field of the address being 0xfec0
            @returns True if this address is a site local IPv6 address.
         */

        bool IsSiteLocal() const;

        /**
            Is this an IPv6 multicast address?
            Corresponds to the first field of the IPv6 address being 0xff00
            @returns True if this address is a multicast IPv6 address.
         */

        bool IsMulticast() const;

        /**
            Is this in IPv6 global unicast address?
            Corresponds to any IPv6 address that is not any of the following: Link Local, Site Local, Multicast or Loopback.
            @returns True if this is a global unicast IPv6 address.
         */

        bool IsGlobalUnicast() const;

        bool operator ==( const Address & other ) const;

        bool operator !=( const Address & other ) const;

    protected:

        /** 
            Helper function to parse an address string. 
            Used by the constructors that take a string parameter.
            @param address The string to parse.
         */

        void Parse( const char * address );
    };

    /**
        Serialize integer value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The integer value to serialize in [min,max].
        @param min The minimum value.
        @param max The maximum value.
     */

    #define serialize_int( stream, value, min, max )                    \
        do                                                              \
        {                                                               \
            yojimbo_assert( min < max );                                \
            int32_t int32_value = 0;                                    \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                yojimbo_assert( int64_t(value) >= int64_t(min) );       \
                yojimbo_assert( int64_t(value) <= int64_t(max) );       \
                int32_value = (int32_t) value;                          \
            }                                                           \
            if ( !stream.SerializeInteger( int32_value, min, max ) )    \
            {                                                           \
                return false;                                           \
            }                                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = int32_value;                                    \
                if ( int64_t(value) < int64_t(min) ||                   \
                     int64_t(value) > int64_t(max) )                    \
                {                                                       \
                    return false;                                       \
                }                                                       \
            }                                                           \
        } while (0)


    /**
         Serialize variable integer value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The integer value to serialize.
    */

    #define serialize_varint32( stream, value )                         \
        do                                                              \
        {                                                               \
            uint32_t int32_value = 0;                                   \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                int32_value = (uint32_t) value;                         \
            }                                                           \
            if ( !stream.SerializeVarint32( int32_value ) )             \
            {                                                           \
                return false;                                           \
            }                                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = int32_value;                                    \
            }                                                           \
        } while (0)

    /**
         Serialize variable integer value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The 64 bit integer value to serialize.
    */

    #define serialize_varint64( stream, value )                         \
        do                                                              \
        {                                                               \
            uint64_t int64_value = 0;                                   \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                int64_value = (uint64_t) value;                         \
            }                                                           \
            if ( !stream.SerializeVarint64( int64_value ) )             \
            {                                                           \
                return false;                                           \
            }                                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = int64_value;                                    \
            }                                                           \
        } while (0)

    /**
        Serialize bits to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned integer value to serialize.
        @param bits The number of bits to serialize in [1,32].
     */

    #define serialize_bits( stream, value, bits )                       \
        do                                                              \
        {                                                               \
            yojimbo_assert( bits > 0 );                                 \
            yojimbo_assert( bits <= 32 );                               \
            uint32_t uint32_value = 0;                                  \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                uint32_value = (uint32_t) value;                        \
            }                                                           \
            if ( !stream.SerializeBits( uint32_value, bits ) )          \
            {                                                           \
                return false;                                           \
            }                                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = uint32_value;                                   \
            }                                                           \
        } while (0)

    /**
        Serialize a boolean value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The boolean value to serialize.
     */

    #define serialize_bool( stream, value )                             \
        do                                                              \
        {                                                               \
            uint32_t uint32_bool_value = 0;                             \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                uint32_bool_value = value ? 1 : 0;                      \
            }                                                           \
            serialize_bits( stream, uint32_bool_value, 1 );             \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = uint32_bool_value ? true : false;               \
            }                                                           \
        } while (0)

    template <typename Stream> bool serialize_float_internal( Stream & stream, float & value )
    {
        uint32_t int_value;
        if ( Stream::IsWriting )
        {
            memcpy( &int_value, &value, 4 );
        }
        bool result = stream.SerializeBits( int_value, 32 );
        if ( Stream::IsReading )
        {
            memcpy( &value, &int_value, 4 );
        }
        return result;
    }

    /**
        Serialize floating point value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The float value to serialize.
     */

    #define serialize_float( stream, value )                                        \
        do                                                                          \
        {                                                                           \
            if ( !yojimbo::serialize_float_internal( stream, value ) )              \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    /**
        Serialize a 32 bit unsigned integer to the stream (read/write/measure).
        This is a helper macro to make unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 32 bit integer value to serialize.
     */

    #define serialize_uint32( stream, value ) serialize_bits( stream, value, 32 );

    template <typename Stream> bool serialize_uint64_internal( Stream & stream, uint64_t & value )
    {
        uint32_t hi = 0, lo = 0;
        if ( Stream::IsWriting )
        {
            lo = value & 0xFFFFFFFF;
            hi = value >> 32;
        }
        serialize_bits( stream, lo, 32 );
        serialize_bits( stream, hi, 32 );
        if ( Stream::IsReading )
        {
            value = ( uint64_t(hi) << 32 ) | lo;
        }
        return true;
    }

    /**
        Serialize a 64 bit unsigned integer to the stream (read/write/measure).
        This is a helper macro to make unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 64 bit integer value to serialize.
     */

    #define serialize_uint64( stream, value )                                       \
        do                                                                          \
        {                                                                           \
            if ( !yojimbo::serialize_uint64_internal( stream, value ) )             \
                return false;                                                       \
        } while (0)

    template <typename Stream> bool serialize_double_internal( Stream & stream, double & value )
    {
        union DoubleInt
        {
            double double_value;
            uint64_t int_value;
        };
        DoubleInt tmp = { 0 };
        if ( Stream::IsWriting )
        {
            tmp.double_value = value;
        }
        serialize_uint64( stream, tmp.int_value );
        if ( Stream::IsReading )
        {
            value = tmp.double_value;
        }
        return true;
    }

    /**
        Serialize double precision floating point value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The double precision floating point value to serialize.
     */

    #define serialize_double( stream, value )                                       \
        do                                                                          \
        {                                                                           \
            if ( !yojimbo::serialize_double_internal( stream, value ) )             \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    template <typename Stream> bool serialize_bytes_internal( Stream & stream, uint8_t * data, int bytes )
    {
        return stream.SerializeBytes( data, bytes );
    }

    /**
        Serialize an array of bytes to the stream (read/write/measure).
        This is a helper macro to make unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param data Pointer to the data to be serialized.
        @param bytes The number of bytes to serialize.
     */

    #define serialize_bytes( stream, data, bytes )                                  \
        do                                                                          \
        {                                                                           \
            if ( !yojimbo::serialize_bytes_internal( stream, data, bytes ) )        \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    template <typename Stream> bool serialize_string_internal( Stream & stream, char * string, int buffer_size )
    {
        int length = 0;
        if ( Stream::IsWriting )
        {
            length = (int) strlen( string );
            yojimbo_assert( length < buffer_size );
        }
        serialize_int( stream, length, 0, buffer_size - 1 );
        serialize_bytes( stream, (uint8_t*)string, length );
        if ( Stream::IsReading )
        {
            string[length] = '\0';
        }
        return true;
    }

    /**
        Serialize a string to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param string The string to serialize write/measure. Pointer to buffer to be filled on read.
        @param buffer_size The size of the string buffer. String with terminating null character must fit into this buffer.
     */

    #define serialize_string( stream, string, buffer_size )                                 \
        do                                                                                  \
        {                                                                                   \
            if ( !yojimbo::serialize_string_internal( stream, string, buffer_size ) )       \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    /**
        Serialize an alignment to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
     */

    #define serialize_align( stream )                                                       \
        do                                                                                  \
        {                                                                                   \
            if ( !stream.SerializeAlign() )                                                 \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    /**
        Serialize a safety check to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
     */

    #define serialize_check( stream )                                                       \
        do                                                                                  \
        {                                                                                   \
            if ( !stream.SerializeCheck() )                                                 \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    /**
        Serialize an object to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param object The object to serialize. Must have a serialize method on it.
     */

    #define serialize_object( stream, object )                                              \
        do                                                                                  \
        {                                                                                   \
            if ( !object.Serialize( stream ) )                                              \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        }                                                                                   \
        while(0)

    template <typename Stream> bool serialize_address_internal( Stream & stream, Address & address )
    {
        char buffer[MaxAddressLength];
        if ( Stream::IsWriting )
        {
            yojimbo_assert( address.IsValid() );
            address.ToString( buffer, sizeof( buffer ) );
        }
        serialize_string( stream, buffer, sizeof( buffer ) );
        if ( Stream::IsReading )
        {
            address = Address( buffer );
            if ( !address.IsValid() )
            {
                return false;
            }
        }
        return true;
    }

    /**
        Serialize an address to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The address to serialize. Must be a valid address.
     */

    #define serialize_address( stream, value )                                              \
        do                                                                                  \
        {                                                                                   \
            if ( !yojimbo::serialize_address_internal( stream, value ) )                    \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    template <typename Stream, typename T> bool serialize_int_relative_internal( Stream & stream, T previous, T & current )
    {
        uint32_t difference = 0;
        if ( Stream::IsWriting )
        {
            yojimbo_assert( previous < current );
            difference = current - previous;
        }

        bool oneBit = false;
        if ( Stream::IsWriting )
        {
            oneBit = difference == 1;
        }
        serialize_bool( stream, oneBit );
        if ( oneBit )
        {
            if ( Stream::IsReading )
            {
                current = previous + 1;
            }
            return true;
        }
        
        bool twoBits = false;
        if ( Stream::IsWriting )
        {
            twoBits = difference <= 6;
        }
        serialize_bool( stream, twoBits );
        if ( twoBits )
        {
            serialize_int( stream, difference, 2, 6 );
            if ( Stream::IsReading )
            {
                current = previous + difference;
            }
            return true;
        }
        
        bool fourBits = false;
        if ( Stream::IsWriting )
        {
            fourBits = difference <= 23;
        }
        serialize_bool( stream, fourBits );
        if ( fourBits )
        {
            serialize_int( stream, difference, 7, 23 );
            if ( Stream::IsReading )
            {
                current = previous + difference;
            }
            return true;
        }

        bool eightBits = false;
        if ( Stream::IsWriting )
        {
            eightBits = difference <= 280;
        }
        serialize_bool( stream, eightBits );
        if ( eightBits )
        {
            serialize_int( stream, difference, 24, 280 );
            if ( Stream::IsReading )
            {
                current = previous + difference;
            }
            return true;
        }

        bool twelveBits = false;
        if ( Stream::IsWriting )
        {
            twelveBits = difference <= 4377;
        }
        serialize_bool( stream, twelveBits );
        if ( twelveBits )
        {
            serialize_int( stream, difference, 281, 4377 );
            if ( Stream::IsReading )
            {
                current = previous + difference;
            }
            return true;
        }

        bool sixteenBits = false;
        if ( Stream::IsWriting )
        {
            sixteenBits = difference <= 69914;
        }
        serialize_bool( stream, sixteenBits );
        if ( sixteenBits )
        {
            serialize_int( stream, difference, 4378, 69914 );
            if ( Stream::IsReading )
            {
                current = previous + difference;
            }
            return true;
        }

        uint32_t value = current;
        serialize_uint32( stream, value );
        if ( Stream::IsReading )
        {
            current = value;
        }

        return true;
    }

    /**
        Serialize an integer value relative to another (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param previous The previous integer value.
        @param current The current integer value.
     */

    #define serialize_int_relative( stream, previous, current )                             \
        do                                                                                  \
        {                                                                                   \
            if ( !yojimbo::serialize_int_relative_internal( stream, previous, current ) )   \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    template <typename Stream> bool serialize_ack_relative_internal( Stream & stream, uint16_t sequence, uint16_t & ack )
    {
        int ack_delta = 0;
        bool ack_in_range = false;
        if ( Stream::IsWriting )
        {
            if ( ack < sequence )
            {
                ack_delta = sequence - ack;
            }
            else
            {
                ack_delta = (int)sequence + 65536 - ack;
            }
            yojimbo_assert( ack_delta > 0 );
            yojimbo_assert( uint16_t( sequence - ack_delta ) == ack );
            ack_in_range = ack_delta <= 64;
        }
        serialize_bool( stream, ack_in_range );
        if ( ack_in_range )
        {
            serialize_int( stream, ack_delta, 1, 64 );
            if ( Stream::IsReading )
            {
                ack = sequence - ack_delta;
            }
        }
        else
        {
            serialize_bits( stream, ack, 16 );
        }
        return true;
    }

    /**
        Serialize an ack relative to the current sequence number (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param sequence The current sequence number.
        @param ack The ack sequence number, which is typically near the current sequence number.
     */

    #define serialize_ack_relative( stream, sequence, ack  )                                        \
        do                                                                                          \
        {                                                                                           \
            if ( !yojimbo::serialize_ack_relative_internal( stream, sequence, ack ) )               \
            {                                                                                       \
                return false;                                                                       \
            }                                                                                       \
        } while (0)

    template <typename Stream> bool serialize_sequence_relative_internal( Stream & stream, uint16_t sequence1, uint16_t & sequence2 )
    {
        if ( Stream::IsWriting )
        {
            uint32_t a = sequence1;
            uint32_t b = sequence2 + ( ( sequence1 > sequence2 ) ? 65536 : 0 );
            serialize_int_relative( stream, a, b );
        }
        else
        {
            uint32_t a = sequence1;
            uint32_t b = 0;
            serialize_int_relative( stream, a, b );
            if ( b >= 65536 )
            {
                b -= 65536;
            }
            sequence2 = uint16_t( b );
        }

        return true;
    }

    /**
        Serialize a sequence number relative to another (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param sequence1 The first sequence number to serialize relative to.
        @param sequence2 The second sequence number to be encoded relative to the first.
     */

    #define serialize_sequence_relative( stream, sequence1, sequence2 )                             \
        do                                                                                          \
        {                                                                                           \
            if ( !yojimbo::serialize_sequence_relative_internal( stream, sequence1, sequence2 ) )   \
            {                                                                                       \
                return false;                                                                       \
            }                                                                                       \
        } while (0)

    // read macros corresponding to each serialize_*. useful when you want separate read and write functions.

    #define read_bits( stream, value, bits )                                                \
    do                                                                                      \
    {                                                                                       \
        yojimbo_assert( bits > 0 );                                                         \
        yojimbo_assert( bits <= 32 );                                                       \
        uint32_t uint32_value= 0;                                                           \
        if ( !stream.SerializeBits( uint32_value, bits ) )                                  \
        {                                                                                   \
            return false;                                                                   \
        }                                                                                   \
        value = uint32_value;                                                               \
    } while (0)

    #define read_int( stream, value, min, max )                                             \
        do                                                                                  \
        {                                                                                   \
            yojimbo_assert( min < max );                                                    \
            int32_t int32_value = 0;                                                        \
            if ( !stream.SerializeInteger( int32_value, min, max ) )                        \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            value = int32_value;                                                            \
            if ( value < min || value > max )                                               \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_bool( stream, value ) read_bits( stream, value, 1 )

    #define read_float                  serialize_float
    #define read_uint32                 serialize_uint32
    #define read_uint64                 serialize_uint64
    #define read_double                 serialize_double
    #define read_bytes                  serialize_bytes
    #define read_string                 serialize_string
    #define read_align                  serialize_align
    #define read_check                  serialize_check
    #define read_object                 serialize_object
    #define read_address                serialize_address
    #define read_int_relative           serialize_int_relative
    #define read_ack_relative           serialize_ack_relative
    #define read_sequence_relative      serialize_sequence_relative

    // write macros corresponding to each serialize_*. useful when you want separate read and write functions for some reason.

    #define write_bits( stream, value, bits )                                               \
        do                                                                                  \
        {                                                                                   \
            yojimbo_assert( bits > 0 );                                                     \
            yojimbo_assert( bits <= 32 );                                                   \
            uint32_t uint32_value = (uint32_t) value;                                       \
            if ( !stream.SerializeBits( uint32_value, bits ) )                              \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define write_int( stream, value, min, max )                                            \
        do                                                                                  \
        {                                                                                   \
            yojimbo_assert( min < max );                                                    \
            yojimbo_assert( value >= min );                                                 \
            yojimbo_assert( value <= max );                                                 \
            int32_t int32_value = (int32_t) value;                                          \
            if ( !stream.SerializeInteger( int32_value, min, max ) )                        \
                return false;                                                               \
        } while (0)

    #define write_float                 serialize_float
    #define write_uint32                serialize_uint32
    #define write_uint64                serialize_uint64
    #define write_double                serialize_double
    #define write_bytes                 serialize_bytes
    #define write_string                serialize_string
    #define write_align                 serialize_align
    #define write_check                 serialize_check
    #define write_object                serialize_object
    #define write_address               serialize_address
    #define write_int_relative          serialize_int_relative
    #define write_ack_relative          serialize_ack_relative
    #define write_sequence_relative     serialize_sequence_relative

    /**
        Interface for an object that knows how to read, write and measure how many bits it would take up in a bit stream.
        IMPORTANT: Instead of overriding the serialize virtual methods method directly, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived class to override and redirect them to your templated serialize method.
        This way you can implement read and write for your messages in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you.
        See tests/shared.h for some examples of this.
        @see ReadStream
        @see WriteStream
        @see MeasureStream
     */
    
    class Serializable
    {  
    public:

        virtual ~Serializable() {}

        /**
            Virtual serialize function (read).
            Reads the object in from a bitstream.
            @param stream The stream to read from.
         */

        virtual bool SerializeInternal( class ReadStream & stream ) = 0;

        /**
            Virtual serialize function (write).
            Writes the object to a bitstream.
            @param stream The stream to write to.
         */

        virtual bool SerializeInternal( class WriteStream & stream ) = 0;

        /**
            Virtual serialize function (measure).
            Quickly measures how many bits the object would take if it were written to a bit stream.
            @param stream The read stream.
         */

        virtual bool SerializeInternal( class MeasureStream & stream ) = 0;
    };

    /**
        Helper macro to define virtual serialize functions for read, write and measure that call into the templated serialize function.
        This helps avoid writing boilerplate code, which is nice when you have lots of hand coded message types.
        See tests/shared.h for examples of usage.
     */

    #define YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()                                                               \
        bool SerializeInternal( class yojimbo::ReadStream & stream ) { return Serialize( stream ); };           \
        bool SerializeInternal( class yojimbo::WriteStream & stream ) { return Serialize( stream ); };          \
        bool SerializeInternal( class yojimbo::MeasureStream & stream ) { return Serialize( stream ); };         

    /**
        A reference counted object that can be serialized to a bitstream.

        Messages are objects that are sent between client and server across the connection. They are carried inside the ConnectionPacket generated by the Connection class. Messages can be sent reliable-ordered, or unreliable-unordered, depending on the configuration of the channel they are sent over.
        
        To use messages, create your own set of message classes by inheriting from this class (or from BlockMessage, if you want to attach data blocks to your message), then setup an enum of all your message types and derive a message factory class to create your message instances by type.
        
        There are macros to help make defining your message factory painless:
        
            YOJIMBO_MESSAGE_FACTORY_START
            YOJIMBO_DECLARE_MESSAGE_TYPE
            YOJIMBO_MESSAGE_FACTORY_FINISH
        
        Once you have a message factory, register it with your declared inside your client and server classes using:
        
            YOJIMBO_MESSAGE_FACTORY
        
        which overrides the Client::CreateMessageFactory and Server::CreateMessageFactory methods so the client and server classes use your message factory type.
        
        See tests/shared.h for an example showing you how to do this, and the functional tests inside tests/test.cpp for examples showing how how to send and receive messages.
        
        @see BlockMessage
        @see MessageFactory
        @see Connection
     */

    class Message : public Serializable
    {
    public:

        /**
            Message constructor.
            Don't call this directly, use a message factory instead.
            @param blockMessage 1 if this is a block message, 0 otherwise.
            @see MessageFactory::Create
         */

        Message( int blockMessage = 0 ) : m_refCount(1), m_id(0), m_type(0), m_blockMessage( blockMessage ) {}

        /** 
            Set the message id.
            When messages are sent over a reliable-ordered channel, the message id starts at 0 and increases with each message sent over that channel.
            When messages are sent over an unreliable-unordered channel, the message id is set to the sequence number of the packet it was delivered in.
            @param id The message id.
         */

        void SetId( uint16_t id ) { m_id = id; }

        /**
            Get the message id.
            @returns The message id.
         */

        int GetId() const { return m_id; }

        /**
            Get the message type.
            This corresponds to the type enum value used to create the message in the message factory.
            @returns The message type.
            @see MessageFactory.
         */

        int GetType() const { return m_type; }

        /**
            Get the reference count on the message.
            Messages start with a reference count of 1 when they are created. This is decreased when they are released. 
            When the reference count reaches 0, the message is destroyed.
            @returns The reference count on the message.
         */

        int GetRefCount() const { return m_refCount; }

        /**
            Is this a block message?
            Block messages are of type BlockMessage and can have a data block attached to the message.
            @returns True if this is a block message, false otherwise.
            @see BlockMessage.
         */

        bool IsBlockMessage() const { return m_blockMessage; }

        /**
            Virtual serialize function (read).
            Reads the message in from a bitstream.
            Don't override this method directly, instead, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived message class to redirect it to a templated serialize method.
            This way you can implement serialization for your packets in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you. 
            See tests/shared.h for examples of this.
         */

        virtual bool SerializeInternal( ReadStream & stream ) = 0;

        /**
            Virtual serialize function (write).
            Write the message to a bitstream.
            Don't override this method directly, instead, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived message class to redirect it to a templated serialize method.
            This way you can implement serialization for your packets in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you. 
            See tests/shared.h for examples of this.
         */

        virtual bool SerializeInternal( WriteStream & stream ) = 0;

        /**
            Virtual serialize function (measure).
            Measure how many bits this message would take to write. This is used when working out how many messages will fit within the channel packet budget.
            Don't override this method directly, instead, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived message class to redirect it to a templated serialize method.
            This way you can implement serialization for your packets in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you. 
            See tests/shared.h for examples of this.
         */

        virtual bool SerializeInternal ( MeasureStream & stream ) = 0;

    protected:

        /**
            Set the message type.
            Called by the message factory after it creates a message.
            @param type The message type.
         */

        void SetType( int type ) { m_type = type; }

        /**
            Add a reference to the message.
            This is called when a message is included in a packet and added to the receive queue. 
            This way we don't have to pass messages by value (more efficient) and messages get cleaned up when they are delivered and no packets refer to them.
         */

        void Acquire() { yojimbo_assert( m_refCount > 0 ); m_refCount++; }

        /**
            Remove a reference from the message.
            Message are deleted when the number of references reach zero. Messages have reference count of 1 after creation.
         */

        void Release() { yojimbo_assert( m_refCount > 0 ); m_refCount--; }

        /**
            Message destructor.
            @see MessageFactory::Release
         */

        virtual ~Message()
        {
            yojimbo_assert( m_refCount == 0 );
        }

    private:

        friend class MessageFactory;
      
        Message( const Message & other );        

        const Message & operator = ( const Message & other );

        int m_refCount;                             ///< Number of references on this message object. Starts at 1. Message is destroyed when it reaches 0.
        uint32_t m_id : 16;                         ///< The message id. For messages sent over reliable-ordered channels, this starts at 0 and increases with each message sent. For unreliable-unordered channels this is set to the sequence number of the packet the message was included in.
        uint32_t m_type : 15;                       ///< The message type. Corresponds to the type integer used when the message was created though the message factory.
        uint32_t m_blockMessage : 1;                ///< 1 if this is a block message. 0 otherwise. If 1 then you can cast the Message* to BlockMessage*. Lightweight RTTI.
    };

    /**
        A message which can have a block of data attached to it.
        @see ChannelConfig
     */

    class BlockMessage : public Message
    {
    public:

        /**
            Block message constructor.
            Don't call this directly, use a message factory instead.
            @see MessageFactory::CreateMessage
         */

        explicit BlockMessage() : Message( 1 ), m_allocator(NULL), m_blockData(NULL), m_blockSize(0) {}

        /**
            Attach a block to this message.
            You can only attach one block. This method will assert if a block is already attached.
            @see Client::AttachBlockToMessage
            @see Server::AttachBlockToMessage
         */

        void AttachBlock( Allocator & allocator, uint8_t * blockData, int blockSize )
        {
            yojimbo_assert( blockData );
            yojimbo_assert( blockSize > 0 );
            yojimbo_assert( !m_blockData );
            m_allocator = &allocator;
            m_blockData = blockData;
            m_blockSize = blockSize;
        }

        /** 
            Detach the block from this message.
            By doing this you are responsible for copying the block pointer and allocator and making sure the block is freed.
            This could be used for example, if you wanted to copy off the block and store it somewhere, without the cost of copying it.
            @see Client::DetachBlockFromMessage
            @see Server::DetachBlockFromMessage
         */

        void DetachBlock()
        {
            m_allocator = NULL;
            m_blockData = NULL;
            m_blockSize = 0;
        }

        /**
            Get the allocator used to allocate the block.
            @returns The allocator for the block. NULL if no block is attached to this message.
         */

        Allocator * GetAllocator()
        {
            return m_allocator;
        }

        /**
            Get the block data pointer.
            @returns The block data pointer. NULL if no block is attached.
         */

        uint8_t * GetBlockData()
        {
            return m_blockData;
        }

        /**
            Get a constant pointer to the block data.
            @returns A constant pointer to the block data. NULL if no block is attached.
         */

        const uint8_t * GetBlockData() const
        {
            return m_blockData;
        }

        /**
            Get the size of the block attached to this message.
            @returns The size of the block (bytes). 0 if no block is attached.
         */

        int GetBlockSize() const
        {
            return m_blockSize;
        }

        /**
            Templated serialize function for the block message. Doesn't do anything. The block data is serialized elsewhere.
            You can override the serialize methods on a block message to implement your own serialize function. It's just like a regular message with a block attached to it.
            @see ConnectionPacket
            @see ChannelPacketData
            @see ReliableOrderedChannel
            @see UnreliableUnorderedChannel
         */

        template <typename Stream> bool Serialize( Stream & stream ) { (void) stream; return true; }

        YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();

    protected:

        /**
            If a block was attached to the message, it is freed here.
         */

        ~BlockMessage()
        {
            if ( m_allocator )
            {
                YOJIMBO_FREE( *m_allocator, m_blockData );
                m_blockSize = 0;
                m_allocator = NULL;
            }
        }

    private:

        Allocator * m_allocator;                    ///< Allocator for the block attached to the message. NULL if no block is attached.
        uint8_t * m_blockData;                      ///< The block data. NULL if no block is attached.
        int m_blockSize;                            ///< The block size (bytes). 0 if no block is attached.
    };

    /**
        Message factory error level.
     */

    enum MessageFactoryErrorLevel
    {
        MESSAGE_FACTORY_ERROR_NONE,                                             ///< No error. All is well.
        MESSAGE_FACTORY_ERROR_FAILED_TO_ALLOCATE_MESSAGE,                       ///< Failed to allocate a message. Typically this means we ran out of memory on the allocator backing the message factory.
    };

    /**
        Defines the set of message types that can be created.

        You can derive a message factory yourself to create your own message types, or you can use these helper macros to do it for you:
        
            YOJIMBO_MESSAGE_FACTORY_START
            YOJIMBO_DECLARE_MESSAGE_TYPE
            YOJIMBO_MESSAGE_FACTORY_FINISH
        
        See tests/shared.h for an example showing how to use the macros.
     */

    class MessageFactory
    {        
    public:

        /**
            Message factory allocator.
            Pass in the number of message types for the message factory from the derived class.
            @param allocator The allocator used to create messages.
            @param numTypes The number of message types. Valid types are in [0,numTypes-1].
         */

        MessageFactory( Allocator & allocator, int numTypes )
        {
            m_allocator = &allocator;
            m_numTypes = numTypes;
            m_errorLevel = MESSAGE_FACTORY_ERROR_NONE;
        }

        /**
            Message factory destructor.
            Checks for message leaks if YOJIMBO_DEBUG_MESSAGE_LEAKS is defined and not equal to zero. This is on by default in debug build.
         */

        virtual ~MessageFactory()
        {
            yojimbo_assert( m_allocator );

            m_allocator = NULL;

            #if YOJIMBO_DEBUG_MESSAGE_LEAKS
            if ( allocated_messages.size() )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "you leaked messages!\n" );
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "%d messages leaked\n", (int) allocated_messages.size() );
                typedef std::map<void*,int>::iterator itor_type;
                for ( itor_type i = allocated_messages.begin(); i != allocated_messages.end(); ++i ) 
                {
                    Message * message = (Message*) i->first;
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "leaked message %p (type %d, refcount %d)\n", message, message->GetType(), message->GetRefCount() );
                }
                exit(1);
            }
            #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
        }

        /**
            Create a message by type.
            IMPORTANT: Check the message pointer returned by this call. It can be NULL if there is no memory to create a message!
            Messages returned from this function have one reference added to them. When you are finished with the message, pass it to MessageFactory::Release.
            @param type The message type in [0,numTypes-1].
            @returns The allocated message, or NULL if the message could not be allocated. If the message allocation fails, the message factory error level is set to MESSAGE_FACTORY_ERROR_FAILED_TO_ALLOCATE_MESSAGE.
            @see MessageFactory::AddRef
            @see MessageFactory::ReleaseMessage
         */

        Message * CreateMessage( int type )
        {
            yojimbo_assert( type >= 0 );
            yojimbo_assert( type < m_numTypes );
            Message * message = CreateMessageInternal( type );
            if ( !message )
            {
                m_errorLevel = MESSAGE_FACTORY_ERROR_FAILED_TO_ALLOCATE_MESSAGE;
                return NULL;
            }
            #if YOJIMBO_DEBUG_MESSAGE_LEAKS
            allocated_messages[message] = 1;
            yojimbo_assert( allocated_messages.find( message ) != allocated_messages.end() );
            #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
            return message;
        }

        /**
            Add a reference to a message.
            @param message The message to add a reference to.
            @see MessageFactory::Create
            @see MessageFactory::Release
         */   

        void AcquireMessage( Message * message )
        {
            yojimbo_assert( message );
            if ( message )
                message->Acquire();
        }

        /**
            Remove a reference from a message.
            Messages have 1 reference when created. When the reference count reaches 0, they are destroyed.
            @see MessageFactory::Create
            @see MessageFactory::AddRef
         */

        void ReleaseMessage( Message * message )
        {
            yojimbo_assert( message );
            if ( !message )
            {
                return;
            }
            message->Release();
            if ( message->GetRefCount() == 0 )
            {
                #if YOJIMBO_DEBUG_MESSAGE_LEAKS
                yojimbo_assert( allocated_messages.find( message ) != allocated_messages.end() );
                allocated_messages.erase( message );
                #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
                yojimbo_assert( m_allocator );
                YOJIMBO_DELETE( *m_allocator, Message, message );
            }
        }

        /**
            Get the number of message types supported by this message factory.
            @returns The number of message types.
         */

        int GetNumTypes() const
        {
            return m_numTypes;
        }

        /**
            Get the allocator used to create messages.
            @returns The allocator.
         */

        Allocator & GetAllocator()
        {
            yojimbo_assert( m_allocator );
            return *m_allocator;
        }

        /**
            Get the error level.
            When used with a client or server, an error level on a message factory other than MESSAGE_FACTORY_ERROR_NONE triggers a client disconnect.
         */

        MessageFactoryErrorLevel GetErrorLevel() const
        {
            return m_errorLevel;
        }

        /**
            Clear the error level back to no error.
         */

        void ClearErrorLevel()
        {
            m_errorLevel = MESSAGE_FACTORY_ERROR_NONE;
        }

    protected:

        /**
            This method is overridden to create messages by type.
            @param type The type of message to be created.
            @returns The message created. Its reference count is 1.
         */

        virtual Message * CreateMessageInternal( int type ) { (void) type; return NULL; }

        /**
            Set the message type of a message.
            @param message The message object.
            @param type The message type to set.
         */

        void SetMessageType( Message * message, int type ) { message->SetType( type ); }

    private:

        #if YOJIMBO_DEBUG_MESSAGE_LEAKS
        std::map<void*,int> allocated_messages;                                 ///< The set of allocated messages for this factory. Used to track down message leaks.
        #endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
        
        Allocator * m_allocator;                                                ///< The allocator used to create messages.
        
        int m_numTypes;                                                         ///< The number of message types.
        
        MessageFactoryErrorLevel m_errorLevel;                                  ///< The message factory error level.
    };
}

/** 
    Start a definition of a new message factory.
    This is a helper macro to make declaring your own message factory class easier.
    @param factory_class The name of the message factory class to generate.
    @param num_message_types The number of message types for this factory.
    See tests/shared.h for an example of usage.
 */

#define YOJIMBO_MESSAGE_FACTORY_START( factory_class, num_message_types )                                                               \
                                                                                                                                        \
    class factory_class : public yojimbo::MessageFactory                                                                                \
    {                                                                                                                                   \
    public:                                                                                                                             \
        factory_class( yojimbo::Allocator & allocator ) : MessageFactory( allocator, num_message_types ) {}                             \
        yojimbo::Message * CreateMessageInternal( int type )                                                                            \
        {                                                                                                                               \
            yojimbo::Message * message;                                                                                                 \
            yojimbo::Allocator & allocator = GetAllocator();                                                                            \
            (void) allocator;                                                                                                           \
            switch ( type )                                                                                                             \
            {                                                                                                                           \

/** 
    Add a message type to a message factory.
    This is a helper macro to make declaring your own message factory class easier.
    @param message_type The message type value. This is typically an enum value.
    @param message_class The message class to instantiate when a message of this type is created.
    See tests/shared.h for an example of usage.
 */

#define YOJIMBO_DECLARE_MESSAGE_TYPE( message_type, message_class )                                                                     \
                                                                                                                                        \
                case message_type:                                                                                                      \
                    message = YOJIMBO_NEW( allocator, message_class );                                                                  \
                    if ( !message )                                                                                                     \
                        return NULL;                                                                                                    \
                    SetMessageType( message, message_type );                                                                            \
                    return message;

/** 
    Finish the definition of a new message factory.
    This is a helper macro to make declaring your own message factory class easier.
    See tests/shared.h for an example of usage.
 */

#define YOJIMBO_MESSAGE_FACTORY_FINISH()                                                                                                \
                                                                                                                                        \
                default: return NULL;                                                                                                   \
            }                                                                                                                           \
        }                                                                                                                               \
    };

namespace yojimbo
{
    struct ChannelPacketData
    {
        uint32_t channelIndex : 16;
        uint32_t initialized : 1;
        uint32_t blockMessage : 1;
        uint32_t messageFailedToSerialize : 1;

        struct MessageData
        {
            int numMessages;
            Message ** messages;
        };

        struct BlockData
        {
            BlockMessage * message;
            uint8_t * fragmentData;
            uint64_t messageId : 16;
            uint64_t fragmentId : 16;
            uint64_t fragmentSize : 16;
            uint64_t numFragments : 16;
            int messageType;
        };

        union
        {
            MessageData message;
            BlockData block;
        };

        void Initialize();

        void Free( MessageFactory & messageFactory );

        template <typename Stream> bool Serialize( Stream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( MeasureStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );
    };

    /**
        Channel counters provide insight into the number of times an action was performed by a channel.
        They are intended for use in a telemetry system, eg. reported to some backend logging system to track behavior in a production environment.
     */

    enum ChannelCounters
    {
        CHANNEL_COUNTER_MESSAGES_SENT,                          ///< Number of messages sent over this channel.
        CHANNEL_COUNTER_MESSAGES_RECEIVED,                      ///< Number of messages received over this channel.
        CHANNEL_COUNTER_NUM_COUNTERS                            ///< The number of channel counters.
    };

    /**
        Channel error level.
        If the channel gets into an error state, it sets an error state on the corresponding connection. See yojimbo::CONNECTION_ERROR_CHANNEL.
        This way if any channel on a client/server connection gets into a bad state, that client is automatically kicked from the server.
        @see Client
        @see Server
        @see Connection
     */

    enum ChannelErrorLevel
    {
        CHANNEL_ERROR_NONE = 0,                                 ///< No error. All is well.
        CHANNEL_ERROR_DESYNC,                                   ///< This channel has desynced. This means that the connection protocol has desynced and cannot recover. The client should be disconnected.
        CHANNEL_ERROR_SEND_QUEUE_FULL,                          ///< The user tried to send a message but the send queue was full. This will assert out in development, but in production it sets this error on the channel.
        CHANNEL_ERROR_BLOCKS_DISABLED,                          ///< The channel received a packet containing data for blocks, but this channel is configured to disable blocks. See ChannelConfig::disableBlocks.
        CHANNEL_ERROR_FAILED_TO_SERIALIZE,                      ///< Serialize read failed for a message sent to this channel. Check your message serialize functions, one of them is returning false on serialize read. This can also be caused by a desync in message read and write.
        CHANNEL_ERROR_OUT_OF_MEMORY,                            ///< The channel tried to allocate some memory but couldn't.
    };

    /// Helper function to convert a channel error to a user friendly string.

    inline const char * GetChannelErrorString( ChannelErrorLevel error )
    {
        switch ( error )
        {
            case CHANNEL_ERROR_NONE:                    return "none";
            case CHANNEL_ERROR_DESYNC:                  return "desync";
            case CHANNEL_ERROR_SEND_QUEUE_FULL:         return "send queue full";
            case CHANNEL_ERROR_OUT_OF_MEMORY:           return "out of memory";
            case CHANNEL_ERROR_BLOCKS_DISABLED:         return "blocks disabled";
            case CHANNEL_ERROR_FAILED_TO_SERIALIZE:     return "failed to serialize";
            default:
                yojimbo_assert( false );
                return "(unknown)";
        }
    }

    /// Common functionality shared across all channel types.

    class Channel
    {
    public:

        /**
            Channel constructor.
         */

        Channel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time );

        /**
            Channel destructor.
         */

        virtual ~Channel() {}

        /**
            Reset the channel. 
         */

        virtual void Reset() = 0;

        /**
            Returns true if a message can be sent over this channel.
         */            

        virtual bool CanSendMessage() const = 0;

        /**
            Are there any messages in the send queue?
            @returns True if there is at least one message in the send queue.            
         */

         virtual bool HasMessagesToSend() const = 0;

        /**
            Queue a message to be sent across this channel.
            @param message The message to be sent.
         */

        virtual void SendMessage( Message * message, void *context) = 0;

        /** 
            Pops the next message off the receive queue if one is available.
            @returns A pointer to the received message, NULL if there are no messages to receive. The caller owns the message object returned by this function and is responsible for releasing it via Message::Release.
         */

        virtual Message * ReceiveMessage() = 0;

        /**
            Advance channel time.
            Called by Connection::AdvanceTime for each channel configured on the connection.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Get channel packet data for this channel.
            @param packetData The channel packet data to be filled [out]
            @param packetSequence The sequence number of the packet being generated.
            @param availableBits The maximum number of bits of packet data the channel is allowed to write.
            @returns The number of bits of packet data written by the channel.
            @see ConnectionPacket
            @see Connection::GeneratePacket
         */

        virtual int GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits) = 0;

        /**
            Process packet data included in a connection packet.
            @param packetData The channel packet data to process.
            @param packetSequence The sequence number of the connection packet that contains the channel packet data.
            @see ConnectionPacket
            @see Connection::ProcessPacket
         */

        virtual void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence ) = 0;

        /**
            Process a connection packet ack.
            Depending on the channel type: 
                1. Acks messages and block fragments so they stop being included in outgoing connection packets (reliable-ordered channel), 
                2. Does nothing at all (unreliable-unordered).
            @param sequence The sequence number of the connection packet that was acked.
         */

        virtual void ProcessAck( uint16_t sequence ) = 0;

    public:

        /**
            Get the channel error level.
            @returns The channel error level.
         */

        ChannelErrorLevel GetErrorLevel() const;

        /** 
            Gets the channel index.
            @returns The channel index in [0,numChannels-1].
         */

        int GetChannelIndex() const;

        /**
            Get a counter value.
            @param index The index of the counter to retrieve. See ChannelCounters.
            @returns The value of the counter.
            @see ResetCounters
         */

        uint64_t GetCounter( int index ) const;

        /**
            Resets all counter values to zero.
         */

        void ResetCounters();

    protected:

        /**
            Set the channel error level.
            All errors go through this function to make debug logging easier. 
         */
        
        void SetErrorLevel( ChannelErrorLevel errorLevel );

    protected:

        const ChannelConfig m_config;                                                   ///< Channel configuration data.
        Allocator * m_allocator;                                                        ///< Allocator for allocations matching life cycle of this channel.
        int m_channelIndex;                                                             ///< The channel index in [0,numChannels-1].
        double m_time;                                                                  ///< The current time.
        ChannelErrorLevel m_errorLevel;                                                 ///< The channel error level.
        MessageFactory * m_messageFactory;                                              ///< Message factory for creating and destroying messages.
        uint64_t m_counters[CHANNEL_COUNTER_NUM_COUNTERS];                              ///< Counters for unit testing, stats etc.
    };

    /**
        Messages sent across this channel are guaranteed to arrive in the order they were sent.
        This channel type is best used for control messages and RPCs.
        Messages sent over this channel are included in connection packets until one of those packets is acked. Messages are acked individually and remain in the send queue until acked.
        Blocks attached to messages sent over this channel are split up into fragments. Each fragment of the block is included in a connection packet until one of those packets are acked. Eventually, all fragments are received on the other side, and block is reassembled and attached to the message.
        Only one message block may be in flight over the network at any time, so blocks stall out message delivery slightly. Therefore, only use blocks for large data that won't fit inside a single connection packet where you actually need the channel to split it up into fragments. If your block fits inside a packet, just serialize it inside your message serialize via serialize_bytes instead.
     */

    class ReliableOrderedChannel : public Channel
    {
    public:

        /** 
            Reliable ordered channel constructor.
            @param allocator The allocator to use.
            @param messageFactory Message factory for creating and destroying messages.
            @param config The configuration for this channel.
            @param channelIndex The channel index in [0,numChannels-1].
         */

        ReliableOrderedChannel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time );

        /**
            Reliable ordered channel destructor.
            Any messages still in the send or receive queues will be released.
         */

        ~ReliableOrderedChannel();

        void Reset();

        bool CanSendMessage() const;

        void SendMessage( Message * message, void *context );

        Message * ReceiveMessage();

        void AdvanceTime( double time );

        int GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits );

        void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence );

        void ProcessAck( uint16_t ack );

        /**
            Are there any unacked messages in the send queue?
            Messages are acked individually and remain in the send queue until acked.
            @returns True if there is at least one unacked message in the send queue.            
         */

        bool HasMessagesToSend() const;

        /**
            Get messages to include in a packet.
            Messages are measured to see how many bits they take, and only messages that fit within the channel packet budget will be included. See ChannelConfig::packetBudget.
            Takes care not to send messages too rapidly by respecting ChannelConfig::messageResendTime for each message, and to only include messages that that the receiver is able to buffer in their receive queue. In other words, won't run ahead of the receiver.
            @param messageIds Array of message ids to be filled [out]. Fills up to ChannelConfig::maxMessagesPerPacket messages, make sure your array is at least this size.
            @param numMessageIds The number of message ids written to the array.
            @param remainingPacketBits Number of bits remaining in the packet. Considers this as a hard limit when determining how many messages can fit into the packet.
            @returns Estimate of the number of bits required to serialize the messages (upper bound).
            @see GetMessagePacketData
         */

        int GetMessagesToSend( uint16_t * messageIds, int & numMessageIds, int remainingPacketBits, void *context );

        /**
            Fill channel packet data with messages.
            This is the payload function to fill packet data while sending regular messages (without blocks attached).
            Messages have references added to them when they are added to the packet. They also have a reference while they are stored in a send or receive queue. Messages are cleaned up when they are no longer in a queue, and no longer referenced by any packets.
            @param packetData The packet data to fill [out]
            @param messageIds Array of message ids identifying which messages to add to the packet from the message send queue.
            @param numMessageIds The number of message ids in the array.
            @see GetMessagesToSend
         */

        void GetMessagePacketData( ChannelPacketData & packetData, const uint16_t * messageIds, int numMessageIds );

        /**
            Add a packet entry for the set of messages included in a packet.
            This lets us look up the set of messages that were included in that packet later on when it is acked, so we can ack those messages individually.
            @param messageIds The set of message ids that were included in the packet.
            @param numMessageIds The number of message ids in the array.
            @param sequence The sequence number of the connection packet the messages were included in.
         */

        void AddMessagePacketEntry( const uint16_t * messageIds, int numMessageIds, uint16_t sequence );

        /**
            Process messages included in a packet.
            Any messages that have not already been received are added to the message receive queue. Messages that are added to the receive queue have a reference added. See Message::AddRef.
            @param numMessages The number of messages to process.
            @param messages Array of pointers to messages.
         */

        void ProcessPacketMessages( int numMessages, Message ** messages );

        /**
            Track the oldest unacked message id in the send queue.
            Because messages are acked individually, the send queue is not a true queue and may have holes. 
            Because of this it is necessary to periodically walk forward from the previous oldest unacked message id, to find the current oldest unacked message id. 
            This lets us know our starting point for considering messages to include in the next packet we send.
            @see GetMessagesToSend
         */

        void UpdateOldestUnackedMessageId();

        /**
            True if we are currently sending a block message.
            Block messages are treated differently to regular messages. 
            Regular messages are small so we try to fit as many into the packet we can. See ReliableChannelData::GetMessagesToSend.
            Blocks attached to block messages are usually larger than the maximum packet size or channel budget, so they are split up fragments. 
            While in the mode of sending a block message, each channel packet data generated has exactly one fragment from the current block in it. Fragments keep getting included in packets until all fragments of that block are acked.
            @returns True if currently sending a block message over the network, false otherwise.
            @see BlockMessage
            @see GetFragmentToSend
         */

        bool SendingBlockMessage();

        /**
            Get the next block fragment to send.
            The next block fragment is selected by scanning left to right over the set of fragments in the block, skipping over any fragments that have already been acked or have been sent within ChannelConfig::fragmentResendTime.
            @param messageId The id of the message that the block is attached to [out].
            @param fragmentId The id of the fragment to send [out].
            @param fragmentBytes The size of the fragment in bytes.
            @param numFragments The total number of fragments in this block.
            @param messageType The type of message the block is attached to. See MessageFactory.
            @returns Pointer to the fragment data.
         */

        uint8_t * GetFragmentToSend( uint16_t & messageId, uint16_t & fragmentId, int & fragmentBytes, int & numFragments, int & messageType );

        /**
            Fill the packet data with block and fragment data.
            This is the payload function that fills the channel packet data while we are sending a block message.
            @param packetData The packet data to fill [out]
            @param messageId The id of the message that the block is attached to.
            @param fragmentId The id of the block fragment being sent.
            @param fragmentData The fragment data.
            @param fragmentSize The size of the fragment data (bytes).
            @param numFragments The number of fragments in the block.
            @param messageType The type of message the block is attached to.
            @returns An estimate of the number of bits required to serialize the block message and fragment data (upper bound).
         */

        int GetFragmentPacketData( ChannelPacketData & packetData, 
                                   uint16_t messageId, 
                                   uint16_t fragmentId, 
                                   uint8_t * fragmentData, 
                                   int fragmentSize, 
                                   int numFragments, 
                                   int messageType );

        /**
            Adds a packet entry for the fragment.
            This lets us look up the fragment that was in the packet later on when it is acked, so we can ack that block fragment.
            @param messageId The message id that the block was attached to.
            @param fragmentId The fragment id.
            @param sequence The sequence number of the packet the fragment was included in.
         */

        void AddFragmentPacketEntry( uint16_t messageId, uint16_t fragmentId, uint16_t sequence );

        /**
            Process a packet fragment.
            The fragment is added to the set of received fragments for the block. When all packet fragments are received, that block is reconstructed, attached to the block message and added to the message receive queue.
            @param messageType The type of the message this block fragment is attached to. This is used to make sure this message type actually allows blocks to be attached to it.
            @param messageId The id of the message the block fragment belongs to.
            @param numFragments The number of fragments in the block.
            @param fragmentId The id of the fragment in [0,numFragments-1].
            @param fragmentData The fragment data.
            @param fragmentBytes The size of the fragment data in bytes.
            @param blockMessage Pointer to the block message. Passed this in only with the first fragment (0), pass NULL for all other fragments.
         */

        void ProcessPacketFragment( int messageType, 
                                    uint16_t messageId, 
                                    int numFragments, 
                                    uint16_t fragmentId, 
                                    const uint8_t * fragmentData, 
                                    int fragmentBytes, 
                                    BlockMessage * blockMessage );

    protected:

        /**
            An entry in the send queue of the reliable-ordered channel.
            Messages stay into the send queue until acked. Each message is acked individually, so there can be "holes" in the message send queue.
         */

        struct MessageSendQueueEntry
        {
            Message * message;                                                          ///< Pointer to the message. When inserted in the send queue the message has one reference. It is released when the message is acked and removed from the send queue.
            double timeLastSent;                                                        ///< The time the message was last sent. Used to implement ChannelConfig::messageResendTime.
            uint32_t measuredBits : 31;                                                 ///< The number of bits the message takes up in a bit stream.
            uint32_t block : 1;                                                         ///< 1 if this is a block message. Block messages are treated differently to regular messages when sent over a reliable-ordered channel.
        };

        /**
            An entry in the receive queue of the reliable-ordered channel.
         */

        struct MessageReceiveQueueEntry
        {
            Message * message;                                                          ///< The message pointer. Has at a reference count of at least 1 while in the receive queue. Ownership of the message is passed back to the caller when the message is dequeued.
        };

        /**
            Maps packet level acks to messages and fragments for the reliable-ordered channel.
         */

        struct SentPacketEntry
        {
            double timeSent;                                                            ///< The time the packet was sent. Used to estimate round trip time.
            uint16_t * messageIds;                                                      ///< Pointer to an array of message ids. Dynamically allocated because the user can configure the maximum number of messages in a packet per-channel with ChannelConfig::maxMessagesPerPacket.
            uint32_t numMessageIds : 16;                                                ///< The number of message ids in in the array.
            uint32_t acked : 1;                                                         ///< 1 if this packet has been acked.
            uint64_t block : 1;                                                         ///< 1 if this packet contains a fragment of a block message.
            uint64_t blockMessageId : 16;                                               ///< The block message id. Valid only if "block" is 1.
            uint64_t blockFragmentId : 16;                                              ///< The block fragment id. Valid only if "block" is 1.
        };

        /**
            Internal state for a block being sent across the reliable ordered channel.
            Stores the block data and tracks which fragments have been acked. The block send completes when all fragments have been acked.
            IMPORTANT: Although there can be multiple block messages in the message send and receive queues, only one data block can be in flights over the wire at a time.
         */

        struct SendBlockData
        {
            SendBlockData( Allocator & allocator, int maxFragmentsPerBlock )
            {
                m_allocator = &allocator;
                ackedFragment = YOJIMBO_NEW( allocator, BitArray, allocator, maxFragmentsPerBlock );
                fragmentSendTime = (double*) YOJIMBO_ALLOCATE( allocator, sizeof( double) * maxFragmentsPerBlock );
                yojimbo_assert( ackedFragment );
                yojimbo_assert( fragmentSendTime );
                Reset();
            }

            ~SendBlockData()
            {
                YOJIMBO_DELETE( *m_allocator, BitArray, ackedFragment );
                YOJIMBO_FREE( *m_allocator, fragmentSendTime );
            }

            void Reset()
            {
                active = false;
                numFragments = 0;
                numAckedFragments = 0;
                blockMessageId = 0;
                blockSize = 0;
            }

            bool active;                                                                ///< True if we are currently sending a block.
            int blockSize;                                                              ///< The size of the block (bytes).
            int numFragments;                                                           ///< Number of fragments in the block being sent.
            int numAckedFragments;                                                      ///< Number of acked fragments in the block being sent.
            uint16_t blockMessageId;                                                    ///< The message id the block is attached to.
            BitArray * ackedFragment;                                                   ///< Has fragment n been received?
            double * fragmentSendTime;                                                  ///< Last time fragment was sent.

        private:

            Allocator * m_allocator;                                                    ///< Allocator used to create the block data.
        
            SendBlockData( const SendBlockData & other );
            
            SendBlockData & operator = ( const SendBlockData & other );
        };

        /**
            Internal state for a block being received across the reliable ordered channel.
            Stores the fragments received over the network for the block, and completes once all fragments have been received.
            IMPORTANT: Although there can be multiple block messages in the message send and receive queues, only one data block can be in flights over the wire at a time.
         */

        struct ReceiveBlockData
        {
            ReceiveBlockData( Allocator & allocator, int maxBlockSize, int maxFragmentsPerBlock )
            {
                m_allocator = &allocator;
                receivedFragment = YOJIMBO_NEW( allocator, BitArray, allocator, maxFragmentsPerBlock );
                blockData = (uint8_t*) YOJIMBO_ALLOCATE( allocator, maxBlockSize );
                yojimbo_assert( receivedFragment && blockData );
                blockMessage = NULL;
                Reset();
            }

            ~ReceiveBlockData()
            {
                YOJIMBO_DELETE( *m_allocator, BitArray, receivedFragment );
                YOJIMBO_FREE( *m_allocator, blockData );
            }

            void Reset()
            {
                active = false;
                numFragments = 0;
                numReceivedFragments = 0;
                messageId = 0;
                messageType = 0;
                blockSize = 0;
            }

            bool active;                                                                ///< True if we are currently receiving a block.
            int numFragments;                                                           ///< The number of fragments in this block
            int numReceivedFragments;                                                   ///< The number of fragments received.
            uint16_t messageId;                                                         ///< The message id corresponding to the block.
            int messageType;                                                            ///< Message type of the block being received.
            uint32_t blockSize;                                                         ///< Block size in bytes.
            BitArray * receivedFragment;                                                ///< Has fragment n been received?
            uint8_t * blockData;                                                        ///< Block data for receive.
            BlockMessage * blockMessage;                                                ///< Block message (sent with fragment 0).

        private:

            Allocator * m_allocator;                                                    ///< Allocator used to free the data on shutdown.

            ReceiveBlockData( const ReceiveBlockData & other );
            
            ReceiveBlockData & operator = ( const ReceiveBlockData & other );
        };

    private:

        uint16_t m_sendMessageId;                                                       ///< Id of the next message to be added to the send queue.
        uint16_t m_receiveMessageId;                                                    ///< Id of the next message to be added to the receive queue.
        uint16_t m_oldestUnackedMessageId;                                              ///< Id of the oldest unacked message in the send queue.
        SequenceBuffer<SentPacketEntry> * m_sentPackets;                                ///< Stores information per sent connection packet about messages and block data included in each packet. Used to walk from connection packet level acks to message and data block fragment level acks.
        SequenceBuffer<MessageSendQueueEntry> * m_messageSendQueue;                     ///< Message send queue.
        SequenceBuffer<MessageReceiveQueueEntry> * m_messageReceiveQueue;               ///< Message receive queue.
        uint16_t * m_sentPacketMessageIds;                                              ///< Array of n message ids per sent connection packet. Allows the maximum number of messages per-packet to be allocated dynamically.
        SendBlockData * m_sendBlock;                                                    ///< Data about the block being currently sent.
        ReceiveBlockData * m_receiveBlock;                                              ///< Data about the block being currently received.

    private:

        ReliableOrderedChannel( const ReliableOrderedChannel & other );

        ReliableOrderedChannel & operator = ( const ReliableOrderedChannel & other );
    };

    /**
        Messages sent across this channel are not guaranteed to arrive, and may be received in a different order than they were sent.
        This channel type is best used for time critical data like snapshots and object state.
     */

    class UnreliableUnorderedChannel : public Channel
    {
    public:

        /** 
            Reliable ordered channel constructor.
            @param allocator The allocator to use.
            @param messageFactory Message factory for creating and destroying messages.
            @param config The configuration for this channel.
            @param channelIndex The channel index in [0,numChannels-1].
         */

        UnreliableUnorderedChannel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time );

        /**
            Unreliable unordered channel destructor.
            Any messages still in the send or receive queues will be released.
         */

        ~UnreliableUnorderedChannel();

        void Reset();

        bool CanSendMessage() const;

        bool HasMessagesToSend() const;

        void SendMessage( Message * message, void *context );

        Message * ReceiveMessage();

        void AdvanceTime( double time );

        int GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits );

        void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence );

        void ProcessAck( uint16_t ack );

    protected:

        Queue<Message*> * m_messageSendQueue;                   ///< Message send queue.
        Queue<Message*> * m_messageReceiveQueue;                ///< Message receive queue.

    private:

        UnreliableUnorderedChannel( const UnreliableUnorderedChannel & other );

        UnreliableUnorderedChannel & operator = ( const UnreliableUnorderedChannel & other );
    };

    /// Connection error level.

    enum ConnectionErrorLevel
    {
        CONNECTION_ERROR_NONE = 0,                              ///< No error. All is well.
        CONNECTION_ERROR_CHANNEL,                               ///< A channel is in an error state.
        CONNECTION_ERROR_ALLOCATOR,                             ///< The allocator is an error state.
        CONNECTION_ERROR_MESSAGE_FACTORY,                       ///< The message factory is in an error state.
        CONNECTION_ERROR_READ_PACKET_FAILED,                    ///< Failed to read packet. Received an invalid packet?     
    };

    /**
        Sends and receives messages across a set of user defined channels.
     */

    class Connection
    {
    public:

        Connection( Allocator & allocator, MessageFactory & messageFactory, const ConnectionConfig & connectionConfig, double time );

        ~Connection();

        void Reset();

        bool CanSendMessage( int channelIndex ) const;

        bool HasMessagesToSend( int channelIndex ) const;

        void SendMessage( int channelIndex, Message * message, void *context = 0);

        Message * ReceiveMessage( int channelIndex );

        void ReleaseMessage( Message * message );

        bool GeneratePacket( void * context, uint16_t packetSequence, uint8_t * packetData, int maxPacketBytes, int & packetBytes );

        bool ProcessPacket( void * context, uint16_t packetSequence, const uint8_t * packetData, int packetBytes );

        void ProcessAcks( const uint16_t * acks, int numAcks );

        void AdvanceTime( double time );

        ConnectionErrorLevel GetErrorLevel() { return m_errorLevel; }

    private:

        Allocator * m_allocator;                                ///< Allocator passed in to the connection constructor.
        MessageFactory * m_messageFactory;                      ///< Message factory for creating and destroying messages.
        ConnectionConfig m_connectionConfig;                    ///< Connection configuration.
        Channel * m_channel[MaxChannels];                       ///< Array of connection channels. Array size corresponds to m_connectionConfig.numChannels
        ConnectionErrorLevel m_errorLevel;                      ///< The connection error level.
    };

    /**
        Simulates packet loss, latency, jitter and duplicate packets.
        This is useful during development, so your game is tested and played under real world conditions, instead of ideal LAN conditions.
        This simulator works on packet send. This means that if you want 125ms of latency (round trip), you must to add 125/2 = 62.5ms of latency to each side.
     */

    class NetworkSimulator
    {
    public:

        /**
            Create a network simulator.
            Initial network conditions are set to:
                Latency: 0ms
                Jitter: 0ms
                Packet Loss: 0%
                Duplicates: 0%
            @param allocator The allocator to use.
            @param numPackets The maximum number of packets that can be stored in the simulator at any time.
            @param time The initial time value in seconds.
         */

        NetworkSimulator( Allocator & allocator, int numPackets, double time );

        /**
            Network simulator destructor.
            Any packet data still in the network simulator is destroyed.
         */

        ~NetworkSimulator();

        /**
            Set the latency in milliseconds.
            This latency is added on packet send. To simulate a round trip time of 100ms, add 50ms of latency to both sides of the connection.
            @param milliseconds The latency to add in milliseconds.
         */

        void SetLatency( float milliseconds );

        /**
            Set the packet jitter in milliseconds.
            Jitter is applied +/- this amount in milliseconds. To be truly effective, jitter must be applied together with some latency.
            @param milliseconds The amount of jitter to add in milliseconds (+/-).
         */

        void SetJitter( float milliseconds );

        /**
            Set the amount of packet loss to apply on send.
            @param percent The packet loss percentage. 0% = no packet loss. 100% = all packets are dropped.
         */

        void SetPacketLoss( float percent );

        /**
            Set percentage chance of packet duplicates.
            If the duplicate chance succeeds, a duplicate packet is added to the queue with a random delay of up to 1 second.
            @param percent The percentage chance of a packet duplicate being sent. 0% = no duplicate packets. 100% = all packets have a duplicate sent.
         */

        void SetDuplicates( float percent );

        /**
            Is the network simulator active?
            The network simulator is active when packet loss, latency, duplicates or jitter are non-zero values.
            This is used by the transport to know whether it should shunt packets through the simulator, or send them directly to the network. This is a minor optimization.
         */

        bool IsActive() const;

        /**
            Queue a packet to send.
            IMPORTANT: Ownership of the packet data pointer is *not* transferred to the network simulator. It makes a copy of the data instead.
            @param to The slot index the packet should be sent to.
            @param packetData The packet data.
            @param packetBytes The packet size (bytes).
         */
        
        void SendPacket( int to, uint8_t * packetData, int packetBytes );

        /**
            Receive packets sent to any address.
            IMPORTANT: You take ownership of the packet data you receive and are responsible for freeing it. See NetworkSimulator::GetAllocator.
            @param maxPackets The maximum number of packets to receive.
            @param packetData Array of packet data pointers to be filled [out].
            @param packetBytes Array of packet sizes to be filled [out].
            @param to Array of to indices to be filled [out].
            @returns The number of packets received.
         */

        int ReceivePackets( int maxPackets, uint8_t * packetData[], int packetBytes[], int to[] );

        /**
            Discard all packets in the network simulator.
            This is useful if the simulator needs to be reset and used for another purpose.
         */

        void DiscardPackets();

        /**
            Discard packets sent to a particular client index.
            This is called when a client disconnects from the server.
         */

        void DiscardClientPackets( int clientIndex );

        /**
            Advance network simulator time.
            You must pump this regularly otherwise the network simulator won't work.
            @param time The current time value. Please make sure you use double values for time so you retain sufficient precision as time increases.
         */

        void AdvanceTime( double time );

        /**
            Get the allocator to use to free packet data.
            @returns The allocator that packet data is allocated with.
         */

        Allocator & GetAllocator() { yojimbo_assert( m_allocator ); return *m_allocator; }

    protected:

        /**
            Helper function to update the active flag whenever network settings are changed.
            Active is set to true if any of the network conditions are non-zero. This allows you to quickly check if the network simulator is active and would actually do something.
         */

        void UpdateActive();

    private:

        Allocator * m_allocator;                        ///< The allocator passed in to the constructor. It's used to allocate and free packet data.
        float m_latency;                                ///< Latency in milliseconds
        float m_jitter;                                 ///< Jitter in milliseconds +/-
        float m_packetLoss;                             ///< Packet loss percentage.
        float m_duplicates;                             ///< Duplicate packet percentage
        bool m_active;                                  ///< True if network simulator is active, eg. if any of the network settings above are enabled.

        /// A packet buffered in the network simulator.

        struct PacketEntry
        {
            PacketEntry()
            {
                to = 0;
                deliveryTime = 0.0;
                packetData = NULL;
                packetBytes = 0;
            }

            int to;                                     ///< To index this packet should be sent to (for server -> client packets).
            double deliveryTime;                        ///< Delivery time for this packet (seconds).
            uint8_t * packetData;                       ///< Packet data (owns this pointer).
            int packetBytes;                            ///< Size of packet in bytes.
        };

        double m_time;                                  ///< Current time from last call to advance time.
        int m_currentIndex;                             ///< Current index in the packet entry array. New packets are inserted here.
        int m_numPacketEntries;                         ///< Number of elements in the packet entry array.
        PacketEntry * m_packetEntries;                  ///< Pointer to dynamically allocated packet entries. This is where buffered packets are stored.
    };

    /** 
        Specifies the message factory and callbacks for clients and servers.
        An instance of this class is passed into the client and server constructors. 
        You can share the same adapter across a client/server pair if you have local multiplayer, eg. loopback.
     */

    class Adapter
    {
    public:

        virtual ~Adapter() {}

        /**
            Override this function to specify your own custom allocator class.
            @param allocator The base allocator that must be used to allocate your allocator instance.
            @param memory The block of memory backing your allocator.
            @param bytes The number of bytes of memory available to your allocator.
            @returns A pointer to the allocator instance you created.
         */

        virtual Allocator * CreateAllocator( Allocator & allocator, void * memory, size_t bytes )
        {
            return YOJIMBO_NEW( allocator, TLSF_Allocator, memory, bytes );
        }

        /**
            You must override this method to create the message factory used by the client and server.
            @param allocator The allocator that must be used to create your message factory instance via YOJIMBO_NEW
            @returns The message factory pointer you created.

         */

        virtual MessageFactory * CreateMessageFactory( Allocator & allocator )
        {
            (void) allocator;
            yojimbo_assert( false );
            return NULL;
        }

        /** 
            Override this callback to process packets sent from client to server over loopback.
            @param clientIndex The client index in range [0,maxClients-1]
            @param packetData The packet data (raw) to be sent to the server.
            @param packetBytes The number of packet bytes in the server.
            @param packetSequence The sequence number of the packet.
            @see Client::ConnectLoopback
         */

        virtual void ClientSendLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
        {
            (void) clientIndex;
            (void) packetData;
            (void) packetBytes;
            (void) packetSequence;
            yojimbo_assert( false );
        }

        /**
            Override this callback to process packets sent from client to server over loopback.
            @param clientIndex The client index in range [0,maxClients-1]
            @param packetData The packet data (raw) to be sent to the server.
            @param packetBytes The number of packet bytes in the server.
            @param packetSequence The sequence number of the packet.
            @see Server::ConnectLoopbackClient
         */

        virtual void ServerSendLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
        {
            (void) clientIndex;
            (void) packetData;
            (void) packetBytes;
            (void) packetSequence;
            yojimbo_assert( false );
        }

        /**
            Override this to get a callback when a client connects on the server.
         */

        virtual void OnServerClientConnected( int clientIndex )
        {
            (void) clientIndex;
        }

        /**
            Override this to get a callback when a client disconnects from the server.
         */

        virtual void OnServerClientDisconnected( int clientIndex )
        {
            (void) clientIndex;
        }
    };

    /**
        Network information for a connection.
        Contains statistics like round trip time (RTT), packet loss %, bandwidth estimates, number of packets sent, received and acked.
     */

    struct NetworkInfo
    {
        float RTT;                                  ///< Round trip time estimate (milliseconds).
        float packetLoss;                           ///< Packet loss percent.
        float sentBandwidth;                        ///< Sent bandwidth (kbps).
        float receivedBandwidth;                    ///< Received bandwidth (kbps).
        float ackedBandwidth;                       ///< Acked bandwidth (kbps).
        uint64_t numPacketsSent;                    ///< Number of packets sent.
        uint64_t numPacketsReceived;                ///< Number of packets received.
        uint64_t numPacketsAcked;                   ///< Number of packets acked.
    };

    /**
        The server interface.
     */

    class ServerInterface
    {
    public:

        virtual ~ServerInterface() {}

        /**
            Set the context for reading and writing packets.
            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.
            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time. 
            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Start the server and allocate client slots.
            Each client that connects to this server occupies one of the client slots allocated by this function.
            @param maxClients The number of client slots to allocate. Must be in range [1,MaxClients]
            @see Server::Stop
         */

        virtual void Start( int maxClients ) = 0;

        /**
            Stop the server and free client slots.
            Any clients that are connected at the time you call stop will be disconnected.
            When the server is stopped, clients cannot connect to the server.
            @see Server::Start.
         */

        virtual void Stop() = 0;

        /**
            Disconnect the client at the specified client index.
            @param clientIndex The index of the client to disconnect in range [0,maxClients-1], where maxClients is the number of client slots allocated in Server::Start.
            @see Server::IsClientConnected
         */

        virtual void DisconnectClient( int clientIndex ) = 0;

        /**
            Disconnect all clients from the server.
            Client slots remain allocated as per the last call to Server::Start, they are simply made available for new clients to connect.
         */

        virtual void DisconnectAllClients() = 0;

        /**
            Send packets to connected clients.
            This function drives the sending of packets that transmit messages to clients.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from connected clients.
            This function drives the procesing of messages included in packets received from connected clients.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance server time.
            Call this at the end of each frame to advance the server time forward. 
            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the server running?
            The server is running after you have called Server::Start. It is not running before the first server start, and after you call Server::Stop.
            Clients can only connect to the server while it is running.
            @returns true if the server is currently running.
         */

        virtual bool IsRunning() const = 0;

        /**
            Get the maximum number of clients that can connect to the server.
            Corresponds to the maxClients parameter passed into the last call to Server::Start.
            @returns The maximum number of clients that can connect to the server. In other words, the number of client slots.
         */

        virtual int GetMaxClients() const = 0;

        /**
            Is a client connected to a client slot?
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns True if the client is connected.
         */

        virtual bool IsClientConnected( int clientIndex ) const = 0;

        /**
            Get the unique id of the client
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns The unique id of the client.
         */

        virtual uint64_t GetClientId( int clientIndex ) const = 0;

        /** 
            Get the number of clients that are currently connected to the server.
            @returns the number of connected clients.
         */

        virtual int GetNumConnectedClients() const = 0;

        /**
            Gets the current server time.
            @see Server::AdvanceTime
         */

        virtual double GetTime() const = 0;

        /**
            Create a message of the specified type for a specific client.
            @param clientIndex The index of the client this message belongs to. Determines which client heap is used to allocate the message.
            @param type The type of the message to create. The message types corresponds to the message factory created by the adaptor set on the server.
         */

        virtual Message * CreateMessage( int clientIndex, int type ) = 0;

        /**
            Helper function to allocate a data block.
            This is typically used to create blocks of data to attach to block messages. See BlockMessage for details.
            @param clientIndex The index of the client this message belongs to. Determines which client heap is used to allocate the data.
            @param bytes The number of bytes to allocate.
            @returns The pointer to the data block. This must be attached to a message via Client::AttachBlockToMessage, or freed via Client::FreeBlock.
         */

        virtual uint8_t * AllocateBlock( int clientIndex, int bytes ) = 0;

        /**
            Attach data block to message.
            @param clientIndex The index of the client this block belongs to.
            @param message The message to attach the block to. This message must be derived from BlockMessage.
            @param block Pointer to the block of data to attach. Must be created via Client::AllocateBlock.
            @param bytes Length of the block of data in bytes.
         */

        virtual void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes ) = 0;

        /**
            Free a block of memory.
            @param clientIndex The index of the client this block belongs to.
            @param block The block of memory created by Client::AllocateBlock.
         */

        virtual void FreeBlock( int clientIndex, uint8_t * block ) = 0;

        /**
            Can we send a message to a particular client on a channel?
            @param clientIndex The index of the client to send a message to.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns True if a message can be sent over the channel, false otherwise.
         */

        virtual bool CanSendMessage( int clientIndex, int channelIndex ) const = 0;

        /**
            Send a message to a client over a channel.
            @param clientIndex The index of the client to send a message to.
            @param channelIndex The channel index in range [0,numChannels-1].
            @param message The message to send.
         */

        virtual void SendMessage( int clientIndex, int channelIndex, Message * message ) = 0;

        /**
            Receive a message from a client over a channel.
            @param clientIndex The index of the client to receive messages from.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns The message received, or NULL if no message is available. Make sure to release this message by calling Server::ReleaseMessage.
         */

        virtual Message * ReceiveMessage( int clientIndex, int channelIndex ) = 0;

        /**
            Release a message.
            Call this for messages received by Server::ReceiveMessage.
            @param clientIndex The index of the client that the message belongs to.
            @param message The message to release.
         */

        virtual void ReleaseMessage( int clientIndex, Message * message ) = 0;

        /**
            Get client network info.
            Call this to receive information about the client network connection, eg. round trip time, packet loss %, # of packets sent and so on.
            @param clientIndex The index of the client.
            @param info The struct to be filled with network info [out].
         */

        virtual void GetNetworkInfo( int clientIndex, NetworkInfo & info ) const = 0;

        /**
            Connect a loopback client.
            This allows you to have local clients connected to a server, for example for integrated server or singleplayer.
            @param clientIndex The index of the client.
            @param clientId The unique client id.
            @param userData User data for this client. Optional. Pass NULL if not needed.
         */

        virtual void ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData ) = 0;

        /**
            Disconnect a loopback client.
            Loopback clients are not disconnected by regular Disconnect or DisconnectAllClient calls. You need to call this function instead.
            @param clientIndex The index of the client to disconnect. Must already be a connected loopback client.
         */

        virtual void DisconnectLoopbackClient( int clientIndex ) = 0;

        /**
            Is this client a loopback client?
            @param clientIndex The client index.
            @returns true if the client is a connected loopback client, false otherwise.
         */

        virtual bool IsLoopbackClient( int clientIndex ) const = 0;

        /**
            Process loopback packet.
            Use this to pass packets from a client directly to the loopback client slot on the server.
            @param clientIndex The client index. Must be an already connected loopback client.
            @param packetData The packet data to process.
            @param packetBytes The number of bytes of packet data.
            @param packetSequence The packet sequence number.
         */

        virtual void ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) = 0;
    };

    /**
        Common functionality across all server implementations.
     */

    class BaseServer : public ServerInterface
    {
    public:

        BaseServer( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time );

        ~BaseServer();

        void SetContext( void * context );

        void Start( int maxClients );

        void Stop();

        void AdvanceTime( double time );

        bool IsRunning() const { return m_running; }

        int GetMaxClients() const { return m_maxClients; }

        double GetTime() const { return m_time; }

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicates( float percent );

        Message * CreateMessage( int clientIndex, int type );

        uint8_t * AllocateBlock( int clientIndex, int bytes );

        void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes );

        void FreeBlock( int clientIndex, uint8_t * block );

        bool CanSendMessage( int clientIndex, int channelIndex ) const;

        bool HasMessagesToSend( int clientIndex, int channelIndex ) const;

        void SendMessage( int clientIndex, int channelIndex, Message * message );

        Message * ReceiveMessage( int clientIndex, int channelIndex );

        void ReleaseMessage( int clientIndex, Message * message );

        void GetNetworkInfo( int clientIndex, NetworkInfo & info ) const;

    protected:

        uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        void * GetContext() { return m_context; }

        Adapter & GetAdapter() { yojimbo_assert( m_adapter ); return *m_adapter; }

        Allocator & GetGlobalAllocator() { yojimbo_assert( m_globalAllocator ); return *m_globalAllocator; }

        MessageFactory & GetClientMessageFactory( int clientIndex );

        NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        reliable_endpoint_t * GetClientEndpoint( int clientIndex );

        Connection & GetClientConnection( int clientIndex );

        virtual void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        virtual int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );
        
        static int StaticProcessPacketFunction( void * context,int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static void * StaticAllocateFunction( void * context, uint64_t bytes );
        
        static void StaticFreeFunction( void * context, void * pointer );

    private:

        ClientServerConfig m_config;                                ///< Base client/server config.
        Allocator * m_allocator;                                    ///< Allocator passed in to constructor.
        Adapter * m_adapter;                                        ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                           ///< Optional serialization context.
        int m_maxClients;                                           ///< Maximum number of clients supported.
        bool m_running;                                             ///< True if server is currently running, eg. after "Start" is called, before "Stop".
        double m_time;                                              ///< Current server time in seconds.
        uint8_t * m_globalMemory;                                   ///< The block of memory backing the global allocator. Allocated with m_allocator.
        uint8_t * m_clientMemory[MaxClients];                       ///< The block of memory backing the per-client allocators. Allocated with m_allocator.
        Allocator * m_globalAllocator;                              ///< The global allocator. Used for allocations that don't belong to a specific client.
        Allocator * m_clientAllocator[MaxClients];                  ///< Array of per-client allocator. These are used for allocations related to connected clients.
        MessageFactory * m_clientMessageFactory[MaxClients];        ///< Array of per-client message factories. This silos message allocations per-client slot.
        Connection * m_clientConnection[MaxClients];                ///< Array of per-client connection classes. This is how messages are exchanged with clients.
        reliable_endpoint_t * m_clientEndpoint[MaxClients];         ///< Array of per-client reliable.io endpoints.
        NetworkSimulator * m_networkSimulator;                      ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional. 
        uint8_t * m_packetBuffer;                                   ///< Buffer used when writing packets.
    };

    /**
        Dedicated server implementation.
     */

    class Server : public BaseServer
    {
    public:

        Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Server();

        void Start( int maxClients );

        void Stop();

        void DisconnectClient( int clientIndex );

        void DisconnectAllClients();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        bool IsClientConnected( int clientIndex ) const;

        uint64_t GetClientId( int clientIndex ) const;

        int GetNumConnectedClients() const;

        void ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData );

        void DisconnectLoopbackClient( int clientIndex );

        bool IsLoopbackClient( int clientIndex ) const;

        void ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        const Address & GetAddress() const { return m_boundAddress; }

    private:

        void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void ConnectDisconnectCallbackFunction( int clientIndex, int connected );

        void SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        static void StaticConnectDisconnectCallbackFunction( void * context, int clientIndex, int connected );

        static void StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        ClientServerConfig m_config;
        netcode_server_t * m_server;
        Address m_address;                                  // original address passed to ctor
        Address m_boundAddress;                             // address after socket bind, eg. valid port
        uint8_t m_privateKey[KeyBytes];
    };

    /**
        The set of client states.
     */

    enum ClientState
    {
        CLIENT_STATE_ERROR = -1,
        CLIENT_STATE_DISCONNECTED = 0,
        CLIENT_STATE_CONNECTING,
        CLIENT_STATE_CONNECTED,
    };

    /** 
        The common interface for all clients.
     */

    class ClientInterface
    {
    public:

        virtual ~ClientInterface() {}

        /**
            Set the context for reading and writing packets.
            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.
            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time. 
            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Disconnect from the server.
         */

        virtual void Disconnect() = 0;

        /**
            Send packets to server.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from the server.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance client time.
            Call this at the end of each frame to advance the client time forward. 
            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the client connecting to a server?
            This is true while the client is negotiation connection with a server.
            @returns true if the client is currently connecting to, but is not yet connected to a server.
         */

        virtual bool IsConnecting() const = 0;

        /**
            Is the client connected to a server?
            This is true once a client successfully finishes connection negotiatio, and connects to a server. It is false while connecting to a server.
            @returns true if the client is connected to a server.
         */

        virtual bool IsConnected() const = 0;

        /**
            Is the client in a disconnected state?
            A disconnected state corresponds to the client being in the disconnected, or in an error state. Both are logically "disconnected".
            @returns true if the client is disconnected.
         */

        virtual bool IsDisconnected() const = 0;

        /**
            Is the client in an error state?
            When the client disconnects because of an error, it enters into this error state.
            @returns true if the client is in an error state.
         */

        virtual bool ConnectionFailed() const = 0;

        /**
            Get the current client state.
         */

        virtual ClientState GetClientState() const = 0;

        /**
            Get the client index.
            The client index is the slot number that the client is occupying on the server. 
            @returns The client index in [0,maxClients-1], where maxClients is the number of client slots allocated on the server in Server::Start.
         */

        virtual int GetClientIndex() const = 0;

        /**
            Get the client id.
            The client id is a unique identifier of this client.
            @returns The client id.
         */

        virtual uint64_t GetClientId() const = 0;

        /**
            Get the current client time.
            @see Client::AdvanceTime
         */

        virtual double GetTime() const = 0;

        /**
            Create a message of the specified type.
            @param type The type of the message to create. The message types corresponds to the message factory created by the adaptor set on this client.
         */

        virtual Message * CreateMessage( int type ) = 0;

        /**
            Helper function to allocate a data block.
            This is typically used to create blocks of data to attach to block messages. See BlockMessage for details.
            @param bytes The number of bytes to allocate.
            @returns The pointer to the data block. This must be attached to a message via Client::AttachBlockToMessage, or freed via Client::FreeBlock.
         */

        virtual uint8_t * AllocateBlock( int bytes ) = 0;

        /**
            Attach data block to message.
            @param message The message to attach the block to. This message must be derived from BlockMessage.
            @param block Pointer to the block of data to attach. Must be created via Client::AllocateBlock.
            @param bytes Length of the block of data in bytes.
         */

        virtual void AttachBlockToMessage( Message * message, uint8_t * block, int bytes ) = 0;

        /**
            Free a block of memory.
            @param block The block of memory created by Client::AllocateBlock.
         */

        virtual void FreeBlock( uint8_t * block ) = 0;

        /**
            Can we send a message on a channel?
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns True if a message can be sent over the channel, false otherwise.
         */

        virtual bool CanSendMessage( int channelIndex ) const = 0;

        /**
            Send a message on a channel.
            @param channelIndex The channel index in range [0,numChannels-1].
            @param message The message to send.
         */

        virtual void SendMessage( int channelIndex, Message * message ) = 0;

        /**
            Receive a message from a channel.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns The message received, or NULL if no message is available. Make sure to release this message by calling Client::ReleaseMessage.
         */

        virtual Message * ReceiveMessage( int channelIndex ) = 0;

        /**
            Release a message.
            Call this for messages received by Client::ReceiveMessage.
            @param message The message to release.
         */

        virtual void ReleaseMessage( Message * message ) = 0;

        /**
            Get client network info.
            Call this to receive information about the client network connection to the server, eg. round trip time, packet loss %, # of packets sent and so on.
            @param info The struct to be filled with network info [out].
         */

        virtual void GetNetworkInfo( NetworkInfo & info ) const = 0;

        /**
            Connect to server over loopback.
            This allows you to have local clients connected to a server, for example for integrated server or singleplayer.
            @param clientIndex The index of the client.
            @param clientId The unique client id.
            @param maxClients The maximum number of clients supported by the server.
         */

        virtual void ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients ) = 0;

        /**
            Disconnect from server over loopback.
         */

        virtual void DisconnectLoopback() = 0;

        /**
            Is this a loopback client?
            @returns true if the client is a loopback client, false otherwise.
         */

        virtual bool IsLoopback() const = 0;

        /**
            Process loopback packet.
            Use this to pass packets from a server directly to the loopback client.
            @param packetData The packet data to process.
            @param packetBytes The number of bytes of packet data.
            @param packetSequence The packet sequence number.
         */

        virtual void ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) = 0;
    };

    /**
        Functionality that is common across all client implementations.
     */

    class BaseClient : public ClientInterface
    {
    public:

        /**
            Base client constructor.
            @param allocator The allocator for all memory used by the client.
            @param config The base client/server configuration.
            @param time The current time in seconds. See ClientInterface::AdvanceTime
            @param allocator The adapter to the game program. Specifies allocators, message factory to use etc.
         */

        explicit BaseClient( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time );

        ~BaseClient();

        void SetContext( void * context ) { yojimbo_assert( IsDisconnected() ); m_context = context; }

        void Disconnect();

        void AdvanceTime( double time );

        bool IsConnecting() const { return m_clientState == CLIENT_STATE_CONNECTING; }

        bool IsConnected() const { return m_clientState == CLIENT_STATE_CONNECTED; }

        bool IsDisconnected() const { return m_clientState <= CLIENT_STATE_DISCONNECTED; }

        bool ConnectionFailed() const { return m_clientState == CLIENT_STATE_ERROR; }

        ClientState GetClientState() const { return m_clientState; }

        int GetClientIndex() const { return m_clientIndex; }

        double GetTime() const { return m_time; }

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicates( float percent );

        Message * CreateMessage( int type );

        uint8_t * AllocateBlock( int bytes );

        void AttachBlockToMessage( Message * message, uint8_t * block, int bytes );

        void FreeBlock( uint8_t * block );

        bool CanSendMessage( int channelIndex ) const;

        bool HasMessagesToSend( int channelIndex ) const;

        void SendMessage( int channelIndex, Message * message );

        Message * ReceiveMessage( int channelIndex );

        void ReleaseMessage( Message * message );

        void GetNetworkInfo( NetworkInfo & info ) const;

    protected:

        uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        void * GetContext() { return m_context; }

        Adapter & GetAdapter() { yojimbo_assert( m_adapter ); return *m_adapter; }

        void CreateInternal();

        void DestroyInternal();

        void SetClientState( ClientState clientState );

        Allocator & GetClientAllocator() { yojimbo_assert( m_clientAllocator ); return *m_clientAllocator; }

        MessageFactory & GetMessageFactory() { yojimbo_assert( m_messageFactory ); return *m_messageFactory; }

        NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        reliable_endpoint_t * GetEndpoint() { return m_endpoint; }

        Connection & GetConnection() { yojimbo_assert( m_connection ); return *m_connection; }

        virtual void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        virtual int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );
        
        static int StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static void * StaticAllocateFunction( void * context, uint64_t bytes );
        
        static void StaticFreeFunction( void * context, void * pointer );

    private:

        ClientServerConfig m_config;                                        ///< The client/server configuration.
        Allocator * m_allocator;                                            ///< The allocator passed to the client on creation.
        Adapter * m_adapter;                                                ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                                   ///< Context lets the user pass information to packet serialize functions.
        uint8_t * m_clientMemory;                                           ///< The memory backing the client allocator. Allocated from m_allocator.
        Allocator * m_clientAllocator;                                      ///< The client allocator. Everything allocated between connect and disconnected is allocated and freed via this allocator.
        reliable_endpoint_t * m_endpoint;                                   ///< reliable.io endpoint.
        MessageFactory * m_messageFactory;                                  ///< The client message factory. Created and destroyed on each connection attempt.
        Connection * m_connection;                                          ///< The client connection for exchanging messages with the server.
        NetworkSimulator * m_networkSimulator;                              ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional. 
        ClientState m_clientState;                                          ///< The current client state. See ClientInterface::GetClientState
        int m_clientIndex;                                                  ///< The client slot index on the server [0,maxClients-1]. -1 if not connected.
        double m_time;                                                      ///< The current client time. See ClientInterface::AdvanceTime
        uint8_t * m_packetBuffer;                                           ///< Buffer used to read and write packets.

    private:

        BaseClient( const BaseClient & other );
        
        const BaseClient & operator = ( const BaseClient & other );
    };

    /**
        Implementation of client for dedicated servers.
     */

    class Client : public BaseClient
    {
    public:

        /**
            The client constructor.
            @param allocator The allocator for all memory used by the client.
            @param address The address the client should bind to.
            @param config The client/server configuration.
            @param time The current time in seconds. See ClientInterface::AdvanceTime
         */

        explicit Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Client();

        void InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address & address );

        void InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses );

        void Connect( uint64_t clientId, uint8_t * connectToken );

        void Disconnect();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        int GetClientIndex() const;

        uint64_t GetClientId() const { return m_clientId; }

        void ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients );

        void DisconnectLoopback();

        bool IsLoopback() const;

        void ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        const Address & GetAddress() const { return m_boundAddress; }

    private:

        bool GenerateInsecureConnectToken( uint8_t * connectToken, 
                                           const uint8_t privateKey[], 
                                           uint64_t clientId, 
                                           const Address serverAddresses[], 
                                           int numServerAddresses );

        void CreateClient( const Address & address );

        void DestroyClient();

        void StateChangeCallbackFunction( int previous, int current );

        static void StaticStateChangeCallbackFunction( void * context, int previous, int current );

        void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        static void StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        ClientServerConfig m_config;                    ///< Client/server configuration.
        netcode_client_t * m_client;                    ///< netcode.io client data.
        Address m_address;                              ///< Original address passed to ctor.
        Address m_boundAddress;                         ///< Address after socket bind, eg. with valid port
        uint64_t m_clientId;                            ///< The globally unique client id (set on each call to connect)
    };

    /**
        Matcher status enum.
        Designed for when the matcher will be made non-blocking. The matcher is currently blocking in Matcher::RequestMatch
     */

    enum MatchStatus
    {
        MATCH_IDLE,                 ///< The matcher is idle.
        MATCH_BUSY,                 ///< The matcher is requesting a match.
        MATCH_READY,                ///< The match response is ready to read with Matcher::GetConnectToken.
        MATCH_FAILED                ///< The matcher failed to find a match.
    };

    /**
        Communicates with the matcher web service over HTTPS.
        See docker/matcher/matcher.go for details. Launch the matcher via "premake5 matcher".
        This class will be improved in the future, most importantly to make Matcher::RequestMatch a non-blocking operation.
     */

    class Matcher
    {
    public:

        /**
            Matcher constructor.
            @param allocator The allocator to use for allocations.
         */

        explicit Matcher( Allocator & allocator );
       
        /**
            Matcher destructor.
         */

        ~Matcher();

        /**
            Initialize the matcher. 
            @returns True if the matcher initialized successfully, false otherwise.
         */

        bool Initialize();

        /** 
            Request a match.
            This is how clients get connect tokens from matcher.go. 
            They request a match and the server replies with a set of servers to connect to, and a connect token to pass to that server.
            IMPORTANT: This function is currently blocking. It will be made non-blocking in the near future.
            @param protocolId The protocol id that we are using. Used to filter out servers with different protocol versions.
            @param clientId A unique client identifier that identifies each client to your back end services. If you don't have this yet, just roll a random 64 bit number.
            @see Matcher::GetMatchStatus
            @see Matcher::GetConnectToken
         */

        void RequestMatch( uint64_t protocolId, uint64_t clientId, bool verifyCertificate );

        /**
            Get the current match status.
            Because Matcher::RequestMatch is currently blocking this will be MATCH_READY or MATCH_FAILED immediately after that function returns.
            If the status is MATCH_READY you can call Matcher::GetMatchResponse to get the match response data corresponding to the last call to Matcher::RequestMatch.
            @returns The current match status.
         */

        MatchStatus GetMatchStatus();

        /**
            Get connect token.
            This can only be called if the match status is MATCH_READY.
            @param connectToken The connect token data to fill [out].
            @see Matcher::RequestMatch
            @see Matcher::GetMatchStatus
         */

        void GetConnectToken( uint8_t * connectToken );

    private:

        Matcher( const Matcher & matcher );

        const Matcher & operator = ( const Matcher & other );

        Allocator * m_allocator;                                ///< The allocator passed into the constructor.
        bool m_initialized;                                     ///< True if the matcher was successfully initialized. See Matcher::Initialize.
        MatchStatus m_matchStatus;                              ///< The current match status.
#if YOJIMBO_WITH_MBEDTLS
		struct MatcherInternal * m_internal;                    ///< Internals are in here to avoid spilling details of mbedtls library outside of yojimbo_matcher.cpp
        uint8_t m_connectToken[ConnectTokenBytes];              ///< The connect token data from the last call to Matcher::RequestMatch once the match status is MATCH_READY.
#endif // #if YOJIMBO_WITH_MBEDTLS
    };
}

#endif // #ifndef YOJIMBO_H
