/*
    serialize

    Copyright © 2016 - 2026, Más Bandwidth LLC.

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

/*
    If you use this library in a product, please credit
    "serialize - Glenn Fiedler and Rowan Claude" in your product credits. The
    license doesn't require this credit. It's an official request, and honoring
    it is appreciated.
*/

#ifndef SERIALIZE_H
#define SERIALIZE_H

/** @file */

#define SERIALIZE_VERSION_MAJOR 1
#define SERIALIZE_VERSION_MINOR 15
#define SERIALIZE_VERSION_PATCH 0
#define SERIALIZE_VERSION "1.15.0"

#if defined(_MSC_VER)
#define serialize_restrict __restrict
#else // #if defined(_MSC_VER)
#define serialize_restrict __restrict__
#endif // #if defined(_MSC_VER)

/*
    SERIALIZE_ALWAYS_INLINE — how the per-field serialize spine is spelled, both directions.

    A serialize function is a chain of fallible calls — if ( !stream.SerializeX( ... ) )
    return false; — and LLVM's static branch heuristics treat each success/failure split as
    roughly even odds, so block frequency decays geometrically down the chain. A few fields in,
    every remaining callsite is judged cold and held to the cold-callsite inline threshold (45,
    where a hot callsite in the same function gets 250+), which the write helpers do not fit:
    measured on Apple silicon (Apple clang 21, schema bench, -O2 and -O3), WriteStream::
    SerializeBits and SerializeInteger were refused at cost 60-105 against threshold 45,
    stranding 2-6 calls per packet in every write path, and the write rows trailed the C and
    Rust ports 14-23% while the branchless reads led every language. With the demand in place
    the write spine inlines end to end and the same write rows lead instead
    (+57-92%, both optimization levels).

    The read spine makes the same demand — BitReader::ReadBits/ReadAlign and ReadStream's six
    per-field methods, the exact mirror of the write set. The original measurement said the
    reads did not need it (the per-message read rows were `full` in the emitted code), but the
    remarks named the exception: inside a large generated reader the fallible chain decays far
    enough that the read helpers strand at the cold-callsite threshold 45 — ReadStream::
    SerializeInteger64 refused at cost=70, SerializeBytes at 115 and 130, SerializeAlign at 80.
    The demand shipped first as a default-off switch, then won its tournament: armed alongside
    the schema emitter's matching read demand (Apple clang 21, -O3, schema bench, tournament-air
    and confirmation-air eras) the read rows swept with zero regressions — batch read +68%,
    rigidbody_at_rest read +244%, rigidbody_moving read +232%, inputpacket read +187%, testdata
    read +109%, confirmed in a second independent pass — so per the feature lifecycle the
    winner became unconditional code and the switch was deleted.
    The compressed-float and string bodies stay undemanded deliberately: their cost is the
    work, not the call, the same boundary serialize.c draws on its own read spine.

    The sibling ports' other lever — pinning the error edges cold (serialize.rs's #[cold]
    constructor) — was tried here as __builtin_expect on the macros' error branches, measured,
    and REJECTED: it left every cold-callsite refusal in place, and the cold blocks it created
    invited Apple clang's machine outliner to shred the write bodies into bl-called fragments
    (bits write -25%, packet read -21% at -O2, measured). Do not re-add it without measuring.

    MSVC spells the demand __forceinline; compilers with neither spelling fall back to the
    plain inline hint, and lose only the optimization.
*/
#if defined( _MSC_VER )
#define SERIALIZE_ALWAYS_INLINE __forceinline
#elif defined( __GNUC__ ) || defined( __clang__ )
#define SERIALIZE_ALWAYS_INLINE inline __attribute__(( always_inline ))
#else // #if defined( _MSC_VER )
#define SERIALIZE_ALWAYS_INLINE inline
#endif // #if defined( _MSC_VER )

/*
    SERIALIZE_BULK_COPY — how WriteBytes moves memory, spelled so the fortify capture
    can never touch it. Where _FORTIFY_SOURCE is armed (glibc honors it in C++ too, and
    some toolchains arm it by default; Darwin's capture fires for C, which is how
    serialize.c hit it), the string.h macros rewrite
    a plain memcpy call into __builtin___memcpy_chk before the compiler proper ever
    sees it, and the checked form rides LLVM's mid-pipeline as an opaque call. Late
    simplification folds the check away — the destination's object size is unknowable
    here, so it never checked anything — but the inliner prices the function before
    the fold, and WriteBytes sits exactly at that boundary: not on the demanded spine
    (see SERIALIZE_ALWAYS_INLINE above), so its body is costed call by call in every
    generated string and byte-array caller. clang and GCC spell the builtin, which
    lowers identically to memcpy but is never captured; MSVC never fortifies, so plain
    memcpy is already the same thing there. Semantics are byte-identical either way —
    only the mid-pipeline IR form changes. The always-inlined packer paths (WriteBits,
    FlushBits, ReadBits) keep their plain memcpy spelling: the demand makes inline
    pricing moot there.
*/
#if defined( __clang__ ) || defined( __GNUC__ )
#define SERIALIZE_BULK_COPY( dst, src, bytes ) __builtin_memcpy( ( dst ), ( src ), ( bytes ) )
#else // #if defined( __clang__ ) || defined( __GNUC__ )
#define SERIALIZE_BULK_COPY( dst, src, bytes ) memcpy( ( dst ), ( src ), ( bytes ) )
#endif // #if defined( __clang__ ) || defined( __GNUC__ )

/*
    SERIALIZE_FLOAT_FORCE_ROUND( x ) — pins a float intermediate to its rounded float32
    value, so no floating point contraction setting can fuse it into a neighboring
    operation. The compressed float quantization requires TWO distinct roundings on write
    and on read (STANDARD.md): the product must round to float32 before the following add.
    A plain float local suppresses only STATEMENT-LOCAL contraction (clang's default
    -ffp-contract=on); under cross-statement contraction (-ffp-contract=fast, which is
    GCC's default at every optimization level) the compiler fuses straight through the
    local, rounds once, and moves the wire bits by one ulp on any FMA target — measured
    on arm64 and on x86-64 with FMA. This macro is what makes the wire bits identical
    under every contraction mode, so they no longer depend on the consumer's compiler
    flags.

    The reference pair: on GCC and clang an empty asm with a register output operand makes
    the stored value opaque to the contraction pass at zero cost — the product and the add
    compile to separate rounded instructions with no memory traffic ("+w" is an FP/SIMD
    register on arm64, "+x" an XMM register on x86). Everywhere else — MSVC, and GNU
    targets whose register class this header does not spell — the same property is
    emulated with a volatile store, which is correct on every compiler at the cost of a
    stack round-trip. (MSVC's /fp:precise does not contract, so the volatile slot there is
    belt on an uncontracted build; /fp:fast builds are held to the rounding by it.)
*/
#if ( defined( __GNUC__ ) || defined( __clang__ ) ) && ( defined( __aarch64__ ) || defined( _M_ARM64 ) )
#define SERIALIZE_FLOAT_FORCE_ROUND( x ) __asm__ ( "" : "+w" ( x ) )
#elif ( defined( __GNUC__ ) || defined( __clang__ ) ) && ( defined( __x86_64__ ) || defined( __i386__ ) )
#define SERIALIZE_FLOAT_FORCE_ROUND( x ) __asm__ ( "" : "+x" ( x ) )
#else
#define SERIALIZE_FLOAT_FORCE_ROUND( x )                                                    \
    do                                                                                      \
    {                                                                                       \
        volatile float serialize_float_force_round_slot = ( x );                            \
        ( x ) = serialize_float_force_round_slot;                                           \
    }                                                                                       \
    while ( 0 )
#endif

#ifndef serialize_assert
#include <assert.h>
#define serialize_assert assert
#endif // #ifndef serialize_assert

// static_assert is C++11, and consumers vendor this header into pre-C++11 builds, so compile
// time invariants go through this macro: real static_assert (message included) from C++11 up
// and on MSVC (which ships static_assert in every language mode it supports), and a negative
// size array emulation before that — the message string is dropped, and the compile error
// points at the macro use site. the attribute silences unused-local-typedef warnings.
#if ( defined( __cplusplus ) && __cplusplus >= 201103L ) || defined( _MSC_VER )
#define serialize_static_assert( condition, message ) static_assert( condition, message )
#else // #if ( defined( __cplusplus ) && __cplusplus >= 201103L ) || defined( _MSC_VER )
#define serialize_static_assert_join2( a, b ) a##b
#define serialize_static_assert_join( a, b ) serialize_static_assert_join2( a, b )
#define serialize_static_assert( condition, message ) typedef char serialize_static_assert_join( serialize_static_assert_line_, __LINE__ )[ ( condition ) ? 1 : -1 ] __attribute__(( unused ))
#endif // #if ( defined( __cplusplus ) && __cplusplus >= 201103L ) || defined( _MSC_VER )

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#if !defined(SERIALIZE_DEBUG) && !defined(SERIALIZE_RELEASE)
#if defined(NDEBUG)
#define SERIALIZE_RELEASE
#else
#define SERIALIZE_DEBUG
#endif
#elif defined(SERIALIZE_DEBUG) && defined(SERIALIZE_RELEASE)
#error Can only define one of debug & release
#endif

#if !defined(SERIALIZE_LITTLE_ENDIAN ) && !defined( SERIALIZE_BIG_ENDIAN )

  #ifdef __BYTE_ORDER__
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      #define SERIALIZE_LITTLE_ENDIAN 1
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      #define SERIALIZE_BIG_ENDIAN 1
    #else
      #error Unknown machine endianness detected. User needs to define SERIALIZE_LITTLE_ENDIAN or SERIALIZE_BIG_ENDIAN.
    #endif // __BYTE_ORDER__

  // Detect with GLIBC's endian.h
  #elif defined(__GLIBC__)
    #include <endian.h>
    #if (__BYTE_ORDER == __LITTLE_ENDIAN)
      #define SERIALIZE_LITTLE_ENDIAN 1
    #elif (__BYTE_ORDER == __BIG_ENDIAN)
      #define SERIALIZE_BIG_ENDIAN 1
    #else
      #error Unknown machine endianness detected. User needs to define SERIALIZE_LITTLE_ENDIAN or SERIALIZE_BIG_ENDIAN.
    #endif // __BYTE_ORDER

  // Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
  #elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
    #define SERIALIZE_LITTLE_ENDIAN 1
  #elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
    #define SERIALIZE_BIG_ENDIAN 1

  // Detect with architecture macros
  #elif    defined(__sparc)     || defined(__sparc__)                           \
        || defined(_POWER)      || defined(__powerpc__)                         \
        || defined(__ppc__)     || defined(__hpux)      || defined(__hppa)      \
        || defined(_MIPSEB)     || defined(_POWER)      || defined(__s390__)
    #define SERIALIZE_BIG_ENDIAN 1
  #elif    defined(__i386__)    || defined(__alpha__)   || defined(__ia64)      \
        || defined(__ia64__)    || defined(_M_IX86)     || defined(_M_IA64)     \
        || defined(_M_ALPHA)    || defined(__amd64)     || defined(__amd64__)   \
        || defined(_M_AMD64)    || defined(__x86_64)    || defined(__x86_64__)  \
        || defined(_M_X64)      || defined(__bfin__)    || defined(_M_ARM64)
    #define SERIALIZE_LITTLE_ENDIAN 1
  #elif defined(_MSC_VER) && defined(_M_ARM)
    #define SERIALIZE_LITTLE_ENDIAN 1
  #else
    #error Unknown machine endianness detected. User needs to define SERIALIZE_LITTLE_ENDIAN or SERIALIZE_BIG_ENDIAN.
  #endif
#endif

#ifndef SERIALIZE_LITTLE_ENDIAN
#define SERIALIZE_LITTLE_ENDIAN 0
#endif

#ifndef SERIALIZE_BIG_ENDIAN
#define SERIALIZE_BIG_ENDIAN 0
#endif

// implicit narrowing is a deliberate style inside this header. push/pop so the disabled
// warnings do not leak into code that includes it. note that code using the serialize_* macros
// compiles at the including file's warning state: disable 4127 and 4244 there if needed.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#endif // #ifdef _MSC_VER

// everything the library itself needs. serialize.h is intentionally self contained:
// including it into a translation unit with no prior includes must compile.
#include <stdint.h>     // fixed width integer types
#include <stddef.h>     // size_t, NULL
#include <string.h>     // memcpy, memset, strlen
#include <wchar.h>      // wcslen
#include <math.h>       // ceil, floor

// 128 bit integer support.
//
// serialize::uint128_t and serialize::int128_t exist on every platform. Where the compiler
// provides native __int128 (gcc, clang, clang-cl) they are unsigned __int128 and __int128: the
// fastest representation. On compilers without it (pure MSVC), serialize defines the emulated
// pair below — serialize_uint128_t and serialize_int128_t, full 128 bit integer types with standard
// semantics — and announces them with the SERIALIZE_UINT128_DEFINED handshake macro. The two structs
// are one facility: one announce macro covers the pair, and copies are carried whole, never
// just one half.
//
// THE SHARING CONVENTION: single header libraries cannot include each other, so this definition
// block is carried, verbatim, by sibling projects under the same handshake (the fixed point
// library, and games built on these libraries). Whichever header is included first defines the
// pair; everyone after sees SERIALIZE_UINT128_DEFINED and aliases them, so the types are defined
// exactly once per translation unit instead of two or three times. The copies exist by design;
// the handshake makes them mutually exclusive per translation unit, and the differential tests
// against native __int128 in each C++ home are what keep every copy honest — drift fails loudly.
//
// The block is dual language: in C the types are bare POD structs of two uint64_t members,
// low half then high half, matching little endian layout and the wire order. C projects add
// their own function wrappers in their own tree. Everything else — constructors, the full
// operator surface, the explicit conversions — is C++ only, inside #ifdef __cplusplus.
//
// The C++ in this block must stay usable by pre-C++11 consumers: a vendored header does not get
// to choose the consumer's -std, and real consumers compile it at C++98/03 (clang accepts the
// declaration-only C++11 extensions here — `= default`, the explicit conversion operators to the
// 64 bit lanes — with a warning in those modes, but overload resolution before C++11 never
// SELECTS an explicit conversion operator, so every conversion the library performs routes
// through C++98 constructs: converting constructors). CI compiles and runs the tests at
// -std=c++03 to hold this.
//
// Semantics match native __int128 exactly, with documented choices where native has none:
// shift counts outside [0,127] yield zero (all sign bits for the signed arithmetic right
// shift; native shifts by 128 or more are undefined behavior), and signed INT128_MIN over -1
// wraps to INT128_MIN, the bit pattern native hardware produces.
//
// DIVISION OR MODULO BY ZERO IS UNDEFINED. Do not do it. It is undefined for native
// __int128 too — C++ says so, and the hardware disagrees with itself: measured, arm64
// returns zero for x / 0 but returns the DIVIDEND for x % 0, while x86-64 IDIV/DIV raises a
// hardware exception and terminates. There is therefore no portable behavior available to
// match, and any value this block returned would be a divergence from somewhere.
// What the block guarantees is only that it is TOTAL — it returns zero quotient and zero
// remainder rather than trapping, so a caller's mistake cannot crash a process and the block
// keeps its no-assert-dependency design. That is an implementation detail, not a contract:
// callers may not rely on it, and the differential test against native deliberately excludes
// zero divisors, because agreement there is not a property either side can promise.
//
// When tests are enabled the pair is defined even where native __int128 exists, so the test
// suite can prove the emulation agrees with native operator by operator and produces byte
// identical wire.

#if !defined( SERIALIZE_UINT128_DEFINED ) && ( !defined( __SIZEOF_INT128__ ) || SERIALIZE_ENABLE_TESTS )

#ifdef __cplusplus
struct serialize_int128_t;      // forward declaration for the bit preserving converting constructor below
#endif

typedef struct serialize_uint128_t
{
    uint64_t lo;            ///< the low 64 bits. first, matching the little endian layout and the wire order
    uint64_t hi;            ///< the high 64 bits

#ifdef __cplusplus

    serialize_uint128_t() = default;

    // construction mirrors native conversion to unsigned __int128 exactly: unsigned sources zero
    // extend, signed sources sign extend (a negative value wraps modulo 2^128, so the high lane
    // fills with ones). one constructor per standard integer type keeps every call an exact match
    // after integer promotion — a lone uint64_t constructor would zero extend negative int64
    // values where native sign extends, silently diverging the wire between representations.

    serialize_uint128_t( int value )                : lo( uint64_t( int64_t( value ) ) ), hi( ( value < 0 ) ? 0xFFFFFFFFFFFFFFFFULL : 0 ) {}
    serialize_uint128_t( long value )               : lo( uint64_t( int64_t( value ) ) ), hi( ( value < 0 ) ? 0xFFFFFFFFFFFFFFFFULL : 0 ) {}
    serialize_uint128_t( long long value )          : lo( uint64_t( int64_t( value ) ) ), hi( ( value < 0 ) ? 0xFFFFFFFFFFFFFFFFULL : 0 ) {}
    serialize_uint128_t( unsigned int value )       : lo( value ), hi( 0 ) {}
    serialize_uint128_t( unsigned long value )      : lo( value ), hi( 0 ) {}
    serialize_uint128_t( unsigned long long value ) : lo( value ), hi( 0 ) {}

    // bit preserving conversion from the signed type, defined inline after serialize_int128_t below.
    // this is a converting CONSTRUCTOR, not a conversion operator on serialize_int128_t, on purpose:
    // explicit conversion operators are C++11, and consumers vendor this header into pre-C++11
    // builds where a functional-style cast through one does not compile (overload resolution never
    // considers explicit conversion functions before C++11). an explicit converting constructor is
    // C++98 and behaves identically at every cast site.
    explicit serialize_uint128_t( serialize_int128_t value );

    explicit operator uint64_t () const
    {
        return lo;
    }

    bool operator == ( serialize_uint128_t other ) const
    {
        return lo == other.lo && hi == other.hi;
    }

    bool operator != ( serialize_uint128_t other ) const
    {
        return ! ( *this == other );
    }

    bool operator < ( serialize_uint128_t other ) const
    {
        return ( hi != other.hi ) ? ( hi < other.hi ) : ( lo < other.lo );
    }

    bool operator > ( serialize_uint128_t other ) const
    {
        return other < *this;
    }

    bool operator <= ( serialize_uint128_t other ) const
    {
        return ! ( other < *this );
    }

    bool operator >= ( serialize_uint128_t other ) const
    {
        return ! ( *this < other );
    }

    serialize_uint128_t operator ~ () const
    {
        serialize_uint128_t result( 0 );
        result.lo = ~lo;
        result.hi = ~hi;
        return result;
    }

    serialize_uint128_t operator & ( serialize_uint128_t other ) const
    {
        serialize_uint128_t result( 0 );
        result.lo = lo & other.lo;
        result.hi = hi & other.hi;
        return result;
    }

    serialize_uint128_t operator | ( serialize_uint128_t other ) const
    {
        serialize_uint128_t result( 0 );
        result.lo = lo | other.lo;
        result.hi = hi | other.hi;
        return result;
    }

    serialize_uint128_t operator ^ ( serialize_uint128_t other ) const
    {
        serialize_uint128_t result( 0 );
        result.lo = lo ^ other.lo;
        result.hi = hi ^ other.hi;
        return result;
    }

    serialize_uint128_t operator << ( int shift ) const
    {
        // shifting a uint64 lane by 64 is undefined behavior, so the half boundary is an explicit
        // branch. shift counts outside [0,127] yield zero, documented above.
        serialize_uint128_t result( 0 );
        if ( shift == 0 )
        {
            result = *this;
        }
        else if ( shift > 0 && shift < 64 )
        {
            result.hi = ( hi << shift ) | ( lo >> ( 64 - shift ) );
            result.lo = lo << shift;
        }
        else if ( shift >= 64 && shift < 128 )
        {
            result.hi = lo << ( shift - 64 );
            result.lo = 0;
        }
        return result;
    }

    serialize_uint128_t operator >> ( int shift ) const
    {
        // shifting a uint64 lane by 64 is undefined behavior, so the half boundary is an explicit
        // branch. shift counts outside [0,127] yield zero, documented above.
        serialize_uint128_t result( 0 );
        if ( shift == 0 )
        {
            result = *this;
        }
        else if ( shift > 0 && shift < 64 )
        {
            result.lo = ( lo >> shift ) | ( hi << ( 64 - shift ) );
            result.hi = hi >> shift;
        }
        else if ( shift >= 64 && shift < 128 )
        {
            result.lo = hi >> ( shift - 64 );
            result.hi = 0;
        }
        return result;
    }

    serialize_uint128_t operator + ( serialize_uint128_t other ) const
    {
        serialize_uint128_t result( 0 );
        result.lo = lo + other.lo;
        result.hi = hi + other.hi + ( ( result.lo < lo ) ? 1 : 0 );         // carry out of the low lane
        return result;
    }

    serialize_uint128_t operator - ( serialize_uint128_t other ) const
    {
        serialize_uint128_t result( 0 );
        result.lo = lo - other.lo;
        result.hi = hi - other.hi - ( ( lo < other.lo ) ? 1 : 0 );          // borrow out of the low lane
        return result;
    }

    serialize_uint128_t operator * ( serialize_uint128_t other ) const
    {
        // schoolbook multiplication in 32 bit limbs over the uint64 lanes: the low 64 x 64 product
        // is computed exactly, then the cross products fold into the high lane modulo 2^64
        const uint64_t a_low = lo & 0xFFFFFFFFULL;
        const uint64_t a_high = lo >> 32;
        const uint64_t b_low = other.lo & 0xFFFFFFFFULL;
        const uint64_t b_high = other.lo >> 32;

        const uint64_t product_ll = a_low * b_low;
        const uint64_t product_lh = a_low * b_high;
        const uint64_t product_hl = a_high * b_low;
        const uint64_t product_hh = a_high * b_high;

        const uint64_t carry = ( ( product_ll >> 32 ) + ( product_lh & 0xFFFFFFFFULL ) + ( product_hl & 0xFFFFFFFFULL ) ) >> 32;

        serialize_uint128_t result( 0 );
        result.lo = product_ll + ( product_lh << 32 ) + ( product_hl << 32 );
        result.hi = product_hh + ( product_lh >> 32 ) + ( product_hl >> 32 ) + carry;
        result.hi += lo * other.hi + hi * other.lo;
        return result;
    }

    static void DivMod( serialize_uint128_t dividend, serialize_uint128_t divisor, serialize_uint128_t & quotient, serialize_uint128_t & remainder )
    {
        // shift subtract long division. division by zero is UNDEFINED (documented above) — this
        // returns zero quotient and zero remainder only to stay TOTAL, since the block has no
        // assert dependency by design. Not a contract; callers may not rely on the value.
        quotient = serialize_uint128_t( 0 );
        remainder = serialize_uint128_t( 0 );
        if ( divisor == serialize_uint128_t( 0 ) )
        {
            return;
        }
        if ( dividend.hi == 0 && divisor.hi == 0 )
        {
            quotient = serialize_uint128_t( dividend.lo / divisor.lo );
            remainder = serialize_uint128_t( dividend.lo % divisor.lo );
            return;
        }
        for ( int i = 127; i >= 0; i-- )
        {
            remainder = remainder << 1;
            remainder.lo |= ( dividend >> i ).lo & 1;
            if ( remainder >= divisor )
            {
                remainder = remainder - divisor;
                quotient = quotient | ( serialize_uint128_t( 1 ) << i );
            }
        }
    }

    serialize_uint128_t operator / ( serialize_uint128_t other ) const
    {
        serialize_uint128_t quotient( 0 );
        serialize_uint128_t remainder( 0 );
        DivMod( *this, other, quotient, remainder );
        return quotient;
    }

    serialize_uint128_t operator % ( serialize_uint128_t other ) const
    {
        serialize_uint128_t quotient( 0 );
        serialize_uint128_t remainder( 0 );
        DivMod( *this, other, quotient, remainder );
        return remainder;
    }

    serialize_uint128_t operator + () const
    {
        return *this;
    }

    serialize_uint128_t operator - () const
    {
        return serialize_uint128_t( 0 ) - *this;
    }

    serialize_uint128_t & operator += ( serialize_uint128_t other ) { *this = *this + other; return *this; }
    serialize_uint128_t & operator -= ( serialize_uint128_t other ) { *this = *this - other; return *this; }
    serialize_uint128_t & operator *= ( serialize_uint128_t other ) { *this = *this * other; return *this; }
    serialize_uint128_t & operator /= ( serialize_uint128_t other ) { *this = *this / other; return *this; }
    serialize_uint128_t & operator %= ( serialize_uint128_t other ) { *this = *this % other; return *this; }
    serialize_uint128_t & operator &= ( serialize_uint128_t other ) { *this = *this & other; return *this; }
    serialize_uint128_t & operator |= ( serialize_uint128_t other ) { *this = *this | other; return *this; }
    serialize_uint128_t & operator ^= ( serialize_uint128_t other ) { *this = *this ^ other; return *this; }
    serialize_uint128_t & operator <<= ( int shift )          { *this = *this << shift; return *this; }
    serialize_uint128_t & operator >>= ( int shift )          { *this = *this >> shift; return *this; }

    serialize_uint128_t & operator ++ ()
    {
        *this = *this + serialize_uint128_t( 1 );
        return *this;
    }

    serialize_uint128_t operator ++ ( int )
    {
        serialize_uint128_t before = *this;
        ++( *this );
        return before;
    }

    serialize_uint128_t & operator -- ()
    {
        *this = *this - serialize_uint128_t( 1 );
        return *this;
    }

    serialize_uint128_t operator -- ( int )
    {
        serialize_uint128_t before = *this;
        --( *this );
        return before;
    }

#endif // #ifdef __cplusplus

} serialize_uint128_t;

/**
    The emulated signed 128 bit integer: a thin two's complement layer over the unsigned lanes.
    Addition, subtraction, multiplication and the bitwise operators produce the same bit patterns
    as the unsigned type (two's complement), so they delegate to it instead of duplicating the
    arithmetic. The signed specific pieces are the comparisons (the high lane compares signed),
    operator>> (ARITHMETIC shift: vacated bits fill with the sign), division and modulo (sign
    extraction, unsigned divmod on the magnitudes, sign application — C++ truncation toward zero
    with the remainder sign following the dividend), unary minus, the sign extending int64_t
    constructor, and the bit preserving conversions to and from the unsigned type.
    Documented choices where native has none: shift counts outside [0,127] yield zero for <<
    and all sign bits for >> (the limit of shifting further); and the one overflowing division,
    INT128_MIN over -1, wraps to INT128_MIN quotient and zero remainder — the same bit pattern
    native two's complement hardware produces.
    Division or modulo by zero is UNDEFINED, as it is for native __int128 — see the block comment
    on serialize_uint128_t. This type stays total and returns zero rather than trapping, which is
    an implementation detail and not a contract.
    This struct is part of the serialize_uint128_t definition block above and is carried with it: the
    two types are one facility, announced together by the single SERIALIZE_UINT128_DEFINED handshake
    macro and defined exactly once per translation unit. Copies in sibling projects carry the
    whole pair, never just one half.
 */

typedef struct serialize_int128_t
{
    uint64_t lo;            ///< the low 64 bits. first, matching the little endian layout and the wire order
    uint64_t hi;            ///< the high 64 bits. the top bit is the sign

#ifdef __cplusplus

    serialize_int128_t() = default;

    // construction mirrors native conversion to __int128 exactly: signed sources sign extend,
    // unsigned sources zero extend (any uint64 is below 2^127, so the conversion is value
    // preserving with a zero high lane). one constructor per standard integer type keeps every
    // call an exact match after integer promotion — a lone int64_t constructor would wrap large
    // uint64 values negative where native keeps them positive, silently diverging the wire
    // between representations.

    serialize_int128_t( int value )                : lo( uint64_t( int64_t( value ) ) ), hi( ( value < 0 ) ? 0xFFFFFFFFFFFFFFFFULL : 0 ) {}
    serialize_int128_t( long value )               : lo( uint64_t( int64_t( value ) ) ), hi( ( value < 0 ) ? 0xFFFFFFFFFFFFFFFFULL : 0 ) {}
    serialize_int128_t( long long value )          : lo( uint64_t( int64_t( value ) ) ), hi( ( value < 0 ) ? 0xFFFFFFFFFFFFFFFFULL : 0 ) {}
    serialize_int128_t( unsigned int value )       : lo( value ), hi( 0 ) {}
    serialize_int128_t( unsigned long value )      : lo( value ), hi( 0 ) {}
    serialize_int128_t( unsigned long long value ) : lo( value ), hi( 0 ) {}

    explicit serialize_int128_t( serialize_uint128_t value ) : lo( value.lo ), hi( value.hi ) {}        // bit preserving

    // the bit preserving conversion in the other direction is the explicit
    // serialize_uint128_t( serialize_int128_t ) constructor, declared on the unsigned type and
    // defined after this struct — a constructor rather than a C++11 explicit conversion operator
    // so serialize_uint128_t( signed_value ) compiles in pre-C++11 consumers too

    explicit operator int64_t () const
    {
        return int64_t( lo );               // the low lane, wrapping two's complement like a native narrowing conversion
    }

    bool IsNegative() const
    {
        return ( hi >> 63 ) != 0;
    }

    bool operator == ( serialize_int128_t other ) const
    {
        return lo == other.lo && hi == other.hi;
    }

    bool operator != ( serialize_int128_t other ) const
    {
        return ! ( *this == other );
    }

    bool operator < ( serialize_int128_t other ) const
    {
        // signed ordering: the high lanes compare signed, the low lanes break ties unsigned
        if ( hi != other.hi )
        {
            return int64_t( hi ) < int64_t( other.hi );
        }
        return lo < other.lo;
    }

    bool operator > ( serialize_int128_t other ) const
    {
        return other < *this;
    }

    bool operator <= ( serialize_int128_t other ) const
    {
        return ! ( other < *this );
    }

    bool operator >= ( serialize_int128_t other ) const
    {
        return ! ( *this < other );
    }

    // two's complement: addition, subtraction, multiplication and the bitwise operators are the
    // same bit patterns as unsigned, so they delegate to the unsigned type. signed overflow wraps
    // by construction, exactly like the underlying hardware.

    serialize_int128_t operator + ( serialize_int128_t other ) const { return serialize_int128_t( serialize_uint128_t( *this ) + serialize_uint128_t( other ) ); }
    serialize_int128_t operator - ( serialize_int128_t other ) const { return serialize_int128_t( serialize_uint128_t( *this ) - serialize_uint128_t( other ) ); }
    serialize_int128_t operator * ( serialize_int128_t other ) const { return serialize_int128_t( serialize_uint128_t( *this ) * serialize_uint128_t( other ) ); }

    serialize_int128_t operator ~ () const                     { return serialize_int128_t( ~serialize_uint128_t( *this ) ); }
    serialize_int128_t operator & ( serialize_int128_t other ) const { return serialize_int128_t( serialize_uint128_t( *this ) & serialize_uint128_t( other ) ); }
    serialize_int128_t operator | ( serialize_int128_t other ) const { return serialize_int128_t( serialize_uint128_t( *this ) | serialize_uint128_t( other ) ); }
    serialize_int128_t operator ^ ( serialize_int128_t other ) const { return serialize_int128_t( serialize_uint128_t( *this ) ^ serialize_uint128_t( other ) ); }

    serialize_int128_t operator << ( int shift ) const
    {
        // a logical shift of the bit pattern, matching what native two's complement hardware does.
        // shift counts outside [0,127] yield zero, matching the unsigned type
        return serialize_int128_t( serialize_uint128_t( *this ) << shift );
    }

    serialize_int128_t operator >> ( int shift ) const
    {
        // ARITHMETIC shift right: the vacated high bits fill with the sign. shift counts outside
        // [0,127] yield all sign bits — 0 for non negative values, -1 for negative ones — which is
        // the limit of shifting further, documented above
        if ( shift < 0 || shift >= 128 )
        {
            return IsNegative() ? serialize_int128_t( -1 ) : serialize_int128_t( 0 );
        }
        serialize_uint128_t result = serialize_uint128_t( *this ) >> shift;
        if ( IsNegative() && shift > 0 )
        {
            result = result | ( ~serialize_uint128_t( 0 ) << ( 128 - shift ) );
        }
        return serialize_int128_t( result );
    }

    serialize_int128_t operator + () const
    {
        return *this;
    }

    serialize_int128_t operator - () const
    {
        return serialize_int128_t( -serialize_uint128_t( *this ) );                 // two's complement negation. -INT128_MIN wraps to itself, like native
    }

    static void DivMod( serialize_int128_t dividend, serialize_int128_t divisor, serialize_int128_t & quotient, serialize_int128_t & remainder )
    {
        // sign extraction, unsigned shift subtract division on the magnitudes, then sign
        // application: C++ semantics, truncation toward zero with the remainder sign following
        // the dividend. the documented edge choices live in the comment block above.
        const bool dividend_negative = dividend.IsNegative();
        const bool divisor_negative = divisor.IsNegative();
        const serialize_uint128_t dividend_magnitude = dividend_negative ? -serialize_uint128_t( dividend ) : serialize_uint128_t( dividend );
        const serialize_uint128_t divisor_magnitude = divisor_negative ? -serialize_uint128_t( divisor ) : serialize_uint128_t( divisor );
        serialize_uint128_t unsigned_quotient( 0 );
        serialize_uint128_t unsigned_remainder( 0 );
        serialize_uint128_t::DivMod( dividend_magnitude, divisor_magnitude, unsigned_quotient, unsigned_remainder );
        quotient = serialize_int128_t( ( dividend_negative != divisor_negative ) ? -unsigned_quotient : unsigned_quotient );
        remainder = serialize_int128_t( dividend_negative ? -unsigned_remainder : unsigned_remainder );
    }

    serialize_int128_t operator / ( serialize_int128_t other ) const
    {
        serialize_int128_t quotient( 0 );
        serialize_int128_t remainder( 0 );
        DivMod( *this, other, quotient, remainder );
        return quotient;
    }

    serialize_int128_t operator % ( serialize_int128_t other ) const
    {
        serialize_int128_t quotient( 0 );
        serialize_int128_t remainder( 0 );
        DivMod( *this, other, quotient, remainder );
        return remainder;
    }

    serialize_int128_t & operator += ( serialize_int128_t other ) { *this = *this + other; return *this; }
    serialize_int128_t & operator -= ( serialize_int128_t other ) { *this = *this - other; return *this; }
    serialize_int128_t & operator *= ( serialize_int128_t other ) { *this = *this * other; return *this; }
    serialize_int128_t & operator /= ( serialize_int128_t other ) { *this = *this / other; return *this; }
    serialize_int128_t & operator %= ( serialize_int128_t other ) { *this = *this % other; return *this; }
    serialize_int128_t & operator &= ( serialize_int128_t other ) { *this = *this & other; return *this; }
    serialize_int128_t & operator |= ( serialize_int128_t other ) { *this = *this | other; return *this; }
    serialize_int128_t & operator ^= ( serialize_int128_t other ) { *this = *this ^ other; return *this; }
    serialize_int128_t & operator <<= ( int shift )         { *this = *this << shift; return *this; }
    serialize_int128_t & operator >>= ( int shift )         { *this = *this >> shift; return *this; }

    serialize_int128_t & operator ++ ()
    {
        *this = *this + serialize_int128_t( 1 );
        return *this;
    }

    serialize_int128_t operator ++ ( int )
    {
        serialize_int128_t before = *this;
        ++( *this );
        return before;
    }

    serialize_int128_t & operator -- ()
    {
        *this = *this - serialize_int128_t( 1 );
        return *this;
    }

    serialize_int128_t operator -- ( int )
    {
        serialize_int128_t before = *this;
        --( *this );
        return before;
    }

#endif // #ifdef __cplusplus

} serialize_int128_t;

#ifdef __cplusplus

// the bit preserving signed to unsigned conversion declared inside serialize_uint128_t above,
// defined here where serialize_int128_t is complete
inline serialize_uint128_t::serialize_uint128_t( serialize_int128_t value ) : lo( value.lo ), hi( value.hi ) {}

#endif // #ifdef __cplusplus

#define SERIALIZE_UINT128_DEFINED 1

#endif // #if !defined( SERIALIZE_UINT128_DEFINED ) && ( !defined( __SIZEOF_INT128__ ) || SERIALIZE_ENABLE_TESTS )

namespace serialize
{
#if defined(__SIZEOF_INT128__)

    /**
        Native 128 bit integer types, where the compiler provides them (gcc, clang, clang-cl).
        int128_t is the storage type for wide fixed point (Q112.16 and friends). uint128_t is the 128 bit wire interchange type. On compilers without __int128 both fall back to the emulated pair below, so serialize::int128_t and serialize::uint128_t exist on every platform.
        __extension__ keeps -Wpedantic quiet: __int128 is a language extension.
     */

    __extension__ typedef          __int128  int128_t;
    __extension__ typedef unsigned __int128 uint128_t;

#else // #if defined(__SIZEOF_INT128__)

    /**
        The 128 bit integer types on compilers without native __int128: the emulated ::serialize_uint128_t and ::serialize_int128_t pair.
        See the comment block on serialize_uint128_t above for the sharing convention between projects.
     */

    typedef ::serialize_int128_t   int128_t;
    typedef ::serialize_uint128_t uint128_t;

#endif // #if defined(__SIZEOF_INT128__)

    /**
        Calculates the population count of an unsigned 32 bit integer at compile time.
        Population count is the number of bits in the integer that set to 1.
        See "Hacker's Delight" and http://www.hackersdelight.org/hdcodetxt/popArrayHS.c.txt
        @see serialize::Log2
        @see serialize::BitsRequired
     */

    template <uint32_t x> struct PopCount
    {
        enum {   a = x - ( ( x >> 1 )       & 0x55555555 ),
                 b =   ( ( ( a >> 2 )       & 0x33333333 ) + ( a & 0x33333333 ) ),
                 c =   ( ( ( b >> 4 ) + b ) & 0x0f0f0f0f ),
                 d =   c + ( c >> 8 ),
                 e =   d + ( d >> 16 ),

            result = e & 0x0000003f
        };
    };

    /**
        Calculates the log 2 of an unsigned 32 bit integer at compile time.
        @see serialize::Log2
        @see serialize::BitsRequired
     */

    template <uint32_t x> struct Log2
    {
        enum {   a = x | ( x >> 1 ),
                 b = a | ( a >> 2 ),
                 c = b | ( b >> 4 ),
                 d = c | ( c >> 8 ),
                 e = d | ( d >> 16 ),
                 f = e >> 1,

            result = PopCount<f>::result
        };
    };

    /**
        Calculates the number of bits required to serialize an integer value in [min,max] at compile time.
        @see Log2
        @see PopCount
     */

    template <int64_t min, int64_t max> struct BitsRequired
    {
        static const uint32_t result = ( min == max ) ? 0 : ( Log2<uint32_t(max-min)>::result + 1 );
    };

    /**
        Calculates the number of bits needed to represent an unsigned 64 bit integer at compile time.
        This is floor( log2( x ) ) + 1, and zero for an input of zero.
        @see serialize::BitsRequired64
     */

    template <uint64_t x> struct BitCount64
    {
        static const int result = 1 + BitCount64< ( x >> 1 ) >::result;
    };

    template <> struct BitCount64<0>
    {
        static const int result = 0;
    };

    /**
        Calculates the number of bits required to serialize a 64 bit integer value in [min,max] at compile time.
        The subtraction is performed in the unsigned domain, so ranges wider than 2^63 work.
        @see serialize::BitsRequired
        @see serialize::bits_required64
     */

    template <uint64_t min, uint64_t max> struct BitsRequired64
    {
        static const int result = BitCount64< max - min >::result;
    };

#if defined(__SIZEOF_INT128__)

    /**
        Calculates the number of bits needed to represent an unsigned 128 bit integer at compile time.
        This is floor( log2( x ) ) + 1, and zero for an input of zero.
        @see serialize::BitsRequired128
     */

    template <uint128_t x> struct BitCount128
    {
        static const int result = 1 + BitCount128< ( x >> 1 ) >::result;
    };

    template <> struct BitCount128<0>
    {
        static const int result = 0;
    };

    /**
        Calculates the number of bits required to serialize a 128 bit integer value in [min,max] at compile time.
        The subtraction is performed in the unsigned domain, so ranges wider than 2^127 work.
        @see serialize::BitsRequired64
     */

    template <uint128_t min, uint128_t max> struct BitsRequired128
    {
        static const int result = BitCount128< max - min >::result;
    };

#endif // #if defined(__SIZEOF_INT128__)

    /**
        Calculates the population count of an unsigned 32 bit integer.
        The population count is the number of bits in the integer set to 1.
        @param x The input integer value.
        @returns The number of bits set to 1 in the input value.
     */

    inline uint32_t popcount( uint32_t x )
    {
#ifdef __GNUC__
        return __builtin_popcount( x );
#else // #ifdef __GNUC__
        const uint32_t a = x - ( ( x >> 1 )       & 0x55555555 );
        const uint32_t b =   ( ( ( a >> 2 )       & 0x33333333 ) + ( a & 0x33333333 ) );
        const uint32_t c =   ( ( ( b >> 4 ) + b ) & 0x0f0f0f0f );
        const uint32_t d =   c + ( c >> 8 );
        const uint32_t e =   d + ( d >> 16 );
        const uint32_t result = e & 0x0000003f;
        return result;
#endif // #ifdef __GNUC__
    }

    /**
        Calculates the log base 2 of an unsigned 32 bit integer.
        @param x The input integer value.
        @returns The log base 2 of the input.
     */

    inline uint32_t log2( uint32_t x )
    {
        const uint32_t a = x | ( x >> 1 );
        const uint32_t b = a | ( a >> 2 );
        const uint32_t c = b | ( b >> 4 );
        const uint32_t d = c | ( c >> 8 );
        const uint32_t e = d | ( d >> 16 );
        const uint32_t f = e >> 1;
        return popcount( f );
    }

    /**
        Calculates the number of bits required to serialize an integer in range [min,max].
        @param min The minimum value.
        @param max The maximum value.
        @returns The number of bits required to serialize the integer.
     */

    inline int bits_required( uint32_t min, uint32_t max )
    {
#ifdef __GNUC__
        return ( min == max ) ? 0 : 32 - __builtin_clz( max - min );
#else // #ifdef __GNUC__
        return ( min == max ) ? 0 : log2( max - min ) + 1;
#endif // #ifdef __GNUC__
    }

    /**
        Calculates the number of bits required to serialize a 64 bit integer in range [min,max].
        @param min The minimum value.
        @param max The maximum value.
        @returns The number of bits required to serialize the integer in [0,64].
     */

    inline int bits_required64( uint64_t min, uint64_t max )
    {
        if ( min == max )
        {
            return 0;
        }
        // subtract in the unsigned domain: max - min overflows signed arithmetic when the range is wider than 2^63
        const uint64_t diff = max - min;
#ifdef __GNUC__
        return 64 - __builtin_clzll( diff );
#else // #ifdef __GNUC__
        const uint32_t high = uint32_t( diff >> 32 );
        return high ? ( 32 + bits_required( 0, high ) ) : bits_required( 0, uint32_t( diff ) );
#endif // #ifdef __GNUC__
    }

    /**
        Calculates the number of bits required to serialize a 128 bit integer in range [min,max].
        The subtraction is performed in the unsigned domain, so ranges wider than 2^127 work.
        Unlike the compile time serialize::BitsRequired128, which needs native __int128 as a non type
        template parameter, this runs on every platform: the emulated pair supports every operation
        it uses, so the 128 bit ranged codec works on pure MSVC too.
        @param min The minimum value.
        @param max The maximum value.
        @returns The number of bits required to serialize the integer in [0,128].
     */

    inline int bits_required128( uint128_t min, uint128_t max )
    {
        if ( min == max )
        {
            return 0;
        }
        // subtract in the unsigned domain: max - min overflows signed arithmetic when the range is wider than 2^127
        const uint128_t diff = max - min;
        const uint64_t high = uint64_t( diff >> 64 );
        return high ? ( 64 + bits_required64( 0, high ) ) : bits_required64( 0, uint64_t( diff ) );
    }

    /**
        Reverse the order of bytes in a 64 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint64_t bswap( uint64_t value )
    {
#ifdef __GNUC__
        return __builtin_bswap64( value );
#else // #ifdef __GNUC__
        value = ( value & 0x00000000FFFFFFFF ) << 32 | ( value & 0xFFFFFFFF00000000 ) >> 32;
        value = ( value & 0x0000FFFF0000FFFF ) << 16 | ( value & 0xFFFF0000FFFF0000 ) >> 16;
        value = ( value & 0x00FF00FF00FF00FF ) << 8  | ( value & 0xFF00FF00FF00FF00 ) >> 8;
        return value;
#endif // #ifdef __GNUC__
    }

    /**
        Reverse the order of bytes in a 32 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint32_t bswap( uint32_t value )
    {
#ifdef __GNUC__
        return __builtin_bswap32( value );
#else // #ifdef __GNUC__
        return ( value & 0x000000ff ) << 24 | ( value & 0x0000ff00 ) << 8 | ( value & 0x00ff0000 ) >> 8 | ( value & 0xff000000 ) >> 24;
#endif // #ifdef __GNUC__
    }

    /**
        Reverse the order of bytes in a 16 bit integer.
        @param value The input value.
        @returns The input value with the byte order reversed.
     */

    inline uint16_t bswap( uint16_t value )
    {
        return ( value & 0x00ff ) << 8 | ( value & 0xff00 ) >> 8;
    }

    /**
        Template to convert an integer value from local byte order to network byte order.
        IMPORTANT: Because most machines are little endian, serialize defines network byte order to be little endian.
        @param value The input value in local byte order. Supported integer types: uint64_t, uint32_t, uint16_t.
        @returns The input value converted to network byte order. If this processor is little endian the output is the same as the input. If the processor is big endian, the output is the input byte swapped.
        @see serialize::bswap
     */

    template <typename T> T host_to_network( T value )
    {
#if SERIALIZE_BIG_ENDIAN
        return bswap( value );
#else // #if SERIALIZE_BIG_ENDIAN
        return value;
#endif // #if SERIALIZE_BIG_ENDIAN
    }

    /**
        Template to convert an integer value from network byte order to local byte order.
        IMPORTANT: Because most machines are little endian, serialize defines network byte order to be little endian.
        @param value The input value in network byte order. Supported integer types: uint64_t, uint32_t, uint16_t.
        @returns The input value converted to local byte order. If this processor is little endian the output is the same as the input. If the processor is big endian, the output is the input byte swapped.
        @see serialize::bswap
     */

    template <typename T> T network_to_host( T value )
    {
#if SERIALIZE_BIG_ENDIAN
        return bswap( value );
#else // #if SERIALIZE_BIG_ENDIAN
        return value;
#endif // #if SERIALIZE_BIG_ENDIAN
    }

    /**
        Convert a signed integer to an unsigned integer with zig-zag encoding.
        0,-1,+1,-2,+2... becomes 0,1,2,3,4 ...
        @param n The input value.
        @returns The input value converted from signed to unsigned with zig-zag encoding.
     */

    inline uint32_t signed_to_unsigned( int32_t n )
    {
        // shift in the unsigned domain: left shift of a negative signed value is undefined behavior pre C++20
        return ( uint32_t(n) << 1 ) ^ ( 0 - ( uint32_t(n) >> 31 ) );
    }

    /**
        Convert an unsigned integer to as signed integer with zig-zag encoding.
        0,1,2,3,4... becomes 0,-1,+1,-2,+2...
        @param n The input value.
        @returns The input value converted from unsigned to signed with zig-zag encoding.
     */

    inline int32_t unsigned_to_signed( uint32_t n )
    {
        return int32_t( ( n >> 1 ) ^ ( 0 - ( n & 1 ) ) );
    }

    /**
        Bitpacks unsigned integer values to a buffer.
        Integer bit values are written to a 64 bit scratch value from right to left.
        Once the scratch fills to 64 bits it is flushed to memory as a qword; the handful of bits that spilled past 64 carry over into the next scratch. Flushing half as often as a dword design makes writes ~30% faster.
        The bit stream is written to memory in little endian order, which is considered network byte order for this library.
        IMPORTANT: The buffer size must be a multiple of 8 bytes, because words are stored to memory 8 bytes at a time. Bytes past the end of the written data are only ever written as zeros.
        IMPORTANT: The buffer must not overlap the BitWriter object itself. The hot write methods promise this to the compiler with a restrict qualified this pointer (see WriteBits), and it is asserted in debug builds.
        @see BitReader
     */

    class BitWriter
    {
    public:

        BitWriter() : m_data( NULL ), m_scratch( 0 ), m_numBits( 0 ), m_bitsWritten( 0 ), m_wordIndex( 0 ), m_scratchBits( 0 ) {}

        void Initialize( void * serialize_restrict data, int64_t bytes )
        {
            serialize_assert( data );
            serialize_assert( ( bytes % 8 ) == 0 );
            serialize_assert( (const uint8_t*) data + bytes <= (const uint8_t*) (const void*) this || (const uint8_t*) (const void*) ( this + 1 ) <= (const uint8_t*) data );   // the buffer must not overlap the writer object itself: WriteBits and friends promise this to the compiler (see WriteBits)
            m_data = (uint8_t*) data;
            m_numBits = bytes * 8;
            m_bitsWritten = 0;
            m_wordIndex = 0;
            m_scratch = 0;
            m_scratchBits = 0;
        }

        /**
            Bit writer constructor.
            Creates a bit writer object to write to the specified buffer.
            @param data The pointer to the buffer to fill with bitpacked data. Does not need to be aligned: each word is stored with memcpy, matching the bit reader. Must not overlap the BitWriter object itself (see WriteBits).
            @param bytes The size of the buffer in bytes. Must be a multiple of 8, because the bit writer stores qwords to memory. Buffer sizes are effectively unlimited, because bit counts are stored in 64 bit signed integers.
         */

        BitWriter( void * serialize_restrict data, int64_t bytes ) : m_data( (uint8_t*) data )
        {
            serialize_assert( data );
            serialize_assert( ( bytes % 8 ) == 0 );
            serialize_assert( (const uint8_t*) data + bytes <= (const uint8_t*) (const void*) this || (const uint8_t*) (const void*) ( this + 1 ) <= (const uint8_t*) data );   // the buffer must not overlap the writer object itself: WriteBits and friends promise this to the compiler (see WriteBits)
            m_numBits = bytes * 8;
            m_bitsWritten = 0;
            m_wordIndex = 0;
            m_scratch = 0;
            m_scratchBits = 0;
        }

        /**
            Write bits to the buffer.
            Bits are written to the buffer as-is, without padding to nearest byte. Will assert if you try to write past the end of the buffer.
            A boolean value writes just 1 bit to the buffer, a value in range [0,31] can be written with just 5 bits and so on.
            IMPORTANT: When you have finished writing to your buffer, take care to call BitWrite::FlushBits, otherwise the last word of data will not get flushed to memory!
            @param value The integer value to write to the buffer. Must be in [0,(1<<bits)-1].
            @param bits The number of bits to encode in [1,32].
            @see BitReader::ReadBits
         */

        // serialize_restrict on 'this': m_data is a uint8_t pointer, and a uint8_t store legally aliases
        // anything -- including this object's own members. Without the qualifier, in any serialize function
        // it does not fully inline, the compiler is forced to reload m_scratch / m_scratchBits / m_wordIndex /
        // m_bitsWritten from memory after every word flushed to the buffer, because the flush store could have
        // modified them. Restricting 'this' promises the buffer never overlaps the writer object itself
        // (asserted in the constructor in debug builds), which lets member state stay in registers across
        // consecutive writes. Measured on Apple Silicon (Apple clang 21, -O3): +38% stream write throughput
        // in bench.cpp, +38% to +152% on schema-generated write paths, zero change where serialize functions
        // fully inline (the raw bitpacker loop compiles to identical code). The read path does not need this:
        // the branchless reader stores nothing through m_data, so it already keeps its state in registers.

        SERIALIZE_ALWAYS_INLINE void WriteBits( uint32_t value, int bits ) serialize_restrict
        {
            serialize_assert( m_data );                 // if this fires, the writer was used before Initialize
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            serialize_assert( m_bitsWritten + bits <= m_numBits );
            serialize_assert( uint64_t( value ) <= ( ( 1ULL << bits ) - 1 ) );

            m_scratch |= uint64_t( value ) << m_scratchBits;

            const int newScratchBits = m_scratchBits + bits;

            if ( newScratchBits >= 64 )
            {
                const uint64_t word = host_to_network( m_scratch );
                memcpy( m_data + (size_t) m_wordIndex * 8, &word, sizeof( word ) );
                m_wordIndex++;
                // recover the bits that spilled past 64. newScratchBits >= 64 with bits <= 32 implies the shift is in [1,32]
                m_scratch = uint64_t( value ) >> ( 64 - m_scratchBits );
                m_scratchBits = newScratchBits - 64;
            }
            else
            {
                m_scratchBits = newScratchBits;
            }

            m_bitsWritten += bits;
        }

        /**
            Write an alignment to the bit stream, padding zeros so the bit index becomes is a multiple of 8.
            This is useful if you want to write some data to a packet that should be byte aligned. For example, an array of bytes, or a string.
            IMPORTANT: If the current bit index is already a multiple of 8, nothing is written.
            @see BitReader::ReadAlign
         */

        SERIALIZE_ALWAYS_INLINE void WriteAlign()
        {
            const int remainderBits = m_bitsWritten % 8;

            if ( remainderBits != 0 )
            {
                uint32_t zero = 0;
                WriteBits( zero, 8 - remainderBits );
                serialize_assert( ( m_bitsWritten % 8 ) == 0 );
            }
        }

        /**
            Write an array of bytes to the bit stream.
            Use this when you have to copy a large block of data into your bitstream.
            Faster than just writing each byte to the bit stream via BitWriter::WriteBits( value, 8 ), because it aligns to byte index and copies into the buffer without bitpacking.
            The body is fused: one qword store flushes the partial scratch word (its high bytes are zero and the payload overwrites them), one bulk copy lands the whole payload at the byte cursor, and one qword load reloads the trailing partial word into the scratch — masked to its tail bits — so later writes pack into it exactly as if its bytes had gone through the packer.
            The old shape pushed the block's head and tail through the packer a byte at a time, and that loop priced the function out of inlining in generated callers (refused at cost 345 against threshold 225 for a string body; fused it inlines at cost 90); serialize.c fused its body first, and the wire bytes are identical either way.
            The tail load reads only the word the final flush is already obliged to store, and the byte-swapped scratch is what makes the edge words single moves on either endianness — the golden vectors pin that, and CI runs them on s390x.
            @param data The byte array data to write to the bit stream.
            @param bytes The number of bytes to write.
            @see BitReader::ReadBytes
         */

        void WriteBytes( const uint8_t * serialize_restrict data, int64_t bytes ) serialize_restrict     // restrict qualified this: see WriteBits
        {
            serialize_assert( m_data );                 // if this fires, the writer was used before Initialize
            serialize_assert( uint64_t(m_bitsWritten) + uint64_t(bytes) * 8 <= uint64_t(m_numBits) );
            serialize_assert( ( m_bitsWritten % 8 ) == 0 );                         // byte aligned (GetAlignBits() == 0, spelled directly: a restrict qualified function cannot call unqualified members on some compilers)
            serialize_assert( m_scratchBits == m_bitsWritten % 64 );                // mid-stream the scratch tracks the cursor (writing after FlushBits mid-stream was never supported)

            // the head: one word store, not a byte loop. the partial scratch word goes to the
            // buffer whole — its low scratchBits are the bytes already written, its high bytes
            // are zero, and the payload copy below overwrites exactly those zero bytes. the
            // buffer size is a multiple of 8, so the qword store stays in bounds (see FlushBits).
            if ( m_scratchBits != 0 )
            {
                const uint64_t word = host_to_network( m_scratch );
                SERIALIZE_BULK_COPY( m_data + (size_t) m_wordIndex * 8, &word, sizeof( word ) );
            }

            // the body: the whole payload, straight in at the byte cursor
            SERIALIZE_BULK_COPY( m_data + (size_t) ( m_bitsWritten >> 3 ), data, (size_t) bytes );

            m_bitsWritten += bytes * 8;
            m_wordIndex = m_bitsWritten / 64;

            // the tail: reload the trailing partial word into the scratch, masked to its tail
            // bits, so later writes pack into it exactly as if its bytes had gone through the
            // packer. the load reads the word the final flush is already obliged to store
            // (tailBits != 0 keeps m_scratchBits != 0, so FlushBits WILL store it), so it
            // touches no memory the stream does not already own; the bits above the tail are
            // whatever the buffer held, and the mask discards them.
            const int tailBits = (int) ( m_bitsWritten % 64 );
            if ( tailBits != 0 )
            {
                uint64_t word;
                SERIALIZE_BULK_COPY( &word, m_data + (size_t) m_wordIndex * 8, sizeof( word ) );
                m_scratch = network_to_host( word ) & ( ( uint64_t(1) << tailBits ) - 1 );
            }
            else
            {
                m_scratch = 0;
            }
            m_scratchBits = tailBits;
        }

        /**
            Flush any remaining bits to memory.
            Call this once after you've finished writing bits to flush the last word of scratch to memory!
            @see BitWriter::WriteBits
         */

        void FlushBits() serialize_restrict     // restrict qualified this: see WriteBits
        {
            if ( m_scratchBits != 0 )
            {
                serialize_assert( m_data );             // if this fires, the writer was used before Initialize
                serialize_assert( m_scratchBits < 64 );
                const uint64_t word = host_to_network( m_scratch );
                memcpy( m_data + (size_t) m_wordIndex * 8, &word, sizeof( word ) );     // stores a full qword: the buffer size is a multiple of 8 so this stays in bounds, and bytes past the written data are zeros
                m_scratch = 0;
                m_scratchBits = 0;
                m_wordIndex++;
            }
        }

        /**
            How many align bits would be written, if we were to write an align right now?
            @returns Result in [0,7], where 0 is zero bits required to align (already aligned) and 7 is worst case.
         */

        int GetAlignBits() const
        {
            return ( 8 - ( m_bitsWritten % 8 ) ) % 8;
        }

        /**
            How many bits have we written so far?
            @returns The number of bits written to the bit buffer.
         */

        int64_t GetBitsWritten() const
        {
            return m_bitsWritten;
        }

        /**
            How many bits are still available to write?
            For example, if the buffer size is 4, we have 32 bits available to write, if we have already written 10 bytes then 22 are still available to write.
            @returns The number of bits available to write.
         */

        int64_t GetBitsAvailable() const
        {
            return m_numBits - m_bitsWritten;
        }

        /**
            Get a pointer to the data written by the bit writer.
            Corresponds to the data block passed in to the constructor.
            @returns Pointer to the data written by the bit writer.
         */

        const uint8_t * GetData() const
        {
            return (uint8_t*) m_data;
        }

        /**
            The number of bytes flushed to memory.
            This is effectively the size of the packet that you should send after you have finished bitpacking values with this class.
            The returned value is not always a multiple of 8, even though we flush qwords to memory. You won't miss any data in this case because the order of bits written is designed to work with the little endian memory layout.
            IMPORTANT: Make sure you call BitWriter::FlushBits before calling this method, otherwise you risk missing the last word of data.
         */

        int64_t GetBytesWritten() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

    private:

        uint8_t * m_data;               ///< The buffer we are writing to. The buffer size is a multiple of 8, so qword stores always stay in bounds.
        uint64_t m_scratch;             ///< The scratch value where we write bits to (right to left). When it fills to 64 bits it is stored to memory as a qword and the bits that spilled past 64 carry over.
        int64_t m_numBits;              ///< The number of bits in the buffer. This is equivalent to the size of the buffer in bytes multiplied by 8.
        int64_t m_bitsWritten;          ///< The number of bits written so far.
        int64_t m_wordIndex;            ///< The current word index. The next word flushed to memory will be at this index in m_data.
        int m_scratchBits;              ///< The number of valid bits in scratch, in [0,63].
    };

    /**
        Reads bit packed integer values from a buffer.
        Relies on the user reconstructing the exact same set of bit reads as bit writes when the buffer was written. This is an unattributed bitpacked binary stream!
        Implementation: branchless. Each read loads a 64 bit window from the current byte position with memcpy and shifts by the bit remainder.
        There is no scratch state and no refill branch, so reads carry no dependency between calls other than advancing the bit index. This makes the reader significantly faster than a word-at-a-time design.
        IMPORTANT: The buffer allocation must extend at least 8 bytes past the end of the packet data, because the reader loads 8 byte windows at byte granularity. The bytes past the end are loaded but never interpreted.
     */

    class BitReader
    {
    public:

        BitReader()
        {
            m_data = NULL;
            m_numBytes = 0;
            m_numBits = 0;
            m_bitsRead = 0;
        }

        void Initialize( const void * serialize_restrict data, int64_t bytes )
        {
            serialize_assert( data );
            m_data = (const uint8_t*) data;
            m_numBytes = bytes;
            m_numBits = m_numBytes * 8;
            m_bitsRead = 0;
        }

        /**
            Bit reader constructor.
            Any buffer size is supported, as non-multiples of four naturally occur when packets are read from the network.
            IMPORTANT: The actual buffer allocated for the packet data must extend at least 8 bytes past the end of the data, because the reader loads a 64 bit window from the current byte position, and near the end of the stream that window begins inside the final bytes. The bytes past the end are loaded but never interpreted.
            @param data Pointer to the bitpacked data to read. Does not need to be aligned: the reader loads each window with memcpy, which packet payloads require because they typically start at an unaligned offset once the transport header is stripped.
            @param bytes The number of bytes of bitpacked data to read. Buffer sizes are effectively unlimited, because bit counts are stored in 64 bit signed integers.
            @see BitWriter
         */

        BitReader( const void * serialize_restrict data, int64_t bytes ) : m_data( (const uint8_t*) data ), m_numBytes( bytes )
        {
            serialize_assert( data );
            m_numBits = m_numBytes * 8;
            m_bitsRead = 0;
        }

        /**
            Would the bit reader would read past the end of the buffer if it read this many bits?
            @param bits The number of bits that would be read.
            @returns True if reading the number of bits would read past the end of the buffer.
         */

        bool WouldReadPastEnd( int bits ) const
        {
            return m_bitsRead + bits > m_numBits;
        }

        /**
            Read bits from the bit buffer.
            This function will assert in debug builds if this read would read past the end of the buffer.
            In production situations, the higher level ReadStream takes care of checking all packet data and never calling this function if it would read past the end of the buffer.
            @param bits The number of bits to read in [1,32].
            @returns The integer value read in range [0,(1<<bits)-1].
            @see BitReader::WouldReadPastEnd
            @see BitWriter::WriteBits
         */

        SERIALIZE_ALWAYS_INLINE uint32_t ReadBits( int bits )
        {
            serialize_assert( m_data );                 // if this fires, the reader was used before Initialize
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            serialize_assert( m_bitsRead + bits <= m_numBits );

            // loads up to 7 bytes past the last data byte: the allocation contract covers this
            uint64_t window;
            memcpy( &window, m_data + ( m_bitsRead >> 3 ), sizeof( window ) );
            window = network_to_host( window );

            const uint32_t output = uint32_t( window >> ( m_bitsRead & 7 ) ) & uint32_t( ( uint64_t(1) << bits ) - 1 );

            m_bitsRead += bits;

            return output;
        }

        /**
            Read an align.
            Call this on read to correspond to a WriteAlign call when the bitpacked buffer was written.
            This makes sure we skip ahead to the next aligned byte index. As a safety check, we verify that the padding to next byte is zero bits and return false if that's not the case.
            This will typically abort packet read. Just another safety measure...
            @returns True if we successfully read an align and skipped ahead past zero pad, false otherwise (probably means, no align was written to the stream).
            @see BitWriter::WriteAlign
         */

        SERIALIZE_ALWAYS_INLINE bool ReadAlign()
        {
            const int remainderBits = m_bitsRead % 8;
            if ( remainderBits != 0 )
            {
                uint32_t value = ReadBits( 8 - remainderBits );
                serialize_assert( m_bitsRead % 8 == 0 );
                if ( value != 0 )
                    return false;
            }
            return true;
        }

        /**
            Read bytes from the bitpacked data.
            @see BitWriter::WriteBytes
         */

        void ReadBytes( uint8_t * serialize_restrict data, int64_t bytes )
        {
            serialize_assert( m_data );                 // if this fires, the reader was used before Initialize
            serialize_assert( GetAlignBits() == 0 );
            serialize_assert( uint64_t(m_bitsRead) + uint64_t(bytes) * 8 <= uint64_t(m_numBits) );

            // the bit index is byte aligned here (see the align assert), so this is a straight copy
            memcpy( data, m_data + ( m_bitsRead >> 3 ), (size_t) bytes );

            m_bitsRead += bytes * 8;
        }

        /**
            How many align bits would be read, if we were to read an align right now?
            @returns Result in [0,7], where 0 is zero bits required to align (already aligned) and 7 is worst case.
         */

        int GetAlignBits() const
        {
            return ( 8 - m_bitsRead % 8 ) % 8;
        }

        /**
            How many bits have we read so far?
            @returns The number of bits read from the bit buffer so far.
         */

        int64_t GetBitsRead() const
        {
            return m_bitsRead;
        }

        /**
            How many bits are still available to read?
            For example, if the buffer size is 4, we have 32 bits available to read, if we have already written 10 bytes then 22 are still available.
            @returns The number of bits available to read.
         */

        int64_t GetBitsRemaining() const
        {
            return m_numBits - m_bitsRead;
        }

    private:

        const uint8_t * serialize_restrict m_data;          ///< The bitpacked data we're reading. The allocation extends at least 8 bytes past the end of the data.
        int64_t m_numBits;                                  ///< Number of bits to read in the buffer. Of course, we can't *really* know this so it's actually m_numBytes * 8.
        int64_t m_numBytes;                                 ///< Number of bytes to read in the buffer. We know this, and this is the non-rounded up version.
        int64_t m_bitsRead;                                 ///< Number of bits read from the buffer so far. This is the only state the reader carries between reads.
    };

    /**
        Functionality common to all stream classes.
     */

    class BaseStream
    {
    public:

        /**
            Base stream constructor.
         */

        explicit BaseStream() : m_context( NULL ), m_allocator( NULL ) {}

        /**
            Set a context on the stream.
            The context lets you pass data through to your serialize functions, for example lookup tables or min/max ranges needed to read and write values.
            Call BaseStream::GetContext inside your serialize method to retrieve it.
         */

        void SetContext( void * context )
        {
            m_context = context;
        }

        /**
            Get the context pointer set on the stream.

            @returns The context pointer. May be NULL.
         */

        void * GetContext() const
        {
            return m_context;
        }

        /**
            Set an allocator pointer on the stream.
            This can be helpful if you want to perform allocations within serialize functions.
         */

        void SetAllocator( void * allocator )
        {
            m_allocator = allocator;
        }

        /**
            Get the allocator pointer set on the stream.

            @returns The allocator pointer. May be NULL.
         */

        void * GetAllocator() const
        {
            return m_allocator;
        }

    private:

        void * m_context;                           ///< The context pointer set on the stream. May be NULL.
        void * m_allocator;                         ///< The allocator pointer set on the stream. May be NULL.
    };

    /**
        Stream class for writing bitpacked data.
        This class is a wrapper around the bit writer class. Its purpose is to provide unified interface for reading and writing.
        You can determine if you are writing to a stream by calling Stream::IsWriting inside your templated serialize method.
        This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.
        IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead.
        @see BitWriter
     */

    class WriteStream : public BaseStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        WriteStream() : m_writer() {}

        void Initialize( uint8_t * buffer, int64_t bytes )
        {
            m_writer.Initialize( buffer, bytes );
        }

        /**
            Write stream constructor.
            @param buffer The buffer to write to. Does not need to be aligned.
            @param bytes The number of bytes in the buffer. Must be a multiple of 8, because the bit writer stores qwords to memory.
         */

        WriteStream( uint8_t * buffer, int64_t bytes ) : m_writer( buffer, bytes ) {}

        /**
            Serialize an integer (write).
            @param value The integer value in [min,max].
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on write.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            serialize_assert( min <= max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required( min, max );
            if ( bits == 0 )
            {
                return true;                // degenerate range: the value IS the range, nothing to send
            }
            // subtract in the unsigned domain: value - min overflows signed arithmetic when the range is wider than 2^31
            uint32_t unsigned_value = uint32_t(value) - uint32_t(min);
            m_writer.WriteBits( unsigned_value, bits );
            return true;
        }

        /**
            Serialize a 64 bit integer (write).
            @param value The integer value in [min,max].
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on write.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeInteger64( int64_t value, int64_t min, int64_t max )
        {
            serialize_assert( min <= max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required64( uint64_t(min), uint64_t(max) );
            if ( bits == 0 )
            {
                return true;                // degenerate range: the value IS the range, nothing to send
            }
            // subtract in the unsigned domain: value - min overflows signed arithmetic when the range is wider than 2^63
            const uint64_t unsigned_value = uint64_t(value) - uint64_t(min);
            if ( bits <= 32 )
            {
                m_writer.WriteBits( uint32_t( unsigned_value ), bits );
            }
            else
            {
                // low dword first, then the high remainder: same convention as serialize_bits and serialize_uint64
                m_writer.WriteBits( uint32_t( unsigned_value & 0xFFFFFFFF ), 32 );
                m_writer.WriteBits( uint32_t( unsigned_value >> 32 ), bits - 32 );
            }
            return true;
        }

        /**
            Serialize a 128 bit integer (write).
            @param value The integer value in [min,max].
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on write.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeInteger128( int128_t value, int128_t min, int128_t max )
        {
            serialize_assert( min <= max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required128( uint128_t(min), uint128_t(max) );
            // subtract in the unsigned domain: value - min overflows signed arithmetic when the range is wider than 2^127
            const uint128_t unsigned_value = uint128_t(value) - uint128_t(min);
            // 32 bit groups, least significant first: the same convention as serialize_bits, serialize_uint64 and the wide fixed point path
            const uint32_t group0 = uint32_t( uint64_t( unsigned_value       ) & 0xFFFFFFFF );
            const uint32_t group1 = uint32_t( uint64_t( unsigned_value >> 32 ) & 0xFFFFFFFF );
            const uint32_t group2 = uint32_t( uint64_t( unsigned_value >> 64 ) & 0xFFFFFFFF );
            const uint32_t group3 = uint32_t( uint64_t( unsigned_value >> 96 ) & 0xFFFFFFFF );
            if ( bits <= 32 )
            {
                m_writer.WriteBits( group0, bits );
            }
            else if ( bits <= 64 )
            {
                m_writer.WriteBits( group0, 32 );
                m_writer.WriteBits( group1, bits - 32 );
            }
            else if ( bits <= 96 )
            {
                m_writer.WriteBits( group0, 32 );
                m_writer.WriteBits( group1, 32 );
                m_writer.WriteBits( group2, bits - 64 );
            }
            else
            {
                m_writer.WriteBits( group0, 32 );
                m_writer.WriteBits( group1, 32 );
                m_writer.WriteBits( group2, 32 );
                m_writer.WriteBits( group3, bits - 96 );
            }
            return true;
        }

        /**
            Serialize a number of bits (write).
            @param value The unsigned integer value to serialize. Must be in range [0,(1<<bits)-1].
            @param bits The number of bits to write in [1,32].
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeBits( uint32_t value, int bits )
        {
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            m_writer.WriteBits( value, bits );
            return true;
        }

        /**
            Serialize an array of bytes (write).
            @param data Array of bytes to be written.
            @param bytes The number of bytes to write.
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeBytes( const uint8_t * data, int64_t bytes )
        {
            serialize_assert( data );
            serialize_assert( bytes >= 0 );
            SerializeAlign();
            m_writer.WriteBytes( data, bytes );
            return true;
        }

        /**
            Serialize an align (write).
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeAlign()
        {
            m_writer.WriteAlign();
            return true;
        }

        /**
            If we were to write an align right now, how many bits would be required?
            @returns The number of zero pad bits required to achieve byte alignment in [0,7].
         */

        int GetAlignBits() const
        {
            return m_writer.GetAlignBits();
        }

        /**
            Flush the stream to memory after you finish writing.
            Always call this after you finish writing and before you call WriteStream::GetData, or you'll potentially truncate the last word of data you wrote.
            @see BitWriter::FlushBits
         */

        void Flush()
        {
            m_writer.FlushBits();
        }

        /**
            Get a pointer to the data written by the stream.
            IMPORTANT: Call WriteStream::Flush before you call this function!
            @returns A pointer to the data written by the stream
         */

        const uint8_t * GetData() const
        {
            return m_writer.GetData();
        }

        /**
            How many bytes have been written so far?
            @returns Number of bytes written. This is effectively the packet size.
         */

        int64_t GetBytesProcessed() const
        {
            return m_writer.GetBytesWritten();
        }

        /**
            Get number of bits written so far.
            @returns Number of bits written.
         */

        int64_t GetBitsProcessed() const
        {
            return m_writer.GetBitsWritten();
        }

    private:

        BitWriter m_writer;                 ///< The bit writer used for all bitpacked write operations.
    };

    /**
        Stream class for reading bitpacked data.
        This class is a wrapper around the bit reader class. Its purpose is to provide unified interface for reading and writing.
        You can determine if you are reading from a stream by calling Stream::IsReading inside your templated serialize method.
        This is evaluated at compile time, letting the compiler generate optimized serialize functions without the hassle of maintaining separate read and write functions.
        IMPORTANT: Generally, you don't call methods on this class directly. Use the serialize_* macros instead.
        @see BitReader
     */

    class ReadStream : public BaseStream
    {
    public:

        enum { IsWriting = 0 };
        enum { IsReading = 1 };

        ReadStream()
        {
            // ...
        }

        void Initialize( const uint8_t * buffer, int64_t bytes )
        {
            m_reader.Initialize( buffer, bytes );
        }

        /**
            Read stream constructor.
            @param buffer The buffer to read from.
            @param bytes The number of bytes of packet data to read. IMPORTANT: the underlying allocation must extend at least 8 bytes past the end of the data, because the bit reader loads 64 bit windows at byte granularity. See BitReader for details.
         */

        ReadStream( const uint8_t * buffer, int64_t bytes ) : m_reader( buffer, bytes ) {}

        /**
            Serialize an integer (read).
            @param value The integer value read is stored here. It is guaranteed to be in [min,max] if this function succeeds.
            @param min The minimum allowed value.
            @param max The maximum allowed value.
            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            serialize_assert( min <= max );
            const int bits = bits_required( min, max );
            if ( bits == 0 )
            {
                value = min;                // degenerate range: the value IS the range
                return true;
            }
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint32_t unsigned_value = m_reader.ReadBits( bits );
            if ( unsigned_value > uint32_t(max) - uint32_t(min) )
                return false;
            // add in the unsigned domain: unsigned_value + min overflows signed arithmetic when the range is wider than 2^31
            value = int32_t( unsigned_value + uint32_t(min) );
            return true;
        }

        /**
            Serialize a 64 bit integer (read).
            @param value The integer value read is stored here. It is guaranteed to be in [min,max] if this function succeeds.
            @param min The minimum allowed value.
            @param max The maximum allowed value.
            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeInteger64( int64_t & value, int64_t min, int64_t max )
        {
            serialize_assert( min <= max );
            const int bits = bits_required64( uint64_t(min), uint64_t(max) );
            if ( bits == 0 )
            {
                value = min;                // degenerate range: the value IS the range
                return true;
            }
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint64_t unsigned_value;
            if ( bits <= 32 )
            {
                unsigned_value = m_reader.ReadBits( bits );
            }
            else
            {
                // low dword first, then the high remainder: same convention as serialize_bits and serialize_uint64
                const uint32_t lo = m_reader.ReadBits( 32 );
                const uint32_t hi = m_reader.ReadBits( bits - 32 );
                unsigned_value = ( uint64_t(hi) << 32 ) | lo;
            }
            if ( unsigned_value > uint64_t(max) - uint64_t(min) )
                return false;
            // add in the unsigned domain: unsigned_value + min overflows signed arithmetic when the range is wider than 2^63
            value = int64_t( unsigned_value + uint64_t(min) );
            return true;
        }

        /**
            Serialize a 128 bit integer (read).
            @param value The integer value read is stored here. It is guaranteed to be in [min,max] if this function succeeds.
            @param min The minimum allowed value.
            @param max The maximum allowed value.
            @returns Returns true if the serialize succeeded and the value is in the correct range. False otherwise.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeInteger128( int128_t & value, int128_t min, int128_t max )
        {
            serialize_assert( min <= max );
            const int bits = bits_required128( uint128_t(min), uint128_t(max) );
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            // 32 bit groups, least significant first: the same convention as the write path
            uint32_t group0 = 0;
            uint32_t group1 = 0;
            uint32_t group2 = 0;
            uint32_t group3 = 0;
            if ( bits <= 32 )
            {
                group0 = m_reader.ReadBits( bits );
            }
            else if ( bits <= 64 )
            {
                group0 = m_reader.ReadBits( 32 );
                group1 = m_reader.ReadBits( bits - 32 );
            }
            else if ( bits <= 96 )
            {
                group0 = m_reader.ReadBits( 32 );
                group1 = m_reader.ReadBits( 32 );
                group2 = m_reader.ReadBits( bits - 64 );
            }
            else
            {
                group0 = m_reader.ReadBits( 32 );
                group1 = m_reader.ReadBits( 32 );
                group2 = m_reader.ReadBits( 32 );
                group3 = m_reader.ReadBits( bits - 96 );
            }
            const uint128_t unsigned_value = ( uint128_t( group3 ) << 96 ) | ( uint128_t( group2 ) << 64 ) | ( uint128_t( group1 ) << 32 ) | uint128_t( group0 );
            if ( unsigned_value > uint128_t(max) - uint128_t(min) )
                return false;
            // add in the unsigned domain: unsigned_value + min overflows signed arithmetic when the range is wider than 2^127
            value = int128_t( unsigned_value + uint128_t(min) );
            return true;
        }

        /**
            Serialize a number of bits (read).
            @param value The integer value read is stored here. Will be in range [0,(1<<bits)-1].
            @param bits The number of bits to read in [1,32].
            @returns Returns true if the serialize read succeeded, false otherwise.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeBits( uint32_t & value, int bits )
        {
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            if ( m_reader.WouldReadPastEnd( bits ) )
                return false;
            uint32_t read_value = m_reader.ReadBits( bits );
            value = read_value;
            return true;
        }

        /**
            Serialize an array of bytes (read).
            @param data Array of bytes to read.
            @param bytes The number of bytes to read.
            @returns Returns true if the serialize read succeeded. False otherwise.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeBytes( uint8_t * data, int64_t bytes )
        {
            if ( bytes < 0 )
                return false;
            if ( !SerializeAlign() )
                return false;
            // compare in bytes rather than bits, consistent with the 64 bit bookkeeping
            if ( bytes > m_reader.GetBitsRemaining() / 8 )
                return false;
            m_reader.ReadBytes( data, bytes );
            return true;
        }

        /**
            Serialize an align (read).
            @returns Returns true if the serialize read succeeded. False otherwise.
         */

        SERIALIZE_ALWAYS_INLINE bool SerializeAlign()
        {
            const int alignBits = m_reader.GetAlignBits();
            if ( m_reader.WouldReadPastEnd( alignBits ) )
                return false;
            if ( !m_reader.ReadAlign() )
                return false;
            return true;
        }

        /**
            If we were to read an align right now, how many bits would we need to read?
            @returns The number of zero pad bits required to achieve byte alignment in [0,7].
         */

        int GetAlignBits() const
        {
            return m_reader.GetAlignBits();
        }

        /**
            Get number of bits read so far.
            @returns Number of bits read.
         */

        int64_t GetBitsProcessed() const
        {
            return m_reader.GetBitsRead();
        }

        /**
            How many bytes have been read so far?
            @returns Number of bytes read. Effectively this is the number of bits read, rounded up to the next byte where necessary.
         */

        int64_t GetBytesProcessed() const
        {
            return ( m_reader.GetBitsRead() + 7 ) / 8;
        }

    private:

        BitReader m_reader;             ///< The bit reader used for all bitpacked read operations.
    };

    /**
        Stream class for estimating how many bits it would take to serialize something.
        This class acts like a bit writer (IsWriting is 1, IsReading is 0), but instead of writing data, it counts how many bits would be written.
        Note that when the serialization includes alignment to byte (see MeasureStream::SerializeAlign), this is an estimate and not an exact measurement. The estimate is guaranteed to be conservative.
        @see BitWriter
        @see BitReader
     */

    class MeasureStream : public BaseStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        /**
            Measure stream constructor.
         */

        explicit MeasureStream() : m_bitsWritten(0) {}

        /**
            Serialize an integer (measure).
            @param value The integer value to write. Not actually used or checked.
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on measure.
         */

        bool SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            (void) value;
            serialize_assert( min <= max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required( min, max );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize a 64 bit integer (measure).
            @param value The integer value to write. Not actually used or checked.
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts only on measure.
         */

        bool SerializeInteger64( int64_t value, int64_t min, int64_t max )
        {
            (void) value;
            serialize_assert( min <= max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required64( uint64_t(min), uint64_t(max) );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize a 128 bit integer (measure).
            @param value The integer value to measure. Not actually used or checked beyond the debug asserts.
            @param min The minimum value.
            @param max The maximum value.
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeInteger128( int128_t value, int128_t min, int128_t max )
        {
            (void) value;
            serialize_assert( min <= max );
            serialize_assert( value >= min );
            serialize_assert( value <= max );
            const int bits = bits_required128( uint128_t(min), uint128_t(max) );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize a number of bits (write).
            @param value The unsigned integer value to serialize. Not actually used or checked.
            @param bits The number of bits to write in [1,32].
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBits( uint32_t value, int bits )
        {
            (void) value;
            serialize_assert( bits > 0 );
            serialize_assert( bits <= 32 );
            m_bitsWritten += bits;
            return true;
        }

        /**
            Serialize an array of bytes (measure).
            @param data Array of bytes to 'write'. Not actually used.
            @param bytes The number of bytes to 'write'.
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeBytes( const uint8_t * data, int64_t bytes )
        {
            (void) data;
            serialize_assert( bytes >= 0 );
            SerializeAlign();
            m_bitsWritten += bytes * 8;
            return true;
        }

        /**
            Serialize an align (measure).
            @returns Always returns true. All checking is performed by debug asserts on write.
         */

        bool SerializeAlign()
        {
            const int alignBits = GetAlignBits();
            m_bitsWritten += alignBits;
            return true;
        }

        /**
            If we were to write an align right now, how many bits would be required?
            IMPORTANT: Since the number of bits required for alignment depends on where an object is written in the final bit stream, this measurement is conservative.
            @returns Always returns worst case 7 bits.
         */

        int GetAlignBits() const
        {
            return 7;
        }

        /**
            Get number of bits written so far.
            @returns Number of bits written.
         */

        int64_t GetBitsProcessed() const
        {
            return m_bitsWritten;
        }

        /**
            How many bytes have been written so far?
            @returns Number of bytes written.
         */

        int64_t GetBytesProcessed() const
        {
            return ( m_bitsWritten + 7 ) / 8;
        }

    private:

        int64_t m_bitsWritten;          ///< Counts the number of bits written.
    };

    /**
        Serialize integer value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The integer value to serialize in [min,max].
        @param min The minimum value.
        @param max The maximum value.
     */

    #define serialize_int( stream, value, min, max )                    \
        do                                                              \
        {                                                               \
            serialize_assert( (min) <= (max) );                          \
            int32_t int32_value = 0;                                    \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                serialize_assert( int64_t(value) >= int64_t(min) );     \
                serialize_assert( int64_t(value) <= int64_t(max) );     \
                int32_value = (int32_t) ( value );                      \
            }                                                           \
            if ( !stream.SerializeInteger( int32_value, min, max ) )    \
            {                                                           \
                return false;                                           \
            }                                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = int32_value;                                    \
                if ( int64_t(value) < int64_t(min) ||                   \
                     int64_t(value) > int64_t(max) )                    \
                {                                                       \
                    return false;                                       \
                }                                                       \
            }                                                           \
        } while (0)

    /**
        Serialize a 64 bit integer value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        The full 64 bit range is supported, and the minimal number of bits for [min,max] is used on the wire.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The 64 bit integer value to serialize in [min,max].
        @param min The minimum value.
        @param max The maximum value.
     */

    #define serialize_int64( stream, value, min, max )                  \
        do                                                              \
        {                                                               \
            serialize_assert( int64_t(min) <= int64_t(max) );            \
            int64_t int64_value = 0;                                    \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                serialize_assert( int64_t(value) >= int64_t(min) );     \
                serialize_assert( int64_t(value) <= int64_t(max) );     \
                int64_value = (int64_t) ( value );                      \
            }                                                           \
            if ( !stream.SerializeInteger64( int64_value, min, max ) )  \
            {                                                           \
                return false;                                           \
            }                                                           \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = int64_value;                                    \
                if ( int64_t(value) < int64_t(min) ||                   \
                     int64_t(value) > int64_t(max) )                    \
                {                                                       \
                    return false;                                       \
                }                                                       \
            }                                                           \
        } while (0)

    /**
        Serialize a ranged 128 bit integer to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        The value is serialized as an offset from min in the minimal number of bits for the range, exactly as serialize_int and serialize_int64 do at their widths, and the bounds are runtime values. Where the range fits 64 bits or fewer the bytes are identical to serialize_int64 over the same bounds.
        The bounds and value are serialize::int128_t, which exists on every platform: native __int128 where the compiler provides it, the emulated pair where it doesn't. The bit count comes from the runtime serialize::bits_required128, so this works on pure MSVC — unlike the compile time serialize::BitsRequired128, which needs native __int128.
        **Do not confuse this with serialize_uint128**, which is not ranged — it is a raw 128 bit field and always costs 128 bits.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The 128 bit integer value to serialize, in [min,max].
        @param min The minimum value.
        @param max The maximum value.
     */

    #define serialize_int128( stream, value, min, max )                                             \
        do                                                                                          \
        {                                                                                           \
            serialize_assert( serialize::int128_t(min) < serialize::int128_t(max) );                \
            serialize::int128_t int128_value = 0;                                                   \
            if ( Stream::IsWriting )                                                                \
            {                                                                                       \
                serialize_assert( serialize::int128_t(value) >= serialize::int128_t(min) );         \
                serialize_assert( serialize::int128_t(value) <= serialize::int128_t(max) );         \
                int128_value = serialize::int128_t( value );                                        \
            }                                                                                       \
            if ( !stream.SerializeInteger128( int128_value, min, max ) )                            \
            {                                                                                       \
                return false;                                                                       \
            }                                                                                       \
            if ( Stream::IsReading )                                                                \
            {                                                                                       \
                value = int128_value;                                                               \
                if ( serialize::int128_t(value) < serialize::int128_t(min) ||                       \
                     serialize::int128_t(value) > serialize::int128_t(max) )                        \
                {                                                                                   \
                    return false;                                                                   \
                }                                                                                   \
            }                                                                                       \
        } while (0)

    /**
        Serialize bits to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned integer value to serialize.
        @param bits The number of bits to serialize in [1,32].
     */

    #define serialize_bits( stream, value, bits )                       \
        do                                                              \
        {                                                               \
            serialize_assert( (bits) > 0 );                             \
            serialize_assert( (bits) <= 64 );                           \
            if ( (bits) <= 32 )                                         \
            {                                                           \
                uint32_t uint32_value = 0;                              \
                if ( Stream::IsWriting )                                \
                {                                                       \
                    uint32_value = (uint32_t) ( value );                \
                }                                                       \
                if ( !stream.SerializeBits( uint32_value, bits ) )      \
                {                                                       \
                    return false;                                       \
                }                                                       \
                if ( Stream::IsReading )                                \
                {                                                       \
                    value = uint32_value;                               \
                }                                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                uint32_t hi = 0, lo = 0;                                \
                if ( Stream::IsWriting )                                \
                {                                                       \
                    lo = uint32_t( uint64_t(value) & 0xFFFFFFFF );      \
                    hi = uint32_t( uint64_t(value) >> 32 );             \
                }                                                       \
                if ( !stream.SerializeBits( lo, 32 ) )                  \
                {                                                       \
                    return false;                                       \
                }                                                       \
                if ( !stream.SerializeBits( hi, (bits) - 32 ) )         \
                {                                                       \
                    return false;                                       \
                }                                                       \
                if ( Stream::IsReading )                                \
                {                                                       \
                    value = ( uint64_t(hi) << 32 ) | lo;                \
                }                                                       \
            }                                                           \
        } while (0)


    /**
        Serialize a boolean value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The boolean value to serialize.
     */

    #define serialize_bool( stream, value )                             \
        do                                                              \
        {                                                               \
            uint32_t uint32_bool_value = 0;                             \
            if ( Stream::IsWriting )                                    \
            {                                                           \
                uint32_bool_value = ( value ) ? 1 : 0;                  \
            }                                                           \
            serialize_bits( stream, uint32_bool_value, 1 );             \
            if ( Stream::IsReading )                                    \
            {                                                           \
                value = uint32_bool_value ? true : false;               \
            }                                                           \
        } while (0)

    template <typename Stream> SERIALIZE_ALWAYS_INLINE bool serialize_float_internal( Stream & stream, float & value )
    {
        uint32_t int_value = 0;
        if ( Stream::IsWriting )
        {
            memcpy( (char*) &int_value, &value, 4 );
        }
        bool result = stream.SerializeBits( int_value, 32 );
        if ( Stream::IsReading )
        {
            memcpy( (char*) &value, &int_value, 4 );
        }
        return result;
    }

    /**
        Serialize floating point value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The float value to serialize.
     */

    #define serialize_float( stream, value )                                        \
        do                                                                          \
        {                                                                           \
            if ( !serialize::serialize_float_internal( stream, value ) )            \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    /**
        Derive the compressed float wire constants from a (min,max,res) declaration.
        This is the derivation serialize_compressed_float performs on every call, exposed so it can be paid once instead: the constants depend only on the declaration, never on the value, so a schema compiler runs the same derivation at code generation time and passes the results to serialize_compressed_float_precomputed at every call site.
        serialize_compressed_float itself derives with exactly this function and forwards to exactly that entry point, so the two entry points are wire identical by construction.
        A declaration whose delta = max - min, or whose delta / res, is not finite in float32 is non-conforming (STANDARD.md, adopted 2026-08-15) and asserts in debug builds.
        @param min The minimum float value. Must be less than max.
        @param max The maximum float value.
        @param res The resolution the float value is quantized to.
        @param max_integer_value The quantization step count: ceil( ( max - min ) / res ), clamped to [1,4294967040]. Values quantize to integers in [0,max_integer_value].
        @param bits The wire width: bits_required( 0, max_integer_value ), the number of bits a quantized value occupies on the wire, in [1,32].
        @param delta The range width max - min, computed in float32. The quantization arithmetic is pinned to float32, so the wire depends on this exact value, not on the real-number difference.
     */

    inline void serialize_compressed_float_params( float min, float max, float res, uint32_t & max_integer_value, int & bits, float & delta )
    {
        serialize_assert( min < max && res > 0 );

        delta = max - min;

        float values = delta / res;

        // a declaration whose delta or values is not finite in float32 is non-conforming
        // (STANDARD.md, adopted 2026-08-15) -- assert in debug, per the writer-trusted model.
        // finiteness is spelled x - x == 0 (NaN and both infinities fail it: Inf - Inf is NaN)
        // so the header needs no isfinite and no new include.
        serialize_assert( delta - delta == 0.0f );
        serialize_assert( values - values == 0.0f );

        // clamp so the uint32_t cast below is defined even for pathological delta / res (the !>= form also catches NaN)
        if ( !( values >= 1.0f ) )
        {
            values = 1.0f;
        }
        else if ( values > 4294967040.0f )      // largest float below 2^32
        {
            values = 4294967040.0f;
        }

        max_integer_value = (uint32_t) ceil(values);

        bits = bits_required( 0, max_integer_value );
    }

    /**
        Serialize compressed floating point value from precomputed wire constants (read/write/measure).
        The audited home of the compressed float quantization arithmetic. serialize_compressed_float derives its constants per call and forwards here; generated code passes constants a schema compiler derived at generation time with the same arithmetic as serialize::serialize_compressed_float_params, skipping the per-field divide, clamp, ceil and bits_required. The two entry points are wire identical by construction, and test_compressed_float_precomputed_differential holds both, plus a frozen copy of the pre-split function, to byte and bit identity across the declaration corpus.
        On write, the value is clamped into the declared range and quantized; writing a non-finite value is non-conforming and asserts in debug (STANDARD.md). On read, an integer above max_integer_value smuggled into the bit headroom is rejected and the function returns false.
        The constants must be exactly what serialize_compressed_float_params derives for a conforming declaration — anything else is a caller bug, debug asserted per the writer-trusted model.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The float value to serialize. Written on write/measure, filled in on read.
        @param max_integer_value The quantization step count, in [1,4294967040].
        @param bits The wire width in bits. Must equal serialize::bits_required( 0, max_integer_value ).
        @param delta The range width max - min, in float32.
        @param min The minimum float value of the range.
        @returns True if the serialize succeeded, false if the read data is truncated or encodes an integer above max_integer_value.
     */

    template <typename Stream> bool serialize_compressed_float_precomputed_internal( Stream & stream, float & value, uint32_t max_integer_value, int bits, float delta, float min )
    {
        serialize_assert( max_integer_value >= 1 );
        serialize_assert( bits == bits_required( 0, max_integer_value ) );
        serialize_assert( delta > 0.0f );
        serialize_assert( delta - delta == 0.0f );          // finite in float32 (Inf - Inf is NaN)

        uint32_t integerValue = 0;

        if ( Stream::IsWriting )
        {
            // writing a non-finite value (NaN, +/-Inf) through compressed_float is
            // non-conforming (STANDARD.md, adopted 2026-08-15) -- assert in debug. in release
            // the clamp below remains the backstop that keeps the uint32 cast defined.
            serialize_assert( value - value == 0.0f );

            // clamp with the !>= / !<= form so a NaN value is forced into range instead of reaching the uint32 cast below
            float normalizedValue = (value - min) / delta;
            if ( !( normalizedValue >= 0.0f ) )
            {
                normalizedValue = 0.0f;
            }
            else if ( !( normalizedValue <= 1.0f ) )
            {
                normalizedValue = 1.0f;
            }
            // STANDARD.md pins this to float32 with TWO roundings: the product
            // rounds before 0.5 is added. The local plus the barrier are what
            // force that, and neither is optional. Written as one expression, a
            // compiler permitted to contract (clang's default is
            // -ffp-contract=on) emits a single FMA and rounds ONCE -- and that
            // changes the wire. On arm64 at -O2 this quantized 0.005 over
            // [0, 10] at resolution 0.01 to 0 where every conformant runtime
            // writes 1; on x86-64 the same source did not contract and wrote 1,
            // so the goldens (generated on x86) and CI stayed green while
            // Apple Silicon emitted different bytes. The local alone is not
            // enough: under cross-statement contraction (-ffp-contract=fast,
            // GCC's default) the compiler fuses straight through a plain float
            // local, which is how a consumer building this header at ordinary
            // -O2 defaults shipped divergent arm64 wire (issue #92). The
            // barrier closes that: the wire bits are identical under every
            // contraction mode. Do not fold this back into one expression, and
            // do not remove the barrier -- the -ffp-contract=fast test build
            // and the write-side negative control both go red if either
            // rounding is ever lost.
            float scaled = normalizedValue * max_integer_value;
            SERIALIZE_FLOAT_FORCE_ROUND( scaled );
            integerValue = (uint32_t) floor( scaled + 0.5f );
            // STANDARD.md: the integer clamp is normative (2026-08-23). Once
            // max_integer_value >= 2^23 the float32 ulp at the top of the range
            // reaches 1, so the rounded sum can exceed max_integer_value itself:
            // the writer emits a code its own reader rejects, or one bit wider
            // than the field. Clamping after the floor closes both; no byte
            // changes for any declaration outside [2^23, 2^24).
            if ( integerValue > max_integer_value )
            {
                integerValue = (uint32_t) max_integer_value;
            }
        }

        if ( !stream.SerializeBits( integerValue, bits ) )
        {
            return false;
        }

        if ( Stream::IsReading )
        {
            if ( integerValue > max_integer_value )
            {
                return false;
            }
            const float normalizedValue = integerValue / float(max_integer_value);
            // The reader rounds twice for the same reason the writer above does:
            // the product must round to float32 BEFORE min is added, and the
            // local plus the barrier are what force that. Written as one
            // expression, a compiler permitted to contract (clang's default
            // -ffp-contract=on is enough — no -ffast-math required) fuses the
            // multiply and add into a single FMA on arm64 and rounds ONCE. That
            // decodes a float one ulp away from every conformant runtime whenever
            // min is non-zero, so a value read on arm64 and re-encoded produces
            // different wire. The local alone is not enough: under
            // cross-statement contraction (-ffp-contract=fast, GCC's default at
            // every -O level) the compiler fuses straight through it — measured
            // on arm64: 6666/20000 * 200 - 100 fused decodes 0xC2055C29 where
            // every conformant runtime decodes 0xC2055C2A (issues #92/#95). The
            // barrier closes that: the decoded bit patterns are identical under
            // every contraction mode. The conformance vector with min = -100
            // pins them exactly, and the -ffp-contract=fast test build and the
            // read-side negative control both go red if this is ever folded
            // back into one expression or the barrier removed.
            float scaledValue = normalizedValue * delta;
            SERIALIZE_FLOAT_FORCE_ROUND( scaledValue );
            value = scaledValue + min;
        }

        return true;
    }

    template <typename Stream> bool serialize_compressed_float_internal( Stream & stream, float & value, float min, float max, float res )
    {
        // derive the wire constants, then run the one audited home of the quantization
        // arithmetic. this body IS the pre-#82 function, split at the line schema issue
        // mas-bandwidth/schema#82 names: everything that depends only on the declaration
        // lives in serialize_compressed_float_params, everything that touches the value or
        // the wire lives in serialize_compressed_float_precomputed_internal, statement for
        // statement. test_compressed_float_precomputed_differential holds this composition
        // to byte and bit identity against a frozen copy of the original unsplit body.
        uint32_t max_integer_value = 0;
        int bits = 0;
        float delta = 0.0f;
        serialize_compressed_float_params( min, max, res, max_integer_value, bits, delta );
        return serialize_compressed_float_precomputed_internal( stream, value, max_integer_value, bits, delta, min );
    }

    /**
        Serialize compressed floating point value (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The float value to serialize.
     */

    #define serialize_compressed_float(stream, value, min, max, res)                                \
    do                                                                                              \
    {                                                                                               \
        if ( !serialize::serialize_compressed_float_internal( stream, value, min, max, res) )       \
        {                                                                                           \
            return false;                                                                           \
        }                                                                                           \
    } while (0)

    /**
        Serialize compressed floating point value from precomputed wire constants (read/write/measure).
        The precomputed companion to serialize_compressed_float, designed for generated code: a schema compiler derives max_integer_value, bits and delta from the declaration at code generation time with the same arithmetic as serialize::serialize_compressed_float_params and passes them as literals, so the per-field derivation (a divide, a clamp, a ceil and a bits_required) is never paid at runtime. Wire bytes are identical to serialize_compressed_float by construction.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The float value to serialize.
        @param max_integer_value The quantization step count, exactly as serialize::serialize_compressed_float_params derives it from the declaration.
        @param bits The wire width in bits: serialize::bits_required( 0, max_integer_value ).
        @param delta The range width max - min, in float32.
        @param min The minimum float value of the range.
     */

    #define serialize_compressed_float_precomputed( stream, value, max_integer_value, bits, delta, min )            \
    do                                                                                                              \
    {                                                                                                               \
        if ( !serialize::serialize_compressed_float_precomputed_internal( stream, value, max_integer_value, bits, delta, min ) ) \
        {                                                                                                           \
            return false;                                                                                           \
        }                                                                                                           \
    } while (0)

    template <typename Stream> SERIALIZE_ALWAYS_INLINE bool serialize_double_internal( Stream & stream, double & value )
    {
        union DoubleInt
        {
            double double_value;
            uint64_t int_value;
        };
        DoubleInt tmp = { 0 };
        if ( Stream::IsWriting )
        {
            tmp.double_value = value;
        }
        serialize_bits( stream, tmp.int_value, 64 );
        if ( Stream::IsReading )
        {
            value = tmp.double_value;
        }
        return true;
    }

    /**
        Serialize double precision floating point value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The double precision floating point value to serialize.
     */

    #define serialize_double( stream, value )                                       \
        do                                                                          \
        {                                                                           \
            if ( !serialize::serialize_double_internal( stream, value ) )           \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    template <typename Stream> SERIALIZE_ALWAYS_INLINE bool serialize_bytes_internal( Stream & stream, uint8_t * data, int64_t bytes )
    {
        return stream.SerializeBytes( data, bytes );
    }

    /**
        Serialize unsigned 8 bit integer (read/write/measure).
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 16 bit integer value.
     */

    #define serialize_uint8( stream, value ) serialize_bits( stream, value, 8 )

    /**
        Serialize unsigned 16 bit integer (read/write/measure).
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 16 bit integer value.
     */

    #define serialize_uint16( stream, value ) serialize_bits( stream, value, 16 )

    /**
        Serialize unsigned 32 bit integer (read/write/measure).
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 32 bit integer value.
     */

    #define serialize_uint32( stream, value ) serialize_bits( stream, value, 32 )

    /**
        Serialize unsigned 64 bit integer (read/write/measure).
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 64 bit integer value.
     */

    #define serialize_uint64( stream, value ) serialize_bits( stream, value, 64 )

    /**
        Serialize a 128 bit unsigned integer (read/write/measure), templated over the 128 bit type.
        The wire format is 128 bits raw: the low 64 bit half first, then the high half, following the lo-then-hi convention of serialize_bits.
        UInt128 may be the native unsigned __int128 (see serialize::uint128_t) or an emulated 128 bit unsigned integer type on compilers without native support, such as pure MSVC.
        The requirements on UInt128 are exactly: construction from uint64_t, operator\<\< and operator\>\> with int shift counts, operator|, and explicit conversion (static_cast) to uint64_t truncating to the low 64 bits.
        This function is a template and only compiles when instantiated, so it is available on every compiler; only the serialize::uint128_t convenience typedef needs the __SIZEOF_INT128__ guard.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The 128 bit unsigned integer value.
        @returns True if the serialize succeeded, false otherwise.
     */

    template <typename Stream, typename UInt128> bool serialize_uint128_internal( Stream & stream, UInt128 & value )
    {
        // refuse narrower types at compile time: a uint64_t here would shift by 64 — undefined
        // behavior — and silently is not the 128 bit operation the caller asked for
        serialize_static_assert( sizeof( UInt128 ) == 16, "serialize_uint128 requires a 128 bit type (did you mean serialize_uint64?)" );

        uint64_t low_half = 0;
        uint64_t high_half = 0;
        if ( Stream::IsWriting )
        {
            low_half = uint64_t( value );
            high_half = uint64_t( value >> 64 );
        }
        serialize_bits( stream, low_half, 64 );
        serialize_bits( stream, high_half, 64 );
        if ( Stream::IsReading )
        {
            value = ( UInt128( high_half ) << 64 ) | UInt128( low_half );
        }
        return true;
    }

    /**
        Serialize unsigned 128 bit integer (read/write/measure).
        The wire format is 128 bits raw: the low 64 bit half first, then the high half, following the lo-then-hi convention of serialize_bits.
        The value may be serialize::uint128_t — which exists on every platform: native unsigned __int128 where the compiler provides it, the emulated serialize_uint128_t elsewhere — or any 128 bit unsigned integer type meeting the requirements documented on serialize_uint128_internal. All representations produce identical bytes.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The 128 bit unsigned integer value.
     */

    #define serialize_uint128( stream, value )                                      \
        do                                                                          \
        {                                                                           \
            if ( !serialize::serialize_uint128_internal( stream, value ) )          \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    /**
        Serialize an array of bytes to the stream (read/write/measure).
        This is a helper macro to make unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param data Pointer to the data to be serialized.
        @param bytes The number of bytes to serialize.
     */

    #define serialize_bytes( stream, data, bytes )                                  \
        do                                                                          \
        {                                                                           \
            if ( !serialize::serialize_bytes_internal( stream, data, bytes ) )      \
            {                                                                       \
                return false;                                                       \
            }                                                                       \
        } while (0)

    /*
        UTF-8 well-formedness, one validator with two callers (STANDARD.md, adopted
        2026-08-15): the WRITE path's contract check — a debug-only assert per the
        writes-trusted doctrine, compiled out under NDEBUG — and the READ path's
        refusal rule ("Readers must refuse malformed string payloads"), which binds
        in every build mode, because the read side faces untrusted data. Rejects
        overlong encodings, surrogate code points, values above U+10FFFF, truncated
        sequences and stray continuation bytes. NUL bytes are VALID UTF-8: the
        interior-NUL refusal is a separate rule with its own reason.
    */

    inline bool serialize_string_is_valid_utf8( const char * string, int length )
    {
        int i = 0;
        while ( i < length )
        {
            const uint32_t lead = (uint8_t) string[i];
            if ( lead < 0x80 )
            {
                i += 1;
            }
            else if ( ( lead & 0xE0 ) == 0xC0 )
            {
                if ( lead < 0xC2 )                                                              // overlong
                    return false;
                if ( i + 1 >= length )
                    return false;
                if ( ( (uint8_t) string[i+1] & 0xC0 ) != 0x80 )
                    return false;
                i += 2;
            }
            else if ( ( lead & 0xF0 ) == 0xE0 )
            {
                if ( i + 2 >= length )
                    return false;
                const uint32_t byte1 = (uint8_t) string[i+1];
                const uint32_t byte2 = (uint8_t) string[i+2];
                if ( ( byte1 & 0xC0 ) != 0x80 || ( byte2 & 0xC0 ) != 0x80 )
                    return false;
                if ( lead == 0xE0 && byte1 < 0xA0 )                                             // overlong
                    return false;
                if ( lead == 0xED && byte1 >= 0xA0 )                                            // surrogate code point
                    return false;
                i += 3;
            }
            else if ( ( lead & 0xF8 ) == 0xF0 )
            {
                if ( lead > 0xF4 )                                                              // above U+10FFFF
                    return false;
                if ( i + 3 >= length )
                    return false;
                const uint32_t byte1 = (uint8_t) string[i+1];
                const uint32_t byte2 = (uint8_t) string[i+2];
                const uint32_t byte3 = (uint8_t) string[i+3];
                if ( ( byte1 & 0xC0 ) != 0x80 || ( byte2 & 0xC0 ) != 0x80 || ( byte3 & 0xC0 ) != 0x80 )
                    return false;
                if ( lead == 0xF0 && byte1 < 0x90 )                                             // overlong
                    return false;
                if ( lead == 0xF4 && byte1 >= 0x90 )                                            // above U+10FFFF
                    return false;
                i += 4;
            }
            else
            {
                return false;                                                                   // continuation or invalid lead byte
            }
        }
        return true;
    }

    template <typename Stream> bool serialize_string_internal( Stream & stream, char * string, int buffer_size )
    {
        int length = 0;
        if ( Stream::IsWriting )
        {
            length = (int) strlen( string );
            serialize_assert( length < buffer_size );
            // the writer's contract, debug only. See serialize_string_is_valid_utf8.
            serialize_assert( serialize_string_is_valid_utf8( string, length ) );
        }
        serialize_int( stream, length, 0, buffer_size - 1 );
        serialize_bytes( stream, (uint8_t*)string, length );
        if ( Stream::IsReading )
        {
            // STANDARD.md, "Readers must refuse malformed string payloads" (adopted
            // 2026-08-15). Interior NUL first: a conforming writer derives the length
            // from strlen, so a zero byte among the transmitted bytes only arrives
            // doctored — and it gives the payload TWO lengths, the wire length and the
            // strlen every consumer downstream computes, with everything between them
            // riding invisibly past whichever side uses the other. NUL is valid UTF-8,
            // so the validator below cannot catch it.
            for ( int i = 0; i < length; i++ )
            {
                if ( string[i] == '\0' )
                {
                    return false;
                }
            }
            if ( !serialize_string_is_valid_utf8( string, length ) )
            {
                return false;
            }
            string[length] = '\0';
        }
        return true;
    }

    // Counts the UTF-16 code units a wide string transmits — which on a 4 byte wchar_t
    // platform is more than its character count when astral text is present, and is the
    // count the wstring length field carries (STANDARD.md: each 32 bit group is one UTF-16
    // code unit). Shared by write and measure, so both agree on cost.

    inline int serialize_wstring_unit_count( const wchar_t * string )
    {
        int units = 0;
        for ( int i = 0; string[i] != L'\0'; i++ )
        {
            const uint32_t character = (uint32_t) string[i];
            units += ( character >= 0x10000 && character <= 0x10FFFF ) ? 2 : 1;
        }
        return units;
    }

    /*
        The wstring payload is well-formed UTF-16 BY CONTRACT (STANDARD.md, adopted
        2026-08-15): an unpaired surrogate is a writer contract violation, debug-asserted
        per the writes-trusted doctrine. On a 4 byte wchar_t platform the string is UTF-32,
        where a surrogate CODE POINT or a value above U+10FFFF is the malformation; on a
        2 byte platform it is UTF-16, where the pairing itself is checked. Referenced only
        from serialize_assert; compiles out under NDEBUG.
    */

    inline bool serialize_wstring_is_valid_utf16( const wchar_t * string )
    {
        // through a local rather than tested inline, so MSVC /W4 does not flag the constant conditional
        const bool wide_wchar = sizeof( wchar_t ) >= 4;
        int i = 0;
        while ( string[i] != L'\0' )
        {
            const uint32_t character = (uint32_t) string[i];
            if ( wide_wchar )
            {
                if ( character >= 0xD800 && character <= 0xDFFF )       // a surrogate is not a code point
                    return false;
                if ( character > 0x10FFFF )                             // above Unicode
                    return false;
                i += 1;
            }
            else
            {
                if ( character >= 0xD800 && character <= 0xDBFF )
                {
                    const uint32_t next = (uint32_t) string[i+1];
                    if ( next < 0xDC00 || next > 0xDFFF )               // high surrogate without its pair
                        return false;
                    i += 2;
                }
                else if ( character >= 0xDC00 && character <= 0xDFFF )
                {
                    return false;                                       // low surrogate with no high before it
                }
                else
                {
                    i += 1;
                }
            }
        }
        return true;
    }

    // Wire format is 32 bits per group, EACH GROUP ONE UTF-16 CODE UNIT (STANDARD.md, adopted
    // 2026-08-15), so streams are byte-identical between platforms with 2 and 4 byte wchar_t:
    // the 4 byte platform converts at the boundary — an astral code point splits into its
    // surrogate pair on write, and the pair recombines on read. The length field counts the
    // units transmitted.
    //
    // On read, malformed payloads are REFUSED in every build mode (STANDARD.md, "Readers must
    // refuse malformed wstring payloads", adopted 2026-08-15): a group above 0xFFFF is not a
    // code unit; an unpaired surrogate fails the read — a high surrogate without its low, a
    // low with no high before it, or a dangling high as the final group — and so does an
    // interior NUL group, because a conforming writer derives the length from the terminator,
    // so a zero group only arrives doctored and gives the payload two lengths. Well-formed
    // surrogate PAIRS pass: they are how astral text travels.

    template <typename Stream> bool serialize_wstring_internal( Stream & stream, wchar_t * string, int buffer_size )
    {
        int length = 0;
        if ( Stream::IsWriting )
        {
            // the writer's contract, debug only. See serialize_wstring_is_valid_utf16.
            serialize_assert( serialize_wstring_is_valid_utf16( string ) );
            length = serialize_wstring_unit_count( string );
            serialize_assert( length < buffer_size );
        }
        serialize_int( stream, length, 0, buffer_size - 1 );
        if ( Stream::IsWriting )
        {
            for ( int i = 0; string[i] != L'\0'; i++ )
            {
                const uint32_t character = (uint32_t) string[i];
                if ( character >= 0x10000 && character <= 0x10FFFF )
                {
                    // an astral code point in a 4 byte wchar_t: split into its surrogate
                    // pair at the boundary, so the bytes are the ones a 2 byte wchar_t
                    // platform produces
                    uint32_t high_surrogate = 0xD800 + ( ( character - 0x10000 ) >> 10 );
                    uint32_t low_surrogate = 0xDC00 + ( ( character - 0x10000 ) & 0x3FF );
                    serialize_bits( stream, high_surrogate, 32 );
                    serialize_bits( stream, low_surrogate, 32 );
                }
                else
                {
                    uint32_t unit = character;
                    serialize_bits( stream, unit, 32 );
                }
            }
        }
        else
        {
            // Each group is one UTF-16 code unit (STANDARD.md, adopted 2026-08-15), and
            // malformed payloads are REFUSED in every build mode: a group above 0xFFFF is
            // not a code unit and no conforming writer emits one; an interior NUL gives
            // the payload two lengths; the pair discipline refuses a high surrogate
            // without its low, a low with no high before it, and a dangling high as the
            // final group. Well-formed pairs pass — they are how astral text travels —
            // and on a 4 byte wchar_t they recombine at the boundary, the inverse of the
            // split the writer performed; a 2 byte wchar_t stores units as they arrive.
            // through a local rather than tested inline, so MSVC /W4 does not flag the constant conditional
            const bool wide_wchar = sizeof( wchar_t ) >= 4;
            uint32_t pending = 0;                   // a high surrogate awaiting its pair
            bool have_pending = false;
            int output_index = 0;
            for ( int i = 0; i < length; i++ )
            {
                uint32_t character = 0;
                serialize_bits( stream, character, 32 );
                if ( character > 0xFFFF )
                {
                    return false;                   // not a UTF-16 code unit: nothing conforming emits one
                }
                if ( character == 0 )
                {
                    return false;                   // interior NUL: the two-lengths smuggling primitive
                }
                if ( have_pending )
                {
                    if ( character < 0xDC00 || character > 0xDFFF )
                    {
                        return false;               // high surrogate without its low
                    }
                    if ( wide_wchar )
                    {
                        string[output_index++] = (wchar_t) ( 0x10000 + ( ( pending - 0xD800 ) << 10 ) + ( character - 0xDC00 ) );
                    }
                    else
                    {
                        string[output_index++] = (wchar_t) pending;
                        string[output_index++] = (wchar_t) character;
                    }
                    have_pending = false;
                    continue;
                }
                if ( character >= 0xDC00 && character <= 0xDFFF )
                {
                    return false;                   // low surrogate with no high before it
                }
                if ( character >= 0xD800 && character <= 0xDBFF )
                {
                    pending = character;
                    have_pending = true;
                    continue;
                }
                string[output_index++] = (wchar_t) character;
            }
            if ( have_pending )
            {
                return false;                       // the final group is a dangling high surrogate
            }
            string[output_index] = L'\0';
        }
        return true;
    }


    /**
        Serialize a string to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        On read, malformed payloads are refused (STANDARD.md, adopted 2026-08-15): invalid UTF-8 fails the read, and so does an interior NUL byte among the transmitted bytes.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param string The string to serialize write/measure. Pointer to buffer to be filled on read.
        @param buffer_size The size of the string buffer. String with terminating null character must fit into this buffer.
     */

    #define serialize_string( stream, string, buffer_size )                                 \
        do                                                                                  \
        {                                                                                   \
            if ( !serialize::serialize_string_internal( stream, string, buffer_size ) )     \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    /**
        Serialize a wide string to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        The wire format is 32 bits per group, each group one UTF-16 code unit, so streams are byte-identical between platforms with 2 and 4 byte wchar_t: on a 4 byte wchar_t platform an astral code point splits into its surrogate pair on write and recombines on read. The payload is well-formed UTF-16 by the writer's contract (debug-asserted).
        On read, malformed payloads are refused (STANDARD.md, adopted 2026-08-15): a group above 0xFFFF is not a code unit and fails the read, an unpaired surrogate fails the read, and so does an interior NUL group among the transmitted groups.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param string The wide string to serialize write/measure. Pointer to buffer to be filled on read.
        @param buffer_size The size of the string buffer in wide characters. String with terminating null character must fit into this buffer.
     */

    #define serialize_wstring( stream, string, buffer_size )                                \
        do                                                                                  \
        {                                                                                   \
            if ( !serialize::serialize_wstring_internal( stream, string, buffer_size ) )    \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)


    /**
        Serialize an alignment to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
     */

    #define serialize_align( stream )                                                       \
        do                                                                                  \
        {                                                                                   \
            if ( !stream.SerializeAlign() )                                                 \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    /**
        Serialize an object to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param object The object to serialize. Must have a serialize method on it.
     */

    #define serialize_object( stream, object )                                              \
        do                                                                                  \
        {                                                                                   \
            if ( !( object ).Serialize( stream ) )                                          \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        }                                                                                   \
        while(0)

    template <typename Stream, typename T> bool serialize_int_relative_internal( Stream & stream, T previous, T & current )
    {
        uint32_t difference = 0;
        if ( Stream::IsWriting )
        {
            serialize_assert( previous < current );
            // subtract in the unsigned domain: current - previous overflows signed arithmetic when the gap is wider than 2^31
            difference = uint32_t( current ) - uint32_t( previous );
        }

        bool oneBit = false;
        if ( Stream::IsWriting )
        {
            oneBit = difference == 1;
        }
        serialize_bool( stream, oneBit );
        if ( oneBit )
        {
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + 1 );
            }
            return true;
        }

        bool twoBits = false;
        if ( Stream::IsWriting )
        {
            twoBits = difference <= 6;
        }
        serialize_bool( stream, twoBits );
        if ( twoBits )
        {
            serialize_int( stream, difference, 2, 6 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        bool fourBits = false;
        if ( Stream::IsWriting )
        {
            fourBits = difference <= 23;
        }
        serialize_bool( stream, fourBits );
        if ( fourBits )
        {
            serialize_int( stream, difference, 7, 23 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        bool eightBits = false;
        if ( Stream::IsWriting )
        {
            eightBits = difference <= 280;
        }
        serialize_bool( stream, eightBits );
        if ( eightBits )
        {
            serialize_int( stream, difference, 24, 280 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        bool twelveBits = false;
        if ( Stream::IsWriting )
        {
            twelveBits = difference <= 4377;
        }
        serialize_bool( stream, twelveBits );
        if ( twelveBits )
        {
            serialize_int( stream, difference, 281, 4377 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        bool sixteenBits = false;
        if ( Stream::IsWriting )
        {
            sixteenBits = difference <= 69914;
        }
        serialize_bool( stream, sixteenBits );
        if ( sixteenBits )
        {
            serialize_int( stream, difference, 4378, 69914 );
            if ( Stream::IsReading )
            {
                // reconstruct in the unsigned domain: previous + difference overflows signed arithmetic near the type maximum
                current = T( uint32_t( previous ) + difference );
            }
            return true;
        }

        uint32_t value = current;
        serialize_bits( stream, value, 32 );
        if ( Stream::IsReading )
        {
            current = value;
            if ( current <= previous )
            {
                return false;
            }
        }

        return true;
    }

    /**
        Serialize an integer value relative to another (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param previous The previous integer value.
        @param current The current integer value.
     */

    #define serialize_int_relative( stream, previous, current )                             \
        do                                                                                  \
        {                                                                                   \
            if ( !serialize::serialize_int_relative_internal( stream, previous, current ) ) \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    /**
        Compile time trait marking the integer types usable as fixed point storage.
        Written locally because std::is_integral is not guaranteed to cover __int128 on every compiler, and this header does not include \<type_traits\>.
     */

    template <typename T> struct FixedPointInteger                  { enum { is_integer = 0, is_signed = 0 }; };
    template <> struct FixedPointInteger<signed char>               { enum { is_integer = 1, is_signed = 1 }; };
    template <> struct FixedPointInteger<unsigned char>             { enum { is_integer = 1, is_signed = 0 }; };
    template <> struct FixedPointInteger<short>                     { enum { is_integer = 1, is_signed = 1 }; };
    template <> struct FixedPointInteger<unsigned short>            { enum { is_integer = 1, is_signed = 0 }; };
    template <> struct FixedPointInteger<int>                       { enum { is_integer = 1, is_signed = 1 }; };
    template <> struct FixedPointInteger<unsigned int>              { enum { is_integer = 1, is_signed = 0 }; };
    template <> struct FixedPointInteger<long>                      { enum { is_integer = 1, is_signed = 1 }; };
    template <> struct FixedPointInteger<unsigned long>             { enum { is_integer = 1, is_signed = 0 }; };
    template <> struct FixedPointInteger<long long>                 { enum { is_integer = 1, is_signed = 1 }; };
    template <> struct FixedPointInteger<unsigned long long>        { enum { is_integer = 1, is_signed = 0 }; };
#if defined(__SIZEOF_INT128__)
    template <> struct FixedPointInteger<int128_t>                  { enum { is_integer = 1, is_signed = 1 }; };
    template <> struct FixedPointInteger<uint128_t>                 { enum { is_integer = 1, is_signed = 0 }; };
#endif // #if defined(__SIZEOF_INT128__)
#if defined(SERIALIZE_UINT128_DEFINED)
    // the emulated pair works as storage too. without native __int128 these ARE the serialize
    // typedefs; with it they are a distinct set of types (present when tests are enabled, or
    // when a sibling header defined the block first), so the specializations never collide
    template <> struct FixedPointInteger< ::serialize_int128_t >            { enum { is_integer = 1, is_signed = 1 }; };
    template <> struct FixedPointInteger< ::serialize_uint128_t >           { enum { is_integer = 1, is_signed = 0 }; };
#endif // #if defined(SERIALIZE_UINT128_DEFINED)

    /**
        Compile time map from a 128 bit fixed point storage type to its unsigned counterpart,
        in which the wide codec runs its raw offset math. Both the native and the emulated types
        map to their unsigned partner; two's complement wrap around is exact in both, so the
        codec is representation generic and wide fixed point works on compilers without native
        __int128.
     */

    template <typename T> struct FixedPointUnsigned;

#if defined(__SIZEOF_INT128__)
    template <> struct FixedPointUnsigned<int128_t>                 { typedef uint128_t type; };
    template <> struct FixedPointUnsigned<uint128_t>                { typedef uint128_t type; };
#endif // #if defined(__SIZEOF_INT128__)
#if defined(SERIALIZE_UINT128_DEFINED)
    template <> struct FixedPointUnsigned< ::serialize_int128_t >           { typedef ::serialize_uint128_t type; };
    template <> struct FixedPointUnsigned< ::serialize_uint128_t >          { typedef ::serialize_uint128_t type; };
#endif // #if defined(SERIALIZE_UINT128_DEFINED)

    /**
        Fixed point codec, specialized on storage width.
        The false specialization covers storage of 64 bits or fewer and works entirely in the 64 bit unsigned domain.
        The true specialization covers 128 bit storage. It is representation generic — the raw offset math runs in the storage type's unsigned counterpart via FixedPointUnsigned, and its compile time constants live in the 64 bit domain — so it works with native __int128 storage and with the emulated pair alike, and wide fixed point is available on every platform.
        Splitting on width keeps every shift and every compile time bit count valid for the domain it runs in.
        IMPORTANT: Generally, you don't use this directly. Use the serialize_fixed, read_fixed and write_fixed macros instead.
     */

    template <bool WideStorage> struct FixedPointSerializer;

    template <> struct FixedPointSerializer<false>
    {
        template <int IntegerBits, int FractionalBits, int64_t MinUnits, int64_t MaxUnits, typename Stream, typename Storage>
        static bool Serialize( Stream & stream, Storage & value )
        {
            // the whole unit capacity of the Q format. computed in the unsigned domain so the widest formats (Q64.0 and friends) cannot overflow signed arithmetic
            const int64_t min_representable_units = FixedPointInteger<Storage>::is_signed ?
                int64_t( uint64_t(0) - ( uint64_t(1) << ( IntegerBits - 1 ) ) ) : int64_t(0);
            const int64_t max_representable_units = FixedPointInteger<Storage>::is_signed ?
                int64_t( ( uint64_t(1) << ( IntegerBits - 1 ) ) - 1 ) :
                ( ( IntegerBits >= 64 ) ? INT64_MAX : int64_t( ( uint64_t(1) << ( IntegerBits < 64 ? IntegerBits : 0 ) ) - 1 ) );

            serialize_static_assert( MinUnits >= min_representable_units, "serialize_fixed min bound in whole units does not fit the Q format" );
            serialize_static_assert( MaxUnits <= max_representable_units, "serialize_fixed max bound in whole units does not fit the Q format" );

            // shift the whole unit bounds into raw fixed point units in the unsigned domain, so negative bounds wrap two's complement instead of invoking undefined behavior.
            // everything below is a compile time constant: the bounds, the range and the bit count all fold, so the call site carries no runtime bit width computation at all.
            const uint64_t raw_min = uint64_t( MinUnits ) << FractionalBits;
            const uint64_t raw_max = uint64_t( MaxUnits ) << FractionalBits;
            const uint64_t raw_range = raw_max - raw_min;

            if ( MinUnits == MaxUnits )
            {
                // degenerate range: the value IS the range, nothing to send (STANDARD.md: min == max costs zero bits, on every storage width)
                if ( Stream::IsWriting )
                {
                    serialize_assert( uint64_t( value ) == raw_min );       // all checking is performed by debug asserts on write
                }
                if ( Stream::IsReading )
                {
                    value = Storage( raw_min );
                }
                return true;
            }

            const int bits = BitsRequired64<raw_min, raw_max>::result;

            uint64_t offset = 0;

            if ( Stream::IsWriting )
            {
                // subtract in the unsigned domain: raw - raw_min overflows signed arithmetic when the range is wider than 2^63
                offset = uint64_t( value ) - raw_min;
                serialize_assert( offset <= raw_range );        // the value must be within [min,max] whole units. all checking is performed by debug asserts on write
            }

            if ( bits <= 32 )
            {
                uint32_t unsigned_value = uint32_t( offset );
                if ( !stream.SerializeBits( unsigned_value, bits ) )
                {
                    return false;
                }
                offset = unsigned_value;
            }
            else
            {
                // low dword first, then the high remainder: same convention as serialize_bits and serialize_int64
                uint32_t low_half = uint32_t( offset & 0xFFFFFFFF );
                uint32_t high_half = uint32_t( offset >> 32 );
                if ( !stream.SerializeBits( low_half, 32 ) )
                {
                    return false;
                }
                if ( !stream.SerializeBits( high_half, bits - 32 ) )
                {
                    return false;
                }
                offset = ( uint64_t( high_half ) << 32 ) | low_half;
            }

            if ( Stream::IsReading )
            {
                // reject raw values outside [raw_min,raw_max] smuggled into the bit headroom. reject, never clamp
                if ( offset > raw_range )
                {
                    return false;
                }
                // reconstruct in the unsigned domain, then convert: wraps two's complement for signed storage
                value = Storage( raw_min + offset );
            }

            return true;
        }
    };

    template <> struct FixedPointSerializer<true>
    {
        template <int IntegerBits, int FractionalBits, int64_t MinUnits, int64_t MaxUnits, typename Stream, typename Storage>
        static bool Serialize( Stream & stream, Storage & value )
        {
            // the unsigned counterpart of the storage type: native unsigned __int128 for native
            // storage, the emulated serialize_uint128_t for emulated storage. all raw offset math runs
            // in this type, where wrap around is exact two's complement in both representations,
            // so this codec is representation generic and needs no compiler guard at all.
            typedef typename FixedPointUnsigned<Storage>::type Unsigned;

            // the whole unit capacity of the Q format, in the 64 bit compile time domain: with 65
            // or more integer bits the capacity covers any int64 bound, and below that the
            // capacity math fits 64 bits. the inner clamps keep the unselected ternary arms free
            // of invalid shift counts.
            const int64_t min_representable_units = FixedPointInteger<Storage>::is_signed ?
                ( ( IntegerBits >= 65 ) ? INT64_MIN : int64_t( uint64_t(0) - ( uint64_t(1) << ( ( IntegerBits <= 64 ? IntegerBits : 1 ) - 1 ) ) ) ) : int64_t(0);
            const int64_t max_representable_units = FixedPointInteger<Storage>::is_signed ?
                ( ( IntegerBits >= 64 ) ? INT64_MAX : int64_t( ( uint64_t(1) << ( ( IntegerBits < 64 ? IntegerBits : 1 ) - 1 ) ) - 1 ) ) :
                ( ( IntegerBits >= 64 ) ? INT64_MAX : int64_t( ( uint64_t(1) << ( IntegerBits < 64 ? IntegerBits : 0 ) ) - 1 ) );

            serialize_static_assert( MinUnits >= min_representable_units, "serialize_fixed min bound in whole units does not fit the Q format" );
            serialize_static_assert( MaxUnits <= max_representable_units, "serialize_fixed max bound in whole units does not fit the Q format" );

            // shift the whole unit bounds into raw fixed point units in the unsigned 128 bit domain, so negative bounds wrap two's complement instead of invoking undefined behavior.
            // the storage constructor sign extends the int64 bounds for signed storage. everything below folds: the bounds, the range, the bit count and the group structure.
            const Unsigned raw_min = Unsigned( Storage( MinUnits ) ) << FractionalBits;
            const Unsigned raw_max = Unsigned( Storage( MaxUnits ) ) << FractionalBits;
            const Unsigned raw_range = raw_max - raw_min;

            if ( MinUnits == MaxUnits )
            {
                // degenerate range: the value IS the range, nothing to send. the wide path must agree
                // with the narrow path here — FractionalBits of zeros is NOT a degenerate encoding
                // (STANDARD.md: min == max costs zero bits, on every storage width)
                if ( Stream::IsWriting )
                {
                    serialize_assert( Unsigned( value ) == raw_min );       // all checking is performed by debug asserts on write
                }
                if ( Stream::IsReading )
                {
                    value = Storage( raw_min );
                }
                return true;
            }

            // the wire cost, computed in the 64 bit compile time domain: the range in whole units
            // is exact in a uint64, and shifting it left by FractionalBits adds exactly
            // FractionalBits to its bit length.
            const int bits = BitsRequired64< uint64_t( MinUnits ), uint64_t( MaxUnits ) >::result + FractionalBits;

            Unsigned offset = 0;

            if ( Stream::IsWriting )
            {
                // subtract in the unsigned domain: raw - raw_min overflows signed arithmetic when the range is wider than 2^127
                offset = Unsigned( value ) - raw_min;
                serialize_assert( offset <= raw_range );        // the value must be within [min,max] whole units. all checking is performed by debug asserts on write
            }

            // the offset is written in 32 bit groups, least significant group first: the same convention as serialize_bits.
            // the group structure is selected at compile time, so each SerializeBits call receives a constant bit count.
            uint32_t group0 = 0;
            uint32_t group1 = 0;
            uint32_t group2 = 0;
            uint32_t group3 = 0;

            if ( Stream::IsWriting )
            {
                group0 = uint32_t( uint64_t( offset )       & 0xFFFFFFFF );
                group1 = uint32_t( uint64_t( offset >> 32 ) & 0xFFFFFFFF );
                group2 = uint32_t( uint64_t( offset >> 64 ) & 0xFFFFFFFF );
                group3 = uint32_t( uint64_t( offset >> 96 ) & 0xFFFFFFFF );
            }

            if ( bits <= 32 )
            {
                if ( !stream.SerializeBits( group0, bits ) )
                {
                    return false;
                }
            }
            else if ( bits <= 64 )
            {
                if ( !stream.SerializeBits( group0, 32 ) )
                {
                    return false;
                }
                if ( !stream.SerializeBits( group1, bits - 32 ) )
                {
                    return false;
                }
            }
            else if ( bits <= 96 )
            {
                if ( !stream.SerializeBits( group0, 32 ) )
                {
                    return false;
                }
                if ( !stream.SerializeBits( group1, 32 ) )
                {
                    return false;
                }
                if ( !stream.SerializeBits( group2, bits - 64 ) )
                {
                    return false;
                }
            }
            else
            {
                if ( !stream.SerializeBits( group0, 32 ) )
                {
                    return false;
                }
                if ( !stream.SerializeBits( group1, 32 ) )
                {
                    return false;
                }
                if ( !stream.SerializeBits( group2, 32 ) )
                {
                    return false;
                }
                if ( !stream.SerializeBits( group3, bits - 96 ) )
                {
                    return false;
                }
            }

            if ( Stream::IsReading )
            {
                offset = ( Unsigned( group3 ) << 96 ) | ( Unsigned( group2 ) << 64 ) | ( Unsigned( group1 ) << 32 ) | Unsigned( group0 );

                // reject raw values outside [raw_min,raw_max] smuggled into the bit headroom. reject, never clamp
                if ( offset > raw_range )
                {
                    return false;
                }
                // reconstruct in the unsigned domain, then convert: wraps two's complement for signed storage
                value = Storage( raw_min + offset );
            }

            return true;
        }
    };

    template <int IntegerBits, int FractionalBits, int64_t MinUnits, int64_t MaxUnits, typename Stream, typename Storage>
    bool serialize_fixed_internal( Stream & stream, Storage & value )
    {
        serialize_static_assert( FixedPointInteger<Storage>::is_integer == 1, "serialize_fixed storage must be an integer type" );
        serialize_static_assert( IntegerBits >= 1, "serialize_fixed needs at least one integer bit. the sign bit counts for signed storage" );
        serialize_static_assert( FractionalBits >= 0, "serialize_fixed fractional bits can't be negative" );
        serialize_static_assert( IntegerBits + FractionalBits == 8 * (int) sizeof( Storage ), "serialize_fixed integer bits plus fractional bits must equal the number of bits in the storage type" );
        serialize_static_assert( MinUnits <= MaxUnits, "serialize_fixed min must not exceed max" );

        return FixedPointSerializer< ( 8 * (int) sizeof( Storage ) > 64 ) >::template Serialize<IntegerBits, FractionalBits, MinUnits, MaxUnits>( stream, value );
    }

    /**
        Serialize a fixed point value to the stream (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        The Q format and the bounds are compile time constants: integer_bits plus fraction_bits must equal the number of bits in the storage type, with the sign bit counting towards integer_bits for signed storage. For example, Q48.16 in an int64_t is ( 48, 16 ) and Q112.16 in a serialize::int128_t is ( 112, 16 ). Any integer storage type works, including 128 bit storage on every platform: native __int128 where the compiler provides it, the emulated pair where it doesn't.
        The bounds are whole units and must be constant expressions: a runtime bound fails to compile, and bounds that don't fit the Q format fail with a static assert. The value is serialized as an offset from min in the minimal number of bits for the range — a constant of the call site — and the round trip is exact: fixed point values are integers underneath, so unlike compressed floats there is no quantization error and results are identical across platforms.
        A degenerate range where min == max is legal and costs zero bits on every storage width: nothing is written, and the reader recovers the value from the range alone — the raw value min << fraction_bits.
        For storage of 64 bits or fewer the wire format is byte identical to serialize_int64 of the raw value over the raw bounds.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The fixed point value to serialize, in [min,max] whole units. The storage type sets the width of the Q format.
        @param integer_bits The number of integer bits in the Q format, including the sign bit for signed storage. Compile time constant.
        @param fraction_bits The number of fractional bits in the Q format. Compile time constant.
        @param min The minimum value in whole units. Compile time constant.
        @param max The maximum value in whole units. Compile time constant.
     */

    #define serialize_fixed( stream, value, integer_bits, fraction_bits, min, max )                                 \
        do                                                                                                          \
        {                                                                                                           \
            if ( !serialize::serialize_fixed_internal<integer_bits, fraction_bits, min, max>( stream, value ) )     \
            {                                                                                                       \
                return false;                                                                                       \
            }                                                                                                       \
        } while (0)

    // read macros corresponding to each serialize_*. useful when you want separate read and write functions.

    #define read_bits( stream, value, bits )                                                \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( (bits) > 0 );                                                 \
            serialize_assert( (bits) <= 64 );                                               \
            if ( (bits) <= 32 )                                                             \
            {                                                                               \
                uint32_t uint32_value;                                                      \
                if ( !stream.SerializeBits( uint32_value, bits ) )                          \
                {                                                                           \
                    return false;                                                           \
                }                                                                           \
                value = uint32_value;                                                       \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                uint32_t lo = 0;                                                            \
                uint32_t hi = 0;                                                            \
                if ( !stream.SerializeBits( lo, 32 ) )                                      \
                {                                                                           \
                    return false;                                                           \
                }                                                                           \
                if ( !stream.SerializeBits( hi, (bits) - 32 ) )                             \
                {                                                                           \
                    return false;                                                           \
                }                                                                           \
                value = ( uint64_t(hi) << 32 ) | lo;                                        \
            }                                                                               \
        } while (0)

    #define read_int( stream, value, min, max )                                             \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( (min) <= (max) );                                              \
            int32_t int32_value = 0;                                                        \
            if ( !stream.SerializeInteger( int32_value, min, max ) )                        \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            if ( int32_value < int32_t(min) || int32_value > int32_t(max) )                 \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            value = int32_value;                                                            \
        } while (0)

    #define read_int64( stream, value, min, max )                                           \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( int64_t(min) <= int64_t(max) );                                \
            int64_t int64_value = 0;                                                        \
            if ( !stream.SerializeInteger64( int64_value, min, max ) )                      \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            if ( int64_value < int64_t(min) || int64_value > int64_t(max) )                 \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            value = int64_value;                                                            \
        } while (0)

    #define read_int128( stream, value, min, max )                                          \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( serialize::int128_t(min) < serialize::int128_t(max) );        \
            serialize::int128_t int128_value = 0;                                           \
            if ( !stream.SerializeInteger128( int128_value, min, max ) )                    \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            value = int128_value;                                                           \
            if ( serialize::int128_t(value) < serialize::int128_t(min) ||                   \
                 serialize::int128_t(value) > serialize::int128_t(max) )                    \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_fixed( stream, value, integer_bits, fraction_bits, min, max )                                  \
        do                                                                                                      \
        {                                                                                                       \
            if ( !serialize::serialize_fixed_internal<integer_bits, fraction_bits, min, max>( stream, value ) ) \
            {                                                                                                   \
                return false;                                                                                   \
            }                                                                                                   \
        } while (0)

    #define read_bool( stream, value )      read_bits( stream, value, 1 )
    #define read_uint8( stream, value )     read_bits( stream, value, 8 )
    #define read_uint16( stream, value )    read_bits( stream, value, 16 )
    #define read_uint32( stream, value )    read_bits( stream, value, 32 )
    #define read_uint64( stream, value )    read_bits( stream, value, 64 )

    #define read_uint128( stream, value )                                                   \
        do                                                                                  \
        {                                                                                   \
            if ( !serialize::serialize_uint128_internal( stream, value ) )                  \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_float                  serialize_float
    #define read_double                 serialize_double

    #define read_bytes( stream, data, bytes )                                               \
        do                                                                                  \
        {                                                                                   \
            uint8_t * data_ptr = (uint8_t*) ( data );                                       \
            if ( !stream.SerializeBytes( data_ptr, bytes ) )                                \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_string( stream, string, buffer_size )                                      \
        do                                                                                  \
        {                                                                                   \
            char * string_ptr = (char*) ( string );                                         \
            if ( !serialize_string_internal( stream, string_ptr, buffer_size ) )            \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)

    #define read_wstring( stream, string, buffer_size )                                     \
        do                                                                                  \
        {                                                                                   \
            wchar_t * wstring_ptr = (wchar_t*) ( string );                                  \
            if ( !serialize_wstring_internal( stream, wstring_ptr, buffer_size ) )          \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        } while (0)


    #define read_align                  serialize_align
    #define read_object                 serialize_object
    #define read_int_relative           serialize_int_relative

    // write macros corresponding to each serialize_*. useful when you want separate read and write functions.

    #define write_bits( stream, value, bits )                                               \
        do                                                                                  \
        {                                                                                   \
            uint64_t uint64_value = value;                                                  \
            if ( (bits) <= 32 )                                                             \
            {                                                                               \
                uint32_t uint32_value = (uint32_t) uint64_value;                            \
                stream.SerializeBits( uint32_value, bits );                                 \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                uint32_t lo = uint32_t( uint64_value & 0xFFFFFFFF );                        \
                uint32_t hi = uint32_t( uint64_value >> 32 );                               \
                stream.SerializeBits( lo, 32 );                                             \
                stream.SerializeBits( hi, (bits) - 32 );                                    \
            }                                                                               \
        } while (0)

    #define write_int( stream, value, min, max )                                            \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( (int32_t) ( min ) <= (int32_t) ( max ) );                     \
            serialize_assert( (int32_t) ( value ) >= (int32_t) ( min ) );                   \
            serialize_assert( (int32_t) ( value ) <= (int32_t) ( max ) );                   \
            int32_t int32_value = (int32_t) ( value );                                      \
            stream.SerializeInteger( int32_value, min, max );                               \
        } while (0)

    #define write_int64( stream, value, min, max )                                          \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( int64_t( min ) <= int64_t( max ) );                            \
            serialize_assert( int64_t( value ) >= int64_t( min ) );                         \
            serialize_assert( int64_t( value ) <= int64_t( max ) );                         \
            int64_t int64_value = (int64_t) ( value );                                      \
            stream.SerializeInteger64( int64_value, min, max );                             \
        } while (0)

    #define write_int128( stream, value, min, max )                                         \
        do                                                                                  \
        {                                                                                   \
            serialize_assert( serialize::int128_t( min ) < serialize::int128_t( max ) );    \
            serialize_assert( serialize::int128_t( value ) >= serialize::int128_t( min ) ); \
            serialize_assert( serialize::int128_t( value ) <= serialize::int128_t( max ) ); \
            serialize::int128_t int128_value = serialize::int128_t( value );                \
            stream.SerializeInteger128( int128_value, min, max );                           \
        } while (0)

    #define write_fixed( stream, value, integer_bits, fraction_bits, min, max )                                 \
        do                                                                                                      \
        {                                                                                                       \
            serialize::serialize_fixed_internal<integer_bits, fraction_bits, min, max>( stream, value );        \
        } while (0)

    #define write_bool( stream, value )         write_bits( stream, value, 1 )
    #define write_uint8( stream, value )        write_bits( stream, value, 8 )
    #define write_uint16( stream, value )       write_bits( stream, value, 16 )
    #define write_uint32( stream, value )       write_bits( stream, value, 32 )
    #define write_uint64( stream, value )       write_bits( stream, value, 64 )

    #define write_uint128( stream, value )                                                  \
        do                                                                                  \
        {                                                                                   \
            serialize::uint128_t uint128_value = ( value );                                 \
            serialize::serialize_uint128_internal( stream, uint128_value );                 \
        } while (0)

    #define write_float( stream, value )                                                    \
        do                                                                                  \
        {                                                                                   \
            float float_value = (float) ( value );                                          \
            uint32_t int_value;                                                             \
            memcpy( (char*) &int_value, &float_value, 4 );                                  \
            stream.SerializeBits( int_value, 32 );                                          \
        } while (0)

    #define write_double( stream, value )                                                   \
        do                                                                                  \
        {                                                                                   \
            double double_value = (double) ( value );                                       \
            uint64_t int64_value;                                                           \
            memcpy( (char*) &int64_value, &double_value, 8 );                               \
            write_bits( stream, int64_value, 64 );                                          \
        } while (0)

    #define write_bytes( stream, data, bytes )                                              \
        do                                                                                  \
        {                                                                                   \
            const uint8_t * data_ptr = (const uint8_t*) ( data );                           \
            stream.SerializeBytes( data_ptr, bytes );                                       \
        } while (0)

    #define write_string( stream, string, buffer_size )                                     \
        do                                                                                  \
        {                                                                                   \
            int length = (int) strlen( string );                                            \
            serialize_assert( length < (buffer_size) );                                     \
            /* the writer's contract, debug only. See serialize_string_is_valid_utf8 */     \
            serialize_assert( serialize::serialize_string_is_valid_utf8( string, length ) );\
            write_int( stream, length, 0, (buffer_size) - 1 );                              \
            write_bytes( stream, (uint8_t*) ( string ), length );                           \
        } while (0)

    #define write_wstring( stream, string, buffer_size )                                            \
        do                                                                                          \
        {                                                                                           \
            const wchar_t * wstring_ptr = (const wchar_t*) ( string );                              \
            /* the writer's contract, debug only. See serialize_wstring_is_valid_utf16 */           \
            serialize_assert( serialize::serialize_wstring_is_valid_utf16( wstring_ptr ) );         \
            int wstring_units = serialize::serialize_wstring_unit_count( wstring_ptr );             \
            serialize_assert( wstring_units < (buffer_size) );                                      \
            write_int( stream, wstring_units, 0, (buffer_size) - 1 );                               \
            for ( int i = 0; wstring_ptr[i] != L'\0'; i++ )                                         \
            {                                                                                       \
                const uint32_t wstring_character = (uint32_t) wstring_ptr[i];                       \
                if ( wstring_character >= 0x10000 && wstring_character <= 0x10FFFF )                \
                {                                                                                   \
                    /* astral: split into the surrogate pair at the boundary, matching */           \
                    /* serialize_wstring_internal byte for byte */                                  \
                    write_bits( stream, 0xD800 + ( ( wstring_character - 0x10000 ) >> 10 ), 32 );   \
                    write_bits( stream, 0xDC00 + ( ( wstring_character - 0x10000 ) & 0x3FF ), 32 ); \
                }                                                                                   \
                else                                                                                \
                {                                                                                   \
                    write_bits( stream, wstring_character, 32 );                                    \
                }                                                                                   \
            }                                                                                       \
        } while (0)


    #define write_align( stream )                                                           \
        do                                                                                  \
        {                                                                                   \
            stream.SerializeAlign();                                                        \
        } while (0)

    #define write_object( stream, object )                                                  \
        do                                                                                  \
        {                                                                                   \
            ( object ).Serialize( stream );                                                 \
        }                                                                                   \
        while(0)

    #define write_int_relative( stream, previous, current )                                 \
        do                                                                                  \
        {                                                                                   \
            int current_value = (int) ( current );                                          \
            serialize::serialize_int_relative_internal( stream, previous, current_value );  \
        } while (0)

    // The compile time parameter surface below uses C++14 relaxed constexpr, and consumers vendor
    // this header into pre-C++11 builds (the cxx03-consumer CI leg compiles it at -std=c++03).
    // The surface is additive and aimed at generated code, so language modes older than C++14
    // simply do not get it: everything else in the header stays available. MSVC reports
    // __cplusplus as 199711L unless /Zc:__cplusplus is set, so it is detected by version instead
    // (VS2017, _MSC_VER 1910, carries full C++14 constexpr in every language mode it supports).
#if ( defined( __cplusplus ) && __cplusplus >= 201402L ) || ( defined( _MSC_VER ) && _MSC_VER >= 1910 )
#define SERIALIZE_HAS_COMPILE_TIME_SURFACE 1
#endif

#if defined( SERIALIZE_HAS_COMPILE_TIME_SURFACE )

    // ------------------------------------------------------------------------------------------
    //
    //      Compile time parameter surface (experimental, for generated code)
    //
    //      When serialize calls are generated from a schema, min/max/bits are known at code
    //      generation time. The templates below move them into template argument position, so
    //      the bit count is a compile time constant: no runtime bits_required call, no runtime
    //      min/max parameter plumbing, and the 32/64 bit split resolves on a constant condition.
    //      The wire bytes are identical to the runtime forms given identical inputs, so a code
    //      generator can switch between forms without a wire change.
    //
    //      Humans keep the serialize_* macros above. Generated code calls these templates
    //      directly and emits its own `if ( !... ) return false;`, or uses the thin
    //      serialize_*_compile_time macro wrappers below.
    //
    // ------------------------------------------------------------------------------------------

    /**
        Calculates the number of bits required to serialize an integer in range [min,max], as a constant expression.
        The constexpr companion to serialize::bits_required and serialize::bits_required64, for use in template argument position.
        C++14 relaxed constexpr, matching the oldest standard this header is compiled at in practice (compilers' defaults: this project configures no -std flag).
        @param min The minimum value, converted to the unsigned domain (sign extended for signed ranges).
        @param max The maximum value, converted to the unsigned domain (sign extended for signed ranges).
        @returns The number of bits required to serialize the integer in [0,64].
     */

    constexpr int bits_required64_constexpr( uint64_t min, uint64_t max )
    {
        // subtract in the unsigned domain: max - min overflows signed arithmetic when the range is wider than 2^63.
        // sign extended int32 bounds wrap to the same difference, so this one function covers both integer domains.
        int bits = 0;
        for ( uint64_t diff = max - min; diff != 0; diff >>= 1 )
        {
            bits++;
        }
        return bits;
    }

    /**
        Serialize an integer with compile time bounds (read/write/measure).
        The compile time companion to WriteStream::SerializeInteger and ReadStream::SerializeInteger: the bit count is a constant, so there is no runtime bits_required call and no min/max parameter traffic.
        Wire bytes are identical to the runtime form given identical inputs.
        On write, the value is checked by debug asserts only, matching the runtime form. On read, the value off the wire is validated against the constant bounds and the function returns false on out of range data.
        @tparam Min The minimum value. Must be less than Max (enforced at compile time).
        @tparam Max The maximum value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The integer value in [Min,Max]. Written on write/measure, filled in on read.
        @returns True if the serialize succeeded, false if the read data is truncated or out of range.
     */

    template <int32_t Min, int32_t Max, typename Stream> bool SerializeIntConst( Stream & stream, int32_t & value )
    {
        static_assert( Min < Max, "serialize: min must be less than max" );
        constexpr int bits = bits_required64_constexpr( uint64_t( int64_t( Min ) ), uint64_t( int64_t( Max ) ) );
        uint32_t unsigned_value = 0;
        if ( Stream::IsWriting )
        {
            serialize_assert( value >= Min );
            serialize_assert( value <= Max );
            // subtract in the unsigned domain: value - min overflows signed arithmetic when the range is wider than 2^31
            unsigned_value = uint32_t( value ) - uint32_t( Min );
        }
        if ( !stream.SerializeBits( unsigned_value, bits ) )
        {
            return false;
        }
        if ( Stream::IsReading )
        {
            if ( unsigned_value > uint32_t( Max ) - uint32_t( Min ) )
            {
                return false;
            }
            // add in the unsigned domain: unsigned_value + min overflows signed arithmetic when the range is wider than 2^31
            value = int32_t( unsigned_value + uint32_t( Min ) );
        }
        return true;
    }

    /**
        Serialize a 64 bit integer with compile time bounds (read/write/measure).
        The compile time companion to WriteStream::SerializeInteger64 and ReadStream::SerializeInteger64: the bit count is a constant, and the one dword vs two dword split resolves on a constant condition, so the dead branch folds.
        Wire bytes are identical to the runtime form given identical inputs: low dword first, then the high remainder, same convention as serialize_bits and serialize_uint64.
        On write, the value is checked by debug asserts only, matching the runtime form. On read, the value off the wire is validated against the constant bounds and the function returns false on out of range data.
        @tparam Min The minimum value. Must be less than Max (enforced at compile time).
        @tparam Max The maximum value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The integer value in [Min,Max]. Written on write/measure, filled in on read.
        @returns True if the serialize succeeded, false if the read data is truncated or out of range.
     */

    template <int64_t Min, int64_t Max, typename Stream> bool SerializeInt64Const( Stream & stream, int64_t & value )
    {
        static_assert( Min < Max, "serialize: min must be less than max" );
        constexpr int bits = bits_required64_constexpr( uint64_t( Min ), uint64_t( Max ) );
        uint64_t unsigned_value = 0;
        if ( Stream::IsWriting )
        {
            serialize_assert( value >= Min );
            serialize_assert( value <= Max );
            // subtract in the unsigned domain: value - min overflows signed arithmetic when the range is wider than 2^63
            unsigned_value = uint64_t( value ) - uint64_t( Min );
        }
        if ( bits <= 32 )       // constant condition: the dead branch folds at compile time
        {
            uint32_t unsigned_value32 = uint32_t( unsigned_value );
            if ( !stream.SerializeBits( unsigned_value32, bits ) )
            {
                return false;
            }
            if ( Stream::IsReading )
            {
                unsigned_value = unsigned_value32;
            }
        }
        else
        {
            // low dword first, then the high remainder: same convention as serialize_bits and serialize_uint64
            uint32_t lo = uint32_t( unsigned_value & 0xFFFFFFFF );
            uint32_t hi = uint32_t( unsigned_value >> 32 );
            if ( !stream.SerializeBits( lo, 32 ) )
            {
                return false;
            }
            if ( !stream.SerializeBits( hi, bits - 32 ) )
            {
                return false;
            }
            if ( Stream::IsReading )
            {
                unsigned_value = ( uint64_t( hi ) << 32 ) | lo;
            }
        }
        if ( Stream::IsReading )
        {
            if ( unsigned_value > uint64_t( Max ) - uint64_t( Min ) )
            {
                return false;
            }
            // add in the unsigned domain: unsigned_value + min overflows signed arithmetic when the range is wider than 2^63
            value = int64_t( unsigned_value + uint64_t( Min ) );
        }
        return true;
    }

    /**
        Serialize a compile time number of bits (read/write/measure).
        The compile time companion to WriteStream::SerializeBits and ReadStream::SerializeBits for widths up to 32: the width is a constant, so there is no runtime bits parameter traffic.
        Wire bytes are identical to the runtime form given identical inputs.
        On write, the value is checked by debug asserts only, matching the runtime form (it must be in [0,(1<<Bits)-1]).
        @tparam Bits The number of bits to serialize, in [1,32] (enforced at compile time).
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned integer value. Written on write/measure, filled in on read.
        @returns True if the serialize succeeded, false if the read data is truncated.
     */

    template <int Bits, typename Stream> bool SerializeBitsConst( Stream & stream, uint32_t & value )
    {
        static_assert( Bits > 0, "serialize: bits must be greater than zero" );
        static_assert( Bits <= 32, "serialize: bits must be less than or equal to 32. use SerializeBits64Const for wider values" );
        return stream.SerializeBits( value, Bits );
    }

    /**
        Serialize a compile time number of bits from a 64 bit value (read/write/measure).
        The compile time companion to the serialize_bits macro for widths up to 64: the width is a constant, and the one dword vs two dword split resolves on a constant condition, so the dead branch folds.
        Wire bytes are identical to the runtime form given identical inputs: low dword first, then the high remainder.
        On write, the value is checked by debug asserts only, matching the runtime form (it must be in [0,(1<<Bits)-1]).
        @tparam Bits The number of bits to serialize, in [1,64] (enforced at compile time).
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 64 bit integer value. Written on write/measure, filled in on read.
        @returns True if the serialize succeeded, false if the read data is truncated.
     */

    template <int Bits, typename Stream> bool SerializeBits64Const( Stream & stream, uint64_t & value )
    {
        static_assert( Bits > 0, "serialize: bits must be greater than zero" );
        static_assert( Bits <= 64, "serialize: bits must be less than or equal to 64" );
        if ( Bits <= 32 )       // constant condition: the dead branch folds at compile time
        {
            uint32_t value32 = uint32_t( value );
            if ( !stream.SerializeBits( value32, Bits ) )
            {
                return false;
            }
            if ( Stream::IsReading )
            {
                value = value32;
            }
        }
        else
        {
            // low dword first, then the high remainder: same convention as serialize_bits and serialize_uint64
            uint32_t lo = uint32_t( value & 0xFFFFFFFF );
            uint32_t hi = uint32_t( value >> 32 );
            if ( !stream.SerializeBits( lo, 32 ) )
            {
                return false;
            }
            if ( !stream.SerializeBits( hi, Bits - 32 ) )
            {
                return false;
            }
            if ( Stream::IsReading )
            {
                value = ( uint64_t( hi ) << 32 ) | lo;
            }
        }
        return true;
    }

    /**
        Serialize integer value with compile time bounds (read/write/measure).
        The compile time companion to serialize_int. min and max are expanded into template argument position, so they must be constant expressions: a runtime value fails to compile, deliberately.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The integer value to serialize in [min,max].
        @param min The minimum value. Must be a constant expression.
        @param max The maximum value. Must be a constant expression.
     */

    #define serialize_int_compile_time( stream, value, min, max )                           \
        do                                                                                  \
        {                                                                                   \
            int32_t int32_value = 0;                                                        \
            if ( Stream::IsWriting )                                                        \
            {                                                                               \
                int32_value = (int32_t) ( value );                                          \
            }                                                                               \
            if ( !serialize::SerializeIntConst<(min), (max)>( stream, int32_value ) )       \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            if ( Stream::IsReading )                                                        \
            {                                                                               \
                value = int32_value;                                                        \
            }                                                                               \
        } while (0)

    /**
        Serialize a 64 bit integer value with compile time bounds (read/write/measure).
        The compile time companion to serialize_int64. min and max are expanded into template argument position, so they must be constant expressions: a runtime value fails to compile, deliberately.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The 64 bit integer value to serialize in [min,max].
        @param min The minimum value. Must be a constant expression.
        @param max The maximum value. Must be a constant expression.
     */

    #define serialize_int64_compile_time( stream, value, min, max )                         \
        do                                                                                  \
        {                                                                                   \
            int64_t int64_value = 0;                                                        \
            if ( Stream::IsWriting )                                                        \
            {                                                                               \
                int64_value = (int64_t) ( value );                                          \
            }                                                                               \
            if ( !serialize::SerializeInt64Const<(min), (max)>( stream, int64_value ) )     \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            if ( Stream::IsReading )                                                        \
            {                                                                               \
                value = int64_value;                                                        \
            }                                                                               \
        } while (0)

    /**
        Serialize a compile time number of bits to the stream (read/write/measure).
        The compile time companion to serialize_bits for widths up to 32. bits is expanded into template argument position, so it must be a constant expression: a runtime value fails to compile, deliberately.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned integer value to serialize.
        @param bits The number of bits to serialize in [1,32]. Must be a constant expression.
     */

    #define serialize_bits_compile_time( stream, value, bits )                              \
        do                                                                                  \
        {                                                                                   \
            uint32_t uint32_value = 0;                                                      \
            if ( Stream::IsWriting )                                                        \
            {                                                                               \
                uint32_value = (uint32_t) ( value );                                        \
            }                                                                               \
            if ( !serialize::SerializeBitsConst<(bits)>( stream, uint32_value ) )           \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            if ( Stream::IsReading )                                                        \
            {                                                                               \
                value = uint32_value;                                                       \
            }                                                                               \
        } while (0)

    /**
        Serialize a compile time number of bits from a 64 bit value to the stream (read/write/measure).
        The compile time companion to serialize_bits for widths up to 64. bits is expanded into template argument position, so it must be a constant expression: a runtime value fails to compile, deliberately.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The unsigned 64 bit integer value to serialize.
        @param bits The number of bits to serialize in [1,64]. Must be a constant expression.
     */

    #define serialize_bits64_compile_time( stream, value, bits )                            \
        do                                                                                  \
        {                                                                                   \
            uint64_t uint64_value = 0;                                                      \
            if ( Stream::IsWriting )                                                        \
            {                                                                               \
                uint64_value = (uint64_t) ( value );                                        \
            }                                                                               \
            if ( !serialize::SerializeBits64Const<(bits)>( stream, uint64_value ) )         \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            if ( Stream::IsReading )                                                        \
            {                                                                               \
                value = uint64_value;                                                       \
            }                                                                               \
        } while (0)

    /**
        Serialize a boolean value to the stream through the compile time surface (read/write/measure).
        The compile time companion to serialize_bool: one bit, width resolved at compile time.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param value The boolean value to serialize.
     */

    #define serialize_bool_compile_time( stream, value )                                    \
        do                                                                                  \
        {                                                                                   \
            uint32_t uint32_bool_value = 0;                                                 \
            if ( Stream::IsWriting )                                                        \
            {                                                                               \
                uint32_bool_value = ( value ) ? 1 : 0;                                      \
            }                                                                               \
            if ( !serialize::SerializeBitsConst<1>( stream, uint32_bool_value ) )           \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            if ( Stream::IsReading )                                                        \
            {                                                                               \
                value = uint32_bool_value ? true : false;                                   \
            }                                                                               \
        } while (0)

#endif // #if defined( SERIALIZE_HAS_COMPILE_TIME_SURFACE )
}

inline void serialize_copy_string( char * dest, const char * source, size_t dest_size )
{
    serialize_assert( dest );
    serialize_assert( source );
    serialize_assert( dest_size >= 1 );
    memset( dest, 0, dest_size );
    for ( size_t i = 0; i < dest_size - 1; i++ )
    {
        if ( source[i] == '\0' )
            break;
        dest[i] = source[i];
    }
}

inline void serialize_copy_wstring( wchar_t * dest, const wchar_t * source, size_t dest_size )
{
    serialize_assert( dest );
    serialize_assert( source );
    serialize_assert( dest_size >= 1 );
    memset( dest, 0, dest_size * sizeof( wchar_t ) );
    for ( size_t i = 0; i < dest_size - 1; i++ )
    {
        if ( source[i] == L'\0' )
            break;
        dest[i] = source[i];
    }
}


#if SERIALIZE_ENABLE_TESTS

#include <stdio.h>      // printf
#include <stdlib.h>     // exit, getenv

/**
    Under a passing test, the test's name prints and nothing else. The suite's
    informational narration -- check counts, negative-control statistics, skip
    reasons, which contraction discipline this build exercised -- is opt-in:
    set SERIALIZE_TEST_VERBOSE=1 in the environment to restore it. Failures
    print everything relevant regardless, and no check runs or does not run
    because of this switch -- it gates narration only.
 */
inline bool serialize_test_verbose()
{
    const char * value = getenv( "SERIALIZE_TEST_VERBOSE" );
    return value != NULL && value[0] != '\0' && !( value[0] == '0' && value[1] == '\0' );
}

inline void SerializeCheckHandler( const char * condition,
                                   const char * function,
                                   const char * file,
                                   int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
    fflush( stdout );       // the trap below skips atexit, so a buffered stdout (any pipe: CI) would lose the line above
#ifndef NDEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define serialize_check( condition )                                                    \
do                                                                                      \
{                                                                                       \
    if ( !(condition) )                                                                 \
    {                                                                                   \
        SerializeCheckHandler( #condition, __FUNCTION__, __FILE__, __LINE__ );          \
    }                                                                                   \
} while(0)

inline void test_endian()
{
    uint32_t value = 0x11223344;

    const char * bytes = (const char*) &value;

#if SERIALIZE_LITTLE_ENDIAN

    serialize_check( bytes[0] == 0x44 );
    serialize_check( bytes[1] == 0x33 );
    serialize_check( bytes[2] == 0x22 );
    serialize_check( bytes[3] == 0x11 );

#else // #if SERIALIZE_LITTLE_ENDIAN

    serialize_check( bytes[3] == 0x44 );
    serialize_check( bytes[2] == 0x33 );
    serialize_check( bytes[1] == 0x22 );
    serialize_check( bytes[0] == 0x11 );

#endif // #if SERIALIZE_LITTLE_ENDIAN
}

inline void test_bitpacker()
{
    const int BufferSize = 256;

    uint8_t buffer[BufferSize];

    serialize::BitWriter writer( buffer, BufferSize );

    serialize_check( writer.GetData() == buffer );
    serialize_check( writer.GetBitsWritten() == 0 );
    serialize_check( writer.GetBytesWritten() == 0 );
    serialize_check( writer.GetBitsAvailable() == BufferSize * 8 );

    writer.WriteBits( 0, 1 );
    writer.WriteBits( 1, 1 );
    writer.WriteBits( 10, 8 );
    writer.WriteBits( 255, 8 );
    writer.WriteBits( 1000, 10 );
    writer.WriteBits( 50000, 16 );
    writer.WriteBits( 9999999, 32 );
    writer.FlushBits();

    const int bitsWritten = 1 + 1 + 8 + 8 + 10 + 16 + 32;

    serialize_check( writer.GetBytesWritten() == 10 );
    serialize_check( writer.GetBitsWritten() == bitsWritten );
    serialize_check( writer.GetBitsAvailable() == BufferSize * 8 - bitsWritten );

    const int bytesWritten = writer.GetBytesWritten();

    serialize_check( bytesWritten == 10 );

    memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );

    serialize::BitReader reader( buffer, bytesWritten );

    serialize_check( reader.GetBitsRead() == 0 );
    serialize_check( reader.GetBitsRemaining() == bytesWritten * 8 );

    uint32_t a = reader.ReadBits( 1 );
    uint32_t b = reader.ReadBits( 1 );
    uint32_t c = reader.ReadBits( 8 );
    uint32_t d = reader.ReadBits( 8 );
    uint32_t e = reader.ReadBits( 10 );
    uint32_t f = reader.ReadBits( 16 );
    uint32_t g = reader.ReadBits( 32 );

    serialize_check( a == 0 );
    serialize_check( b == 1 );
    serialize_check( c == 10 );
    serialize_check( d == 255 );
    serialize_check( e == 1000 );
    serialize_check( f == 50000 );
    serialize_check( g == 9999999 );

    serialize_check( reader.GetBitsRead() == bitsWritten );
    serialize_check( reader.GetBitsRemaining() == bytesWritten * 8 - bitsWritten );
}

inline void test_bits_required()
{
    serialize_check( serialize::bits_required( 0, 0 ) == 0 );
    serialize_check( serialize::bits_required( 0, 1 ) == 1 );
    serialize_check( serialize::bits_required( 0, 2 ) == 2 );
    serialize_check( serialize::bits_required( 0, 3 ) == 2 );
    serialize_check( serialize::bits_required( 0, 4 ) == 3 );
    serialize_check( serialize::bits_required( 0, 5 ) == 3 );
    serialize_check( serialize::bits_required( 0, 6 ) == 3 );
    serialize_check( serialize::bits_required( 0, 7 ) == 3 );
    serialize_check( serialize::bits_required( 0, 8 ) == 4 );
    serialize_check( serialize::bits_required( 0, 255 ) == 8 );
    serialize_check( serialize::bits_required( 0, 65535 ) == 16 );
    serialize_check( serialize::bits_required( 0, 4294967295 ) == 32 );
}

inline void test_bits_required64()
{
    serialize_check( serialize::bits_required64( 0, 0 ) == 0 );
    serialize_check( serialize::bits_required64( 0, 1 ) == 1 );
    serialize_check( serialize::bits_required64( 0, 255 ) == 8 );
    serialize_check( serialize::bits_required64( 0, 4294967295ULL ) == 32 );
    serialize_check( serialize::bits_required64( 0, 4294967296ULL ) == 33 );
    serialize_check( serialize::bits_required64( 0, ( 1ULL << 40 ) ) == 41 );
    serialize_check( serialize::bits_required64( 0, 0xFFFFFFFFFFFFFFFFULL ) == 64 );
    serialize_check( serialize::bits_required64( uint64_t(INT64_MIN), uint64_t(INT64_MAX) ) == 64 );
    serialize_check( serialize::bits_required64( uint64_t(-5000000000LL), uint64_t(+5000000000LL) ) == 34 );
}

inline void test_bits_required128()
{
    typedef serialize::uint128_t u128;

    serialize_check( serialize::bits_required128( u128( 0 ), u128( 0 ) ) == 0 );
    serialize_check( serialize::bits_required128( u128( 0 ), u128( 1 ) ) == 1 );
    serialize_check( serialize::bits_required128( u128( 0 ), u128( 255 ) ) == 8 );
    serialize_check( serialize::bits_required128( u128( 0 ), u128( 4294967295ULL ) ) == 32 );
    serialize_check( serialize::bits_required128( u128( 0 ), u128( 4294967296ULL ) ) == 33 );
    serialize_check( serialize::bits_required128( u128( 0 ), u128( 0xFFFFFFFFFFFFFFFFULL ) ) == 64 );

    // the boundary the 64 bit helper cannot reach: one past a full low lane needs the high lane
    serialize_check( serialize::bits_required128( u128( 0 ), u128( 1 ) << 64 ) == 65 );
    serialize_check( serialize::bits_required128( u128( 0 ), ( u128( 1 ) << 127 ) ) == 128 );
    serialize_check( serialize::bits_required128( u128( 0 ), ~u128( 0 ) ) == 128 );

    // the two helpers must agree wherever the range fits 64 bits, or the wire identity claim in
    // STANDARD.md is false and serialize_int128 would silently disagree with serialize_int64
    serialize_check( serialize::bits_required128( u128( 0 ), u128( 4294967296ULL ) ) == serialize::bits_required64( 0, 4294967296ULL ) );
    serialize_check( serialize::bits_required128( u128( 0 ), u128( ( 1ULL << 40 ) ) ) == serialize::bits_required64( 0, ( 1ULL << 40 ) ) );

    // NEGATIVE BOUNDS MUST ARRIVE SIGN EXTENDED, which is what the codec does — it converts
    // serialize::int128_t bounds straight to the unsigned domain. This is the same 34 bits the 64
    // bit helper reports for the same signed range.
    serialize_check( serialize::bits_required128( u128( serialize::int128_t( -5000000000LL ) ), u128( serialize::int128_t( +5000000000LL ) ) ) == 34 );

    // AND THE TRAP IT WOULD BE EASY TO WALK INTO, pinned so nobody "fixes" the conversion:
    // widening an ALREADY WRAPPED uint64_t bound zero extends instead of sign extending, so the
    // range comes out just under 2^128 and the field would cost 128 bits instead of 34. Correct
    // arithmetic on the wrong input — no assert would ever fire.
    serialize_check( serialize::bits_required128( u128( uint64_t(-5000000000LL) ), u128( uint64_t(+5000000000LL) ) ) == 128 );

    // a range wider than 2^127: the subtraction must run in the unsigned domain or this wraps wrong
    serialize_check( serialize::bits_required128( u128( 1 ), ~u128( 0 ) ) == 128 );
}

inline void test_zigzag()
{
    serialize_check( serialize::signed_to_unsigned( 0 ) == 0 );
    serialize_check( serialize::signed_to_unsigned( -1 ) == 1 );
    serialize_check( serialize::signed_to_unsigned( +1 ) == 2 );
    serialize_check( serialize::signed_to_unsigned( -2 ) == 3 );
    serialize_check( serialize::signed_to_unsigned( +2 ) == 4 );
    serialize_check( serialize::signed_to_unsigned( INT32_MAX ) == 0xFFFFFFFE );
    serialize_check( serialize::signed_to_unsigned( INT32_MIN ) == 0xFFFFFFFF );

    serialize_check( serialize::unsigned_to_signed( 0 ) == 0 );
    serialize_check( serialize::unsigned_to_signed( 1 ) == -1 );
    serialize_check( serialize::unsigned_to_signed( 2 ) == +1 );
    serialize_check( serialize::unsigned_to_signed( 3 ) == -2 );
    serialize_check( serialize::unsigned_to_signed( 4 ) == +2 );
    serialize_check( serialize::unsigned_to_signed( 0xFFFFFFFE ) == INT32_MAX );
    serialize_check( serialize::unsigned_to_signed( 0xFFFFFFFF ) == INT32_MIN );

    const int32_t values[] = { 0, -1, +1, -2, +2, 12345, -12345, INT32_MAX, INT32_MIN };

    for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
    {
        serialize_check( serialize::unsigned_to_signed( serialize::signed_to_unsigned( values[i] ) ) == values[i] );
    }
}

const int MaxItems = 11;

struct TestData
{
    TestData()
    {
        memset( this, 0, sizeof( TestData ) );
    }

    int a,b,c;
    uint32_t d : 8;
    uint32_t e : 8;
    uint32_t f : 8;
    bool g;
    int numItems;
    int items[MaxItems];
    float float_value;
    float compressed_float_value;
    double double_value;
    uint8_t uint8_value;
    uint16_t uint16_value;
    uint32_t uint32_value;
    uint64_t uint64_value;
    int int_relative;
    int64_t int64_full;
    int64_t int64_range;
    uint8_t bytes[17];
    char string[256];
    wchar_t wstring[256];
};

struct TestContext
{
    int min;
    int max;
};

struct TestObject
{
    TestData data;

    void Init()
    {
        data.a = 1;
        data.b = -2;
        data.c = 150;
        data.d = 55;
        data.e = 255;
        data.f = 127;
        data.g = true;

        data.numItems = MaxItems / 2;
        for ( int i = 0; i < data.numItems; ++i )
            data.items[i] = i + 10;

        data.compressed_float_value = 2.13f;
        data.float_value = 3.1415926f;
        data.double_value = 1 / 3.0;
        data.uint8_value = 123;
        data.uint16_value = 0x1234;
        data.uint32_value = 0x12345678;
        data.uint64_value = 0x1234567898765432L;
        data.int_relative = 5;
        data.int64_full = -123456789012345LL;
        data.int64_range = 4123456789LL;

        for ( int i = 0; i < (int) sizeof( data.bytes ); ++i )
            data.bytes[i] = (uint8_t) ( i + 5 ) * 13;

        serialize_copy_string( data.string, "hello world!", sizeof(data.string) - 1 );

        // Explicit code points, not a cyrillic literal. This header carries no BOM, and MSVC
        // decodes a BOM-less UTF-8 source as the local ANSI code page, silently mangling wide
        // literals. Writing the code points makes this independent of source encoding entirely,
        // which also protects consumers who include this header without /utf-8. The assertions
        // further down already do this for the same family of reason.
        const wchar_t hello_world[] = { 0x043F, 0x0440, 0x0438, 0x0432, 0x0456, 0x0442,    // privit
                                        0x002C, 0x0020,                                    // ", "
                                        0x0441, 0x0432, 0x0456, 0x0442, 0x0021, 0 };       // svit!
        serialize_copy_wstring( data.wstring, hello_world, sizeof(data.wstring) / sizeof(wchar_t) - 1 );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        const TestContext & context = *(const TestContext*) stream.GetContext();

        serialize_int( stream, data.a, context.min, context.max );
        serialize_int( stream, data.b, context.min, context.max );

        serialize_int( stream, data.c, -100, 10000 );

        serialize_bits( stream, data.d, 6 );
        serialize_bits( stream, data.e, 8 );
        serialize_bits( stream, data.f, 7 );

        serialize_align( stream );

        serialize_bool( stream, data.g );

        serialize_int( stream, data.numItems, 0, MaxItems - 1 );
        for ( int i = 0; i < data.numItems; ++i )
            serialize_bits( stream, data.items[i], 8 );

        serialize_float( stream, data.float_value );

        serialize_compressed_float( stream, data.compressed_float_value, 0, 10, 0.01 );

        serialize_double( stream, data.double_value );

        serialize_uint8( stream, data.uint8_value );
        serialize_uint16( stream, data.uint16_value );
        serialize_uint32( stream, data.uint32_value );
        serialize_uint64( stream, data.uint64_value );

        serialize_int_relative( stream, data.a, data.int_relative );

        serialize_int64( stream, data.int64_full, INT64_MIN, INT64_MAX );
        serialize_int64( stream, data.int64_range, -5000000000LL, +5000000000LL );

        serialize_bytes( stream, data.bytes, sizeof( data.bytes ) );

        serialize_string( stream, data.string, sizeof( data.string ) );
        serialize_wstring( stream, data.wstring, sizeof( data.wstring ) / sizeof( wchar_t ) );

        return true;
    }

    bool operator == ( const TestObject & other ) const
    {
        return memcmp( &data, &other.data, sizeof( TestData ) ) == 0;
    }

    bool operator != ( const TestObject & other ) const
    {
        return ! ( *this == other );
    }
};

inline void test_serialize()
{
    const int BufferSize = 1024;

    uint8_t buffer[BufferSize];

    TestContext context;
    context.min = -10;
    context.max = +10;

    serialize::WriteStream writeStream( buffer, BufferSize );

    TestObject writeObject;
    writeObject.Init();
    writeStream.SetContext( &context );
    writeObject.Serialize( writeStream );
    writeStream.Flush();

    const int bytesWritten = writeStream.GetBytesProcessed();

    memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );

    TestObject readObject;
    serialize::ReadStream readStream( buffer, bytesWritten );
    readStream.SetContext( &context );
    readObject.Serialize( readStream );

    serialize_check( readObject == writeObject );
}

bool ReadFunction( serialize::ReadStream & readStream )
{
    // IMPORTANT: You wouldn't normally write a read function like this, but I'm just checking each value as it's read in
    // Note that the only thing the read function has to have is to return bool: true on success, false on failing to read.
    // This is important because protects you from maliciously crafted packets.

    {
        uint32_t value;
        read_bits( readStream, value, 4 );
        serialize_check( value == 13 );
    }

    {
        bool value;
        read_bool( readStream, value );
        serialize_check( value == true );
    }

    {
        uint8_t value;
        read_uint8( readStream, value );
        serialize_check( value == 255 );
    }

    {
        uint16_t value;
        read_uint16( readStream, value );
        serialize_check( value == 65535 );
    }

    {
        uint32_t value;
        read_uint32( readStream, value );
        serialize_check( value == 0xFFFFFFFF );
    }

    {
        uint64_t value;
        read_uint64( readStream, value );
        serialize_check( value == 0xFFFFFFFFFFFFFFFFULL );      // i am very full
    }

    {
        int value;
        read_int( readStream, value, 10, 90 );
        serialize_check( value == 55 );
    }

    {
        int64_t value;
        read_int64( readStream, value, -60000000000LL, 60000000000LL );
        serialize_check( value == -50000000001LL );
    }

    {
        int64_t value = 0;
        read_fixed( readStream, value, 48, 16, -100000, +100000 );
        serialize_check( value == int64_t( 12345 ) * 65536 + 32768 );       // 12345.5 in Q48.16
    }

    {
        serialize::uint128_t value = 0;
        read_uint128( readStream, value );
        serialize_check( value == ( ( serialize::uint128_t( 0x0123456789ABCDEFULL ) << 64 ) | 0xFEDCBA9876543210ULL ) );
    }

    {
        float value;
        read_float( readStream, value );
        serialize_check( value == 100.0f );
    }

    {
        double value;
        read_double( readStream, value );
        serialize_check( value == 1000000000.0 );
    }

    {
        char value[5];
        read_bytes( readStream, value, 5 );
        serialize_check( value[0] == 1 );
        serialize_check( value[1] == 2 );
        serialize_check( value[2] == 3 );
        serialize_check( value[3] == 4 );
        serialize_check( value[4] == 5 );
    }

    {
        char string[10];
        read_string( readStream, string, 10 );
        serialize_check( string[0] == 'h' );
        serialize_check( string[1] == 'e' );
        serialize_check( string[2] == 'l' );
        serialize_check( string[3] == 'l' );
        serialize_check( string[4] == 'o' );
        serialize_check( string[5] == '\0' );
    }

    {
        wchar_t wstring[20];
        read_wstring( readStream, wstring, 20 );
        // explicit code points rather than cyrillic literals: serialize_check stringizes its
        // condition into a narrow string, which warns as C4566 on MSVC with a western code page
        serialize_check( wstring[0] == 0x043F );        // 'п'
        serialize_check( wstring[1] == 0x0440 );        // 'р'
        serialize_check( wstring[2] == 0x0438 );        // 'и'
        serialize_check( wstring[3] == 0x0432 );        // 'в'
        serialize_check( wstring[4] == 0x0456 );        // 'і'
        serialize_check( wstring[5] == 0x0442 );        // 'т'
    }

    read_align( readStream );

    TestContext context;
    context.min = -10;
    context.max = +10;

    readStream.SetContext( &context );
    {
        TestObject expectedObject;
        expectedObject.Init();

        TestObject readObject;

        read_object( readStream, readObject );

        serialize_check( readObject == expectedObject );
    }

    {
        int value;
        read_int_relative( readStream, 100, value );
        serialize_check( value == 105 );
    }

    return true;
}

inline void test_read_write()
{
    const int BufferSize = 10 * 1024;

    uint8_t buffer[BufferSize];

    int bytesWritten = 0;

    // write to the buffer
    {
        serialize::WriteStream writeStream;
        writeStream.Initialize( buffer, BufferSize );

        write_bits( writeStream, 13, 4 );
        write_bool( writeStream, true );
        write_uint8( writeStream, 255 );
        write_uint16( writeStream, 65535 );
        write_uint32( writeStream, 0xFFFFFFFF );
        write_uint64( writeStream, 0xFFFFFFFFFFFFFFFFULL );
        write_int( writeStream, 55, 10, 90 );
        write_int64( writeStream, -50000000001LL, -60000000000LL, 60000000000LL );

        int64_t fixed_point_value = int64_t( 12345 ) * 65536 + 32768;               // 12345.5 in Q48.16
        write_fixed( writeStream, fixed_point_value, 48, 16, -100000, +100000 );

        serialize::uint128_t big_value = ( serialize::uint128_t( 0x0123456789ABCDEFULL ) << 64 ) | 0xFEDCBA9876543210ULL;
        write_uint128( writeStream, big_value );

        write_float( writeStream, 100.0f );
        write_double( writeStream, 1000000000.0f );

        char data[5] = { 1, 2, 3, 4, 5 };
        write_bytes( writeStream, data, 5 );

        const char * string = "hello";
        write_string( writeStream, string, 10 );

        // explicit code points, see the note above
        const wchar_t wstring_storage[] = { 0x043F, 0x0440, 0x0438, 0x0432, 0x0456, 0x0442, 0 };
        const wchar_t * wstring = wstring_storage;
        write_wstring( writeStream, wstring, 20 );

        write_align( writeStream );

        TestContext context;
        context.min = -10;
        context.max = +10;

        writeStream.SetContext( &context );

        TestObject object;
        object.Init();

        write_object( writeStream, object );

        write_int_relative( writeStream, 100, 105 );

        writeStream.Flush();

        bytesWritten = writeStream.GetBytesProcessed();

        memset( buffer + bytesWritten, 0, BufferSize - bytesWritten );
    }

    // read from the buffer
    {
        serialize::ReadStream readStream;
        readStream.Initialize( buffer, bytesWritten );
        serialize_check( ReadFunction( readStream ) );
    }
}

inline void test_serialize_integer_validation()
{
    // bits_required(0,5) is 3 bits, so a malicious packet can encode 6 or 7. reads must reject values above max.
    uint8_t buffer[4 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

    serialize::WriteStream writeStream( buffer, 8 );
    uint32_t out_of_range = 7;
    writeStream.SerializeBits( out_of_range, 3 );
    writeStream.Flush();

    serialize::ReadStream readStream( buffer, 4 );
    int32_t value = 0;
    serialize_check( readStream.SerializeInteger( value, 0, 5 ) == false );
}

inline void test_serialize_degenerate_range()
{
    // STANDARD.md: a degenerate range where min == max costs ZERO BITS -- the
    // value is known from the range alone and nothing is written. This is not
    // a curiosity: schema emits it for empty enums and empty types.
    //
    // The library used to assert( min < max ), so a debug build aborted on
    // exactly the case the format defines and the release build handled
    // correctly. This test is here so debug and release cannot drift apart
    // again.
    uint8_t buffer[16] = { 0 };

    serialize::WriteStream writeStream( buffer, 16 );
    int32_t degenerate = 5;
    int32_t after = 3;
    serialize_check( writeStream.SerializeInteger( degenerate, 5, 5 ) );
    serialize_check( writeStream.GetBitsProcessed() == 0 );      // nothing written
    serialize_check( writeStream.SerializeInteger( after, 0, 7 ) );
    serialize_check( writeStream.GetBitsProcessed() == 3 );      // the NEXT field starts at bit 0
    writeStream.Flush();

    serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
    int32_t read_degenerate = 0;
    int32_t read_after = 0;
    serialize_check( readStream.SerializeInteger( read_degenerate, 5, 5 ) );
    serialize_check( read_degenerate == 5 );                     // recovered from the range
    serialize_check( readStream.GetBitsProcessed() == 0 );
    serialize_check( readStream.SerializeInteger( read_after, 0, 7 ) );
    serialize_check( read_after == 3 );

    // and the measure stream must agree that it costs nothing
    serialize::MeasureStream measureStream;
    int32_t measured = 5;
    serialize_check( measureStream.SerializeInteger( measured, 5, 5 ) );
    serialize_check( measureStream.GetBitsProcessed() == 0 );
}

inline void test_serialize_degenerate_range_64()
{
    // The 64 bit twin of the test above, and it did NOT hold when written.
    //
    // 1.6.x relaxed the degenerate range in nine places and missed the 64 bit
    // path entirely: SerializeInteger64 had no bits == 0 early return, so it
    // fell through to WriteBits( value, 0 ) -- which asserts bits > 0 -- while
    // serialize_int64, read_int64 and write_int64 all asserted min < max above
    // it, stricter than the very method they call (SerializeInteger64 already
    // asserted min <= max). So C++ aborted on a degenerate 64 bit range that
    // C, C#, Go and Rust all accept, and the divergence was invisible because
    // no vector in the family used one.
    //
    // The bounds below are deliberately wider than 2^32 so the field would
    // take the two-dword path if it took any path at all.
    uint8_t buffer[16] = { 0 };

    const int64_t point = int64_t(1) << 40;

    serialize::WriteStream writeStream( buffer, 16 );
    int64_t degenerate = point;
    int32_t after = 3;
    serialize_check( writeStream.SerializeInteger64( degenerate, point, point ) );
    serialize_check( writeStream.GetBitsProcessed() == 0 );      // nothing written
    serialize_check( writeStream.SerializeInteger( after, 0, 7 ) );
    serialize_check( writeStream.GetBitsProcessed() == 3 );      // the NEXT field starts at bit 0
    writeStream.Flush();

    serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
    int64_t read_degenerate = 0;
    int32_t read_after = 0;
    serialize_check( readStream.SerializeInteger64( read_degenerate, point, point ) );
    serialize_check( read_degenerate == point );                 // recovered from the range
    serialize_check( readStream.GetBitsProcessed() == 0 );
    serialize_check( readStream.SerializeInteger( read_after, 0, 7 ) );
    serialize_check( read_after == 3 );

    serialize::MeasureStream measureStream;
    int64_t measured = point;
    serialize_check( measureStream.SerializeInteger64( measured, point, point ) );
    serialize_check( measureStream.GetBitsProcessed() == 0 );
}

inline void test_serialize_integer_full_range()
{
    // ranges wider than 2^31 overflow if [min,max] arithmetic is done signed (undefined behavior)
    const int32_t values[] = { INT32_MIN, INT32_MIN + 1, -1, 0, +1, INT32_MAX - 1, INT32_MAX };

    for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        serialize_check( writeStream.SerializeInteger( values[i], INT32_MIN, INT32_MAX ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        int32_t value = 0;
        serialize_check( readStream.SerializeInteger( value, INT32_MIN, INT32_MAX ) == true );
        serialize_check( value == values[i] );
    }

    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        serialize_check( writeStream.SerializeInteger( 1000000000, -2000000000, 2000000000 ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        int32_t value = 0;
        serialize_check( readStream.SerializeInteger( value, -2000000000, 2000000000 ) == true );
        serialize_check( value == 1000000000 );
    }
}

inline void test_serialize_int64_full_range()
{
    // ranges wider than 2^63 overflow if [min,max] arithmetic is done signed (undefined behavior)
    {
        const int64_t values[] = { INT64_MIN, INT64_MIN + 1, -1, 0, +1, INT64_MAX - 1, INT64_MAX };

        for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
        {
            uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

            serialize::WriteStream writeStream( buffer, 16 );
            serialize_check( writeStream.SerializeInteger64( values[i], INT64_MIN, INT64_MAX ) == true );
            writeStream.Flush();

            serialize::ReadStream readStream( buffer, 16 );
            int64_t value = 0;
            serialize_check( readStream.SerializeInteger64( value, INT64_MIN, INT64_MAX ) == true );
            serialize_check( value == values[i] );
        }
    }

    // ranges spanning more than 32 bits use the two dword path
    {
        const int64_t min = -5000000000LL;
        const int64_t max = +5000000000LL;
        const int64_t values[] = { min, min + 1, -1, 0, +1, 4123456789LL, max - 1, max };

        for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
        {
            uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

            serialize::WriteStream writeStream( buffer, 16 );
            serialize_check( writeStream.SerializeInteger64( values[i], min, max ) == true );
            writeStream.Flush();

            serialize::ReadStream readStream( buffer, 16 );
            int64_t value = 0;
            serialize_check( readStream.SerializeInteger64( value, min, max ) == true );
            serialize_check( value == values[i] );
        }
    }

    // small ranges use the single dword path and the minimal number of bits
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        serialize_check( writeStream.SerializeInteger64( 55, -100, +100 ) == true );
        writeStream.Flush();

        serialize_check( writeStream.GetBitsProcessed() == 8 );        // bits_required64(-100,100) == 8, same as the 32 bit path

        serialize::ReadStream readStream( buffer, 8 );
        int64_t value = 0;
        serialize_check( readStream.SerializeInteger64( value, -100, +100 ) == true );
        serialize_check( value == 55 );
    }
}

inline void test_serialize_int64_validation()
{
    // a malicious packet can smuggle an out of range value into the bit headroom of the two dword path. reads must reject it.
    {
        uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 16 );
        const uint64_t out_of_range = ( 1ULL << 34 ) + 5;               // range [0, 2^34] is 35 bits, so values above 2^34 fit in the headroom
        uint32_t lo = uint32_t( out_of_range & 0xFFFFFFFF );
        uint32_t hi = uint32_t( out_of_range >> 32 );
        writeStream.SerializeBits( lo, 32 );
        writeStream.SerializeBits( hi, 3 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 16 );
        int64_t value = 0;
        serialize_check( readStream.SerializeInteger64( value, 0, int64_t( 1ULL << 34 ) ) == false );
    }

    // reads past the end of the buffer must fail cleanly
    {
        uint8_t buffer[4 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::ReadStream readStream( buffer, 4 );
        int64_t value = 0;
        serialize_check( readStream.SerializeInteger64( value, INT64_MIN, INT64_MAX ) == false );
    }
}

inline void test_serialize_bytes_validation()
{
    // negative and huge byte counts must be rejected, not overflow the bounds check in bits
    uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data
    uint8_t data[16];

    {
        serialize::ReadStream readStream( buffer, 16 );
        serialize_check( readStream.SerializeBytes( data, -1 ) == false );
    }

    {
        serialize::ReadStream readStream( buffer, 16 );
        serialize_check( readStream.SerializeBytes( data, 1 << 29 ) == false );
    }
}

inline void test_wstring_validation()
{
    // Wide-string coverage. Before this existed the only exercise of the wstring path
    // was incidental — a field inside test_serialize, a pair of calls in test_read_write,
    // and the golden vector. None of them covered the boundaries, and none covered the
    // documented failure behaviour at all. See STANDARD.md, "wstring".

    const int BufferSize = 32;

    // empty string: length 0, no characters, and the reader appends the terminator
    {
        uint8_t buffer[256];
        wchar_t empty[BufferSize] = { 0 };
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        serialize_check( serialize::serialize_wstring_internal( writeStream, empty, BufferSize ) );
        writeStream.Flush();

        wchar_t read_back[BufferSize];
        memset( read_back, 0xFF, sizeof( read_back ) );
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_wstring_internal( readStream, read_back, BufferSize ) );
        serialize_check( read_back[0] == L'\0' );
    }

    // longest legal string: buffer_size - 1 characters, since the terminator is not sent
    {
        uint8_t buffer[512];
        wchar_t full[BufferSize];
        for ( int i = 0; i < BufferSize - 1; ++i )
            full[i] = (wchar_t) ( 0x0041 + ( i % 26 ) );
        full[BufferSize - 1] = 0;

        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        serialize_check( serialize::serialize_wstring_internal( writeStream, full, BufferSize ) );
        writeStream.Flush();

        wchar_t read_back[BufferSize];
        memset( read_back, 0xFF, sizeof( read_back ) );
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_wstring_internal( readStream, read_back, BufferSize ) );
        serialize_check( wcscmp( read_back, full ) == 0 );
    }

    // the measure stream must agree with the write stream on cost
    {
        uint8_t buffer[256];
        wchar_t text[BufferSize] = { 0x0041, 0x0042, 0x0043, 0 };

        serialize::MeasureStream measureStream;
        serialize_check( serialize::serialize_wstring_internal( measureStream, text, BufferSize ) );

        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        serialize_check( serialize::serialize_wstring_internal( writeStream, text, BufferSize ) );
        writeStream.Flush();

        serialize_check( measureStream.GetBitsProcessed() == writeStream.GetBitsProcessed() );
    }

    // A GROUP ABOVE 0xFFFF IS NOT A UTF-16 CODE UNIT (STANDARD.md, adopted 2026-08-15):
    // each 32 bit group carries one code unit, so no conforming writer can emit a larger
    // value — astral text travels as a surrogate pair. The doctored group is REFUSED on
    // every platform, subsuming the old width-dependent behaviour (accept on 4 byte
    // wchar_t, refuse on 2), whose acceptance half was a wire divergence between
    // platforms once each group became one unit. The value is planted with raw bit
    // operations so the test does not depend on the local width to produce it.
    {
        uint8_t buffer[256];
        const uint32_t above_bmp = 0x0001F600;      // beyond 16 bits by construction

        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        serialize_check( writeStream.SerializeInteger( 1, 0, BufferSize - 1 ) );   // length 1
        serialize_check( writeStream.SerializeBits( above_bmp, 32 ) );
        writeStream.Flush();

        wchar_t read_back[BufferSize];
        memset( read_back, 0, sizeof( read_back ) );
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        const bool result = serialize::serialize_wstring_internal( readStream, read_back, BufferSize );

        serialize_check( result == false );
        serialize_check( read_back[0] != (wchar_t) ( above_bmp & 0xFFFF ) );    // nothing truncated left behind
    }
}

inline void test_wstring_utf16_code_units()
{
    // wstring transmits UTF-16 CODE UNITS: an astral code point is a surrogate pair on the
    // wire, and 2 and 4 byte wchar_t platforms produce IDENTICAL bytes — the 4 byte platform
    // converts at the boundary, splitting on write and recombining on read (STANDARD.md,
    // adopted 2026-08-15). The input is built per the local width, the expected bytes are the
    // code unit stream spelled out directly with raw bit operations, and the two must agree
    // everywhere. Mirrors the vector serialize.c pins in test/roundtrip.c.

    const int BufferSize = 8;

    // U+1F600 is the surrogate pair 0xD83D 0xDE00: one character but two units on a 4 byte
    // wchar_t platform. On a 2 byte platform the string already holds the pair.
    wchar_t ws_in[8];
    if ( sizeof( wchar_t ) >= 4 )
    {
        ws_in[0] = (wchar_t) 0x0001F600;
        ws_in[1] = (wchar_t) 0x0041;
        ws_in[2] = L'\0';
    }
    else
    {
        ws_in[0] = (wchar_t) 0xD83DU;
        ws_in[1] = (wchar_t) 0xDE00U;
        ws_in[2] = (wchar_t) 0x0041;
        ws_in[3] = L'\0';
    }

    // the wire, spelled out with raw bit operations: three units in a [0,7] length field,
    // then each unit as a 32 bit group
    uint8_t expected_bytes[64];
    memset( expected_bytes, 0, sizeof( expected_bytes ) );
    int expected_bytes_processed = 0;
    {
        serialize::WriteStream expectedStream( expected_bytes, sizeof( expected_bytes ) );
        serialize_check( expectedStream.SerializeInteger( 3, 0, BufferSize - 1 ) );
        uint32_t high_surrogate = 0xD83D;
        uint32_t low_surrogate = 0xDE00;
        uint32_t letter_a = 0x0041;
        serialize_check( expectedStream.SerializeBits( high_surrogate, 32 ) );
        serialize_check( expectedStream.SerializeBits( low_surrogate, 32 ) );
        serialize_check( expectedStream.SerializeBits( letter_a, 32 ) );
        expectedStream.Flush();
        expected_bytes_processed = expectedStream.GetBytesProcessed();
    }

    // write side: the unified serialize path must produce exactly those bytes
    int bits_written = 0;
    uint8_t buffer[64];
    {
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        serialize_check( serialize::serialize_wstring_internal( writeStream, ws_in, BufferSize ) );
        writeStream.Flush();
        serialize_check( writeStream.GetBytesProcessed() == expected_bytes_processed );
        serialize_check( memcmp( buffer, expected_bytes, expected_bytes_processed ) == 0 );
        bits_written = writeStream.GetBitsProcessed();
    }

    // the measure counts the units transmitted, not the characters held
    {
        serialize::MeasureStream measureStream;
        serialize_check( serialize::serialize_wstring_internal( measureStream, ws_in, BufferSize ) );
        serialize_check( measureStream.GetBitsProcessed() == bits_written );
    }

    // the write-only macro form must stay byte-identical to the unified path
    // (STANDARD.md, "Read-only and write-only forms")
    {
        uint8_t macro_buffer[64];
        memset( macro_buffer, 0, sizeof( macro_buffer ) );
        serialize::WriteStream writeStream( macro_buffer, sizeof( macro_buffer ) );
        const wchar_t * wstring = ws_in;
        write_wstring( writeStream, wstring, BufferSize );
        writeStream.Flush();
        serialize_check( writeStream.GetBytesProcessed() == expected_bytes_processed );
        serialize_check( memcmp( macro_buffer, expected_bytes, expected_bytes_processed ) == 0 );
    }

    // read side: the code unit stream decodes to the platform's own representation —
    // recombined on 4 byte wchar_t, the pair itself on 2 byte
    {
        wchar_t read_back[BufferSize];
        memset( read_back, 0xFF, sizeof( read_back ) );
        serialize::ReadStream readStream( expected_bytes, expected_bytes_processed );
        serialize_check( serialize::serialize_wstring_internal( readStream, read_back, BufferSize ) );
        serialize_check( wcscmp( read_back, ws_in ) == 0 );
    }
}

inline void test_string_read_validation()
{
    // STANDARD.md, "Readers must refuse malformed string payloads" (adopted 2026-08-15).
    // Every refused stream below is DOCTORED with raw bit operations — no conforming
    // writer can produce it — and the refusal binds in every build mode. Proven
    // red-then-green: against the pre-ruling reader, every doctored stream here was
    // ACCEPTED (harness run recorded with the ruling batch); with the refusals in
    // place they are REFUSED, and the valid-content controls are unchanged.

    const int BufferSize = 16;

    // invalid UTF-8: 0xFF can never appear anywhere in well-formed UTF-8
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        int32_t length = 3;
        serialize_check( writeStream.SerializeInteger( length, 0, BufferSize - 1 ) );
        const uint8_t payload[3] = { 0xFF, 0xFE, 0xFF };
        serialize_check( writeStream.SerializeBytes( payload, 3 ) );
        writeStream.Flush();

        char read_back[BufferSize];
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_string_internal( readStream, read_back, BufferSize ) == false );
    }

    // truncated UTF-8: a 3 byte lead as the final transmitted byte
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        int32_t length = 2;
        serialize_check( writeStream.SerializeInteger( length, 0, BufferSize - 1 ) );
        const uint8_t payload[2] = { 'a', 0xE2 };
        serialize_check( writeStream.SerializeBytes( payload, 2 ) );
        writeStream.Flush();

        char read_back[BufferSize];
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_string_internal( readStream, read_back, BufferSize ) == false );
    }

    // interior NUL: wire length 3, strlen 1 — the TWO-LENGTHS smuggling primitive. A
    // conforming writer derives the length from strlen, so this stream is impossible
    // from conformance; accepted, it hands the application a payload whose wire length
    // and C length disagree, and everything between them travels invisibly.
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        int32_t length = 3;
        serialize_check( writeStream.SerializeInteger( length, 0, BufferSize - 1 ) );
        const uint8_t payload[3] = { 'a', 0x00, 'b' };
        serialize_check( writeStream.SerializeBytes( payload, 3 ) );
        writeStream.Flush();

        char read_back[BufferSize];
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_string_internal( readStream, read_back, BufferSize ) == false );
    }

    // control: valid multi-byte UTF-8 — 2, 3 and 4 byte sequences, built from explicit
    // bytes so the source file encoding can never change the test — still round trips.
    // The refusal costs no valid stream.
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        char text[BufferSize];
        const uint8_t utf8[11] = { 'h', 0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80, 0 };  // h, e-acute, euro sign, U+1F600
        memcpy( text, utf8, sizeof( utf8 ) );

        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        serialize_check( serialize::serialize_string_internal( writeStream, text, BufferSize ) );
        writeStream.Flush();

        char read_back[BufferSize];
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_string_internal( readStream, read_back, BufferSize ) );
        serialize_check( strcmp( read_back, text ) == 0 );
    }
}

inline void test_wstring_read_validation()
{
    // STANDARD.md, "Readers must refuse malformed wstring payloads" (adopted 2026-08-15).
    // Same shape as the narrow test above: doctored streams refused in every build mode,
    // proven red-then-green, valid content unchanged.

    const int BufferSize = 8;

    // high surrogate followed by a non-surrogate: unpaired, refused
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        int32_t length = 2;
        serialize_check( writeStream.SerializeInteger( length, 0, BufferSize - 1 ) );
        uint32_t group0 = 0xD800;
        uint32_t group1 = 0x0041;
        serialize_check( writeStream.SerializeBits( group0, 32 ) );
        serialize_check( writeStream.SerializeBits( group1, 32 ) );
        writeStream.Flush();

        wchar_t read_back[BufferSize];
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_wstring_internal( readStream, read_back, BufferSize ) == false );
    }

    // low surrogate with no high before it: refused
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        int32_t length = 1;
        serialize_check( writeStream.SerializeInteger( length, 0, BufferSize - 1 ) );
        uint32_t group0 = 0xDC00;
        serialize_check( writeStream.SerializeBits( group0, 32 ) );
        writeStream.Flush();

        wchar_t read_back[BufferSize];
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_wstring_internal( readStream, read_back, BufferSize ) == false );
    }

    // high surrogate as the final transmitted group: dangling, refused
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        int32_t length = 1;
        serialize_check( writeStream.SerializeInteger( length, 0, BufferSize - 1 ) );
        uint32_t group0 = 0xD83D;
        serialize_check( writeStream.SerializeBits( group0, 32 ) );
        writeStream.Flush();

        wchar_t read_back[BufferSize];
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_wstring_internal( readStream, read_back, BufferSize ) == false );
    }

    // interior NUL group: wire length 3, wcslen 1 — the same two-lengths primitive as
    // the narrow string, refused
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        int32_t length = 3;
        serialize_check( writeStream.SerializeInteger( length, 0, BufferSize - 1 ) );
        uint32_t group0 = 0x0041;
        uint32_t group1 = 0x0000;
        uint32_t group2 = 0x0042;
        serialize_check( writeStream.SerializeBits( group0, 32 ) );
        serialize_check( writeStream.SerializeBits( group1, 32 ) );
        serialize_check( writeStream.SerializeBits( group2, 32 ) );
        writeStream.Flush();

        wchar_t read_back[BufferSize];
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_wstring_internal( readStream, read_back, BufferSize ) == false );
    }

    // control: a well-formed surrogate PAIR is valid UTF-16 — it is how astral text
    // travels — and must be ACCEPTED. Against main's reader the pair is stored as its
    // two units on every platform; recombination on 4 byte wchar_t is the surrogate
    // boundary branch's work and composes on top of this check.
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, sizeof( buffer ) );
        int32_t length = 2;
        serialize_check( writeStream.SerializeInteger( length, 0, BufferSize - 1 ) );
        uint32_t group0 = 0xD83D;
        uint32_t group1 = 0xDE00;
        serialize_check( writeStream.SerializeBits( group0, 32 ) );
        serialize_check( writeStream.SerializeBits( group1, 32 ) );
        writeStream.Flush();

        wchar_t read_back[BufferSize];
        memset( read_back, 0, sizeof( read_back ) );
        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize_check( serialize::serialize_wstring_internal( readStream, read_back, BufferSize ) == true );
    }
}

inline void test_int_relative_validation()
{
    // the 32 bit fallback must reject values that violate the previous < current contract
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        uint32_t six_false_bools = 0;
        writeStream.SerializeBits( six_false_bools, 6 );
        uint32_t bad_current = 50;
        writeStream.SerializeBits( bad_current, 32 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        int previous = 100;
        int current = 0;
        serialize_check( serialize::serialize_int_relative_internal( readStream, previous, current ) == false );
    }

    // a legitimate fallback round trip must still succeed
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        int previous = 100;
        int written = 100000;
        serialize_check( serialize::serialize_int_relative_internal( writeStream, previous, written ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        int current = 0;
        serialize_check( serialize::serialize_int_relative_internal( readStream, previous, current ) == true );
        serialize_check( current == written );
    }

    // gaps wider than 2^31 overflow if the difference is computed in signed arithmetic (undefined behavior)
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        int previous = -1000;
        int written = INT32_MAX;
        serialize_check( serialize::serialize_int_relative_internal( writeStream, previous, written ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        int current = 0;
        serialize_check( serialize::serialize_int_relative_internal( readStream, previous, current ) == true );
        serialize_check( current == written );
    }

    // read side reconstructs current = previous + difference; a large previous overflows signed arithmetic.
    // this must wrap in the unsigned domain rather than invoke undefined behavior.
    {
        // difference of 1 exercises the oneBit branch, difference of 5 exercises a bucket branch
        const int differences[] = { 1, 5 };

        for ( int d = 0; d < (int) ( sizeof(differences) / sizeof(differences[0]) ); d++ )
        {
            uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

            serialize::WriteStream writeStream( buffer, 8 );
            int prevWrite = 10;
            int curWrite = prevWrite + differences[d];
            serialize_check( serialize::serialize_int_relative_internal( writeStream, prevWrite, curWrite ) == true );
            writeStream.Flush();

            serialize::ReadStream readStream( buffer, 8 );
            int previous = INT32_MAX;                        // previous + difference exceeds INT32_MAX
            int current = 0;
            serialize_check( serialize::serialize_int_relative_internal( readStream, previous, current ) == true );
            serialize_check( current == int32_t( uint32_t( INT32_MAX ) + uint32_t( differences[d] ) ) );
        }
    }
}

inline void test_compressed_float_validation()
{
    // a malicious packet can encode integer values above maxIntegerValue in the bit headroom. reads must reject them.
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        uint32_t out_of_range = 1023;                       // maxIntegerValue is 1000 for [0,10] at res 0.01 -> 10 bits
        writeStream.SerializeBits( out_of_range, 10 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        float value = 0.0f;
        serialize_check( serialize::serialize_compressed_float_internal( readStream, value, 0.0f, 10.0f, 0.01f ) == false );
    }

    // huge delta / res ratios must not overflow the uint32 quantization range (undefined behavior)
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        float written = 5000000000.0f;
        serialize_check( serialize::serialize_compressed_float_internal( writeStream, written, 0.0f, 10000000000.0f, 1.0f ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        float value = 0.0f;
        serialize_check( serialize::serialize_compressed_float_internal( readStream, value, 0.0f, 10000000000.0f, 1.0f ) == true );
        serialize_check( fabs( value - written ) <= 4096.0f );
    }

    // writing NaN is non-conforming and asserts in debug (STANDARD.md, adopted 2026-08-15;
    // test_compressed_float_non_finite_asserts proves the assert fires). in release the
    // asserts compile out and the clamp is the backstop: a NaN value must not reach the
    // uint32 cast (clamp comparisons are all false for NaN).
#if defined( NDEBUG )
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        uint32_t nan_bits = 0x7fc00000;                 // quiet NaN bit pattern, built without the NAN macro (finite-math builds reject it)
        float written = 0.0f;
        memcpy( &written, &nan_bits, 4 );
        serialize_check( serialize::serialize_compressed_float_internal( writeStream, written, 0.0f, 10.0f, 0.01f ) == true );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        float value = -1.0f;
        serialize_check( serialize::serialize_compressed_float_internal( readStream, value, 0.0f, 10.0f, 0.01f ) == true );
        serialize_check( value >= 0.0f && value <= 10.0f );      // NaN clamps to the low end of the range
    }
#endif // #if defined( NDEBUG )
}

#if !defined( NDEBUG ) && !defined( _WIN32 )

#include <unistd.h>         // fork, _exit
#include <sys/wait.h>       // waitpid

// run fn in a forked child with stderr silenced, and report whether it died on a signal --
// which is what a fired debug assert looks like from outside (assert calls abort, SIGABRT).
inline bool serialize_test_assert_fires( void (*fn)() )
{
    fflush( stdout );
    fflush( stderr );
    pid_t pid = fork();
    if ( pid == 0 )
    {
        freopen( "/dev/null", "w", stderr );    // the child's assert message is the expected outcome, not test noise
        fn();
        _exit( 0 );                             // the assert did not fire
    }
    if ( pid < 0 )
        return false;
    int status = 0;
    waitpid( pid, &status, 0 );
    return WIFSIGNALED( status );
}

inline void serialize_test_write_non_finite_declaration()
{
    // delta = max - min overflows float32 to +Inf: a non-conforming declaration
    uint8_t buffer[8] = { 0 };
    serialize::WriteStream writeStream( buffer, 8 );
    float value = 0.0f;
    serialize::serialize_compressed_float_internal( writeStream, value, -3e38f, 3e38f, 1.0f );
}

inline void serialize_test_write_non_finite_value()
{
    // a NaN value over a perfectly good declaration: a non-conforming write
    uint8_t buffer[8] = { 0 };
    serialize::WriteStream writeStream( buffer, 8 );
    uint32_t nan_bits = 0x7fc00000;             // quiet NaN bit pattern, built without the NAN macro (finite-math builds reject it)
    float value = 0.0f;
    memcpy( &value, &nan_bits, 4 );
    serialize::serialize_compressed_float_internal( writeStream, value, 0.0f, 10.0f, 0.01f );
}

inline void serialize_test_write_non_finite_value_precomputed()
{
    // the same non-conforming NaN write, through the precomputed entry point directly
    uint8_t buffer[8] = { 0 };
    serialize::WriteStream writeStream( buffer, 8 );
    uint32_t nan_bits = 0x7fc00000;             // quiet NaN bit pattern, built without the NAN macro (finite-math builds reject it)
    float value = 0.0f;
    memcpy( &value, &nan_bits, 4 );
    serialize::serialize_compressed_float_precomputed_internal( writeStream, value, 1000, 10, 10.0f, 0.0f );
}

inline void serialize_test_precomputed_inconsistent_bits()
{
    // a wire width that disagrees with the step count is a caller bug: the field would not
    // occupy the width every other conforming implementation of the declaration expects
    uint8_t buffer[8] = { 0 };
    serialize::WriteStream writeStream( buffer, 8 );
    float value = 5.0f;
    serialize::serialize_compressed_float_precomputed_internal( writeStream, value, 1000, 11, 10.0f, 0.0f );
}

#endif // #if !defined( NDEBUG ) && !defined( _WIN32 )

inline void test_compressed_float_non_finite_asserts()
{
    // STANDARD.md, compressed_float (adopted 2026-08-15): a declaration whose delta or
    // values is not finite in float32 is non-conforming, and writing a non-finite value is
    // non-conforming -- conforming writers assert in debug builds. prove each assert fires,
    // in a forked child so the abort is observed rather than suffered.
#if !defined( NDEBUG ) && !defined( _WIN32 )
    serialize_check( serialize_test_assert_fires( serialize_test_write_non_finite_declaration ) == true );
    serialize_check( serialize_test_assert_fires( serialize_test_write_non_finite_value ) == true );
#else
    // silently skipped: needs an assert-enabled build and fork. An expected
    // skip is not information — the suite prints test names and failures only.
#endif // #if !defined( NDEBUG ) && !defined( _WIN32 )
}

inline void test_compressed_float_precomputed_asserts()
{
    // the precomputed entry point carries the same write-side contract as
    // serialize_compressed_float (a non-finite value asserts, STANDARD.md) plus its own:
    // constants that are not what serialize_compressed_float_params derives are a caller
    // bug. prove each assert fires, in a forked child so the abort is observed rather
    // than suffered.
#if !defined( NDEBUG ) && !defined( _WIN32 )
    serialize_check( serialize_test_assert_fires( serialize_test_write_non_finite_value_precomputed ) == true );
    serialize_check( serialize_test_assert_fires( serialize_test_precomputed_inconsistent_bits ) == true );
#else
    // silently skipped: needs an assert-enabled build and fork. An expected
    // skip is not information — the suite prints test names and failures only.
#endif // #if !defined( NDEBUG ) && !defined( _WIN32 )
}

inline void test_compressed_float_precomputed_validation()
{
    // the constants serialize_compressed_float derives on every call, derived once instead:
    // the precomputed read path must refuse the same smuggled integers and accept the same
    // conforming ones as the derive-per-call path.
    uint32_t max_integer_value = 0;
    int bits = 0;
    float delta = 0.0f;
    serialize::serialize_compressed_float_params( 0.0f, 10.0f, 0.01f, max_integer_value, bits, delta );
    serialize_check( max_integer_value == 1000 );
    serialize_check( bits == 10 );
    serialize_check( delta == 10.0f );

    // a malicious packet can encode integer values above max_integer_value in the bit headroom. reads must reject them.
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        uint32_t out_of_range = 1023;                       // max_integer_value is 1000 for [0,10] at res 0.01 -> 10 bits
        writeStream.SerializeBits( out_of_range, 10 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        float value = 0.0f;
        serialize_check( serialize::serialize_compressed_float_precomputed_internal( readStream, value, max_integer_value, bits, delta, 0.0f ) == false );
    }

    // the highest conforming integer still decodes
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        uint32_t top_of_range = 1000;
        writeStream.SerializeBits( top_of_range, 10 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 8 );
        float value = 0.0f;
        serialize_check( serialize::serialize_compressed_float_precomputed_internal( readStream, value, max_integer_value, bits, delta, 0.0f ) == true );
        serialize_check( value == 10.0f );                  // 1000 / 1000 * 10 + 0: exact at the top of the range
    }
}

// Fixed point test helpers. Every configuration in the matrix runs the same case list, and every
// round trip also runs the measure stream and requires exact agreement with the write stream:
// fixed point serialization involves no alignment, so measure is exact, not just conservative.

template <int IntegerBits, int FractionalBits, int64_t MinUnits, int64_t MaxUnits, typename Storage>
inline void check_fixed_round_trip( Storage raw_value )
{
    uint8_t buffer[32 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

    serialize::WriteStream writeStream( buffer, 32 );
    Storage written = raw_value;
    serialize_check( ( serialize::serialize_fixed_internal<IntegerBits, FractionalBits, MinUnits, MaxUnits>( writeStream, written ) ) == true );
    writeStream.Flush();

    serialize::MeasureStream measureStream;
    Storage measured = raw_value;
    serialize_check( ( serialize::serialize_fixed_internal<IntegerBits, FractionalBits, MinUnits, MaxUnits>( measureStream, measured ) ) == true );
    serialize_check( measureStream.GetBitsProcessed() == writeStream.GetBitsProcessed() );

    serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
    Storage read_back = 0;
    serialize_check( ( serialize::serialize_fixed_internal<IntegerBits, FractionalBits, MinUnits, MaxUnits>( readStream, read_back ) ) == true );
    serialize_check( read_back == raw_value );
}

template <int IntegerBits, int FractionalBits, int64_t MinUnits, int64_t MaxUnits, typename Storage>
inline void check_fixed_cases( Storage one_unit )
{
    // the multiplications keep the Storage typed operand on the left, so the emulated 128 bit
    // types (member operators only) work as Storage here too
    const Storage raw_min = Storage( one_unit * Storage( MinUnits ) );
    const Storage raw_max = Storage( one_unit * Storage( MaxUnits ) );

    // exact raw bounds, and one raw step inside each
    check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( raw_min );
    check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( raw_max );
    check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( Storage( raw_min + 1 ) );
    check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( Storage( raw_max - 1 ) );

    // whole unit values one unit inside each bound
    check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( Storage( one_unit * Storage( MinUnits + 1 ) ) );
    check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( Storage( one_unit * Storage( MaxUnits - 1 ) ) );

    // a value with every fraction bit set
    check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( Storage( raw_min + one_unit - 1 ) );

    // the middle of the range, computed without overflowing the storage type
    check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( Storage( raw_min / 2 + raw_max / 2 ) );

    // zero, one and minus one whole units, where the bounds allow them
    if ( MinUnits <= 0 && MaxUnits >= 0 )
    {
        check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( Storage( 0 ) );
    }
    if ( MinUnits <= 1 && MaxUnits >= 1 )
    {
        check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( one_unit );
    }
    if ( MinUnits <= -1 && MaxUnits >= -1 )
    {
        check_fixed_round_trip<IntegerBits, FractionalBits, MinUnits, MaxUnits>( Storage( Storage( 0 ) - one_unit ) );
    }
}

template <int IntegerBits, int FractionalBits, int64_t MinUnits, int64_t MaxUnits, typename Storage>
inline void check_fixed_rejects_out_of_range( Storage )
{
    // recompute the wire parameters independently of the codec, then hand build a stream encoding
    // an offset of exactly raw_range + 1: one raw step past raw_max, smuggled into the bit headroom
    const uint64_t raw_range = ( uint64_t( MaxUnits ) << FractionalBits ) - ( uint64_t( MinUnits ) << FractionalBits );
    const int bits = serialize::bits_required64( 0, raw_range );

    const uint64_t max_encodable = ( bits < 64 ) ? ( ( uint64_t(1) << bits ) - 1 ) : 0xFFFFFFFFFFFFFFFFULL;
    if ( raw_range == max_encodable )
    {
        return;                             // no headroom: every encoding decodes in range for this configuration
    }

    const uint64_t smuggled = raw_range + 1;

    uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

    serialize::WriteStream writeStream( buffer, 16 );
    if ( bits <= 32 )
    {
        writeStream.SerializeBits( uint32_t( smuggled ), bits );
    }
    else
    {
        writeStream.SerializeBits( uint32_t( smuggled & 0xFFFFFFFF ), 32 );
        writeStream.SerializeBits( uint32_t( smuggled >> 32 ), bits - 32 );
    }
    writeStream.Flush();

    serialize::ReadStream readStream( buffer, 16 );
    Storage value = 0;
    serialize_check( ( serialize::serialize_fixed_internal<IntegerBits, FractionalBits, MinUnits, MaxUnits>( readStream, value ) ) == false );
}

inline void test_serialize_fixed()
{
    // the storage x Q format matrix. every configuration runs the full case list in check_fixed_cases:
    // exact raw bounds, one raw step inside each, whole unit values inside each bound, all fraction
    // bits set, the middle of the range, and zero / +1.0 / -1.0 units where the bounds allow them.

    // int16_t
    check_fixed_cases<8, 8, -100, +100>( int16_t( 256 ) );
    check_fixed_cases<12, 4, -2000, +2000>( int16_t( 16 ) );

    // int32_t
    check_fixed_cases<16, 16, -30000, +30000>( int32_t( 65536 ) );
    check_fixed_cases<24, 8, -8000000, +8000000>( int32_t( 256 ) );
    check_fixed_cases<32, 0, -100000, +100000>( int32_t( 1 ) );                                 // pure integer Q: fraction_bits == 0 is legal

    // int64_t
    check_fixed_cases<48, 16, -100000000000LL, +100000000000LL>( int64_t( 65536 ) );
    check_fixed_cases<32, 32, -1000000, +1000000>( int64_t( 1 ) << 32 );
    check_fixed_cases<64, 0, -5000000000LL, +5000000000LL>( int64_t( 1 ) );                     // pure integer Q at full width

    // unsigned storage
    check_fixed_cases<16, 0, 0, 60000>( uint16_t( 1 ) );
    check_fixed_cases<16, 16, 0, 60000>( uint32_t( 65536 ) );
    check_fixed_cases<48, 16, 0, 1000000000>( uint64_t( 65536 ) );

    // single unit range: the whole wire is the fractional part
    check_fixed_cases<16, 16, 0, 1>( int32_t( 65536 ) );

    // asymmetric bounds
    check_fixed_cases<48, 16, -3, +100000>( int64_t( 65536 ) );

    // the wire cost is a compile time constant of the call site. pin a few
    {
        uint8_t buffer[16];
        serialize::WriteStream stream( buffer, 16 );
        int64_t value = int64_t( 12345 ) * 65536 + 32768;                                       // 12345.5 in Q48.16
        serialize_check( ( serialize::serialize_fixed_internal<48, 16, -100000, +100000>( stream, value ) ) == true );
        serialize_check( stream.GetBitsProcessed() == 34 );         // 200000 << 16 raw values needs 34 bits
    }
    {
        uint8_t buffer[8];
        serialize::WriteStream stream( buffer, 8 );
        int32_t value = 65536 / 2;                                                              // 0.5 in Q16.16
        serialize_check( ( serialize::serialize_fixed_internal<16, 16, 0, 1>( stream, value ) ) == true );
        serialize_check( stream.GetBitsProcessed() == 17 );         // 1 << 16 raw values needs 17 bits
    }
    {
        uint8_t buffer[8];
        serialize::WriteStream stream( buffer, 8 );
        int16_t value = -832;                                                                   // -3.25 in Q8.8
        serialize_check( ( serialize::serialize_fixed_internal<8, 8, -100, +100>( stream, value ) ) == true );
        serialize_check( stream.GetBitsProcessed() == 16 );         // 200 << 8 raw values needs 16 bits
    }

    // compile time refusals. each of the following fails with a serialize_static_assert, which is the point.
    // kept as comments because a compile failure can't run inside the suite: uncomment one to verify.
    //
    //     int32_t value = 0;
    //     serialize::serialize_fixed_internal<16, 8, 0, 100>( stream, value );                 // 16 + 8 != 32: the Q format doesn't fill the storage type
    //     serialize::serialize_fixed_internal<16, 16, -40000, +40000>( stream, value );        // bounds exceed the Q16.16 whole unit capacity [-32768,32767]
    //     serialize::serialize_fixed_internal<16, 16, 200, 100>( stream, value );              // min must not exceed max (min == max is LEGAL: zero bits, see test_serialize_fixed_degenerate)
    //     float not_an_integer = 0.0f;
    //     serialize::serialize_fixed_internal<16, 16, 0, 100>( stream, not_an_integer );       // storage must be an integer type
    //     int runtime_bound = 100;
    //     serialize_fixed( stream, value, 16, 16, 0, runtime_bound );                          // bounds must be compile time constants
}

inline void test_serialize_fixed_validation()
{
    // a malicious packet can smuggle a raw value past raw_max into the bit headroom of the offset
    // encoding. reads must reject one raw step past the top of the range, on every configuration
    // in the matrix that has headroom.
    check_fixed_rejects_out_of_range<8, 8, -100, +100>( int16_t( 0 ) );
    check_fixed_rejects_out_of_range<12, 4, -2000, +2000>( int16_t( 0 ) );
    check_fixed_rejects_out_of_range<16, 16, -30000, +30000>( int32_t( 0 ) );
    check_fixed_rejects_out_of_range<24, 8, -8000000, +8000000>( int32_t( 0 ) );
    check_fixed_rejects_out_of_range<32, 0, -100000, +100000>( int32_t( 0 ) );
    check_fixed_rejects_out_of_range<48, 16, -100000000000LL, +100000000000LL>( int64_t( 0 ) );
    check_fixed_rejects_out_of_range<32, 32, -1000000, +1000000>( int64_t( 0 ) );
    check_fixed_rejects_out_of_range<64, 0, -5000000000LL, +5000000000LL>( int64_t( 0 ) );
    check_fixed_rejects_out_of_range<16, 0, 0, 60000>( uint16_t( 0 ) );
    check_fixed_rejects_out_of_range<16, 16, 0, 60000>( uint32_t( 0 ) );
    check_fixed_rejects_out_of_range<48, 16, 0, 1000000000>( uint64_t( 0 ) );
    check_fixed_rejects_out_of_range<16, 16, 0, 1>( int32_t( 0 ) );
    check_fixed_rejects_out_of_range<48, 16, -3, +100000>( int64_t( 0 ) );

    // reads past the end of the buffer must fail cleanly
    {
        uint8_t buffer[4 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::ReadStream readStream( buffer, 2 );
        int64_t value = 0;
        serialize_check( ( serialize::serialize_fixed_internal<48, 16, -100000000000LL, +100000000000LL>( readStream, value ) ) == false );
    }
}

inline void test_serialize_fixed_matches_int64()
{
    // fraction_bits == 0 is pure integer Q, and for storage of 64 bits or fewer the fixed point
    // wire format is byte identical to serialize_int64 of the raw value over the raw bounds.
    // sweep values and require identical bytes and identical bit counts: this equivalence binds
    // the new path to the proven one.

    const int64_t values[] = { -5000000000LL, -4999999999LL, -1, 0, +1, 12345678, 4999999999LL, 5000000000LL };

    for ( int i = 0; i < (int) ( sizeof( values ) / sizeof( values[0] ) ); i++ )
    {
        // > 32 bit range: the two group path
        uint8_t fixed_buffer[16] = { 0 };
        serialize::WriteStream fixedStream( fixed_buffer, 16 );
        int64_t fixed_value = values[i];
        serialize_check( ( serialize::serialize_fixed_internal<64, 0, -5000000000LL, +5000000000LL>( fixedStream, fixed_value ) ) == true );
        fixedStream.Flush();

        uint8_t int64_buffer[16] = { 0 };
        serialize::WriteStream int64Stream( int64_buffer, 16 );
        serialize_check( int64Stream.SerializeInteger64( values[i], -5000000000LL, +5000000000LL ) == true );
        int64Stream.Flush();

        serialize_check( fixedStream.GetBitsProcessed() == int64Stream.GetBitsProcessed() );
        serialize_check( memcmp( fixed_buffer, int64_buffer, sizeof( fixed_buffer ) ) == 0 );
    }

    // <= 32 bit range: the single group path, on 32 bit storage
    const int32_t narrow_values[] = { -100000, -99999, -1, 0, +1, 54321, 99999, 100000 };

    for ( int i = 0; i < (int) ( sizeof( narrow_values ) / sizeof( narrow_values[0] ) ); i++ )
    {
        uint8_t fixed_buffer[16] = { 0 };
        serialize::WriteStream fixedStream( fixed_buffer, 16 );
        int32_t fixed_value = narrow_values[i];
        serialize_check( ( serialize::serialize_fixed_internal<32, 0, -100000, +100000>( fixedStream, fixed_value ) ) == true );
        fixedStream.Flush();

        uint8_t int64_buffer[16] = { 0 };
        serialize::WriteStream int64Stream( int64_buffer, 16 );
        serialize_check( int64Stream.SerializeInteger64( narrow_values[i], -100000, +100000 ) == true );
        int64Stream.Flush();

        serialize_check( fixedStream.GetBitsProcessed() == int64Stream.GetBitsProcessed() );
        serialize_check( memcmp( fixed_buffer, int64_buffer, sizeof( fixed_buffer ) ) == 0 );
    }

    // the equivalence is not limited to fraction_bits == 0: for any Q format the wire is
    // serialize_int64 of the raw value over the raw bounds. fixed point adds no wire structure,
    // only the compile time scaling convention.
    const int32_t q16_16_raw_values[] = { -30000 * 65536, -( 3 * 65536 + 16384 ), 0, 65536 / 2, 12345 * 65536 + 1, 30000 * 65536 };

    for ( int i = 0; i < (int) ( sizeof( q16_16_raw_values ) / sizeof( q16_16_raw_values[0] ) ); i++ )
    {
        uint8_t fixed_buffer[16] = { 0 };
        serialize::WriteStream fixedStream( fixed_buffer, 16 );
        int32_t fixed_value = q16_16_raw_values[i];
        serialize_check( ( serialize::serialize_fixed_internal<16, 16, -30000, +30000>( fixedStream, fixed_value ) ) == true );
        fixedStream.Flush();

        uint8_t int64_buffer[16] = { 0 };
        serialize::WriteStream int64Stream( int64_buffer, 16 );
        serialize_check( int64Stream.SerializeInteger64( q16_16_raw_values[i], int64_t( -30000 ) * 65536, int64_t( +30000 ) * 65536 ) == true );
        int64Stream.Flush();

        serialize_check( fixedStream.GetBitsProcessed() == int64Stream.GetBitsProcessed() );
        serialize_check( memcmp( fixed_buffer, int64_buffer, sizeof( fixed_buffer ) ) == 0 );
    }
}

template <int IntegerBits, int FractionalBits, int64_t MinUnits, int64_t MaxUnits, typename Storage>
inline void check_fixed_wide_rejects_out_of_range( Storage )
{
    // recompute the wire parameters independently of the codec, then hand build a stream encoding
    // an offset of exactly raw_range + 1: one raw step past raw_max, smuggled into the bit headroom
    const serialize::uint128_t raw_min = serialize::uint128_t( serialize::int128_t( MinUnits ) ) << FractionalBits;
    const serialize::uint128_t raw_max = serialize::uint128_t( serialize::int128_t( MaxUnits ) ) << FractionalBits;
    const serialize::uint128_t raw_range = raw_max - raw_min;

    int bits = 0;
    for ( serialize::uint128_t x = raw_range; x != 0; x >>= 1 )
    {
        bits++;
    }

    const serialize::uint128_t max_encodable = ( bits < 128 ) ? ( ( serialize::uint128_t( 1 ) << bits ) - 1 ) : ~( serialize::uint128_t( 0 ) );
    if ( raw_range == max_encodable )
    {
        return;                             // no headroom: every encoding decodes in range for this configuration
    }

    serialize::uint128_t smuggled = raw_range + 1;

    uint8_t buffer[24 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

    serialize::WriteStream writeStream( buffer, 24 );
    int bits_left = bits;
    while ( bits_left > 0 )
    {
        const int group_bits = ( bits_left < 32 ) ? bits_left : 32;
        writeStream.SerializeBits( uint32_t( uint64_t( smuggled ) & 0xFFFFFFFF ), group_bits );
        smuggled >>= group_bits;
        bits_left -= group_bits;
    }
    writeStream.Flush();

    serialize::ReadStream readStream( buffer, 24 );
    Storage value = 0;
    serialize_check( ( serialize::serialize_fixed_internal<IntegerBits, FractionalBits, MinUnits, MaxUnits>( readStream, value ) ) == false );
}

inline void test_serialize_fixed_wide()
{
#if defined(__SIZEOF_INT128__)
    // compile time bit counts in the 128 bit domain. the metafunction needs native __int128
    // template parameters, so these checks are the one native only piece of this test
    serialize_check( ( serialize::BitsRequired128<0, 0>::result ) == 0 );
    serialize_check( ( serialize::BitsRequired128<0, 1>::result ) == 1 );
    serialize_check( ( serialize::BitsRequired128<0, ( serialize::uint128_t( 1 ) << 64 )>::result ) == 65 );
    serialize_check( ( serialize::BitsRequired128<0, ~( serialize::uint128_t( 0 ) )>::result ) == 128 );
#endif // #if defined(__SIZEOF_INT128__)

    // the matrix, wide: Q112.16 with a raw range past 64 bits (three groups on the wire), Q112.16
    // with a small range (a single group on wide storage), Q64.64 (the fraction alone spans 64 bits),
    // Q64.64 over the full unit range (128 bits on the wire, four groups), and the unsigned wide case.
    check_fixed_cases<112, 16, -1152921504606846976LL, +1152921504606846976LL>( serialize::int128_t( 65536 ) );     // ±2^60 units: 78 bits on the wire
    check_fixed_cases<112, 16, -2, +2>( serialize::int128_t( 65536 ) );
    check_fixed_cases<64, 64, -1000, +1000>( serialize::int128_t( 1 ) << 64 );
    check_fixed_cases<64, 64, INT64_MIN, INT64_MAX>( serialize::int128_t( 1 ) << 64 );                              // full unit range: 128 bits on the wire
    check_fixed_cases<112, 16, 0, 2305843009213693952LL>( serialize::uint128_t( 65536 ) );                          // 2^61 units, unsigned

    // the 33..64 bit two group band on wide storage: both boundaries exactly, plus the example's
    // own Q112.16 ±1e11 shape (54 bits), which used to live only in example.cpp and not in CI
    check_fixed_cases<112, 16, -32768, +32768>( serialize::int128_t( 65536 ) );                                     // 33 bits: the band's low edge
    check_fixed_cases<112, 16, -100000000000LL, +100000000000LL>( serialize::int128_t( 65536 ) );                   // 54 bits: the example's shape
    check_fixed_cases<112, 16, -140737488355328LL, +140737488355327LL>( serialize::int128_t( 65536 ) );             // 64 bits: the band's high edge

    // the wire cost is a compile time constant of the call site, wide paths included. pin a few
    {
        uint8_t buffer[16];
        serialize::WriteStream stream( buffer, 16 );
        serialize::int128_t value = serialize::int128_t( 12345 ) * 65536;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -1152921504606846976LL, +1152921504606846976LL>( stream, value ) ) == true );
        serialize_check( stream.GetBitsProcessed() == 78 );         // 2^61 << 16 raw values needs 78 bits
    }
    {
        uint8_t buffer[24];
        serialize::WriteStream stream( buffer, 24 );
        serialize::int128_t value = 0;
        serialize_check( ( serialize::serialize_fixed_internal<64, 64, INT64_MIN, INT64_MAX>( stream, value ) ) == true );
        serialize_check( stream.GetBitsProcessed() == 128 );        // the full unit range costs the full storage width
    }
    {
        uint8_t buffer[16];
        serialize::WriteStream stream( buffer, 16 );
        serialize::int128_t value = serialize::int128_t( 12345678901LL ) * serialize::int128_t( 65536 );
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -100000000000LL, +100000000000LL>( stream, value ) ) == true );
        serialize_check( stream.GetBitsProcessed() == 54 );         // the example's shape: 2e11 << 16 raw values needs 54 bits, inside the two group band
    }
    {
        uint8_t buffer[16];
        serialize::WriteStream stream( buffer, 16 );
        serialize::int128_t value = 0;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -32768, +32768>( stream, value ) ) == true );
        serialize_check( stream.GetBitsProcessed() == 33 );         // the band's low edge
    }
    {
        uint8_t buffer[16];
        serialize::WriteStream stream( buffer, 16 );
        serialize::int128_t value = 0;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -140737488355328LL, +140737488355327LL>( stream, value ) ) == true );
        serialize_check( stream.GetBitsProcessed() == 64 );         // the band's high edge
    }

    // one raw step past raw_max must be rejected on read, through every group structure —
    // the 33..64 bit two group band included
    check_fixed_wide_rejects_out_of_range<112, 16, -1152921504606846976LL, +1152921504606846976LL>( serialize::int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, -2, +2>( serialize::int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<64, 64, -1000, +1000>( serialize::int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, 0, 2305843009213693952LL>( serialize::uint128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, -32768, +32768>( serialize::int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, -100000000000LL, +100000000000LL>( serialize::int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, -140737488355328LL, +140737488355327LL>( serialize::int128_t( 0 ) );

    // reads past the end of the buffer must fail cleanly
    {
        uint8_t buffer[4 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::ReadStream readStream( buffer, 4 );
        serialize::int128_t value = 0;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -1152921504606846976LL, +1152921504606846976LL>( readStream, value ) ) == false );
    }
}

inline void test_serialize_fixed_wide_emulated()
{
    // the wide codec is representation generic: the emulated 128 bit pair works as storage, which
    // is what makes wide fixed point available on compilers without native __int128. run the wide
    // case list through the emulated types explicitly, on every platform — where native exists
    // this doubles as proof that both representations drive the same codec.
    check_fixed_cases<112, 16, -1152921504606846976LL, +1152921504606846976LL>( ::serialize_int128_t( 65536 ) );
    check_fixed_cases<112, 16, -2, +2>( ::serialize_int128_t( 65536 ) );
    check_fixed_cases<64, 64, -1000, +1000>( ::serialize_int128_t( 1 ) << 64 );
    check_fixed_cases<64, 64, INT64_MIN, INT64_MAX>( ::serialize_int128_t( 1 ) << 64 );
    check_fixed_cases<112, 16, 0, 2305843009213693952LL>( ::serialize_uint128_t( 65536 ) );

    // the 33..64 bit two group band on emulated wide storage: both boundaries and the example's shape
    check_fixed_cases<112, 16, -32768, +32768>( ::serialize_int128_t( 65536 ) );
    check_fixed_cases<112, 16, -100000000000LL, +100000000000LL>( ::serialize_int128_t( 65536 ) );
    check_fixed_cases<112, 16, -140737488355328LL, +140737488355327LL>( ::serialize_int128_t( 65536 ) );

    // one raw step past raw_max must be rejected through the emulated read path too
    check_fixed_wide_rejects_out_of_range<112, 16, -1152921504606846976LL, +1152921504606846976LL>( ::serialize_int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<64, 64, -1000, +1000>( ::serialize_int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, 0, 2305843009213693952LL>( ::serialize_uint128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, -32768, +32768>( ::serialize_int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, -100000000000LL, +100000000000LL>( ::serialize_int128_t( 0 ) );
    check_fixed_wide_rejects_out_of_range<112, 16, -140737488355328LL, +140737488355327LL>( ::serialize_int128_t( 0 ) );

#if defined(__SIZEOF_INT128__)
    // cross representation wire identity: native and emulated storage must produce byte identical
    // wire through the same configuration, and each must read the other's bytes back exactly
    {
        const int64_t raw = -( int64_t( 54321 ) * 65536 + 12345 );

        uint8_t native_buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data
        serialize::WriteStream nativeStream( native_buffer, 16 );
        serialize::int128_t native_value = raw;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -1152921504606846976LL, +1152921504606846976LL>( nativeStream, native_value ) ) == true );
        nativeStream.Flush();

        uint8_t emulated_buffer[16 + 8] = { 0 };
        serialize::WriteStream emulatedStream( emulated_buffer, 16 );
        ::serialize_int128_t emulated_value( raw );
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -1152921504606846976LL, +1152921504606846976LL>( emulatedStream, emulated_value ) ) == true );
        emulatedStream.Flush();

        serialize_check( nativeStream.GetBitsProcessed() == emulatedStream.GetBitsProcessed() );
        serialize_check( memcmp( native_buffer, emulated_buffer, 16 ) == 0 );

        // read the native bytes back through emulated storage
        serialize::ReadStream readStream( native_buffer, nativeStream.GetBytesProcessed() );
        ::serialize_int128_t read_back( 0 );
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -1152921504606846976LL, +1152921504606846976LL>( readStream, read_back ) ) == true );
        serialize_check( read_back == ::serialize_int128_t( raw ) );
    }
#endif // #if defined(__SIZEOF_INT128__)
}

inline void test_serialize_fixed_degenerate()
{
    // STANDARD.md: a degenerate range where min == max is LEGAL and costs ZERO
    // BITS on every storage width -- nothing is written, and the reader
    // recovers the value from the range alone: the raw value min << fraction_bits.
    //
    // The wide path is why this test exists (serialize#54): ports computed the
    // wide bit count as bits_required( min, max ) + fraction_bits, which
    // degenerates to fraction_bits ZEROS when min == max while their narrow
    // paths wrote nothing -- the same library disagreeing with itself across
    // the 64/128 storage boundary. This library refused to compile the case
    // outright (static assert min < max), stricter than the format. Both
    // paths must write zero bits, and this test pins each of them.

    // narrow storage: Q16.16
    {
        uint8_t buffer[16] = { 0 };

        serialize::WriteStream writeStream( buffer, 16 );
        int32_t degenerate = 5 * 65536;                              // 5.0 in Q16.16: the raw value IS min << 16
        int32_t after = 3;
        serialize_check( ( serialize::serialize_fixed_internal<16, 16, 5, 5>( writeStream, degenerate ) ) == true );
        serialize_check( writeStream.GetBitsProcessed() == 0 );      // nothing written
        serialize_check( writeStream.SerializeInteger( after, 0, 7 ) );
        serialize_check( writeStream.GetBitsProcessed() == 3 );      // the NEXT field starts at bit 0
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        int32_t read_degenerate = 0;
        int32_t read_after = 0;
        serialize_check( ( serialize::serialize_fixed_internal<16, 16, 5, 5>( readStream, read_degenerate ) ) == true );
        serialize_check( read_degenerate == 5 * 65536 );             // recovered from the range
        serialize_check( readStream.GetBitsProcessed() == 0 );
        serialize_check( readStream.SerializeInteger( read_after, 0, 7 ) );
        serialize_check( read_after == 3 );

        serialize::MeasureStream measureStream;
        int32_t measured = 5 * 65536;
        serialize_check( ( serialize::serialize_fixed_internal<16, 16, 5, 5>( measureStream, measured ) ) == true );
        serialize_check( measureStream.GetBitsProcessed() == 0 );
    }

    // narrow storage, negative degenerate bound: Q48.16 at -7.0 -- the raw min
    // is negative, and the reader must still recover it exactly
    {
        uint8_t buffer[16] = { 0 };

        serialize::WriteStream writeStream( buffer, 16 );
        int64_t degenerate = int64_t( -7 ) * 65536;
        int32_t after = 3;
        serialize_check( ( serialize::serialize_fixed_internal<48, 16, -7, -7>( writeStream, degenerate ) ) == true );
        serialize_check( writeStream.GetBitsProcessed() == 0 );
        serialize_check( writeStream.SerializeInteger( after, 0, 7 ) );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        int64_t read_degenerate = 0;
        int32_t read_after = 0;
        serialize_check( ( serialize::serialize_fixed_internal<48, 16, -7, -7>( readStream, read_degenerate ) ) == true );
        serialize_check( read_degenerate == int64_t( -7 ) * 65536 );
        serialize_check( readStream.SerializeInteger( read_after, 0, 7 ) );
        serialize_check( read_after == 3 );
    }

    // wide storage: Q112.16 -- the path that used to cost fraction_bits zeros
    // in the ports. zero bits here, exactly like the narrow path
    {
        uint8_t buffer[16] = { 0 };

        serialize::WriteStream writeStream( buffer, 16 );
        serialize::int128_t degenerate = serialize::int128_t( 9 * 65536 );  // 9.0 in Q112.16
        int32_t after = 3;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, 9, 9>( writeStream, degenerate ) ) == true );
        serialize_check( writeStream.GetBitsProcessed() == 0 );      // zero bits, NOT fraction_bits
        serialize_check( writeStream.SerializeInteger( after, 0, 7 ) );
        serialize_check( writeStream.GetBitsProcessed() == 3 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize::int128_t read_degenerate( 0 );
        int32_t read_after = 0;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, 9, 9>( readStream, read_degenerate ) ) == true );
        serialize_check( read_degenerate == serialize::int128_t( 9 * 65536 ) );
        serialize_check( readStream.GetBitsProcessed() == 0 );
        serialize_check( readStream.SerializeInteger( read_after, 0, 7 ) );
        serialize_check( read_after == 3 );

        serialize::MeasureStream measureStream;
        serialize::int128_t measured = serialize::int128_t( 9 * 65536 );
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, 9, 9>( measureStream, measured ) ) == true );
        serialize_check( measureStream.GetBitsProcessed() == 0 );
    }

    // wide storage, negative degenerate bound: Q112.16 at -9.0
    {
        uint8_t buffer[16] = { 0 };

        serialize::WriteStream writeStream( buffer, 16 );
        serialize::int128_t degenerate = serialize::int128_t( -9 * 65536 );
        int32_t after = 3;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -9, -9>( writeStream, degenerate ) ) == true );
        serialize_check( writeStream.GetBitsProcessed() == 0 );
        serialize_check( writeStream.SerializeInteger( after, 0, 7 ) );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        serialize::int128_t read_degenerate( 0 );
        int32_t read_after = 0;
        serialize_check( ( serialize::serialize_fixed_internal<112, 16, -9, -9>( readStream, read_degenerate ) ) == true );
        serialize_check( read_degenerate == serialize::int128_t( -9 * 65536 ) );
        serialize_check( readStream.SerializeInteger( read_after, 0, 7 ) );
        serialize_check( read_after == 3 );
    }

    // wide storage, emulated representation: Q64.64 over min == max == 0. the
    // old wide formula would have made this 64 bits of zeros -- the fraction
    // alone spans the full 64 bit fractional field. explicitly the emulated
    // pair, so the degenerate path is proven in both representations on every
    // platform
    {
        uint8_t buffer[16] = { 0 };

        serialize::WriteStream writeStream( buffer, 16 );
        ::serialize_int128_t degenerate( 0 );
        int32_t after = 3;
        serialize_check( ( serialize::serialize_fixed_internal<64, 64, 0, 0>( writeStream, degenerate ) ) == true );
        serialize_check( writeStream.GetBitsProcessed() == 0 );      // zero bits, not the 64 bit fractional field
        serialize_check( writeStream.SerializeInteger( after, 0, 7 ) );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
        ::serialize_int128_t read_degenerate( 1 );                   // a wrong value, so recovery is observable
        int32_t read_after = 0;
        serialize_check( ( serialize::serialize_fixed_internal<64, 64, 0, 0>( readStream, read_degenerate ) ) == true );
        serialize_check( read_degenerate == ::serialize_int128_t( 0 ) );
        serialize_check( readStream.SerializeInteger( read_after, 0, 7 ) );
        serialize_check( read_after == 3 );
    }
}

inline void test_serialize_uint128()
{
    // round trips across the value patterns: zero, max, each half alone, alternating bits, distinct halves
    {
        const serialize::uint128_t values[] = {
            0,
            ~( serialize::uint128_t( 0 ) ),
            serialize::uint128_t( 0xFFFFFFFFFFFFFFFFULL ) << 64,                                // high half only
            serialize::uint128_t( 0xFFFFFFFFFFFFFFFFULL ),                                      // low half only
            ( serialize::uint128_t( 0xAAAAAAAAAAAAAAAAULL ) << 64 ) | 0x5555555555555555ULL,    // alternating bits
            ( serialize::uint128_t( 0x0123456789ABCDEFULL ) << 64 ) | 0xFEDCBA9876543210ULL,    // distinct halves
        };

        for ( int i = 0; i < (int) ( sizeof( values ) / sizeof( values[0] ) ); i++ )
        {
            uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

            serialize::WriteStream writeStream( buffer, 16 );
            serialize::uint128_t written = values[i];
            serialize_check( serialize::serialize_uint128_internal( writeStream, written ) == true );
            writeStream.Flush();

            serialize::MeasureStream measureStream;
            serialize::uint128_t measured = values[i];
            serialize_check( serialize::serialize_uint128_internal( measureStream, measured ) == true );
            serialize_check( measureStream.GetBitsProcessed() == writeStream.GetBitsProcessed() );
            serialize_check( writeStream.GetBitsProcessed() == 128 );

            serialize::ReadStream readStream( buffer, writeStream.GetBytesProcessed() );
            serialize::uint128_t read_back = 0;
            serialize_check( serialize::serialize_uint128_internal( readStream, read_back ) == true );
            serialize_check( read_back == values[i] );
        }
    }

    // cross form consistency: serialize_uint128 must be byte identical to two serialize_uint64
    // operations on the halves, low half first. this is the portability story: an implementation
    // without a 128 bit type reproduces the wire exactly with two 64 bit operations.
    {
        const uint64_t low_half = 0xFEDCBA9876543210ULL;
        const uint64_t high_half = 0x0123456789ABCDEFULL;

        uint8_t uint128_buffer[16 + 8] = { 0 };
        serialize::WriteStream uint128Stream( uint128_buffer, 16 );
        serialize::uint128_t value = ( serialize::uint128_t( high_half ) << 64 ) | low_half;
        serialize_check( serialize::serialize_uint128_internal( uint128Stream, value ) == true );
        uint128Stream.Flush();

        uint8_t halves_buffer[16 + 8] = { 0 };
        serialize::WriteStream halvesStream( halves_buffer, 16 );
        write_uint64( halvesStream, low_half );
        write_uint64( halvesStream, high_half );
        halvesStream.Flush();

        serialize_check( uint128Stream.GetBitsProcessed() == halvesStream.GetBitsProcessed() );
        serialize_check( memcmp( uint128_buffer, halves_buffer, 16 ) == 0 );
    }

    // golden pin: the wire format for a uint128 is its 16 bytes in little endian order, low half
    // first. pinned forever. additive: the pre-existing golden test below is untouched, and this
    // pin runs on every platform — serialize::uint128_t exists everywhere, so the native and
    // emulated representations must both reproduce exactly these bytes.
    {
        static const uint8_t golden_uint128_bytes[] =
        {
            0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
            0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01
        };

        const serialize::uint128_t golden_value = ( serialize::uint128_t( 0x0123456789ABCDEFULL ) << 64 ) | 0xFEDCBA9876543210ULL;

        uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 16 );
        serialize::uint128_t written = golden_value;
        serialize_check( serialize::serialize_uint128_internal( writeStream, written ) == true );
        writeStream.Flush();
        serialize_check( writeStream.GetBytesProcessed() == 16 );
        serialize_check( memcmp( buffer, golden_uint128_bytes, 16 ) == 0 );

        memcpy( buffer, golden_uint128_bytes, 16 );
        serialize::ReadStream readStream( buffer, 16 );
        serialize::uint128_t read_back = 0;
        serialize_check( serialize::serialize_uint128_internal( readStream, read_back ) == true );
        serialize_check( read_back == golden_value );
    }
}

inline void test_serialize_int128()
{
    typedef serialize::int128_t i128;

    // 1. WIRE IDENTITY WITH serialize_int64 wherever the range fits 64 bits. this is what lets a
    //    schema widen a field without a wire change, so it is pinned by memcmp rather than assumed.
    {
        const int64_t min64 = -5000000000LL;
        const int64_t max64 = +5000000000LL;
        const int64_t values[] = { min64, min64 + 1, -1, 0, +1, 4123456789LL, max64 - 1, max64 };

        for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
        {
            uint8_t buffer128[32 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data
            uint8_t buffer64[32 + 8] = { 0 };

            serialize::WriteStream w128( buffer128, 32 );
            serialize_check( w128.SerializeInteger128( i128( values[i] ), i128( min64 ), i128( max64 ) ) == true );
            w128.Flush();

            serialize::WriteStream w64( buffer64, 32 );
            serialize_check( w64.SerializeInteger64( values[i], min64, max64 ) == true );
            w64.Flush();

            serialize_check( w128.GetBitsProcessed() == w64.GetBitsProcessed() );
            serialize_check( w128.GetBytesProcessed() == w64.GetBytesProcessed() );
            serialize_check( memcmp( buffer128, buffer64, (size_t) w64.GetBytesProcessed() ) == 0 );

            serialize::ReadStream readStream( buffer128, 32 );
            i128 read_back = 0;
            serialize_check( readStream.SerializeInteger128( read_back, i128( min64 ), i128( max64 ) ) == true );
            serialize_check( read_back == i128( values[i] ) );
        }
    }

    // 2. the wide bands the 64 bit path cannot express at all: three group and four group ranges,
    //    including the widest possible range, which exercises the fourth group and the unsigned
    //    domain subtraction that a signed one would overflow
    {
        const i128 wide_min = -( i128( 1 ) << 100 );
        const i128 wide_max =  ( i128( 1 ) << 100 );
        const i128 values[] = { wide_min, wide_min + 1, i128( -1 ), i128( 0 ), i128( 1 ), ( i128( 1 ) << 99 ), wide_max - 1, wide_max };

        for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
        {
            uint8_t buffer[32 + 8] = { 0 };

            serialize::WriteStream writeStream( buffer, 32 );
            serialize_check( writeStream.SerializeInteger128( values[i], wide_min, wide_max ) == true );
            writeStream.Flush();
            serialize_check( writeStream.GetBitsProcessed() == 102 );        // bits_required128( -2^100, 2^100 ) == 102

            serialize::ReadStream readStream( buffer, 32 );
            i128 read_back = 0;
            serialize_check( readStream.SerializeInteger128( read_back, wide_min, wide_max ) == true );
            serialize_check( read_back == values[i] );
        }
    }

    // 3. the full 128 bit range: every group full, and the range is wider than 2^127
    {
        const i128 full_min = i128( ( serialize::uint128_t( 1 ) << 127 ) );                  // INT128_MIN
        const i128 full_max = i128( ~( serialize::uint128_t( 1 ) << 127 ) );                 // INT128_MAX
        const i128 values[] = { full_min, full_min + 1, i128( -1 ), i128( 0 ), i128( 1 ), full_max - 1, full_max };

        for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
        {
            uint8_t buffer[32 + 8] = { 0 };

            serialize::WriteStream writeStream( buffer, 32 );
            serialize_check( writeStream.SerializeInteger128( values[i], full_min, full_max ) == true );
            writeStream.Flush();
            serialize_check( writeStream.GetBitsProcessed() == 128 );

            serialize::ReadStream readStream( buffer, 32 );
            i128 read_back = 0;
            serialize_check( readStream.SerializeInteger128( read_back, full_min, full_max ) == true );
            serialize_check( read_back == values[i] );
        }
    }

    // 4. the measure stream must agree with the write stream exactly, at every group width
    {
        const i128 cases[][3] =
        {
            { i128( 0 ), i128( 0 ), i128( 255 ) },
            { i128( 7 ), i128( -5000000000LL ), i128( +5000000000LL ) },
            { i128( 1 ), -( i128( 1 ) << 100 ), ( i128( 1 ) << 100 ) },
            { i128( 0 ), i128( ( serialize::uint128_t( 1 ) << 127 ) ), i128( ~( serialize::uint128_t( 1 ) << 127 ) ) },
        };

        for ( int i = 0; i < (int) ( sizeof(cases) / sizeof(cases[0]) ); i++ )
        {
            uint8_t buffer[32 + 8] = { 0 };

            serialize::WriteStream writeStream( buffer, 32 );
            serialize_check( writeStream.SerializeInteger128( cases[i][0], cases[i][1], cases[i][2] ) == true );
            writeStream.Flush();

            serialize::MeasureStream measureStream;
            serialize_check( measureStream.SerializeInteger128( cases[i][0], cases[i][1], cases[i][2] ) == true );
            serialize_check( measureStream.GetBitsProcessed() == writeStream.GetBitsProcessed() );
        }
    }

    // 5. a value outside the bounds must be REFUSED on read. the bit count is identical for both
    //    bound pairs here, so the reader consumes the same bits and the range check is what
    //    convicts it — proving the refusal, not just the absence of a crash
    {
        uint8_t buffer[32 + 8] = { 0 };

        serialize::WriteStream writeStream( buffer, 32 );
        serialize_check( writeStream.SerializeInteger128( i128( 255 ), i128( 0 ), i128( 255 ) ) == true );
        writeStream.Flush();

        serialize_check( serialize::bits_required128( serialize::uint128_t( 0 ), serialize::uint128_t( 200 ) ) == 8 );

        serialize::ReadStream readStream( buffer, 32 );
        i128 read_back = 0;
        serialize_check( readStream.SerializeInteger128( read_back, i128( 0 ), i128( 200 ) ) == false );
    }

    // 6. a truncated buffer must be refused rather than read past the end
    {
        uint8_t buffer[32 + 8] = { 0 };

        serialize::ReadStream readStream( buffer, 4 );          // 32 bits available, 128 required
        i128 read_back = 0;
        serialize_check( readStream.SerializeInteger128( read_back, i128( ( serialize::uint128_t( 1 ) << 127 ) ), i128( ~( serialize::uint128_t( 1 ) << 127 ) ) ) == false );
    }

    // 7. THE GOLDEN PIN, and its bytes were derived from STANDARD.md's stated rule in the
    //    conformance checker's language, not read back out of this implementation — so this
    //    memcmp is the document and the code agreeing, rather than the code agreeing with itself.
    //    Bounds of +/- 2^70 need 72 bits, which is the THREE GROUP structure: 32, 32, then 8.
    {
        static const uint8_t golden_int128_bytes[] =
        {
            0x11, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
            0x3F, 0x00, 0x00, 0x00
        };

        const i128 golden_min = -( i128( 1 ) << 70 );
        const i128 golden_max =  ( i128( 1 ) << 70 );
        const i128 golden_value = -( i128( 0x0123456789ABCDEFLL ) );

        uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 16 );
        serialize_check( writeStream.SerializeInteger128( golden_value, golden_min, golden_max ) == true );
        writeStream.Flush();
        serialize_check( writeStream.GetBitsProcessed() == 72 );
        serialize_check( memcmp( buffer, golden_int128_bytes, sizeof( golden_int128_bytes ) ) == 0 );

        memcpy( buffer, golden_int128_bytes, sizeof( golden_int128_bytes ) );
        serialize::ReadStream readStream( buffer, 16 );
        i128 read_back = 0;
        serialize_check( readStream.SerializeInteger128( read_back, golden_min, golden_max ) == true );
        serialize_check( read_back == golden_value );
    }
}

// builds an emulated 128 bit value from its two lanes. the emulated type is defined on every
// platform when tests are enabled — where native __int128 exists it coexists with it, so the
// tests below run everywhere and the differential test can compare the two representations.

inline ::serialize_uint128_t uint128_emulated( uint64_t hi, uint64_t lo )
{
    ::serialize_uint128_t result( lo );
    result.hi = hi;
    return result;
}

inline void test_uint128_emulation()
{
    // known answer checks for the emulated 128 bit unsigned integer, unguarded so they run on
    // every platform — including the ones with no native __int128, where the emulation is the
    // only implementation and the differential test below cannot run.

    const uint64_t lo = 0xFEDCBA9876543210ULL;
    const uint64_t hi = 0x0123456789ABCDEFULL;

    // construction from uint64_t fills the low lane. explicit conversion truncates back to it
    {
        ::serialize_uint128_t value( lo );
        serialize_check( value.lo == lo && value.hi == 0 );
        serialize_check( uint64_t( value ) == lo );
        serialize_check( uint64_t( uint128_emulated( hi, lo ) ) == lo );
    }

    // construction from every standard integer type mirrors native conversion: signed sources
    // sign extend (negatives wrap modulo 2^128), unsigned sources zero extend. these are the
    // exact cross signed cases that diverged before the per-type constructors existed.
    {
        serialize_check( ::serialize_uint128_t( int64_t( -1 ) ) == uint128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL ) );
        serialize_check( ::serialize_uint128_t( INT64_MIN ) == uint128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL ) );
        serialize_check( ::serialize_uint128_t( uint64_t( 1 ) << 63 ) == uint128_emulated( 0, 0x8000000000000000ULL ) );
        serialize_check( ::serialize_uint128_t( UINT64_MAX ) == uint128_emulated( 0, 0xFFFFFFFFFFFFFFFFULL ) );
        serialize_check( ::serialize_uint128_t( -5 ) == uint128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFBULL ) );       // int: sign extends
        serialize_check( ::serialize_uint128_t( 3000000000U ) == uint128_emulated( 0, 3000000000ULL ) );                          // unsigned: zero extends
        serialize_check( ::serialize_uint128_t( int64_t( 7 ) ) == uint128_emulated( 0, 7 ) );                                     // non negative signed: zero high lane
    }

    // all six comparisons, driven by the high lane, the low lane, and equality
    {
        const ::serialize_uint128_t a = uint128_emulated( 1, 0 );
        const ::serialize_uint128_t b = uint128_emulated( 0, 0xFFFFFFFFFFFFFFFFULL );
        const ::serialize_uint128_t c = uint128_emulated( 1, 1 );
        serialize_check( a > b );                                       // the high lane dominates
        serialize_check( b < a );
        serialize_check( c > a );                                       // the low lane breaks ties
        serialize_check( a >= b && a <= a && a >= a );
        serialize_check( b <= a && b != a && a == a );
        serialize_check( !( a < b ) && !( a == b ) && !( a != a ) );
    }

    // left shift edges: 0, 1, 63, 64, 65, 127, and out of range counts
    {
        const ::serialize_uint128_t value = uint128_emulated( hi, lo );
        serialize_check( ( value << 0 ) == value );
        serialize_check( ( value << 1 ) == uint128_emulated( ( hi << 1 ) | ( lo >> 63 ), lo << 1 ) );
        serialize_check( ( value << 63 ) == uint128_emulated( ( hi << 63 ) | ( lo >> 1 ), lo << 63 ) );
        serialize_check( ( value << 64 ) == uint128_emulated( lo, 0 ) );                    // the low lane becomes the high lane
        serialize_check( ( value << 65 ) == uint128_emulated( lo << 1, 0 ) );
        serialize_check( ( value << 127 ) == uint128_emulated( lo << 63, 0 ) );
        serialize_check( ( value << 128 ) == ::serialize_uint128_t( 0 ) );                        // >= 128 yields zero, documented
        serialize_check( ( value << 200 ) == ::serialize_uint128_t( 0 ) );
    }

    // right shift edges: the mirror image
    {
        const ::serialize_uint128_t value = uint128_emulated( hi, lo );
        serialize_check( ( value >> 0 ) == value );
        serialize_check( ( value >> 1 ) == uint128_emulated( hi >> 1, ( lo >> 1 ) | ( hi << 63 ) ) );
        serialize_check( ( value >> 63 ) == uint128_emulated( hi >> 63, ( lo >> 63 ) | ( hi << 1 ) ) );
        serialize_check( ( value >> 64 ) == uint128_emulated( 0, hi ) );                    // the high lane becomes the low lane
        serialize_check( ( value >> 65 ) == uint128_emulated( 0, hi >> 1 ) );
        serialize_check( ( value >> 127 ) == uint128_emulated( 0, hi >> 63 ) );
        serialize_check( ( value >> 128 ) == ::serialize_uint128_t( 0 ) );                        // >= 128 yields zero, documented
        serialize_check( ( value >> 200 ) == ::serialize_uint128_t( 0 ) );
    }

    // addition carries out of the low lane, subtraction borrows back into it
    {
        const ::serialize_uint128_t max64( 0xFFFFFFFFFFFFFFFFULL );
        const ::serialize_uint128_t all_ones = uint128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL );
        serialize_check( max64 + ::serialize_uint128_t( 1 ) == uint128_emulated( 1, 0 ) );        // carry
        serialize_check( all_ones + ::serialize_uint128_t( 1 ) == ::serialize_uint128_t( 0 ) );         // wraps modulo 2^128
        serialize_check( ::serialize_uint128_t( 0 ) - ::serialize_uint128_t( 1 ) == all_ones );         // borrow wraps back
        serialize_check( uint128_emulated( 1, 0 ) - ::serialize_uint128_t( 1 ) == max64 );        // borrow out of the high lane

        ::serialize_uint128_t accumulator( 0xFFFFFFFFFFFFFFFFULL );
        accumulator += ::serialize_uint128_t( 1 );
        serialize_check( accumulator == uint128_emulated( 1, 0 ) );
        accumulator -= ::serialize_uint128_t( 1 );
        serialize_check( accumulator == max64 );
    }

    // increment and decrement, both forms, across the lane boundary
    {
        ::serialize_uint128_t value( 0xFFFFFFFFFFFFFFFFULL );
        serialize_check( value++ == ::serialize_uint128_t( 0xFFFFFFFFFFFFFFFFULL ) );             // post returns the value before
        serialize_check( value == uint128_emulated( 1, 0 ) );
        serialize_check( ++value == uint128_emulated( 1, 1 ) );                             // pre returns the value after
        serialize_check( value-- == uint128_emulated( 1, 1 ) );
        serialize_check( --value == ::serialize_uint128_t( 0xFFFFFFFFFFFFFFFFULL ) );
    }

    // multiplication: lane crossing products with known answers
    {
        const ::serialize_uint128_t two_to_32( uint64_t( 1 ) << 32 );
        const ::serialize_uint128_t max64( 0xFFFFFFFFFFFFFFFFULL );
        const ::serialize_uint128_t all_ones = uint128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL );
        serialize_check( ::serialize_uint128_t( 3 ) * ::serialize_uint128_t( 5 ) == ::serialize_uint128_t( 15 ) );
        serialize_check( two_to_32 * two_to_32 == uint128_emulated( 1, 0 ) );                                       // 2^64 crosses into the high lane
        serialize_check( max64 * max64 == uint128_emulated( 0xFFFFFFFFFFFFFFFEULL, 1 ) );                           // 2^128 - 2^65 + 1
        serialize_check( uint128_emulated( 1, 0 ) * ::serialize_uint128_t( 5 ) == uint128_emulated( 5, 0 ) );             // the high lane path
        serialize_check( all_ones * all_ones == ::serialize_uint128_t( 1 ) );                                             // ( 2^128 - 1 )^2 mod 2^128

        ::serialize_uint128_t accumulator( 3 );
        accumulator *= ::serialize_uint128_t( 5 );
        serialize_check( accumulator == ::serialize_uint128_t( 15 ) );
    }

    // division and modulo: the small fast path, wide dividends, wide divisors, and the
    // TOTALITY of division by zero — the operation is undefined and these pin only that it
    // returns rather than traps, never that the value is a contract
    {
        const ::serialize_uint128_t all_ones = uint128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL );
        serialize_check( ::serialize_uint128_t( 100 ) / ::serialize_uint128_t( 7 ) == ::serialize_uint128_t( 14 ) );
        serialize_check( ::serialize_uint128_t( 100 ) % ::serialize_uint128_t( 7 ) == ::serialize_uint128_t( 2 ) );
        serialize_check( uint128_emulated( 1, 7 ) / ::serialize_uint128_t( 16 ) == ::serialize_uint128_t( uint64_t( 1 ) << 60 ) );      // 2^64 + 7 over 16
        serialize_check( uint128_emulated( 1, 7 ) % ::serialize_uint128_t( 16 ) == ::serialize_uint128_t( 7 ) );
        serialize_check( uint128_emulated( uint64_t( 1 ) << 63, 0 ) / uint128_emulated( 1, 0 ) == ::serialize_uint128_t( uint64_t( 1 ) << 63 ) );     // 2^127 over 2^64
        serialize_check( all_ones / ::serialize_uint128_t( 3 ) == uint128_emulated( 0x5555555555555555ULL, 0x5555555555555555ULL ) );
        serialize_check( all_ones % ::serialize_uint128_t( 3 ) == ::serialize_uint128_t( 0 ) );
        serialize_check( ::serialize_uint128_t( 7 ) / uint128_emulated( 1, 0 ) == ::serialize_uint128_t( 0 ) );                 // dividend below divisor
        serialize_check( ::serialize_uint128_t( 7 ) % uint128_emulated( 1, 0 ) == ::serialize_uint128_t( 7 ) );
        serialize_check( all_ones / ::serialize_uint128_t( 0 ) == ::serialize_uint128_t( 0 ) );                                 // by zero is UNDEFINED: this pins totality, not a value contract
        serialize_check( all_ones % ::serialize_uint128_t( 0 ) == ::serialize_uint128_t( 0 ) );

        ::serialize_uint128_t accumulator( 100 );
        accumulator /= ::serialize_uint128_t( 7 );
        serialize_check( accumulator == ::serialize_uint128_t( 14 ) );
        accumulator = ::serialize_uint128_t( 100 );
        accumulator %= ::serialize_uint128_t( 7 );
        serialize_check( accumulator == ::serialize_uint128_t( 2 ) );
    }

    // bitwise operators work lane by lane
    {
        const ::serialize_uint128_t a = uint128_emulated( 0xF0F0F0F0F0F0F0F0ULL, 0xAAAAAAAAAAAAAAAAULL );
        const ::serialize_uint128_t b = uint128_emulated( 0xFF00FF00FF00FF00ULL, 0xCCCCCCCCCCCCCCCCULL );
        serialize_check( ( a & b ) == uint128_emulated( 0xF000F000F000F000ULL, 0x8888888888888888ULL ) );
        serialize_check( ( a | b ) == uint128_emulated( 0xFFF0FFF0FFF0FFF0ULL, 0xEEEEEEEEEEEEEEEEULL ) );
        serialize_check( ( a ^ b ) == uint128_emulated( 0x0FF00FF00FF00FF0ULL, 0x6666666666666666ULL ) );
        serialize_check( ~a == uint128_emulated( 0x0F0F0F0F0F0F0F0FULL, 0x5555555555555555ULL ) );

        ::serialize_uint128_t accumulator = a;
        accumulator &= b;
        serialize_check( accumulator == ( a & b ) );
        accumulator = a;
        accumulator |= b;
        serialize_check( accumulator == ( a | b ) );
        accumulator = a;
        accumulator ^= b;
        serialize_check( accumulator == ( a ^ b ) );
        accumulator = a;
        accumulator <<= 64;
        serialize_check( accumulator == ( a << 64 ) );
        accumulator = a;
        accumulator >>= 64;
        serialize_check( accumulator == ( a >> 64 ) );
    }

    // unary plus is the identity, unary minus is two's complement negation
    {
        const ::serialize_uint128_t value = uint128_emulated( hi, lo );
        serialize_check( +value == value );
        serialize_check( -::serialize_uint128_t( 0 ) == ::serialize_uint128_t( 0 ) );
        serialize_check( -::serialize_uint128_t( 1 ) == uint128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL ) );
        serialize_check( -value + value == ::serialize_uint128_t( 0 ) );
    }
}

inline ::serialize_int128_t int128_emulated( uint64_t hi, uint64_t lo )
{
    ::serialize_int128_t result( 0 );
    result.hi = hi;
    result.lo = lo;
    return result;
}

inline void test_int128_emulation()
{
    // known answer checks for the emulated signed 128 bit integer, unguarded so they run on every
    // platform. the signed type is a thin two's complement layer over the unsigned lanes, so these
    // concentrate on the signed specific pieces: sign extension, signed ordering, the arithmetic
    // right shift, division sign quadrants, and the documented edge choices.

    const ::serialize_int128_t int128_min = int128_emulated( 0x8000000000000000ULL, 0 );
    const ::serialize_int128_t int128_max = int128_emulated( 0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL );

    // the int64_t constructor sign extends into the high lane; the explicit conversion truncates
    // back to the low lane; the unsigned conversions preserve the bit pattern both ways
    {
        serialize_check( ::serialize_int128_t( -1 ) == int128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL ) );
        serialize_check( ::serialize_int128_t( INT64_MIN ) == int128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL ) );
        serialize_check( ::serialize_int128_t( INT64_MAX ) == int128_emulated( 0, 0x7FFFFFFFFFFFFFFFULL ) );
        serialize_check( ::serialize_int128_t( 1 ) == int128_emulated( 0, 1 ) );
        serialize_check( int64_t( ::serialize_int128_t( -5 ) ) == -5 );
        serialize_check( int64_t( ::serialize_int128_t( INT64_MIN ) ) == INT64_MIN );

        // unsigned sources are value preserving with a zero high lane, exactly like native: a
        // uint64 with the top bit set stays a large positive value, it does NOT wrap negative.
        // these are the exact cross signed cases that diverged before the per-type constructors.
        serialize_check( ::serialize_int128_t( uint64_t( 1 ) << 63 ) == int128_emulated( 0, 0x8000000000000000ULL ) );
        serialize_check( ::serialize_int128_t( uint64_t( 1 ) << 63 ) > ::serialize_int128_t( 0 ) );
        serialize_check( ::serialize_int128_t( UINT64_MAX ) == int128_emulated( 0, 0xFFFFFFFFFFFFFFFFULL ) );
        serialize_check( ::serialize_int128_t( UINT64_MAX ) > ::serialize_int128_t( 0 ) );
        serialize_check( ::serialize_int128_t( -5 ) == int128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFBULL ) );     // int: sign extends
        serialize_check( ::serialize_int128_t( 3000000000U ) == int128_emulated( 0, 3000000000ULL ) );                        // unsigned: zero extends

        const ::serialize_uint128_t bit_pattern = ::serialize_uint128_t( ::serialize_int128_t( -1 ) );
        serialize_check( bit_pattern.lo == 0xFFFFFFFFFFFFFFFFULL && bit_pattern.hi == 0xFFFFFFFFFFFFFFFFULL );
        serialize_check( ::serialize_int128_t( bit_pattern ) == ::serialize_int128_t( -1 ) );
    }

    // signed ordering: negatives below positives, the high lane compares signed, the low lane
    // breaks ties unsigned
    {
        serialize_check( ::serialize_int128_t( -1 ) < ::serialize_int128_t( 0 ) );
        serialize_check( ::serialize_int128_t( 0 ) < ::serialize_int128_t( 1 ) );
        serialize_check( int128_min < ::serialize_int128_t( INT64_MIN ) );
        serialize_check( int128_min < ::serialize_int128_t( -1 ) );
        serialize_check( int128_max > ::serialize_int128_t( INT64_MAX ) );
        serialize_check( int128_min < int128_max );
        serialize_check( int128_emulated( 0xFFFFFFFFFFFFFFFFULL, 5 ) < int128_emulated( 0xFFFFFFFFFFFFFFFFULL, 7 ) );       // low lane tiebreak among negatives
        serialize_check( ::serialize_int128_t( -3 ) <= ::serialize_int128_t( -3 ) && ::serialize_int128_t( -3 ) >= ::serialize_int128_t( -3 ) );
        serialize_check( ::serialize_int128_t( -3 ) != ::serialize_int128_t( 3 ) );
        serialize_check( !( ::serialize_int128_t( 1 ) < ::serialize_int128_t( -1 ) ) );
    }

    // addition, subtraction and multiplication with mixed signs, and the documented wrap
    {
        serialize_check( ::serialize_int128_t( -3 ) + ::serialize_int128_t( 5 ) == ::serialize_int128_t( 2 ) );
        serialize_check( ::serialize_int128_t( 3 ) - ::serialize_int128_t( 5 ) == ::serialize_int128_t( -2 ) );
        serialize_check( ::serialize_int128_t( -3 ) * ::serialize_int128_t( 5 ) == ::serialize_int128_t( -15 ) );
        serialize_check( ::serialize_int128_t( -3 ) * ::serialize_int128_t( -5 ) == ::serialize_int128_t( 15 ) );
        serialize_check( int128_max + ::serialize_int128_t( 1 ) == int128_min );                          // wraps two's complement, documented
        serialize_check( int128_min - ::serialize_int128_t( 1 ) == int128_max );
        serialize_check( ::serialize_int128_t( INT64_MAX ) * ::serialize_int128_t( 4 ) == int128_emulated( 1, 0xFFFFFFFFFFFFFFFCULL ) );        // crosses the lane boundary
    }

    // the arithmetic right shift fills with the sign: edges 0, 1, 63, 64, 65, 127 and out of range
    {
        serialize_check( ( int128_min >> 0 ) == int128_min );
        serialize_check( ( int128_min >> 1 ) == int128_emulated( 0xC000000000000000ULL, 0 ) );
        serialize_check( ( int128_min >> 63 ) == int128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0 ) );                             // -2^64
        serialize_check( ( int128_min >> 64 ) == ::serialize_int128_t( INT64_MIN ) );
        serialize_check( ( int128_min >> 65 ) == ::serialize_int128_t( INT64_MIN / 2 ) );
        serialize_check( ( int128_min >> 127 ) == ::serialize_int128_t( -1 ) );
        serialize_check( ( int128_min >> 128 ) == ::serialize_int128_t( -1 ) );                           // all sign bits, documented
        serialize_check( ( int128_min >> 200 ) == ::serialize_int128_t( -1 ) );
        serialize_check( ( ::serialize_int128_t( -7 ) >> 1 ) == ::serialize_int128_t( -4 ) );                   // arithmetic shift rounds toward negative infinity
        serialize_check( ( int128_max >> 127 ) == ::serialize_int128_t( 0 ) );
        serialize_check( ( int128_max >> 128 ) == ::serialize_int128_t( 0 ) );                            // non negative: sign fill is zero
        serialize_check( ( ( ::serialize_int128_t( 1 ) << 100 ) >> 100 ) == ::serialize_int128_t( 1 ) );
    }

    // the left shift moves the bit pattern like hardware: negative values included
    {
        serialize_check( ( ::serialize_int128_t( -1 ) << 1 ) == ::serialize_int128_t( -2 ) );
        serialize_check( ( ::serialize_int128_t( -1 ) << 64 ) == int128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0 ) );
        serialize_check( ( ::serialize_int128_t( 1 ) << 127 ) == int128_min );
        serialize_check( ( ::serialize_int128_t( -1 ) << 128 ) == ::serialize_int128_t( 0 ) );                  // out of range yields zero, documented
    }

    // division and modulo: all four sign quadrants, truncation toward zero, the remainder sign
    // follows the dividend, and the documented edge choices
    {
        serialize_check( ::serialize_int128_t( 7 ) / ::serialize_int128_t( 3 ) == ::serialize_int128_t( 2 ) );
        serialize_check( ::serialize_int128_t( 7 ) % ::serialize_int128_t( 3 ) == ::serialize_int128_t( 1 ) );
        serialize_check( ::serialize_int128_t( -7 ) / ::serialize_int128_t( 3 ) == ::serialize_int128_t( -2 ) );
        serialize_check( ::serialize_int128_t( -7 ) % ::serialize_int128_t( 3 ) == ::serialize_int128_t( -1 ) );
        serialize_check( ::serialize_int128_t( 7 ) / ::serialize_int128_t( -3 ) == ::serialize_int128_t( -2 ) );
        serialize_check( ::serialize_int128_t( 7 ) % ::serialize_int128_t( -3 ) == ::serialize_int128_t( 1 ) );
        serialize_check( ::serialize_int128_t( -7 ) / ::serialize_int128_t( -3 ) == ::serialize_int128_t( 2 ) );
        serialize_check( ::serialize_int128_t( -7 ) % ::serialize_int128_t( -3 ) == ::serialize_int128_t( -1 ) );
        serialize_check( ::serialize_int128_t( -1 ) / ::serialize_int128_t( 2 ) == ::serialize_int128_t( 0 ) );       // truncation toward zero, not the floor
        serialize_check( ::serialize_int128_t( -1 ) % ::serialize_int128_t( 2 ) == ::serialize_int128_t( -1 ) );
        serialize_check( int128_min / ::serialize_int128_t( 2 ) == int128_emulated( 0xC000000000000000ULL, 0 ) );
        serialize_check( int128_min / ::serialize_int128_t( 1 ) == int128_min );
        serialize_check( int128_min / ::serialize_int128_t( -1 ) == int128_min );                         // the one overflowing case wraps, documented
        serialize_check( int128_min % ::serialize_int128_t( -1 ) == ::serialize_int128_t( 0 ) );
        serialize_check( int128_max / ::serialize_int128_t( 0 ) == ::serialize_int128_t( 0 ) );                 // by zero is UNDEFINED: this pins totality, not a value contract
        serialize_check( int128_max % ::serialize_int128_t( 0 ) == ::serialize_int128_t( 0 ) );
    }

    // unary minus is two's complement negation; ~ and the bitwise operators work on the pattern
    {
        serialize_check( -::serialize_int128_t( 5 ) == ::serialize_int128_t( -5 ) );
        serialize_check( -::serialize_int128_t( -5 ) == ::serialize_int128_t( 5 ) );
        serialize_check( -int128_min == int128_min );                                               // -INT128_MIN wraps to itself, documented
        serialize_check( +::serialize_int128_t( -5 ) == ::serialize_int128_t( -5 ) );
        serialize_check( ~::serialize_int128_t( 0 ) == ::serialize_int128_t( -1 ) );
        serialize_check( ( ::serialize_int128_t( -1 ) & ::serialize_int128_t( 5 ) ) == ::serialize_int128_t( 5 ) );
        serialize_check( ( ::serialize_int128_t( 0 ) | ::serialize_int128_t( -1 ) ) == ::serialize_int128_t( -1 ) );
        serialize_check( ( ::serialize_int128_t( -1 ) ^ ::serialize_int128_t( -1 ) ) == ::serialize_int128_t( 0 ) );
    }

    // increment and decrement, both forms, across zero and across the lane boundary
    {
        ::serialize_int128_t value( -1 );
        serialize_check( ++value == ::serialize_int128_t( 0 ) );
        serialize_check( value++ == ::serialize_int128_t( 0 ) );
        serialize_check( value == ::serialize_int128_t( 1 ) );
        serialize_check( --value == ::serialize_int128_t( 0 ) );
        serialize_check( value-- == ::serialize_int128_t( 0 ) );
        serialize_check( value == ::serialize_int128_t( -1 ) );

        ::serialize_int128_t boundary = int128_emulated( 0, 0xFFFFFFFFFFFFFFFFULL );                      // 2^64 - 1
        serialize_check( ++boundary == int128_emulated( 1, 0 ) );

        ::serialize_int128_t accumulator( -10 );
        accumulator += ::serialize_int128_t( 4 );  serialize_check( accumulator == ::serialize_int128_t( -6 ) );
        accumulator -= ::serialize_int128_t( -6 ); serialize_check( accumulator == ::serialize_int128_t( 0 ) );
        accumulator = ::serialize_int128_t( -3 );
        accumulator *= ::serialize_int128_t( -3 ); serialize_check( accumulator == ::serialize_int128_t( 9 ) );
        accumulator /= ::serialize_int128_t( -2 ); serialize_check( accumulator == ::serialize_int128_t( -4 ) );
        accumulator %= ::serialize_int128_t( 3 );  serialize_check( accumulator == ::serialize_int128_t( -1 ) );
        accumulator <<= 64;                  serialize_check( accumulator == int128_emulated( 0xFFFFFFFFFFFFFFFFULL, 0 ) );
        accumulator >>= 64;                  serialize_check( accumulator == ::serialize_int128_t( -1 ) );
        accumulator &= ::serialize_int128_t( 6 );  serialize_check( accumulator == ::serialize_int128_t( 6 ) );
        accumulator |= ::serialize_int128_t( 1 );  serialize_check( accumulator == ::serialize_int128_t( 7 ) );
        accumulator ^= ::serialize_int128_t( -1 ); serialize_check( accumulator == ::serialize_int128_t( -8 ) );
    }
}

#if defined(__SIZEOF_INT128__)

inline serialize::uint128_t uint128_native_from_emulated( ::serialize_uint128_t value )
{
    return ( serialize::uint128_t( value.hi ) << 64 ) | value.lo;
}

inline void check_uint128_agree( ::serialize_uint128_t emulated, serialize::uint128_t native )
{
    serialize_check( emulated.lo == uint64_t( native ) );
    serialize_check( emulated.hi == uint64_t( native >> 64 ) );
}

inline void test_uint128_differential()
{
    // where native __int128 exists, the emulated type is proven against it operator by operator:
    // a fixed seed LCG generates operand pairs, and every operator must agree exactly. this is
    // the test that keeps the sibling copies of the emulation block honest — drift fails loudly.

    uint64_t lcg = 0x123456789ABCDEF0ULL;

    #define serialize_test_next_lcg() ( lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL )

    for ( int i = 0; i < 400; i++ )
    {
        uint64_t a_hi = serialize_test_next_lcg();
        uint64_t a_lo = serialize_test_next_lcg();
        uint64_t b_hi = serialize_test_next_lcg();
        uint64_t b_lo = serialize_test_next_lcg();

        // bias some operands toward the interesting edges: empty lanes, saturated lanes, equal operands
        switch ( i % 8 )
        {
            case 1: a_hi = 0; break;
            case 2: a_lo = 0; break;
            case 3: b_hi = 0; break;
            case 4: b_lo = 0; break;
            case 5: a_hi = 0xFFFFFFFFFFFFFFFFULL; a_lo = 0xFFFFFFFFFFFFFFFFULL; break;
            case 6: b_hi = 0; b_lo = uint64_t( 1 ) << ( i % 63 ); break;                    // powers of two divide cleanly
            case 7: b_hi = a_hi; b_lo = a_lo; break;
            default: break;
        }

        const ::serialize_uint128_t ea = uint128_emulated( a_hi, a_lo );
        const ::serialize_uint128_t eb = uint128_emulated( b_hi, b_lo );
        const serialize::uint128_t na = ( serialize::uint128_t( a_hi ) << 64 ) | a_lo;
        const serialize::uint128_t nb = ( serialize::uint128_t( b_hi ) << 64 ) | b_lo;

        check_uint128_agree( ea + eb, na + nb );
        check_uint128_agree( ea - eb, na - nb );
        check_uint128_agree( ea * eb, na * nb );
        check_uint128_agree( ea & eb, na & nb );
        check_uint128_agree( ea | eb, na | nb );
        check_uint128_agree( ea ^ eb, na ^ nb );
        check_uint128_agree( ~ea, ~na );
        check_uint128_agree( -ea, serialize::uint128_t( 0 ) - na );
        check_uint128_agree( +ea, na );

        if ( eb != ::serialize_uint128_t( 0 ) )
        {
            check_uint128_agree( ea / eb, na / nb );            // zero divisors are excluded PERMANENTLY: by zero is undefined on both sides and native hardware disagrees with itself (arm64 x/0 is 0 but x%0 is the dividend; x86-64 traps), so agreement is not a property either side can promise. Totality is pinned in test_uint128_emulation instead
            check_uint128_agree( ea % eb, na % nb );
        }

        const int shift = int( b_lo % 128 );                    // native shifts of 128 or more are undefined; the emulated choice is pinned in test_uint128_emulation
        check_uint128_agree( ea << shift, na << shift );
        check_uint128_agree( ea >> shift, na >> shift );

        serialize_check( ( ea == eb ) == ( na == nb ) );
        serialize_check( ( ea != eb ) == ( na != nb ) );
        serialize_check( ( ea <  eb ) == ( na <  nb ) );
        serialize_check( ( ea >  eb ) == ( na >  nb ) );
        serialize_check( ( ea <= eb ) == ( na <= nb ) );
        serialize_check( ( ea >= eb ) == ( na >= nb ) );

        // construction agreement from both signednesses of the drawn lanes
        check_uint128_agree( ::serialize_uint128_t( a_lo ), serialize::uint128_t( a_lo ) );
        check_uint128_agree( ::serialize_uint128_t( int64_t( a_lo ) ), serialize::uint128_t( int64_t( a_lo ) ) );

        ::serialize_uint128_t emulated_accumulator = ea;
        serialize::uint128_t native_accumulator = na;
        check_uint128_agree( ++emulated_accumulator, ++native_accumulator );
        check_uint128_agree( emulated_accumulator++, native_accumulator++ );
        check_uint128_agree( emulated_accumulator, native_accumulator );
        check_uint128_agree( --emulated_accumulator, --native_accumulator );
        check_uint128_agree( emulated_accumulator--, native_accumulator-- );
        check_uint128_agree( emulated_accumulator, native_accumulator );

        emulated_accumulator = ea;
        native_accumulator = na;
        emulated_accumulator += eb; native_accumulator += nb; check_uint128_agree( emulated_accumulator, native_accumulator );
        emulated_accumulator -= eb; native_accumulator -= nb; check_uint128_agree( emulated_accumulator, native_accumulator );
        emulated_accumulator *= eb; native_accumulator *= nb; check_uint128_agree( emulated_accumulator, native_accumulator );
        emulated_accumulator &= eb; native_accumulator &= nb; check_uint128_agree( emulated_accumulator, native_accumulator );
        emulated_accumulator |= eb; native_accumulator |= nb; check_uint128_agree( emulated_accumulator, native_accumulator );
        emulated_accumulator ^= eb; native_accumulator ^= nb; check_uint128_agree( emulated_accumulator, native_accumulator );
        emulated_accumulator <<= shift; native_accumulator <<= shift; check_uint128_agree( emulated_accumulator, native_accumulator );
        emulated_accumulator >>= shift; native_accumulator >>= shift; check_uint128_agree( emulated_accumulator, native_accumulator );
        if ( eb != ::serialize_uint128_t( 0 ) )
        {
            emulated_accumulator = ea; native_accumulator = na;
            emulated_accumulator /= eb; native_accumulator /= nb; check_uint128_agree( emulated_accumulator, native_accumulator );
            emulated_accumulator = ea; native_accumulator = na;
            emulated_accumulator %= eb; native_accumulator %= nb; check_uint128_agree( emulated_accumulator, native_accumulator );
        }
    }

    #undef serialize_test_next_lcg

    // construction differential: the exact cross signed cases the audit proved uncovered.
    // identical source expressions must construct identical values in both representations.
    {
        check_uint128_agree( ::serialize_uint128_t( int64_t( -1 ) ),       serialize::uint128_t( int64_t( -1 ) ) );
        check_uint128_agree( ::serialize_uint128_t( INT64_MIN ),           serialize::uint128_t( INT64_MIN ) );
        check_uint128_agree( ::serialize_uint128_t( uint64_t( 1 ) << 63 ), serialize::uint128_t( uint64_t( 1 ) << 63 ) );
        check_uint128_agree( ::serialize_uint128_t( UINT64_MAX ),          serialize::uint128_t( UINT64_MAX ) );
        check_uint128_agree( ::serialize_uint128_t( -5 ),                  serialize::uint128_t( -5 ) );
        check_uint128_agree( ::serialize_uint128_t( 3000000000U ),         serialize::uint128_t( 3000000000U ) );
    }

    // cross representation wire identity: the emulated type and native __int128 must produce
    // byte identical wire through serialize_uint128, and each must read the other's bytes back
    // exactly. this is what makes the emulation a drop in replacement on compilers without
    // native 128 bit support.
    {
        const uint64_t lanes[][2] =                             // { hi, lo }
        {
            { 0, 0 },
            { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL },
            { 0xFFFFFFFFFFFFFFFFULL, 0 },                                       // high half only
            { 0, 0xFFFFFFFFFFFFFFFFULL },                                       // low half only
            { 0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL },                   // distinct halves
        };

        for ( int i = 0; i < (int) ( sizeof( lanes ) / sizeof( lanes[0] ) ); i++ )
        {
            ::serialize_uint128_t emulated = uint128_emulated( lanes[i][0], lanes[i][1] );
            serialize::uint128_t native = uint128_native_from_emulated( emulated );

            uint8_t emulated_buffer[16 + 8] = { 0 };            // + 8: read buffer allocations extend 8 bytes past the data
            serialize::WriteStream emulatedStream( emulated_buffer, 16 );
            serialize_check( serialize::serialize_uint128_internal( emulatedStream, emulated ) == true );
            emulatedStream.Flush();

            uint8_t native_buffer[16 + 8] = { 0 };
            serialize::WriteStream nativeStream( native_buffer, 16 );
            serialize_check( serialize::serialize_uint128_internal( nativeStream, native ) == true );
            nativeStream.Flush();

            serialize_check( emulatedStream.GetBitsProcessed() == nativeStream.GetBitsProcessed() );
            serialize_check( memcmp( emulated_buffer, native_buffer, 16 ) == 0 );

            // read the native bytes back through the emulated type
            serialize::ReadStream readStream( native_buffer, 16 );
            ::serialize_uint128_t read_back( 0 );
            serialize_check( serialize::serialize_uint128_internal( readStream, read_back ) == true );
            serialize_check( read_back == uint128_emulated( lanes[i][0], lanes[i][1] ) );
        }
    }
}

inline serialize::int128_t int128_native_from_emulated( ::serialize_int128_t value )
{
    return serialize::int128_t( ( serialize::uint128_t( value.hi ) << 64 ) | value.lo );
}

inline void check_int128_agree( ::serialize_int128_t emulated, serialize::int128_t native )
{
    serialize_check( emulated.lo == uint64_t( serialize::uint128_t( native ) ) );
    serialize_check( emulated.hi == uint64_t( serialize::uint128_t( native ) >> 64 ) );
}

inline void test_int128_differential()
{
    // the signed counterpart of test_uint128_differential: a fixed seed LCG generates operand
    // pairs biased toward the sign boundaries, and every operator must agree exactly with native
    // signed __int128. where the native operation would be undefined behavior — signed overflow
    // in + - * << and unary minus — the native expectation is computed in the unsigned domain,
    // which is the defined spelling of the two's complement wrap the emulation implements.

    const serialize::int128_t native_int128_min = serialize::int128_t( serialize::uint128_t( 1 ) << 127 );

    uint64_t lcg = 0xFEDCBA9876543210ULL;

    #define serialize_test_next_lcg() ( lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL )

    for ( int i = 0; i < 400; i++ )
    {
        uint64_t a_hi = serialize_test_next_lcg();
        uint64_t a_lo = serialize_test_next_lcg();
        uint64_t b_hi = serialize_test_next_lcg();
        uint64_t b_lo = serialize_test_next_lcg();

        // bias operands toward the sign boundaries: zero, ±1, INT64_MIN/MAX sign extended,
        // INT128_MIN/MAX, negated and equal pairs
        switch ( i % 10 )
        {
            case 1: a_hi = 0; a_lo = 0; break;
            case 2: a_hi = 0xFFFFFFFFFFFFFFFFULL; a_lo = 0xFFFFFFFFFFFFFFFFULL; break;              // -1
            case 3: a_hi = 0xFFFFFFFFFFFFFFFFULL; a_lo = 0x8000000000000000ULL; break;              // INT64_MIN sign extended
            case 4: a_hi = 0; a_lo = 0x7FFFFFFFFFFFFFFFULL; break;                                  // INT64_MAX
            case 5: a_hi = 0x8000000000000000ULL; a_lo = 0; break;                                  // INT128_MIN
            case 6: a_hi = 0x7FFFFFFFFFFFFFFFULL; a_lo = 0xFFFFFFFFFFFFFFFFULL; break;              // INT128_MAX
            case 7: b_hi = 0xFFFFFFFFFFFFFFFFULL; b_lo = 0xFFFFFFFFFFFFFFFFULL; break;              // -1: the overflowing divisor
            case 8: b_hi = ~a_hi; b_lo = ~a_lo; break;                                              // ~a: b = -a - 1, mixed signs
            case 9: b_hi = a_hi; b_lo = a_lo; break;
            default: break;
        }

        const ::serialize_int128_t ea = int128_emulated( a_hi, a_lo );
        const ::serialize_int128_t eb = int128_emulated( b_hi, b_lo );
        const serialize::uint128_t ua = ( serialize::uint128_t( a_hi ) << 64 ) | a_lo;
        const serialize::uint128_t ub = ( serialize::uint128_t( b_hi ) << 64 ) | b_lo;
        const serialize::int128_t na = serialize::int128_t( ua );
        const serialize::int128_t nb = serialize::int128_t( ub );

        // + - * and unary minus: native expectations via the unsigned domain, avoiding native
        // signed overflow, which is undefined behavior. the wrap is the semantics under test.
        check_int128_agree( ea + eb, serialize::int128_t( ua + ub ) );
        check_int128_agree( ea - eb, serialize::int128_t( ua - ub ) );
        check_int128_agree( ea * eb, serialize::int128_t( ua * ub ) );
        check_int128_agree( -ea, serialize::int128_t( serialize::uint128_t( 0 ) - ua ) );
        check_int128_agree( +ea, na );

        check_int128_agree( ea & eb, na & nb );
        check_int128_agree( ea | eb, na | nb );
        check_int128_agree( ea ^ eb, na ^ nb );
        check_int128_agree( ~ea, ~na );

        if ( nb != 0 && ! ( na == native_int128_min && nb == -1 ) )
        {
            check_int128_agree( ea / eb, na / nb );         // the excluded case is native undefined behavior; the emulated wrap is pinned in test_int128_emulation
            check_int128_agree( ea % eb, na % nb );
        }

        const int shift = int( b_lo % 128 );                // native shifts of 128 or more are undefined; the emulated choice is pinned in test_int128_emulation
        check_int128_agree( ea >> shift, na >> shift );                                             // native >> on negatives is the arithmetic shift on every target compiler
        check_int128_agree( ea << shift, serialize::int128_t( ua << shift ) );                      // native << on negatives is undefined pre C++20: the unsigned domain is the defined spelling

        serialize_check( ( ea == eb ) == ( na == nb ) );
        serialize_check( ( ea != eb ) == ( na != nb ) );
        serialize_check( ( ea <  eb ) == ( na <  nb ) );
        serialize_check( ( ea >  eb ) == ( na >  nb ) );
        serialize_check( ( ea <= eb ) == ( na <= nb ) );
        serialize_check( ( ea >= eb ) == ( na >= nb ) );

        // increments and decrements via the unsigned domain, since native ++ overflows at INT128_MAX
        ::serialize_int128_t incremented = ea;
        ++incremented;
        check_int128_agree( incremented, serialize::int128_t( ua + 1 ) );
        incremented--;
        check_int128_agree( incremented, na );
        ::serialize_int128_t decremented = ea;
        --decremented;
        check_int128_agree( decremented, serialize::int128_t( ua - 1 ) );

        // conversions: the sign extending and value preserving constructors and the truncating
        // conversion match native, from both signednesses of the drawn lane
        const int64_t seed64 = int64_t( a_lo );
        check_int128_agree( ::serialize_int128_t( seed64 ), serialize::int128_t( seed64 ) );
        check_int128_agree( ::serialize_int128_t( a_lo ), serialize::int128_t( a_lo ) );
        serialize_check( int64_t( ea ) == int64_t( uint64_t( ua ) ) );

        // compound forms via the unsigned domain where overflow could occur
        ::serialize_int128_t accumulator = ea;
        accumulator += eb; check_int128_agree( accumulator, serialize::int128_t( ua + ub ) );
        accumulator = ea;
        accumulator -= eb; check_int128_agree( accumulator, serialize::int128_t( ua - ub ) );
        accumulator = ea;
        accumulator *= eb; check_int128_agree( accumulator, serialize::int128_t( ua * ub ) );
        accumulator = ea;
        accumulator &= eb; check_int128_agree( accumulator, na & nb );
        accumulator = ea;
        accumulator |= eb; check_int128_agree( accumulator, na | nb );
        accumulator = ea;
        accumulator ^= eb; check_int128_agree( accumulator, na ^ nb );
        accumulator = ea;
        accumulator >>= shift; check_int128_agree( accumulator, na >> shift );
        accumulator = ea;
        accumulator <<= shift; check_int128_agree( accumulator, serialize::int128_t( ua << shift ) );
        if ( nb != 0 && ! ( na == native_int128_min && nb == -1 ) )
        {
            accumulator = ea;
            accumulator /= eb; check_int128_agree( accumulator, na / nb );
            accumulator = ea;
            accumulator %= eb; check_int128_agree( accumulator, na % nb );
        }
    }

    #undef serialize_test_next_lcg

    // construction differential: the exact cross signed cases the audit proved uncovered.
    // identical source expressions must construct identical values in both representations.
    {
        check_int128_agree( ::serialize_int128_t( int64_t( -1 ) ),       serialize::int128_t( int64_t( -1 ) ) );
        check_int128_agree( ::serialize_int128_t( INT64_MIN ),           serialize::int128_t( INT64_MIN ) );
        check_int128_agree( ::serialize_int128_t( uint64_t( 1 ) << 63 ), serialize::int128_t( uint64_t( 1 ) << 63 ) );
        check_int128_agree( ::serialize_int128_t( UINT64_MAX ),          serialize::int128_t( UINT64_MAX ) );
        check_int128_agree( ::serialize_int128_t( -5 ),                  serialize::int128_t( -5 ) );
        check_int128_agree( ::serialize_int128_t( 3000000000U ),         serialize::int128_t( 3000000000U ) );
    }
}

#endif // #if defined(__SIZEOF_INT128__)

#if defined( SERIALIZE_HAS_COMPILE_TIME_SURFACE )

// Compile time surface tests. Every compile time form is held to the coverage of its runtime twin
// (round trip, boundary values, read side rejection, measure agreement), and to the wire identity
// property: given identical inputs, the compile time form must produce byte identical wire data to
// the runtime form. That property is what lets a code generator switch forms without a wire change.

// each helper drives the actual macro form under test, so the tests cover the macros as well as the templates

template <typename Stream> bool serialize_int_runtime_form( Stream & stream, int32_t & value, int32_t min, int32_t max )
{
    serialize_int( stream, value, min, max );
    return true;
}

template <typename Stream> bool serialize_int64_runtime_form( Stream & stream, int64_t & value, int64_t min, int64_t max )
{
    serialize_int64( stream, value, min, max );
    return true;
}

template <typename Stream> bool serialize_bits64_runtime_form( Stream & stream, uint64_t & value, int bits )
{
    serialize_bits( stream, value, bits );
    return true;
}

template <int32_t Min, int32_t Max, typename Stream> bool serialize_int_compile_time_form( Stream & stream, int32_t & value )
{
    serialize_int_compile_time( stream, value, Min, Max );
    return true;
}

template <int64_t Min, int64_t Max, typename Stream> bool serialize_int64_compile_time_form( Stream & stream, int64_t & value )
{
    serialize_int64_compile_time( stream, value, Min, Max );
    return true;
}

template <int Bits, typename Stream> bool serialize_bits_compile_time_form( Stream & stream, uint32_t & value )
{
    serialize_bits_compile_time( stream, value, Bits );
    return true;
}

template <int Bits, typename Stream> bool serialize_bits64_compile_time_form( Stream & stream, uint64_t & value )
{
    serialize_bits64_compile_time( stream, value, Bits );
    return true;
}

inline void test_compile_time_bits_required()
{
    // usable in template argument position by construction
    static_assert( serialize::bits_required64_constexpr( 0, 255 ) == 8, "must be a constant expression" );

    // agrees with the runtime bits_required across the 32 bit table
    serialize_check( serialize::bits_required64_constexpr( 0, 0 ) == 0 );
    serialize_check( serialize::bits_required64_constexpr( 0, 1 ) == 1 );
    serialize_check( serialize::bits_required64_constexpr( 0, 2 ) == 2 );
    serialize_check( serialize::bits_required64_constexpr( 0, 3 ) == 2 );
    serialize_check( serialize::bits_required64_constexpr( 0, 4 ) == 3 );
    serialize_check( serialize::bits_required64_constexpr( 0, 7 ) == 3 );
    serialize_check( serialize::bits_required64_constexpr( 0, 8 ) == 4 );
    serialize_check( serialize::bits_required64_constexpr( 0, 255 ) == 8 );
    serialize_check( serialize::bits_required64_constexpr( 0, 65535 ) == 16 );
    serialize_check( serialize::bits_required64_constexpr( 0, 4294967295ULL ) == 32 );

    // agrees with the runtime bits_required64 on 64 bit ranges
    serialize_check( serialize::bits_required64_constexpr( 0, 4294967296ULL ) == 33 );
    serialize_check( serialize::bits_required64_constexpr( 0, ( 1ULL << 40 ) ) == 41 );
    serialize_check( serialize::bits_required64_constexpr( 0, 0xFFFFFFFFFFFFFFFFULL ) == 64 );
    serialize_check( serialize::bits_required64_constexpr( uint64_t(INT64_MIN), uint64_t(INT64_MAX) ) == 64 );
    serialize_check( serialize::bits_required64_constexpr( uint64_t(-5000000000LL), uint64_t(+5000000000LL) ) == 34 );

    // sign extended int32 bounds wrap to the same difference as the uint32 runtime path
    serialize_check( serialize::bits_required64_constexpr( uint64_t( int64_t( -100 ) ), uint64_t( int64_t( +100 ) ) ) == serialize::bits_required( uint32_t( -100 ), uint32_t( +100 ) ) );
    serialize_check( serialize::bits_required64_constexpr( uint64_t( int64_t( INT32_MIN ) ), uint64_t( int64_t( INT32_MAX ) ) ) == 32 );

    // sweep agreement with the runtime form
    for ( uint32_t max = 0; max <= 1000; max++ )
    {
        serialize_check( serialize::bits_required64_constexpr( 0, max ) == serialize::bits_required( 0, max ) );
    }
}

template <int32_t Min, int32_t Max> inline void check_compile_time_int_range( const int32_t * values, int num_values )
{
    for ( int i = 0; i < num_values; i++ )
    {
        const int32_t value = values[i];

        // measure: both forms agree on cost
        serialize::MeasureStream measureRuntime;
        int32_t measure_value = value;
        serialize_check( serialize_int_runtime_form( measureRuntime, measure_value, Min, Max ) == true );
        serialize::MeasureStream measureCompileTime;
        measure_value = value;
        serialize_check( ( serialize_int_compile_time_form<Min, Max>( measureCompileTime, measure_value ) == true ) );
        serialize_check( measureRuntime.GetBitsProcessed() == measureCompileTime.GetBitsProcessed() );

        // write: the wire identity property, byte for byte
        uint8_t buffer_runtime[8 + 8] = { 0 };              // + 8: read buffer allocations extend 8 bytes past the data
        uint8_t buffer_compile_time[8 + 8] = { 0 };         // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeRuntime( buffer_runtime, 8 );
        int32_t write_value = value;
        serialize_check( serialize_int_runtime_form( writeRuntime, write_value, Min, Max ) == true );
        writeRuntime.Flush();

        serialize::WriteStream writeCompileTime( buffer_compile_time, 8 );
        write_value = value;
        serialize_check( ( serialize_int_compile_time_form<Min, Max>( writeCompileTime, write_value ) == true ) );
        writeCompileTime.Flush();

        serialize_check( writeRuntime.GetBitsProcessed() == writeCompileTime.GetBitsProcessed() );
        serialize_check( memcmp( buffer_runtime, buffer_compile_time, 8 ) == 0 );

        // round trip through the compile time form
        serialize::ReadStream readStream( buffer_compile_time, (int) writeCompileTime.GetBytesProcessed() );
        int32_t read_value = 0;
        serialize_check( ( serialize_int_compile_time_form<Min, Max>( readStream, read_value ) == true ) );
        serialize_check( read_value == value );

        // cross decode: runtime written bytes through the compile time form
        serialize::ReadStream crossStream( buffer_runtime, (int) writeRuntime.GetBytesProcessed() );
        read_value = 0;
        serialize_check( ( serialize_int_compile_time_form<Min, Max>( crossStream, read_value ) == true ) );
        serialize_check( read_value == value );
    }
}

inline void test_compile_time_int()
{
    {
        const int32_t values[] = { -100, -99, -37, 0, +1, +99, +100 };
        check_compile_time_int_range<-100, +100>( values, (int) ( sizeof(values) / sizeof(values[0]) ) );
    }

    {
        const int32_t values[] = { 0, 1, 255, 32768, 65534, 65535 };
        check_compile_time_int_range<0, 65535>( values, (int) ( sizeof(values) / sizeof(values[0]) ) );
    }

    // ranges wider than 2^31 overflow if [min,max] arithmetic is done signed (undefined behavior)
    {
        const int32_t values[] = { INT32_MIN, INT32_MIN + 1, -1, 0, +1, INT32_MAX - 1, INT32_MAX };
        check_compile_time_int_range<INT32_MIN, INT32_MAX>( values, (int) ( sizeof(values) / sizeof(values[0]) ) );
    }

    {
        const int32_t values[] = { -2000000000, -1, 0, +1000000000, +2000000000 };
        check_compile_time_int_range<-2000000000, +2000000000>( values, (int) ( sizeof(values) / sizeof(values[0]) ) );
    }
}

template <int64_t Min, int64_t Max> inline void check_compile_time_int64_range( const int64_t * values, int num_values )
{
    for ( int i = 0; i < num_values; i++ )
    {
        const int64_t value = values[i];

        // measure: both forms agree on cost
        serialize::MeasureStream measureRuntime;
        int64_t measure_value = value;
        serialize_check( serialize_int64_runtime_form( measureRuntime, measure_value, Min, Max ) == true );
        serialize::MeasureStream measureCompileTime;
        measure_value = value;
        serialize_check( ( serialize_int64_compile_time_form<Min, Max>( measureCompileTime, measure_value ) == true ) );
        serialize_check( measureRuntime.GetBitsProcessed() == measureCompileTime.GetBitsProcessed() );

        // write: the wire identity property, byte for byte
        uint8_t buffer_runtime[8 + 8] = { 0 };              // + 8: read buffer allocations extend 8 bytes past the data
        uint8_t buffer_compile_time[8 + 8] = { 0 };         // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeRuntime( buffer_runtime, 8 );
        int64_t write_value = value;
        serialize_check( serialize_int64_runtime_form( writeRuntime, write_value, Min, Max ) == true );
        writeRuntime.Flush();

        serialize::WriteStream writeCompileTime( buffer_compile_time, 8 );
        write_value = value;
        serialize_check( ( serialize_int64_compile_time_form<Min, Max>( writeCompileTime, write_value ) == true ) );
        writeCompileTime.Flush();

        serialize_check( writeRuntime.GetBitsProcessed() == writeCompileTime.GetBitsProcessed() );
        serialize_check( memcmp( buffer_runtime, buffer_compile_time, 8 ) == 0 );

        // round trip through the compile time form
        serialize::ReadStream readStream( buffer_compile_time, (int) writeCompileTime.GetBytesProcessed() );
        int64_t read_value = 0;
        serialize_check( ( serialize_int64_compile_time_form<Min, Max>( readStream, read_value ) == true ) );
        serialize_check( read_value == value );

        // cross decode: runtime written bytes through the compile time form
        serialize::ReadStream crossStream( buffer_runtime, (int) writeRuntime.GetBytesProcessed() );
        read_value = 0;
        serialize_check( ( serialize_int64_compile_time_form<Min, Max>( crossStream, read_value ) == true ) );
        serialize_check( read_value == value );
    }
}

inline void test_compile_time_int64()
{
    // small ranges use the single dword path
    {
        const int64_t values[] = { -100, -99, -1, 0, +1, +55, +99, +100 };
        check_compile_time_int64_range<-100, +100>( values, (int) ( sizeof(values) / sizeof(values[0]) ) );
    }

    // ranges spanning more than 32 bits use the two dword path
    {
        const int64_t values[] = { -5000000000LL, -5000000000LL + 1, -1, 0, +1, 4123456789LL, +5000000000LL - 1, +5000000000LL };
        check_compile_time_int64_range<-5000000000LL, +5000000000LL>( values, (int) ( sizeof(values) / sizeof(values[0]) ) );
    }

    // ranges wider than 2^63 overflow if [min,max] arithmetic is done signed (undefined behavior)
    {
        const int64_t values[] = { INT64_MIN, INT64_MIN + 1, -1, 0, +1, INT64_MAX - 1, INT64_MAX };
        check_compile_time_int64_range<INT64_MIN, INT64_MAX>( values, (int) ( sizeof(values) / sizeof(values[0]) ) );
    }

    // the single dword path uses the minimal number of bits, same as the runtime form
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        int64_t value = 55;
        serialize_check( ( serialize::SerializeInt64Const<-100, +100>( writeStream, value ) == true ) );
        writeStream.Flush();
        serialize_check( writeStream.GetBitsProcessed() == 8 );        // bits_required64(-100,100) == 8, same as the 32 bit path
    }
}

template <int Bits> inline void check_compile_time_bits_width()
{
    // ( ( 1 << ( Bits - 1 ) ) << 1 ) - 1 is the all ones value without shifting by the full width (undefined behavior at Bits == 32)
    const uint32_t max_value = uint32_t( ( ( uint64_t(1) << ( Bits - 1 ) ) << 1 ) - 1 );
    const uint32_t values[] = { 0, 1, max_value >> 1, max_value };

    for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
    {
        const uint32_t value = values[i];

        // measure: both forms agree on cost
        serialize::MeasureStream measureRuntime;
        uint64_t measure_value64 = value;
        serialize_check( serialize_bits64_runtime_form( measureRuntime, measure_value64, Bits ) == true );
        serialize::MeasureStream measureCompileTime;
        uint32_t measure_value = value;
        serialize_check( serialize_bits_compile_time_form<Bits>( measureCompileTime, measure_value ) == true );
        serialize_check( measureRuntime.GetBitsProcessed() == measureCompileTime.GetBitsProcessed() );

        // write: the wire identity property, byte for byte
        uint8_t buffer_runtime[8 + 8] = { 0 };              // + 8: read buffer allocations extend 8 bytes past the data
        uint8_t buffer_compile_time[8 + 8] = { 0 };         // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeRuntime( buffer_runtime, 8 );
        uint64_t write_value64 = value;
        serialize_check( serialize_bits64_runtime_form( writeRuntime, write_value64, Bits ) == true );
        writeRuntime.Flush();

        serialize::WriteStream writeCompileTime( buffer_compile_time, 8 );
        uint32_t write_value = value;
        serialize_check( serialize_bits_compile_time_form<Bits>( writeCompileTime, write_value ) == true );
        writeCompileTime.Flush();

        serialize_check( writeRuntime.GetBitsProcessed() == writeCompileTime.GetBitsProcessed() );
        serialize_check( memcmp( buffer_runtime, buffer_compile_time, 8 ) == 0 );

        // round trip through the compile time form
        serialize::ReadStream readStream( buffer_compile_time, (int) writeCompileTime.GetBytesProcessed() );
        uint32_t read_value = 0;
        serialize_check( serialize_bits_compile_time_form<Bits>( readStream, read_value ) == true );
        serialize_check( read_value == value );

        // cross decode: runtime written bytes through the compile time form
        serialize::ReadStream crossStream( buffer_runtime, (int) writeRuntime.GetBytesProcessed() );
        read_value = 0;
        serialize_check( serialize_bits_compile_time_form<Bits>( crossStream, read_value ) == true );
        serialize_check( read_value == value );
    }
}

template <int Bits> inline void check_compile_time_bits64_width()
{
    // ( ( 1 << ( Bits - 1 ) ) << 1 ) - 1 is the all ones value without shifting by the full width (undefined behavior at Bits == 64)
    const uint64_t max_value = ( ( uint64_t(1) << ( Bits - 1 ) ) << 1 ) - 1;
    const uint64_t values[] = { 0, 1, max_value >> 1, max_value };

    for ( int i = 0; i < (int) ( sizeof(values) / sizeof(values[0]) ); i++ )
    {
        const uint64_t value = values[i];

        // measure: both forms agree on cost
        serialize::MeasureStream measureRuntime;
        uint64_t measure_value = value;
        serialize_check( serialize_bits64_runtime_form( measureRuntime, measure_value, Bits ) == true );
        serialize::MeasureStream measureCompileTime;
        measure_value = value;
        serialize_check( serialize_bits64_compile_time_form<Bits>( measureCompileTime, measure_value ) == true );
        serialize_check( measureRuntime.GetBitsProcessed() == measureCompileTime.GetBitsProcessed() );

        // write: the wire identity property, byte for byte
        uint8_t buffer_runtime[8 + 8] = { 0 };              // + 8: read buffer allocations extend 8 bytes past the data
        uint8_t buffer_compile_time[8 + 8] = { 0 };         // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeRuntime( buffer_runtime, 8 );
        uint64_t write_value = value;
        serialize_check( serialize_bits64_runtime_form( writeRuntime, write_value, Bits ) == true );
        writeRuntime.Flush();

        serialize::WriteStream writeCompileTime( buffer_compile_time, 8 );
        write_value = value;
        serialize_check( serialize_bits64_compile_time_form<Bits>( writeCompileTime, write_value ) == true );
        writeCompileTime.Flush();

        serialize_check( writeRuntime.GetBitsProcessed() == writeCompileTime.GetBitsProcessed() );
        serialize_check( memcmp( buffer_runtime, buffer_compile_time, 8 ) == 0 );

        // round trip through the compile time form
        serialize::ReadStream readStream( buffer_compile_time, (int) writeCompileTime.GetBytesProcessed() );
        uint64_t read_value = 0;
        serialize_check( serialize_bits64_compile_time_form<Bits>( readStream, read_value ) == true );
        serialize_check( read_value == value );

        // cross decode: runtime written bytes through the compile time form
        serialize::ReadStream crossStream( buffer_runtime, (int) writeRuntime.GetBytesProcessed() );
        read_value = 0;
        serialize_check( serialize_bits64_compile_time_form<Bits>( crossStream, read_value ) == true );
        serialize_check( read_value == value );
    }
}

inline void test_compile_time_bits()
{
    check_compile_time_bits_width<1>();
    check_compile_time_bits_width<2>();
    check_compile_time_bits_width<7>();
    check_compile_time_bits_width<8>();
    check_compile_time_bits_width<13>();
    check_compile_time_bits_width<23>();
    check_compile_time_bits_width<31>();
    check_compile_time_bits_width<32>();

    check_compile_time_bits64_width<1>();
    check_compile_time_bits64_width<20>();
    check_compile_time_bits64_width<32>();
    check_compile_time_bits64_width<33>();
    check_compile_time_bits64_width<48>();
    check_compile_time_bits64_width<63>();
    check_compile_time_bits64_width<64>();
}

inline void test_compile_time_int_validation()
{
    // bits_required(0,5) is 3 bits, so a malicious packet can encode 6 or 7. reads must reject values above max.
    {
        uint8_t buffer[4 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 8 );
        uint32_t out_of_range = 7;
        writeStream.SerializeBits( out_of_range, 3 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 4 );
        int32_t value = 0;
        serialize_check( ( serialize::SerializeIntConst<0, 5>( readStream, value ) == false ) );
    }

    // reads past the end of the buffer must fail cleanly
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::ReadStream readStream( buffer, 1 );
        int32_t value = 0;
        serialize_check( ( serialize::SerializeIntConst<0, 65535>( readStream, value ) == false ) );
    }
}

inline void test_compile_time_int64_validation()
{
    // a malicious packet can smuggle an out of range value into the bit headroom of the two dword path. reads must reject it.
    {
        uint8_t buffer[16 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::WriteStream writeStream( buffer, 16 );
        const uint64_t out_of_range = ( 1ULL << 34 ) + 5;               // range [0, 2^34] is 35 bits, so values above 2^34 fit in the headroom
        uint32_t lo = uint32_t( out_of_range & 0xFFFFFFFF );
        uint32_t hi = uint32_t( out_of_range >> 32 );
        writeStream.SerializeBits( lo, 32 );
        writeStream.SerializeBits( hi, 3 );
        writeStream.Flush();

        serialize::ReadStream readStream( buffer, 16 );
        int64_t value = 0;
        serialize_check( ( serialize::SerializeInt64Const<0, ( int64_t(1) << 34 )>( readStream, value ) ) == false );
    }

    // reads past the end of the buffer must fail cleanly (the low dword fits, the high remainder does not)
    {
        uint8_t buffer[4 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::ReadStream readStream( buffer, 4 );
        int64_t value = 0;
        serialize_check( ( serialize::SerializeInt64Const<INT64_MIN, INT64_MAX>( readStream, value ) ) == false );
    }
}

inline void test_compile_time_bits_validation()
{
    // reads past the end of the buffer must fail cleanly
    {
        uint8_t buffer[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::ReadStream readStream( buffer, 1 );
        uint32_t value = 0;
        serialize_check( serialize::SerializeBitsConst<13>( readStream, value ) == false );
    }

    // the two dword path: the low dword fits, the high remainder does not
    {
        uint8_t buffer[4 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

        serialize::ReadStream readStream( buffer, 4 );
        uint64_t value = 0;
        serialize_check( serialize::SerializeBits64Const<48>( readStream, value ) == false );
    }
}

// a packet exercising every compile time form next to its runtime twin, field for field

struct CompileTimeTestPacket
{
    CompileTimeTestPacket()
    {
        memset( (void*) this, 0, sizeof( CompileTimeTestPacket ) );
    }

    int32_t int_a;          // [-100,+100]
    int32_t int_b;          // [0,65535]
    int32_t int_c;          // [INT32_MIN,INT32_MAX]
    int64_t int64_small;    // [-100,+100]: single dword path
    int64_t int64_wide;     // [-5000000000,+5000000000]: two dword path
    int64_t int64_full;     // [INT64_MIN,INT64_MAX]
    uint32_t bits1;
    uint32_t bits7;
    uint32_t bits13;
    uint32_t bits23;
    uint32_t bits32;
    uint64_t bits48;        // the >32 bit serialize_bits case
    uint64_t bits64;
    bool flag_a;
    bool flag_b;

    void InitTypical()
    {
        int_a = -37;
        int_b = 12345;
        int_c = -123456789;
        int64_small = 55;
        int64_wide = 4123456789LL;
        int64_full = -123456789012345LL;
        bits1 = 1;
        bits7 = 97;
        bits13 = 5000;
        bits23 = 1234567;
        bits32 = 0xDEADBEEF;
        bits48 = 0x123456789ABCULL;
        bits64 = 0x123456789ABCDEF0ULL;
        flag_a = true;
        flag_b = false;
    }

    void InitLow()          // every field at its minimum legal value
    {
        int_a = -100;
        int_b = 0;
        int_c = INT32_MIN;
        int64_small = -100;
        int64_wide = -5000000000LL;
        int64_full = INT64_MIN;
        bits1 = 0;
        bits7 = 0;
        bits13 = 0;
        bits23 = 0;
        bits32 = 0;
        bits48 = 0;
        bits64 = 0;
        flag_a = false;
        flag_b = false;
    }

    void InitHigh()         // every field at its maximum legal value
    {
        int_a = +100;
        int_b = 65535;
        int_c = INT32_MAX;
        int64_small = +100;
        int64_wide = +5000000000LL;
        int64_full = INT64_MAX;
        bits1 = 1;
        bits7 = 127;
        bits13 = 8191;
        bits23 = 8388607;
        bits32 = 0xFFFFFFFF;
        bits48 = ( 1ULL << 48 ) - 1;
        bits64 = 0xFFFFFFFFFFFFFFFFULL;
        flag_a = true;
        flag_b = true;
    }

    template <typename Stream> bool SerializeRuntime( Stream & stream )
    {
        serialize_int( stream, int_a, -100, +100 );
        serialize_int( stream, int_b, 0, 65535 );
        serialize_int( stream, int_c, INT32_MIN, INT32_MAX );
        serialize_int64( stream, int64_small, -100, +100 );
        serialize_int64( stream, int64_wide, -5000000000LL, +5000000000LL );
        serialize_int64( stream, int64_full, INT64_MIN, INT64_MAX );
        serialize_bits( stream, bits1, 1 );
        serialize_bits( stream, bits7, 7 );
        serialize_bits( stream, bits13, 13 );
        serialize_bits( stream, bits23, 23 );
        serialize_bits( stream, bits32, 32 );
        serialize_bits( stream, bits48, 48 );
        serialize_bits( stream, bits64, 64 );
        serialize_bool( stream, flag_a );
        serialize_bool( stream, flag_b );
        return true;
    }

    template <typename Stream> bool SerializeCompileTime( Stream & stream )
    {
        serialize_int_compile_time( stream, int_a, -100, +100 );
        serialize_int_compile_time( stream, int_b, 0, 65535 );
        serialize_int_compile_time( stream, int_c, INT32_MIN, INT32_MAX );
        serialize_int64_compile_time( stream, int64_small, -100, +100 );
        serialize_int64_compile_time( stream, int64_wide, -5000000000LL, +5000000000LL );
        serialize_int64_compile_time( stream, int64_full, INT64_MIN, INT64_MAX );
        serialize_bits_compile_time( stream, bits1, 1 );
        serialize_bits_compile_time( stream, bits7, 7 );
        serialize_bits_compile_time( stream, bits13, 13 );
        serialize_bits_compile_time( stream, bits23, 23 );
        serialize_bits_compile_time( stream, bits32, 32 );
        serialize_bits64_compile_time( stream, bits48, 48 );
        serialize_bits64_compile_time( stream, bits64, 64 );
        serialize_bool_compile_time( stream, flag_a );
        serialize_bool_compile_time( stream, flag_b );
        return true;
    }

    bool operator == ( const CompileTimeTestPacket & other ) const
    {
        return int_a == other.int_a &&
               int_b == other.int_b &&
               int_c == other.int_c &&
               int64_small == other.int64_small &&
               int64_wide == other.int64_wide &&
               int64_full == other.int64_full &&
               bits1 == other.bits1 &&
               bits7 == other.bits7 &&
               bits13 == other.bits13 &&
               bits23 == other.bits23 &&
               bits32 == other.bits32 &&
               bits48 == other.bits48 &&
               bits64 == other.bits64 &&
               flag_a == other.flag_a &&
               flag_b == other.flag_b;
    }

    bool operator != ( const CompileTimeTestPacket & other ) const
    {
        return ! ( *this == other );
    }
};

inline void check_compile_time_packet( CompileTimeTestPacket & packet )
{
    const int BufferSize = 64;

    uint8_t buffer_runtime[BufferSize + 8];             // + 8: read buffer allocations extend 8 bytes past the data
    uint8_t buffer_compile_time[BufferSize + 8];        // + 8: read buffer allocations extend 8 bytes past the data
    memset( buffer_runtime, 0, sizeof( buffer_runtime ) );
    memset( buffer_compile_time, 0, sizeof( buffer_compile_time ) );

    serialize::WriteStream writeRuntime( buffer_runtime, BufferSize );
    serialize_check( packet.SerializeRuntime( writeRuntime ) == true );
    writeRuntime.Flush();

    serialize::WriteStream writeCompileTime( buffer_compile_time, BufferSize );
    serialize_check( packet.SerializeCompileTime( writeCompileTime ) == true );
    writeCompileTime.Flush();

    // the wire identity property: byte identical, so a code generator can switch forms without a wire change
    serialize_check( writeRuntime.GetBitsProcessed() == writeCompileTime.GetBitsProcessed() );
    serialize_check( memcmp( buffer_runtime, buffer_compile_time, BufferSize ) == 0 );

    // measure streams agree with the write streams (no aligns in this packet, so the measure is exact)
    serialize::MeasureStream measureRuntime;
    serialize_check( packet.SerializeRuntime( measureRuntime ) == true );
    serialize::MeasureStream measureCompileTime;
    serialize_check( packet.SerializeCompileTime( measureCompileTime ) == true );
    serialize_check( measureRuntime.GetBitsProcessed() == writeRuntime.GetBitsProcessed() );
    serialize_check( measureCompileTime.GetBitsProcessed() == writeCompileTime.GetBitsProcessed() );

    const int bytesWritten = (int) writeRuntime.GetBytesProcessed();

    // cross decode: runtime written bytes through the compile time form...
    {
        serialize::ReadStream readStream( buffer_runtime, bytesWritten );
        CompileTimeTestPacket read_packet;
        serialize_check( read_packet.SerializeCompileTime( readStream ) == true );
        serialize_check( read_packet == packet );
    }

    // ...and compile time written bytes through the runtime form
    {
        serialize::ReadStream readStream( buffer_compile_time, bytesWritten );
        CompileTimeTestPacket read_packet;
        serialize_check( read_packet.SerializeRuntime( readStream ) == true );
        serialize_check( read_packet == packet );
    }
}

inline void test_compile_time_packet()
{
    CompileTimeTestPacket packet;

    packet.InitTypical();
    check_compile_time_packet( packet );

    packet.InitLow();
    check_compile_time_packet( packet );

    packet.InitHigh();
    check_compile_time_packet( packet );
}

#endif // #if defined( SERIALIZE_HAS_COMPILE_TIME_SURFACE )

// Golden wire format test. The exact bytes produced by the serializer are pinned down here and must never change.
// If this test fails, the wire format has changed and previously written data no longer decodes: a breaking change.
// The values below are chosen so every platform quantizes identically (see the compressed float: 5.0 in [0,10]
// normalizes to exactly 0.5, so fp contraction differences between compilers cannot shift the quantized integer).

struct GoldenWireData
{
    uint32_t bits4;
    uint32_t bits11;
    uint32_t bits24;
    uint32_t bits32;
    int32_t int_small;
    int32_t int_full;
    bool flag;
    float float_value;
    float compressed_float_value;
    double double_value;
    uint8_t uint8_value;
    uint16_t uint16_value;
    uint32_t uint32_value;
    uint64_t uint64_value;
    int relative_near;
    int relative_far;
    uint8_t bytes[7];
    char string[16];
    wchar_t wstring[8];
    int16_t fixed_q8_8;
    int32_t fixed_q16_16;
    int64_t fixed_q48_16;
    uint32_t fixed_q16_16_unsigned;
    serialize::int128_t fixed_q112_16_wide;
    serialize::int128_t fixed_q64_64_wide;
};

inline void GoldenWireInit( GoldenWireData & data )
{
    memset( (void*) &data, 0, sizeof( GoldenWireData ) );
    data.bits4 = 13;
    data.bits11 = 1445;
    data.bits24 = 11259375;
    data.bits32 = 0xDEADBEEF;
    data.int_small = -37;
    data.int_full = -123456789;
    data.flag = true;
    data.float_value = 3.1415926f;
    data.compressed_float_value = 5.0f;
    data.double_value = 1.0 / 3.0;
    data.uint8_value = 0x7F;
    data.uint16_value = 0x1234;
    data.uint32_value = 0x12345678;
    data.uint64_value = 0x123456789ABCDEF0ULL;
    data.relative_near = 101;                   // difference of 1 from the base: exercises the one bit branch
    data.relative_far = 2100;                   // difference of 2000 from the base: exercises the twelve bit bucket
    const uint8_t golden_byte_data[7] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0x01 };
    memcpy( data.bytes, golden_byte_data, sizeof( golden_byte_data ) );
    serialize_copy_string( data.string, "golden", sizeof( data.string ) );
    // built from explicit code points so the source file encoding can never change the golden bytes
    const wchar_t golden_wide_string[4] = { 0x043C, 0x0438, 0x0440, 0 };            // cyrillic, BMP only
    serialize_copy_wstring( data.wstring, golden_wide_string, sizeof( data.wstring ) / sizeof( wchar_t ) );
    data.fixed_q8_8 = int16_t( -( 3 * 256 + 64 ) );                                 // -3.25 in Q8.8
    data.fixed_q16_16 = 1234 * 65536 + 32768;                                       // 1234.5 in Q16.16
    data.fixed_q48_16 = -( int64_t( 54321 ) * 65536 + 12345 );                      // -54321.1883... in Q48.16
    data.fixed_q16_16_unsigned = 29999u * 65536 + 65535;                            // 29999.99998... in Q16.16: every fraction bit set
    data.fixed_q112_16_wide = serialize::int128_t( -( int64_t( 98765432109LL ) * 65536 + 4321 ) );      // -98765432109.066 in Q112.16: 75 bits on the wire, three groups
    data.fixed_q64_64_wide = ( serialize::int128_t( 0x0123456789ABCDEFLL ) << 64 )
                           + serialize::int128_t( 0x0FEDCBA987654321LL );           // Q64.64 over the full unit range: 128 bits, four groups, every group distinct
}

template <typename Stream> bool GoldenWireSerialize( Stream & stream, GoldenWireData & data )
{
    const int relative_base = 100;
    serialize_bits( stream, data.bits4, 4 );
    serialize_bits( stream, data.bits11, 11 );
    serialize_bits( stream, data.bits24, 24 );
    serialize_bits( stream, data.bits32, 32 );
    serialize_int( stream, data.int_small, -100, +100 );
    serialize_int( stream, data.int_full, INT32_MIN, INT32_MAX );
    serialize_bool( stream, data.flag );
    serialize_float( stream, data.float_value );
    serialize_compressed_float( stream, data.compressed_float_value, 0.0f, 10.0f, 0.01f );
    serialize_double( stream, data.double_value );
    serialize_uint8( stream, data.uint8_value );
    serialize_uint16( stream, data.uint16_value );
    serialize_uint32( stream, data.uint32_value );
    serialize_uint64( stream, data.uint64_value );
    serialize_int_relative( stream, relative_base, data.relative_near );
    serialize_int_relative( stream, relative_base, data.relative_far );
    serialize_align( stream );
    serialize_bytes( stream, data.bytes, (int) sizeof( data.bytes ) );
    serialize_string( stream, data.string, (int) sizeof( data.string ) );
    serialize_wstring( stream, data.wstring, (int) ( sizeof( data.wstring ) / sizeof( wchar_t ) ) );
    serialize_align( stream );                  // the fixed point section starts byte aligned, so every byte pinned above it stays put
    serialize_fixed( stream, data.fixed_q8_8, 8, 8, -100, +100 );
    serialize_fixed( stream, data.fixed_q16_16, 16, 16, -2000, +2000 );
    serialize_fixed( stream, data.fixed_q48_16, 48, 16, -100000, +100000 );
    serialize_fixed( stream, data.fixed_q16_16_unsigned, 16, 16, 0, 30000 );
    serialize_align( stream );                  // the wide fixed section starts byte aligned, so every byte pinned above it stays put
    serialize_fixed( stream, data.fixed_q112_16_wide, 112, 16, -144115188075855872LL, +144115188075855872LL );      // ±2^57 units: 75 bits, the three group structure
    serialize_fixed( stream, data.fixed_q64_64_wide, 64, 64, INT64_MIN, INT64_MAX );                                // full unit range: 128 bits, the four group structure
    return true;
}

static const uint8_t golden_wire_bytes[] =
{
    0x5D, 0xDA, 0xF7, 0xE6, 0xD5, 0x77, 0xDF, 0x56, 0xEF, 0x9F, 0x75, 0x19,
    0x52, 0xBC, 0xDA, 0x0F, 0x49, 0x40, 0xF4, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0xFF, 0xFC, 0xD1, 0x48, 0xE0, 0x59, 0xD1, 0x48, 0xC0, 0x7B,
    0xF3, 0x6A, 0xE2, 0x59, 0xD1, 0x48, 0x84, 0xB7, 0x06, 0xDE, 0xAD, 0xBE,
    0xEF, 0xCA, 0xFE, 0x01, 0x06, 0x67, 0x6F, 0x6C, 0x64, 0x65, 0x6E, 0xE3,
    0x21, 0x00, 0x00, 0xC0, 0x21, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
    0xC0, 0x60, 0x00, 0x80, 0xA2, 0x7C, 0xFC, 0xEC, 0x26, 0xCB, 0xFF, 0xFF,
    0x4B, 0x1D, 0x1F, 0xEF, 0xD2, 0x1A, 0x1F, 0x01, 0xE9, 0xFF, 0xFF, 0x09,
    0x19, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x78, 0x6F, 0x5E, 0x4D, 0x3C,
    0x2B, 0x1A, 0x09, 0x04
};

inline void test_golden_wire_format()
{
    // write side: serializing the golden values must produce exactly the golden bytes
    {
        uint8_t buffer[256];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        GoldenWireData data;
        GoldenWireInit( data );
        serialize_check( GoldenWireSerialize( stream, data ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( golden_wire_bytes ) );
        serialize_check( memcmp( buffer, golden_wire_bytes, sizeof( golden_wire_bytes ) ) == 0 );
    }

    // read side: the golden bytes must decode to the expected values, on every platform, forever
    {
        uint8_t buffer[256];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, golden_wire_bytes, sizeof( golden_wire_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( golden_wire_bytes ) );
        GoldenWireData data;
        memset( (void*) &data, 0, sizeof( GoldenWireData ) );
        serialize_check( GoldenWireSerialize( stream, data ) == true );

        GoldenWireData expected;
        GoldenWireInit( expected );
        serialize_check( data.bits4 == expected.bits4 );
        serialize_check( data.bits11 == expected.bits11 );
        serialize_check( data.bits24 == expected.bits24 );
        serialize_check( data.bits32 == expected.bits32 );
        serialize_check( data.int_small == expected.int_small );
        serialize_check( data.int_full == expected.int_full );
        serialize_check( data.flag == expected.flag );
        serialize_check( data.float_value == expected.float_value );
        serialize_check( fabs( data.compressed_float_value - expected.compressed_float_value ) <= 0.01f );
        serialize_check( data.double_value == expected.double_value );
        serialize_check( data.uint8_value == expected.uint8_value );
        serialize_check( data.uint16_value == expected.uint16_value );
        serialize_check( data.uint32_value == expected.uint32_value );
        serialize_check( data.uint64_value == expected.uint64_value );
        serialize_check( data.relative_near == expected.relative_near );
        serialize_check( data.relative_far == expected.relative_far );
        serialize_check( memcmp( data.bytes, expected.bytes, sizeof( data.bytes ) ) == 0 );
        serialize_check( strcmp( data.string, expected.string ) == 0 );
        serialize_check( wcscmp( data.wstring, expected.wstring ) == 0 );
        serialize_check( data.fixed_q8_8 == expected.fixed_q8_8 );
        serialize_check( data.fixed_q16_16 == expected.fixed_q16_16 );
        serialize_check( data.fixed_q48_16 == expected.fixed_q48_16 );
        serialize_check( data.fixed_q16_16_unsigned == expected.fixed_q16_16_unsigned );
        serialize_check( data.fixed_q112_16_wide == expected.fixed_q112_16_wide );
        serialize_check( data.fixed_q64_64_wide == expected.fixed_q64_64_wide );
    }
}

inline void test_trailing_bits()
{
    // STANDARD.md, "Trailing bits" (adopted 2026-08-15): writers must emit zero in the unused
    // bits of the final byte; readers must not reject a stream for their contents. The third
    // rule — non-zero trailing bits as a provenance signal — belongs to tooling (see
    // tools/conformance), never to this read path, which is exactly what the reader half of
    // this test proves.

    // writer obligation: emit a message that ends 3 bits into its final byte, into a buffer
    // pre-filled with 0xFF so the zeros must come from the writer, not from the caller.
    {
        uint8_t buffer[64];
        memset( buffer, 0xFF, sizeof( buffer ) );

        serialize::WriteStream writeStream( buffer, (int) sizeof( buffer ) );
        uint32_t head = 0xDEADBEEF;
        serialize_check( writeStream.SerializeBits( head, 32 ) == true );
        uint32_t tail = 5;
        serialize_check( writeStream.SerializeBits( tail, 3 ) == true );
        writeStream.Flush();

        const int bytesWritten = (int) writeStream.GetBytesProcessed();
        const int bitsInFinalByte = (int) ( writeStream.GetBitsProcessed() % 8 );
        serialize_check( bitsInFinalByte == 3 );                                        // the stream really does end unaligned
        const uint8_t trailingMask = (uint8_t) ( 0xFF << bitsInFinalByte );
        serialize_check( ( buffer[bytesWritten-1] & trailingMask ) == 0 );              // writers must write zero

        // reader indifference, small stream: set every trailing bit and read back. the
        // doctored stream must be accepted and must decode the same values.
        buffer[bytesWritten-1] |= trailingMask;
        serialize::ReadStream readStream( buffer, bytesWritten );
        uint32_t readHead = 0;
        serialize_check( readStream.SerializeBits( readHead, 32 ) == true );
        serialize_check( readHead == 0xDEADBEEF );
        uint32_t readTail = 0;
        serialize_check( readStream.SerializeBits( readTail, 3 ) == true );
        serialize_check( readTail == 5 );
    }

    // reader indifference, full message: doctor the trailing bits of the golden stream.
    // a conforming reader accepts the doctored stream and decodes byte-identical results.
    {
        uint8_t buffer[256];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, golden_wire_bytes, sizeof( golden_wire_bytes ) );

        serialize::ReadStream cleanStream( buffer, (int) sizeof( golden_wire_bytes ) );
        GoldenWireData cleanData;
        memset( (void*) &cleanData, 0, sizeof( GoldenWireData ) );
        serialize_check( GoldenWireSerialize( cleanStream, cleanData ) == true );

        const int bitsInFinalByte = (int) ( cleanStream.GetBitsProcessed() % 8 );
        serialize_check( bitsInFinalByte != 0 );                                        // golden ends unaligned, so this test can discriminate
        const uint8_t trailingMask = (uint8_t) ( 0xFF << bitsInFinalByte );
        serialize_check( ( golden_wire_bytes[sizeof( golden_wire_bytes ) - 1] & trailingMask ) == 0 );  // the pinned emission met the writer obligation

        buffer[sizeof( golden_wire_bytes ) - 1] |= trailingMask;                        // set every trailing bit

        serialize::ReadStream doctoredStream( buffer, (int) sizeof( golden_wire_bytes ) );
        GoldenWireData doctoredData;
        memset( (void*) &doctoredData, 0, sizeof( GoldenWireData ) );
        serialize_check( GoldenWireSerialize( doctoredStream, doctoredData ) == true );                 // readers must not reject
        serialize_check( memcmp( &doctoredData, &cleanData, sizeof( GoldenWireData ) ) == 0 );          // and must decode identically
        serialize_check( doctoredStream.GetBitsProcessed() == cleanStream.GetBitsProcessed() );
    }
}

inline void test_past_end_poison()
{
    // STANDARD.md, "Past-end memory is an implementation contract, not a format concern"
    // (ruled 2026-08-15: the draft stands). The C++ reader loads 64-bit windows at byte
    // granularity and requires its caller to allocate at least 8 bytes past the data; bytes
    // past the end are loaded but never interpreted. This proves the "never interpreted"
    // half: poison planted beyond the stream end must not change a single decoded byte, and
    // must not change refusal behavior.

    // accept path: identical decode with a zeroed tail and a poisoned tail
    {
        uint8_t cleanBuffer[256];
        uint8_t poisonBuffer[256];
        memset( cleanBuffer, 0, sizeof( cleanBuffer ) );
        memset( poisonBuffer, 0xFF, sizeof( poisonBuffer ) );           // poison everywhere, including the loaded-but-never-interpreted window
        memcpy( cleanBuffer, golden_wire_bytes, sizeof( golden_wire_bytes ) );
        memcpy( poisonBuffer, golden_wire_bytes, sizeof( golden_wire_bytes ) );

        serialize::ReadStream cleanStream( cleanBuffer, (int) sizeof( golden_wire_bytes ) );
        GoldenWireData cleanData;
        memset( (void*) &cleanData, 0, sizeof( GoldenWireData ) );
        serialize_check( GoldenWireSerialize( cleanStream, cleanData ) == true );

        serialize::ReadStream poisonStream( poisonBuffer, (int) sizeof( golden_wire_bytes ) );
        GoldenWireData poisonData;
        memset( (void*) &poisonData, 0, sizeof( GoldenWireData ) );
        serialize_check( GoldenWireSerialize( poisonStream, poisonData ) == true );

        serialize_check( memcmp( &poisonData, &cleanData, sizeof( GoldenWireData ) ) == 0 );    // byte-identical decode
        serialize_check( poisonStream.GetBitsProcessed() == cleanStream.GetBitsProcessed() );
    }

    // refusal path: truncate the stream one byte short so the decode must fail. the bytes at
    // and past the truncated end are exactly where the reader's 64-bit window loads from, and
    // the refusal must be identical whether they are zero or poison.
    {
        const int truncatedBytes = (int) sizeof( golden_wire_bytes ) - 1;

        uint8_t cleanBuffer[256];
        uint8_t poisonBuffer[256];
        memset( cleanBuffer, 0, sizeof( cleanBuffer ) );
        memset( poisonBuffer, 0xFF, sizeof( poisonBuffer ) );
        memcpy( cleanBuffer, golden_wire_bytes, truncatedBytes );
        memcpy( poisonBuffer, golden_wire_bytes, truncatedBytes );

        serialize::ReadStream cleanStream( cleanBuffer, truncatedBytes );
        GoldenWireData cleanData;
        memset( (void*) &cleanData, 0, sizeof( GoldenWireData ) );
        serialize_check( GoldenWireSerialize( cleanStream, cleanData ) == false );

        serialize::ReadStream poisonStream( poisonBuffer, truncatedBytes );
        GoldenWireData poisonData;
        memset( (void*) &poisonData, 0, sizeof( GoldenWireData ) );
        serialize_check( GoldenWireSerialize( poisonStream, poisonData ) == false );

        serialize_check( poisonStream.GetBitsProcessed() == cleanStream.GetBitsProcessed() );   // refused at the same point
        serialize_check( memcmp( &poisonData, &cleanData, sizeof( GoldenWireData ) ) == 0 );    // with identical partial state
    }
}


// ---------------------------------------------------------------------------------------
// Does THIS build actually fuse, and does it fuse ACROSS statements?
//
// The compressed float's whole FMA discipline is a pair of stores through float locals that
// keep the quantization's two roundings distinct. Whether a build EXERCISES that discipline
// depends on two things the source cannot see: whether the target has an FMA instruction at
// all, and what the compiler's contraction setting is. -ffp-contract=on buys nothing on an
// x86-64 without FMA, and a build that prints a green log while fusing nothing has claimed a
// discipline it never ran. So measure it and say which.
//
// The three probes are deliberately different questions:
//
//   is_live               -- one contractible statement against the same arithmetic forced
//                            through a volatile store. If they ever differ, this target has
//                            an FMA and this build is permitted to use it.
//   crosses_statements    -- a PLAIN local store against the volatile one. The language
//                            rounds a float local, so only a compiler contracting ACROSS the
//                            statement boundary can make these two disagree. That is the
//                            -ffp-contract=fast behaviour (GCC's default at every -O level),
//                            and it is not a stricter `on`: it is a different mode, in which
//                            a plain float local suppresses nothing. The WIRE is safe in
//                            such a build -- the audited home pins both roundings with
//                            SERIALIZE_FLOAT_FORCE_ROUND, which no contraction mode fuses
//                            through -- but the frozen pre-split oracle in the differential
//                            below is a verbatim copy of code that predates the barrier, so
//                            it fuses, computes one-rounding values, and its comparisons
//                            stand down on such a build (the negative-control sentinels
//                            keep their teeth everywhere; see the differential).
//   barrier_holds         -- the audited home's shape, a store pinned by
//                            SERIALIZE_FLOAT_FORCE_ROUND, against the volatile reference.
//                            These must NEVER disagree: the barrier is what the wire's
//                            contraction invariance rests on, so a toolchain that fuses
//                            through it is refused by name before a single vector runs.
//
// crosses_statements has to be DETECTED rather than inferred from the flag, because GCC
// before 14 mapped -ffp-contract=on onto fast. On such a toolchain, asking for
// statement-local contraction silently gets cross-statement contraction instead, and the
// honest report says which mode is actually live -- not a bit pattern mismatch that reads
// like a port bug.
// ---------------------------------------------------------------------------------------

// The three spellings, each in its OWN function and each kept out of line. Out of line is
// deliberate: written as three expressions in one loop they share the subexpression `a * b`,
// the optimizer merges it, and a multiply with more than one use is not one the backend will
// fuse -- so the probe could report "does not fuse" about a build that fuses perfectly well.
// noinline keeps the three arithmetics from being merged into one and makes the probe a
// question about the compiler's setting rather than about its common-subexpression pass.
//
// One honest limit, measured rather than assumed: at -O0 no contraction setting produces an
// FMA at all, so an unoptimized build reports "not available" -- which is exactly true of
// that binary, and is why the report is printed rather than asserted.

#if defined( _MSC_VER )
#define SERIALIZE_TEST_NOINLINE __declspec(noinline)
#elif defined( __GNUC__ ) || defined( __clang__ )
#define SERIALIZE_TEST_NOINLINE __attribute__((noinline))
#else
#define SERIALIZE_TEST_NOINLINE
#endif

// contractible: one expression, so any compiler permitted to contract at all may fuse it
SERIALIZE_TEST_NOINLINE inline float serialize_test_fp_one_expression( float a, float b, float c )
{
    return a * b + c;
}

// two statements through a plain float local. The language rounds a float local, so only a
// compiler contracting ACROSS the statement boundary can fuse this -- which is the shape of
// the compressed float's audited home, and the property under test.
SERIALIZE_TEST_NOINLINE inline float serialize_test_fp_two_statements( float a, float b, float c )
{
    const float product = a * b;
    return product + c;
}

// the reference: a volatile store forces the product to memory as float32, so no contraction
// setting on any compiler can fuse through it. This is what the other three are measured against.
SERIALIZE_TEST_NOINLINE inline float serialize_test_fp_forced_unfused( float a, float b, float c )
{
    volatile float product = a * b;
    return product + c;
}

// the audited home's shape: a local pinned by SERIALIZE_FLOAT_FORCE_ROUND. This is the
// mechanism the wire's contraction invariance rests on, probed directly.
SERIALIZE_TEST_NOINLINE inline float serialize_test_fp_two_statements_barriered( float a, float b, float c )
{
    float product = a * b;
    SERIALIZE_FLOAT_FORCE_ROUND( product );
    return product + c;
}

inline bool serialize_test_fp_contraction_is_live()
{
    for ( uint32_t code = 1; code < 20000; code++ )
    {
        const float norm = (float) code / 20000.0f;
        const float fused = serialize_test_fp_one_expression( norm, 200.0f, -100.0f );
        const float unfused = serialize_test_fp_forced_unfused( norm, 200.0f, -100.0f );
        uint32_t pattern_fused, pattern_unfused;
        memcpy( &pattern_fused, &fused, 4 );
        memcpy( &pattern_unfused, &unfused, 4 );
        if ( pattern_fused != pattern_unfused )
        {
            return true;
        }
    }
    return false;
}

inline bool serialize_test_fp_contraction_crosses_statements()
{
    for ( uint32_t code = 1; code < 20000; code++ )
    {
        const float norm = (float) code / 20000.0f;
        const float across = serialize_test_fp_two_statements( norm, 200.0f, -100.0f );
        const float unfused = serialize_test_fp_forced_unfused( norm, 200.0f, -100.0f );
        uint32_t pattern_across, pattern_unfused;
        memcpy( &pattern_across, &across, 4 );
        memcpy( &pattern_unfused, &unfused, 4 );
        if ( pattern_across != pattern_unfused )
        {
            return true;
        }
    }
    return false;
}

inline bool serialize_test_fp_barrier_holds()
{
    for ( uint32_t code = 1; code < 20000; code++ )
    {
        const float norm = (float) code / 20000.0f;
        const float barriered = serialize_test_fp_two_statements_barriered( norm, 200.0f, -100.0f );
        const float unfused = serialize_test_fp_forced_unfused( norm, 200.0f, -100.0f );
        uint32_t pattern_barriered, pattern_unfused;
        memcpy( &pattern_barriered, &barriered, 4 );
        memcpy( &pattern_unfused, &unfused, 4 );
        if ( pattern_barriered != pattern_unfused )
        {
            return false;
        }
    }
    return true;
}

// The caption for this build, so the two runs CMake performs are telling apart in a CI log.
// CMakeLists.txt defines it on the -ffp-contract=on target; the default covers a hand build
// and says nothing it cannot know.

#if !defined( SERIALIZE_TEST_FP_CONTRACT )
#define SERIALIZE_TEST_FP_CONTRACT "the default flags"
#endif // #if !defined( SERIALIZE_TEST_FP_CONTRACT )

// Conformance vector for compressed_float over a range with a NON-ZERO min. Additive: the
// golden wire test above is untouched and its bytes are pinned forever.
//
// The golden's compressed_float is 5.0 over [0,10]: min is 0, so the reader's final add
// contributes nothing and a reader whose multiply and add are fused into one FMA decodes
// the identical float — that vector is structurally blind to reader contraction. This one
// pins the decoded BIT PATTERNS, not just the wire bytes, over [-100,100] at resolution
// 0.01 (max_integer_value = 20000, 15 bits per value), where the add is load-bearing:
//
//   written value   quantized   pinned decode   what a wrong reading produces
//   0.0             10000       0x00000000      nothing — on-quantum sanity row, agrees everywhere
//   -99.875         13          0xC2C7BD71      a writer widened to double quantizes 12, not 13:
//                                               the scaled product is exactly 12.5, a boundary
//                                               value landing between quanta, and double resolves
//                                               it to 12.499999... — different wire bytes
//   -33.34          6666        0xC2055C2A      a reader contracted to an FMA decodes 0xC2055C29
//
// The last row is why this test exists: at plain -O2 on arm64 (clang's default
// -ffp-contract=on), the pre-fix single-expression reader decoded 0xC2055C29 — one ulp off
// every conformant runtime, so an arm64 re-encode produced different wire — and no vector
// in the family could see it. Under -ffast-math (the gate binaries' former flags) the 0.0
// row also breaks: reciprocal approximation of the division decodes 0xB6298800.

template <typename Stream> bool CompressedFloatNonZeroMinSerialize( Stream & stream, float & a, float & b, float & c )
{
    serialize_compressed_float( stream, a, -100.0f, 100.0f, 0.01f );
    serialize_compressed_float( stream, b, -100.0f, 100.0f, 0.01f );
    serialize_compressed_float( stream, c, -100.0f, 100.0f, 0.01f );
    serialize_align( stream );
    return true;
}

inline void test_compressed_float_conformance_nonzero_min()
{
    static const uint8_t pinned_bytes[6] = { 0x10, 0xA7, 0x06, 0x80, 0x82, 0x06 };

    // write side: the strict two-rounding quantization must produce exactly these bytes
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        float a = 0.0f;
        float b = -99.875f;
        float c = -33.34f;
        serialize_check( CompressedFloatNonZeroMinSerialize( stream, a, b, c ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
        serialize_check( memcmp( buffer, pinned_bytes, sizeof( pinned_bytes ) ) == 0 );
    }

    // read side: the decoded floats are pinned bit-exactly. tolerance comparison would
    // defeat the purpose — the divergence this detects is a single ulp.
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, pinned_bytes, sizeof( pinned_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( pinned_bytes ) );
        float a = -1.0f;
        float b = -1.0f;
        float c = -1.0f;
        serialize_check( CompressedFloatNonZeroMinSerialize( stream, a, b, c ) == true );
        uint32_t bits_a, bits_b, bits_c;
        memcpy( &bits_a, &a, 4 );
        memcpy( &bits_b, &b, 4 );
        memcpy( &bits_c, &c, 4 );
        // These stay pinned unconditionally on every build the library supports, under every
        // contraction mode including -ffp-contract=fast: the reader's two roundings are
        // pinned by SERIALIZE_FLOAT_FORCE_ROUND, which no contraction setting fuses through.
        // Before the barrier, a consumer building this header at gcc's arm64 defaults
        // reconstructed the last of these as 0xC2055C29 -- one ulp off every conformant
        // runtime -- and this check is the one that caught it (issue #95).
        serialize_check( bits_a == 0x00000000u );
        serialize_check( bits_b == 0xC2C7BD71u );
        serialize_check( bits_c == 0xC2055C2Au );
    }
}

// Conformance vector for the WRITER's fusion class. Additive: every earlier vector is
// untouched. The nonzero-min vector above discriminates READER fusion (its write rows do not
// move under a fused writer -- measured), so a deleted writer barrier was invisible to the
// whole family. This one pins wire BYTES a fused writer cannot produce.
//
// The discriminating band is [2^23, 2^24) step counts -- the same band the normative integer
// clamp lives in (STANDARD.md, schema#109) -- because that is where the float32 ulp of the
// scaled product reaches 1 and the product's rounding decides the integer directly. Over
// [0, 16777215] at resolution 1 (max_integer_value = 2^24 - 1, 24 bits per value), measured
// exhaustively over every float in range on arm64: a writer whose product and +0.5 are fused
// into one FMA moves the quantized integer on 4,194,304 inputs -- every even float in the
// top binade -- and the first of them is 2^23 itself:
//
//   written value   pinned code   what a fused writer produces
//   8388608.0       8388608       8388609 -- one code up, different wire bytes
//   11184811.0      11184812      on-quantum-adjacent sanity row, agrees everywhere
//   16777215.0      16777215      the top code: the normative clamp's witness row
//
// The decoded BIT PATTERNS are pinned too. min is 0, so this vector is structurally blind to
// reader fusion -- that is the nonzero-min vector's job; the two vectors together cover both
// sides. serialize_test() runs on every supported build, including -ffp-contract=fast, so
// this is the vector that goes red if the writer's barrier is ever deleted.

template <typename Stream> bool CompressedFloatWriterFusionSerialize( Stream & stream, float & a, float & b, float & c )
{
    serialize_compressed_float( stream, a, 0.0f, 16777215.0f, 1.0f );
    serialize_compressed_float( stream, b, 0.0f, 16777215.0f, 1.0f );
    serialize_compressed_float( stream, c, 0.0f, 16777215.0f, 1.0f );
    serialize_align( stream );
    return true;
}

inline void test_compressed_float_conformance_writer_fusion()
{
    static const uint8_t pinned_bytes[9] = { 0x00, 0x00, 0x80, 0xAC, 0xAA, 0xAA, 0xFF, 0xFF, 0xFF };

    // write side: the strict two-rounding quantization must produce exactly these bytes.
    // 8388608.0f is the row a fused writer moves; the pinned bytes are unreachable from a
    // build whose writer rounds once.
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        float a = 8388608.0f;
        float b = 11184811.0f;
        float c = 16777215.0f;
        serialize_check( CompressedFloatWriterFusionSerialize( stream, a, b, c ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
        serialize_check( memcmp( buffer, pinned_bytes, sizeof( pinned_bytes ) ) == 0 );
    }

    // read side: the decoded floats are pinned bit-exactly, same terms as every conformance
    // vector -- the divergences this family detects are single ulps, and tolerance would
    // hide them.
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, pinned_bytes, sizeof( pinned_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( pinned_bytes ) );
        float a = -1.0f;
        float b = -1.0f;
        float c = -1.0f;
        serialize_check( CompressedFloatWriterFusionSerialize( stream, a, b, c ) == true );
        uint32_t bits_a, bits_b, bits_c;
        memcpy( &bits_a, &a, 4 );
        memcpy( &bits_b, &b, 4 );
        memcpy( &bits_c, &c, 4 );
        serialize_check( bits_a == 0x4B000000u );       // 8388608.0
        serialize_check( bits_b == 0x4B2AAAACu );       // 11184812.0
        serialize_check( bits_c == 0x4B7FFFFFu );       // 16777215.0
    }
}

// The non-zero-min conformance vector again, through the PRECOMPUTED entry point, with the
// constants a schema compiler derives for [-100,100] at resolution 0.01 written as literals —
// exactly what generated code would pass. Same pinned bytes, same pinned decoded bit patterns:
// the precomputed path is held to the conformance law directly, independently of the
// differential below.

template <typename Stream> bool CompressedFloatPrecomputedConformanceSerialize( Stream & stream, float & a, float & b, float & c )
{
    serialize_compressed_float_precomputed( stream, a, 20000, 15, 200.0f, -100.0f );
    serialize_compressed_float_precomputed( stream, b, 20000, 15, 200.0f, -100.0f );
    serialize_compressed_float_precomputed( stream, c, 20000, 15, 200.0f, -100.0f );
    serialize_align( stream );
    return true;
}

inline void test_compressed_float_precomputed_conformance()
{
    static const uint8_t pinned_bytes[6] = { 0x10, 0xA7, 0x06, 0x80, 0x82, 0x06 };

    // write side: the precomputed quantization must produce exactly the pinned bytes
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        float a = 0.0f;
        float b = -99.875f;
        float c = -33.34f;
        serialize_check( CompressedFloatPrecomputedConformanceSerialize( stream, a, b, c ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
        serialize_check( memcmp( buffer, pinned_bytes, sizeof( pinned_bytes ) ) == 0 );
    }

    // read side: the decoded floats are pinned bit-exactly, as in the derive-per-call vector above
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, pinned_bytes, sizeof( pinned_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( pinned_bytes ) );
        float a = -1.0f;
        float b = -1.0f;
        float c = -1.0f;
        serialize_check( CompressedFloatPrecomputedConformanceSerialize( stream, a, b, c ) == true );
        uint32_t bits_a, bits_b, bits_c;
        memcpy( &bits_a, &a, 4 );
        memcpy( &bits_b, &b, 4 );
        memcpy( &bits_c, &c, 4 );
        // pinned on the same terms as the derive-per-call vector above: see the note there
        serialize_check( bits_a == 0x00000000u );
        serialize_check( bits_b == 0xC2C7BD71u );
        serialize_check( bits_c == 0xC2055C2Au );
    }
}

// The mas-bandwidth/schema#82 differential: the derive-per-call compressed float against the
// precomputed one. Three implementations must be indistinguishable in measured bits, wire
// bytes, read acceptance and decoded BIT PATTERNS on every input:
//
//   reference   -- serialize_compressed_float_frozen_reference below, a verbatim copy of the
//                  serialize_compressed_float_internal body as it stood BEFORE it was split
//                  into serialize_compressed_float_params plus the precomputed home. FROZEN:
//                  never edit it. it is the code the audited home replaced, kept so the
//                  differential proves the split changed nothing, forever.
//   legacy      -- serialize_compressed_float( stream, value, min, max, res ), which since the
//                  split derives the constants per call and forwards to the audited home.
//   precomputed -- serialize_compressed_float_precomputed( ... ) with constants derived once
//                  per declaration by serialize_compressed_float_params, exactly as a schema
//                  compiler derives them at generation time.
//
// the declaration corpus is every compressed float declaration the schema compiler's examples,
// bench corpus and test data emit (harvested from the mas-bandwidth/schema PR #79 differential,
// which proved the same property for the Go generation-time fold), plus this repo's golden,
// conformance, fuzz and example declarations, plus shapes at the edges of the derivation
// itself: resolution coarser than the range, a step count that exactly fills its wire width,
// a million steps, and the clamp at the largest float below 2^32.
//
// inputs per declaration: a dense sweep with overshoot past both bounds, the quantization step
// edges and midpoints with their one-ulp neighbors (the midpoints are where a fused or widened
// writer diverges — STANDARD.md's 0.005-over-[0,10] class), specials (bounds and their one-ulp
// neighbors, negative zero, subnormals, float extremes), LCG uniform in-range values and LCG
// uniform float32 bit patterns — and on the read side every representable wire integer
// including the bit headroom, exhaustively up to 16-bit widths and sampled with the boundary
// codes pinned above that. decoded values compare by BIT PATTERN, never by tolerance: the
// divergence a fused or widened decode produces is one ulp, invisible to any tolerance.

template <typename Stream> bool serialize_compressed_float_frozen_reference( Stream & stream, float & value, float min, float max, float res )
{
    // verbatim pre-split serialize_compressed_float_internal, comments elided, statements
    // identical (locals included: the float stores are what pin the two roundings). DO NOT EDIT.
    serialize_assert( min < max && res > 0 );

    const float delta = max - min;

    float values = delta / res;

    serialize_assert( delta - delta == 0.0f );
    serialize_assert( values - values == 0.0f );

    if ( !( values >= 1.0f ) )
    {
        values = 1.0f;
    }
    else if ( values > 4294967040.0f )      // largest float below 2^32
    {
        values = 4294967040.0f;
    }

    const uint32_t maxIntegerValue = (uint32_t) ceil(values);

    const int bits = serialize::bits_required( 0, maxIntegerValue );

    uint32_t integerValue = 0;

    if ( Stream::IsWriting )
    {
        serialize_assert( value - value == 0.0f );

        float normalizedValue = (value - min) / delta;
        if ( !( normalizedValue >= 0.0f ) )
        {
            normalizedValue = 0.0f;
        }
        else if ( !( normalizedValue <= 1.0f ) )
        {
            normalizedValue = 1.0f;
        }
        const float scaled = normalizedValue * maxIntegerValue;
        integerValue = (uint32_t) floor( scaled + 0.5f );
        // STANDARD.md: the integer clamp is normative (2026-08-23) -- same
        // clamp as serialize_compressed_float above, same reason.
        if ( integerValue > maxIntegerValue )
        {
            integerValue = maxIntegerValue;
        }
    }

    if ( !stream.SerializeBits( integerValue, bits ) )
    {
        return false;
    }

    if ( Stream::IsReading )
    {
        if ( integerValue > maxIntegerValue )
        {
            return false;
        }
        const float normalizedValue = integerValue / float(maxIntegerValue);
        const float scaledValue = normalizedValue * delta;
        value = scaledValue + min;
    }

    return true;
}

// each helper drives the actual macro form under test, so the differential covers the macros as
// well as the internals (the compile time surface tests established this pattern)

template <typename Stream> bool serialize_compressed_float_legacy_form( Stream & stream, float & value, float min, float max, float res )
{
    serialize_compressed_float( stream, value, min, max, res );
    return true;
}

template <typename Stream> bool serialize_compressed_float_precomputed_form( Stream & stream, float & value, uint32_t max_integer_value, int bits, float delta, float min )
{
    serialize_compressed_float_precomputed( stream, value, max_integer_value, bits, delta, min );
    return true;
}

// ---------------------------------------------------------------------------------------
// THE NEGATIVE CONTROLS -- proof that this differential's eyes are open.
//
// Every check below compares implementations that are SUPPOSED to agree, and a test where
// everything agrees cannot distinguish "the split changed nothing" from "the comparison
// cannot see this class of change". The distinction is not hypothetical here: the whole FMA
// discipline in the audited home is a pair of stores through float locals, and whether
// DELETING them is visible depends entirely on the build. Measured on clang 21 / arm64 by
// folding the reader's two roundings back into one expression in
// serialize_compressed_float_precomputed_internal and rebuilding:
//
//     -ffp-contract=off    the falsified build PASSES  -- no teeth
//     -ffp-contract=on     the falsified build is RED  -- teeth
//     -ffp-contract=fast   the falsified build PASSES  -- no teeth
//
// The two "safe" settings are blind for opposite reasons: `off` forbids contraction in the
// frozen oracle too, so the fused spelling and the stored one compute the same thing;
// `fast` fuses ACROSS statements, so the frozen oracle fuses as well and the two forms stop
// being distinguishable. The mode that looks strictest has no teeth, and the mode this repo
// pins for the wire is the other one with no teeth. That is why CMakeLists.txt builds this
// suite a SECOND time at -ffp-contract=on: `off` is correct for the wire and toothless for
// this property, and the second build adds back the discrimination it costs.
//
// That covers the compiler. The two sentinels below cover the rest, and they do it on EVERY
// host and under EVERY flag: they are the same one-rounding perturbations spelled in
// DOUBLE, where no FMA hardware and no contraction setting is required to produce the
// divergence. Each runs alongside the real path over the whole corpus, and the differential
// FAILS if either ever stops diverging -- i.e. if a future edit ever leaves the wire bytes
// or the decoded bit patterns blind to a single-rounding quantization.
//
// This is the part that generalises, and it is the reason a control beats a contraction
// sweep: a sweep is something a person has to remember to run, and it proved this property
// exactly once. A control that fails when it stops diverging is permanent, and it does not
// depend on anyone choosing the right flag. It pays for itself immediately: at
// -ffp-contract=fast the read control drops to ZERO, because the compiler fuses the audited
// home into exactly the one-rounding sentinel -- so this suite now fails a `fast` build BY
// ITSELF, naming the reason, instead of passing green while blind.
//
// The sentinels are deliberately NOT compared against the frozen oracle for equality. They
// are supposed to disagree. That is the whole point. The divergence counts are printed, so
// the mass is visible and not just the verdict.
// ---------------------------------------------------------------------------------------

static uint64_t compressed_float_sentinel_write_divergences = 0;
static uint64_t compressed_float_sentinel_read_divergences = 0;

// Whether the frozen pre-split oracle computes conformant values in THIS build. The frozen
// reference is a verbatim copy of code that predates SERIALIZE_FLOAT_FORCE_ROUND, so under
// cross-statement contraction (-ffp-contract=fast, GCC's default) it fuses its two roundings
// into one while the barriered audited home does not. On such a build its comparisons assert
// the wrong thing and stand down; the sentinels above and the conformance vectors carry the
// teeth there. Set from the measured probe at the top of the differential -- detected, never
// inferred from flags.
static bool compressed_float_frozen_oracle_conformant = true;

// the writer's quantization with the two float roundings collapsed: the product and the
// +0.5 both taken in double, rounded once by the floor. the "widened to double"
// perturbation, permanently on watch.
inline uint32_t compressed_float_sentinel_write_code_one_rounding( float value, uint32_t max_integer_value, float delta, float min )
{
    float normalized = ( value - min ) / delta;
    if ( !( normalized >= 0.0f ) )
    {
        normalized = 0.0f;
    }
    else if ( !( normalized <= 1.0f ) )
    {
        normalized = 1.0f;
    }
    return (uint32_t) floor( (double) normalized * (double) max_integer_value + 0.5 );
}

// the reader's reconstruction with the multiply and the add collapsed into one rounding --
// exactly what an FMA contraction produces, spelled in double so it happens on hosts
// without an FMA instruction too.
inline float compressed_float_sentinel_read_value_one_rounding( uint32_t integer_value, uint32_t max_integer_value, float delta, float min )
{
    const float normalized = integer_value / float(max_integer_value);
    return (float) ( (double) normalized * (double) delta + (double) min );
}

static uint64_t compressed_float_differential_check_count = 0;

#define serialize_differential_check( condition )                           \
    do                                                                      \
    {                                                                       \
        serialize_check( condition );                                       \
        compressed_float_differential_check_count++;                        \
    } while (0)

// one written value through all three implementations: measured bits, wire bytes and decoded
// bit patterns must agree exactly
inline void check_compressed_float_value_agrees( float value, float min, float max, float res, uint32_t max_integer_value, int bits, float delta )
{
    // measure: all three forms agree on cost
    serialize::MeasureStream measureReference;
    float measure_value = value;
    serialize_differential_check( serialize_compressed_float_frozen_reference( measureReference, measure_value, min, max, res ) == true );
    serialize::MeasureStream measureLegacy;
    measure_value = value;
    serialize_differential_check( serialize_compressed_float_legacy_form( measureLegacy, measure_value, min, max, res ) == true );
    serialize::MeasureStream measurePrecomputed;
    measure_value = value;
    serialize_differential_check( serialize_compressed_float_precomputed_form( measurePrecomputed, measure_value, max_integer_value, bits, delta, min ) == true );
    serialize_differential_check( measureReference.GetBitsProcessed() == measureLegacy.GetBitsProcessed() );
    serialize_differential_check( measureReference.GetBitsProcessed() == measurePrecomputed.GetBitsProcessed() );

    // write: byte identical wire from all three
    uint8_t buffer_reference[8 + 8] = { 0 };            // + 8: read buffer allocations extend 8 bytes past the data
    uint8_t buffer_legacy[8 + 8] = { 0 };               // + 8: read buffer allocations extend 8 bytes past the data
    uint8_t buffer_precomputed[8 + 8] = { 0 };          // + 8: read buffer allocations extend 8 bytes past the data

    serialize::WriteStream writeReference( buffer_reference, 8 );
    float write_value = value;
    serialize_differential_check( serialize_compressed_float_frozen_reference( writeReference, write_value, min, max, res ) == true );
    writeReference.Flush();

    serialize::WriteStream writeLegacy( buffer_legacy, 8 );
    write_value = value;
    serialize_differential_check( serialize_compressed_float_legacy_form( writeLegacy, write_value, min, max, res ) == true );
    writeLegacy.Flush();

    serialize::WriteStream writePrecomputed( buffer_precomputed, 8 );
    write_value = value;
    serialize_differential_check( serialize_compressed_float_precomputed_form( writePrecomputed, write_value, max_integer_value, bits, delta, min ) == true );
    writePrecomputed.Flush();

    serialize_differential_check( writeReference.GetBitsProcessed() == writeLegacy.GetBitsProcessed() );
    serialize_differential_check( writeReference.GetBitsProcessed() == writePrecomputed.GetBitsProcessed() );
    serialize_differential_check( memcmp( buffer_legacy, buffer_precomputed, 8 ) == 0 );
    if ( compressed_float_frozen_oracle_conformant )
    {
        serialize_differential_check( memcmp( buffer_reference, buffer_legacy, 8 ) == 0 );
        serialize_differential_check( memcmp( buffer_reference, buffer_precomputed, 8 ) == 0 );
    }

    // read: decoded BIT PATTERNS agree exactly — one ulp of divergence must fail
    serialize::ReadStream readReference( buffer_precomputed, 8 );
    float decoded_reference = 0.0f;
    serialize_differential_check( serialize_compressed_float_frozen_reference( readReference, decoded_reference, min, max, res ) == true );
    serialize::ReadStream readLegacy( buffer_precomputed, 8 );
    float decoded_legacy = 0.0f;
    serialize_differential_check( serialize_compressed_float_legacy_form( readLegacy, decoded_legacy, min, max, res ) == true );
    serialize::ReadStream readPrecomputed( buffer_precomputed, 8 );
    float decoded_precomputed = 0.0f;
    serialize_differential_check( serialize_compressed_float_precomputed_form( readPrecomputed, decoded_precomputed, max_integer_value, bits, delta, min ) == true );

    uint32_t pattern_reference, pattern_legacy, pattern_precomputed;
    memcpy( &pattern_reference, &decoded_reference, 4 );
    memcpy( &pattern_legacy, &decoded_legacy, 4 );
    memcpy( &pattern_precomputed, &decoded_precomputed, 4 );
    serialize_differential_check( pattern_legacy == pattern_precomputed );
    if ( compressed_float_frozen_oracle_conformant )
    {
        serialize_differential_check( pattern_reference == pattern_legacy );
        serialize_differential_check( pattern_reference == pattern_precomputed );
    }

    // the write-side negative control: the one-rounding quantization must still be able to
    // produce a DIFFERENT wire code than the audited home does, or the byte comparison above
    // has gone blind. The code is read back off the audited home's own wire rather than
    // recomputed, so the control is measured against the bytes the SHIPPED path actually
    // made — which keeps its teeth on every build, including one whose frozen oracle has
    // stood down.
    {
        serialize::ReadStream sentinelStream( buffer_precomputed, 8 );
        uint32_t wire_code = 0;
        if ( sentinelStream.SerializeBits( wire_code, bits ) )
        {
            if ( compressed_float_sentinel_write_code_one_rounding( value, max_integer_value, delta, min ) != wire_code )
            {
                compressed_float_sentinel_write_divergences++;
            }
        }
    }
}

// one wire integer through all three read paths: acceptance must agree (the headroom refusal),
// and accepted codes must decode to identical bit patterns
inline void check_compressed_float_code_agrees( uint32_t code, float min, float max, float res, uint32_t max_integer_value, int bits, float delta )
{
    uint8_t buffer[8 + 8] = { 0 };              // + 8: read buffer allocations extend 8 bytes past the data
    serialize::WriteStream writeStream( buffer, 8 );
    uint32_t raw = code;
    serialize_differential_check( writeStream.SerializeBits( raw, bits ) == true );
    writeStream.Flush();

    serialize::ReadStream readReference( buffer, 8 );
    float decoded_reference = 0.0f;
    const bool ok_reference = serialize_compressed_float_frozen_reference( readReference, decoded_reference, min, max, res );

    serialize::ReadStream readLegacy( buffer, 8 );
    float decoded_legacy = 0.0f;
    const bool ok_legacy = serialize_compressed_float_legacy_form( readLegacy, decoded_legacy, min, max, res );

    serialize::ReadStream readPrecomputed( buffer, 8 );
    float decoded_precomputed = 0.0f;
    const bool ok_precomputed = serialize_compressed_float_precomputed_form( readPrecomputed, decoded_precomputed, max_integer_value, bits, delta, min );

    serialize_differential_check( ok_reference == ok_legacy );
    serialize_differential_check( ok_reference == ok_precomputed );
    serialize_differential_check( ok_reference == ( code <= max_integer_value ) );      // the headroom refusal itself

    if ( ok_reference )
    {
        uint32_t pattern_reference, pattern_legacy, pattern_precomputed;
        memcpy( &pattern_reference, &decoded_reference, 4 );
        memcpy( &pattern_legacy, &decoded_legacy, 4 );
        memcpy( &pattern_precomputed, &decoded_precomputed, 4 );
        serialize_differential_check( pattern_legacy == pattern_precomputed );
        if ( compressed_float_frozen_oracle_conformant )
        {
            serialize_differential_check( pattern_reference == pattern_legacy );
            serialize_differential_check( pattern_reference == pattern_precomputed );
        }

        // the read-side negative control: a one-rounding reconstruction must still be able
        // to land on a DIFFERENT bit pattern, or the pattern comparison above has gone
        // blind. The divergence is one ulp, which is exactly why this differential never
        // compares decoded values with a tolerance. Measured against the SHIPPED path's
        // decode, so it keeps its teeth on every build: if the audited home's two roundings
        // are ever fused -- a deleted barrier under -ffp-contract=fast is exactly this --
        // the home decodes what the sentinel decodes, the divergences drop to zero, and the
        // nonzero assertion below the corpus fails by name.
        const float sentinel_value = compressed_float_sentinel_read_value_one_rounding( code, max_integer_value, delta, min );
        uint32_t pattern_sentinel;
        memcpy( &pattern_sentinel, &sentinel_value, 4 );
        if ( pattern_sentinel != pattern_precomputed )
        {
            compressed_float_sentinel_read_divergences++;
        }
    }
}

struct CompressedFloatShape
{
    float min;
    float max;
    float res;
    uint32_t expected_max_integer_value;        // pinned: the constants a schema compiler emits for this declaration
    int expected_bits;                          // (the first eleven rows are the values the schema PR #79 differential published)
};

static const CompressedFloatShape compressed_float_shapes[] =
{
    // the schema compiler's corpus: examples, bench/corpus/RealWorld.schema and its test data
    { 0.0f,       2000.0f,        0.1f,       20000,       15 },
    { -2.0f,      2.0f,           0.25f,      16,          5  },
    { -90.0f,     90.0f,          0.5f,       360,         9  },
    { 0.0f,       30.0f,          0.5f,       60,          6  },
    { -100.0f,    100.0f,         0.25f,      800,         10 },
    { 0.0f,       2000.0f,        1.0f,       2000,        11 },
    { 0.0f,       10.0f,          0.02f,      500,         9  },
    { 0.0f,       100.0f,         0.01f,      10000,       14 },
    { -180.0f,    180.0f,         0.01f,      36000,       16 },
    { 0.0f,       10.0f,          0.01f,      1000,        10 },       // also this repo's golden wire declaration
    { -5.0f,      5.0f,           0.001f,     10000,       14 },
    // this repo's own declarations
    { -100.0f,    100.0f,         0.01f,      20000,       15 },       // the non-zero-min conformance vector
    { -10.0f,     10.0f,          0.01f,      2000,        11 },       // the fuzz harness declaration
    { -1.0f,      1.0f,           0.001f,     2000,        11 },       // example.cpp's orientation declaration
    // shapes at the edges of the derivation itself
    { 0.0f,       1.0f,           2.0f,       1,           1  },       // resolution coarser than the range: values clamps up to 1
    { 0.0f,       15.0f,          1.0f,       15,          4  },       // step count exactly fills the wire width: no headroom to refuse
    { 0.0f,       1000000.0f,     1.0f,       1000000,     20 },       // a million steps
    { 0.0f,       10000000000.0f, 1.0f,       4294967040u, 32 },       // values clamps down to the largest float below 2^32
    // shapes that discriminate the rounding rule itself: a fractional step count BELOW the half step,
    // where ceil and round disagree. Every row above lands on an integer or within half a step of one,
    // so all of them derive the same constants under either rule -- the corpus could not see a swap.
    { 0.0f,       10.0f,          0.3f,       34,          6  },       // 33.333332 steps: ceil 34, round 33 -- same width, different step count
    { 0.0f,       63.3f,          1.0f,       64,          7  },       // 63.3 steps: ceil 64 (7 bits), round 63 (6 bits) -- straddles a power of two, so the WIRE WIDTH moves
    // shapes in [2^23, 2^24), where the float32 ulp reaches 1 and the +0.5 rounding could
    // push the code past max_integer_value before the normative clamp (2026-08-23,
    // schema#109). The corpus was empty in this band, which is how the defect hid.
    { 0.0f,       8388609.0f,     1.0f,       8388609u,    24 },       // 2^23+1: the reader-rejects witness
    { 0.0f,       16777215.0f,    1.0f,       16777215u,   24 },       // 2^24-1: the wire-divergence witness
};

inline void test_compressed_float_top_of_range_clamp()
{
    // STANDARD.md's normative integer clamp (2026-08-23, schema#109; ruling: Glenn, live).
    // In [2^23, 2^24) the float32 ulp is 1, so scaled + 0.5f lands on a tie and
    // round-to-even can push the code past max_integer_value. Before the clamp,
    // witness A wrote a top-of-range code its own reader rejected, and witness B
    // wrote a code one bit wider than the field.
    {
        // witness A: [0, 8388609] at resolution 1 -> max_integer_value 2^23+1, 24 bits
        uint8_t buffer[16 + 8] = { 0 };
        serialize::WriteStream writeStream( buffer, 16 );
        float written = 8388609.0f;
        serialize_check( serialize::serialize_compressed_float_internal( writeStream, written, 0.0f, 8388609.0f, 1.0f ) == true );
        writeStream.Flush();
        serialize::ReadStream readStream( buffer, 16 );
        float value = 0.0f;
        serialize_check( serialize::serialize_compressed_float_internal( readStream, value, 0.0f, 8388609.0f, 1.0f ) == true );
        serialize_check( value == 8388609.0f );
    }
    {
        // witness B: [0, 16777215] at resolution 1 -> max_integer_value 2^24-1, 24 bits;
        // the unclamped code was 2^24, one bit wider than the field
        uint8_t buffer[16 + 8] = { 0 };
        serialize::WriteStream writeStream( buffer, 16 );
        float written = 16777215.0f;
        serialize_check( serialize::serialize_compressed_float_internal( writeStream, written, 0.0f, 16777215.0f, 1.0f ) == true );
        writeStream.Flush();
        serialize::ReadStream readStream( buffer, 16 );
        float value = 0.0f;
        serialize_check( serialize::serialize_compressed_float_internal( readStream, value, 0.0f, 16777215.0f, 1.0f ) == true );
        serialize_check( value == 16777215.0f );
    }
}

inline void test_compressed_float_precomputed_differential()
{
    // the frozen oracle predates the barrier: under cross-statement contraction it fuses,
    // so its comparisons stand down on such a build (see the declaration above)
    compressed_float_frozen_oracle_conformant = !serialize_test_fp_contraction_crosses_statements();

    const float float_max = 3.402823466e+38f;               // FLT_MAX, spelled so the header needs no float.h

    uint64_t lcg = 0xC0FFEE1234567890ULL;                   // fixed seed: failures reproduce

    #define serialize_differential_next_lcg() ( lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL )

    const int num_shapes = (int) ( sizeof( compressed_float_shapes ) / sizeof( compressed_float_shapes[0] ) );

    for ( int s = 0; s < num_shapes; s++ )
    {
        const float min = compressed_float_shapes[s].min;
        const float max = compressed_float_shapes[s].max;
        const float res = compressed_float_shapes[s].res;

        uint32_t max_integer_value = 0;
        int bits = 0;
        float delta = 0.0f;
        serialize::serialize_compressed_float_params( min, max, res, max_integer_value, bits, delta );

        // the derived constants are pinned against the schema compiler's generation-time table
        serialize_differential_check( max_integer_value == compressed_float_shapes[s].expected_max_integer_value );
        serialize_differential_check( bits == compressed_float_shapes[s].expected_bits );
        serialize_differential_check( delta == max - min );

        const double dmin = (double) min;
        const double ddelta = (double) delta;

        // dense sweep with overshoot a quarter of the range past both bounds
        {
            const int sweep_steps = 2048;
            const double lo = dmin - 0.25 * ddelta;
            const double span = 1.5 * ddelta;
            for ( int i = 0; i <= sweep_steps; i++ )
            {
                const float value = (float) ( lo + span * i / sweep_steps );
                check_compressed_float_value_agrees( value, min, max, res, max_integer_value, bits, delta );
            }
        }

        // quantization step edges and midpoints, with their one-ulp neighbors. the midpoints
        // are the discriminating band: 0.005 over [0,10] at 0.01 quantizes to 1 under the
        // required two roundings and to 0 under a fused or widened writer (STANDARD.md)
        {
            const uint32_t stride = max_integer_value / 512 + 1;
            for ( uint64_t k = 0; k <= max_integer_value; k += stride )      // 64 bit: k += stride must not wrap at the 2^32-clamped shape
            {
                const float on_quantum = (float) ( dmin + ddelta * ( (double) k / (double) max_integer_value ) );
                const float midpoint = (float) ( dmin + ddelta * ( ( (double) k + 0.5 ) / (double) max_integer_value ) );
                check_compressed_float_value_agrees( on_quantum, min, max, res, max_integer_value, bits, delta );
                check_compressed_float_value_agrees( nextafterf( on_quantum, -float_max ), min, max, res, max_integer_value, bits, delta );
                check_compressed_float_value_agrees( nextafterf( on_quantum, +float_max ), min, max, res, max_integer_value, bits, delta );
                check_compressed_float_value_agrees( midpoint, min, max, res, max_integer_value, bits, delta );
                check_compressed_float_value_agrees( nextafterf( midpoint, -float_max ), min, max, res, max_integer_value, bits, delta );
                check_compressed_float_value_agrees( nextafterf( midpoint, +float_max ), min, max, res, max_integer_value, bits, delta );
            }
        }

        // specials: the bounds and their one-ulp neighbors, both zeros, subnormals, extremes
        {
            const float specials[] =
            {
                min,
                max,
                nextafterf( min, -float_max ),
                nextafterf( min, +float_max ),
                nextafterf( max, -float_max ),
                nextafterf( max, +float_max ),
                min - res,
                max + res,
                min + res * 0.5f,
                max - res * 0.5f,
                min - delta,
                max + delta,
                0.0f,
                -0.0f,
                res,
                -res,
                float_max,
                -float_max,
                1.175494351e-38f,               // FLT_MIN
                -1.175494351e-38f,
                1.401298464e-45f,               // the smallest subnormal
                -1.401298464e-45f,
                1.0e30f,
                -1.0e30f,
            };
            for ( int i = 0; i < (int) ( sizeof( specials ) / sizeof( specials[0] ) ); i++ )
            {
                check_compressed_float_value_agrees( specials[i], min, max, res, max_integer_value, bits, delta );
            }
        }

#if defined( NDEBUG )
        // non-finite inputs are non-conforming and assert in debug (proven by the fork tests),
        // so the release build is where the differential can drive them: the clamp must force
        // NaN and both infinities to the same wire in all three implementations
        {
            const uint32_t non_finite_patterns[] = { 0x7F800000u, 0xFF800000u, 0x7FC00000u, 0x7F800001u, 0xFFC00001u };
            for ( int i = 0; i < (int) ( sizeof( non_finite_patterns ) / sizeof( non_finite_patterns[0] ) ); i++ )
            {
                float value = 0.0f;
                memcpy( &value, &non_finite_patterns[i], 4 );
                check_compressed_float_value_agrees( value, min, max, res, max_integer_value, bits, delta );
            }
        }
#endif // #if defined( NDEBUG )

        // LCG uniform values across the range and its overshoot band
        {
            for ( int i = 0; i < 2048; i++ )
            {
                const double fraction = (double) ( serialize_differential_next_lcg() >> 11 ) * ( 1.0 / 9007199254740992.0 );     // [0,1) in 53 bits
                const float value = (float) ( dmin - 0.25 * ddelta + fraction * 1.5 * ddelta );
                check_compressed_float_value_agrees( value, min, max, res, max_integer_value, bits, delta );
            }
        }

        // LCG uniform float32 bit patterns, finite ones (non-finite writes assert in debug;
        // the release-only block above drives those deliberately)
        {
            for ( int i = 0; i < 2048; i++ )
            {
                const uint32_t pattern = (uint32_t) ( serialize_differential_next_lcg() >> 32 );
                float value = 0.0f;
                memcpy( &value, &pattern, 4 );
                if ( value - value == 0.0f )
                {
                    check_compressed_float_value_agrees( value, min, max, res, max_integer_value, bits, delta );
                }
            }
        }

        // the read side: every representable wire integer, including the bit headroom a
        // malicious packet can encode into. exhaustive up to 16-bit widths; above that the
        // boundary codes are pinned and the interior is sampled
        {
            const uint32_t top_code = ( bits == 32 ) ? 0xFFFFFFFFu : ( ( 1u << bits ) - 1u );
            if ( bits <= 16 )
            {
                for ( uint32_t code = 0; code <= top_code; code++ )
                {
                    check_compressed_float_code_agrees( code, min, max, res, max_integer_value, bits, delta );
                }
            }
            else
            {
                for ( uint32_t code = 0; code <= 1024; code++ )
                {
                    check_compressed_float_code_agrees( code, min, max, res, max_integer_value, bits, delta );
                }
                const uint32_t window_lo = max_integer_value - 512;                 // max_integer_value >= 2^16 here, so no underflow
                const uint32_t window_hi = ( top_code - max_integer_value < 512 ) ? top_code : max_integer_value + 512;
                for ( uint32_t code = window_lo; code <= window_hi && code >= window_lo; code++ )
                {
                    check_compressed_float_code_agrees( code, min, max, res, max_integer_value, bits, delta );
                }
                for ( uint32_t code = top_code - 64; code <= top_code && code >= top_code - 64; code++ )
                {
                    check_compressed_float_code_agrees( code, min, max, res, max_integer_value, bits, delta );
                }
                for ( int i = 0; i < 2048; i++ )
                {
                    const uint32_t code = ( (uint32_t) ( serialize_differential_next_lcg() >> 32 ) ) & top_code;
                    check_compressed_float_code_agrees( code, min, max, res, max_integer_value, bits, delta );
                }
            }
        }
    }

    #undef serialize_differential_next_lcg

    // the coverage floor: if the differential ever silently shrinks below the mass it was
    // built with, that is a test bug, and it fails here instead of fading quietly. A build
    // whose frozen oracle stood down runs fewer comparisons, so it is held to its own
    // measured floor rather than the full one.
    serialize_check( compressed_float_differential_check_count >= ( compressed_float_frozen_oracle_conformant ? 2000000 : 1600000 ) );

    if ( serialize_test_verbose() )
    {
        printf( "    (%llu checks, three implementations, %d declarations)\n",
                (unsigned long long) compressed_float_differential_check_count, num_shapes );
    }

    // the negative controls, CHECKED rather than merely reported: if a one-rounding writer
    // ever stops producing a different wire code, or a one-rounding reader ever stops
    // producing a different bit pattern, then everything above agrees for a reason that has
    // nothing to do with the split being correct, and this differential is decoration.
    serialize_check( compressed_float_sentinel_write_divergences > 0 );
    serialize_check( compressed_float_sentinel_read_divergences > 0 );

    if ( serialize_test_verbose() )
    {
        printf( "    (negative controls diverge on %llu wire codes and %llu decoded patterns -- both must be nonzero, or the comparison cannot see a single-rounding quantization)\n",
                (unsigned long long) compressed_float_sentinel_write_divergences,
                (unsigned long long) compressed_float_sentinel_read_divergences );
    }
}

#undef serialize_differential_check

// Conformance vector for float/double BIT TRANSPARENCY (STANDARD.md, "Floating Point",
// ratified 2026-08-15 from the #56 re-audit). Additive: golden_wire_bytes is untouched.
//
// The golden's float is 3.1415926f and its double 1.0/3.0 — ordinary values that any
// sanitizing implementation reproduces — and the document-side checker compares them by
// TOLERANCE, which is structurally unable to see a canonicalized NaN payload, a quieted
// signaling bit, or a sign-of-zero flip. These patterns are the ones a sanitizer breaks,
// constructed by bit-cast (never by literal) and compared by bits (never by value: NaN
// compares unequal to itself and -0.0 == 0.0, so value comparison proves nothing here).

static const uint8_t golden_float_bytes[] =
{
    0x01, 0x00, 0xC0, 0x7F,                             // f32 0x7FC00001: quiet NaN, payload 1
    0x01, 0x00, 0x80, 0x7F,                             // f32 0x7F800001: SIGNALING NaN
    0x00, 0x00, 0x80, 0xFF,                             // f32 0xFF800000: -Inf
    0x00, 0x00, 0x00, 0x80,                             // f32 0x80000000: -0.0
    0x01, 0x00, 0x00, 0x00,                             // f32 0x00000001: smallest denormal
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF4, 0x7F,     // f64 0x7FF4000000000001: signaling NaN, payload 1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80      // f64 0x8000000000000000: -0.0
};

static const uint32_t golden_float_patterns[5] = { 0x7FC00001u, 0x7F800001u, 0xFF800000u, 0x80000000u, 0x00000001u };
static const uint64_t golden_double_patterns[2] = { 0x7FF4000000000001ULL, 0x8000000000000000ULL };

template <typename Stream> bool GoldenFloatSerialize( Stream & stream, float * float_values, double * double_values )
{
    for ( int i = 0; i < 5; i++ )
    {
        serialize_float( stream, float_values[i] );
    }
    for ( int i = 0; i < 2; i++ )
    {
        serialize_double( stream, double_values[i] );
    }
    return true;
}

inline void test_golden_float_bit_transparency()
{
    // write side: the bit patterns above, bit-cast into float/double and serialized, must
    // produce exactly the pinned little-endian bytes — no quieting, no canonicalization
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        float float_values[5];
        double double_values[2];
        for ( int i = 0; i < 5; i++ )
            memcpy( &float_values[i], &golden_float_patterns[i], 4 );
        for ( int i = 0; i < 2; i++ )
            memcpy( &double_values[i], &golden_double_patterns[i], 8 );
        serialize_check( GoldenFloatSerialize( stream, float_values, double_values ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( golden_float_bytes ) );
        serialize_check( memcmp( buffer, golden_float_bytes, sizeof( golden_float_bytes ) ) == 0 );
    }

    // read side: the recovered BIT PATTERNS must equal the transmitted ones exactly.
    // a tolerance-based harness passes a sanitizing reader forever — which is the point
    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, golden_float_bytes, sizeof( golden_float_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( golden_float_bytes ) );
        float float_values[5];
        double double_values[2];
        memset( float_values, 0, sizeof( float_values ) );
        memset( double_values, 0, sizeof( double_values ) );
        serialize_check( GoldenFloatSerialize( stream, float_values, double_values ) == true );
        for ( int i = 0; i < 5; i++ )
        {
            uint32_t bits = 0;
            memcpy( &bits, &float_values[i], 4 );
            serialize_check( bits == golden_float_patterns[i] );
        }
        for ( int i = 0; i < 2; i++ )
        {
            uint64_t bits = 0;
            memcpy( &bits, &double_values[i], 8 );
            serialize_check( bits == golden_double_patterns[i] );
        }
    }
}

// Conformance vectors for the zero-count and unaligned bytes paths (STANDARD.md, "bytes —
// count may be zero", ratified 2026-08-15 from the #56 re-audit). Additive pins. Each one
// discriminates: the wrong-but-plausible implementation produces DIFFERENT bytes, spelled
// out per vector, so a port carrying the divergence fails the pin instead of agreeing with
// itself forever — round-trip self-tests cannot catch a skipped align, because both halves
// skip the same one.

template <typename Stream> bool ZeroLengthBytesSerialize( Stream & stream, uint32_t & head, uint8_t * data, uint32_t & tail )
{
    serialize_bits( stream, head, 3 );
    serialize_bytes( stream, data, 0 );
    serialize_bits( stream, tail, 8 );
    return true;
}

inline void test_golden_zero_length_bytes()
{
    // { bits(5,3); bytes(count=0); bits(0xA5,8) }. The zero-length bytes still ALIGNS:
    // the pad lands in bits [3,8) and 0xA5 occupies byte 1. A port that early-returns on
    // count == 0 writes { 0x2D, 0x05 } — every later field shifted by five bits.
    static const uint8_t pinned_bytes[2] = { 0x05, 0xA5 };

    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        uint32_t head = 5;
        uint32_t tail = 0xA5;
        uint8_t data[1] = { 0 };
        serialize_check( ZeroLengthBytesSerialize( stream, head, data, tail ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
        serialize_check( memcmp( buffer, pinned_bytes, sizeof( pinned_bytes ) ) == 0 );
    }

    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, pinned_bytes, sizeof( pinned_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( pinned_bytes ) );
        uint32_t head = 0;
        uint32_t tail = 0;
        uint8_t data[1] = { 0 };
        serialize_check( ZeroLengthBytesSerialize( stream, head, data, tail ) == true );
        serialize_check( head == 5 );
        serialize_check( tail == 0xA5 );
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
    }
}

template <typename Stream> bool ZeroLengthStringSerialize( Stream & stream, uint32_t & head, char * string, uint32_t & tail )
{
    serialize_bits( stream, head, 3 );
    serialize_string( stream, string, 8 );
    serialize_bits( stream, tail, 8 );
    return true;
}

inline void test_golden_zero_length_string()
{
    // { bits(5,3); string("", buffer_size 8); bits(0xA5,8) }. The empty string is a 3-bit
    // length of 0, then the zero-length payload's align pads bits [6,8). A port whose
    // STRING path skips the empty-payload align writes { 0x45, 0x29 } — the exact hazard
    // the C port's comment names, and a separate code path from bytes in three ports.
    static const uint8_t pinned_bytes[2] = { 0x05, 0xA5 };

    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        uint32_t head = 5;
        uint32_t tail = 0xA5;
        char string[8] = { 0 };
        serialize_check( ZeroLengthStringSerialize( stream, head, string, tail ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
        serialize_check( memcmp( buffer, pinned_bytes, sizeof( pinned_bytes ) ) == 0 );
    }

    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, pinned_bytes, sizeof( pinned_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( pinned_bytes ) );
        uint32_t head = 0;
        uint32_t tail = 0;
        char string[8];
        memset( string, 0xFF, sizeof( string ) );
        serialize_check( ZeroLengthStringSerialize( stream, head, string, tail ) == true );
        serialize_check( head == 5 );
        serialize_check( string[0] == '\0' );
        serialize_check( tail == 0xA5 );
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
    }
}

template <typename Stream> bool UnalignedBytesSerialize( Stream & stream, uint32_t & head, uint8_t * data, uint32_t & tail )
{
    serialize_bits( stream, head, 1 );
    serialize_bytes( stream, data, 2 );
    serialize_bits( stream, tail, 4 );
    return true;
}

inline void test_golden_unaligned_bytes()
{
    // { bits(1,1); bytes({0xEF,0xBE}, 2); bits(0x0F,4) }. Exercises serialize_bytes' OWN
    // align from bit index 1 — the case the golden vector structurally shadows, because an
    // explicit serialize_align immediately precedes its bytes field, making the operation's
    // internal align a no-op there.
    static const uint8_t pinned_bytes[4] = { 0x01, 0xEF, 0xBE, 0x0F };

    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream stream( buffer, (int) sizeof( buffer ) );
        uint32_t head = 1;
        uint32_t tail = 0x0F;
        uint8_t data[2] = { 0xEF, 0xBE };
        serialize_check( UnalignedBytesSerialize( stream, head, data, tail ) == true );
        stream.Flush();
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
        serialize_check( memcmp( buffer, pinned_bytes, sizeof( pinned_bytes ) ) == 0 );
    }

    {
        uint8_t buffer[64];
        memset( buffer, 0, sizeof( buffer ) );
        memcpy( buffer, pinned_bytes, sizeof( pinned_bytes ) );
        serialize::ReadStream stream( buffer, (int) sizeof( pinned_bytes ) );
        uint32_t head = 0;
        uint32_t tail = 0;
        uint8_t data[2] = { 0, 0 };
        serialize_check( UnalignedBytesSerialize( stream, head, data, tail ) == true );
        serialize_check( head == 1 );
        serialize_check( data[0] == 0xEF );
        serialize_check( data[1] == 0xBE );
        serialize_check( tail == 0x0F );
        serialize_check( stream.GetBytesProcessed() == (int) sizeof( pinned_bytes ) );
    }
}

inline void test_measure_bound()
{
    // STANDARD.md, "The Measure Stream" (adopted 2026-08-15): a measure reports a size
    // sufficient at ANY starting bit position — a bound, not the packet size — and the
    // expected implementation charges worst-case 7 bits per alignment-performing
    // operation. Exact-from-zero accounting is non-conforming: it under-counts every
    // unaligned start.

    // the ruling's worked example: { bits(8); align; bits(8) }
    {
        serialize::MeasureStream measureStream;
        uint32_t byte_value = 0xAB;
        serialize_check( measureStream.SerializeBits( byte_value, 8 ) == true );
        serialize_check( measureStream.SerializeAlign() == true );
        serialize_check( measureStream.SerializeBits( byte_value, 8 ) == true );
        serialize_check( measureStream.GetBitsProcessed() == 23 );      // 8 + 7 + 8: the conservative charge.
                                                                        // an exact-from-zero measure reports 16 —
                                                                        // 2 bytes — which is NOT enough room when
                                                                        // the message lands at bit offset 1

        // written at every starting offset, the message's actual span never exceeds the
        // measure — the property the bound exists to guarantee
        for ( int offset = 0; offset < 8; offset++ )
        {
            uint8_t buffer[64];
            memset( buffer, 0, sizeof( buffer ) );
            serialize::WriteStream writeStream( buffer, (int) sizeof( buffer ) );
            for ( int i = 0; i < offset; i++ )
            {
                uint32_t one_bit = 1;
                serialize_check( writeStream.SerializeBits( one_bit, 1 ) == true );
            }
            const int64_t start = writeStream.GetBitsProcessed();
            serialize_check( writeStream.SerializeBits( byte_value, 8 ) == true );
            serialize_check( writeStream.SerializeAlign() == true );
            serialize_check( writeStream.SerializeBits( byte_value, 8 ) == true );
            const int64_t span = writeStream.GetBitsProcessed() - start;
            serialize_check( span <= measureStream.GetBitsProcessed() );
            if ( offset == 0 )
                serialize_check( span == 16 );                          // 2 bytes from an aligned start...
            if ( offset == 1 )
                serialize_check( span == 23 );                          // ...3 bytes of room needed from offset 1
        }
    }

    // measure >= written, across every message this suite pins. (The fuzz harness holds
    // the same inequality over arbitrary op programs on every run.)
    {
        GoldenWireData data;
        GoldenWireInit( data );
        serialize::MeasureStream measureStream;
        serialize_check( GoldenWireSerialize( measureStream, data ) == true );
        uint8_t buffer[256];
        memset( buffer, 0, sizeof( buffer ) );
        serialize::WriteStream writeStream( buffer, (int) sizeof( buffer ) );
        serialize_check( GoldenWireSerialize( writeStream, data ) == true );
        writeStream.Flush();
        serialize_check( measureStream.GetBitsProcessed() >= writeStream.GetBitsProcessed() );
    }

    {
        float float_values[5];
        double double_values[2];
        for ( int i = 0; i < 5; i++ )
            memcpy( &float_values[i], &golden_float_patterns[i], 4 );
        for ( int i = 0; i < 2; i++ )
            memcpy( &double_values[i], &golden_double_patterns[i], 8 );
        serialize::MeasureStream measureStream;
        serialize_check( GoldenFloatSerialize( measureStream, float_values, double_values ) == true );
        serialize_check( measureStream.GetBitsProcessed() >= 5 * 32 + 2 * 64 );     // no aligns: exact, and still a bound
    }

    {
        uint32_t head = 5;
        uint32_t tail = 0xA5;
        uint8_t data[1] = { 0 };
        serialize::MeasureStream measureStream;
        serialize_check( ZeroLengthBytesSerialize( measureStream, head, data, tail ) == true );
        serialize_check( measureStream.GetBitsProcessed() >= 16 );                  // 3 + pad + 8 written; measure charges 3 + 7 + 8
    }

    {
        uint32_t head = 5;
        uint32_t tail = 0xA5;
        char string[8] = { 0 };
        serialize::MeasureStream measureStream;
        serialize_check( ZeroLengthStringSerialize( measureStream, head, string, tail ) == true );
        serialize_check( measureStream.GetBitsProcessed() >= 16 );
    }

    {
        uint32_t head = 1;
        uint32_t tail = 0x0F;
        uint8_t data[2] = { 0xEF, 0xBE };
        serialize::MeasureStream measureStream;
        serialize_check( UnalignedBytesSerialize( measureStream, head, data, tail ) == true );
        serialize_check( measureStream.GetBitsProcessed() >= 28 );                  // 28 bits written; measure charges 1 + 7 + 16 + 4
    }
}

inline void test_unaligned_writer()
{
    // the bit writer stores each dword with memcpy, so the write buffer does not need 4 byte alignment.
    // exercise every offset within a dword, covering the WriteBits, WriteBytes and FlushBits store paths.

    uint32_t storage_words[( 256 + 4 ) / 4];                    // uint32_t backing guarantees 4 byte alignment without requiring C++11 alignas
    uint8_t * storage = (uint8_t*) storage_words;

    for ( int offset = 0; offset < 4; offset++ )
    {
        memset( storage, 0, sizeof( storage_words ) );

        uint8_t * buffer = storage + offset;

        uint8_t data[13];
        for ( int i = 0; i < (int) sizeof( data ); i++ )
            data[i] = (uint8_t) ( i * 47 + offset );

        serialize::WriteStream writeStream( buffer, 256 );
        writeStream.SerializeBits( 0x12345678, 32 );
        writeStream.SerializeBits( 123, 7 );
        writeStream.SerializeBytes( data, (int) sizeof( data ) );
        writeStream.SerializeBits( 0xDEADBEEF, 32 );
        writeStream.Flush();

        const int bytesWritten = writeStream.GetBytesProcessed();

        serialize::ReadStream readStream( buffer, bytesWritten );
        uint32_t a = 0;
        serialize_check( readStream.SerializeBits( a, 32 ) == true );
        serialize_check( a == 0x12345678 );
        uint32_t b = 0;
        serialize_check( readStream.SerializeBits( b, 7 ) == true );
        serialize_check( b == 123 );
        uint8_t read_data[13];
        memset( read_data, 0, sizeof( read_data ) );
        serialize_check( readStream.SerializeBytes( read_data, (int) sizeof( read_data ) ) == true );
        serialize_check( memcmp( read_data, data, sizeof( data ) ) == 0 );
        uint32_t c = 0;
        serialize_check( readStream.SerializeBits( c, 32 ) == true );
        serialize_check( c == 0xDEADBEEF );
    }
}

inline void test_large_buffer()
{
    // bit counts are 64 bit, so buffers larger than the old 256 MB limit work. write a bulk
    // block that carries the stream past the 2^31 bit boundary (256 MB), then verify that
    // bitpacked values round trip on the far side of it.

    const int64_t bufferSize = int64_t( 320 ) * 1024 * 1024;
    uint8_t * buffer = (uint8_t*) malloc( (size_t) bufferSize + 8 );        // + 8: read buffer allocations extend 8 bytes past the data
    if ( !buffer )
    {
        if ( serialize_test_verbose() )
        {
            printf( "(skipped test_large_buffer: could not allocate the buffer)\n" );
        }
        return;
    }

    static uint8_t chunk[1024*1024];
    for ( int i = 0; i < (int) sizeof( chunk ); i++ )
        chunk[i] = (uint8_t) ( i * 37 );

    const int numChunks = 300;                                              // 300 MB of bulk data: past the 256 MB boundary

    int64_t bytesWritten = 0;
    {
        serialize::WriteStream writeStream( buffer, bufferSize );
        for ( int i = 0; i < numChunks; i++ )
            serialize_check( writeStream.SerializeBytes( chunk, sizeof( chunk ) ) == true );
        uint32_t sentinel = 0xDEADBEEF;
        serialize_check( writeStream.SerializeBits( sentinel, 32 ) == true );
        int32_t value = -12345;
        serialize_check( writeStream.SerializeInteger( value, -100000, +100000 ) == true );
        writeStream.Flush();
        bytesWritten = writeStream.GetBytesProcessed();
        serialize_check( writeStream.GetBitsProcessed() > int64_t( 1 ) << 31 );     // the bit count really did cross the old 32 bit boundary
    }

    {
        serialize::ReadStream readStream( buffer, bytesWritten );
        static uint8_t readChunk[1024*1024];
        for ( int i = 0; i < numChunks; i++ )
            serialize_check( readStream.SerializeBytes( readChunk, sizeof( readChunk ) ) == true );
        serialize_check( memcmp( readChunk, chunk, sizeof( chunk ) ) == 0 );        // the final chunk, decoded from past the boundary
        uint32_t sentinel = 0;
        serialize_check( readStream.SerializeBits( sentinel, 32 ) == true );
        serialize_check( sentinel == 0xDEADBEEF );
        int32_t value = 0;
        serialize_check( readStream.SerializeInteger( value, -100000, +100000 ) == true );
        serialize_check( value == -12345 );
        serialize_check( readStream.GetBitsProcessed() > int64_t( 1 ) << 31 );
    }

    free( buffer );
}

#define SERIALIZE_RUN_TEST( test_function )                                 \
    do                                                                      \
    {                                                                       \
        printf( #test_function "\n" );                                      \
        test_function();                                                    \
    }                                                                       \
    while (0)

inline void serialize_test()
{
    const bool fp_contraction_live = serialize_test_fp_contraction_is_live();
    const bool fp_contraction_crosses_statements = serialize_test_fp_contraction_crosses_statements();

#if defined( SERIALIZE_TEST_FP_CONTRACT_REQUESTED_ON )
    // This binary is CMake's second build of the suite, and it asked for STATEMENT-LOCAL
    // contraction. GCC before 14 mapped -ffp-contract=on onto =fast and gives cross-statement
    // contraction instead, which is a different and unsupported thing. That is a fact about
    // the toolchain, not a defect in the library, so this build stands down rather than
    // reporting a failure it did not find. The -ffp-contract=off build is still the gate on
    // such a compiler, and its negative controls still carry the discrimination there.
    // Anyone transplanting the -ffp-contract=on build to another repo should carry this check.
    if ( fp_contraction_crosses_statements )
    {
        printf( "*** STANDING DOWN: asked for -ffp-contract=on and this compiler produced CROSS-STATEMENT contraction (GCC before 14 maps =on onto =fast), so the discriminating build is not available on this toolchain ***\n\n" );
        return;
    }
#endif // #if defined( SERIALIZE_TEST_FP_CONTRACT_REQUESTED_ON )

    // which build this is, and whether its contraction setting is buying anything on this
    // target. Reported under SERIALIZE_TEST_VERBOSE, never asserted: a host with no FMA
    // instruction cannot fuse however the flag is set, and that is a fact about the host,
    // not a failure. A green log must never claim a discipline it did not exercise — the
    // claim is on demand now, and the default log claims nothing beyond the names.
    if ( serialize_test_verbose() )
    {
        printf( "built with %s; fp contraction is %s in this build\n\n",
                SERIALIZE_TEST_FP_CONTRACT,
                !fp_contraction_live               ? "NOT AVAILABLE -- this target does not fuse, so the compressed float's rounding barriers are compiled but not exercised"
                : fp_contraction_crosses_statements ? "LIVE and CROSSING STATEMENTS -- the wire path is barriered against it; the frozen pre-split oracle fuses in this build, so its comparisons stand down"
                                                    : "LIVE and statement-local -- the compressed float's rounding barriers are under test" );
    }

#if defined( __FAST_MATH__ )
    // -ffast-math is refused by name, before a single vector runs. Contraction is not the
    // problem there -- the barriers hold under fast-math too -- the unsafe transformations
    // are: reciprocal approximation of the reader's division decodes 0xB6298800 where every
    // conformant runtime decodes 0x00000000, and no barrier on the product can repair a
    // quotient that was never correctly rounded. A -ffast-math build cannot produce a
    // conformant wire, so the honest report is this one, not a bit pattern mismatch that
    // reads like a library bug.
    printf( "This build uses -ffast-math (or -Ofast), which licenses reciprocal approximation and\n"
            "reassociation in the exact arithmetic the compressed float conformance vectors pin.\n"
            "The wire cannot be conformant from this build. Rebuild without -ffast-math.\n\n" );
    serialize_check( false );
#endif // #if defined( __FAST_MATH__ )

    // The barrier is what the wire's contraction invariance rests on: the audited home pins
    // the quantization's two roundings with SERIALIZE_FLOAT_FORCE_ROUND, so the wire bits do
    // not depend on the consumer's contraction flag -- a build at GCC's default
    // -ffp-contract=fast produces the same bytes and the same decoded bit patterns as one at
    // -ffp-contract=off (issues #92/#95; the conformance vectors hold this bit-exactly on
    // every build below). A toolchain that fuses THROUGH the barrier would take that
    // property back, so it is probed directly and refused by name, before a single vector
    // runs. The differential's negative controls guard the same property flag-independently
    // from the other side: if the audited home's roundings are ever fused, its decode
    // collapses onto the one-rounding sentinel, the divergence count drops to zero, and the
    // differential fails.
    if ( !serialize_test_fp_barrier_holds() )
    {
        printf( "This toolchain contracts floating point THROUGH SERIALIZE_FLOAT_FORCE_ROUND. The compressed\n"
                "float's quantization requires TWO distinct roundings on write and on read (STANDARD.md), and\n"
                "the barrier is what pins them under every contraction mode. On this build it does not hold, so\n"
                "the wire cannot be conformant. Rebuild with -ffp-contract=off.\n\n" );
        serialize_check( !"SERIALIZE_FLOAT_FORCE_ROUND does not hold on this toolchain" );
    }

    // while ( 1 )
    {
        SERIALIZE_RUN_TEST( test_endian );
        SERIALIZE_RUN_TEST( test_bitpacker );
        SERIALIZE_RUN_TEST( test_bits_required );
        SERIALIZE_RUN_TEST( test_bits_required64 );
        SERIALIZE_RUN_TEST( test_bits_required128 );
        SERIALIZE_RUN_TEST( test_zigzag );
        SERIALIZE_RUN_TEST( test_serialize );
        SERIALIZE_RUN_TEST( test_read_write );
        SERIALIZE_RUN_TEST( test_serialize_integer_validation );
        SERIALIZE_RUN_TEST( test_serialize_degenerate_range );
        SERIALIZE_RUN_TEST( test_serialize_degenerate_range_64 );
        SERIALIZE_RUN_TEST( test_serialize_integer_full_range );
        SERIALIZE_RUN_TEST( test_serialize_int64_full_range );
        SERIALIZE_RUN_TEST( test_serialize_int64_validation );
        SERIALIZE_RUN_TEST( test_serialize_bytes_validation );
        SERIALIZE_RUN_TEST( test_wstring_validation );
        SERIALIZE_RUN_TEST( test_wstring_utf16_code_units );
        SERIALIZE_RUN_TEST( test_string_read_validation );
        SERIALIZE_RUN_TEST( test_wstring_read_validation );
        SERIALIZE_RUN_TEST( test_int_relative_validation );
        SERIALIZE_RUN_TEST( test_compressed_float_validation );
        SERIALIZE_RUN_TEST( test_compressed_float_top_of_range_clamp );
        SERIALIZE_RUN_TEST( test_compressed_float_non_finite_asserts );
        SERIALIZE_RUN_TEST( test_compressed_float_precomputed_validation );
        SERIALIZE_RUN_TEST( test_compressed_float_precomputed_asserts );
        SERIALIZE_RUN_TEST( test_serialize_fixed );
        SERIALIZE_RUN_TEST( test_serialize_fixed_validation );
        SERIALIZE_RUN_TEST( test_serialize_fixed_matches_int64 );
        SERIALIZE_RUN_TEST( test_serialize_fixed_wide );
        SERIALIZE_RUN_TEST( test_serialize_fixed_wide_emulated );
        SERIALIZE_RUN_TEST( test_serialize_fixed_degenerate );
        SERIALIZE_RUN_TEST( test_serialize_uint128 );
        SERIALIZE_RUN_TEST( test_serialize_int128 );
        SERIALIZE_RUN_TEST( test_uint128_emulation );
        SERIALIZE_RUN_TEST( test_int128_emulation );
#if defined(__SIZEOF_INT128__)
        SERIALIZE_RUN_TEST( test_uint128_differential );
        SERIALIZE_RUN_TEST( test_int128_differential );
#endif // #if defined(__SIZEOF_INT128__)
#if defined( SERIALIZE_HAS_COMPILE_TIME_SURFACE )
        SERIALIZE_RUN_TEST( test_compile_time_bits_required );
        SERIALIZE_RUN_TEST( test_compile_time_int );
        SERIALIZE_RUN_TEST( test_compile_time_int64 );
        SERIALIZE_RUN_TEST( test_compile_time_bits );
        SERIALIZE_RUN_TEST( test_compile_time_int_validation );
        SERIALIZE_RUN_TEST( test_compile_time_int64_validation );
        SERIALIZE_RUN_TEST( test_compile_time_bits_validation );
        SERIALIZE_RUN_TEST( test_compile_time_packet );
#endif // #if defined( SERIALIZE_HAS_COMPILE_TIME_SURFACE )
        SERIALIZE_RUN_TEST( test_golden_wire_format );
        SERIALIZE_RUN_TEST( test_trailing_bits );
        SERIALIZE_RUN_TEST( test_past_end_poison );
        SERIALIZE_RUN_TEST( test_compressed_float_conformance_nonzero_min );
        SERIALIZE_RUN_TEST( test_compressed_float_conformance_writer_fusion );
        SERIALIZE_RUN_TEST( test_compressed_float_precomputed_conformance );
        SERIALIZE_RUN_TEST( test_compressed_float_precomputed_differential );
        SERIALIZE_RUN_TEST( test_golden_float_bit_transparency );
        SERIALIZE_RUN_TEST( test_golden_zero_length_bytes );
        SERIALIZE_RUN_TEST( test_golden_zero_length_string );
        SERIALIZE_RUN_TEST( test_golden_unaligned_bytes );
        SERIALIZE_RUN_TEST( test_measure_bound );
        SERIALIZE_RUN_TEST( test_unaligned_writer );
        SERIALIZE_RUN_TEST( test_large_buffer );
    }
}

#endif // #if SERIALIZE_ENABLE_TESTS

#ifdef _MSC_VER
#pragma warning( pop )
#endif // #ifdef _MSC_VER

#endif // #ifndef SERIALIZE_H
