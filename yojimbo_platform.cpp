/*
    Yojimbo Client/Server Network Protocol Library.

    Copyright © 2016, The Network Protocol Company, Inc.

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

#include "yojimbo_config.h"
#include "yojimbo_platform.h"
#include <assert.h>

#if __APPLE__

// ===============================
//              MacOS
// ===============================

#include <unistd.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

namespace yojimbo
{
    void platform_sleep( double time )
    {
        usleep( (int) ( time * 1000000 ) );
    }

    double platform_time()
    {
        static uint64_t start = 0;

        static mach_timebase_info_data_t timebase_info;

        if ( start == 0 )
        {
            mach_timebase_info( &timebase_info );
            start = mach_absolute_time();
            return 0.0;
        }

        uint64_t current = mach_absolute_time();

        assert( current > start );

        return ( double( current - start ) * double( timebase_info.numer ) / double( timebase_info.denom ) ) / 1000000000.0;
    }
}

#elif __linux

// ===============================
//              Linux
// ===============================

#include <unistd.h>
#include <time.h>

namespace yojimbo
{
    void platform_sleep( double time )
    {
        usleep( (int) ( time * 1000000 ) );
    }

    double platform_time()
    {
        static double start = -1;

        if ( start == -1 )
        {
            timespec ts;
            clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
            start = ts.tv_sec + double( ts.tv_nsec ) / 1000000000.0;
            return 0.0;
        }

        timespec ts;
        clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
        double current = ts.tv_sec + double( ts.tv_nsec ) / 1000000000.0;
        return current - start;
    }
}

#elif defined(_WIN32)

// ===============================
//             Wandows
// ===============================

#define NOMINMAX
#include <windows.h>

namespace yojimbo
{
    void platform_sleep( double time )
    {
        const int milliseconds = time * 1000;
        Sleep( milliseconds );
    }

    static bool timer_initialized = false;
    static LARGE_INTEGER timer_frequency;
    static LARGE_INTEGER timer_start;

    double platform_time()
    {
        if ( !timer_initialized )
        {
            QueryPerformanceFrequency( &timer_frequency );
            QueryPerformanceCounter( &timer_start );
            timer_initialized = true;
        }
        LARGE_INTEGER now;
        QueryPerformanceCounter( &now );
        return double( now.QuadPart - timer_start.QuadPart ) / double( timer_frequency.QuadPart );
    }
}

#else

#error unsupported platform!

#endif
