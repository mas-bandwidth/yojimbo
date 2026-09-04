/*
    reliable

    Copyright © 2017 - 2026, Más Bandwidth LLC

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

/*
    If you use this library in a product, please credit
    "reliable - Glenn Fiedler and Rowan Claude" in your product credits. The
    license doesn't require this credit. It's an official request, and honoring
    it is appreciated.
*/

#ifndef RELIABLE_H
#define RELIABLE_H

#define RELIABLE_VERSION_FULL    "1.4.2"
#define RELIABLE_VERSION_MAJOR   1
#define RELIABLE_VERSION_MINOR   4
#define RELIABLE_VERSION_PATCH   2

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#if !defined(RELIABLE_DEBUG) && !defined(RELIABLE_RELEASE)
#if defined(NDEBUG)
#define RELIABLE_RELEASE
#else
#define RELIABLE_DEBUG
#endif
#elif defined(RELIABLE_DEBUG) && defined(RELIABLE_RELEASE)
#error Can only define one of debug & release
#endif

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define RELIABLE_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define RELIABLE_BIG_ENDIAN 1
#elif  defined(__386__) || defined(i386)    || defined(__i386__)  \
    || defined(__X86)   || defined(_M_IX86)                       \
    || defined(_M_X64)  || defined(__x86_64__)                    \
    || defined(alpha)   || defined(__alpha) || defined(__alpha__) \
    || defined(_M_ALPHA)                                          \
    || defined(ARM)     || defined(_ARM)    || defined(__arm__)   \
    || defined(__aarch64__) 								      \
    || defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) \
    || defined(_WIN32_WCE) || defined(__NT__)                     \
    || defined(__MIPSEL__)
  #define RELIABLE_LITTLE_ENDIAN 1
#else
  #define RELIABLE_BIG_ENDIAN 1
#endif

#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT                          0
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED                      1
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED                         2
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_STALE                         3
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_INVALID                       4
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_SEND             5
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE          6
#define RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_SENT                        7
#define RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED                    8
#define RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID                     9
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_DUPLICATE                     10
#define RELIABLE_ENDPOINT_NUM_COUNTERS                                      11

#define RELIABLE_MAX_PACKET_HEADER_BYTES 9
#define RELIABLE_FRAGMENT_HEADER_BYTES   5

#define RELIABLE_LOG_LEVEL_NONE     0
#define RELIABLE_LOG_LEVEL_ERROR    1
#define RELIABLE_LOG_LEVEL_INFO     2
#define RELIABLE_LOG_LEVEL_DEBUG    3

#define RELIABLE_OK         1
#define RELIABLE_ERROR      0

// the public surface. every function below carries RELIABLE_EXPORT and nothing else in the
// library does, so a shared build exports exactly this header and a static build is unchanged.
// the build system defines RELIABLE_SHARED for both the library and its consumers when
// BUILD_SHARED_LIBS is on, and RELIABLE_BUILDING for the library itself.

#if defined(RELIABLE_SHARED)
    #if defined(_WIN32) || defined(__CYGWIN__)
        #if defined(RELIABLE_BUILDING)
            #define RELIABLE_EXPORT __declspec(dllexport)
        #else
            #define RELIABLE_EXPORT __declspec(dllimport)
        #endif
    #elif defined(__GNUC__)
        #define RELIABLE_EXPORT __attribute__((visibility("default")))
    #else
        #define RELIABLE_EXPORT
    #endif
#else
    #define RELIABLE_EXPORT
#endif

#ifdef __cplusplus
#define RELIABLE_CONST const
extern "C" {
#else
#if defined(__STDC__)
#define RELIABLE_CONST const
#else
#define RELIABLE_CONST
#endif
#endif

// initialize the library. call this once before creating any endpoints. returns RELIABLE_OK on success

RELIABLE_EXPORT int reliable_init(void);

// shut down the library. call this once after all endpoints are destroyed

RELIABLE_EXPORT void reliable_term(void);

struct reliable_config_t
{
    char name[256];                                                             // name of the endpoint. used in log output
    void * context;                                                             // passed to the transmit and process packet callbacks
    uint64_t id;                                                                // id of the endpoint. passed to callbacks so shared callbacks can tell endpoints apart
    int max_packet_size;                                                        // maximum packet size that can be sent or received (bytes)
    int fragment_above;                                                         // packets larger than this many bytes are sent as fragments
    int max_fragments;                                                          // maximum number of fragments per-packet. 256 max. must cover max_packet_size / fragment_size
    int fragment_size;                                                          // size of each fragment (bytes)
    int ack_buffer_size;                                                        // maximum number of acks buffered between calls to reliable_endpoint_clear_acks
    int sent_packets_buffer_size;                                               // number of sent packets tracked for acks, packet loss and bandwidth stats
    int received_packets_buffer_size;                                           // number of received packets tracked. also the window for stale and duplicate packet rejection
    int fragment_reassembly_buffer_size;                                        // number of packets that can be under reassembly from fragments at the same time
    float rtt_smoothing_factor;                                                 // exponential smoothing factor for the rtt moving average
    int rtt_history_size;                                                       // number of rtt samples kept for min/max/avg rtt and jitter
    float packet_loss_smoothing_factor;                                         // exponential smoothing factor for packet loss
    float bandwidth_smoothing_factor;                                           // exponential smoothing factor for bandwidth
    int packet_header_size;                                                     // assumed network header overhead per-packet, used only for bandwidth stats. 28 = IPv4 + UDP
    void (*transmit_packet_function)(void*,uint64_t,uint16_t,uint8_t*,int);     // called to send a packet: (context, id, sequence, packet_data, packet_bytes). must not send packets on the same endpoint
    int (*process_packet_function)(void*,uint64_t,uint16_t,uint8_t*,int);       // called when a packet is received: (context, id, sequence, packet_data, packet_bytes). return 1 to accept and ack the packet, 0 to reject it (rejected packets are not acked and may be processed again if they arrive again)
    void * allocator_context;                                                   // passed to the allocate and free functions
    void * (*allocate_function)(void*,size_t);                                  // custom allocator. NULL = malloc
    void (*free_function)(void*,void*);                                         // custom free. NULL = free
};

// pointer lifetimes across the callbacks and the config:
//
//   context, allocator_context      owned by you. the endpoint stores them and hands them
//                                   back unchanged, so they must outlive the endpoint. the
//                                   library never dereferences either one.
//
//   transmit_packet_function
//     packet_data                   points into the endpoint's own transmit scratch buffer
//                                   and is valid only for the duration of the call. the next
//                                   send on that endpoint overwrites it, which is why the
//                                   callback must not send on the same endpoint. copy the
//                                   bytes if you need them after you return.
//
//   process_packet_function
//     packet_data                   valid only for the duration of the call. for an
//                                   unfragmented packet it points into the buffer you passed
//                                   to reliable_endpoint_receive_packet, past the header. for
//                                   a reassembled packet it points into an internal buffer
//                                   that the endpoint may free as soon as you return. copy the
//                                   bytes if you need them after you return. it is yours to
//                                   read, not to free: reliable_endpoint_free_packet is for
//                                   memory you allocated with the endpoint's allocator.
//
//   allocate_function, free_function
//                                   called for every allocation the endpoint makes, including
//                                   during reliable_endpoint_create. returning NULL from
//                                   allocate_function is a supported outcome: create frees
//                                   whatever it had allocated and returns NULL.

// fills a config with sensible defaults for a client/server game exchanging packets at 60HZ

RELIABLE_EXPORT void reliable_default_config( struct reliable_config_t * config );

// creates an endpoint. one endpoint per connection: a client has one, a server has one per client slot.
// endpoints are not thread safe: use one endpoint per-thread or protect each endpoint with your own lock.
// (the log level, printf and assert handlers are global to the process.)
//
// returns NULL in every build, debug and release, if the config is not valid or if any
// allocation fails. a refused config is one whose fields are out of range or whose
// relationships do not hold: fragment_above must not exceed max_packet_size, and
// max_fragments * fragment_size must cover max_packet_size. on failure nothing is retained:
// every allocation already made is handed back to free_function before create returns.
// check the result.

RELIABLE_EXPORT struct reliable_endpoint_t * reliable_endpoint_create( struct reliable_config_t * config, double time );

// returns the sequence number the next sent packet will have. use it to map acked sequence numbers back to the contents of packets you sent

RELIABLE_EXPORT uint16_t reliable_endpoint_next_packet_sequence( struct reliable_endpoint_t * endpoint );

// sends a packet. the packet is handed to the transmit packet callback, split into fragments first if larger than config.fragment_above

RELIABLE_EXPORT void reliable_endpoint_send_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes );

// call this for each packet received from your socket. valid packets are passed to the process packet callback. stale and duplicate packets are dropped

RELIABLE_EXPORT void reliable_endpoint_receive_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes );

// frees a packet using the endpoint's allocator

RELIABLE_EXPORT void reliable_endpoint_free_packet( struct reliable_endpoint_t * endpoint, void * packet );

// returns a read only view of the sequence numbers of sent packets acked since the last clear.
// the array belongs to the endpoint. it stays valid until the next call to
// reliable_endpoint_receive_packet, reliable_endpoint_clear_acks, reliable_endpoint_reset or
// reliable_endpoint_destroy on that endpoint. copy what you need before calling any of those

RELIABLE_EXPORT RELIABLE_CONST uint16_t * reliable_endpoint_get_acks( struct reliable_endpoint_t * endpoint, int * num_acks );

// clears the ack array. call this once per-frame after processing acks. if you don't, the ack buffer fills up and new acks are dropped

RELIABLE_EXPORT void reliable_endpoint_clear_acks( struct reliable_endpoint_t * endpoint );

// resets the endpoint to its initial state: acks, counters, sequence number, all tracking
// buffers, the rtt history and every rtt, jitter, packet loss and bandwidth statistic are
// cleared. the config and the allocator are kept, so the endpoint is immediately usable again

RELIABLE_EXPORT void reliable_endpoint_reset( struct reliable_endpoint_t * endpoint );

// updates rtt, jitter, packet loss and bandwidth stats. call once per-frame with the current time

RELIABLE_EXPORT void reliable_endpoint_update( struct reliable_endpoint_t * endpoint, double time );

// rtt and jitter are in milliseconds, packet loss is a percentage, bandwidth is in kilobits per-second

RELIABLE_EXPORT float reliable_endpoint_rtt( struct reliable_endpoint_t * endpoint );       // exponentially smoothed moving average

RELIABLE_EXPORT float reliable_endpoint_rtt_min( struct reliable_endpoint_t * endpoint );

RELIABLE_EXPORT float reliable_endpoint_rtt_max( struct reliable_endpoint_t * endpoint );

RELIABLE_EXPORT float reliable_endpoint_rtt_avg( struct reliable_endpoint_t * endpoint );

RELIABLE_EXPORT float reliable_endpoint_jitter_avg_vs_min_rtt( struct reliable_endpoint_t * endpoint );

RELIABLE_EXPORT float reliable_endpoint_jitter_max_vs_min_rtt( struct reliable_endpoint_t * endpoint );

RELIABLE_EXPORT float reliable_endpoint_jitter_stddev_vs_avg_rtt( struct reliable_endpoint_t * endpoint );

RELIABLE_EXPORT float reliable_endpoint_packet_loss( struct reliable_endpoint_t * endpoint );

RELIABLE_EXPORT void reliable_endpoint_bandwidth( struct reliable_endpoint_t * endpoint, float * sent_bandwidth_kbps, float * received_bandwidth_kbps, float * acked_bandwidth_kpbs );

// returns a read only view of the RELIABLE_ENDPOINT_NUM_COUNTERS counters, indexed with
// RELIABLE_ENDPOINT_COUNTER_*. the array belongs to the endpoint and stays valid until it is
// reset or destroyed

RELIABLE_EXPORT RELIABLE_CONST uint64_t * reliable_endpoint_counters( struct reliable_endpoint_t * endpoint );

// destroys an endpoint, freeing everything it allocated

RELIABLE_EXPORT void reliable_endpoint_destroy( struct reliable_endpoint_t * endpoint );

// sets the log level (process-wide). RELIABLE_LOG_LEVEL_NONE by default

RELIABLE_EXPORT void reliable_log_level( int level );

// overrides where log output goes (process-wide). default is printf

RELIABLE_EXPORT void reliable_set_printf_function( int (*function)( RELIABLE_CONST char *, ... ) );

RELIABLE_EXPORT extern void (*reliable_assert_function)( RELIABLE_CONST char *, RELIABLE_CONST char *, RELIABLE_CONST char * file, int line );

#ifdef RELIABLE_DEBUG
#define reliable_assert( condition )                                                        \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
        reliable_assert_function( #condition, __FUNCTION__, __FILE__, __LINE__ );           \
        exit(1);                                                                            \
    }                                                                                       \
} while(0)
#else
#define reliable_assert( ignore ) ((void)0)
#endif

RELIABLE_EXPORT void reliable_set_assert_function( void (*function)( RELIABLE_CONST char * /*condition*/, 
                                   RELIABLE_CONST char * /*function*/, 
                                   RELIABLE_CONST char * /*file*/, 
                                   int /*line*/ ) );

RELIABLE_EXPORT void reliable_copy_string( char * dest, RELIABLE_CONST char * source, size_t dest_size );

#ifdef __cplusplus
}
#endif

#endif // #ifndef RELIABLE_H
