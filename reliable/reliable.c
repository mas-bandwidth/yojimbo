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

#include "reliable.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <float.h>
#include <limits.h>
#include <math.h>

#ifndef RELIABLE_ENABLE_TESTS
#define RELIABLE_ENABLE_TESTS 0
#endif // #ifndef RELIABLE_ENABLE_TESTS

#ifndef RELIABLE_ENABLE_LOGGING
#define RELIABLE_ENABLE_LOGGING 1
#endif // #ifndef RELIABLE_ENABLE_LOGGING

// ------------------------------------------------------------------

static void default_assert_handler( RELIABLE_CONST char * condition, RELIABLE_CONST char * function, RELIABLE_CONST char * file, int line )
{
    printf( "assert failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
    fflush( stdout );
    #if defined( __GNUC__ )
    __builtin_trap();
    #elif defined( _MSC_VER )
    __debugbreak();
    #endif
    exit( 1 );
}

static int log_level = 0;
static int (*printf_function)( RELIABLE_CONST char *, ... ) = ( int (*)( RELIABLE_CONST char *, ... ) ) printf;
void (*reliable_assert_function)( RELIABLE_CONST char *, RELIABLE_CONST char *, RELIABLE_CONST char * file, int line ) = default_assert_handler;

void reliable_log_level( int level )
{
    log_level = level;
}

void reliable_set_printf_function( int (*function)( RELIABLE_CONST char *, ... ) )
{
    reliable_assert( function );
    printf_function = function;
}

void reliable_set_assert_function( void (*function)( RELIABLE_CONST char *, RELIABLE_CONST char *, RELIABLE_CONST char * file, int line ) )
{
    reliable_assert_function = function;
}

#if RELIABLE_ENABLE_LOGGING

static void reliable_printf( int level, RELIABLE_CONST char * format, ... ) 
{
    if ( level > log_level )
        return;
    va_list args;
    va_start( args, format );
    char buffer[4*1024];
    vsnprintf( buffer, sizeof(buffer), format, args );
    printf_function( "%s", buffer );
    va_end( args );
}

#else // #if RELIABLE_ENABLE_LOGGING

static void reliable_printf( int level, RELIABLE_CONST char * format, ... ) 
{
    (void) level;
    (void) format;
}

#endif // #if RELIABLE_ENABLE_LOGGING

static void * reliable_default_allocate_function( void * context, size_t bytes )
{
    (void) context;
    return malloc( bytes );
}

static void reliable_default_free_function( void * context, void * pointer )
{
    (void) context;
    free( pointer );
}

// multiplies an element count by an element size for an allocation. returns 0 if the count is
// negative or the product does not fit in a size_t, so a config that would wrap the arithmetic
// is refused instead of allocating a buffer smaller than the code goes on to use

static int reliable_checked_size( int count, size_t element_bytes, size_t * result )
{
    reliable_assert( result );
    if ( count <= 0 || element_bytes == 0 )
    {
        return 0;
    }
    if ( (size_t) count > SIZE_MAX / element_bytes )
    {
        return 0;
    }
    *result = ( (size_t) count ) * element_bytes;
    return 1;
}

// ------------------------------------------------------------------

int reliable_init(void)
{
    return RELIABLE_OK;
}

void reliable_term(void)
{
}

// ---------------------------------------------------------------

static int reliable_sequence_greater_than( uint16_t s1, uint16_t s2 )
{
    return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
           ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
}

static int reliable_sequence_less_than( uint16_t s1, uint16_t s2 )
{
    return reliable_sequence_greater_than( s2, s1 );
}

// ---------------------------------------------------------------

struct reliable_sequence_buffer_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    uint16_t sequence;
    int num_entries;
    int entry_stride;
    uint32_t * entry_sequence;
    uint8_t * entry_data;
};

static void reliable_sequence_buffer_destroy( struct reliable_sequence_buffer_t * sequence_buffer );

static struct reliable_sequence_buffer_t * reliable_sequence_buffer_create( int num_entries, 
                                                                     int entry_stride, 
                                                                     void * allocator_context, 
                                                                     void * (*allocate_function)(void*,size_t), 
                                                                     void (*free_function)(void*,void*) )
{
    reliable_assert( num_entries > 0 );
    reliable_assert( entry_stride > 0 );

    if ( allocate_function == NULL )
    {
        allocate_function = reliable_default_allocate_function;
    }

    if ( free_function == NULL )
    {
        free_function = reliable_default_free_function;
    }

    size_t entry_sequence_bytes;
    size_t entry_data_bytes;

    if ( !reliable_checked_size( num_entries, sizeof( uint32_t ), &entry_sequence_bytes ) )
    {
        return NULL;
    }

    if ( !reliable_checked_size( num_entries, (size_t) entry_stride, &entry_data_bytes ) )
    {
        return NULL;
    }

    struct reliable_sequence_buffer_t * sequence_buffer = (struct reliable_sequence_buffer_t*)
        allocate_function( allocator_context, sizeof( struct reliable_sequence_buffer_t ) );

    if ( sequence_buffer == NULL )
    {
        return NULL;
    }

    memset( sequence_buffer, 0, sizeof( struct reliable_sequence_buffer_t ) );

    sequence_buffer->allocator_context = allocator_context;
    sequence_buffer->allocate_function = allocate_function;
    sequence_buffer->free_function = free_function;
    sequence_buffer->sequence = 0;
    sequence_buffer->num_entries = num_entries;
    sequence_buffer->entry_stride = entry_stride;
    sequence_buffer->entry_sequence = (uint32_t*) allocate_function( allocator_context, entry_sequence_bytes );
    sequence_buffer->entry_data = (uint8_t*) allocate_function( allocator_context, entry_data_bytes );

    if ( sequence_buffer->entry_sequence == NULL || sequence_buffer->entry_data == NULL )
    {
        reliable_sequence_buffer_destroy( sequence_buffer );
        return NULL;
    }

    memset( sequence_buffer->entry_sequence, 0xFF, entry_sequence_bytes );
    memset( sequence_buffer->entry_data, 0, entry_data_bytes );

    return sequence_buffer;
}

static void reliable_sequence_buffer_destroy( struct reliable_sequence_buffer_t * sequence_buffer )
{
    reliable_assert( sequence_buffer );
    if ( sequence_buffer->entry_sequence )
    {
        sequence_buffer->free_function( sequence_buffer->allocator_context, sequence_buffer->entry_sequence );
    }
    if ( sequence_buffer->entry_data )
    {
        sequence_buffer->free_function( sequence_buffer->allocator_context, sequence_buffer->entry_data );
    }
    sequence_buffer->free_function( sequence_buffer->allocator_context, sequence_buffer );
}

static void reliable_sequence_buffer_reset( struct reliable_sequence_buffer_t * sequence_buffer )
{
    reliable_assert( sequence_buffer );
    sequence_buffer->sequence = 0;
    memset( sequence_buffer->entry_sequence, 0xFF, sizeof( uint32_t) * sequence_buffer->num_entries );
}

static void reliable_sequence_buffer_remove_entries( struct reliable_sequence_buffer_t * sequence_buffer, 
                                              int start_sequence, 
                                              int finish_sequence, 
                                              void (*cleanup_function)(void*,void*,void(*free_function)(void*,void*)) )
{
    reliable_assert( sequence_buffer );
    if ( finish_sequence < start_sequence ) 
    {
        finish_sequence += 65536;
    }
    if ( finish_sequence - start_sequence < sequence_buffer->num_entries )
    {
        int sequence;
        for ( sequence = start_sequence; sequence <= finish_sequence; ++sequence )
        {
            if ( cleanup_function )
            {
                cleanup_function( sequence_buffer->entry_data + sequence_buffer->entry_stride * ( sequence % sequence_buffer->num_entries ), 
                                  sequence_buffer->allocator_context, 
                                  sequence_buffer->free_function );
            }
            sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] = 0xFFFFFFFF;
        }
    }
    else
    {
        int i;
        for ( i = 0; i < sequence_buffer->num_entries; ++i )
        {
            if ( cleanup_function )
            {
                cleanup_function( sequence_buffer->entry_data + sequence_buffer->entry_stride * i, 
                                  sequence_buffer->allocator_context, 
                                  sequence_buffer->free_function );
            }
            sequence_buffer->entry_sequence[i] = 0xFFFFFFFF;
        }
    }
}

static int reliable_sequence_buffer_test_insert( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    return reliable_sequence_less_than( sequence, sequence_buffer->sequence - ((uint16_t)sequence_buffer->num_entries) ) ? ((uint16_t)0) : ((uint16_t)1);
}

static void * reliable_sequence_buffer_insert( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    if ( reliable_sequence_less_than( sequence, sequence_buffer->sequence - ((uint16_t)sequence_buffer->num_entries) ) )
    {
        return NULL;
    }
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence, NULL );
        sequence_buffer->sequence = sequence + 1;
    }
    int index = sequence % sequence_buffer->num_entries;
    sequence_buffer->entry_sequence[index] = sequence;
    return sequence_buffer->entry_data + index * sequence_buffer->entry_stride;
}

static void reliable_sequence_buffer_advance( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence, NULL );
        sequence_buffer->sequence = sequence + 1;
    }
}

static void * reliable_sequence_buffer_insert_with_cleanup( struct reliable_sequence_buffer_t * sequence_buffer, 
                                                     uint16_t sequence, 
                                                     void (*cleanup_function)(void*,void*,void(*free_function)(void*,void*)) )
{
    reliable_assert( sequence_buffer );
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence, cleanup_function );
        sequence_buffer->sequence = sequence + 1;
    }
    else if ( reliable_sequence_less_than( sequence, sequence_buffer->sequence - ((uint16_t)sequence_buffer->num_entries) ) )
    {
        return NULL;
    }
    int index = sequence % sequence_buffer->num_entries;
    if ( sequence_buffer->entry_sequence[index] != 0xFFFFFFFF )
    {
        cleanup_function( sequence_buffer->entry_data + sequence_buffer->entry_stride * ( sequence % sequence_buffer->num_entries ), 
                          sequence_buffer->allocator_context, 
                          sequence_buffer->free_function );
    }
    sequence_buffer->entry_sequence[index] = sequence;
    return sequence_buffer->entry_data + index * sequence_buffer->entry_stride;
}

static void reliable_sequence_buffer_advance_with_cleanup( struct reliable_sequence_buffer_t * sequence_buffer,
                                                    uint16_t sequence,
                                                    void (*cleanup_function)(void*,void*,void(*free_function)(void*,void*)) )
{
    reliable_assert( sequence_buffer );
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence, cleanup_function );
        sequence_buffer->sequence = sequence + 1;
    }
}

static void reliable_sequence_buffer_remove_with_cleanup( struct reliable_sequence_buffer_t * sequence_buffer, 
                                                   uint16_t sequence, 
                                                   void (*cleanup_function)(void*,void*,void(*free_function)(void*,void*)) )
{
    reliable_assert( sequence_buffer );
    int index = sequence % sequence_buffer->num_entries;
    if ( sequence_buffer->entry_sequence[index] != 0xFFFFFFFF )
    {
        sequence_buffer->entry_sequence[index] = 0xFFFFFFFF;
        cleanup_function( sequence_buffer->entry_data + sequence_buffer->entry_stride * index, sequence_buffer->allocator_context, sequence_buffer->free_function );
    }
}

static int reliable_sequence_buffer_exists( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    return sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] == (uint32_t) sequence;
}

static void * reliable_sequence_buffer_find( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    int index = sequence % sequence_buffer->num_entries;
    return ( ( sequence_buffer->entry_sequence[index] == (uint32_t) sequence ) ) ? ( sequence_buffer->entry_data + index * sequence_buffer->entry_stride ) : NULL;

}

static void * reliable_sequence_buffer_at_index( struct reliable_sequence_buffer_t * sequence_buffer, int index )
{
    reliable_assert( sequence_buffer );
    reliable_assert( index >= 0 );
    reliable_assert( index < sequence_buffer->num_entries );
    return sequence_buffer->entry_sequence[index] != 0xFFFFFFFF ? ( sequence_buffer->entry_data + index * sequence_buffer->entry_stride ) : NULL;
}

static void reliable_sequence_buffer_generate_ack_bits( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t * ack, uint32_t * ack_bits )
{
    reliable_assert( sequence_buffer );
    reliable_assert( ack );
    reliable_assert( ack_bits );
    *ack = sequence_buffer->sequence - 1;
    *ack_bits = 0;
    uint32_t mask = 1;
    int i;
    for ( i = 0; i < 32; ++i )
    {
        uint16_t sequence = *ack - ((uint16_t)i);
        if ( reliable_sequence_buffer_exists( sequence_buffer, sequence ) )
            *ack_bits |= mask;
        mask <<= 1;
    }
}

// ---------------------------------------------------------------

static void reliable_write_uint8( uint8_t ** p, uint8_t value )
{
    **p = value;
    ++(*p);
}

static void reliable_write_uint16( uint8_t ** p, uint16_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = value >> 8;
    *p += 2;
}

static uint8_t reliable_read_uint8( uint8_t ** p )
{
    uint8_t value = **p;
    ++(*p);
    return value;
}

static uint16_t reliable_read_uint16( uint8_t ** p )
{
    uint16_t value;
    value = (*p)[0];
    value |= ( ( (uint16_t)( (*p)[1] ) ) << 8 );
    *p += 2;
    return value;
}

// ---------------------------------------------------------------

struct reliable_fragment_reassembly_data_t
{
    uint16_t sequence;
    uint16_t ack;
    uint32_t ack_bits;
    int num_fragments_received;
    int num_fragments_total;
    uint8_t * packet_data;
    int packet_bytes;
    int packet_header_bytes;
    uint8_t fragment_received[256];
};

static void reliable_fragment_reassembly_data_cleanup( void * data, void * allocator_context, void (*free_function)(void*,void*) )

{
    reliable_assert( free_function );
    struct reliable_fragment_reassembly_data_t * reassembly_data = (struct reliable_fragment_reassembly_data_t*) data;
    if ( reassembly_data->packet_data )
    {
        free_function( allocator_context, reassembly_data->packet_data );
        reassembly_data->packet_data = NULL;
    }
}

// ---------------------------------------------------------------

struct reliable_endpoint_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    struct reliable_config_t config;
    double time;
    float rtt;
    float rtt_min;
    float rtt_max;
    float rtt_avg;
    float jitter_avg_vs_min_rtt;
    float jitter_max_vs_min_rtt;
    float jitter_stddev_vs_avg_rtt;
    float packet_loss;
    float sent_bandwidth_kbps;
    float received_bandwidth_kbps;
    float acked_bandwidth_kbps;
    int num_acks;
    uint16_t * acks;
    uint16_t sequence;
    float * rtt_history_buffer;
    uint8_t * transmit_buffer;
    struct reliable_sequence_buffer_t * sent_packets;
    struct reliable_sequence_buffer_t * received_packets;
    struct reliable_sequence_buffer_t * fragment_reassembly;
    uint64_t counters[RELIABLE_ENDPOINT_NUM_COUNTERS];
};

struct reliable_sent_packet_data_t
{
    double time;
    uint32_t acked : 1;
    uint32_t packet_bytes : 31;
};

struct reliable_received_packet_data_t
{
    double time;
    uint32_t packet_bytes;
};

void reliable_default_config( struct reliable_config_t * config )
{
    reliable_assert( config );
    memset( config, 0, sizeof( struct reliable_config_t ) );
    config->name[0] = 'e';
    config->name[1] = 'n';
    config->name[2] = 'd';
    config->name[3] = 'p';
    config->name[4] = 'o';
    config->name[5] = 'i';
    config->name[6] = 'n';
    config->name[7] = 't';
    config->name[8] = '\0';
    config->max_packet_size = 16 * 1024;
    config->fragment_above = 1024;
    config->max_fragments = 16;
    config->fragment_size = 1024;
    config->ack_buffer_size = 256;
    config->sent_packets_buffer_size = 256;
    config->received_packets_buffer_size = 256;
    config->fragment_reassembly_buffer_size = 64;
    config->rtt_smoothing_factor = 0.0025f;
    config->rtt_history_size = 512;
    config->packet_loss_smoothing_factor = 0.1f;
    config->bandwidth_smoothing_factor = 0.1f;
    config->packet_header_size = 28;                    // note: UDP over IPv4 = 20 + 8 bytes, UDP over IPv6 = 40 + 8 bytes
}

// checks the config a caller hands to reliable_endpoint_create. every field is range checked
// and the two relationships between fields are checked: a fragment threshold above the maximum
// packet size can never fire, and a fragment count that does not cover the maximum packet size
// means a large packet has nowhere to go. logs what is wrong and returns 0 to refuse the config

static int reliable_config_valid( struct reliable_config_t * config )
{
    if ( config == NULL )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[reliable] config is NULL\n" );
        return 0;
    }

    if ( config->max_packet_size <= 0 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] max_packet_size must be positive\n", config->name );
        return 0;
    }

    if ( config->fragment_above <= 0 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] fragment_above must be positive\n", config->name );
        return 0;
    }

    if ( config->fragment_size <= 0 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] fragment_size must be positive\n", config->name );
        return 0;
    }

    if ( config->max_fragments <= 0 || config->max_fragments > 256 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] max_fragments must be between 1 and 256\n", config->name );
        return 0;
    }

    if ( config->ack_buffer_size <= 0 || config->sent_packets_buffer_size <= 0 || 
         config->received_packets_buffer_size <= 0 || config->fragment_reassembly_buffer_size <= 0 || 
         config->rtt_history_size <= 0 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] buffer sizes must be positive\n", config->name );
        return 0;
    }

    if ( config->transmit_packet_function == NULL || config->process_packet_function == NULL )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] transmit and process packet functions are required\n", config->name );
        return 0;
    }

    if ( config->fragment_above > config->max_packet_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] fragment_above (%d) is above max_packet_size (%d)\n", 
                         config->name, config->fragment_above, config->max_packet_size );
        return 0;
    }

    // max_fragments * fragment_size must cover max_packet_size. written as a division of the
    // two values already known to be positive, so neither side can overflow

    if ( config->max_fragments <= ( config->max_packet_size - 1 ) / config->fragment_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] max_fragments (%d) times fragment_size (%d) does not cover max_packet_size (%d)\n",
                         config->name, config->max_fragments, config->fragment_size, config->max_packet_size );
        return 0;
    }

    return 1;
}

struct reliable_endpoint_t * reliable_endpoint_create( struct reliable_config_t * config, double time )
{
    if ( !reliable_config_valid( config ) )
    {
        return NULL;
    }

    void * allocator_context = config->allocator_context;
    void * (*allocate_function)(void*,size_t) = config->allocate_function;
    void (*free_function)(void*,void*) = config->free_function;

    if ( allocate_function == NULL )
    {
        allocate_function = reliable_default_allocate_function;
    }

    if ( free_function == NULL )
    {
        free_function = reliable_default_free_function;
    }

    // every size the endpoint allocates, computed up front so an overflowing config is refused
    // before anything is allocated

    size_t acks_bytes;
    size_t rtt_history_bytes;

    if ( !reliable_checked_size( config->ack_buffer_size, sizeof( uint16_t ), &acks_bytes ) )
    {
        return NULL;
    }

    if ( !reliable_checked_size( config->rtt_history_size, sizeof( float ), &rtt_history_bytes ) )
    {
        return NULL;
    }

    // scratch buffer for outgoing packets, so the send path doesn't allocate. sized for whichever is larger: a regular packet or a fragment

    if ( config->max_packet_size > INT_MAX - RELIABLE_MAX_PACKET_HEADER_BYTES || 
         config->fragment_size > INT_MAX - RELIABLE_FRAGMENT_HEADER_BYTES - RELIABLE_MAX_PACKET_HEADER_BYTES )
    {
        return NULL;
    }

    int transmit_buffer_size = config->max_packet_size + RELIABLE_MAX_PACKET_HEADER_BYTES;
    int fragment_transmit_buffer_size = RELIABLE_FRAGMENT_HEADER_BYTES + RELIABLE_MAX_PACKET_HEADER_BYTES + config->fragment_size;
    if ( fragment_transmit_buffer_size > transmit_buffer_size )
    {
        transmit_buffer_size = fragment_transmit_buffer_size;
    }

    // from here on every allocation is checked and the failure path hands back everything
    // already taken, in every build, so the caller only ever sees a complete endpoint or NULL

    struct reliable_endpoint_t * endpoint = (struct reliable_endpoint_t*) allocate_function( allocator_context, sizeof( struct reliable_endpoint_t ) );

    if ( endpoint == NULL )
    {
        return NULL;
    }

    memset( endpoint, 0, sizeof( struct reliable_endpoint_t ) );

    endpoint->allocator_context = allocator_context;
    endpoint->allocate_function = allocate_function;
    endpoint->free_function = free_function;
    endpoint->config = *config;
    endpoint->time = time;

    endpoint->acks = (uint16_t*) allocate_function( allocator_context, acks_bytes );

    endpoint->sent_packets = reliable_sequence_buffer_create( config->sent_packets_buffer_size, 
                                                              sizeof( struct reliable_sent_packet_data_t ), 
                                                              allocator_context, 
                                                              allocate_function, 
                                                              free_function );

    endpoint->received_packets = reliable_sequence_buffer_create( config->received_packets_buffer_size, 
                                                                  sizeof( struct reliable_received_packet_data_t ), 
                                                                  allocator_context, 
                                                                  allocate_function, 
                                                                  free_function );
    
    endpoint->fragment_reassembly = reliable_sequence_buffer_create( config->fragment_reassembly_buffer_size, 
                                                                     sizeof( struct reliable_fragment_reassembly_data_t ), 
                                                                     allocator_context, 
                                                                     allocate_function, 
                                                                     free_function );

    endpoint->rtt_history_buffer = (float*) allocate_function( allocator_context, rtt_history_bytes );

    endpoint->transmit_buffer = (uint8_t*) allocate_function( allocator_context, (size_t) transmit_buffer_size );

    if ( endpoint->acks == NULL || 
         endpoint->sent_packets == NULL || 
         endpoint->received_packets == NULL || 
         endpoint->fragment_reassembly == NULL || 
         endpoint->rtt_history_buffer == NULL || 
         endpoint->transmit_buffer == NULL )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] failed to allocate endpoint\n", config->name );
        reliable_endpoint_destroy( endpoint );
        return NULL;
    }

    for ( int i = 0; i < config->rtt_history_size; i++ )
    {
        endpoint->rtt_history_buffer[i] = -1.0f;
    }

    memset( endpoint->acks, 0, acks_bytes );

    return endpoint;
}

void reliable_endpoint_destroy( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );

    // every member is checked rather than asserted, because reliable_endpoint_create hands a
    // partly built endpoint to this function to unwind an allocation failure

    if ( endpoint->fragment_reassembly )
    {
        int i;
        for ( i = 0; i < endpoint->config.fragment_reassembly_buffer_size; ++i )
        {
            struct reliable_fragment_reassembly_data_t * reassembly_data = (struct reliable_fragment_reassembly_data_t*) 
                reliable_sequence_buffer_at_index( endpoint->fragment_reassembly, i );

            if ( reassembly_data && reassembly_data->packet_data )
            {
                endpoint->free_function( endpoint->allocator_context, reassembly_data->packet_data );
                reassembly_data->packet_data = NULL;
            }
        }
    }

    if ( endpoint->acks )
    {
        endpoint->free_function( endpoint->allocator_context, endpoint->acks );
    }

    if ( endpoint->sent_packets )
    {
        reliable_sequence_buffer_destroy( endpoint->sent_packets );
    }

    if ( endpoint->received_packets )
    {
        reliable_sequence_buffer_destroy( endpoint->received_packets );
    }

    if ( endpoint->fragment_reassembly )
    {
        reliable_sequence_buffer_destroy( endpoint->fragment_reassembly );
    }

    if ( endpoint->rtt_history_buffer )
    {
        endpoint->free_function( endpoint->allocator_context, endpoint->rtt_history_buffer );
    }

    if ( endpoint->transmit_buffer )
    {
        endpoint->free_function( endpoint->allocator_context, endpoint->transmit_buffer );
    }

    endpoint->free_function( endpoint->allocator_context, endpoint );
}

uint16_t reliable_endpoint_next_packet_sequence( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->sequence;
}

static int reliable_write_packet_header( uint8_t * packet_data, uint16_t sequence, uint16_t ack, uint32_t ack_bits )
{
    uint8_t * p = packet_data;

    uint8_t prefix_byte = 0;

    if ( ( ack_bits & 0x000000FF ) != 0x000000FF )
    {
        prefix_byte |= (1<<1);
    }

    if ( ( ack_bits & 0x0000FF00 ) != 0x0000FF00 )
    {
        prefix_byte |= (1<<2);
    }

    if ( ( ack_bits & 0x00FF0000 ) != 0x00FF0000 )
    {
        prefix_byte |= (1<<3);
    }

    if ( ( ack_bits & 0xFF000000 ) != 0xFF000000 )
    {
        prefix_byte |= (1<<4);
    }

    int sequence_difference = sequence - ack;
    if ( sequence_difference < 0 )
        sequence_difference += 65536;
    if ( sequence_difference <= 255 )
        prefix_byte |= (1<<5);

    reliable_write_uint8( &p, prefix_byte );

    reliable_write_uint16( &p, sequence );

    if ( sequence_difference <= 255 )
    {
        reliable_write_uint8( &p, (uint8_t) sequence_difference );
    }
    else
    {
        reliable_write_uint16( &p, ack );
    }

    if ( ( ack_bits & 0x000000FF ) != 0x000000FF )
    {
        reliable_write_uint8( &p, (uint8_t) ( ack_bits & 0x000000FF ) );
    }

    if ( ( ack_bits & 0x0000FF00 ) != 0x0000FF00 )
    {
        reliable_write_uint8( &p, (uint8_t) ( ( ack_bits & 0x0000FF00 ) >> 8 ) );
    }

    if ( ( ack_bits & 0x00FF0000 ) != 0x00FF0000 )
    {
        reliable_write_uint8( &p, (uint8_t) ( ( ack_bits & 0x00FF0000 ) >> 16 ) );
    }

    if ( ( ack_bits & 0xFF000000 ) != 0xFF000000 )
    {
        reliable_write_uint8( &p, (uint8_t) ( ( ack_bits & 0xFF000000 ) >> 24 ) );
    }

    reliable_assert( p - packet_data <= RELIABLE_MAX_PACKET_HEADER_BYTES );

    return (int) ( p - packet_data );
}

void reliable_endpoint_send_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( endpoint );
    reliable_assert( packet_data );
    reliable_assert( packet_bytes > 0 );

    if ( packet_bytes > endpoint->config.max_packet_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] packet too large to send. packet is %d bytes, maximum is %d\n", 
            endpoint->config.name, packet_bytes, endpoint->config.max_packet_size );
        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_SEND]++;
        return;
    }

    uint16_t sequence = endpoint->sequence++;
    uint16_t ack;
    uint32_t ack_bits;

    reliable_sequence_buffer_generate_ack_bits( endpoint->received_packets, &ack, &ack_bits );

    reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] sending packet %d\n", endpoint->config.name, sequence );

    struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) reliable_sequence_buffer_insert( endpoint->sent_packets, sequence );

    reliable_assert( sent_packet_data );

    sent_packet_data->time = endpoint->time;
    sent_packet_data->packet_bytes = endpoint->config.packet_header_size + packet_bytes;
    sent_packet_data->acked = 0;

    if ( packet_bytes <= endpoint->config.fragment_above )
    {
        // regular packet

        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] sending packet %d without fragmentation\n", endpoint->config.name, sequence );

        uint8_t * transmit_packet_data = endpoint->transmit_buffer;

        int packet_header_bytes = reliable_write_packet_header( transmit_packet_data, sequence, ack, ack_bits );

        memcpy( transmit_packet_data + packet_header_bytes, packet_data, packet_bytes );

        endpoint->config.transmit_packet_function( endpoint->config.context, endpoint->config.id, sequence, transmit_packet_data, packet_header_bytes + packet_bytes );
    }
    else
    {
        // fragmented packet

        uint8_t packet_header[RELIABLE_MAX_PACKET_HEADER_BYTES];

        memset( packet_header, 0, RELIABLE_MAX_PACKET_HEADER_BYTES );

        int packet_header_bytes = reliable_write_packet_header( packet_header, sequence, ack, ack_bits );        

        int num_fragments = ( packet_bytes / endpoint->config.fragment_size ) + ( ( packet_bytes % endpoint->config.fragment_size ) != 0 ? 1 : 0 );

        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] sending packet %d as %d fragments\n", endpoint->config.name, sequence, num_fragments );

        reliable_assert( num_fragments >= 1 );
        reliable_assert( num_fragments <= endpoint->config.max_fragments );

        uint8_t * fragment_packet_data = endpoint->transmit_buffer;

        uint8_t * q = packet_data;

        uint8_t * end = q + packet_bytes;

        int fragment_id;
        for ( fragment_id = 0; fragment_id < num_fragments; ++fragment_id )
        {
            uint8_t * p = fragment_packet_data;

            reliable_write_uint8( &p, 1 );
            reliable_write_uint16( &p, sequence );
            reliable_write_uint8( &p, (uint8_t) fragment_id );
            reliable_write_uint8( &p, (uint8_t) ( num_fragments - 1 ) );

            if ( fragment_id == 0 )
            {
                memcpy( p, packet_header, packet_header_bytes );
                p += packet_header_bytes;
            }

            int bytes_to_copy = endpoint->config.fragment_size;
            if ( q + bytes_to_copy > end )
            {
                bytes_to_copy = (int) ( end - q );
            }

            memcpy( p, q, bytes_to_copy );

            p += bytes_to_copy;
            q += bytes_to_copy;

            int fragment_packet_bytes = (int) ( p - fragment_packet_data );

            endpoint->config.transmit_packet_function( endpoint->config.context, endpoint->config.id, sequence, fragment_packet_data, fragment_packet_bytes );

            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_SENT]++;
        }
    }

    endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT]++;
}

static int reliable_read_packet_header( RELIABLE_CONST char * name, uint8_t * packet_data, int packet_bytes, uint16_t * sequence, uint16_t * ack, uint32_t * ack_bits )
{
    if ( packet_bytes < 3 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too small for packet header (1)\n", name );
        return -1;
    }

    uint8_t * p = packet_data;

    uint8_t prefix_byte = reliable_read_uint8( &p );

    if ( ( prefix_byte & 1 ) != 0 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] prefix byte does not indicate a regular packet\n", name );
        return -1;
    }

    *sequence = reliable_read_uint16( &p );

    if ( prefix_byte & (1<<5) )
    {
        if ( packet_bytes < 3 + 1 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too small for packet header (2)\n", name );
            return -1;
        }
        uint8_t sequence_difference = reliable_read_uint8( &p );
        *ack = *sequence - sequence_difference;
    }
    else
    {
        if ( packet_bytes < 3 + 2 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too small for packet header (3)\n", name );
            return -1;
        }
        *ack = reliable_read_uint16( &p );
    }

    int expected_bytes = 0;
    int i;
    for ( i = 1; i <= 4; ++i )
    {
        if ( prefix_byte & (1<<i) )
        {
            expected_bytes++;
        }
    }
    if ( packet_bytes < ( p - packet_data ) + expected_bytes )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too small for packet header (4)\n", name );
        return -1;
    }

    *ack_bits = 0xFFFFFFFF;

    if ( prefix_byte & (1<<1) )
    {
        *ack_bits &= 0xFFFFFF00;
        *ack_bits |= (uint32_t) ( reliable_read_uint8( &p ) );
    }

    if ( prefix_byte & (1<<2) )
    {
        *ack_bits &= 0xFFFF00FF;
        *ack_bits |= (uint32_t) ( reliable_read_uint8( &p ) ) << 8;
    }

    if ( prefix_byte & (1<<3) )
    {
        *ack_bits &= 0xFF00FFFF;
        *ack_bits |= (uint32_t) ( reliable_read_uint8( &p ) ) << 16;
    }

    if ( prefix_byte & (1<<4) )
    {
        *ack_bits &= 0x00FFFFFF;
        *ack_bits |= (uint32_t) ( reliable_read_uint8( &p ) ) << 24;
    }

    return (int) ( p - packet_data );
}

static int reliable_read_fragment_header( char * name, 
                                   uint8_t * packet_data, 
                                   int packet_bytes, 
                                   int max_fragments, 
                                   int fragment_size, 
                                   int * fragment_id, 
                                   int * num_fragments, 
                                   int * fragment_bytes, 
                                   uint16_t * sequence, 
                                   uint16_t * ack, 
                                   uint32_t * ack_bits )
{
    if ( packet_bytes < RELIABLE_FRAGMENT_HEADER_BYTES )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet is too small to read fragment header\n", name );
        return -1;
    }

    uint8_t * p = packet_data;

    uint8_t prefix_byte = reliable_read_uint8( &p );
    if ( prefix_byte != 1 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] prefix byte is not a fragment\n", name );
        return -1;
    }
    
    *sequence = reliable_read_uint16( &p );
    *fragment_id = (int) reliable_read_uint8( &p );
    *num_fragments = ( (int) reliable_read_uint8( &p ) ) + 1;

    if ( *num_fragments > max_fragments )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] num fragments %d outside of range of max fragments %d\n", name, *num_fragments, max_fragments );
        return -1;
    }

    if ( *fragment_id >= *num_fragments )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] fragment id %d outside of range of num fragments %d\n", name, *fragment_id, *num_fragments );
        return -1;
    }

    *fragment_bytes = packet_bytes - RELIABLE_FRAGMENT_HEADER_BYTES;

    uint16_t packet_sequence = 0;
    uint16_t packet_ack = 0;
    uint32_t packet_ack_bits = 0;

    if ( *fragment_id == 0 )
    {
        int packet_header_bytes = reliable_read_packet_header( name,
                                                               packet_data + RELIABLE_FRAGMENT_HEADER_BYTES,
                                                               packet_bytes - RELIABLE_FRAGMENT_HEADER_BYTES,
                                                               &packet_sequence,
                                                               &packet_ack, 
                                                               &packet_ack_bits );

        if ( packet_header_bytes < 0 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] bad packet header in fragment\n", name );
            return -1;
        }

        if ( packet_sequence != *sequence )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] bad packet sequence in fragment. expected %d, got %d\n", name, *sequence, packet_sequence );
            return -1;
        }

        // the packet header is re-encoded canonically during reassembly, so a non-canonical
        // header would shift where the fragment payload lands. reject it here instead.

        uint8_t canonical_header[RELIABLE_MAX_PACKET_HEADER_BYTES];
        int canonical_header_bytes = reliable_write_packet_header( canonical_header, packet_sequence, packet_ack, packet_ack_bits );
        if ( canonical_header_bytes != packet_header_bytes || memcmp( canonical_header, packet_data + RELIABLE_FRAGMENT_HEADER_BYTES, canonical_header_bytes ) != 0 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] non-canonical packet header in fragment\n", name );
            return -1;
        }

        *fragment_bytes = packet_bytes - packet_header_bytes - RELIABLE_FRAGMENT_HEADER_BYTES;
    }

    *ack = packet_ack;
    *ack_bits = packet_ack_bits;

    if ( *fragment_bytes > fragment_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] fragment bytes %d > fragment size %d\n", name, *fragment_bytes, fragment_size );
        return - 1;
    }

    if ( *fragment_id != *num_fragments - 1 && *fragment_bytes != fragment_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] fragment %d is %d bytes, which is not the expected fragment size %d\n", 
            name, *fragment_id, *fragment_bytes, fragment_size );
        return -1;
    }

    return (int) ( p - packet_data );
}

static void reliable_store_fragment_data( struct reliable_fragment_reassembly_data_t * reassembly_data, 
                                   uint16_t sequence, 
                                   uint16_t ack, 
                                   uint32_t ack_bits, 
                                   int fragment_id, 
                                   int fragment_size, 
                                   uint8_t * fragment_data, 
                                   int fragment_bytes )
{
    if ( fragment_id == 0 )
    {
        uint8_t packet_header[RELIABLE_MAX_PACKET_HEADER_BYTES];

        memset( packet_header, 0, RELIABLE_MAX_PACKET_HEADER_BYTES );

        reassembly_data->packet_header_bytes = reliable_write_packet_header( packet_header, sequence, ack, ack_bits );

        memcpy( reassembly_data->packet_data + RELIABLE_MAX_PACKET_HEADER_BYTES - reassembly_data->packet_header_bytes, 
                packet_header, 
                reassembly_data->packet_header_bytes );

        fragment_data += reassembly_data->packet_header_bytes;
        fragment_bytes -= reassembly_data->packet_header_bytes;
    }

    if ( fragment_id == reassembly_data->num_fragments_total - 1 )
    {
        reassembly_data->packet_bytes = ( reassembly_data->num_fragments_total - 1 ) * fragment_size + fragment_bytes;
    }

    size_t offset = RELIABLE_MAX_PACKET_HEADER_BYTES + fragment_id * fragment_size;
    size_t end_offset = offset + fragment_bytes;
    size_t max_size = RELIABLE_MAX_PACKET_HEADER_BYTES +
                      reassembly_data->num_fragments_total * fragment_size;
    
    if ( fragment_bytes < 0 || end_offset > max_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG,
            "[reliable] invalid fragment size %d (would write past %zu/%zu)\n",
            fragment_bytes, end_offset, max_size );
        return;
    }
    
    memcpy( reassembly_data->packet_data + RELIABLE_MAX_PACKET_HEADER_BYTES + fragment_id * fragment_size, fragment_data, fragment_bytes );
}

void reliable_endpoint_receive_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( endpoint );
    reliable_assert( packet_data );
    reliable_assert( packet_bytes > 0 );

    if ( packet_bytes > endpoint->config.max_packet_size + RELIABLE_MAX_PACKET_HEADER_BYTES + RELIABLE_FRAGMENT_HEADER_BYTES )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too large to receive. packet is at least %d bytes, maximum is %d\n",
            endpoint->config.name, packet_bytes - ( RELIABLE_MAX_PACKET_HEADER_BYTES + RELIABLE_FRAGMENT_HEADER_BYTES ), endpoint->config.max_packet_size );
        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE]++;
        return;
    }

    uint8_t prefix_byte = packet_data[0];

    if ( ( prefix_byte & 1 ) == 0 )
    {
        // regular packet

        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED]++;

        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;

        int packet_header_bytes = reliable_read_packet_header( endpoint->config.name, packet_data, packet_bytes, &sequence, &ack, &ack_bits );
        if ( packet_header_bytes < 0 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] ignoring invalid packet. could not read packet header\n", endpoint->config.name );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_INVALID]++;
            return;
        }

        reliable_assert( packet_header_bytes <= packet_bytes );

        int packet_payload_bytes = packet_bytes - packet_header_bytes;

        if ( packet_payload_bytes > endpoint->config.max_packet_size )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] packet too large to receive. packet is at %d bytes, maximum is %d\n",
                endpoint->config.name, packet_payload_bytes, endpoint->config.max_packet_size );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE]++;
            return;
        }

        if ( !reliable_sequence_buffer_test_insert( endpoint->received_packets, sequence ) )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] ignoring stale packet %d\n", endpoint->config.name, sequence );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_STALE]++;
            return;
        }

        if ( reliable_sequence_buffer_exists( endpoint->received_packets, sequence ) )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] ignoring duplicate packet %d\n", endpoint->config.name, sequence );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_DUPLICATE]++;
            return;
        }

        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] processing packet %d\n", endpoint->config.name, sequence );

        if ( endpoint->config.process_packet_function( endpoint->config.context, 
                                                       endpoint->config.id, 
                                                       sequence, 
                                                       packet_data + packet_header_bytes, 
                                                       packet_bytes - packet_header_bytes ) )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] process packet %d successful\n", endpoint->config.name, sequence );

            struct reliable_received_packet_data_t * received_packet_data = (struct reliable_received_packet_data_t*) 
                reliable_sequence_buffer_insert( endpoint->received_packets, sequence );

            reliable_sequence_buffer_advance_with_cleanup( endpoint->fragment_reassembly, sequence, reliable_fragment_reassembly_data_cleanup );

            reliable_assert( received_packet_data );

            received_packet_data->time = endpoint->time;
            received_packet_data->packet_bytes = endpoint->config.packet_header_size + packet_bytes;

            int i;
            for ( i = 0; i < 32; ++i )
            {
                if ( ack_bits & 1 )
                {                    
                    uint16_t ack_sequence = ack - ((uint16_t)i);
                    
                    struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) 
                        reliable_sequence_buffer_find( endpoint->sent_packets, ack_sequence );

                    if ( sent_packet_data && !sent_packet_data->acked )
                    {
                        if ( endpoint->num_acks < endpoint->config.ack_buffer_size )
                        {
                            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] acked packet %d\n", endpoint->config.name, ack_sequence );
                            endpoint->acks[endpoint->num_acks++] = ack_sequence;
                            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED]++;
                            sent_packet_data->acked = 1;

                            const float rtt = (float) ( endpoint->time - sent_packet_data->time ) * 1000.0f;

                            reliable_assert( rtt >= 0.0 );

                            int index = ack_sequence % endpoint->config.rtt_history_size;

                            endpoint->rtt_history_buffer[index] = rtt;

                            if ( ( endpoint->rtt == 0.0f && rtt > 0.0f ) || fabs( endpoint->rtt - rtt ) < 0.00001 )
                            {
                                endpoint->rtt = rtt;
                            }
                            else
                            {
                                endpoint->rtt += ( rtt - endpoint->rtt ) * endpoint->config.rtt_smoothing_factor;
                            }
                        }
                        else
                        {
                            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] ack buffer is full. dropped ack for packet %d. make sure you call reliable_endpoint_clear_acks\n",
                                endpoint->config.name, ack_sequence );
                        }
                    }
                }
                ack_bits >>= 1;
            }
        }
        else
        {
            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] process packet failed\n", endpoint->config.name );
        }
    }
    else
    {
        // fragment packet

        int fragment_id;
        int num_fragments;
        int fragment_bytes;

        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;

        int fragment_header_bytes = reliable_read_fragment_header( endpoint->config.name, 
                                                                   packet_data, 
                                                                   packet_bytes, 
                                                                   endpoint->config.max_fragments, 
                                                                   endpoint->config.fragment_size,
                                                                   &fragment_id, 
                                                                   &num_fragments, 
                                                                   &fragment_bytes, 
                                                                   &sequence, 
                                                                   &ack, 
                                                                   &ack_bits );

        if ( fragment_header_bytes < 0 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] ignoring invalid fragment. could not read fragment header\n", endpoint->config.name );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID]++;
            return;
        }

        if ( reliable_sequence_buffer_exists( endpoint->received_packets, sequence ) )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] ignoring fragment %d of packet %d. packet already received\n",
                endpoint->config.name, fragment_id, sequence );
            return;
        }

        struct reliable_fragment_reassembly_data_t * reassembly_data = (struct reliable_fragment_reassembly_data_t*)
            reliable_sequence_buffer_find( endpoint->fragment_reassembly, sequence );

        if ( !reassembly_data )
        {
            reassembly_data = (struct reliable_fragment_reassembly_data_t*) 
                reliable_sequence_buffer_insert_with_cleanup( endpoint->fragment_reassembly, sequence, reliable_fragment_reassembly_data_cleanup );

            if ( !reassembly_data )
            {
                reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] ignoring invalid fragment. could not insert in reassembly buffer (stale)\n", endpoint->config.name );
                endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID]++;
                return;
            }

            reliable_sequence_buffer_advance( endpoint->received_packets, sequence );

            size_t packet_buffer_size = (size_t) RELIABLE_MAX_PACKET_HEADER_BYTES + (size_t) num_fragments * (size_t) endpoint->config.fragment_size;

            reassembly_data->sequence = sequence;
            reassembly_data->ack = 0;
            reassembly_data->ack_bits = 0;
            reassembly_data->num_fragments_received = 0;
            reassembly_data->num_fragments_total = num_fragments;
            reassembly_data->packet_data = (uint8_t*) endpoint->allocate_function( endpoint->allocator_context, packet_buffer_size );
            reliable_assert( reassembly_data->packet_data );
            reassembly_data->packet_bytes = 0;
            reassembly_data->packet_header_bytes = 0;
            memset( reassembly_data->fragment_received, 0, sizeof( reassembly_data->fragment_received ) );
        }

        if ( num_fragments != (int) reassembly_data->num_fragments_total )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] ignoring invalid fragment. fragment count mismatch. expected %d, got %d\n", 
                endpoint->config.name, (int) reassembly_data->num_fragments_total, num_fragments );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID]++;
            return;
        }

        if ( reassembly_data->fragment_received[fragment_id] )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] ignoring fragment %d of packet %d. fragment already received\n", 
                endpoint->config.name, fragment_id, sequence );
            return;
        }

        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] received fragment %d of packet %d (%d/%d)\n", 
            endpoint->config.name, fragment_id, sequence, reassembly_data->num_fragments_received+1, num_fragments );

        reassembly_data->num_fragments_received++;
        reassembly_data->fragment_received[fragment_id] = 1;

        reliable_store_fragment_data( reassembly_data, 
                                      sequence, 
                                      ack, 
                                      ack_bits, 
                                      fragment_id, 
                                      endpoint->config.fragment_size, 
                                      packet_data + fragment_header_bytes, 
                                      packet_bytes - fragment_header_bytes );

        if ( reassembly_data->num_fragments_received == reassembly_data->num_fragments_total )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] completed reassembly of packet %d\n", endpoint->config.name, sequence );

            reliable_endpoint_receive_packet( endpoint, 
                                              reassembly_data->packet_data + RELIABLE_MAX_PACKET_HEADER_BYTES - reassembly_data->packet_header_bytes, 
                                              reassembly_data->packet_header_bytes + reassembly_data->packet_bytes );

            reliable_sequence_buffer_remove_with_cleanup( endpoint->fragment_reassembly, sequence, reliable_fragment_reassembly_data_cleanup );
        }

        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED]++;
    }
}

void reliable_endpoint_free_packet( struct reliable_endpoint_t * endpoint, void * packet )
{
    reliable_assert( endpoint );
    reliable_assert( packet );
    endpoint->free_function( endpoint->allocator_context, packet );
}

RELIABLE_CONST uint16_t * reliable_endpoint_get_acks( struct reliable_endpoint_t * endpoint, int * num_acks )
{
    reliable_assert( endpoint );
    reliable_assert( num_acks );
    *num_acks = endpoint->num_acks;
    return endpoint->acks;
}

void reliable_endpoint_clear_acks( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    endpoint->num_acks = 0;
}

void reliable_endpoint_reset( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );

    endpoint->num_acks = 0;
    endpoint->sequence = 0;

    // every value a getter can return goes back to what it was at create, so the header's
    // promise holds for the statistics as well as for the buffers

    endpoint->rtt = 0.0f;
    endpoint->rtt_min = 0.0f;
    endpoint->rtt_max = 0.0f;
    endpoint->rtt_avg = 0.0f;
    endpoint->jitter_avg_vs_min_rtt = 0.0f;
    endpoint->jitter_max_vs_min_rtt = 0.0f;
    endpoint->jitter_stddev_vs_avg_rtt = 0.0f;
    endpoint->packet_loss = 0.0f;
    endpoint->sent_bandwidth_kbps = 0.0f;
    endpoint->received_bandwidth_kbps = 0.0f;
    endpoint->acked_bandwidth_kbps = 0.0f;

    memset( endpoint->acks, 0, endpoint->config.ack_buffer_size * sizeof( uint16_t ) );
    memset( endpoint->counters, 0, RELIABLE_ENDPOINT_NUM_COUNTERS * sizeof( uint64_t ) );

    int rtt_index;
    for ( rtt_index = 0; rtt_index < endpoint->config.rtt_history_size; ++rtt_index )
    {
        endpoint->rtt_history_buffer[rtt_index] = -1.0f;
    }

    int i;
    for ( i = 0; i < endpoint->config.fragment_reassembly_buffer_size; ++i )
    {
        struct reliable_fragment_reassembly_data_t * reassembly_data = (struct reliable_fragment_reassembly_data_t*) 
            reliable_sequence_buffer_at_index( endpoint->fragment_reassembly, i );

        if ( reassembly_data && reassembly_data->packet_data )
        {
            endpoint->free_function( endpoint->allocator_context, reassembly_data->packet_data );
            reassembly_data->packet_data = NULL;
        }
    }

    reliable_sequence_buffer_reset( endpoint->sent_packets );
    reliable_sequence_buffer_reset( endpoint->received_packets );
    reliable_sequence_buffer_reset( endpoint->fragment_reassembly );
}

void reliable_endpoint_update( struct reliable_endpoint_t * endpoint, double time )
{
    reliable_assert( endpoint );

    endpoint->time = time;

    // calculate min and max rtt
    {
        float min_rtt = FLT_MAX;
        float max_rtt = 0.0f;
        float sum_rtt = 0.0f;
        int count = 0;
        for ( int i = 0; i < endpoint->config.rtt_history_size; i++ )
        {
            const float rtt = endpoint->rtt_history_buffer[i];
            if ( rtt >= 0.0f )
            {
                if ( rtt < min_rtt )
                {
                    min_rtt = rtt;
                }
                if ( rtt > max_rtt )
                {
                    max_rtt = rtt;
                }
                sum_rtt += rtt;
                count++;
            }
        }
        // the sample count, not the value of min_rtt, says whether the history is empty. a
        // sentinel compared against a real rtt reports 0 for a link slow enough to reach it

        if ( count > 0 )
        {
            endpoint->rtt_min = min_rtt;
            endpoint->rtt_max = max_rtt;
            endpoint->rtt_avg = sum_rtt / (float)count;
        }
        else
        {
            endpoint->rtt_min = 0.0f;
            endpoint->rtt_max = 0.0f;
            endpoint->rtt_avg = 0.0f;
        }
    }

    // calculate average jitter vs. min rtt
    {
        float sum = 0.0f;
        int count = 0;
        for ( int i = 0; i < endpoint->config.rtt_history_size; i++ )
        {
            if ( endpoint->rtt_history_buffer[i] >= 0.0f )
            {
                sum += ( endpoint->rtt_history_buffer[i] - endpoint->rtt_min );
                count++;
            }
        }
        if ( count > 0 )
        {
            endpoint->jitter_avg_vs_min_rtt = sum / (float)count;
        }
        else
        {
            endpoint->jitter_avg_vs_min_rtt = 0.0f;
        }
    }

    // calculate max jitter vs. min rtt
    {
        float max = 0.0f;
        for ( int i = 0; i < endpoint->config.rtt_history_size; i++ )
        {
            if ( endpoint->rtt_history_buffer[i] >= 0.0f )
            {
                float difference = ( endpoint->rtt_history_buffer[i] - endpoint->rtt_min );
                if ( difference > max )
                {
                    max = difference;
                }
            }
        }
        endpoint->jitter_max_vs_min_rtt = max;
    }

    // calculate stddev jitter vs. avg rtt
    {
        float sum = 0.0f;
        int count = 0;
        for ( int i = 0; i < endpoint->config.rtt_history_size; i++ )
        {
            if ( endpoint->rtt_history_buffer[i] >= 0.0f )
            {
                float deviation = ( endpoint->rtt_history_buffer[i] - endpoint->rtt_avg );
                sum += deviation * deviation;
                count++;
            }
        }
        if ( count > 0 )
        {
            endpoint->jitter_stddev_vs_avg_rtt = (float) pow( sum / (float)count, 0.5f );
        }
        else
        {
            endpoint->jitter_stddev_vs_avg_rtt = 0.0f;
        }
    }

    // calculate packet loss
    {
        uint32_t base_sequence = ( endpoint->sent_packets->sequence - endpoint->config.sent_packets_buffer_size + 1 ) + 0xFFFF;
        int i;
        int num_sent = 0;
        int num_dropped = 0;
        int num_samples = endpoint->config.sent_packets_buffer_size / 2;
        for ( i = 0; i < num_samples; ++i )
        {
            uint16_t sequence = (uint16_t) ( base_sequence + i );
            struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) reliable_sequence_buffer_find( endpoint->sent_packets, sequence );
            if ( sent_packet_data )
            {
                num_sent++;
                if ( !sent_packet_data->acked )
                {
                    num_dropped++;
                }
            }
        }
        if ( num_sent > 0 )
        {
            float packet_loss = ( (float) num_dropped ) / ( (float) num_sent ) * 100.0f;
            if ( fabs( endpoint->packet_loss - packet_loss ) > 0.00001 )
            {
                endpoint->packet_loss += ( packet_loss - endpoint->packet_loss ) * endpoint->config.packet_loss_smoothing_factor;
            }
            else
            {
                endpoint->packet_loss = packet_loss;
            }
        }
        else
        {
            endpoint->packet_loss = 0.0f;
        }
    }

    // calculate sent bandwidth
    {
        uint32_t base_sequence = ( endpoint->sent_packets->sequence - endpoint->config.sent_packets_buffer_size + 1 ) + 0xFFFF;
        int i;
        int bytes_sent = 0;
        double start_time = FLT_MAX;
        double finish_time = 0.0;
        int num_samples = endpoint->config.sent_packets_buffer_size / 2;
        for ( i = 0; i < num_samples; ++i )
        {
            uint16_t sequence = (uint16_t) ( base_sequence + i );
            struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) 
                reliable_sequence_buffer_find( endpoint->sent_packets, sequence );
            if ( !sent_packet_data )
            {
                continue;
            }
            bytes_sent += sent_packet_data->packet_bytes;
            if ( sent_packet_data->time < start_time )
            {
                start_time = sent_packet_data->time;
            }
            if ( sent_packet_data->time > finish_time )
            {
                finish_time = sent_packet_data->time;
            }
        }
        if ( start_time != FLT_MAX && finish_time > start_time )
        {
            float sent_bandwidth_kbps = (float) ( ( (double) bytes_sent ) / ( finish_time - start_time ) * 8.0f / 1000.0f );
            if ( fabs( endpoint->sent_bandwidth_kbps - sent_bandwidth_kbps ) > 0.00001 )
            {
                endpoint->sent_bandwidth_kbps += ( sent_bandwidth_kbps - endpoint->sent_bandwidth_kbps ) * endpoint->config.bandwidth_smoothing_factor;
            }
            else
            {
                endpoint->sent_bandwidth_kbps = sent_bandwidth_kbps;
            }
        }
    }

    // calculate received bandwidth
    {
        uint32_t base_sequence = ( endpoint->received_packets->sequence - endpoint->config.received_packets_buffer_size + 1 ) + 0xFFFF;
        int i;
        int bytes_sent = 0;
        double start_time = FLT_MAX;
        double finish_time = 0.0;
        int num_samples = endpoint->config.received_packets_buffer_size / 2;
        for ( i = 0; i < num_samples; ++i )
        {
            uint16_t sequence = (uint16_t) ( base_sequence + i );
            struct reliable_received_packet_data_t * received_packet_data = (struct reliable_received_packet_data_t*) 
                reliable_sequence_buffer_find( endpoint->received_packets, sequence );
            if ( !received_packet_data )
            {
                continue;
            }
            bytes_sent += received_packet_data->packet_bytes;
            if ( received_packet_data->time < start_time )
            {
                start_time = received_packet_data->time;
            }
            if ( received_packet_data->time > finish_time )
            {
                finish_time = received_packet_data->time;
            }
        }
        if ( start_time != FLT_MAX && finish_time > start_time )
        {
            float received_bandwidth_kbps = (float) ( ( (double) bytes_sent ) / ( finish_time - start_time ) * 8.0f / 1000.0f );
            if ( fabs( endpoint->received_bandwidth_kbps - received_bandwidth_kbps ) > 0.00001 )
            {
                endpoint->received_bandwidth_kbps += ( received_bandwidth_kbps - endpoint->received_bandwidth_kbps ) * endpoint->config.bandwidth_smoothing_factor;
            }
            else
            {
                endpoint->received_bandwidth_kbps = received_bandwidth_kbps;
            }
        }
    }

    // calculate acked bandwidth
    {
        uint32_t base_sequence = ( endpoint->sent_packets->sequence - endpoint->config.sent_packets_buffer_size + 1 ) + 0xFFFF;
        int i;
        int bytes_sent = 0;
        double start_time = FLT_MAX;
        double finish_time = 0.0;
        int num_samples = endpoint->config.sent_packets_buffer_size / 2;
        for ( i = 0; i < num_samples; ++i )
        {
            uint16_t sequence = (uint16_t) ( base_sequence + i );
            struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) 
                reliable_sequence_buffer_find( endpoint->sent_packets, sequence );
            if ( !sent_packet_data || !sent_packet_data->acked )
            {
                continue;
            }
            bytes_sent += sent_packet_data->packet_bytes;
            if ( sent_packet_data->time < start_time )
            {
                start_time = sent_packet_data->time;
            }
            if ( sent_packet_data->time > finish_time )
            {
                finish_time = sent_packet_data->time;
            }
        }
        if ( start_time != FLT_MAX && finish_time > start_time )
        {
            float acked_bandwidth_kbps = (float) ( ( (double) bytes_sent ) / ( finish_time - start_time ) * 8.0f / 1000.0f );
            if ( fabs( endpoint->acked_bandwidth_kbps - acked_bandwidth_kbps ) > 0.00001 )
            {
                endpoint->acked_bandwidth_kbps += ( acked_bandwidth_kbps - endpoint->acked_bandwidth_kbps ) * endpoint->config.bandwidth_smoothing_factor;
            }
            else
            {
                endpoint->acked_bandwidth_kbps = acked_bandwidth_kbps;
            }
        }
    }
}

float reliable_endpoint_rtt( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->rtt;
}

float reliable_endpoint_rtt_min( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->rtt_min;
}

float reliable_endpoint_rtt_max( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->rtt_max;
}

float reliable_endpoint_rtt_avg( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->rtt_avg;
}

float reliable_endpoint_jitter_avg_vs_min_rtt( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->jitter_avg_vs_min_rtt;
}

float reliable_endpoint_jitter_max_vs_min_rtt( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->jitter_max_vs_min_rtt;
}


float reliable_endpoint_jitter_stddev_vs_avg_rtt( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->jitter_stddev_vs_avg_rtt;
}

float reliable_endpoint_packet_loss( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->packet_loss;
}

void reliable_endpoint_bandwidth( struct reliable_endpoint_t * endpoint, float * sent_bandwidth_kbps, float * received_bandwidth_kbps, float * acked_bandwidth_kbps )
{
    reliable_assert( endpoint );
    reliable_assert( sent_bandwidth_kbps );
    reliable_assert( acked_bandwidth_kbps );
    reliable_assert( received_bandwidth_kbps );
    *sent_bandwidth_kbps = endpoint->sent_bandwidth_kbps;
    *received_bandwidth_kbps = endpoint->received_bandwidth_kbps;
    *acked_bandwidth_kbps = endpoint->acked_bandwidth_kbps;
}

RELIABLE_CONST uint64_t * reliable_endpoint_counters( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->counters;
}

void reliable_copy_string( char * dest, RELIABLE_CONST char * source, size_t dest_size )
{
    reliable_assert( dest );
    reliable_assert( source );
    reliable_assert( dest_size >= 1 );
    memset( dest, 0, dest_size );
    for ( size_t i = 0; i < dest_size - 1; i++ )
    {
        if ( source[i] == '\0' )
            break;
        dest[i] = source[i];
    }
}

// ---------------------------------------------------------------

#if RELIABLE_ENABLE_TESTS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

static void check_handler( RELIABLE_CONST char * condition, 
                           RELIABLE_CONST char * function,
                           RELIABLE_CONST char * file,
                           int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
    fflush( stdout );
#ifdef RELIABLE_DEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define check( condition )                                                                                      \
do                                                                                                              \
{                                                                                                               \
    if ( !(condition) )                                                                                         \
    {                                                                                                           \
        check_handler( #condition, (RELIABLE_CONST char*) __FUNCTION__, __FILE__, __LINE__ );                   \
    }                                                                                                           \
} while(0)

static void test_endian()
{
    uint32_t value = 0x11223344;

    char * bytes = (char*) &value;

#if RELIABLE_LITTLE_ENDIAN

    check( bytes[0] == 0x44 );
    check( bytes[1] == 0x33 );
    check( bytes[2] == 0x22 );
    check( bytes[3] == 0x11 );

#else // #if RELIABLE_LITTLE_ENDIAN

    check( bytes[3] == 0x44 );
    check( bytes[2] == 0x33 );
    check( bytes[1] == 0x22 );
    check( bytes[0] == 0x11 );

#endif // #if RELIABLE_LITTLE_ENDIAN
}

struct test_sequence_data_t
{
    uint16_t sequence;
};

#define TEST_SEQUENCE_BUFFER_SIZE 256

static void test_sequence_buffer()
{
    struct reliable_sequence_buffer_t * sequence_buffer = reliable_sequence_buffer_create( TEST_SEQUENCE_BUFFER_SIZE, 
                                                                                           sizeof( struct test_sequence_data_t ), 
                                                                                           NULL, 
                                                                                           NULL, 
                                                                                           NULL );

    check( sequence_buffer );
    check( sequence_buffer->sequence == 0 );
    check( sequence_buffer->num_entries == TEST_SEQUENCE_BUFFER_SIZE );
    check( sequence_buffer->entry_stride == sizeof( struct test_sequence_data_t ) );

    int i;
    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        check( reliable_sequence_buffer_find( sequence_buffer, ((uint16_t)i) ) == NULL );
    }                                                                      

    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE*4; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_insert( sequence_buffer, ((uint16_t)i) );
        check( entry );
        entry->sequence = (uint16_t) i;
        check( sequence_buffer->sequence == i + 1 );
    }

    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_insert( sequence_buffer, ((uint16_t)i) );
        check( entry == NULL );
    }    

    int index = TEST_SEQUENCE_BUFFER_SIZE * 4;
    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_find( sequence_buffer, (uint16_t) index );
        check( entry );
        check( entry->sequence == (uint32_t) index );
        index--;
    }

    reliable_sequence_buffer_reset( sequence_buffer );

    check( sequence_buffer );
    check( sequence_buffer->sequence == 0 );
    check( sequence_buffer->num_entries == TEST_SEQUENCE_BUFFER_SIZE );
    check( sequence_buffer->entry_stride == sizeof( struct test_sequence_data_t ) );

    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        check( reliable_sequence_buffer_find( sequence_buffer, (uint16_t) i ) == NULL );
    }

    reliable_sequence_buffer_destroy( sequence_buffer );
}

static void test_generate_ack_bits()
{
    struct reliable_sequence_buffer_t * sequence_buffer = reliable_sequence_buffer_create( TEST_SEQUENCE_BUFFER_SIZE, 
                                                                                           sizeof( struct test_sequence_data_t ), 
                                                                                           NULL, 
                                                                                           NULL, 
                                                                                           NULL );

    uint16_t ack = 0;
    uint32_t ack_bits = 0xFFFFFFFF;

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );
    check( ack == 0xFFFF );
    check( ack_bits == 0 );

    int i;
    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        reliable_sequence_buffer_insert( sequence_buffer, (uint16_t) i );
    }

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );
    check( ack == TEST_SEQUENCE_BUFFER_SIZE );
    check( ack_bits == 0xFFFFFFFF );

    reliable_sequence_buffer_reset( sequence_buffer );

    uint16_t input_acks[] = { 1, 5, 9, 11 };
    int input_num_acks = sizeof( input_acks ) / sizeof( uint16_t );
    for ( i = 0; i < input_num_acks; ++i )
    {
        reliable_sequence_buffer_insert( sequence_buffer, input_acks[i] );
    }

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );

    check( ack == 11 );
    check( ack_bits == ( 1 | (1<<(11-9)) | (1<<(11-5)) | (1<<(11-1)) ) );

    reliable_sequence_buffer_destroy( sequence_buffer );
}

static void test_packet_header()
{
    uint16_t write_sequence;
    uint16_t write_ack;
    uint32_t write_ack_bits;

    uint16_t read_sequence;
    uint16_t read_ack;
    uint32_t read_ack_bits;

    uint8_t packet_data[RELIABLE_MAX_PACKET_HEADER_BYTES];

    // worst case, sequence and ack are far apart, no packets acked.

    write_sequence = 10000;
    write_ack = 100;
    write_ack_bits = 0;

    int bytes_written = reliable_write_packet_header( packet_data, write_sequence, write_ack, write_ack_bits );

    check( bytes_written == RELIABLE_MAX_PACKET_HEADER_BYTES );

    int bytes_read = reliable_read_packet_header( "test_packet_header", packet_data, bytes_written, &read_sequence, &read_ack, &read_ack_bits );

    check( bytes_read == bytes_written );

    check( read_sequence == write_sequence );
    check( read_ack == write_ack );
    check( read_ack_bits == write_ack_bits );

    // rare case. sequence and ack are far apart, significant # of acks are missing

    write_sequence = 10000;
    write_ack = 100;
    write_ack_bits = 0xFEFEFFFE;

    bytes_written = reliable_write_packet_header( packet_data, write_sequence, write_ack, write_ack_bits );

    check( bytes_written == 1 + 2 + 2 + 3 );

    bytes_read = reliable_read_packet_header( "test_packet_header", packet_data, bytes_written, &read_sequence, &read_ack, &read_ack_bits );

    check( bytes_read == bytes_written );

    check( read_sequence == write_sequence );
    check( read_ack == write_ack );
    check( read_ack_bits == write_ack_bits );

    // common case under packet loss. sequence and ack are close together, some acks are missing

    write_sequence = 200;
    write_ack = 100;
    write_ack_bits = 0xFFFEFFFF;

    bytes_written = reliable_write_packet_header( packet_data, write_sequence, write_ack, write_ack_bits );

    check( bytes_written == 1 + 2 + 1 + 1 );

    bytes_read = reliable_read_packet_header( "test_packet_header", packet_data, bytes_written, &read_sequence, &read_ack, &read_ack_bits );

    check( bytes_read == bytes_written );

    check( read_sequence == write_sequence );
    check( read_ack == write_ack );
    check( read_ack_bits == write_ack_bits );

    // ideal case. no packet loss.

    write_sequence = 200;
    write_ack = 100;
    write_ack_bits = 0xFFFFFFFF;

    bytes_written = reliable_write_packet_header( packet_data, write_sequence, write_ack, write_ack_bits );

    check( bytes_written == 1 + 2 + 1 );

    bytes_read = reliable_read_packet_header( "test_packet_header", packet_data, bytes_written, &read_sequence, &read_ack, &read_ack_bits );

    check( bytes_read == bytes_written );

    check( read_sequence == write_sequence );
    check( read_ack == write_ack );
    check( read_ack_bits == write_ack_bits );
}

struct test_context_t
{
    int drop;
    int allow_packets;
    struct reliable_endpoint_t * sender;
    struct reliable_endpoint_t * receiver;
};

void test_default_context( struct test_context_t * context )
{
    memset( context, 0, sizeof( *context ) );
    context->allow_packets = -1;
}

static void test_transmit_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) sequence;

    struct test_context_t * context = (struct test_context_t*) _context;

    if ( context->drop )
    {
        return;
    }

    if ( context->allow_packets >= 0 )
    {
        if ( context->allow_packets == 0 )
        {
            return;
        }

        context->allow_packets--;
    }

    if ( id == 0 )
    {
        reliable_endpoint_receive_packet( context->receiver, packet_data, packet_bytes );
    }
    else if ( id == 1 )
    {
        reliable_endpoint_receive_packet( context->sender, packet_data, packet_bytes );
    }
}

static int test_process_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    struct test_context_t * context = (struct test_context_t*) _context;

    (void) context;
    (void) id;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;

    return 1;
}

#define TEST_ACKS_NUM_ITERATIONS 256

static void test_acks()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    double delta_time = 0.01;

    int i;
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );

        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        time += delta_time;
    }

    uint8_t sender_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( sender_acked_packet, 0, sizeof( sender_acked_packet ) );
    int sender_num_acks;
    RELIABLE_CONST uint16_t * sender_acks = reliable_endpoint_get_acks( context.sender, &sender_num_acks );
    for ( i = 0; i < sender_num_acks; ++i )
    {
        if ( sender_acks[i] < TEST_ACKS_NUM_ITERATIONS )
        {
            sender_acked_packet[sender_acks[i]] = 1;
        }
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( sender_acked_packet[i] == 1 );
    }

    uint8_t receiver_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( receiver_acked_packet, 0, sizeof( receiver_acked_packet ) );
    int receiver_num_acks;
    RELIABLE_CONST uint16_t * receiver_acks = reliable_endpoint_get_acks( context.receiver, &receiver_num_acks );
    for ( i = 0; i < receiver_num_acks; ++i )
    {
        if ( receiver_acks[i] < TEST_ACKS_NUM_ITERATIONS )
            receiver_acked_packet[receiver_acks[i]] = 1;
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( receiver_acked_packet[i] == 1 );
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

static void test_acks_packet_loss()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    const double delta_time = 0.1f;

    int i;
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );

        context.drop = ( i % 2 );

        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        time += delta_time;
    }

    uint8_t sender_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( sender_acked_packet, 0, sizeof( sender_acked_packet ) );
    int sender_num_acks;
    RELIABLE_CONST uint16_t * sender_acks = reliable_endpoint_get_acks( context.sender, &sender_num_acks );
    for ( i = 0; i < sender_num_acks; ++i )
    {
        if ( sender_acks[i] < TEST_ACKS_NUM_ITERATIONS )
        {
            sender_acked_packet[sender_acks[i]] = 1;
        }
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( sender_acked_packet[i] == (i+1) % 2 );
    }

    uint8_t receiver_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( receiver_acked_packet, 0, sizeof( receiver_acked_packet ) );
    int receiver_num_acks;
    RELIABLE_CONST uint16_t * receiver_acks = reliable_endpoint_get_acks( context.receiver, &receiver_num_acks );
    for ( i = 0; i < receiver_num_acks; ++i )
    {
        if ( receiver_acks[i] < TEST_ACKS_NUM_ITERATIONS )
        {
            receiver_acked_packet[receiver_acks[i]] = 1;
        }
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( receiver_acked_packet[i] == (i+1) % 2 );
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

static int test_duplicate_packets_num_processed = 0;

static void test_duplicate_packets_transmit_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) sequence;

    struct test_context_t * context = (struct test_context_t*) _context;

    if ( id == 0 )
    {
        // deliver each packet to the receiver twice, simulating duplication on the network
        reliable_endpoint_receive_packet( context->receiver, packet_data, packet_bytes );
        reliable_endpoint_receive_packet( context->receiver, packet_data, packet_bytes );
    }
}

static int test_duplicate_packets_process_packet_function( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) id;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;

    test_duplicate_packets_num_processed++;

    return 1;
}

#define TEST_DUPLICATE_PACKETS_NUM_ITERATIONS 16

static void test_duplicate_packets()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );

    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_duplicate_packets_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_duplicate_packets_transmit_packet_function;
    receiver_config.process_packet_function = &test_duplicate_packets_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    test_duplicate_packets_num_processed = 0;

    int i;
    for ( i = 0; i < TEST_DUPLICATE_PACKETS_NUM_ITERATIONS; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
    }

    check( test_duplicate_packets_num_processed == TEST_DUPLICATE_PACKETS_NUM_ITERATIONS );

    RELIABLE_CONST uint64_t * receiver_counters = reliable_endpoint_counters( context.receiver );

    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED] == 2 * TEST_DUPLICATE_PACKETS_NUM_ITERATIONS );
    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_DUPLICATE] == TEST_DUPLICATE_PACKETS_NUM_ITERATIONS );

    // duplicate fragments arriving after their packet was delivered must not restart reassembly

    uint16_t fragmented_sequence = reliable_endpoint_next_packet_sequence( context.sender );

    uint8_t large_packet[2048];
    memset( large_packet, 0, sizeof( large_packet ) );
    reliable_endpoint_send_packet( context.sender, large_packet, sizeof( large_packet ) );

    check( test_duplicate_packets_num_processed == TEST_DUPLICATE_PACKETS_NUM_ITERATIONS + 1 );
    check( !reliable_sequence_buffer_exists( context.receiver->fragment_reassembly, fragmented_sequence ) );

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

static uint8_t test_stale_packets_first_packet[64];
static int test_stale_packets_first_packet_bytes = 0;
static int test_stale_packets_num_processed = 0;

static void test_stale_packets_transmit_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    struct test_context_t * context = (struct test_context_t*) _context;

    if ( id == 0 )
    {
        if ( sequence == 0 && test_stale_packets_first_packet_bytes == 0 )
        {
            reliable_assert( packet_bytes <= (int) sizeof( test_stale_packets_first_packet ) );
            memcpy( test_stale_packets_first_packet, packet_data, packet_bytes );
            test_stale_packets_first_packet_bytes = packet_bytes;
        }

        reliable_endpoint_receive_packet( context->receiver, packet_data, packet_bytes );
    }
}

static int test_stale_packets_process_packet_function( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) id;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;

    test_stale_packets_num_processed++;

    return 1;
}

#define TEST_STALE_PACKETS_NUM_ITERATIONS 300

static void test_stale_packets()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );

    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_stale_packets_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_stale_packets_transmit_packet_function;
    receiver_config.process_packet_function = &test_stale_packets_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    test_stale_packets_first_packet_bytes = 0;
    test_stale_packets_num_processed = 0;

    // send enough packets that sequence 0 falls out of the receive window (256 entries)

    int i;
    for ( i = 0; i < TEST_STALE_PACKETS_NUM_ITERATIONS; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
    }

    check( test_stale_packets_num_processed == TEST_STALE_PACKETS_NUM_ITERATIONS );
    check( test_stale_packets_first_packet_bytes > 0 );

    // replaying the first packet must be rejected as stale, not processed

    reliable_endpoint_receive_packet( context.receiver, test_stale_packets_first_packet, test_stale_packets_first_packet_bytes );

    check( test_stale_packets_num_processed == TEST_STALE_PACKETS_NUM_ITERATIONS );

    RELIABLE_CONST uint64_t * receiver_counters = reliable_endpoint_counters( context.receiver );

    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_STALE] == 1 );

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

#define TEST_ACK_BUFFER_OVERFLOW_NUM_PACKETS 32
#define TEST_ACK_BUFFER_OVERFLOW_BUFFER_SIZE 16

static void test_ack_buffer_overflow()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );

    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    // undersized ack buffer on the sender, so a single received packet acking 32 sent packets overflows it

    sender_config.ack_buffer_size = TEST_ACK_BUFFER_OVERFLOW_BUFFER_SIZE;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    int i;
    for ( i = 0; i < TEST_ACK_BUFFER_OVERFLOW_NUM_PACKETS; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
    }

    // one packet back from the receiver acks all 32, but only 16 fit in the ack buffer. the rest are dropped

    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );
    }

    int num_acks;
    reliable_endpoint_get_acks( context.sender, &num_acks );
    check( num_acks == TEST_ACK_BUFFER_OVERFLOW_BUFFER_SIZE );

    RELIABLE_CONST uint64_t * sender_counters = reliable_endpoint_counters( context.sender );
    check( sender_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED] == TEST_ACK_BUFFER_OVERFLOW_BUFFER_SIZE );

    // once the caller clears acks, the dropped acks are reported on the next packet that covers them

    reliable_endpoint_clear_acks( context.sender );

    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );
    }

    reliable_endpoint_get_acks( context.sender, &num_acks );
    check( num_acks == TEST_ACK_BUFFER_OVERFLOW_NUM_PACKETS - TEST_ACK_BUFFER_OVERFLOW_BUFFER_SIZE );
    check( sender_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED] == TEST_ACK_BUFFER_OVERFLOW_NUM_PACKETS );

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

#define TEST_MAX_PACKET_BYTES (4*1024)

static void generate_packet_data_with_size( uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( packet_bytes >= 2 );
    reliable_assert( packet_bytes <= TEST_MAX_PACKET_BYTES );

    packet_data[0] = (uint8_t) ( sequence & 0xFF );
    packet_data[1] = (uint8_t) ( (sequence>>8) & 0xFF );
    int i;
    for ( i = 2; i < packet_bytes; ++i )
    {
        packet_data[i] = (uint8_t) ( ( (int)i + sequence ) % 256 );
    }
}

static int generate_packet_data( uint16_t sequence, uint8_t * packet_data )
{
    int packet_bytes = ( ( (int)sequence * 1023 ) % ( TEST_MAX_PACKET_BYTES - 2 ) ) + 2;
    generate_packet_data_with_size( sequence, packet_data, packet_bytes );
    return packet_bytes;
}

static void validate_packet_data( uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( packet_bytes >= 2 );
    reliable_assert( packet_bytes <= TEST_MAX_PACKET_BYTES );
    uint16_t sequence = 0;
    sequence |= (uint16_t) packet_data[0];
    sequence |= ( (uint16_t) packet_data[1] ) << 8;
    check( packet_bytes == ( ( (int)sequence * 1023 ) % ( TEST_MAX_PACKET_BYTES - 2 ) ) + 2 );
    int i;
    for ( i = 2; i < packet_bytes; ++i )
    {
        check( packet_data[i] == (uint8_t) ( ( (int)i + sequence ) % 256 ) );
    }
}

static int test_process_packet_function_validate( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( packet_data );
    reliable_assert( packet_bytes > 0 );
    reliable_assert( packet_bytes <= TEST_MAX_PACKET_BYTES );

    (void) context;
    (void) id;
    (void) sequence;

    validate_packet_data( packet_data, packet_bytes );

    return 1;
}

static int generate_packet_data_large( uint8_t* packet_data )
{
    int data_bytes = TEST_MAX_PACKET_BYTES - 2;
    reliable_assert( data_bytes >= 2) ;
    reliable_assert( data_bytes <= (1 << 16) );

    packet_data[0] = (uint8_t) (data_bytes & 0xFF);
    packet_data[1] = (uint8_t) ( (data_bytes >> 8) & 0xFF );
    int i;
    for ( i = 2; i < data_bytes; ++i )
    {
        packet_data[i] = (uint8_t) ( i % 256 );
    }
    return data_bytes + 2;
}

static int test_process_packet_function_validate_large( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( packet_data );
    reliable_assert( packet_bytes >= 2 );
    reliable_assert( packet_bytes <= TEST_MAX_PACKET_BYTES );

    (void) context;
    (void) id;
    (void) sequence;

    uint16_t data_bytes = 0;
    data_bytes |= (uint16_t) packet_data[0];
    data_bytes |= ( (uint16_t) packet_data[1] ) << 8;
    check( packet_bytes == data_bytes + 2 );
    int i;
    for ( i = 2; i < data_bytes; ++i )
    {
        check( packet_data[i] == (uint8_t) ( i  % 256 ) );
    }

    return 1;
}

void test_packets()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.fragment_above = 500;
    receiver_config.fragment_above = 500;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function_validate;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function_validate;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    double delta_time = 0.1;

    int i;
    for ( i = 0; i < 16; ++i )
    {
        {
            uint8_t packet_data[TEST_MAX_PACKET_BYTES];
            uint16_t sequence = reliable_endpoint_next_packet_sequence( context.sender );
            int packet_bytes = generate_packet_data( sequence, packet_data );
            reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );
        }

        {
            uint8_t packet_data[TEST_MAX_PACKET_BYTES];
            uint16_t sequence = reliable_endpoint_next_packet_sequence( context.sender );
            int packet_bytes = generate_packet_data( sequence, packet_data );
            reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );
        }

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        reliable_endpoint_clear_acks( context.sender );
        reliable_endpoint_clear_acks( context.receiver );

        time += delta_time;
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

void test_large_packets()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );

    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.max_packet_size = TEST_MAX_PACKET_BYTES;
    receiver_config.max_packet_size = TEST_MAX_PACKET_BYTES;

    sender_config.fragment_above = TEST_MAX_PACKET_BYTES;
    receiver_config.fragment_above = TEST_MAX_PACKET_BYTES;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function_validate_large;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function_validate_large;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    {
        uint8_t packet_data[TEST_MAX_PACKET_BYTES];
        int packet_bytes = generate_packet_data_large( packet_data );
        check( packet_bytes == TEST_MAX_PACKET_BYTES );
        reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );
    }

    reliable_endpoint_update( context.sender, time );
    reliable_endpoint_update( context.receiver, time );

    reliable_endpoint_clear_acks( context.sender );
    reliable_endpoint_clear_acks( context.receiver );

    RELIABLE_CONST uint64_t * receiver_counters = reliable_endpoint_counters( context.receiver );
    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE] == 0 );
    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED] == 1 );

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

void test_sequence_buffer_rollover()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.fragment_above = 500;
    receiver_config.fragment_above = 500;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    uint8_t packet_data[TEST_MAX_PACKET_BYTES] = {0};

    int num_packets_sent = 0;
    int i;
    for (i = 0; i <= 32767; ++i)
    {
        uint8_t packet_data[16] = {0};
        int packet_bytes = sizeof( packet_data ) / sizeof( uint8_t );
        reliable_endpoint_next_packet_sequence( context.sender );
        reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );

        ++num_packets_sent;
    }

    int packet_bytes = sizeof( packet_data ) / sizeof( uint8_t );
    reliable_endpoint_next_packet_sequence( context.sender );
    reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );
    ++num_packets_sent;

    RELIABLE_CONST uint64_t * receiver_counters = reliable_endpoint_counters( context.receiver );

    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED] == (uint16_t) num_packets_sent );
    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID] == 0 );

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

struct test_tracking_allocate_context_t
{
    void* active_allocations[1024];
};

void * test_tracking_allocate_function( void * context, size_t bytes )
{
    struct test_tracking_allocate_context_t* tracking_context = (struct test_tracking_allocate_context_t*)context;
    void * allocation = malloc( bytes );
    int tracking_index;
    for ( tracking_index = 0; tracking_index < (int) ARRAY_LENGTH(tracking_context->active_allocations); ++tracking_index )
    {
        if ( tracking_context->active_allocations[tracking_index] == NULL )
        {
            break;
        }
    }

    reliable_assert(tracking_index < (int) ARRAY_LENGTH(tracking_context->active_allocations));
    tracking_context->active_allocations[tracking_index] = allocation;
    return allocation;
}

void test_tracking_free_function( void * context, void * pointer )
{
    struct test_tracking_allocate_context_t* tracking_context = (struct test_tracking_allocate_context_t*)context;
    int tracking_index;
    for ( tracking_index = 0; tracking_index < (int) ARRAY_LENGTH(tracking_context->active_allocations); ++tracking_index )
    {
        if ( tracking_context->active_allocations[tracking_index] == pointer )
        {
            break;
        }
    }

    reliable_assert( tracking_index < (int) ARRAY_LENGTH(tracking_context->active_allocations) );
    tracking_context->active_allocations[tracking_index] = NULL;
    free( pointer );
}

void test_fragment_cleanup()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );

    struct test_tracking_allocate_context_t tracking_alloc_context;
    memset( &tracking_alloc_context, 0, sizeof( tracking_alloc_context ) );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    receiver_config.allocator_context = &tracking_alloc_context;
    receiver_config.allocate_function = &test_tracking_allocate_function;
    receiver_config.free_function = &test_tracking_free_function;
    receiver_config.fragment_reassembly_buffer_size = 4;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    double delta_time = 0.1;

    int packet_sizes[] = {
        sender_config.fragment_size + sender_config.fragment_size/2,
        10,
        10,
        10,
        10,
    };

    // Make sure we're sending more than receiver_config.fragment_reassembly_buffer_size packets, so the buffer wraps around.
    reliable_assert( (int) ARRAY_LENGTH( packet_sizes ) > receiver_config.fragment_reassembly_buffer_size );

    int i;
    for ( i = 0; i < (int) ARRAY_LENGTH( packet_sizes ); ++i )
    {
        // Only allow one packet per transmit, so that our fragmented packets are only partially
        // delivered.
        context.allow_packets = 1;
        {
            uint8_t packet_data[TEST_MAX_PACKET_BYTES];
            uint16_t sequence = reliable_endpoint_next_packet_sequence( context.sender );
            generate_packet_data_with_size( sequence, packet_data, packet_sizes[i] );
            reliable_endpoint_send_packet( context.sender, packet_data, packet_sizes[i]);
        }

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        reliable_endpoint_clear_acks( context.sender );
        reliable_endpoint_clear_acks( context.receiver );

        time += delta_time;
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
    
    // Make sure that there is no memory that hasn't been freed.
    int tracking_index;
    for ( tracking_index = 0; tracking_index < (int) ARRAY_LENGTH(tracking_alloc_context.active_allocations); ++tracking_index )
    {
        check( tracking_alloc_context.active_allocations[tracking_index] == NULL );
    }
}

static void test_endpoint_reset()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );

    struct test_tracking_allocate_context_t tracking_alloc_context;
    memset( &tracking_alloc_context, 0, sizeof( tracking_alloc_context ) );

    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.fragment_above = 500;
    receiver_config.fragment_above = 500;

    receiver_config.allocator_context = &tracking_alloc_context;
    receiver_config.allocate_function = &test_tracking_allocate_function;
    receiver_config.free_function = &test_tracking_free_function;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    // exchange packets both ways so acks and counters accumulate

    int i;
    for ( i = 0; i < 8; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );

        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        time += 0.01;
    }

    int num_acks;
    reliable_endpoint_get_acks( context.sender, &num_acks );
    check( num_acks > 0 );
    check( reliable_endpoint_counters( context.sender )[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT] > 0 );

    // leave a fragment reassembly in progress on the receiver by delivering only the first fragment of a large packet

    context.allow_packets = 1;
    {
        uint8_t large_packet[1500];
        memset( large_packet, 0, sizeof( large_packet ) );
        reliable_endpoint_send_packet( context.sender, large_packet, sizeof( large_packet ) );
    }
    context.allow_packets = -1;

    reliable_endpoint_reset( context.sender );
    reliable_endpoint_reset( context.receiver );

    check( reliable_endpoint_next_packet_sequence( context.sender ) == 0 );
    check( reliable_endpoint_next_packet_sequence( context.receiver ) == 0 );

    reliable_endpoint_get_acks( context.sender, &num_acks );
    check( num_acks == 0 );

    for ( i = 0; i < RELIABLE_ENDPOINT_NUM_COUNTERS; ++i )
    {
        check( reliable_endpoint_counters( context.sender )[i] == 0 );
        check( reliable_endpoint_counters( context.receiver )[i] == 0 );
    }

    // the endpoints must work normally after reset

    for ( i = 0; i < 8; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );

        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        time += 0.01;
    }

    reliable_endpoint_get_acks( context.sender, &num_acks );
    check( num_acks > 0 );
    check( reliable_endpoint_counters( context.receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED] > 0 );

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );

    // reset must have freed the in-progress reassembly buffer, and destroy must not double-free it

    int tracking_index;
    for ( tracking_index = 0; tracking_index < (int) ARRAY_LENGTH( tracking_alloc_context.active_allocations ); ++tracking_index )
    {
        check( tracking_alloc_context.active_allocations[tracking_index] == NULL );
    }
}

static void test_rtt()
{
    double time = 100.0;
    double delta_time = 0.01;

    struct test_context_t context;
    test_default_context(&context);

    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config(&sender_config);
    reliable_default_config(&receiver_config);

    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create(&sender_config, time);
    context.receiver = reliable_endpoint_create(&receiver_config, time);

    int i;
    for (i = 0; i < 1000; ++i)
    {
        uint8_t dummy_packet[8];
        memset(dummy_packet, 0, sizeof(dummy_packet));

        reliable_endpoint_send_packet(context.sender, dummy_packet, sizeof(dummy_packet));
        reliable_endpoint_send_packet(context.receiver, dummy_packet, sizeof(dummy_packet));

        reliable_endpoint_update(context.sender, time);
        reliable_endpoint_update(context.receiver, time);

        time += delta_time;
    }

    float rtt = reliable_endpoint_rtt(context.sender);
    float rtt_min = reliable_endpoint_rtt_min(context.sender);
    float rtt_max = reliable_endpoint_rtt_max(context.sender);
    float rtt_avg = reliable_endpoint_rtt_avg(context.sender);

    check(rtt == rtt && rtt >= 0); // Check rtt is finite and non-negative
    check(rtt_min >= 0 && rtt_min <= rtt_avg && rtt_avg <= rtt_max);
    check(rtt_max < 1000.0); // Assume RTT is in milliseconds

    reliable_endpoint_destroy(context.sender);
    reliable_endpoint_destroy(context.receiver);
}

// a pair of endpoints wired to each other through test_transmit_packet_function

struct test_pair_t
{
    struct test_context_t context;
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;
};

static void test_pair_configs( struct test_pair_t * pair )
{
    test_default_context( &pair->context );

    reliable_default_config( &pair->sender_config );
    reliable_default_config( &pair->receiver_config );

    reliable_copy_string( pair->sender_config.name, "sender", sizeof( pair->sender_config.name ) );
    pair->sender_config.context = &pair->context;
    pair->sender_config.id = 0;
    pair->sender_config.transmit_packet_function = &test_transmit_packet_function;
    pair->sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( pair->receiver_config.name, "receiver", sizeof( pair->receiver_config.name ) );
    pair->receiver_config.context = &pair->context;
    pair->receiver_config.id = 1;
    pair->receiver_config.transmit_packet_function = &test_transmit_packet_function;
    pair->receiver_config.process_packet_function = &test_process_packet_function;
}

static void test_pair_create( struct test_pair_t * pair, double time )
{
    pair->context.sender = reliable_endpoint_create( &pair->sender_config, time );
    pair->context.receiver = reliable_endpoint_create( &pair->receiver_config, time );
    check( pair->context.sender );
    check( pair->context.receiver );
}

static void test_pair_destroy( struct test_pair_t * pair )
{
    reliable_endpoint_destroy( pair->context.sender );
    reliable_endpoint_destroy( pair->context.receiver );
}

// RL-02: a round trip long enough to reach any fixed sentinel must still be reported

static void test_rtt_min_large()
{
    double time = 100.0;

    struct test_pair_t pair;
    test_pair_configs( &pair );
    test_pair_create( &pair, time );

    uint8_t packet[8];
    memset( packet, 0, sizeof( packet ) );

    reliable_endpoint_send_packet( pair.context.sender, packet, sizeof( packet ) );

    // ten seconds pass before the acknowledgment comes back, so the one rtt sample is 10,000 ms

    time += 10.0;

    reliable_endpoint_update( pair.context.sender, time );
    reliable_endpoint_update( pair.context.receiver, time );

    reliable_endpoint_send_packet( pair.context.receiver, packet, sizeof( packet ) );

    reliable_endpoint_update( pair.context.sender, time );

    int num_acks;
    reliable_endpoint_get_acks( pair.context.sender, &num_acks );
    check( num_acks == 1 );

    check( reliable_endpoint_rtt_min( pair.context.sender ) == 10000.0f );
    check( reliable_endpoint_rtt_max( pair.context.sender ) == 10000.0f );
    check( reliable_endpoint_rtt_avg( pair.context.sender ) == 10000.0f );
    check( reliable_endpoint_jitter_avg_vs_min_rtt( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_jitter_max_vs_min_rtt( pair.context.sender ) == 0.0f );

    // and an endpoint with no samples at all still reports zero

    check( reliable_endpoint_rtt_min( pair.context.receiver ) == 0.0f );

    test_pair_destroy( &pair );
}

// RL-03: reset clears every field a getter can return, the rtt history included

static void test_endpoint_reset_clears_stats()
{
    double time = 100.0;

    struct test_pair_t pair;
    test_pair_configs( &pair );
    test_pair_create( &pair, time );

    // enough packets that the bandwidth window, which samples half the sent packets buffer,
    // lands on packets that were actually sent

    int i;
    for ( i = 0; i < 300; ++i )
    {
        uint8_t packet[64];
        memset( packet, 0, sizeof( packet ) );

        // the reply comes back a step later, so the acknowledgment carries a real round trip

        reliable_endpoint_send_packet( pair.context.sender, packet, sizeof( packet ) );

        time += 0.1;
        reliable_endpoint_update( pair.context.sender, time );
        reliable_endpoint_update( pair.context.receiver, time );

        reliable_endpoint_send_packet( pair.context.receiver, packet, sizeof( packet ) );

        time += 0.1;
        reliable_endpoint_update( pair.context.sender, time );
        reliable_endpoint_update( pair.context.receiver, time );

        reliable_endpoint_clear_acks( pair.context.sender );
        reliable_endpoint_clear_acks( pair.context.receiver );
    }

    check( reliable_endpoint_rtt( pair.context.sender ) > 0.0f );
    check( reliable_endpoint_rtt_min( pair.context.sender ) > 0.0f );
    check( reliable_endpoint_rtt_max( pair.context.sender ) > 0.0f );
    check( reliable_endpoint_rtt_avg( pair.context.sender ) > 0.0f );

    float sent_bandwidth, received_bandwidth, acked_bandwidth;
    reliable_endpoint_bandwidth( pair.context.sender, &sent_bandwidth, &received_bandwidth, &acked_bandwidth );
    check( sent_bandwidth > 0.0f );
    check( received_bandwidth > 0.0f );
    check( acked_bandwidth > 0.0f );

    reliable_endpoint_reset( pair.context.sender );

    check( reliable_endpoint_rtt( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_rtt_min( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_rtt_max( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_rtt_avg( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_jitter_avg_vs_min_rtt( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_jitter_max_vs_min_rtt( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_jitter_stddev_vs_avg_rtt( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_packet_loss( pair.context.sender ) == 0.0f );

    reliable_endpoint_bandwidth( pair.context.sender, &sent_bandwidth, &received_bandwidth, &acked_bandwidth );
    check( sent_bandwidth == 0.0f );
    check( received_bandwidth == 0.0f );
    check( acked_bandwidth == 0.0f );

    // the rtt history is part of the state, so recomputing the statistics after a reset must
    // not resurrect the samples that were in it

    time += 0.1;
    reliable_endpoint_update( pair.context.sender, time );

    check( reliable_endpoint_rtt_min( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_rtt_max( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_rtt_avg( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_jitter_avg_vs_min_rtt( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_jitter_max_vs_min_rtt( pair.context.sender ) == 0.0f );
    check( reliable_endpoint_jitter_stddev_vs_avg_rtt( pair.context.sender ) == 0.0f );

    test_pair_destroy( &pair );
}

// RL-06: a config the library cannot honor is refused instead of asserted

static void test_endpoint_create_invalid_config()
{
    struct reliable_config_t valid;

    reliable_default_config( &valid );
    valid.transmit_packet_function = &test_transmit_packet_function;
    valid.process_packet_function = &test_process_packet_function;

    struct reliable_endpoint_t * endpoint = reliable_endpoint_create( &valid, 0.0 );
    check( endpoint );
    reliable_endpoint_destroy( endpoint );

    check( reliable_endpoint_create( NULL, 0.0 ) == NULL );

    struct reliable_config_t config;

    // the two relationships between fields

    config = valid;
    config.fragment_above = config.max_packet_size + 1;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid;
    config.max_fragments = 4;
    config.fragment_size = 1024;
    config.max_packet_size = 4 * 1024 + 1;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    // exactly covering is allowed, one fragment short is not

    config = valid;
    config.max_fragments = 4;
    config.fragment_size = 1024;
    config.max_packet_size = 4 * 1024;
    config.fragment_above = 1024;
    endpoint = reliable_endpoint_create( &config, 0.0 );
    check( endpoint );
    reliable_endpoint_destroy( endpoint );

    // ranges

    config = valid; config.max_packet_size = 0;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.fragment_above = 0;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.fragment_size = 0;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.max_fragments = 0;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.max_fragments = 257;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.ack_buffer_size = 0;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.sent_packets_buffer_size = -1;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.received_packets_buffer_size = 0;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.fragment_reassembly_buffer_size = 0;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.rtt_history_size = 0;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.transmit_packet_function = NULL;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    config = valid; config.process_packet_function = NULL;
    check( reliable_endpoint_create( &config, 0.0 ) == NULL );

    // the size arithmetic behind every one of those buffers

    size_t bytes = 0;

    check( reliable_checked_size( 1, 4, &bytes ) );
    check( bytes == 4 );

    check( reliable_checked_size( 256, sizeof( uint16_t ), &bytes ) );
    check( bytes == 512 );

    check( !reliable_checked_size( 0, 4, &bytes ) );
    check( !reliable_checked_size( -1, 4, &bytes ) );
    check( !reliable_checked_size( 4, 0, &bytes ) );

    // a product that does not fit in a size_t is refused rather than wrapped

    check( !reliable_checked_size( 4, SIZE_MAX / 2, &bytes ) );
    check( !reliable_checked_size( INT_MAX, SIZE_MAX / 3, &bytes ) );
}

// RL-06: every allocation failure returns NULL, in every build, having freed what it took

#define TEST_FAILING_ALLOCATOR_SLOTS 64

struct test_failing_allocate_context_t
{
    int fail_on;
    int num_allocations;
    void * active_allocations[TEST_FAILING_ALLOCATOR_SLOTS];
};

static void * test_failing_allocate_function( void * context, size_t bytes )
{
    struct test_failing_allocate_context_t * failing_context = (struct test_failing_allocate_context_t*) context;

    if ( failing_context->num_allocations++ == failing_context->fail_on )
    {
        return NULL;
    }

    void * allocation = malloc( bytes );

    int i;
    for ( i = 0; i < TEST_FAILING_ALLOCATOR_SLOTS; ++i )
    {
        if ( failing_context->active_allocations[i] == NULL )
        {
            failing_context->active_allocations[i] = allocation;
            return allocation;
        }
    }

    check( 0 );
    return allocation;
}

static void test_failing_free_function( void * context, void * pointer )
{
    struct test_failing_allocate_context_t * failing_context = (struct test_failing_allocate_context_t*) context;

    int i;
    for ( i = 0; i < TEST_FAILING_ALLOCATOR_SLOTS; ++i )
    {
        if ( failing_context->active_allocations[i] == pointer )
        {
            failing_context->active_allocations[i] = NULL;
            free( pointer );
            return;
        }
    }

    check( 0 );
}

static void test_endpoint_create_allocation_failure()
{
    struct test_failing_allocate_context_t allocator;

    struct reliable_config_t config;
    reliable_default_config( &config );
    config.transmit_packet_function = &test_transmit_packet_function;
    config.process_packet_function = &test_process_packet_function;
    config.allocator_context = &allocator;
    config.allocate_function = &test_failing_allocate_function;
    config.free_function = &test_failing_free_function;

    // how many allocations a successful create makes

    memset( &allocator, 0, sizeof( allocator ) );
    allocator.fail_on = -1;

    struct reliable_endpoint_t * endpoint = reliable_endpoint_create( &config, 0.0 );
    check( endpoint );

    const int num_allocations = allocator.num_allocations;
    check( num_allocations > 1 );

    reliable_endpoint_destroy( endpoint );

    int i;
    for ( i = 0; i < TEST_FAILING_ALLOCATOR_SLOTS; ++i )
    {
        check( allocator.active_allocations[i] == NULL );
    }

    // fail each one in turn

    int fail_on;
    for ( fail_on = 0; fail_on < num_allocations; ++fail_on )
    {
        memset( &allocator, 0, sizeof( allocator ) );
        allocator.fail_on = fail_on;

        check( reliable_endpoint_create( &config, 0.0 ) == NULL );

        for ( i = 0; i < TEST_FAILING_ALLOCATOR_SLOTS; ++i )
        {
            check( allocator.active_allocations[i] == NULL );
        }
    }
}

// RL-07: the sequence number crosses 65535 to 0

static uint8_t test_wrap_acked[65536];

static void test_sequence_wrap()
{
    double time = 100.0;

    struct test_pair_t pair;
    test_pair_configs( &pair );
    test_pair_create( &pair, time );

    memset( test_wrap_acked, 0, sizeof( test_wrap_acked ) );

    const int num_iterations = 65536 + 64;

    int i;
    for ( i = 0; i < num_iterations; ++i )
    {
        uint8_t packet[8];
        memset( packet, 0, sizeof( packet ) );

        reliable_endpoint_send_packet( pair.context.sender, packet, sizeof( packet ) );
        reliable_endpoint_send_packet( pair.context.receiver, packet, sizeof( packet ) );

        int num_acks;
        RELIABLE_CONST uint16_t * acks = reliable_endpoint_get_acks( pair.context.sender, &num_acks );

        int j;
        for ( j = 0; j < num_acks; ++j )
        {
            test_wrap_acked[acks[j]] = 1;
        }

        reliable_endpoint_clear_acks( pair.context.sender );
        reliable_endpoint_clear_acks( pair.context.receiver );
    }

    check( reliable_endpoint_next_packet_sequence( pair.context.sender ) == (uint16_t) num_iterations );

    // the sequences either side of the wrap were acked like any others

    check( test_wrap_acked[65533] );
    check( test_wrap_acked[65534] );
    check( test_wrap_acked[65535] );
    check( test_wrap_acked[0] );
    check( test_wrap_acked[1] );
    check( test_wrap_acked[2] );

    for ( i = 0; i < 65536; ++i )
    {
        check( test_wrap_acked[i] );
    }

    check( reliable_endpoint_counters( pair.context.receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED] == (uint64_t) num_iterations );

    test_pair_destroy( &pair );
}

// RL-07: fragment counts at both ends of the range, and a payload that is an exact multiple
// of the fragment size

struct test_fragment_context_t
{
    struct test_context_t base;
    int num_processed;
    int processed_bytes;
    uint8_t processed[64 * 1024];
};

static int test_fragment_process_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) id;
    (void) sequence;

    struct test_fragment_context_t * context = (struct test_fragment_context_t*) _context;

    check( packet_bytes <= (int) sizeof( context->processed ) );

    context->num_processed++;
    context->processed_bytes = packet_bytes;
    memcpy( context->processed, packet_data, packet_bytes );

    return 1;
}

static void test_fragment_case( int fragment_size, int max_fragments, int packet_bytes, int expected_fragments )
{
    struct test_fragment_context_t context;
    memset( &context, 0, sizeof( context ) );
    context.base.allow_packets = -1;

    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.fragment_size = fragment_size;
    sender_config.max_fragments = max_fragments;
    sender_config.max_packet_size = fragment_size * max_fragments;
    sender_config.fragment_above = fragment_size;
    receiver_config = sender_config;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_fragment_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_fragment_process_packet_function;

    context.base.sender = reliable_endpoint_create( &sender_config, 100.0 );
    context.base.receiver = reliable_endpoint_create( &receiver_config, 100.0 );
    check( context.base.sender );
    check( context.base.receiver );

    uint8_t * packet = (uint8_t*) malloc( packet_bytes );
    check( packet );

    int i;
    for ( i = 0; i < packet_bytes; ++i )
    {
        packet[i] = (uint8_t) ( ( i * 31 ) + 7 );
    }

    reliable_endpoint_send_packet( context.base.sender, packet, packet_bytes );

    check( context.num_processed == 1 );
    check( context.processed_bytes == packet_bytes );
    check( memcmp( context.processed, packet, packet_bytes ) == 0 );

    const uint64_t fragments_sent = reliable_endpoint_counters( context.base.sender )[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_SENT];
    const uint64_t fragments_received = reliable_endpoint_counters( context.base.receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED];

    if ( expected_fragments == 0 )
    {
        // below the threshold: sent whole, no fragments at all
        check( fragments_sent == 0 );
        check( fragments_received == 0 );
    }
    else
    {
        check( fragments_sent == (uint64_t) expected_fragments );
        check( fragments_received == (uint64_t) expected_fragments );
    }

    free( packet );

    reliable_endpoint_destroy( context.base.sender );
    reliable_endpoint_destroy( context.base.receiver );
}

static void test_fragment_counts()
{
    // an exact multiple of the fragment size, where the last fragment is full rather than a remainder
    test_fragment_case( 512, 16, 512 * 4, 4 );

    // the largest packet the config allows, again an exact multiple
    test_fragment_case( 512, 16, 512 * 16, 16 );

    // one fragment: above the threshold by a single byte
    test_fragment_case( 512, 16, 513, 2 );
    test_fragment_case( 64, 256, 65, 2 );

    // 256 fragments, the maximum the wire format can express
    test_fragment_case( 64, 256, 64 * 256, 256 );
    test_fragment_case( 64, 256, 64 * 255 + 1, 256 );

    // at or below the threshold the packet is not fragmented
    test_fragment_case( 512, 16, 512, 0 );
    test_fragment_case( 512, 16, 1, 0 );
}

// RL-07: truncated packets and fragments are rejected rather than acted on

struct test_truncation_context_t
{
    struct test_context_t base;
    int num_processed;
    int num_captured;
    int captured_bytes[300];
    uint8_t captured[300][1024];
};

static void test_truncation_transmit_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) id;
    (void) sequence;

    struct test_truncation_context_t * context = (struct test_truncation_context_t*) _context;

    if ( context->num_captured < (int) ARRAY_LENGTH( context->captured_bytes ) && packet_bytes <= (int) sizeof( context->captured[0] ) )
    {
        memcpy( context->captured[context->num_captured], packet_data, packet_bytes );
        context->captured_bytes[context->num_captured] = packet_bytes;
        context->num_captured++;
    }
}

static int test_truncation_process_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) id;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;

    struct test_truncation_context_t * context = (struct test_truncation_context_t*) _context;
    context->num_processed++;
    return 1;
}

static void test_truncated_packets()
{
    struct test_truncation_context_t context;
    memset( &context, 0, sizeof( context ) );
    context.base.allow_packets = -1;

    struct reliable_config_t config;
    reliable_default_config( &config );
    config.fragment_size = 256;
    config.max_fragments = 16;
    config.max_packet_size = 256 * 16;
    config.fragment_above = 256;
    config.context = &context;
    config.id = 0;
    config.transmit_packet_function = &test_truncation_transmit_packet_function;
    config.process_packet_function = &test_truncation_process_packet_function;

    struct reliable_endpoint_t * sender = reliable_endpoint_create( &config, 100.0 );
    struct reliable_endpoint_t * receiver = reliable_endpoint_create( &config, 100.0 );
    check( sender );
    check( receiver );

    // an unfragmented packet, captured on the wire

    uint8_t packet[200];
    memset( packet, 0xAB, sizeof( packet ) );
    reliable_endpoint_send_packet( sender, packet, sizeof( packet ) );
    check( context.num_captured == 1 );

    const int whole_bytes = context.captured_bytes[0];

    // every truncation of it shorter than the header is rejected, and none is processed

    int truncated_bytes;
    for ( truncated_bytes = 1; truncated_bytes < 4; ++truncated_bytes )
    {
        reliable_endpoint_receive_packet( receiver, context.captured[0], truncated_bytes );
    }

    check( reliable_endpoint_counters( receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_INVALID] == 3 );
    check( context.num_processed == 0 );

    // the whole packet still arrives

    reliable_endpoint_receive_packet( receiver, context.captured[0], whole_bytes );
    check( context.num_processed == 1 );

    // now a fragmented packet

    context.num_captured = 0;

    uint8_t large_packet[1024];
    memset( large_packet, 0xCD, sizeof( large_packet ) );
    reliable_endpoint_send_packet( sender, large_packet, sizeof( large_packet ) );
    check( context.num_captured == 4 );

    // a fragment truncated inside its header is rejected

    for ( truncated_bytes = 1; truncated_bytes < RELIABLE_FRAGMENT_HEADER_BYTES; ++truncated_bytes )
    {
        reliable_endpoint_receive_packet( receiver, context.captured[1], truncated_bytes );
    }

    check( reliable_endpoint_counters( receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED] == 0 );
    check( reliable_endpoint_counters( receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID] == RELIABLE_FRAGMENT_HEADER_BYTES - 1 );

    // a fragment that is not the last one must carry exactly fragment_size bytes, so a
    // truncated body is rejected too

    reliable_endpoint_receive_packet( receiver, context.captured[1], context.captured_bytes[1] - 1 );
    check( reliable_endpoint_counters( receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED] == 0 );
    check( reliable_endpoint_counters( receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID] == RELIABLE_FRAGMENT_HEADER_BYTES );
    check( context.num_processed == 1 );

    // the intact fragments reassemble

    int i;
    for ( i = 0; i < context.num_captured; ++i )
    {
        reliable_endpoint_receive_packet( receiver, context.captured[i], context.captured_bytes[i] );
    }

    check( reliable_endpoint_counters( receiver )[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED] == 4 );
    check( context.num_processed == 2 );

    reliable_endpoint_destroy( sender );
    reliable_endpoint_destroy( receiver );
}

// every function reliable.h declares is called at least once by the suite. this test names
// them all in one place so a new entry point cannot be added without being exercised

static int test_surface_printf_calls = 0;

static int test_surface_printf_function( RELIABLE_CONST char * format, ... )
{
    (void) format;
    test_surface_printf_calls++;
    return 0;
}

static void test_public_api_surface()
{
    check( reliable_init() == RELIABLE_OK );

    reliable_set_assert_function( reliable_assert_function );

    reliable_set_printf_function( &test_surface_printf_function );
    reliable_log_level( RELIABLE_LOG_LEVEL_NONE );

    struct test_pair_t pair;
    test_pair_configs( &pair );
    test_pair_create( &pair, 100.0 );

    check( reliable_endpoint_next_packet_sequence( pair.context.sender ) == 0 );

    uint8_t packet[64];
    memset( packet, 0, sizeof( packet ) );

    reliable_endpoint_send_packet( pair.context.sender, packet, sizeof( packet ) );
    reliable_endpoint_send_packet( pair.context.receiver, packet, sizeof( packet ) );

    reliable_endpoint_receive_packet( pair.context.sender, packet, sizeof( packet ) );

    reliable_endpoint_update( pair.context.sender, 100.1 );

    int num_acks = 0;
    RELIABLE_CONST uint16_t * acks = reliable_endpoint_get_acks( pair.context.sender, &num_acks );
    check( acks );
    check( num_acks == 1 );
    reliable_endpoint_clear_acks( pair.context.sender );
    reliable_endpoint_get_acks( pair.context.sender, &num_acks );
    check( num_acks == 0 );

    ( void ) reliable_endpoint_rtt( pair.context.sender );
    ( void ) reliable_endpoint_rtt_min( pair.context.sender );
    ( void ) reliable_endpoint_rtt_max( pair.context.sender );
    ( void ) reliable_endpoint_rtt_avg( pair.context.sender );
    ( void ) reliable_endpoint_jitter_avg_vs_min_rtt( pair.context.sender );
    ( void ) reliable_endpoint_jitter_max_vs_min_rtt( pair.context.sender );
    ( void ) reliable_endpoint_jitter_stddev_vs_avg_rtt( pair.context.sender );
    ( void ) reliable_endpoint_packet_loss( pair.context.sender );

    float sent_bandwidth, received_bandwidth, acked_bandwidth;
    reliable_endpoint_bandwidth( pair.context.sender, &sent_bandwidth, &received_bandwidth, &acked_bandwidth );

    check( reliable_endpoint_counters( pair.context.sender )[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT] == 1 );

    reliable_endpoint_reset( pair.context.sender );
    check( reliable_endpoint_next_packet_sequence( pair.context.sender ) == 0 );

    void * owned = malloc( 32 );
    check( owned );
    reliable_endpoint_free_packet( pair.context.sender, owned );

    char name[8];
    reliable_copy_string( name, "abcdefghij", sizeof( name ) );
    check( strcmp( name, "abcdefg" ) == 0 );

    struct reliable_config_t defaults;
    reliable_default_config( &defaults );
    check( defaults.max_packet_size > 0 );

    test_pair_destroy( &pair );

    reliable_set_printf_function( ( int (*)( RELIABLE_CONST char *, ... ) ) printf );

    reliable_term();
    check( reliable_init() == RELIABLE_OK );
}

#define RUN_TEST( test_function )                                           \
    do                                                                      \
    {                                                                       \
        printf( #test_function "\n" );                                      \
        test_function();                                                    \
    }                                                                       \
    while (0)

void reliable_test()
{
    // while ( 1 )
    {
        RUN_TEST( test_endian );
        RUN_TEST( test_sequence_buffer );
        RUN_TEST( test_generate_ack_bits );
        RUN_TEST( test_packet_header );
        RUN_TEST( test_acks );
        RUN_TEST( test_acks_packet_loss );
        RUN_TEST( test_duplicate_packets );
        RUN_TEST( test_stale_packets );
        RUN_TEST( test_ack_buffer_overflow );
        RUN_TEST( test_packets );
        RUN_TEST( test_large_packets );
        RUN_TEST( test_sequence_buffer_rollover );
        RUN_TEST( test_fragment_cleanup );
        RUN_TEST( test_rtt );
        RUN_TEST( test_endpoint_reset );
        RUN_TEST( test_endpoint_reset_clears_stats );
        RUN_TEST( test_rtt_min_large );
        RUN_TEST( test_endpoint_create_invalid_config );
        RUN_TEST( test_endpoint_create_allocation_failure );
        RUN_TEST( test_sequence_wrap );
        RUN_TEST( test_fragment_counts );
        RUN_TEST( test_truncated_packets );
        RUN_TEST( test_public_api_surface );
    }
}

#endif // #if RELIABLE_ENABLE_TESTS
