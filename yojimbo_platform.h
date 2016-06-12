/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.
*/

#ifndef YOJIMBO_PLATFORM_H
#define YOJIMBO_PLATFORM_H

#include "yojimbo_config.h"

namespace yojimbo
{
    void platform_sleep( double time );

    double platform_time();
}

#endif // #ifndef YOJIMBO_PLATFORM_H
