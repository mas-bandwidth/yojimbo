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

// todo: document this

#define YOJIMBO_LOG_LEVEL_NONE      0
#define YOJIMBO_LOG_LEVEL_ERROR     1
#define YOJIMBO_LOG_LEVEL_INFO      2
#define YOJIMBO_LOG_LEVEL_DEBUG     3

void yojimbo_log_level( int level );

void yojimbo_printf( int level, const char * format, ... );

extern void (*yojimbo_assert_function)( const char *, const char *, const char * file, int line );

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

void yojimbo_set_printf_function( int (*function)( const char * /*format*/, ... ) );

void yojimbo_set_assert_function( void (*function)( const char * /*condition*/, const char * /*function*/, const char * /*file*/, int /*line*/ ) );

#endif // #ifndef YOJIMBO_PLATFORM_H
