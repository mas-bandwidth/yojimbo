/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#ifndef YOJIMBO_PLATFORM_H
#define YOJIMBO_PLATFORM_H

#include "yojimbo_config.h"

/** @file */

namespace yojimbo
{
    /**
        Sleep for approximately this number of seconds.

        @param time number of seconds to sleep for.
     */

    void platform_sleep( double time );

    /**
        Get a high precision time value in seconds since the application has started.

        IMPORTANT: Please store time in doubles so you retain sufficient precision as time increases.

        @returns The current time value in seconds since the program started.
     */

    double platform_time();
}

#endif // #ifndef YOJIMBO_PLATFORM_H
