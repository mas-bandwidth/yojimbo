/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#ifndef YOJIMBO_PLATFORM_H
#define YOJIMBO_PLATFORM_H

#include "yojimbo_config.h"

/** @file */

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
    Swaps two values.

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
    Get a high precision time value in seconds since the application has started.

    IMPORTANT: Please store time in doubles so you retain sufficient precision as time increases.

    @returns The current time value in seconds since the program started.
 */

double yojimbo_time();

#define YOJIMBO_LOG_LEVEL_NONE      0
#define YOJIMBO_LOG_LEVEL_ERROR     1
#define YOJIMBO_LOG_LEVEL_INFO      2
#define YOJIMBO_LOG_LEVEL_DEBUG     3

/**
    Set the yojimbo log level.

    Valid log levels are:

        YOJIMBO_LOG_LEVEL_NONE
        YOJIMBO_LOG_LEVEL_ERROR
        YOJIMBO_LOG_LEVEL_INFO
        YOJIMBO_LOG_LEVEL_DEBUG

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

#endif // #ifndef YOJIMBO_PLATFORM_H
