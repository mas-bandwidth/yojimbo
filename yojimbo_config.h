#ifndef YOJIMBO_CONFIG_H
#define YOJIMBO_CONFIG_H

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define YOJIMBO_MAJOR_VERSION 1
#define YOJIMBO_MINOR_VERSION 0
#define YOJIMBO_PATCH_VERSION 0

#if !defined(YOJIMBO_DEBUG) && !defined(YOJIMBO_RELEASE)
#if defined(NDEBUG)
#define YOJIMBO_RELEASE
#else
#define YOJIMBO_DEBUG
#endif
#elif defined(YOJIMBO_DEBUG) && defined(YOJIMBO_RELEASE)
#error Can only define one of debug & release
#endif

#ifndef YOJIMBO_DEFAULT_TIMEOUT
#define YOJIMBO_DEFAULT_TIMEOUT 10
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#endif // #ifdef _MSC_VER

#define YOJIMBO_PLATFORM_WINDOWS                    1
#define YOJIMBO_PLATFORM_MAC                        2
#define YOJIMBO_PLATFORM_UNIX                       3

#if defined(_WIN32)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_MAC
#else
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_UNIX
#endif

#ifdef YOJIMBO_DEBUG

#define YOJIMBO_DEBUG_MEMORY_LEAKS                  1
#define YOJIMBO_DEBUG_MESSAGE_LEAKS                 1
#define YOJIMBO_DEBUG_MESSAGE_BUDGET                1

#else // #ifdef YOJIMBO_DEBUG

#define YOJIMBO_DEBUG_MEMORY_LEAKS                  0
#define YOJIMBO_DEBUG_MESSAGE_LEAKS                 0
#define YOJIMBO_DEBUG_MESSAGE_BUDGET                0

#endif // #ifdef YOJIMBO_DEBUG

#define YOJIMBO_ENABLE_LOGGING                      1

#endif // # YOJIMBO_CONFIG_H
