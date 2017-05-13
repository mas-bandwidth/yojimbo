/*
    reliable.io reference implementation

    Copyright © 2017, The Network Protocol Company, Inc.

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
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <signal.h>
#include <inttypes.h>

#define MAX_PACKET_BYTES (16*1024)

static volatile int quit = 0;

void interrupt_handler( int signal )
{
    (void) signal;
    quit = 1;
}

int random_int( int a, int b )
{
    assert( a < b );
    int result = a + rand() % ( b - a + 1 );
    assert( result >= a );
    assert( result <= b );
    return result;
}

struct test_context_t
{
    struct reliable_endpoint_t * client;
    struct reliable_endpoint_t * server;
};

struct test_context_t context;

void test_transmit_packet_function( void * _context, int index, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) sequence;

    struct test_context_t * context = (struct test_context_t*) _context;

    if ( random_int(0,100) < 5 )
        return;

    if ( index == 0 )
    {
        reliable_endpoint_receive_packet( context->server, packet_data, packet_bytes );
    }
    else if ( index == 1 )
    {
        reliable_endpoint_receive_packet( context->client, packet_data, packet_bytes );
    }
}

int generate_packet_data( uint16_t sequence, uint8_t * packet_data )
{
    int packet_bytes = ( ( (int)sequence * 1023 ) % ( MAX_PACKET_BYTES - 2 ) ) + 2;
    assert( packet_bytes >= 2 );
    assert( packet_bytes <= MAX_PACKET_BYTES );
    packet_data[0] = (uint8_t) ( sequence & 0xFF );
    packet_data[1] = (uint8_t) ( (sequence>>8) & 0xFF );
    int i;
    for ( i = 2; i < packet_bytes; ++i )
    {
        packet_data[i] = (uint8_t) ( ( (int)i + sequence ) % 256 );
    }
    return packet_bytes;
}

static void check_handler( char * condition, 
                           char * function,
                           char * file,
                           int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifndef NDEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define check( condition )                                                                      \
do                                                                                              \
{                                                                                               \
    if ( !(condition) )                                                                         \
    {                                                                                           \
        check_handler( #condition, (char*) __FUNCTION__, (char*) __FILE__, __LINE__ );          \
    }                                                                                           \
} while(0)

void check_packet_data( uint8_t * packet_data, int packet_bytes )
{
    assert( packet_bytes >= 2 );
    assert( packet_bytes <= MAX_PACKET_BYTES );
    uint16_t sequence = 0;
    sequence |= (uint16_t) packet_data[0];
    sequence |= ( (uint16_t) packet_data[1] ) << 8;
    check( packet_bytes == ( ( (int)sequence * 1023 ) % ( MAX_PACKET_BYTES - 2 ) ) + 2 );
    int i;
    for ( i = 2; i < packet_bytes; ++i )
    {
        check( packet_data[i] == (uint8_t) ( ( (int)i + sequence ) % 256 ) );
    }
}

int test_process_packet_function( void * context, int index, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    assert( packet_data );
    assert( packet_bytes > 0 );
    assert( packet_bytes <= MAX_PACKET_BYTES );

    (void) context;
    (void) index;
    (void) sequence;

    check_packet_data( packet_data, packet_bytes );

    return 1;
}

void soak_initialize()
{
    printf( "initializing\n" );

    reliable_init();

    reliable_log_level( RELIABLE_LOG_LEVEL_DEBUG );

    memset( &context, 0, sizeof( context ) );
    
    struct reliable_config_t client_config;
    struct reliable_config_t server_config;

    reliable_default_config( &client_config );
    reliable_default_config( &server_config );

    client_config.fragment_above = 500;
    server_config.fragment_above = 500;

    strcpy( client_config.name, "client" );
    client_config.context = &context;
    client_config.index = 0;
    client_config.transmit_packet_function = &test_transmit_packet_function;
    client_config.process_packet_function = &test_process_packet_function;

    strcpy( server_config.name, "server" );
    server_config.context = &context;
    client_config.index = 1;
    server_config.transmit_packet_function = &test_transmit_packet_function;
    server_config.process_packet_function = &test_process_packet_function;

    context.client = reliable_endpoint_create( &client_config );
    context.server = reliable_endpoint_create( &server_config );
}

void soak_shutdown()
{
    printf( "shutdown\n" );

    reliable_endpoint_destroy( context.client );
    reliable_endpoint_destroy( context.server );

    reliable_term();
}

void soak_iteration( double time )
{
    (void) time;

    uint8_t packet_data[MAX_PACKET_BYTES];
    memset( packet_data, 0, MAX_PACKET_BYTES );
    int packet_bytes;
    uint16_t sequence;

    sequence = reliable_endpoint_next_packet_sequence( context.client );
    packet_bytes = generate_packet_data( sequence, packet_data );
    reliable_endpoint_send_packet( context.client, packet_data, packet_bytes );

    sequence = reliable_endpoint_next_packet_sequence( context.server );
    packet_bytes = generate_packet_data( sequence, packet_data );
    reliable_endpoint_send_packet( context.server, packet_data, packet_bytes );

    reliable_endpoint_update( context.client );
    reliable_endpoint_update( context.server );

    reliable_endpoint_clear_acks( context.client );
    reliable_endpoint_clear_acks( context.server );
}

int main( int argc, char ** argv )
{
    int num_iterations = -1;

    if ( argc == 2 )
        num_iterations = atoi( argv[1] );

    soak_initialize();

    signal( SIGINT, interrupt_handler );

    double time = 0.0;
    double delta_time = 0.1;

    if ( num_iterations > 0 )
    {
        int i;
        for ( i = 0; i < num_iterations; ++i )
        {
            if ( quit )
                break;

            soak_iteration( time );

            time += delta_time;
        }
    }
    else
    {
        while ( !quit )
        {
            soak_iteration( time );

            time += delta_time;
        }
    }

    soak_shutdown();
    
    return 0;
}
