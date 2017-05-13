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

#include <reliable.h>
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

struct reliable_endpoint_t * endpoint;

void test_transmit_packet_function( void * context, int index, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) index;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;
}

int test_process_packet_function( void * context, int index, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) context;
    (void) index;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;
    return 1;
}

void fuzz_initialize()
{
    reliable_init();

    struct reliable_config_t config;
    
    reliable_default_config( &config );

    config.index = 0;
    config.transmit_packet_function = &test_transmit_packet_function;
    config.process_packet_function = &test_process_packet_function;

    endpoint = reliable_endpoint_create( &config );
}

void fuzz_shutdown()
{
    printf( "shutdown\n" );

    reliable_endpoint_destroy( endpoint );

    reliable_term();
}

void fuzz_iteration( double time )
{
    (void) time;

    printf( "." );
    fflush( stdout );

    uint8_t packet_data[MAX_PACKET_BYTES];
    int packet_bytes = random_int( 1, MAX_PACKET_BYTES );
    int i;
    for ( i = 0; i < packet_bytes; ++i )
    {
        packet_data[i] = rand() % 256;
    }

    reliable_endpoint_receive_packet( endpoint, packet_data, packet_bytes );

    reliable_endpoint_update( endpoint );

    reliable_endpoint_clear_acks( endpoint );
}

int main( int argc, char ** argv )
{
    printf( "[fuzz]\n" );

    int num_iterations = -1;

    if ( argc == 2 )
        num_iterations = atoi( argv[1] );

    fuzz_initialize();

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

            fuzz_iteration( time );

            time += delta_time;
        }
    }
    else
    {
        while ( !quit )
        {
            fuzz_iteration( time );

            time += delta_time;
        }
    }

    fuzz_shutdown();
    
    return 0;
}
