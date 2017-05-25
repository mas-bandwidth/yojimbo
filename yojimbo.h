/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#ifndef YOJIMBO_H
#define YOJIMBO_H

#include "yojimbo_config.h"
#include "yojimbo_message.h"
#include "yojimbo_matcher.h"
#include "yojimbo_utility.h"
#include "yojimbo_platform.h"
#include "yojimbo_allocator.h"
#include "yojimbo_client.h"
#include "yojimbo_server.h"
#include "yojimbo_message.h"
#include "yojimbo_connection.h"

/** @file */

/**
    Initialize the yojimbo library.

    Call this before calling any other library functions.

    @returns True if the library was successfully initialized, false otherwise.
 */

bool InitializeYojimbo();

/**
    Shutdown the yojimbo library.

    Call this after you finish using the library and it will run some checks for you (for example, checking for memory leaks in debug build).
 */

void ShutdownYojimbo();

#endif // #ifndef YOJIMBO_H
