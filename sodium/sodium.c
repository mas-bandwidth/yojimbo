/* Amalgamated libsodium subset for yojimbo/netcode — generated; do not edit by hand. */
#include "sodium.h"

/* ================= sodium_version.c ================= */


const char *
sodium_version_string(void)
{
    return SODIUM_VERSION_STRING;
}

int
sodium_library_version_major(void)
{
    return SODIUM_LIBRARY_VERSION_MAJOR;
}

int
sodium_library_version_minor(void)
{
    return SODIUM_LIBRARY_VERSION_MINOR;
}

int
sodium_library_minimal(void)
{
#ifdef SODIUM_LIBRARY_MINIMAL
    return 1;
#else
    return 0;
#endif
}

/* ================= sodium_utils.c ================= */
#ifndef __STDC_WANT_LIB_EXT1__
# define __STDC_WANT_LIB_EXT1__ 1
#endif
#include <assert.h>
#include <errno.h>
#include <limits.h>
#if !defined(__ORBIS__) && !defined(__PROSPERO__)
#include <signal.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#ifdef _WIN32
# include <windows.h>
# if !defined(_XBOX_ONE) && !defined(_GAMING_XBOX)
#  include <wincrypt.h>
# endif
#else
# include <unistd.h>
#endif

#ifndef HAVE_C_VARARRAYS
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
# elif !defined(alloca)
#  if defined(__clang__) || defined(__GNUC__)
#   define alloca __builtin_alloca
#  elif defined _AIX
#   define alloca __alloca
#  elif defined _MSC_VER
#   include <malloc.h>
#   define alloca _alloca
#  else
#   include <stddef.h>
#   ifdef  __cplusplus
extern "C"
#   endif
void *alloca (size_t);
#  endif
# endif
#endif


#ifndef ENOSYS
# define ENOSYS ENXIO
#endif

#if defined(_WIN32) && \
    (!defined(WINAPI_FAMILY) || WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
# define WINAPI_DESKTOP
#endif

#define CANARY_SIZE 16U
#define GARBAGE_VALUE 0xdb

#ifndef MAP_NOCORE
# define MAP_NOCORE 0
#endif
#if !defined(MAP_ANON) && defined(MAP_ANONYMOUS)
# define MAP_ANON MAP_ANONYMOUS
#endif
#if defined(WINAPI_DESKTOP) || (defined(MAP_ANON) && defined(HAVE_MMAP)) || \
    defined(HAVE_POSIX_MEMALIGN)
# define HAVE_ALIGNED_MALLOC
#endif
#if defined(HAVE_MPROTECT) && \
    !(defined(PROT_NONE) && defined(PROT_READ) && defined(PROT_WRITE))
# undef HAVE_MPROTECT
#endif
#if defined(HAVE_ALIGNED_MALLOC) && \
    (defined(WINAPI_DESKTOP) || defined(HAVE_MPROTECT))
# define HAVE_PAGE_PROTECTION
#endif
#if !defined(MADV_DODUMP) && defined(MADV_CORE)
# define MADV_DODUMP   MADV_CORE
# define MADV_DONTDUMP MADV_NOCORE
#endif

#ifdef HAVE_ALIGNED_MALLOC
static size_t        page_size;
#endif // #if HAVE_ALIGNED_MALLOC
static unsigned char canary[CANARY_SIZE];

/* LCOV_EXCL_START */
#ifdef HAVE_WEAK_SYMBOLS
__attribute__((weak)) void
_sodium_dummy_symbol_to_prevent_memzero_lto(void *const  pnt,
                                            const size_t len);
__attribute__((weak)) void
_sodium_dummy_symbol_to_prevent_memzero_lto(void *const  pnt,
                                            const size_t len)
{
    (void) pnt; /* LCOV_EXCL_LINE */
    (void) len; /* LCOV_EXCL_LINE */
}
#endif
/* LCOV_EXCL_STOP */

void
sodium_memzero(void *const pnt, const size_t len)
{
#ifdef _WIN32
    SecureZeroMemory(pnt, len);
#elif defined(HAVE_MEMSET_S)
    if (len > 0U && memset_s(pnt, (rsize_t) len, 0, (rsize_t) len) != 0) {
        sodium_misuse(); /* LCOV_EXCL_LINE */
    }
#elif defined(HAVE_EXPLICIT_BZERO)
    explicit_bzero(pnt, len);
#elif defined(HAVE_EXPLICIT_MEMSET)
    explicit_memset(pnt, 0, len);
#elif HAVE_WEAK_SYMBOLS
    memset(pnt, 0, len);
    _sodium_dummy_symbol_to_prevent_memzero_lto(pnt, len);
# ifdef HAVE_INLINE_ASM
    __asm__ __volatile__ ("" : : "r"(pnt) : "memory");
# endif
#else
    volatile unsigned char *volatile pnt_ =
        (volatile unsigned char *volatile) pnt;
    size_t i = (size_t) 0U;

    while (i < len) {
        pnt_[i++] = 0U;
    }
#endif
}

void
sodium_stackzero(const size_t len)
{
    (void) len;
#ifdef HAVE_C_VARARRAYS
    unsigned char fodder[len];
    sodium_memzero(fodder, len);
#elif HAVE_ALLOCA
    sodium_memzero(alloca(len), len);
#endif
}

#ifdef HAVE_WEAK_SYMBOLS
__attribute__((weak)) void
_sodium_dummy_symbol_to_prevent_memcmp_lto(const unsigned char *b1,
                                           const unsigned char *b2,
                                           const size_t         len);
__attribute__((weak)) void
_sodium_dummy_symbol_to_prevent_memcmp_lto(const unsigned char *b1,
                                           const unsigned char *b2,
                                           const size_t         len)
{
    (void) b1;
    (void) b2;
    (void) len;
}
#endif

int
sodium_memcmp(const void *const b1_, const void *const b2_, size_t len)
{
#ifdef HAVE_WEAK_SYMBOLS
    const unsigned char *b1 = (const unsigned char *) b1_;
    const unsigned char *b2 = (const unsigned char *) b2_;
#else
    const volatile unsigned char *volatile b1 =
        (const volatile unsigned char *volatile) b1_;
    const volatile unsigned char *volatile b2 =
        (const volatile unsigned char *volatile) b2_;
#endif
    size_t                 i;
    volatile unsigned char d = 0U;

#if HAVE_WEAK_SYMBOLS
    _sodium_dummy_symbol_to_prevent_memcmp_lto(b1, b2, len);
#endif
    for (i = 0U; i < len; i++) {
        d |= b1[i] ^ b2[i];
    }
    return (1 & ((d - 1) >> 8)) - 1;
}

#ifdef HAVE_WEAK_SYMBOLS
__attribute__((weak)) void
_sodium_dummy_symbol_to_prevent_compare_lto(const unsigned char *b1,
                                            const unsigned char *b2,
                                            const size_t         len);
__attribute__((weak)) void
_sodium_dummy_symbol_to_prevent_compare_lto(const unsigned char *b1,
                                            const unsigned char *b2,
                                            const size_t         len)
{
    (void) b1;
    (void) b2;
    (void) len;
}
#endif

int
sodium_compare(const unsigned char *b1_, const unsigned char *b2_, size_t len)
{
#ifdef HAVE_WEAK_SYMBOLS
    const unsigned char *b1 = b1_;
    const unsigned char *b2 = b2_;
#else
    const volatile unsigned char *volatile b1 =
        (const volatile unsigned char *volatile) b1_;
    const volatile unsigned char *volatile b2 =
        (const volatile unsigned char *volatile) b2_;
#endif
    size_t                 i;
    volatile unsigned char gt = 0U;
    volatile unsigned char eq = 1U;
    uint16_t               x1, x2;

#if HAVE_WEAK_SYMBOLS
    _sodium_dummy_symbol_to_prevent_compare_lto(b1, b2, len);
#endif
    i = len;
    while (i != 0U) {
        i--;
        x1 = b1[i];
        x2 = b2[i];
        gt |= ((x2 - x1) >> 8) & eq;
        eq &= ((x2 ^ x1) - 1) >> 8;
    }
    return (int) (gt + gt + eq) - 1;
}

int
sodium_is_zero(const unsigned char *n, const size_t nlen)
{
    size_t                 i;
    volatile unsigned char d = 0U;

    for (i = 0U; i < nlen; i++) {
        d |= n[i];
    }
    return 1 & ((d - 1) >> 8);
}

void
sodium_increment(unsigned char *n, const size_t nlen)
{
    size_t        i = 0U;
    uint_fast16_t c = 1U;

#ifdef HAVE_AMD64_ASM
    uint64_t t64, t64_2;
    uint32_t t32;

    if (nlen == 12U) {
        __asm__ __volatile__(
            "xorq %[t64], %[t64] \n"
            "xorl %[t32], %[t32] \n"
            "stc \n"
            "adcq %[t64], (%[out]) \n"
            "adcl %[t32], 8(%[out]) \n"
            : [t64] "=&r"(t64), [t32] "=&r"(t32)
            : [out] "D"(n)
            : "memory", "flags", "cc");
        return;
    } else if (nlen == 24U) {
        __asm__ __volatile__(
            "movq $1, %[t64] \n"
            "xorq %[t64_2], %[t64_2] \n"
            "addq %[t64], (%[out]) \n"
            "adcq %[t64_2], 8(%[out]) \n"
            "adcq %[t64_2], 16(%[out]) \n"
            : [t64] "=&r"(t64), [t64_2] "=&r"(t64_2)
            : [out] "D"(n)
            : "memory", "flags", "cc");
        return;
    } else if (nlen == 8U) {
        __asm__ __volatile__("incq (%[out]) \n"
                             :
                             : [out] "D"(n)
                             : "memory", "flags", "cc");
        return;
    }
#endif
    for (; i < nlen; i++) {
        c += (uint_fast16_t) n[i];
        n[i] = (unsigned char) c;
        c >>= 8;
    }
}

void
sodium_add(unsigned char *a, const unsigned char *b, const size_t len)
{
    size_t        i;
    uint_fast16_t c = 0U;

#ifdef HAVE_AMD64_ASM
    uint64_t t64, t64_2, t64_3;
    uint32_t t32;

    if (len == 12U) {
        __asm__ __volatile__(
            "movq (%[in]), %[t64] \n"
            "movl 8(%[in]), %[t32] \n"
            "addq %[t64], (%[out]) \n"
            "adcl %[t32], 8(%[out]) \n"
            : [t64] "=&r"(t64), [t32] "=&r"(t32)
            : [in] "S"(b), [out] "D"(a)
            : "memory", "flags", "cc");
        return;
    } else if (len == 24U) {
        __asm__ __volatile__(
            "movq (%[in]), %[t64] \n"
            "movq 8(%[in]), %[t64_2] \n"
            "movq 16(%[in]), %[t64_3] \n"
            "addq %[t64], (%[out]) \n"
            "adcq %[t64_2], 8(%[out]) \n"
            "adcq %[t64_3], 16(%[out]) \n"
            : [t64] "=&r"(t64), [t64_2] "=&r"(t64_2), [t64_3] "=&r"(t64_3)
            : [in] "S"(b), [out] "D"(a)
            : "memory", "flags", "cc");
        return;
    } else if (len == 8U) {
        __asm__ __volatile__(
            "movq (%[in]), %[t64] \n"
            "addq %[t64], (%[out]) \n"
            : [t64] "=&r"(t64)
            : [in] "S"(b), [out] "D"(a)
            : "memory", "flags", "cc");
        return;
    }
#endif
    for (i = 0U; i < len; i++) {
        c += (uint_fast16_t) a[i] + (uint_fast16_t) b[i];
        a[i] = (unsigned char) c;
        c >>= 8;
    }
}

void
sodium_sub(unsigned char *a, const unsigned char *b, const size_t len)
{
    uint_fast16_t c = 0U;
    size_t        i;

#ifdef HAVE_AMD64_ASM
    uint64_t t64_1, t64_2, t64_3, t64_4;
    uint64_t t64_5, t64_6, t64_7, t64_8;
    uint32_t t32;

    if (len == 64U) {
        __asm__ __volatile__(
            "movq   (%[in]), %[t64_1] \n"
            "movq  8(%[in]), %[t64_2] \n"
            "movq 16(%[in]), %[t64_3] \n"
            "movq 24(%[in]), %[t64_4] \n"
            "movq 32(%[in]), %[t64_5] \n"
            "movq 40(%[in]), %[t64_6] \n"
            "movq 48(%[in]), %[t64_7] \n"
            "movq 56(%[in]), %[t64_8] \n"
            "subq %[t64_1],   (%[out]) \n"
            "sbbq %[t64_2],  8(%[out]) \n"
            "sbbq %[t64_3], 16(%[out]) \n"
            "sbbq %[t64_4], 24(%[out]) \n"
            "sbbq %[t64_5], 32(%[out]) \n"
            "sbbq %[t64_6], 40(%[out]) \n"
            "sbbq %[t64_7], 48(%[out]) \n"
            "sbbq %[t64_8], 56(%[out]) \n"
            : [t64_1] "=&r"(t64_1), [t64_2] "=&r"(t64_2), [t64_3] "=&r"(t64_3), [t64_4] "=&r"(t64_4),
              [t64_5] "=&r"(t64_5), [t64_6] "=&r"(t64_6), [t64_7] "=&r"(t64_7), [t64_8] "=&r"(t64_8)
            : [in] "S"(b), [out] "D"(a)
            : "memory", "flags", "cc");
        return;
    }
#endif
    for (i = 0U; i < len; i++) {
        c = (uint_fast16_t) a[i] - (uint_fast16_t) b[i] - c;
        a[i] = (unsigned char) c;
        c = (c >> 8) & 1U;
    }
}

int
_sodium_alloc_init(void)
{
#ifdef HAVE_ALIGNED_MALLOC
# if defined(_SC_PAGESIZE)
    long page_size_ = sysconf(_SC_PAGESIZE);
    if (page_size_ > 0L) {
        page_size = (size_t) page_size_;
    }
# elif defined(WINAPI_DESKTOP)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    page_size = (size_t) si.dwPageSize;
# endif
    if (page_size < CANARY_SIZE || page_size < sizeof(size_t)) {
        sodium_misuse(); /* LCOV_EXCL_LINE */
    }
#endif
    randombytes_buf(canary, sizeof canary);

    return 0;
}

int
sodium_mlock(void *const addr, const size_t len)
{
    (void) len;
    (void) addr;
#if defined(MADV_DONTDUMP) && defined(HAVE_MADVISE)
    (void) madvise(addr, len, MADV_DONTDUMP);
#endif
#ifdef HAVE_MLOCK
    return mlock(addr, len);
#elif defined(WINAPI_DESKTOP)
    return -(VirtualLock(addr, len) == 0);
#else
    errno = ENOSYS;
    return -1;
#endif
}

int
sodium_munlock(void *const addr, const size_t len)
{
    sodium_memzero(addr, len);
#if defined(MADV_DODUMP) && defined(HAVE_MADVISE)
    (void) madvise(addr, len, MADV_DODUMP);
#endif
#ifdef HAVE_MLOCK
    return munlock(addr, len);
#elif defined(WINAPI_DESKTOP)
    return -(VirtualUnlock(addr, len) == 0);
#else
    errno = ENOSYS;
    return -1;
#endif
}

static int
_mprotect_noaccess(void *ptr, size_t size)
{
    (void) ptr;
    (void) size;
#ifdef HAVE_MPROTECT
    return mprotect(ptr, size, PROT_NONE);
#elif defined(WINAPI_DESKTOP)
    DWORD old;
    return -(VirtualProtect(ptr, size, PAGE_NOACCESS, &old) == 0);
#else
    errno = ENOSYS;
    return -1;
#endif
}

static int
_mprotect_readonly(void *ptr, size_t size)
{
    (void) ptr;
    (void) size;
#ifdef HAVE_MPROTECT
    return mprotect(ptr, size, PROT_READ);
#elif defined(WINAPI_DESKTOP)
    DWORD old;
    return -(VirtualProtect(ptr, size, PAGE_READONLY, &old) == 0);
#else
    errno = ENOSYS;
    return -1;
#endif
}

static int
_mprotect_readwrite(void *ptr, size_t size)
{
    (void) ptr;
    (void) size;
#ifdef HAVE_MPROTECT
    return mprotect(ptr, size, PROT_READ | PROT_WRITE);
#elif defined(WINAPI_DESKTOP)
    DWORD old;
    return -(VirtualProtect(ptr, size, PAGE_READWRITE, &old) == 0);
#else
    errno = ENOSYS;
    return -1;
#endif
}

#ifdef HAVE_ALIGNED_MALLOC

__attribute__((noreturn)) static void
_out_of_bounds(void)
{
# ifdef SIGSEGV
    raise(SIGSEGV);
# elif defined(SIGKILL)
    raise(SIGKILL);
# endif
    abort(); /* not something we want any higher-level API to catch */
} /* LCOV_EXCL_LINE */

static inline size_t
_page_round(const size_t size)
{
    const size_t page_mask = page_size - 1U;

    return (size + page_mask) & ~page_mask;
}

static __attribute__((malloc)) unsigned char *
_alloc_aligned(const size_t size)
{
    void *ptr;

# if defined(MAP_ANON) && defined(HAVE_MMAP)
    if ((ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                    MAP_ANON | MAP_PRIVATE | MAP_NOCORE, -1, 0)) ==
        MAP_FAILED) {
        ptr = NULL; /* LCOV_EXCL_LINE */
    }               /* LCOV_EXCL_LINE */
# elif defined(HAVE_POSIX_MEMALIGN)
    if (posix_memalign(&ptr, page_size, size) != 0) {
        ptr = NULL; /* LCOV_EXCL_LINE */
    }               /* LCOV_EXCL_LINE */
# elif defined(WINAPI_DESKTOP)
    ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
# else
#  error Bug
# endif
    return (unsigned char *) ptr;
}

static void
_free_aligned(unsigned char *const ptr, const size_t size)
{
    (void) size;
# if defined(MAP_ANON) && defined(HAVE_MMAP)
    (void) munmap(ptr, size);
# elif defined(HAVE_POSIX_MEMALIGN)
    free(ptr);
# elif defined(WINAPI_DESKTOP)
    VirtualFree(ptr, 0U, MEM_RELEASE);
# else
#  error Bug
#endif
}

static unsigned char *
_unprotected_ptr_from_user_ptr(void *const ptr)
{
    uintptr_t      unprotected_ptr_u;
    unsigned char *canary_ptr;
    size_t         page_mask;

    canary_ptr = ((unsigned char *) ptr) - sizeof canary;
    page_mask = page_size - 1U;
    unprotected_ptr_u = ((uintptr_t) canary_ptr & (uintptr_t) ~page_mask);
    if (unprotected_ptr_u <= page_size * 2U) {
        sodium_misuse(); /* LCOV_EXCL_LINE */
    }
    return (unsigned char *) unprotected_ptr_u;
}

#endif /* HAVE_ALIGNED_MALLOC */

#ifndef HAVE_ALIGNED_MALLOC
static __attribute__((malloc)) void *
_sodium_malloc(const size_t size)
{
    return malloc(size > (size_t) 0U ? size : (size_t) 1U);
}
#else
static __attribute__((malloc)) void *
_sodium_malloc(const size_t size)
{
    void          *user_ptr;
    unsigned char *base_ptr;
    unsigned char *canary_ptr;
    unsigned char *unprotected_ptr;
    size_t         size_with_canary;
    size_t         total_size;
    size_t         unprotected_size;

    if (size >= (size_t) SIZE_MAX - page_size * 4U) {
        errno = ENOMEM;
        return NULL;
    }
    if (page_size <= sizeof canary || page_size < sizeof unprotected_size) {
        sodium_misuse(); /* LCOV_EXCL_LINE */
    }
    size_with_canary = (sizeof canary) + size;
    unprotected_size = _page_round(size_with_canary);
    total_size       = page_size + page_size + unprotected_size + page_size;
    if ((base_ptr = _alloc_aligned(total_size)) == NULL) {
        return NULL; /* LCOV_EXCL_LINE */
    }
    unprotected_ptr = base_ptr + page_size * 2U;
    _mprotect_noaccess(base_ptr + page_size, page_size);
# ifndef HAVE_PAGE_PROTECTION
    memcpy(unprotected_ptr + unprotected_size, canary, sizeof canary);
# endif
    _mprotect_noaccess(unprotected_ptr + unprotected_size, page_size);
    sodium_mlock(unprotected_ptr, unprotected_size);
    canary_ptr =
        unprotected_ptr + _page_round(size_with_canary) - size_with_canary;
    user_ptr = canary_ptr + sizeof canary;
    memcpy(canary_ptr, canary, sizeof canary);
    memcpy(base_ptr, &unprotected_size, sizeof unprotected_size);
    _mprotect_readonly(base_ptr, page_size);
    assert(_unprotected_ptr_from_user_ptr(user_ptr) == unprotected_ptr);

    return user_ptr;
}
#endif /* !HAVE_ALIGNED_MALLOC */

__attribute__((malloc)) void *
sodium_malloc(const size_t size)
{
    void *ptr;

    if ((ptr = _sodium_malloc(size)) == NULL) {
        return NULL;
    }
    memset(ptr, (int) GARBAGE_VALUE, size);

    return ptr;
}

__attribute__((malloc)) void *
sodium_allocarray(size_t count, size_t size)
{
    if (count > (size_t) 0U && size >= (size_t) SIZE_MAX / count) {
        errno = ENOMEM;
        return NULL;
    }
    return sodium_malloc(count * size);
}

#ifndef HAVE_ALIGNED_MALLOC
void
sodium_free(void *ptr)
{
    free(ptr);
}
#else
void
sodium_free(void *ptr)
{
    unsigned char *base_ptr;
    unsigned char *canary_ptr;
    unsigned char *unprotected_ptr;
    size_t         total_size;
    size_t         unprotected_size;

    if (ptr == NULL) {
        return;
    }
    canary_ptr      = ((unsigned char *) ptr) - sizeof canary;
    unprotected_ptr = _unprotected_ptr_from_user_ptr(ptr);
    base_ptr        = unprotected_ptr - page_size * 2U;
    memcpy(&unprotected_size, base_ptr, sizeof unprotected_size);
    total_size = page_size + page_size + unprotected_size + page_size;
    _mprotect_readwrite(base_ptr, total_size);
    if (sodium_memcmp(canary_ptr, canary, sizeof canary) != 0) {
        _out_of_bounds();
    }
# ifndef HAVE_PAGE_PROTECTION
    if (sodium_memcmp(unprotected_ptr + unprotected_size, canary,
                      sizeof canary) != 0) {
        _out_of_bounds();
    }
# endif
    sodium_munlock(unprotected_ptr, unprotected_size);
    _free_aligned(base_ptr, total_size);
}
#endif /* HAVE_ALIGNED_MALLOC */

#ifndef HAVE_PAGE_PROTECTION
static int
_sodium_mprotect(void *ptr, int (*cb)(void *ptr, size_t size))
{
    (void) ptr;
    (void) cb;
    errno = ENOSYS;
    return -1;
}
#else
static int
_sodium_mprotect(void *ptr, int (*cb)(void *ptr, size_t size))
{
    unsigned char *base_ptr;
    unsigned char *unprotected_ptr;
    size_t         unprotected_size;

    unprotected_ptr = _unprotected_ptr_from_user_ptr(ptr);
    base_ptr        = unprotected_ptr - page_size * 2U;
    memcpy(&unprotected_size, base_ptr, sizeof unprotected_size);

    return cb(unprotected_ptr, unprotected_size);
}
#endif

int
sodium_mprotect_noaccess(void *ptr)
{
    return _sodium_mprotect(ptr, _mprotect_noaccess);
}

int
sodium_mprotect_readonly(void *ptr)
{
    return _sodium_mprotect(ptr, _mprotect_readonly);
}

int
sodium_mprotect_readwrite(void *ptr)
{
    return _sodium_mprotect(ptr, _mprotect_readwrite);
}

int
sodium_pad(size_t *padded_buflen_p, unsigned char *buf,
           size_t unpadded_buflen, size_t blocksize, size_t max_buflen)
{
    unsigned char          *tail;
    size_t                  i;
    size_t                  xpadlen;
    size_t                  xpadded_len;
    volatile unsigned char  mask;
    unsigned char           barrier_mask;

    if (blocksize <= 0U) {
        return -1;
    }
    xpadlen = blocksize - 1U;
    if ((blocksize & (blocksize - 1U)) == 0U) {
        xpadlen -= unpadded_buflen & (blocksize - 1U);
    } else {
        xpadlen -= unpadded_buflen % blocksize;
    }
    if ((size_t) SIZE_MAX - unpadded_buflen <= xpadlen) {
        sodium_misuse();
    }
    xpadded_len = unpadded_buflen + xpadlen;
    if (xpadded_len >= max_buflen) {
        return -1;
    }
    tail = &buf[xpadded_len];
    if (padded_buflen_p != NULL) {
        *padded_buflen_p = xpadded_len + 1U;
    }
    mask = 0U;
    for (i = 0; i < blocksize; i++) {
        barrier_mask = (unsigned char) (((i ^ xpadlen) - 1U)
           >> ((sizeof(size_t) - 1) * CHAR_BIT));
        *(tail - i) = ((*(tail - i)) & mask) | (0x80 & barrier_mask);
        mask |= barrier_mask;
    }
    return 0;
}

int
sodium_unpad(size_t *unpadded_buflen_p, const unsigned char *buf,
             size_t padded_buflen, size_t blocksize)
{
    const unsigned char *tail;
    unsigned char        acc = 0U;
    unsigned char        c;
    unsigned char        valid = 0U;
    volatile size_t      pad_len = 0U;
    size_t               i;
    size_t               is_barrier;

    if (padded_buflen < blocksize || blocksize <= 0U) {
        return -1;
    }
    tail = &buf[padded_buflen - 1U];

    for (i = 0U; i < blocksize; i++) {
        c = *(tail - i);
        is_barrier =
            (( (acc - 1U) & (pad_len - 1U) & ((c ^ 0x80) - 1U) ) >> 8) & 1U;
        acc |= c;
        pad_len |= i & (1U + ~is_barrier);
        valid |= (unsigned char) is_barrier;
    }
    *unpadded_buflen_p = padded_buflen - 1U - pad_len;

    return (int) (valid - 1U);
}
#undef __STDC_WANT_LIB_EXT1__
#undef alloca
#undef ENOSYS
#undef WINAPI_DESKTOP
#undef CANARY_SIZE
#undef GARBAGE_VALUE
#undef MAP_NOCORE
#undef MAP_ANON
#undef HAVE_ALIGNED_MALLOC
#undef HAVE_PAGE_PROTECTION
#undef MADV_DODUMP
#undef MADV_DONTDUMP

/* ================= sodium_verify.c ================= */

#include <stddef.h>
#include <stdint.h>


size_t
crypto_verify_16_bytes(void)
{
    return crypto_verify_16_BYTES;
}

size_t
crypto_verify_32_bytes(void)
{
    return crypto_verify_32_BYTES;
}

size_t
crypto_verify_64_bytes(void)
{
    return crypto_verify_64_BYTES;
}

#if defined(HAVE_EMMINTRIN_H) && defined(__SSE2__)

# ifdef __GNUC__
#  pragma GCC target("sse2")
# endif
# include <emmintrin.h>

static inline int
crypto_verify_n(const unsigned char *x_, const unsigned char *y_,
                const int n)
{
    const    __m128i zero = _mm_setzero_si128();
    volatile __m128i v1, v2, z;
    volatile int     m;
    int              i;

    const volatile __m128i *volatile x =
        (const volatile __m128i *volatile) (const void *) x_;
    const volatile __m128i *volatile y =
        (const volatile __m128i *volatile) (const void *) y_;
    v1 = _mm_loadu_si128((const __m128i *) &x[0]);
    v2 = _mm_loadu_si128((const __m128i *) &y[0]);
    z = _mm_xor_si128(v1, v2);
    for (i = 1; i < n / 16; i++) {
        v1 = _mm_loadu_si128((const __m128i *) &x[i]);
        v2 = _mm_loadu_si128((const __m128i *) &y[i]);
        z = _mm_or_si128(z, _mm_xor_si128(v1, v2));
    }
    m = _mm_movemask_epi8(_mm_cmpeq_epi32(z, zero));
    v1 = zero; v2 = zero; z = zero;

    return (int) (((uint32_t) m + 1U) >> 16) - 1;
}

#else

static inline int
crypto_verify_n(const unsigned char *x_, const unsigned char *y_,
                const int n)
{
    const volatile unsigned char *volatile x =
        (const volatile unsigned char *volatile) x_;
    const volatile unsigned char *volatile y =
        (const volatile unsigned char *volatile) y_;
    volatile uint_fast16_t d = 0U;
    int i;

    for (i = 0; i < n; i++) {
        d |= x[i] ^ y[i];
    }
    return (1 & ((d - 1) >> 8)) - 1;
}

#endif

int
crypto_verify_16(const unsigned char *x, const unsigned char *y)
{
    return crypto_verify_n(x, y, crypto_verify_16_BYTES);
}

int
crypto_verify_32(const unsigned char *x, const unsigned char *y)
{
    return crypto_verify_n(x, y, crypto_verify_32_BYTES);
}

int
crypto_verify_64(const unsigned char *x, const unsigned char *y)
{
    return crypto_verify_n(x, y, crypto_verify_64_BYTES);
}

/* ================= sodium_runtime.c ================= */
#include <stddef.h>
#include <stdint.h>
#ifdef HAVE_ANDROID_GETCPUFEATURES
# include <cpu-features.h>
#endif

#include <stdio.h>

typedef struct CPUFeatures_ {
    int initialized;
    int has_neon;
    int has_sse2;
    int has_sse3;
    int has_ssse3;
    int has_sse41;
    int has_avx;
    int has_avx2;
    int has_avx512f;
    int has_pclmul;
    int has_aesni;
    int has_rdrand;
} CPUFeatures;

static CPUFeatures _cpu_features;

#define CPUID_EBX_AVX2    0x00000020
#define CPUID_EBX_AVX512F 0x00010000

#define CPUID_ECX_SSE3    0x00000001
#define CPUID_ECX_PCLMUL  0x00000002
#define CPUID_ECX_SSSE3   0x00000200
#define CPUID_ECX_SSE41   0x00080000
#define CPUID_ECX_AESNI   0x02000000
#define CPUID_ECX_XSAVE   0x04000000
#define CPUID_ECX_OSXSAVE 0x08000000
#define CPUID_ECX_AVX     0x10000000
#define CPUID_ECX_RDRAND  0x40000000

#define CPUID_EDX_SSE2    0x04000000

#define XCR0_SSE       0x00000002
#define XCR0_AVX       0x00000004
#define XCR0_OPMASK    0x00000020
#define XCR0_ZMM_HI256 0x00000040
#define XCR0_HI16_ZMM  0x00000080

static int
_sodium_runtime_arm_cpu_features(CPUFeatures * const cpu_features)
{
#ifndef __arm__
    cpu_features->has_neon = 0;
    return -1;
#else
# ifdef __APPLE__
#  ifdef __ARM_NEON__
    cpu_features->has_neon = 1;
#  else
    cpu_features->has_neon = 0;
#  endif
# elif defined(HAVE_ANDROID_GETCPUFEATURES) && \
    defined(ANDROID_CPU_ARM_FEATURE_NEON)
    cpu_features->has_neon =
        (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0x0;
# else
    cpu_features->has_neon = 0;
# endif
    return 0;
#endif
}

static void
_cpuid(unsigned int cpu_info[4U], const unsigned int cpu_info_type)
{
#if defined(_MSC_VER) && \
    (defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86))
    __cpuid((int *) cpu_info, cpu_info_type);
#elif defined(HAVE_CPUID)
    cpu_info[0] = cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;
# ifdef __i386__
    __asm__ __volatile__(
        "pushfl; pushfl; "
        "popl %0; "
        "movl %0, %1; xorl %2, %0; "
        "pushl %0; "
        "popfl; pushfl; popl %0; popfl"
        : "=&r"(cpu_info[0]), "=&r"(cpu_info[1])
        : "i"(0x200000));
    if (((cpu_info[0] ^ cpu_info[1]) & 0x200000) == 0x0) {
        return; /* LCOV_EXCL_LINE */
    }
# endif
# ifdef __i386__
    __asm__ __volatile__("xchgl %%ebx, %k1; cpuid; xchgl %%ebx, %k1"
                         : "=a"(cpu_info[0]), "=&r"(cpu_info[1]),
                           "=c"(cpu_info[2]), "=d"(cpu_info[3])
                         : "0"(cpu_info_type), "2"(0U));
# elif defined(__x86_64__)
    __asm__ __volatile__("xchgq %%rbx, %q1; cpuid; xchgq %%rbx, %q1"
                         : "=a"(cpu_info[0]), "=&r"(cpu_info[1]),
                           "=c"(cpu_info[2]), "=d"(cpu_info[3])
                         : "0"(cpu_info_type), "2"(0U));
# else
    __asm__ __volatile__("cpuid"
                         : "=a"(cpu_info[0]), "=b"(cpu_info[1]),
                           "=c"(cpu_info[2]), "=d"(cpu_info[3])
                         : "0"(cpu_info_type), "2"(0U));
# endif
#else
    (void) cpu_info_type;
    cpu_info[0] = cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;
#endif
}

static int
_sodium_runtime_intel_cpu_features(CPUFeatures * const cpu_features)
{
    unsigned int cpu_info[4];
    unsigned int id;
    uint32_t     xcr0 = 0U;

    _cpuid(cpu_info, 0x0);
    if ((id = cpu_info[0]) == 0U) {
        return -1; /* LCOV_EXCL_LINE */
    }
    _cpuid(cpu_info, 0x00000001);
#ifdef HAVE_EMMINTRIN_H
    cpu_features->has_sse2 = ((cpu_info[3] & CPUID_EDX_SSE2) != 0x0);
#else
    cpu_features->has_sse2   = 0;
#endif

#ifdef HAVE_PMMINTRIN_H
    cpu_features->has_sse3 = ((cpu_info[2] & CPUID_ECX_SSE3) != 0x0);
#else
    cpu_features->has_sse3   = 0;
#endif

#ifdef HAVE_TMMINTRIN_H
    cpu_features->has_ssse3 = ((cpu_info[2] & CPUID_ECX_SSSE3) != 0x0);
#else
    cpu_features->has_ssse3  = 0;
#endif

#ifdef HAVE_SMMINTRIN_H
    cpu_features->has_sse41 = ((cpu_info[2] & CPUID_ECX_SSE41) != 0x0);
#else
    cpu_features->has_sse41  = 0;
#endif

    cpu_features->has_avx = 0;

    (void) xcr0;
#ifdef HAVE_AVXINTRIN_H
    if ((cpu_info[2] & (CPUID_ECX_AVX | CPUID_ECX_XSAVE | CPUID_ECX_OSXSAVE)) ==
        (CPUID_ECX_AVX | CPUID_ECX_XSAVE | CPUID_ECX_OSXSAVE)) {
        xcr0 = 0U;
# if defined(HAVE__XGETBV) || \
        (defined(_MSC_VER) && defined(_XCR_XFEATURE_ENABLED_MASK) && _MSC_FULL_VER >= 160040219)
        xcr0 = (uint32_t) _xgetbv(0);
# elif defined(_MSC_VER) && defined(_M_IX86)
        /*
         * Visual Studio documentation states that eax/ecx/edx don't need to
         * be preserved in inline assembly code. But that doesn't seem to
         * always hold true on Visual Studio 2010.
         */
        __asm {
            push eax
            push ecx
            push edx
            xor ecx, ecx
            _asm _emit 0x0f _asm _emit 0x01 _asm _emit 0xd0
            mov xcr0, eax
            pop edx
            pop ecx
            pop eax
        }
# elif defined(HAVE_AVX_ASM)
        __asm__ __volatile__(".byte 0x0f, 0x01, 0xd0" /* XGETBV */
                             : "=a"(xcr0)
                             : "c"((uint32_t) 0U)
                             : "%edx");
# endif
        if ((xcr0 & (XCR0_SSE | XCR0_AVX)) == (XCR0_SSE | XCR0_AVX)) {
            cpu_features->has_avx = 1;
        }
    }
#endif

    cpu_features->has_avx2 = 0;
#ifdef HAVE_AVX2INTRIN_H
    if (cpu_features->has_avx) {
        unsigned int cpu_info7[4];

        _cpuid(cpu_info7, 0x00000007);
        cpu_features->has_avx2 = ((cpu_info7[1] & CPUID_EBX_AVX2) != 0x0);
    }
#endif

    cpu_features->has_avx512f = 0;
#ifdef HAVE_AVX512FINTRIN_H
    if (cpu_features->has_avx2) {
        unsigned int cpu_info7[4];

        _cpuid(cpu_info7, 0x00000007);
        if ((cpu_info7[1] & CPUID_EBX_AVX512F) == CPUID_EBX_AVX512F &&
            (xcr0 & (XCR0_OPMASK | XCR0_ZMM_HI256 | XCR0_HI16_ZMM))
            == (XCR0_OPMASK | XCR0_ZMM_HI256 | XCR0_HI16_ZMM)) {
            cpu_features->has_avx512f = 1;
        }
    }
#endif

#ifdef HAVE_WMMINTRIN_H
    cpu_features->has_pclmul = ((cpu_info[2] & CPUID_ECX_PCLMUL) != 0x0);
    cpu_features->has_aesni  = ((cpu_info[2] & CPUID_ECX_AESNI) != 0x0);
#else
    cpu_features->has_pclmul = 0;
    cpu_features->has_aesni  = 0;
#endif

#ifdef HAVE_RDRAND
    cpu_features->has_rdrand = ((cpu_info[2] & CPUID_ECX_RDRAND) != 0x0);
#else
    cpu_features->has_rdrand = 0;
#endif

    return 0;
}

int
_sodium_runtime_get_cpu_features(void)
{
    int ret = -1;

    ret &= _sodium_runtime_arm_cpu_features(&_cpu_features);
    ret &= _sodium_runtime_intel_cpu_features(&_cpu_features);
    _cpu_features.initialized = 1;

    #if NETCODE_CRYPTO_LOGS
    printf( "\nCPU features: " );
    if ( _cpu_features.has_sse2 ) printf( "sse2 " );
    if ( _cpu_features.has_ssse3 ) printf( "ssse3 " );
    if ( _cpu_features.has_sse41 ) printf( "sse41 " );
    if ( _cpu_features.has_avx ) printf( "avx " );
    if ( _cpu_features.has_avx2 ) printf( "avx2 " );
    if ( _cpu_features.has_avx512f ) printf( "avx512f " );
    printf( "\n\n" );
    #endif // #if NETCODE_CRYPTO_LOGS
    
    return ret;
}

int
sodium_runtime_has_neon(void)
{
    return _cpu_features.has_neon;
}

int
sodium_runtime_has_sse2(void)
{
    return _cpu_features.has_sse2;
}

int
sodium_runtime_has_sse3(void)
{
    return _cpu_features.has_sse3;
}

int
sodium_runtime_has_ssse3(void)
{
    return _cpu_features.has_ssse3;
}

int
sodium_runtime_has_sse41(void)
{
    return _cpu_features.has_sse41;
}

int
sodium_runtime_has_avx(void)
{
    return _cpu_features.has_avx;
}

int
sodium_runtime_has_avx2(void)
{
    return _cpu_features.has_avx2;
}

int
sodium_runtime_has_avx512f(void)
{
    return _cpu_features.has_avx512f;
}

int
sodium_runtime_has_pclmul(void)
{
    return _cpu_features.has_pclmul;
}

int
sodium_runtime_has_aesni(void)
{
    return _cpu_features.has_aesni;
}

int
sodium_runtime_has_rdrand(void)
{
    return _cpu_features.has_rdrand;
}
#undef CPUID_EBX_AVX2
#undef CPUID_EBX_AVX512F
#undef CPUID_ECX_SSE3
#undef CPUID_ECX_PCLMUL
#undef CPUID_ECX_SSSE3
#undef CPUID_ECX_SSE41
#undef CPUID_ECX_AESNI
#undef CPUID_ECX_XSAVE
#undef CPUID_ECX_OSXSAVE
#undef CPUID_ECX_AVX
#undef CPUID_ECX_RDRAND
#undef CPUID_EDX_SSE2
#undef XCR0_SSE
#undef XCR0_AVX
#undef XCR0_OPMASK
#undef XCR0_ZMM_HI256
#undef XCR0_HI16_ZMM

/* ================= sodium_randombytes_sysrandom.c ================= */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#ifndef _WIN32
# include <unistd.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#ifndef _WIN32
# include <sys/stat.h>
# include <sys/time.h>
#endif
#ifdef __linux__
# ifdef __dietlibc__
#  define _LINUX_SOURCE
#  include <sys/random.h>
#  define HAVE_LINUX_COMPATIBLE_GETRANDOM
# else /* __dietlibc__ */
#  include <sys/syscall.h>
#  if defined(SYS_getrandom) && defined(__NR_getrandom)
#   define getrandom(B, S, F) syscall(SYS_getrandom, (B), (int) (S), (F))
#   define HAVE_LINUX_COMPATIBLE_GETRANDOM
#  endif
# endif /* __dietlibc */
#elif defined(__FreeBSD__)
# include <sys/param.h>
# if defined(__FreeBSD_version) && __FreeBSD_version >= 1200000
#  include <sys/random.h>
#  define HAVE_LINUX_COMPATIBLE_GETRANDOM
# endif
#endif
#if !defined(NO_BLOCKING_RANDOM_POLL) && defined(__linux__)
# define BLOCK_ON_DEV_RANDOM
#endif
#ifdef BLOCK_ON_DEV_RANDOM
# include <poll.h>
#endif


#ifdef _WIN32
/* `RtlGenRandom` is used over `CryptGenRandom` on Microsoft Windows based systems:
 *  - `CryptGenRandom` requires pulling in `CryptoAPI` which causes unnecessary
 *     memory overhead if this API is not being used for other purposes
 *  - `RtlGenRandom` is thus called directly instead. A detailed explanation
 *     can be found here: https://blogs.msdn.microsoft.com/michael_howard/2005/01/14/cryptographically-secure-random-number-on-windows-without-using-cryptoapi/
 *
 * In spite of the disclaimer on the `RtlGenRandom` documentation page that was
 * written back in the Windows XP days, this function is here to stay. The CRT
 * function `rand_s()` directly depends on it, so touching it would break many
 * applications released since Windows XP.
 *
 * Also note that Rust, Firefox and BoringSSL (thus, Google Chrome and everything
 * based on Chromium) also depend on it, and that libsodium allows the RNG to be
 * replaced without patching nor recompiling the library.
 */
# include <windows.h>
# define RtlGenRandom SystemFunction036
# if defined(__cplusplus)
extern "C"
# endif
# if !defined(_XBOX_ONE) && !defined(_GAMING_XBOX)
BOOLEAN NTAPI RtlGenRandom(PVOID RandomBuffer, ULONG RandomBufferLength);
# pragma comment(lib, "advapi32.lib")
# endif
#endif

#if defined(__OpenBSD__) || defined(__CloudABI__)
# define HAVE_SAFE_ARC4RANDOM 1
#endif

#ifndef SSIZE_MAX
# define SSIZE_MAX (SIZE_MAX / 2 - 1)
#endif

#ifdef HAVE_SAFE_ARC4RANDOM

static uint32_t
randombytes_sysrandom(void)
{
    return arc4random();
}

static void
randombytes_sysrandom_stir(void)
{
}

static void
randombytes_sysrandom_buf(void * const buf, const size_t size)
{
    arc4random_buf(buf, size);
}

static int
randombytes_sysrandom_close(void)
{
    return 0;
}

#elif defined(__ORBIS__) || defined(__PROSPERO__)

static uint32_t
randombytes_sysrandom(void)
{
    return 0;
}

static void
randombytes_sysrandom_stir(void)
{
}

static void
randombytes_sysrandom_buf(void * const buf, const size_t size)
{
}

static int
randombytes_sysrandom_close(void)
{
    return 0;
}

#else /* __OpenBSD__ */

typedef struct SysRandom_ {
    int random_data_source_fd;
    int initialized;
    int getrandom_available;
} SysRandom;

static SysRandom stream = {
    SODIUM_C99(.random_data_source_fd =) -1,
    SODIUM_C99(.initialized =) 0,
    SODIUM_C99(.getrandom_available =) 0
};

# ifndef _WIN32
static ssize_t
safe_read(const int fd, void * const buf_, size_t size)
{
    unsigned char *buf = (unsigned char *) buf_;
    ssize_t        readnb;

    assert(size > (size_t) 0U);
    assert(size <= SSIZE_MAX);
    do {
        while ((readnb = read(fd, buf, size)) < (ssize_t) 0 &&
               (errno == EINTR || errno == EAGAIN)); /* LCOV_EXCL_LINE */
        if (readnb < (ssize_t) 0) {
            return readnb; /* LCOV_EXCL_LINE */
        }
        if (readnb == (ssize_t) 0) {
            break; /* LCOV_EXCL_LINE */
        }
        size -= (size_t) readnb;
        buf += readnb;
    } while (size > (ssize_t) 0);

    return (ssize_t) (buf - (unsigned char *) buf_);
}

#  ifdef BLOCK_ON_DEV_RANDOM
static int
randombytes_block_on_dev_random(void)
{
    struct pollfd pfd;
    int           fd;
    int           pret;

    fd = open("/dev/random", O_RDONLY);
    if (fd == -1) {
        return 0;
    }
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    do {
        pret = poll(&pfd, 1, -1);
    } while (pret < 0 && (errno == EINTR || errno == EAGAIN));
    if (pret != 1) {
        (void) close(fd);
        errno = EIO;
        return -1;
    }
    return close(fd);
}
#  endif /* BLOCK_ON_DEV_RANDOM */

static int
randombytes_sysrandom_random_dev_open(void)
{
/* LCOV_EXCL_START */
    struct stat        st;
    static const char *devices[] = {
#  ifndef USE_BLOCKING_RANDOM
        "/dev/urandom",
#  endif
        "/dev/random", NULL
    };
    const char       **device = devices;
    int                fd;

#  ifdef BLOCK_ON_DEV_RANDOM
    if (randombytes_block_on_dev_random() != 0) {
        return -1;
    }
#  endif
    do {
        fd = open(*device, O_RDONLY);
        if (fd != -1) {
            if (fstat(fd, &st) == 0 &&
#  ifdef __COMPCERT__
                1
#  elif defined(S_ISNAM)
                (S_ISNAM(st.st_mode) || S_ISCHR(st.st_mode))
#  else
                S_ISCHR(st.st_mode)
#  endif
               ) {
#  if defined(F_SETFD) && defined(FD_CLOEXEC) && !defined(NN_NINTENDO_SDK)
                (void) fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
#  endif
                return fd;
            }
            (void) close(fd);
        } else if (errno == EINTR) {
            continue;
        }
        device++;
    } while (*device != NULL);

    errno = EIO;
    return -1;
/* LCOV_EXCL_STOP */
}

#  ifdef HAVE_LINUX_COMPATIBLE_GETRANDOM
static int
_randombytes_linux_getrandom(void * const buf, const size_t size)
{
    int readnb;

    assert(size <= 256U);
    do {
        readnb = getrandom(buf, size, 0);
    } while (readnb < 0 && (errno == EINTR || errno == EAGAIN));

    return (readnb == (int) size) - 1;
}

static int
randombytes_linux_getrandom(void * const buf_, size_t size)
{
    unsigned char *buf = (unsigned char *) buf_;
    size_t         chunk_size = 256U;

    do {
        if (size < chunk_size) {
            chunk_size = size;
            assert(chunk_size > (size_t) 0U);
        }
        if (_randombytes_linux_getrandom(buf, chunk_size) != 0) {
            return -1;
        }
        size -= chunk_size;
        buf += chunk_size;
    } while (size > (size_t) 0U);

    return 0;
}
#  endif /* HAVE_LINUX_COMPATIBLE_GETRANDOM */

static void
randombytes_sysrandom_init(void)
{
    const int     errno_save = errno;

#  ifdef HAVE_LINUX_COMPATIBLE_GETRANDOM
    {
        unsigned char fodder[16];

        if (randombytes_linux_getrandom(fodder, sizeof fodder) == 0) {
            stream.getrandom_available = 1;
            errno = errno_save;
            return;
        }
        stream.getrandom_available = 0;
    }
#  endif

    if ((stream.random_data_source_fd =
         randombytes_sysrandom_random_dev_open()) == -1) {
        sodium_misuse(); /* LCOV_EXCL_LINE */
    }
    errno = errno_save;
}

# else /* _WIN32 */

static void
randombytes_sysrandom_init(void)
{
}
# endif /* _WIN32 */

static void
randombytes_sysrandom_stir(void)
{
    if (stream.initialized == 0) {
        randombytes_sysrandom_init();
        stream.initialized = 1;
    }
}

static void
randombytes_sysrandom_stir_if_needed(void)
{
    if (stream.initialized == 0) {
        randombytes_sysrandom_stir();
    }
}

static int
randombytes_sysrandom_close(void)
{
    int ret = -1;

# ifndef _WIN32
    if (stream.random_data_source_fd != -1 &&
        close(stream.random_data_source_fd) == 0) {
        stream.random_data_source_fd = -1;
        stream.initialized = 0;
        ret = 0;
    }
#  ifdef HAVE_LINUX_COMPATIBLE_GETRANDOM
    if (stream.getrandom_available != 0) {
        ret = 0;
    }
#  endif
# else /* _WIN32 */
    if (stream.initialized != 0) {
        stream.initialized = 0;
        ret = 0;
    }
# endif /* _WIN32 */
    return ret;
}

static void
randombytes_sysrandom_buf(void * const buf, const size_t size)
{
    (void) buf;
    randombytes_sysrandom_stir_if_needed();
# if defined(ULLONG_MAX) && defined(SIZE_MAX)
#  if SIZE_MAX > ULLONG_MAX
    /* coverity[result_independent_of_operands] */
    assert(size <= ULLONG_MAX);
#  endif
# endif
# ifndef _WIN32
#  ifdef HAVE_LINUX_COMPATIBLE_GETRANDOM
    if (stream.getrandom_available != 0) {
        if (randombytes_linux_getrandom(buf, size) != 0) {
            sodium_misuse(); /* LCOV_EXCL_LINE */
        }
        return;
    }
#  endif
    if (stream.random_data_source_fd == -1 ||
        safe_read(stream.random_data_source_fd, buf, size) != (ssize_t) size) {
        sodium_misuse(); /* LCOV_EXCL_LINE */
    }
# else /* _WIN32 */
    COMPILER_ASSERT(randombytes_BYTES_MAX <= 0xffffffffUL);
    if (size > (size_t) 0xffffffffUL) {
        sodium_misuse(); /* LCOV_EXCL_LINE */
    }
#  if !defined(_XBOX_ONE) && !defined(_GAMING_XBOX)
    if (! RtlGenRandom((PVOID) buf, (ULONG) size)) {
        sodium_misuse(); /* LCOV_EXCL_LINE */
    }
#  endif
# endif /* _WIN32 */
}

static uint32_t
randombytes_sysrandom(void)
{
    uint32_t r;

    randombytes_sysrandom_buf(&r, sizeof r);

    return r;
}

#endif /* __OpenBSD__ */

static const char *
randombytes_sysrandom_implementation_name(void)
{
    return "sysrandom";
}

struct randombytes_implementation randombytes_sysrandom_implementation = {
    SODIUM_C99(.implementation_name =) randombytes_sysrandom_implementation_name,
    SODIUM_C99(.random =) randombytes_sysrandom,
    SODIUM_C99(.stir =) randombytes_sysrandom_stir,
    SODIUM_C99(.uniform =) NULL,
    SODIUM_C99(.buf =) randombytes_sysrandom_buf,
    SODIUM_C99(.close =) randombytes_sysrandom_close
};
#undef _LINUX_SOURCE
#undef HAVE_LINUX_COMPATIBLE_GETRANDOM
#undef getrandom
#undef BLOCK_ON_DEV_RANDOM
#undef RtlGenRandom
#undef HAVE_SAFE_ARC4RANDOM
#undef SSIZE_MAX

/* ================= sodium_randombytes.c ================= */
#define implementation implementation__rb

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>

#ifdef __EMSCRIPTEN__
# include <emscripten.h>
#endif

#ifdef RANDOMBYTES_DEFAULT_IMPLEMENTATION
#else
# ifdef __native_client__
# else
# endif
#endif
#include <stdio.h>

/* C++Builder defines a "random" macro */
#undef random

static const randombytes_implementation *implementation;

#ifndef RANDOMBYTES_DEFAULT_IMPLEMENTATION
# ifdef __EMSCRIPTEN__
#  define RANDOMBYTES_DEFAULT_IMPLEMENTATION NULL
# else
#  ifdef __native_client__
#   define RANDOMBYTES_DEFAULT_IMPLEMENTATION &randombytes_nativeclient_implementation;
#  else
#   define RANDOMBYTES_DEFAULT_IMPLEMENTATION &randombytes_sysrandom_implementation;
#  endif
# endif
#endif

static void
randombytes_init_if_needed(void)
{
    if (implementation == NULL) {
        implementation = RANDOMBYTES_DEFAULT_IMPLEMENTATION;
        randombytes_stir();
    }
}

int
randombytes_set_implementation(randombytes_implementation *impl)
{
    implementation = impl;

    return 0;
}

const char *
randombytes_implementation_name(void)
{
#ifndef __EMSCRIPTEN__
    randombytes_init_if_needed();
    return implementation->implementation_name();
#else
    return "js";
#endif
}

uint32_t
randombytes_random(void)
{
#ifndef __EMSCRIPTEN__
    randombytes_init_if_needed();
    return implementation->random();
#else
    return EM_ASM_INT_V({
        return Module.getRandomValue();
    });
#endif
}

void
randombytes_stir(void)
{
#ifndef __EMSCRIPTEN__
    randombytes_init_if_needed();
    if (implementation->stir != NULL) {
        implementation->stir();
    }
#else
    EM_ASM({
        if (Module.getRandomValue === undefined) {
            try {
                var window_ = 'object' === typeof window ? window : self;
                var crypto_ = typeof window_.crypto !== 'undefined' ? window_.crypto : window_.msCrypto;
                var randomValuesStandard = function() {
                    var buf = new Uint32Array(1);
                    crypto_.getRandomValues(buf);
                    return buf[0] >>> 0;
                };
                randomValuesStandard();
                Module.getRandomValue = randomValuesStandard;
            } catch (e) {
                try {
                    var crypto = require('crypto');
                    var randomValueNodeJS = function() {
                        var buf = crypto['randomBytes'](4);
                        return (buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]) >>> 0;
                    };
                    randomValueNodeJS();
                    Module.getRandomValue = randomValueNodeJS;
                } catch (e) {
                    throw 'No secure random number generator found';
                }
            }
        }
    });
#endif
}

uint32_t
randombytes_uniform(const uint32_t upper_bound)
{
    uint32_t min;
    uint32_t r;

#ifndef __EMSCRIPTEN__
    randombytes_init_if_needed();
    if (implementation->uniform != NULL) {
        return implementation->uniform(upper_bound);
    }
#endif
    if (upper_bound < 2) {
        return 0;
    }
    min = (1U + ~upper_bound) % upper_bound; /* = 2**32 mod upper_bound */
    do {
        r = randombytes_random();
    } while (r < min);
    /* r is now clamped to a set whose size mod upper_bound == 0
     * the worst case (2**31+1) requires ~ 2 attempts */

    return r % upper_bound;
}

void
randombytes_buf(void * const buf, const size_t size)
{
#ifndef __EMSCRIPTEN__
    randombytes_init_if_needed();
    if (size > (size_t) 0U) {
        implementation->buf(buf, size);
    }
#else
    unsigned char *p = (unsigned char *) buf;
    size_t         i;

    for (i = (size_t) 0U; i < size; i++) {
        p[i] = (unsigned char) randombytes_random();
    }
#endif
}

void
randombytes_buf_deterministic(void * const buf, const size_t size,
                              const unsigned char seed[randombytes_SEEDBYTES])
{
    static const unsigned char nonce[crypto_stream_chacha20_ietf_NONCEBYTES] = {
        'L', 'i', 'b', 's', 'o', 'd', 'i', 'u', 'm', 'D', 'R', 'G'
    };

    COMPILER_ASSERT(randombytes_SEEDBYTES == crypto_stream_chacha20_ietf_KEYBYTES);
#if SIZE_MAX > 0x4000000000ULL
    COMPILER_ASSERT(randombytes_BYTES_MAX <= 0x4000000000ULL);
    if (size > 0x4000000000ULL) {
        sodium_misuse();
    }
#endif
    crypto_stream_chacha20_ietf((unsigned char *) buf, (unsigned long long) size,
                                nonce, seed);
}

size_t
randombytes_seedbytes(void)
{
    return randombytes_SEEDBYTES;
}

int
randombytes_close(void)
{
    if (implementation != NULL && implementation->close != NULL) {
        return implementation->close();
    }
    return 0;
}

void
randombytes(unsigned char * const buf, const unsigned long long buf_len)
{
    assert(buf_len <= SIZE_MAX);
    randombytes_buf(buf, (size_t) buf_len);
}
#undef implementation
#undef RANDOMBYTES_DEFAULT_IMPLEMENTATION

/* ================= sodium_core.c ================= */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
# include <windows.h>
#elif defined(HAVE_PTHREAD)
# include <pthread.h>
#endif


static volatile int initialized;
#ifdef _WIN32
static volatile int locked;
#endif // #ifdef _WIN32

int
sodium_init(void)
{
    if (sodium_crit_enter() != 0) {
        return -1; /* LCOV_EXCL_LINE */
    }
    if (initialized != 0) {
        if (sodium_crit_leave() != 0) {
            return -1; /* LCOV_EXCL_LINE */
        }
        return 1;
    }
    _sodium_runtime_get_cpu_features();
    randombytes_stir();
    _sodium_alloc_init();
    _crypto_onetimeauth_poly1305_pick_best_implementation();
    _crypto_stream_chacha20_pick_best_implementation();
    initialized = 1;
    if (sodium_crit_leave() != 0) {
        return -1; /* LCOV_EXCL_LINE */
    }
    #if NETCODE_CRYPTO_LOGS
    printf( "\n" );
    #endif // #if NETCODE_CRYPTO_LOGS
    return 0;
}

#ifdef _WIN32

static CRITICAL_SECTION _sodium_lock;
static volatile LONG    _sodium_lock_initialized;

int
_sodium_crit_init(void)
{
    LONG status = 0L;

    while ((status = InterlockedCompareExchange(&_sodium_lock_initialized,
                                                1L, 0L)) == 1L) {
        Sleep(0);
    }

    switch (status) {
    case 0L:
        InitializeCriticalSection(&_sodium_lock);
        return InterlockedExchange(&_sodium_lock_initialized, 2L) == 1L ? 0 : -1;
    case 2L:
        return 0;
    default: /* should never be reached */
        return -1;
    }
}

int
sodium_crit_enter(void)
{
    if (_sodium_crit_init() != 0) {
        return -1; /* LCOV_EXCL_LINE */
    }
    EnterCriticalSection(&_sodium_lock);
    assert(locked == 0);
    locked = 1;

    return 0;
}

int
sodium_crit_leave(void)
{
    if (locked == 0) {
# ifdef EPERM
        errno = EPERM;
# endif
        return -1;
    }
    locked = 0;
    LeaveCriticalSection(&_sodium_lock);

    return 0;
}

#elif defined(HAVE_PTHREAD) && !defined(__EMSCRIPTEN__)

static pthread_mutex_t _sodium_lock = PTHREAD_MUTEX_INITIALIZER;

int
sodium_crit_enter(void)
{
    int ret;

    if ((ret = pthread_mutex_lock(&_sodium_lock)) == 0) {
        assert(locked == 0);
        locked = 1;
    }
    return ret;
}

int
sodium_crit_leave(void)
{
    if (locked == 0) {
# ifdef EPERM
        errno = EPERM;
# endif
        return -1;
    }
    locked = 0;

    return pthread_mutex_unlock(&_sodium_lock);
}

#elif defined(HAVE_ATOMIC_OPS) && !defined(__EMSCRIPTEN__) && !defined(__native_client__)

static volatile int _sodium_lock;

int
sodium_crit_enter(void)
{
# ifdef HAVE_NANOSLEEP
    struct timespec q;
    memset(&q, 0, sizeof q);
# endif
    while (__sync_lock_test_and_set(&_sodium_lock, 1) != 0) {
# ifdef HAVE_NANOSLEEP
        (void) nanosleep(&q, NULL);
# elif defined(__x86_64__) || defined(__i386__)
        __asm__ __volatile__ ("pause");
# endif
    }
    return 0;
}

int
sodium_crit_leave(void)
{
    __sync_lock_release(&_sodium_lock);

    return 0;
}

#else

int
sodium_crit_enter(void)
{
    return 0;
}

int
sodium_crit_leave(void)
{
    return 0;
}

#endif

static void (*_misuse_handler)(void);

void
sodium_misuse(void)
{
    void (*handler)(void);

    (void) sodium_crit_leave();
    if (sodium_crit_enter() == 0) {
        handler = _misuse_handler;
        if (handler != NULL) {
            handler();
        }
    }
/* LCOV_EXCL_START */
    abort();
}
/* LCOV_EXCL_STOP */

int
sodium_set_misuse_handler(void (*handler)(void))
{
    if (sodium_crit_enter() != 0) {
        return -1; /* LCOV_EXCL_LINE */
    }
    _misuse_handler = handler;
    if (sodium_crit_leave() != 0) {
        return -1; /* LCOV_EXCL_LINE */
    }
    return 0;
}

/* ================= sodium_chacha20-ref.c ================= */

/*
 chacha-merged.c version 20080118
 D. J. Bernstein
 Public domain.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>



struct chacha_ctx {
    uint32_t input[16];
};

typedef struct chacha_ctx chacha_ctx;

#define U32C(v) (v##U)

#define U32V(v) ((uint32_t)(v) &U32C(0xFFFFFFFF))

#define ROTATE(v, c) (ROTL32(v, c))
#define XOR(v, w) ((v) ^ (w))
#define PLUS(v, w) (U32V((v) + (w)))
#define PLUSONE(v) (PLUS((v), 1))

#define QUARTERROUND(a, b, c, d) \
    a = PLUS(a, b);              \
    d = ROTATE(XOR(d, a), 16);   \
    c = PLUS(c, d);              \
    b = ROTATE(XOR(b, c), 12);   \
    a = PLUS(a, b);              \
    d = ROTATE(XOR(d, a), 8);    \
    c = PLUS(c, d);              \
    b = ROTATE(XOR(b, c), 7);

static void
chacha_keysetup(chacha_ctx *ctx, const uint8_t *k)
{
    ctx->input[0]  = U32C(0x61707865);
    ctx->input[1]  = U32C(0x3320646e);
    ctx->input[2]  = U32C(0x79622d32);
    ctx->input[3]  = U32C(0x6b206574);
    ctx->input[4]  = LOAD32_LE(k + 0);
    ctx->input[5]  = LOAD32_LE(k + 4);
    ctx->input[6]  = LOAD32_LE(k + 8);
    ctx->input[7]  = LOAD32_LE(k + 12);
    ctx->input[8]  = LOAD32_LE(k + 16);
    ctx->input[9]  = LOAD32_LE(k + 20);
    ctx->input[10] = LOAD32_LE(k + 24);
    ctx->input[11] = LOAD32_LE(k + 28);
}

static void
chacha_ivsetup(chacha_ctx *ctx, const uint8_t *iv, const uint8_t *counter)
{
    ctx->input[12] = counter == NULL ? 0 : LOAD32_LE(counter + 0);
    ctx->input[13] = counter == NULL ? 0 : LOAD32_LE(counter + 4);
    ctx->input[14] = LOAD32_LE(iv + 0);
    ctx->input[15] = LOAD32_LE(iv + 4);
}

static void
chacha_ietf_ivsetup(chacha_ctx *ctx, const uint8_t *iv, const uint8_t *counter)
{
    ctx->input[12] = counter == NULL ? 0 : LOAD32_LE(counter);
    ctx->input[13] = LOAD32_LE(iv + 0);
    ctx->input[14] = LOAD32_LE(iv + 4);
    ctx->input[15] = LOAD32_LE(iv + 8);
}

static void
chacha20_encrypt_bytes(chacha_ctx *ctx, const uint8_t *m, uint8_t *c,
                       unsigned long long bytes)
{
    uint32_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14,
        x15;
    uint32_t j0, j1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14,
        j15;
    uint8_t     *ctarget = NULL;
    uint8_t      tmp[64];
    unsigned int i;

    if (!bytes) {
        return; /* LCOV_EXCL_LINE */
    }
    j0  = ctx->input[0];
    j1  = ctx->input[1];
    j2  = ctx->input[2];
    j3  = ctx->input[3];
    j4  = ctx->input[4];
    j5  = ctx->input[5];
    j6  = ctx->input[6];
    j7  = ctx->input[7];
    j8  = ctx->input[8];
    j9  = ctx->input[9];
    j10 = ctx->input[10];
    j11 = ctx->input[11];
    j12 = ctx->input[12];
    j13 = ctx->input[13];
    j14 = ctx->input[14];
    j15 = ctx->input[15];

    for (;;) {
        if (bytes < 64) {
            memset(tmp, 0, 64);
            for (i = 0; i < bytes; ++i) {
                tmp[i] = m[i];
            }
            m       = tmp;
            ctarget = c;
            c       = tmp;
        }
        x0  = j0;
        x1  = j1;
        x2  = j2;
        x3  = j3;
        x4  = j4;
        x5  = j5;
        x6  = j6;
        x7  = j7;
        x8  = j8;
        x9  = j9;
        x10 = j10;
        x11 = j11;
        x12 = j12;
        x13 = j13;
        x14 = j14;
        x15 = j15;
        for (i = 20; i > 0; i -= 2) {
            QUARTERROUND(x0, x4, x8, x12)
            QUARTERROUND(x1, x5, x9, x13)
            QUARTERROUND(x2, x6, x10, x14)
            QUARTERROUND(x3, x7, x11, x15)
            QUARTERROUND(x0, x5, x10, x15)
            QUARTERROUND(x1, x6, x11, x12)
            QUARTERROUND(x2, x7, x8, x13)
            QUARTERROUND(x3, x4, x9, x14)
        }
        x0  = PLUS(x0, j0);
        x1  = PLUS(x1, j1);
        x2  = PLUS(x2, j2);
        x3  = PLUS(x3, j3);
        x4  = PLUS(x4, j4);
        x5  = PLUS(x5, j5);
        x6  = PLUS(x6, j6);
        x7  = PLUS(x7, j7);
        x8  = PLUS(x8, j8);
        x9  = PLUS(x9, j9);
        x10 = PLUS(x10, j10);
        x11 = PLUS(x11, j11);
        x12 = PLUS(x12, j12);
        x13 = PLUS(x13, j13);
        x14 = PLUS(x14, j14);
        x15 = PLUS(x15, j15);

        x0  = XOR(x0, LOAD32_LE(m + 0));
        x1  = XOR(x1, LOAD32_LE(m + 4));
        x2  = XOR(x2, LOAD32_LE(m + 8));
        x3  = XOR(x3, LOAD32_LE(m + 12));
        x4  = XOR(x4, LOAD32_LE(m + 16));
        x5  = XOR(x5, LOAD32_LE(m + 20));
        x6  = XOR(x6, LOAD32_LE(m + 24));
        x7  = XOR(x7, LOAD32_LE(m + 28));
        x8  = XOR(x8, LOAD32_LE(m + 32));
        x9  = XOR(x9, LOAD32_LE(m + 36));
        x10 = XOR(x10, LOAD32_LE(m + 40));
        x11 = XOR(x11, LOAD32_LE(m + 44));
        x12 = XOR(x12, LOAD32_LE(m + 48));
        x13 = XOR(x13, LOAD32_LE(m + 52));
        x14 = XOR(x14, LOAD32_LE(m + 56));
        x15 = XOR(x15, LOAD32_LE(m + 60));

        j12 = PLUSONE(j12);
        /* LCOV_EXCL_START */
        if (!j12) {
            j13 = PLUSONE(j13);
        }
        /* LCOV_EXCL_STOP */

        STORE32_LE(c + 0, x0);
        STORE32_LE(c + 4, x1);
        STORE32_LE(c + 8, x2);
        STORE32_LE(c + 12, x3);
        STORE32_LE(c + 16, x4);
        STORE32_LE(c + 20, x5);
        STORE32_LE(c + 24, x6);
        STORE32_LE(c + 28, x7);
        STORE32_LE(c + 32, x8);
        STORE32_LE(c + 36, x9);
        STORE32_LE(c + 40, x10);
        STORE32_LE(c + 44, x11);
        STORE32_LE(c + 48, x12);
        STORE32_LE(c + 52, x13);
        STORE32_LE(c + 56, x14);
        STORE32_LE(c + 60, x15);

        if (bytes <= 64) {
            if (bytes < 64) {
                for (i = 0; i < (unsigned int) bytes; ++i) {
                    ctarget[i] = c[i]; /* ctarget cannot be NULL */
                }
            }
            ctx->input[12] = j12;
            ctx->input[13] = j13;

            return;
        }
        bytes -= 64;
        c += 64;
        m += 64;
    }
}

static int
stream_ref(unsigned char *c, unsigned long long clen, const unsigned char *n,
           const unsigned char *k)
{
    struct chacha_ctx ctx;

    if (!clen) {
        return 0;
    }
    COMPILER_ASSERT(crypto_stream_chacha20_KEYBYTES == 256 / 8);
    chacha_keysetup(&ctx, k);
    chacha_ivsetup(&ctx, n, NULL);
    memset(c, 0, clen);
    chacha20_encrypt_bytes(&ctx, c, c, clen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ietf_ext_ref(unsigned char *c, unsigned long long clen,
                    const unsigned char *n, const unsigned char *k)
{
    struct chacha_ctx ctx;

    if (!clen) {
        return 0;
    }
    COMPILER_ASSERT(crypto_stream_chacha20_KEYBYTES == 256 / 8);
    chacha_keysetup(&ctx, k);
    chacha_ietf_ivsetup(&ctx, n, NULL);
    memset(c, 0, clen);
    chacha20_encrypt_bytes(&ctx, c, c, clen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ref_xor_ic(unsigned char *c, const unsigned char *m,
                  unsigned long long mlen, const unsigned char *n, uint64_t ic,
                  const unsigned char *k)
{
    struct chacha_ctx ctx;
    uint8_t           ic_bytes[8];
    uint32_t          ic_high;
    uint32_t          ic_low;

    if (!mlen) {
        return 0;
    }
    ic_high = U32V(ic >> 32);
    ic_low  = U32V(ic);
    STORE32_LE(&ic_bytes[0], ic_low);
    STORE32_LE(&ic_bytes[4], ic_high);
    chacha_keysetup(&ctx, k);
    chacha_ivsetup(&ctx, n, ic_bytes);
    chacha20_encrypt_bytes(&ctx, m, c, mlen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ietf_ext_ref_xor_ic(unsigned char *c, const unsigned char *m,
                           unsigned long long mlen, const unsigned char *n,
                           uint32_t ic, const unsigned char *k)
{
    struct chacha_ctx ctx;
    uint8_t           ic_bytes[4];

    if (!mlen) {
        return 0;
    }
    STORE32_LE(ic_bytes, ic);
    chacha_keysetup(&ctx, k);
    chacha_ietf_ivsetup(&ctx, n, ic_bytes);
    chacha20_encrypt_bytes(&ctx, m, c, mlen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

struct crypto_stream_chacha20_implementation
    crypto_stream_chacha20_ref_implementation = {
        SODIUM_C99(.stream =) stream_ref,
        SODIUM_C99(.stream_ietf_ext =) stream_ietf_ext_ref,
        SODIUM_C99(.stream_xor_ic =) stream_ref_xor_ic,
        SODIUM_C99(.stream_ietf_ext_xor_ic =) stream_ietf_ext_ref_xor_ic
    };
#undef U32C
#undef U32V
#undef ROTATE
#undef XOR
#undef PLUS
#undef PLUSONE
#undef QUARTERROUND

/* ================= sodium_dolbeau_chacha20-ssse3.c ================= */
#define chacha_ctx chacha_ctx__ssse3
#define chacha_keysetup chacha_keysetup__ssse3
#define chacha_ivsetup chacha_ivsetup__ssse3
#define chacha_ietf_ivsetup chacha_ietf_ivsetup__ssse3
#define chacha20_encrypt_bytes chacha20_encrypt_bytes__ssse3
#define stream_ref stream_ref__ssse3
#define stream_ietf_ext_ref stream_ietf_ext_ref__ssse3
#define stream_ref_xor_ic stream_ref_xor_ic__ssse3
#define stream_ietf_ext_ref_xor_ic stream_ietf_ext_ref_xor_ic__ssse3

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#if defined(HAVE_EMMINTRIN_H) && defined(HAVE_TMMINTRIN_H)

# ifdef __clang__
#  pragma clang attribute push(__attribute__((target("sse2,ssse3"))), apply_to = function)
# elif defined(__GNUC__)
#  pragma GCC target("sse2,ssse3")
# endif

# include <emmintrin.h>
# include <tmmintrin.h>


# define ROUNDS 20

typedef struct chacha_ctx {
    uint32_t input[16];
} chacha_ctx;

static void
chacha_keysetup(chacha_ctx *ctx, const uint8_t *k)
{
    ctx->input[0]  = 0x61707865;
    ctx->input[1]  = 0x3320646e;
    ctx->input[2]  = 0x79622d32;
    ctx->input[3]  = 0x6b206574;
    ctx->input[4]  = LOAD32_LE(k + 0);
    ctx->input[5]  = LOAD32_LE(k + 4);
    ctx->input[6]  = LOAD32_LE(k + 8);
    ctx->input[7]  = LOAD32_LE(k + 12);
    ctx->input[8]  = LOAD32_LE(k + 16);
    ctx->input[9]  = LOAD32_LE(k + 20);
    ctx->input[10] = LOAD32_LE(k + 24);
    ctx->input[11] = LOAD32_LE(k + 28);
}

static void
chacha_ivsetup(chacha_ctx *ctx, const uint8_t *iv, const uint8_t *counter)
{
    ctx->input[12] = counter == NULL ? 0 : LOAD32_LE(counter + 0);
    ctx->input[13] = counter == NULL ? 0 : LOAD32_LE(counter + 4);
    ctx->input[14] = LOAD32_LE(iv + 0);
    ctx->input[15] = LOAD32_LE(iv + 4);
}

static void
chacha_ietf_ivsetup(chacha_ctx *ctx, const uint8_t *iv, const uint8_t *counter)
{
    ctx->input[12] = counter == NULL ? 0 : LOAD32_LE(counter);
    ctx->input[13] = LOAD32_LE(iv + 0);
    ctx->input[14] = LOAD32_LE(iv + 4);
    ctx->input[15] = LOAD32_LE(iv + 8);
}

static void
chacha20_encrypt_bytes(chacha_ctx *ctx, const uint8_t *m, uint8_t *c,
                       unsigned long long bytes)
{
    uint32_t * const x = &ctx->input[0];

    if (!bytes) {
        return; /* LCOV_EXCL_LINE */
    }
/* inlined sodium_dolbeau_u4.h */

#define VEC4_ROT(A, IMM) \
    _mm_or_si128(_mm_slli_epi32(A, IMM), _mm_srli_epi32(A, (32 - IMM)))

/* same, but replace 2 of the shift/shift/or "rotation" by byte shuffles (8 &
 * 16) (better) */
#define VEC4_QUARTERROUND_SHUFFLE(A, B, C, D) \
    x_##A = _mm_add_epi32(x_##A, x_##B);      \
    t_##A = _mm_xor_si128(x_##D, x_##A);      \
    x_##D = _mm_shuffle_epi8(t_##A, rot16);   \
    x_##C = _mm_add_epi32(x_##C, x_##D);      \
    t_##C = _mm_xor_si128(x_##B, x_##C);      \
    x_##B = VEC4_ROT(t_##C, 12);              \
    x_##A = _mm_add_epi32(x_##A, x_##B);      \
    t_##A = _mm_xor_si128(x_##D, x_##A);      \
    x_##D = _mm_shuffle_epi8(t_##A, rot8);    \
    x_##C = _mm_add_epi32(x_##C, x_##D);      \
    t_##C = _mm_xor_si128(x_##B, x_##C);      \
    x_##B = VEC4_ROT(t_##C, 7)

#define VEC4_QUARTERROUND(A, B, C, D) VEC4_QUARTERROUND_SHUFFLE(A, B, C, D)

if (bytes >= 256) {
    /* constant for shuffling bytes (replacing multiple-of-8 rotates) */
    __m128i rot16 =
        _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
    __m128i rot8 =
        _mm_set_epi8(14, 13, 12, 15, 10, 9, 8, 11, 6, 5, 4, 7, 2, 1, 0, 3);

    __m128i x_0  = _mm_set1_epi32(x[0]);
    __m128i x_1  = _mm_set1_epi32(x[1]);
    __m128i x_2  = _mm_set1_epi32(x[2]);
    __m128i x_3  = _mm_set1_epi32(x[3]);
    __m128i x_4  = _mm_set1_epi32(x[4]);
    __m128i x_5  = _mm_set1_epi32(x[5]);
    __m128i x_6  = _mm_set1_epi32(x[6]);
    __m128i x_7  = _mm_set1_epi32(x[7]);
    __m128i x_8  = _mm_set1_epi32(x[8]);
    __m128i x_9  = _mm_set1_epi32(x[9]);
    __m128i x_10 = _mm_set1_epi32(x[10]);
    __m128i x_11 = _mm_set1_epi32(x[11]);
    __m128i x_12;
    __m128i x_13;
    __m128i x_14   = _mm_set1_epi32(x[14]);
    __m128i x_15   = _mm_set1_epi32(x[15]);
    __m128i orig0  = x_0;
    __m128i orig1  = x_1;
    __m128i orig2  = x_2;
    __m128i orig3  = x_3;
    __m128i orig4  = x_4;
    __m128i orig5  = x_5;
    __m128i orig6  = x_6;
    __m128i orig7  = x_7;
    __m128i orig8  = x_8;
    __m128i orig9  = x_9;
    __m128i orig10 = x_10;
    __m128i orig11 = x_11;
    __m128i orig12;
    __m128i orig13;
    __m128i orig14 = x_14;
    __m128i orig15 = x_15;
    __m128i t_0, t_1, t_2, t_3, t_4, t_5, t_6, t_7, t_8, t_9, t_10, t_11, t_12,
        t_13, t_14, t_15;

    uint32_t in12, in13;
    int      i;

    while (bytes >= 256) {
        const __m128i addv12 = _mm_set_epi64x(1, 0);
        const __m128i addv13 = _mm_set_epi64x(3, 2);
        __m128i       t12, t13;
        uint64_t      in1213;

        x_0  = orig0;
        x_1  = orig1;
        x_2  = orig2;
        x_3  = orig3;
        x_4  = orig4;
        x_5  = orig5;
        x_6  = orig6;
        x_7  = orig7;
        x_8  = orig8;
        x_9  = orig9;
        x_10 = orig10;
        x_11 = orig11;
        x_14 = orig14;
        x_15 = orig15;

        in12   = x[12];
        in13   = x[13];
        in1213 = ((uint64_t) in12) | (((uint64_t) in13) << 32);
        t12    = _mm_set1_epi64x(in1213);
        t13    = _mm_set1_epi64x(in1213);

        x_12 = _mm_add_epi64(addv12, t12);
        x_13 = _mm_add_epi64(addv13, t13);

        t12 = _mm_unpacklo_epi32(x_12, x_13);
        t13 = _mm_unpackhi_epi32(x_12, x_13);

        x_12 = _mm_unpacklo_epi32(t12, t13);
        x_13 = _mm_unpackhi_epi32(t12, t13);

        orig12 = x_12;
        orig13 = x_13;

        in1213 += 4;

        x[12] = in1213 & 0xFFFFFFFF;
        x[13] = (in1213 >> 32) & 0xFFFFFFFF;

        for (i = 0; i < ROUNDS; i += 2) {
            VEC4_QUARTERROUND(0, 4, 8, 12);
            VEC4_QUARTERROUND(1, 5, 9, 13);
            VEC4_QUARTERROUND(2, 6, 10, 14);
            VEC4_QUARTERROUND(3, 7, 11, 15);
            VEC4_QUARTERROUND(0, 5, 10, 15);
            VEC4_QUARTERROUND(1, 6, 11, 12);
            VEC4_QUARTERROUND(2, 7, 8, 13);
            VEC4_QUARTERROUND(3, 4, 9, 14);
        }

#define ONEQUAD_TRANSPOSE(A, B, C, D)                                          \
    {                                                                          \
        __m128i t0, t1, t2, t3;                                                \
                                                                               \
        x_##A = _mm_add_epi32(x_##A, orig##A);                                 \
        x_##B = _mm_add_epi32(x_##B, orig##B);                                 \
        x_##C = _mm_add_epi32(x_##C, orig##C);                                 \
        x_##D = _mm_add_epi32(x_##D, orig##D);                                 \
        t_##A = _mm_unpacklo_epi32(x_##A, x_##B);                              \
        t_##B = _mm_unpacklo_epi32(x_##C, x_##D);                              \
        t_##C = _mm_unpackhi_epi32(x_##A, x_##B);                              \
        t_##D = _mm_unpackhi_epi32(x_##C, x_##D);                              \
        x_##A = _mm_unpacklo_epi64(t_##A, t_##B);                              \
        x_##B = _mm_unpackhi_epi64(t_##A, t_##B);                              \
        x_##C = _mm_unpacklo_epi64(t_##C, t_##D);                              \
        x_##D = _mm_unpackhi_epi64(t_##C, t_##D);                              \
                                                                               \
        t0 = _mm_xor_si128(x_##A, _mm_loadu_si128((const __m128i*) (m + 0)));  \
        _mm_storeu_si128((__m128i*) (c + 0), t0);                              \
        t1 = _mm_xor_si128(x_##B, _mm_loadu_si128((const __m128i*) (m + 64))); \
        _mm_storeu_si128((__m128i*) (c + 64), t1);                             \
        t2 =                                                                   \
            _mm_xor_si128(x_##C, _mm_loadu_si128((const __m128i*) (m + 128))); \
        _mm_storeu_si128((__m128i*) (c + 128), t2);                            \
        t3 =                                                                   \
            _mm_xor_si128(x_##D, _mm_loadu_si128((const __m128i*) (m + 192))); \
        _mm_storeu_si128((__m128i*) (c + 192), t3);                            \
    }

#define ONEQUAD(A, B, C, D) ONEQUAD_TRANSPOSE(A, B, C, D)

        ONEQUAD(0, 1, 2, 3);
        m += 16;
        c += 16;
        ONEQUAD(4, 5, 6, 7);
        m += 16;
        c += 16;
        ONEQUAD(8, 9, 10, 11);
        m += 16;
        c += 16;
        ONEQUAD(12, 13, 14, 15);
        m -= 48;
        c -= 48;

#undef ONEQUAD
#undef ONEQUAD_TRANSPOSE

        bytes -= 256;
        c += 256;
        m += 256;
    }
}
#undef VEC4_ROT
#undef VEC4_QUARTERROUND
#undef VEC4_QUARTERROUND_SHUFFLE
/* inlined sodium_dolbeau_u1.h */
while (bytes >= 64) {
    __m128i       x_0, x_1, x_2, x_3;
    __m128i       t_1;
    const __m128i rot16 =
        _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
    const __m128i rot8 =
        _mm_set_epi8(14, 13, 12, 15, 10, 9, 8, 11, 6, 5, 4, 7, 2, 1, 0, 3);

    uint32_t in12;
    uint32_t in13;
    int      i;

    x_0 = _mm_loadu_si128((const __m128i*) (x + 0));
    x_1 = _mm_loadu_si128((const __m128i*) (x + 4));
    x_2 = _mm_loadu_si128((const __m128i*) (x + 8));
    x_3 = _mm_loadu_si128((const __m128i*) (x + 12));

    for (i = 0; i < ROUNDS; i += 2) {
        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_3 = _mm_shuffle_epi8(x_3, rot16);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_1 = _mm_xor_si128(x_1, x_2);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 12);
        t_1 = _mm_srli_epi32(t_1, 20);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_0 = _mm_shuffle_epi32(x_0, 0x93);
        x_3 = _mm_shuffle_epi8(x_3, rot8);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_3 = _mm_shuffle_epi32(x_3, 0x4e);
        x_1 = _mm_xor_si128(x_1, x_2);
        x_2 = _mm_shuffle_epi32(x_2, 0x39);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 7);
        t_1 = _mm_srli_epi32(t_1, 25);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_3 = _mm_shuffle_epi8(x_3, rot16);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_1 = _mm_xor_si128(x_1, x_2);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 12);
        t_1 = _mm_srli_epi32(t_1, 20);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_0 = _mm_shuffle_epi32(x_0, 0x39);
        x_3 = _mm_shuffle_epi8(x_3, rot8);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_3 = _mm_shuffle_epi32(x_3, 0x4e);
        x_1 = _mm_xor_si128(x_1, x_2);
        x_2 = _mm_shuffle_epi32(x_2, 0x93);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 7);
        t_1 = _mm_srli_epi32(t_1, 25);
        x_1 = _mm_xor_si128(x_1, t_1);
    }
    x_0 = _mm_add_epi32(x_0, _mm_loadu_si128((const __m128i*) (x + 0)));
    x_1 = _mm_add_epi32(x_1, _mm_loadu_si128((const __m128i*) (x + 4)));
    x_2 = _mm_add_epi32(x_2, _mm_loadu_si128((const __m128i*) (x + 8)));
    x_3 = _mm_add_epi32(x_3, _mm_loadu_si128((const __m128i*) (x + 12)));
    x_0 = _mm_xor_si128(x_0, _mm_loadu_si128((const __m128i*) (m + 0)));
    x_1 = _mm_xor_si128(x_1, _mm_loadu_si128((const __m128i*) (m + 16)));
    x_2 = _mm_xor_si128(x_2, _mm_loadu_si128((const __m128i*) (m + 32)));
    x_3 = _mm_xor_si128(x_3, _mm_loadu_si128((const __m128i*) (m + 48)));
    _mm_storeu_si128((__m128i*) (c + 0), x_0);
    _mm_storeu_si128((__m128i*) (c + 16), x_1);
    _mm_storeu_si128((__m128i*) (c + 32), x_2);
    _mm_storeu_si128((__m128i*) (c + 48), x_3);

    in12 = x[12];
    in13 = x[13];
    in12++;
    if (in12 == 0) {
        in13++;
    }
    x[12] = in12;
    x[13] = in13;

    bytes -= 64;
    c += 64;
    m += 64;
}
/* inlined sodium_dolbeau_u0.h */
if (bytes > 0) {
    __m128i       x_0, x_1, x_2, x_3;
    __m128i       t_1;
    const __m128i rot16 =
        _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
    const __m128i rot8 =
        _mm_set_epi8(14, 13, 12, 15, 10, 9, 8, 11, 6, 5, 4, 7, 2, 1, 0, 3);
    uint8_t partialblock[64];

    unsigned int i;

    x_0 = _mm_loadu_si128((const __m128i*) (x + 0));
    x_1 = _mm_loadu_si128((const __m128i*) (x + 4));
    x_2 = _mm_loadu_si128((const __m128i*) (x + 8));
    x_3 = _mm_loadu_si128((const __m128i*) (x + 12));

    for (i = 0; i < ROUNDS; i += 2) {
        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_3 = _mm_shuffle_epi8(x_3, rot16);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_1 = _mm_xor_si128(x_1, x_2);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 12);
        t_1 = _mm_srli_epi32(t_1, 20);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_0 = _mm_shuffle_epi32(x_0, 0x93);
        x_3 = _mm_shuffle_epi8(x_3, rot8);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_3 = _mm_shuffle_epi32(x_3, 0x4e);
        x_1 = _mm_xor_si128(x_1, x_2);
        x_2 = _mm_shuffle_epi32(x_2, 0x39);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 7);
        t_1 = _mm_srli_epi32(t_1, 25);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_3 = _mm_shuffle_epi8(x_3, rot16);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_1 = _mm_xor_si128(x_1, x_2);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 12);
        t_1 = _mm_srli_epi32(t_1, 20);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_0 = _mm_shuffle_epi32(x_0, 0x39);
        x_3 = _mm_shuffle_epi8(x_3, rot8);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_3 = _mm_shuffle_epi32(x_3, 0x4e);
        x_1 = _mm_xor_si128(x_1, x_2);
        x_2 = _mm_shuffle_epi32(x_2, 0x93);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 7);
        t_1 = _mm_srli_epi32(t_1, 25);
        x_1 = _mm_xor_si128(x_1, t_1);
    }
    x_0 = _mm_add_epi32(x_0, _mm_loadu_si128((const __m128i*) (x + 0)));
    x_1 = _mm_add_epi32(x_1, _mm_loadu_si128((const __m128i*) (x + 4)));
    x_2 = _mm_add_epi32(x_2, _mm_loadu_si128((const __m128i*) (x + 8)));
    x_3 = _mm_add_epi32(x_3, _mm_loadu_si128((const __m128i*) (x + 12)));
    _mm_storeu_si128((__m128i*) (partialblock + 0), x_0);
    _mm_storeu_si128((__m128i*) (partialblock + 16), x_1);
    _mm_storeu_si128((__m128i*) (partialblock + 32), x_2);
    _mm_storeu_si128((__m128i*) (partialblock + 48), x_3);

    for (i = 0; i < bytes; i++) {
        c[i] = m[i] ^ partialblock[i];
    }

    sodium_memzero(partialblock, sizeof partialblock);
}
}

static int
stream_ref(unsigned char *c, unsigned long long clen, const unsigned char *n,
           const unsigned char *k)
{
    struct chacha_ctx ctx;

    if (!clen) {
        return 0;
    }
    COMPILER_ASSERT(crypto_stream_chacha20_KEYBYTES == 256 / 8);
    chacha_keysetup(&ctx, k);
    chacha_ivsetup(&ctx, n, NULL);
    memset(c, 0, clen);
    chacha20_encrypt_bytes(&ctx, c, c, clen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ietf_ext_ref(unsigned char *c, unsigned long long clen,
                    const unsigned char *n, const unsigned char *k)
{
    struct chacha_ctx ctx;

    if (!clen) {
        return 0;
    }
    COMPILER_ASSERT(crypto_stream_chacha20_KEYBYTES == 256 / 8);
    chacha_keysetup(&ctx, k);
    chacha_ietf_ivsetup(&ctx, n, NULL);
    memset(c, 0, clen);
    chacha20_encrypt_bytes(&ctx, c, c, clen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ref_xor_ic(unsigned char *c, const unsigned char *m,
                  unsigned long long mlen, const unsigned char *n, uint64_t ic,
                  const unsigned char *k)
{
    struct chacha_ctx ctx;
    uint8_t           ic_bytes[8];
    uint32_t          ic_high;
    uint32_t          ic_low;

    if (!mlen) {
        return 0;
    }
    ic_high = (uint32_t) (ic >> 32);
    ic_low  = (uint32_t) ic;
    STORE32_LE(&ic_bytes[0], ic_low);
    STORE32_LE(&ic_bytes[4], ic_high);
    chacha_keysetup(&ctx, k);
    chacha_ivsetup(&ctx, n, ic_bytes);
    chacha20_encrypt_bytes(&ctx, m, c, mlen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ietf_ext_ref_xor_ic(unsigned char *c, const unsigned char *m,
                           unsigned long long mlen, const unsigned char *n,
                           uint32_t ic, const unsigned char *k)
{
    struct chacha_ctx ctx;
    uint8_t           ic_bytes[4];

    if (!mlen) {
        return 0;
    }
    STORE32_LE(ic_bytes, ic);
    chacha_keysetup(&ctx, k);
    chacha_ietf_ivsetup(&ctx, n, ic_bytes);
    chacha20_encrypt_bytes(&ctx, m, c, mlen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

struct crypto_stream_chacha20_implementation
    crypto_stream_chacha20_dolbeau_ssse3_implementation = {
        SODIUM_C99(.stream =) stream_ref,
        SODIUM_C99(.stream_ietf_ext =) stream_ietf_ext_ref,
        SODIUM_C99(.stream_xor_ic =) stream_ref_xor_ic,
        SODIUM_C99(.stream_ietf_ext_xor_ic =) stream_ietf_ext_ref_xor_ic
    };

# ifdef __clang__
#  pragma clang attribute pop
# endif

#endif

int chacha20_dolbeau_ssse3_link_warning_dummy = 0;
#undef chacha_ctx
#undef chacha_keysetup
#undef chacha_ivsetup
#undef chacha_ietf_ivsetup
#undef chacha20_encrypt_bytes
#undef stream_ref
#undef stream_ietf_ext_ref
#undef stream_ref_xor_ic
#undef stream_ietf_ext_ref_xor_ic
#undef ROUNDS
#undef VEC4_ROT
#undef VEC4_QUARTERROUND_SHUFFLE
#undef VEC4_QUARTERROUND
#undef ONEQUAD_TRANSPOSE
#undef ONEQUAD

/* ================= sodium_dolbeau_chacha20-avx2.c ================= */
#define chacha_ctx chacha_ctx__avx2
#define chacha_keysetup chacha_keysetup__avx2
#define chacha_ivsetup chacha_ivsetup__avx2
#define chacha_ietf_ivsetup chacha_ietf_ivsetup__avx2
#define chacha20_encrypt_bytes chacha20_encrypt_bytes__avx2
#define stream_ref stream_ref__avx2
#define stream_ietf_ext_ref stream_ietf_ext_ref__avx2
#define stream_ref_xor_ic stream_ref_xor_ic__avx2
#define stream_ietf_ext_ref_xor_ic stream_ietf_ext_ref_xor_ic__avx2

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#if defined(HAVE_AVX2INTRIN_H) && defined(HAVE_EMMINTRIN_H) && \
        defined(HAVE_TMMINTRIN_H) && defined(HAVE_SMMINTRIN_H)

# ifdef __clang__
#  pragma clang attribute push(__attribute__((target("sse2,ssse3,sse4.1,avx2"))), apply_to = function)
# elif defined(__GNUC__)
#  pragma GCC target("sse2,ssse3,sse4.1,avx2")
# endif

# include <emmintrin.h>
# include <immintrin.h>
# include <smmintrin.h>
# include <tmmintrin.h>


# define ROUNDS 20

typedef struct chacha_ctx {
    uint32_t input[16];
} chacha_ctx;

static void
chacha_keysetup(chacha_ctx *ctx, const uint8_t *k)
{
    ctx->input[0]  = 0x61707865;
    ctx->input[1]  = 0x3320646e;
    ctx->input[2]  = 0x79622d32;
    ctx->input[3]  = 0x6b206574;
    ctx->input[4]  = LOAD32_LE(k + 0);
    ctx->input[5]  = LOAD32_LE(k + 4);
    ctx->input[6]  = LOAD32_LE(k + 8);
    ctx->input[7]  = LOAD32_LE(k + 12);
    ctx->input[8]  = LOAD32_LE(k + 16);
    ctx->input[9]  = LOAD32_LE(k + 20);
    ctx->input[10] = LOAD32_LE(k + 24);
    ctx->input[11] = LOAD32_LE(k + 28);
}

static void
chacha_ivsetup(chacha_ctx *ctx, const uint8_t *iv, const uint8_t *counter)
{
    ctx->input[12] = counter == NULL ? 0 : LOAD32_LE(counter + 0);
    ctx->input[13] = counter == NULL ? 0 : LOAD32_LE(counter + 4);
    ctx->input[14] = LOAD32_LE(iv + 0);
    ctx->input[15] = LOAD32_LE(iv + 4);
}

static void
chacha_ietf_ivsetup(chacha_ctx *ctx, const uint8_t *iv, const uint8_t *counter)
{
    ctx->input[12] = counter == NULL ? 0 : LOAD32_LE(counter);
    ctx->input[13] = LOAD32_LE(iv + 0);
    ctx->input[14] = LOAD32_LE(iv + 4);
    ctx->input[15] = LOAD32_LE(iv + 8);
}

static void
chacha20_encrypt_bytes(chacha_ctx *ctx, const uint8_t *m, uint8_t *c,
                       unsigned long long bytes)
{
    uint32_t * const x = &ctx->input[0];

    if (!bytes) {
        return; /* LCOV_EXCL_LINE */
    }
/* inlined sodium_dolbeau_u8.h */

#define VEC8_ROT(A, IMM) \
    _mm256_or_si256(_mm256_slli_epi32(A, IMM), _mm256_srli_epi32(A, (32 - IMM)))

/* implements a vector quarter round by-the-book (naive!) */
#define VEC8_QUARTERROUND_NAIVE(A, B, C, D) \
    x_##A = _mm256_add_epi32(x_##A, x_##B); \
    t_##A = _mm256_xor_si256(x_##D, x_##A); \
    x_##D = VEC8_ROT(t_##A, 16);            \
    x_##C = _mm256_add_epi32(x_##C, x_##D); \
    t_##C = _mm256_xor_si256(x_##B, x_##C); \
    x_##B = VEC8_ROT(t_##C, 12);            \
    x_##A = _mm256_add_epi32(x_##A, x_##B); \
    t_##A = _mm256_xor_si256(x_##D, x_##A); \
    x_##D = VEC8_ROT(t_##A, 8);             \
    x_##C = _mm256_add_epi32(x_##C, x_##D); \
    t_##C = _mm256_xor_si256(x_##B, x_##C); \
    x_##B = VEC8_ROT(t_##C, 7)

/* same, but replace 2 of the shift/shift/or "rotation" by byte shuffles (8 &
 * 16) (better) */
#define VEC8_QUARTERROUND_SHUFFLE(A, B, C, D)  \
    x_##A = _mm256_add_epi32(x_##A, x_##B);    \
    t_##A = _mm256_xor_si256(x_##D, x_##A);    \
    x_##D = _mm256_shuffle_epi8(t_##A, rot16); \
    x_##C = _mm256_add_epi32(x_##C, x_##D);    \
    t_##C = _mm256_xor_si256(x_##B, x_##C);    \
    x_##B = VEC8_ROT(t_##C, 12);               \
    x_##A = _mm256_add_epi32(x_##A, x_##B);    \
    t_##A = _mm256_xor_si256(x_##D, x_##A);    \
    x_##D = _mm256_shuffle_epi8(t_##A, rot8);  \
    x_##C = _mm256_add_epi32(x_##C, x_##D);    \
    t_##C = _mm256_xor_si256(x_##B, x_##C);    \
    x_##B = VEC8_ROT(t_##C, 7)

/* same, but replace 2 of the shift/shift/or "rotation" by byte & word shuffles
 * (8 & 16) (not as good as previous) */
#define VEC8_QUARTERROUND_SHUFFLE2(A, B, C, D)                                 \
    x_##A = _mm256_add_epi32(x_##A, x_##B);                                    \
    t_##A = _mm256_xor_si256(x_##D, x_##A);                                    \
    x_##D = _mm256_shufflehi_epi16(_mm256_shufflelo_epi16(t_##A, 0xb1), 0xb1); \
    x_##C = _mm256_add_epi32(x_##C, x_##D);                                    \
    t_##C = _mm256_xor_si256(x_##B, x_##C);                                    \
    x_##B = VEC8_ROT(t_##C, 12);                                               \
    x_##A = _mm256_add_epi32(x_##A, x_##B);                                    \
    t_##A = _mm256_xor_si256(x_##D, x_##A);                                    \
    x_##D = _mm256_shuffle_epi8(t_##A, rot8);                                  \
    x_##C = _mm256_add_epi32(x_##C, x_##D);                                    \
    t_##C = _mm256_xor_si256(x_##B, x_##C);                                    \
    x_##B = VEC8_ROT(t_##C, 7)

#define VEC8_QUARTERROUND(A, B, C, D) VEC8_QUARTERROUND_SHUFFLE(A, B, C, D)

#define VEC8_LINE1(A, B, C, D)              \
    x_##A = _mm256_add_epi32(x_##A, x_##B); \
    x_##D = _mm256_shuffle_epi8(_mm256_xor_si256(x_##D, x_##A), rot16)
#define VEC8_LINE2(A, B, C, D)              \
    x_##C = _mm256_add_epi32(x_##C, x_##D); \
    x_##B = VEC8_ROT(_mm256_xor_si256(x_##B, x_##C), 12)
#define VEC8_LINE3(A, B, C, D)              \
    x_##A = _mm256_add_epi32(x_##A, x_##B); \
    x_##D = _mm256_shuffle_epi8(_mm256_xor_si256(x_##D, x_##A), rot8)
#define VEC8_LINE4(A, B, C, D)              \
    x_##C = _mm256_add_epi32(x_##C, x_##D); \
    x_##B = VEC8_ROT(_mm256_xor_si256(x_##B, x_##C), 7)

#define VEC8_ROUND_SEQ(A1, B1, C1, D1, A2, B2, C2, D2, A3, B3, C3, D3, A4, B4, \
                       C4, D4)                                                 \
    VEC8_LINE1(A1, B1, C1, D1);                                                \
    VEC8_LINE1(A2, B2, C2, D2);                                                \
    VEC8_LINE1(A3, B3, C3, D3);                                                \
    VEC8_LINE1(A4, B4, C4, D4);                                                \
    VEC8_LINE2(A1, B1, C1, D1);                                                \
    VEC8_LINE2(A2, B2, C2, D2);                                                \
    VEC8_LINE2(A3, B3, C3, D3);                                                \
    VEC8_LINE2(A4, B4, C4, D4);                                                \
    VEC8_LINE3(A1, B1, C1, D1);                                                \
    VEC8_LINE3(A2, B2, C2, D2);                                                \
    VEC8_LINE3(A3, B3, C3, D3);                                                \
    VEC8_LINE3(A4, B4, C4, D4);                                                \
    VEC8_LINE4(A1, B1, C1, D1);                                                \
    VEC8_LINE4(A2, B2, C2, D2);                                                \
    VEC8_LINE4(A3, B3, C3, D3);                                                \
    VEC8_LINE4(A4, B4, C4, D4)

#define VEC8_ROUND_HALF(A1, B1, C1, D1, A2, B2, C2, D2, A3, B3, C3, D3, A4, \
                        B4, C4, D4)                                         \
    VEC8_LINE1(A1, B1, C1, D1);                                             \
    VEC8_LINE1(A2, B2, C2, D2);                                             \
    VEC8_LINE2(A1, B1, C1, D1);                                             \
    VEC8_LINE2(A2, B2, C2, D2);                                             \
    VEC8_LINE3(A1, B1, C1, D1);                                             \
    VEC8_LINE3(A2, B2, C2, D2);                                             \
    VEC8_LINE4(A1, B1, C1, D1);                                             \
    VEC8_LINE4(A2, B2, C2, D2);                                             \
    VEC8_LINE1(A3, B3, C3, D3);                                             \
    VEC8_LINE1(A4, B4, C4, D4);                                             \
    VEC8_LINE2(A3, B3, C3, D3);                                             \
    VEC8_LINE2(A4, B4, C4, D4);                                             \
    VEC8_LINE3(A3, B3, C3, D3);                                             \
    VEC8_LINE3(A4, B4, C4, D4);                                             \
    VEC8_LINE4(A3, B3, C3, D3);                                             \
    VEC8_LINE4(A4, B4, C4, D4)

#define VEC8_ROUND_HALFANDHALF(A1, B1, C1, D1, A2, B2, C2, D2, A3, B3, C3, D3, \
                               A4, B4, C4, D4)                                 \
    VEC8_LINE1(A1, B1, C1, D1);                                                \
    VEC8_LINE1(A2, B2, C2, D2);                                                \
    VEC8_LINE2(A1, B1, C1, D1);                                                \
    VEC8_LINE2(A2, B2, C2, D2);                                                \
    VEC8_LINE1(A3, B3, C3, D3);                                                \
    VEC8_LINE1(A4, B4, C4, D4);                                                \
    VEC8_LINE2(A3, B3, C3, D3);                                                \
    VEC8_LINE2(A4, B4, C4, D4);                                                \
    VEC8_LINE3(A1, B1, C1, D1);                                                \
    VEC8_LINE3(A2, B2, C2, D2);                                                \
    VEC8_LINE4(A1, B1, C1, D1);                                                \
    VEC8_LINE4(A2, B2, C2, D2);                                                \
    VEC8_LINE3(A3, B3, C3, D3);                                                \
    VEC8_LINE3(A4, B4, C4, D4);                                                \
    VEC8_LINE4(A3, B3, C3, D3);                                                \
    VEC8_LINE4(A4, B4, C4, D4)

#define VEC8_ROUND(A1, B1, C1, D1, A2, B2, C2, D2, A3, B3, C3, D3, A4, B4, C4, \
                   D4)                                                         \
    VEC8_ROUND_SEQ(A1, B1, C1, D1, A2, B2, C2, D2, A3, B3, C3, D3, A4, B4, C4, \
                   D4)

if (bytes >= 512) {
    /* constant for shuffling bytes (replacing multiple-of-8 rotates) */
    __m256i rot16 =
        _mm256_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2,
                        13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
    __m256i rot8 =
        _mm256_set_epi8(14, 13, 12, 15, 10, 9, 8, 11, 6, 5, 4, 7, 2, 1, 0, 3,
                        14, 13, 12, 15, 10, 9, 8, 11, 6, 5, 4, 7, 2, 1, 0, 3);
    uint32_t in12, in13;

    /* the naive way seems as fast (if not a bit faster) than the vector way */
    __m256i x_0  = _mm256_set1_epi32(x[0]);
    __m256i x_1  = _mm256_set1_epi32(x[1]);
    __m256i x_2  = _mm256_set1_epi32(x[2]);
    __m256i x_3  = _mm256_set1_epi32(x[3]);
    __m256i x_4  = _mm256_set1_epi32(x[4]);
    __m256i x_5  = _mm256_set1_epi32(x[5]);
    __m256i x_6  = _mm256_set1_epi32(x[6]);
    __m256i x_7  = _mm256_set1_epi32(x[7]);
    __m256i x_8  = _mm256_set1_epi32(x[8]);
    __m256i x_9  = _mm256_set1_epi32(x[9]);
    __m256i x_10 = _mm256_set1_epi32(x[10]);
    __m256i x_11 = _mm256_set1_epi32(x[11]);
    __m256i x_12;
    __m256i x_13;
    __m256i x_14 = _mm256_set1_epi32(x[14]);
    __m256i x_15 = _mm256_set1_epi32(x[15]);

    __m256i orig0  = x_0;
    __m256i orig1  = x_1;
    __m256i orig2  = x_2;
    __m256i orig3  = x_3;
    __m256i orig4  = x_4;
    __m256i orig5  = x_5;
    __m256i orig6  = x_6;
    __m256i orig7  = x_7;
    __m256i orig8  = x_8;
    __m256i orig9  = x_9;
    __m256i orig10 = x_10;
    __m256i orig11 = x_11;
    __m256i orig12;
    __m256i orig13;
    __m256i orig14 = x_14;
    __m256i orig15 = x_15;
    __m256i t_0, t_1, t_2, t_3, t_4, t_5, t_6, t_7, t_8, t_9, t_10, t_11, t_12,
        t_13, t_14, t_15;

    while (bytes >= 512) {
        const __m256i addv12  = _mm256_set_epi64x(3, 2, 1, 0);
        const __m256i addv13  = _mm256_set_epi64x(7, 6, 5, 4);
        const __m256i permute = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
        __m256i       t12, t13;

        uint64_t in1213;
        int      i;

        x_0  = orig0;
        x_1  = orig1;
        x_2  = orig2;
        x_3  = orig3;
        x_4  = orig4;
        x_5  = orig5;
        x_6  = orig6;
        x_7  = orig7;
        x_8  = orig8;
        x_9  = orig9;
        x_10 = orig10;
        x_11 = orig11;
        x_14 = orig14;
        x_15 = orig15;

        in12   = x[12];
        in13   = x[13];
        in1213 = ((uint64_t) in12) | (((uint64_t) in13) << 32);
        x_12 = x_13 = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(in1213));

        t12 = _mm256_add_epi64(addv12, x_12);
        t13 = _mm256_add_epi64(addv13, x_13);

        x_12 = _mm256_unpacklo_epi32(t12, t13);
        x_13 = _mm256_unpackhi_epi32(t12, t13);

        t12 = _mm256_unpacklo_epi32(x_12, x_13);
        t13 = _mm256_unpackhi_epi32(x_12, x_13);

        /* required because unpack* are intra-lane */
        x_12 = _mm256_permutevar8x32_epi32(t12, permute);
        x_13 = _mm256_permutevar8x32_epi32(t13, permute);

        orig12 = x_12;
        orig13 = x_13;

        in1213 += 8;

        x[12] = in1213 & 0xFFFFFFFF;
        x[13] = (in1213 >> 32) & 0xFFFFFFFF;

        for (i = 0; i < ROUNDS; i += 2) {
            VEC8_ROUND(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);
            VEC8_ROUND(0, 5, 10, 15, 1, 6, 11, 12, 2, 7, 8, 13, 3, 4, 9, 14);
        }

#define ONEQUAD_TRANSPOSE(A, B, C, D)                                     \
    {                                                                     \
        __m128i t0, t1, t2, t3;                                           \
        x_##A = _mm256_add_epi32(x_##A, orig##A);                         \
        x_##B = _mm256_add_epi32(x_##B, orig##B);                         \
        x_##C = _mm256_add_epi32(x_##C, orig##C);                         \
        x_##D = _mm256_add_epi32(x_##D, orig##D);                         \
        t_##A = _mm256_unpacklo_epi32(x_##A, x_##B);                      \
        t_##B = _mm256_unpacklo_epi32(x_##C, x_##D);                      \
        t_##C = _mm256_unpackhi_epi32(x_##A, x_##B);                      \
        t_##D = _mm256_unpackhi_epi32(x_##C, x_##D);                      \
        x_##A = _mm256_unpacklo_epi64(t_##A, t_##B);                      \
        x_##B = _mm256_unpackhi_epi64(t_##A, t_##B);                      \
        x_##C = _mm256_unpacklo_epi64(t_##C, t_##D);                      \
        x_##D = _mm256_unpackhi_epi64(t_##C, t_##D);                      \
        t0    = _mm_xor_si128(_mm256_extracti128_si256(x_##A, 0),         \
                           _mm_loadu_si128((const __m128i*) (m + 0))); \
        _mm_storeu_si128((__m128i*) (c + 0), t0);                         \
        t1 = _mm_xor_si128(_mm256_extracti128_si256(x_##B, 0),            \
                           _mm_loadu_si128((const __m128i*) (m + 64)));   \
        _mm_storeu_si128((__m128i*) (c + 64), t1);                        \
        t2 = _mm_xor_si128(_mm256_extracti128_si256(x_##C, 0),            \
                           _mm_loadu_si128((const __m128i*) (m + 128)));  \
        _mm_storeu_si128((__m128i*) (c + 128), t2);                       \
        t3 = _mm_xor_si128(_mm256_extracti128_si256(x_##D, 0),            \
                           _mm_loadu_si128((const __m128i*) (m + 192)));  \
        _mm_storeu_si128((__m128i*) (c + 192), t3);                       \
        t0 = _mm_xor_si128(_mm256_extracti128_si256(x_##A, 1),            \
                           _mm_loadu_si128((const __m128i*) (m + 256)));  \
        _mm_storeu_si128((__m128i*) (c + 256), t0);                       \
        t1 = _mm_xor_si128(_mm256_extracti128_si256(x_##B, 1),            \
                           _mm_loadu_si128((const __m128i*) (m + 320)));  \
        _mm_storeu_si128((__m128i*) (c + 320), t1);                       \
        t2 = _mm_xor_si128(_mm256_extracti128_si256(x_##C, 1),            \
                           _mm_loadu_si128((const __m128i*) (m + 384)));  \
        _mm_storeu_si128((__m128i*) (c + 384), t2);                       \
        t3 = _mm_xor_si128(_mm256_extracti128_si256(x_##D, 1),            \
                           _mm_loadu_si128((const __m128i*) (m + 448)));  \
        _mm_storeu_si128((__m128i*) (c + 448), t3);                       \
    }

#define ONEQUAD(A, B, C, D) ONEQUAD_TRANSPOSE(A, B, C, D)

#define ONEQUAD_UNPCK(A, B, C, D)                    \
    {                                                \
        x_##A = _mm256_add_epi32(x_##A, orig##A);    \
        x_##B = _mm256_add_epi32(x_##B, orig##B);    \
        x_##C = _mm256_add_epi32(x_##C, orig##C);    \
        x_##D = _mm256_add_epi32(x_##D, orig##D);    \
        t_##A = _mm256_unpacklo_epi32(x_##A, x_##B); \
        t_##B = _mm256_unpacklo_epi32(x_##C, x_##D); \
        t_##C = _mm256_unpackhi_epi32(x_##A, x_##B); \
        t_##D = _mm256_unpackhi_epi32(x_##C, x_##D); \
        x_##A = _mm256_unpacklo_epi64(t_##A, t_##B); \
        x_##B = _mm256_unpackhi_epi64(t_##A, t_##B); \
        x_##C = _mm256_unpacklo_epi64(t_##C, t_##D); \
        x_##D = _mm256_unpackhi_epi64(t_##C, t_##D); \
    }

#define ONEOCTO(A, B, C, D, A2, B2, C2, D2)                          \
    {                                                                \
        ONEQUAD_UNPCK(A, B, C, D);                                   \
        ONEQUAD_UNPCK(A2, B2, C2, D2);                               \
        t_##A  = _mm256_permute2x128_si256(x_##A, x_##A2, 0x20);     \
        t_##A2 = _mm256_permute2x128_si256(x_##A, x_##A2, 0x31);     \
        t_##B  = _mm256_permute2x128_si256(x_##B, x_##B2, 0x20);     \
        t_##B2 = _mm256_permute2x128_si256(x_##B, x_##B2, 0x31);     \
        t_##C  = _mm256_permute2x128_si256(x_##C, x_##C2, 0x20);     \
        t_##C2 = _mm256_permute2x128_si256(x_##C, x_##C2, 0x31);     \
        t_##D  = _mm256_permute2x128_si256(x_##D, x_##D2, 0x20);     \
        t_##D2 = _mm256_permute2x128_si256(x_##D, x_##D2, 0x31);     \
        t_##A  = _mm256_xor_si256(                                   \
            t_##A, _mm256_loadu_si256((const __m256i*) (m + 0)));   \
        t_##B = _mm256_xor_si256(                                    \
            t_##B, _mm256_loadu_si256((const __m256i*) (m + 64)));   \
        t_##C = _mm256_xor_si256(                                    \
            t_##C, _mm256_loadu_si256((const __m256i*) (m + 128)));  \
        t_##D = _mm256_xor_si256(                                    \
            t_##D, _mm256_loadu_si256((const __m256i*) (m + 192)));  \
        t_##A2 = _mm256_xor_si256(                                   \
            t_##A2, _mm256_loadu_si256((const __m256i*) (m + 256))); \
        t_##B2 = _mm256_xor_si256(                                   \
            t_##B2, _mm256_loadu_si256((const __m256i*) (m + 320))); \
        t_##C2 = _mm256_xor_si256(                                   \
            t_##C2, _mm256_loadu_si256((const __m256i*) (m + 384))); \
        t_##D2 = _mm256_xor_si256(                                   \
            t_##D2, _mm256_loadu_si256((const __m256i*) (m + 448))); \
        _mm256_storeu_si256((__m256i*) (c + 0), t_##A);              \
        _mm256_storeu_si256((__m256i*) (c + 64), t_##B);             \
        _mm256_storeu_si256((__m256i*) (c + 128), t_##C);            \
        _mm256_storeu_si256((__m256i*) (c + 192), t_##D);            \
        _mm256_storeu_si256((__m256i*) (c + 256), t_##A2);           \
        _mm256_storeu_si256((__m256i*) (c + 320), t_##B2);           \
        _mm256_storeu_si256((__m256i*) (c + 384), t_##C2);           \
        _mm256_storeu_si256((__m256i*) (c + 448), t_##D2);           \
    }

        ONEOCTO(0, 1, 2, 3, 4, 5, 6, 7);
        m += 32;
        c += 32;
        ONEOCTO(8, 9, 10, 11, 12, 13, 14, 15);
        m -= 32;
        c -= 32;

#undef ONEQUAD
#undef ONEQUAD_TRANSPOSE
#undef ONEQUAD_UNPCK
#undef ONEOCTO

        bytes -= 512;
        c += 512;
        m += 512;
    }
}
#undef VEC8_ROT
#undef VEC8_QUARTERROUND
#undef VEC8_QUARTERROUND_NAIVE
#undef VEC8_QUARTERROUND_SHUFFLE
#undef VEC8_QUARTERROUND_SHUFFLE2
#undef VEC8_LINE1
#undef VEC8_LINE2
#undef VEC8_LINE3
#undef VEC8_LINE4
#undef VEC8_ROUND
#undef VEC8_ROUND_SEQ
#undef VEC8_ROUND_HALF
#undef VEC8_ROUND_HALFANDHALF
/* inlined sodium_dolbeau_u4.h */

#define VEC4_ROT(A, IMM) \
    _mm_or_si128(_mm_slli_epi32(A, IMM), _mm_srli_epi32(A, (32 - IMM)))

/* same, but replace 2 of the shift/shift/or "rotation" by byte shuffles (8 &
 * 16) (better) */
#define VEC4_QUARTERROUND_SHUFFLE(A, B, C, D) \
    x_##A = _mm_add_epi32(x_##A, x_##B);      \
    t_##A = _mm_xor_si128(x_##D, x_##A);      \
    x_##D = _mm_shuffle_epi8(t_##A, rot16);   \
    x_##C = _mm_add_epi32(x_##C, x_##D);      \
    t_##C = _mm_xor_si128(x_##B, x_##C);      \
    x_##B = VEC4_ROT(t_##C, 12);              \
    x_##A = _mm_add_epi32(x_##A, x_##B);      \
    t_##A = _mm_xor_si128(x_##D, x_##A);      \
    x_##D = _mm_shuffle_epi8(t_##A, rot8);    \
    x_##C = _mm_add_epi32(x_##C, x_##D);      \
    t_##C = _mm_xor_si128(x_##B, x_##C);      \
    x_##B = VEC4_ROT(t_##C, 7)

#define VEC4_QUARTERROUND(A, B, C, D) VEC4_QUARTERROUND_SHUFFLE(A, B, C, D)

if (bytes >= 256) {
    /* constant for shuffling bytes (replacing multiple-of-8 rotates) */
    __m128i rot16 =
        _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
    __m128i rot8 =
        _mm_set_epi8(14, 13, 12, 15, 10, 9, 8, 11, 6, 5, 4, 7, 2, 1, 0, 3);

    __m128i x_0  = _mm_set1_epi32(x[0]);
    __m128i x_1  = _mm_set1_epi32(x[1]);
    __m128i x_2  = _mm_set1_epi32(x[2]);
    __m128i x_3  = _mm_set1_epi32(x[3]);
    __m128i x_4  = _mm_set1_epi32(x[4]);
    __m128i x_5  = _mm_set1_epi32(x[5]);
    __m128i x_6  = _mm_set1_epi32(x[6]);
    __m128i x_7  = _mm_set1_epi32(x[7]);
    __m128i x_8  = _mm_set1_epi32(x[8]);
    __m128i x_9  = _mm_set1_epi32(x[9]);
    __m128i x_10 = _mm_set1_epi32(x[10]);
    __m128i x_11 = _mm_set1_epi32(x[11]);
    __m128i x_12;
    __m128i x_13;
    __m128i x_14   = _mm_set1_epi32(x[14]);
    __m128i x_15   = _mm_set1_epi32(x[15]);
    __m128i orig0  = x_0;
    __m128i orig1  = x_1;
    __m128i orig2  = x_2;
    __m128i orig3  = x_3;
    __m128i orig4  = x_4;
    __m128i orig5  = x_5;
    __m128i orig6  = x_6;
    __m128i orig7  = x_7;
    __m128i orig8  = x_8;
    __m128i orig9  = x_9;
    __m128i orig10 = x_10;
    __m128i orig11 = x_11;
    __m128i orig12;
    __m128i orig13;
    __m128i orig14 = x_14;
    __m128i orig15 = x_15;
    __m128i t_0, t_1, t_2, t_3, t_4, t_5, t_6, t_7, t_8, t_9, t_10, t_11, t_12,
        t_13, t_14, t_15;

    uint32_t in12, in13;
    int      i;

    while (bytes >= 256) {
        const __m128i addv12 = _mm_set_epi64x(1, 0);
        const __m128i addv13 = _mm_set_epi64x(3, 2);
        __m128i       t12, t13;
        uint64_t      in1213;

        x_0  = orig0;
        x_1  = orig1;
        x_2  = orig2;
        x_3  = orig3;
        x_4  = orig4;
        x_5  = orig5;
        x_6  = orig6;
        x_7  = orig7;
        x_8  = orig8;
        x_9  = orig9;
        x_10 = orig10;
        x_11 = orig11;
        x_14 = orig14;
        x_15 = orig15;

        in12   = x[12];
        in13   = x[13];
        in1213 = ((uint64_t) in12) | (((uint64_t) in13) << 32);
        t12    = _mm_set1_epi64x(in1213);
        t13    = _mm_set1_epi64x(in1213);

        x_12 = _mm_add_epi64(addv12, t12);
        x_13 = _mm_add_epi64(addv13, t13);

        t12 = _mm_unpacklo_epi32(x_12, x_13);
        t13 = _mm_unpackhi_epi32(x_12, x_13);

        x_12 = _mm_unpacklo_epi32(t12, t13);
        x_13 = _mm_unpackhi_epi32(t12, t13);

        orig12 = x_12;
        orig13 = x_13;

        in1213 += 4;

        x[12] = in1213 & 0xFFFFFFFF;
        x[13] = (in1213 >> 32) & 0xFFFFFFFF;

        for (i = 0; i < ROUNDS; i += 2) {
            VEC4_QUARTERROUND(0, 4, 8, 12);
            VEC4_QUARTERROUND(1, 5, 9, 13);
            VEC4_QUARTERROUND(2, 6, 10, 14);
            VEC4_QUARTERROUND(3, 7, 11, 15);
            VEC4_QUARTERROUND(0, 5, 10, 15);
            VEC4_QUARTERROUND(1, 6, 11, 12);
            VEC4_QUARTERROUND(2, 7, 8, 13);
            VEC4_QUARTERROUND(3, 4, 9, 14);
        }

#define ONEQUAD_TRANSPOSE(A, B, C, D)                                          \
    {                                                                          \
        __m128i t0, t1, t2, t3;                                                \
                                                                               \
        x_##A = _mm_add_epi32(x_##A, orig##A);                                 \
        x_##B = _mm_add_epi32(x_##B, orig##B);                                 \
        x_##C = _mm_add_epi32(x_##C, orig##C);                                 \
        x_##D = _mm_add_epi32(x_##D, orig##D);                                 \
        t_##A = _mm_unpacklo_epi32(x_##A, x_##B);                              \
        t_##B = _mm_unpacklo_epi32(x_##C, x_##D);                              \
        t_##C = _mm_unpackhi_epi32(x_##A, x_##B);                              \
        t_##D = _mm_unpackhi_epi32(x_##C, x_##D);                              \
        x_##A = _mm_unpacklo_epi64(t_##A, t_##B);                              \
        x_##B = _mm_unpackhi_epi64(t_##A, t_##B);                              \
        x_##C = _mm_unpacklo_epi64(t_##C, t_##D);                              \
        x_##D = _mm_unpackhi_epi64(t_##C, t_##D);                              \
                                                                               \
        t0 = _mm_xor_si128(x_##A, _mm_loadu_si128((const __m128i*) (m + 0)));  \
        _mm_storeu_si128((__m128i*) (c + 0), t0);                              \
        t1 = _mm_xor_si128(x_##B, _mm_loadu_si128((const __m128i*) (m + 64))); \
        _mm_storeu_si128((__m128i*) (c + 64), t1);                             \
        t2 =                                                                   \
            _mm_xor_si128(x_##C, _mm_loadu_si128((const __m128i*) (m + 128))); \
        _mm_storeu_si128((__m128i*) (c + 128), t2);                            \
        t3 =                                                                   \
            _mm_xor_si128(x_##D, _mm_loadu_si128((const __m128i*) (m + 192))); \
        _mm_storeu_si128((__m128i*) (c + 192), t3);                            \
    }

#define ONEQUAD(A, B, C, D) ONEQUAD_TRANSPOSE(A, B, C, D)

        ONEQUAD(0, 1, 2, 3);
        m += 16;
        c += 16;
        ONEQUAD(4, 5, 6, 7);
        m += 16;
        c += 16;
        ONEQUAD(8, 9, 10, 11);
        m += 16;
        c += 16;
        ONEQUAD(12, 13, 14, 15);
        m -= 48;
        c -= 48;

#undef ONEQUAD
#undef ONEQUAD_TRANSPOSE

        bytes -= 256;
        c += 256;
        m += 256;
    }
}
#undef VEC4_ROT
#undef VEC4_QUARTERROUND
#undef VEC4_QUARTERROUND_SHUFFLE
/* inlined sodium_dolbeau_u1.h */
while (bytes >= 64) {
    __m128i       x_0, x_1, x_2, x_3;
    __m128i       t_1;
    const __m128i rot16 =
        _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
    const __m128i rot8 =
        _mm_set_epi8(14, 13, 12, 15, 10, 9, 8, 11, 6, 5, 4, 7, 2, 1, 0, 3);

    uint32_t in12;
    uint32_t in13;
    int      i;

    x_0 = _mm_loadu_si128((const __m128i*) (x + 0));
    x_1 = _mm_loadu_si128((const __m128i*) (x + 4));
    x_2 = _mm_loadu_si128((const __m128i*) (x + 8));
    x_3 = _mm_loadu_si128((const __m128i*) (x + 12));

    for (i = 0; i < ROUNDS; i += 2) {
        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_3 = _mm_shuffle_epi8(x_3, rot16);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_1 = _mm_xor_si128(x_1, x_2);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 12);
        t_1 = _mm_srli_epi32(t_1, 20);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_0 = _mm_shuffle_epi32(x_0, 0x93);
        x_3 = _mm_shuffle_epi8(x_3, rot8);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_3 = _mm_shuffle_epi32(x_3, 0x4e);
        x_1 = _mm_xor_si128(x_1, x_2);
        x_2 = _mm_shuffle_epi32(x_2, 0x39);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 7);
        t_1 = _mm_srli_epi32(t_1, 25);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_3 = _mm_shuffle_epi8(x_3, rot16);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_1 = _mm_xor_si128(x_1, x_2);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 12);
        t_1 = _mm_srli_epi32(t_1, 20);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_0 = _mm_shuffle_epi32(x_0, 0x39);
        x_3 = _mm_shuffle_epi8(x_3, rot8);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_3 = _mm_shuffle_epi32(x_3, 0x4e);
        x_1 = _mm_xor_si128(x_1, x_2);
        x_2 = _mm_shuffle_epi32(x_2, 0x93);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 7);
        t_1 = _mm_srli_epi32(t_1, 25);
        x_1 = _mm_xor_si128(x_1, t_1);
    }
    x_0 = _mm_add_epi32(x_0, _mm_loadu_si128((const __m128i*) (x + 0)));
    x_1 = _mm_add_epi32(x_1, _mm_loadu_si128((const __m128i*) (x + 4)));
    x_2 = _mm_add_epi32(x_2, _mm_loadu_si128((const __m128i*) (x + 8)));
    x_3 = _mm_add_epi32(x_3, _mm_loadu_si128((const __m128i*) (x + 12)));
    x_0 = _mm_xor_si128(x_0, _mm_loadu_si128((const __m128i*) (m + 0)));
    x_1 = _mm_xor_si128(x_1, _mm_loadu_si128((const __m128i*) (m + 16)));
    x_2 = _mm_xor_si128(x_2, _mm_loadu_si128((const __m128i*) (m + 32)));
    x_3 = _mm_xor_si128(x_3, _mm_loadu_si128((const __m128i*) (m + 48)));
    _mm_storeu_si128((__m128i*) (c + 0), x_0);
    _mm_storeu_si128((__m128i*) (c + 16), x_1);
    _mm_storeu_si128((__m128i*) (c + 32), x_2);
    _mm_storeu_si128((__m128i*) (c + 48), x_3);

    in12 = x[12];
    in13 = x[13];
    in12++;
    if (in12 == 0) {
        in13++;
    }
    x[12] = in12;
    x[13] = in13;

    bytes -= 64;
    c += 64;
    m += 64;
}
/* inlined sodium_dolbeau_u0.h */
if (bytes > 0) {
    __m128i       x_0, x_1, x_2, x_3;
    __m128i       t_1;
    const __m128i rot16 =
        _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
    const __m128i rot8 =
        _mm_set_epi8(14, 13, 12, 15, 10, 9, 8, 11, 6, 5, 4, 7, 2, 1, 0, 3);
    uint8_t partialblock[64];

    unsigned int i;

    x_0 = _mm_loadu_si128((const __m128i*) (x + 0));
    x_1 = _mm_loadu_si128((const __m128i*) (x + 4));
    x_2 = _mm_loadu_si128((const __m128i*) (x + 8));
    x_3 = _mm_loadu_si128((const __m128i*) (x + 12));

    for (i = 0; i < ROUNDS; i += 2) {
        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_3 = _mm_shuffle_epi8(x_3, rot16);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_1 = _mm_xor_si128(x_1, x_2);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 12);
        t_1 = _mm_srli_epi32(t_1, 20);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_0 = _mm_shuffle_epi32(x_0, 0x93);
        x_3 = _mm_shuffle_epi8(x_3, rot8);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_3 = _mm_shuffle_epi32(x_3, 0x4e);
        x_1 = _mm_xor_si128(x_1, x_2);
        x_2 = _mm_shuffle_epi32(x_2, 0x39);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 7);
        t_1 = _mm_srli_epi32(t_1, 25);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_3 = _mm_shuffle_epi8(x_3, rot16);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_1 = _mm_xor_si128(x_1, x_2);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 12);
        t_1 = _mm_srli_epi32(t_1, 20);
        x_1 = _mm_xor_si128(x_1, t_1);

        x_0 = _mm_add_epi32(x_0, x_1);
        x_3 = _mm_xor_si128(x_3, x_0);
        x_0 = _mm_shuffle_epi32(x_0, 0x39);
        x_3 = _mm_shuffle_epi8(x_3, rot8);

        x_2 = _mm_add_epi32(x_2, x_3);
        x_3 = _mm_shuffle_epi32(x_3, 0x4e);
        x_1 = _mm_xor_si128(x_1, x_2);
        x_2 = _mm_shuffle_epi32(x_2, 0x93);

        t_1 = x_1;
        x_1 = _mm_slli_epi32(x_1, 7);
        t_1 = _mm_srli_epi32(t_1, 25);
        x_1 = _mm_xor_si128(x_1, t_1);
    }
    x_0 = _mm_add_epi32(x_0, _mm_loadu_si128((const __m128i*) (x + 0)));
    x_1 = _mm_add_epi32(x_1, _mm_loadu_si128((const __m128i*) (x + 4)));
    x_2 = _mm_add_epi32(x_2, _mm_loadu_si128((const __m128i*) (x + 8)));
    x_3 = _mm_add_epi32(x_3, _mm_loadu_si128((const __m128i*) (x + 12)));
    _mm_storeu_si128((__m128i*) (partialblock + 0), x_0);
    _mm_storeu_si128((__m128i*) (partialblock + 16), x_1);
    _mm_storeu_si128((__m128i*) (partialblock + 32), x_2);
    _mm_storeu_si128((__m128i*) (partialblock + 48), x_3);

    for (i = 0; i < bytes; i++) {
        c[i] = m[i] ^ partialblock[i];
    }

    sodium_memzero(partialblock, sizeof partialblock);
}
}

static int
stream_ref(unsigned char *c, unsigned long long clen, const unsigned char *n,
           const unsigned char *k)
{
    struct chacha_ctx ctx;

    if (!clen) {
        return 0;
    }
    COMPILER_ASSERT(crypto_stream_chacha20_KEYBYTES == 256 / 8);
    chacha_keysetup(&ctx, k);
    chacha_ivsetup(&ctx, n, NULL);
    memset(c, 0, clen);
    chacha20_encrypt_bytes(&ctx, c, c, clen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ietf_ext_ref(unsigned char *c, unsigned long long clen,
                    const unsigned char *n, const unsigned char *k)
{
    struct chacha_ctx ctx;

    if (!clen) {
        return 0;
    }
    COMPILER_ASSERT(crypto_stream_chacha20_KEYBYTES == 256 / 8);
    chacha_keysetup(&ctx, k);
    chacha_ietf_ivsetup(&ctx, n, NULL);
    memset(c, 0, clen);
    chacha20_encrypt_bytes(&ctx, c, c, clen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ref_xor_ic(unsigned char *c, const unsigned char *m,
                  unsigned long long mlen, const unsigned char *n, uint64_t ic,
                  const unsigned char *k)
{
    struct chacha_ctx ctx;
    uint8_t           ic_bytes[8];
    uint32_t          ic_high;
    uint32_t          ic_low;

    if (!mlen) {
        return 0;
    }
    ic_high = (uint32_t) (ic >> 32);
    ic_low  = (uint32_t) ic;
    STORE32_LE(&ic_bytes[0], ic_low);
    STORE32_LE(&ic_bytes[4], ic_high);
    chacha_keysetup(&ctx, k);
    chacha_ivsetup(&ctx, n, ic_bytes);
    chacha20_encrypt_bytes(&ctx, m, c, mlen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

static int
stream_ietf_ext_ref_xor_ic(unsigned char *c, const unsigned char *m,
                           unsigned long long mlen, const unsigned char *n,
                           uint32_t ic, const unsigned char *k)
{
    struct chacha_ctx ctx;
    uint8_t           ic_bytes[4];

    if (!mlen) {
        return 0;
    }
    STORE32_LE(ic_bytes, ic);
    chacha_keysetup(&ctx, k);
    chacha_ietf_ivsetup(&ctx, n, ic_bytes);
    chacha20_encrypt_bytes(&ctx, m, c, mlen);
    sodium_memzero(&ctx, sizeof ctx);

    return 0;
}

struct crypto_stream_chacha20_implementation
    crypto_stream_chacha20_dolbeau_avx2_implementation = {
        SODIUM_C99(.stream =) stream_ref,
        SODIUM_C99(.stream_ietf_ext =) stream_ietf_ext_ref,
        SODIUM_C99(.stream_xor_ic =) stream_ref_xor_ic,
        SODIUM_C99(.stream_ietf_ext_xor_ic =) stream_ietf_ext_ref_xor_ic
    };

# ifdef __clang__
#  pragma clang attribute pop
# endif

#endif

int chacha20_dolbeau_link_warning_dummy = 0;
#undef chacha_ctx
#undef chacha_keysetup
#undef chacha_ivsetup
#undef chacha_ietf_ivsetup
#undef chacha20_encrypt_bytes
#undef stream_ref
#undef stream_ietf_ext_ref
#undef stream_ref_xor_ic
#undef stream_ietf_ext_ref_xor_ic
#undef ROUNDS
#undef VEC8_ROT
#undef VEC8_QUARTERROUND_NAIVE
#undef VEC8_QUARTERROUND_SHUFFLE
#undef VEC8_QUARTERROUND_SHUFFLE2
#undef VEC8_QUARTERROUND
#undef VEC8_LINE1
#undef VEC8_LINE2
#undef VEC8_LINE3
#undef VEC8_LINE4
#undef VEC8_ROUND_SEQ
#undef VEC8_ROUND_HALF
#undef VEC8_ROUND_HALFANDHALF
#undef VEC8_ROUND
#undef ONEQUAD_TRANSPOSE
#undef ONEQUAD
#undef ONEQUAD_UNPCK
#undef ONEOCTO
#undef VEC4_ROT
#undef VEC4_QUARTERROUND_SHUFFLE
#undef VEC4_QUARTERROUND

/* ================= sodium_stream_chacha20.c ================= */
#define implementation implementation__cc20
#include <stdio.h>

#if defined(HAVE_AVX2INTRIN_H) && defined(HAVE_EMMINTRIN_H) && \
    defined(HAVE_TMMINTRIN_H) && defined(HAVE_SMMINTRIN_H)
#endif
#if defined(HAVE_EMMINTRIN_H) && defined(HAVE_TMMINTRIN_H)
#endif

static const crypto_stream_chacha20_implementation *implementation =
    &crypto_stream_chacha20_ref_implementation;

size_t
crypto_stream_chacha20_keybytes(void) {
    return crypto_stream_chacha20_KEYBYTES;
}

size_t
crypto_stream_chacha20_noncebytes(void) {
    return crypto_stream_chacha20_NONCEBYTES;
}

size_t
crypto_stream_chacha20_messagebytes_max(void)
{
    return crypto_stream_chacha20_MESSAGEBYTES_MAX;
}

size_t
crypto_stream_chacha20_ietf_keybytes(void) {
    return crypto_stream_chacha20_ietf_KEYBYTES;
}

size_t
crypto_stream_chacha20_ietf_noncebytes(void) {
    return crypto_stream_chacha20_ietf_NONCEBYTES;
}

size_t
crypto_stream_chacha20_ietf_messagebytes_max(void)
{
    return crypto_stream_chacha20_ietf_MESSAGEBYTES_MAX;
}

int
crypto_stream_chacha20(unsigned char *c, unsigned long long clen,
                       const unsigned char *n, const unsigned char *k)
{
    if (clen > crypto_stream_chacha20_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    return implementation->stream(c, clen, n, k);
}

int
crypto_stream_chacha20_xor_ic(unsigned char *c, const unsigned char *m,
                              unsigned long long mlen,
                              const unsigned char *n, uint64_t ic,
                              const unsigned char *k)
{
    if (mlen > crypto_stream_chacha20_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    return implementation->stream_xor_ic(c, m, mlen, n, ic, k);
}

int
crypto_stream_chacha20_xor(unsigned char *c, const unsigned char *m,
                           unsigned long long mlen, const unsigned char *n,
                           const unsigned char *k)
{
    if (mlen > crypto_stream_chacha20_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    return implementation->stream_xor_ic(c, m, mlen, n, 0U, k);
}

int
crypto_stream_chacha20_ietf_ext(unsigned char *c, unsigned long long clen,
                                const unsigned char *n, const unsigned char *k)
{
    if (clen > crypto_stream_chacha20_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    return implementation->stream_ietf_ext(c, clen, n, k);
}

int
crypto_stream_chacha20_ietf_ext_xor_ic(unsigned char *c, const unsigned char *m,
                                       unsigned long long mlen,
                                       const unsigned char *n, uint32_t ic,
                                       const unsigned char *k)
{
    if (mlen > crypto_stream_chacha20_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    return implementation->stream_ietf_ext_xor_ic(c, m, mlen, n, ic, k);
}

static int
crypto_stream_chacha20_ietf_ext_xor(unsigned char *c, const unsigned char *m,
                                    unsigned long long mlen, const unsigned char *n,
                                    const unsigned char *k)
{
    if (mlen > crypto_stream_chacha20_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    return implementation->stream_ietf_ext_xor_ic(c, m, mlen, n, 0U, k);
}

int
crypto_stream_chacha20_ietf(unsigned char *c, unsigned long long clen,
                            const unsigned char *n, const unsigned char *k)
{
    if (clen > crypto_stream_chacha20_ietf_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    return crypto_stream_chacha20_ietf_ext(c, clen, n, k);
}

int
crypto_stream_chacha20_ietf_xor_ic(unsigned char *c, const unsigned char *m,
                                   unsigned long long mlen,
                                   const unsigned char *n, uint32_t ic,
                                   const unsigned char *k)
{
    if ((unsigned long long) ic >
        (64ULL * (1ULL << 32)) / 64ULL - (mlen + 63ULL) / 64ULL) {
        sodium_misuse();
    }
    return crypto_stream_chacha20_ietf_ext_xor_ic(c, m, mlen, n, ic, k);
}

int
crypto_stream_chacha20_ietf_xor(unsigned char *c, const unsigned char *m,
                                unsigned long long mlen, const unsigned char *n,
                                const unsigned char *k)
{
    if (mlen > crypto_stream_chacha20_ietf_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    return crypto_stream_chacha20_ietf_ext_xor(c, m, mlen, n, k);
}

void
crypto_stream_chacha20_ietf_keygen(unsigned char k[crypto_stream_chacha20_ietf_KEYBYTES])
{
    randombytes_buf(k, crypto_stream_chacha20_ietf_KEYBYTES);
}

void
crypto_stream_chacha20_keygen(unsigned char k[crypto_stream_chacha20_KEYBYTES])
{
    randombytes_buf(k, crypto_stream_chacha20_KEYBYTES);
}

int
_crypto_stream_chacha20_pick_best_implementation(void)
{
    implementation = &crypto_stream_chacha20_ref_implementation;
#if defined(HAVE_AVX2INTRIN_H) && defined(HAVE_EMMINTRIN_H) && \
    defined(HAVE_TMMINTRIN_H) && defined(HAVE_SMMINTRIN_H)
    if (sodium_runtime_has_avx2()) {
        #if NETCODE_CRYPTO_LOGS
        printf( "chacha20 -> avx2\n" );
        #endif // #if NETCODE_CRYPTO_LOGS
        implementation = &crypto_stream_chacha20_dolbeau_avx2_implementation;
        return 0;
    }
#endif
#if defined(HAVE_EMMINTRIN_H) && defined(HAVE_TMMINTRIN_H)
    if (sodium_runtime_has_ssse3()) {
        #if NETCODE_CRYPTO_LOGS
        printf( "chacha20 -> ssse3\n" );
        #endif // #if NETCODE_CRYPTO_LOGS
        implementation = &crypto_stream_chacha20_dolbeau_ssse3_implementation;
        return 0;
    }
#endif
    #if NETCODE_CRYPTO_LOGS
    printf( "chacha20 -> ref\n" );
    #endif // #if NETCODE_CRYPTO_LOGS
    return 0;
}
#undef implementation

/* ================= sodium_core_hchacha20.c ================= */

#include <stdint.h>
#include <stdlib.h>


#define QUARTERROUND(A, B, C, D)     \
  do {                               \
      A += B; D = ROTL32(D ^ A, 16); \
      C += D; B = ROTL32(B ^ C, 12); \
      A += B; D = ROTL32(D ^ A,  8); \
      C += D; B = ROTL32(B ^ C,  7); \
  } while(0)

int
crypto_core_hchacha20(unsigned char *out, const unsigned char *in,
                      const unsigned char *k, const unsigned char *c)
{
    int      i;
    uint32_t x0, x1, x2, x3, x4, x5, x6, x7;
    uint32_t x8, x9, x10, x11, x12, x13, x14, x15;

    if (c == NULL) {
        x0 = 0x61707865;
        x1 = 0x3320646e;
        x2 = 0x79622d32;
        x3 = 0x6b206574;
    } else {
        x0 = LOAD32_LE(c +  0);
        x1 = LOAD32_LE(c +  4);
        x2 = LOAD32_LE(c +  8);
        x3 = LOAD32_LE(c + 12);
    }
    x4  = LOAD32_LE(k +  0);
    x5  = LOAD32_LE(k +  4);
    x6  = LOAD32_LE(k +  8);
    x7  = LOAD32_LE(k + 12);
    x8  = LOAD32_LE(k + 16);
    x9  = LOAD32_LE(k + 20);
    x10 = LOAD32_LE(k + 24);
    x11 = LOAD32_LE(k + 28);
    x12 = LOAD32_LE(in +  0);
    x13 = LOAD32_LE(in +  4);
    x14 = LOAD32_LE(in +  8);
    x15 = LOAD32_LE(in + 12);

    for (i = 0; i < 10; i++) {
        QUARTERROUND(x0, x4,  x8, x12);
        QUARTERROUND(x1, x5,  x9, x13);
        QUARTERROUND(x2, x6, x10, x14);
        QUARTERROUND(x3, x7, x11, x15);
        QUARTERROUND(x0, x5, x10, x15);
        QUARTERROUND(x1, x6, x11, x12);
        QUARTERROUND(x2, x7,  x8, x13);
        QUARTERROUND(x3, x4,  x9, x14);
    }

    STORE32_LE(out +  0, x0);
    STORE32_LE(out +  4, x1);
    STORE32_LE(out +  8, x2);
    STORE32_LE(out + 12, x3);
    STORE32_LE(out + 16, x12);
    STORE32_LE(out + 20, x13);
    STORE32_LE(out + 24, x14);
    STORE32_LE(out + 28, x15);

    return 0;
}

size_t
crypto_core_hchacha20_outputbytes(void)
{
    return crypto_core_hchacha20_OUTPUTBYTES;
}

size_t
crypto_core_hchacha20_inputbytes(void)
{
    return crypto_core_hchacha20_INPUTBYTES;
}

size_t
crypto_core_hchacha20_keybytes(void)
{
    return crypto_core_hchacha20_KEYBYTES;
}

size_t
crypto_core_hchacha20_constbytes(void)
{
    return crypto_core_hchacha20_CONSTBYTES;
}
#undef QUARTERROUND

/* ================= sodium_poly1305_donna.c ================= */


#ifdef HAVE_TI_MODE
/* inlined sodium_poly1305_donna64.h */
/*
   poly1305 implementation using 64 bit * 64 bit = 128 bit multiplication
   and 128 bit addition
*/


#define MUL(out, x, y) out = ((uint128_t) x * y)
#define ADD(out, in) out += in
#define ADDLO(out, in) out += in
#define SHR(in, shift) (unsigned long long) (in >> (shift))
#define LO(in) (unsigned long long) (in)

#if defined(_MSC_VER)
# define POLY1305_NOINLINE __declspec(noinline)
#elif defined(__clang__) || defined(__GNUC__)
# define POLY1305_NOINLINE __attribute__((noinline))
#else
# define POLY1305_NOINLINE
#endif

#define poly1305_block_size 16

/* 17 + sizeof(unsigned long long) + 8*sizeof(unsigned long long) */
typedef struct poly1305_state_internal_t {
    unsigned long long r[3];
    unsigned long long h[3];
    unsigned long long pad[2];
    unsigned long long leftover;
    unsigned char      buffer[poly1305_block_size];
    unsigned char      final;
} poly1305_state_internal_t;

static void
poly1305_init(poly1305_state_internal_t *st, const unsigned char key[32])
{
    unsigned long long t0, t1;

    /* r &= 0xffffffc0ffffffc0ffffffc0fffffff */
    t0 = LOAD64_LE(&key[0]);
    t1 = LOAD64_LE(&key[8]);

    /* wiped after finalization */
    st->r[0] = (t0) &0xffc0fffffff;
    st->r[1] = ((t0 >> 44) | (t1 << 20)) & 0xfffffc0ffff;
    st->r[2] = ((t1 >> 24)) & 0x00ffffffc0f;

    /* h = 0 */
    st->h[0] = 0;
    st->h[1] = 0;
    st->h[2] = 0;

    /* save pad for later */
    st->pad[0] = LOAD64_LE(&key[16]);
    st->pad[1] = LOAD64_LE(&key[24]);

    st->leftover = 0;
    st->final    = 0;
}

static void
poly1305_blocks(poly1305_state_internal_t *st, const unsigned char *m,
                unsigned long long bytes)
{
    const unsigned long long hibit =
        (st->final) ? 0ULL : (1ULL << 40); /* 1 << 128 */
    unsigned long long r0, r1, r2;
    unsigned long long s1, s2;
    unsigned long long h0, h1, h2;
    unsigned long long c;
    uint128_t          d0, d1, d2, d;

    r0 = st->r[0];
    r1 = st->r[1];
    r2 = st->r[2];

    h0 = st->h[0];
    h1 = st->h[1];
    h2 = st->h[2];

    s1 = r1 * (5 << 2);
    s2 = r2 * (5 << 2);

    while (bytes >= poly1305_block_size) {
        unsigned long long t0, t1;

        /* h += m[i] */
        t0 = LOAD64_LE(&m[0]);
        t1 = LOAD64_LE(&m[8]);

        h0 += ((t0) &0xfffffffffff);
        h1 += (((t0 >> 44) | (t1 << 20)) & 0xfffffffffff);
        h2 += (((t1 >> 24)) & 0x3ffffffffff) | hibit;

        /* h *= r */
        MUL(d0, h0, r0);
        MUL(d, h1, s2);
        ADD(d0, d);
        MUL(d, h2, s1);
        ADD(d0, d);
        MUL(d1, h0, r1);
        MUL(d, h1, r0);
        ADD(d1, d);
        MUL(d, h2, s2);
        ADD(d1, d);
        MUL(d2, h0, r2);
        MUL(d, h1, r1);
        ADD(d2, d);
        MUL(d, h2, r0);
        ADD(d2, d);

        /* (partial) h %= p */
        c  = SHR(d0, 44);
        h0 = LO(d0) & 0xfffffffffff;
        ADDLO(d1, c);
        c  = SHR(d1, 44);
        h1 = LO(d1) & 0xfffffffffff;
        ADDLO(d2, c);
        c  = SHR(d2, 42);
        h2 = LO(d2) & 0x3ffffffffff;
        h0 += c * 5;
        c  = (h0 >> 44);
        h0 = h0 & 0xfffffffffff;
        h1 += c;

        m += poly1305_block_size;
        bytes -= poly1305_block_size;
    }

    st->h[0] = h0;
    st->h[1] = h1;
    st->h[2] = h2;
}

static POLY1305_NOINLINE void
poly1305_finish(poly1305_state_internal_t *st, unsigned char mac[16])
{
    unsigned long long h0, h1, h2, c;
    unsigned long long g0, g1, g2;
    unsigned long long t0, t1;

    /* process the remaining block */
    if (st->leftover) {
        unsigned long long i = st->leftover;

        st->buffer[i] = 1;

        for (i = i + 1; i < poly1305_block_size; i++) {
            st->buffer[i] = 0;
        }
        st->final = 1;
        poly1305_blocks(st, st->buffer, poly1305_block_size);
    }

    /* fully carry h */
    h0 = st->h[0];
    h1 = st->h[1];
    h2 = st->h[2];

    c = (h1 >> 44);
    h1 &= 0xfffffffffff;
    h2 += c;
    c = (h2 >> 42);
    h2 &= 0x3ffffffffff;
    h0 += c * 5;
    c = (h0 >> 44);
    h0 &= 0xfffffffffff;
    h1 += c;
    c = (h1 >> 44);
    h1 &= 0xfffffffffff;
    h2 += c;
    c = (h2 >> 42);
    h2 &= 0x3ffffffffff;
    h0 += c * 5;
    c = (h0 >> 44);
    h0 &= 0xfffffffffff;
    h1 += c;

    /* compute h + -p */
    g0 = h0 + 5;
    c  = (g0 >> 44);
    g0 &= 0xfffffffffff;
    g1 = h1 + c;
    c  = (g1 >> 44);
    g1 &= 0xfffffffffff;
    g2 = h2 + c - (1ULL << 42);

    /* select h if h < p, or h + -p if h >= p */
    c = (g2 >> ((sizeof(unsigned long long) * 8) - 1)) - 1;
    g0 &= c;
    g1 &= c;
    g2 &= c;
    c  = ~c;
    h0 = (h0 & c) | g0;
    h1 = (h1 & c) | g1;
    h2 = (h2 & c) | g2;

    /* h = (h + pad) */
    t0 = st->pad[0];
    t1 = st->pad[1];

    h0 += ((t0) &0xfffffffffff);
    c = (h0 >> 44);
    h0 &= 0xfffffffffff;
    h1 += (((t0 >> 44) | (t1 << 20)) & 0xfffffffffff) + c;
    c = (h1 >> 44);
    h1 &= 0xfffffffffff;
    h2 += (((t1 >> 24)) & 0x3ffffffffff) + c;
    h2 &= 0x3ffffffffff;

    /* mac = h % (2^128) */
    h0 = ((h0) | (h1 << 44));
    h1 = ((h1 >> 20) | (h2 << 24));

    STORE64_LE(&mac[0], h0);
    STORE64_LE(&mac[8], h1);

    /* zero out the state */
    sodium_memzero((void *) st, sizeof *st);
}
#else
/* inlined sodium_poly1305_donna32.h */
/*
   poly1305 implementation using 32 bit * 32 bit = 64 bit multiplication
   and 64 bit addition
*/

#if defined(_MSC_VER)
# define POLY1305_NOINLINE __declspec(noinline)
#elif defined(__clang__) || defined(__GNUC__)
# define POLY1305_NOINLINE __attribute__((noinline))
#else
# define POLY1305_NOINLINE
#endif


#define poly1305_block_size 16

/* 17 + sizeof(unsigned long long) + 14*sizeof(unsigned long) */
typedef struct poly1305_state_internal_t {
    unsigned long      r[5];
    unsigned long      h[5];
    unsigned long      pad[4];
    unsigned long long leftover;
    unsigned char      buffer[poly1305_block_size];
    unsigned char      final;
} poly1305_state_internal_t;

static void
poly1305_init(poly1305_state_internal_t *st, const unsigned char key[32])
{
    /* r &= 0xffffffc0ffffffc0ffffffc0fffffff - wiped after finalization */
    st->r[0] = (LOAD32_LE(&key[0])) & 0x3ffffff;
    st->r[1] = (LOAD32_LE(&key[3]) >> 2) & 0x3ffff03;
    st->r[2] = (LOAD32_LE(&key[6]) >> 4) & 0x3ffc0ff;
    st->r[3] = (LOAD32_LE(&key[9]) >> 6) & 0x3f03fff;
    st->r[4] = (LOAD32_LE(&key[12]) >> 8) & 0x00fffff;

    /* h = 0 */
    st->h[0] = 0;
    st->h[1] = 0;
    st->h[2] = 0;
    st->h[3] = 0;
    st->h[4] = 0;

    /* save pad for later */
    st->pad[0] = LOAD32_LE(&key[16]);
    st->pad[1] = LOAD32_LE(&key[20]);
    st->pad[2] = LOAD32_LE(&key[24]);
    st->pad[3] = LOAD32_LE(&key[28]);

    st->leftover = 0;
    st->final    = 0;
}

static void
poly1305_blocks(poly1305_state_internal_t *st, const unsigned char *m,
                unsigned long long bytes)
{
    const unsigned long hibit = (st->final) ? 0UL : (1UL << 24); /* 1 << 128 */
    unsigned long       r0, r1, r2, r3, r4;
    unsigned long       s1, s2, s3, s4;
    unsigned long       h0, h1, h2, h3, h4;
    unsigned long long  d0, d1, d2, d3, d4;
    unsigned long       c;

    r0 = st->r[0];
    r1 = st->r[1];
    r2 = st->r[2];
    r3 = st->r[3];
    r4 = st->r[4];

    s1 = r1 * 5;
    s2 = r2 * 5;
    s3 = r3 * 5;
    s4 = r4 * 5;

    h0 = st->h[0];
    h1 = st->h[1];
    h2 = st->h[2];
    h3 = st->h[3];
    h4 = st->h[4];

    while (bytes >= poly1305_block_size) {
        /* h += m[i] */
        h0 += (LOAD32_LE(m + 0)) & 0x3ffffff;
        h1 += (LOAD32_LE(m + 3) >> 2) & 0x3ffffff;
        h2 += (LOAD32_LE(m + 6) >> 4) & 0x3ffffff;
        h3 += (LOAD32_LE(m + 9) >> 6) & 0x3ffffff;
        h4 += (LOAD32_LE(m + 12) >> 8) | hibit;

        /* h *= r */
        d0 = ((unsigned long long) h0 * r0) + ((unsigned long long) h1 * s4) +
             ((unsigned long long) h2 * s3) + ((unsigned long long) h3 * s2) +
             ((unsigned long long) h4 * s1);
        d1 = ((unsigned long long) h0 * r1) + ((unsigned long long) h1 * r0) +
             ((unsigned long long) h2 * s4) + ((unsigned long long) h3 * s3) +
             ((unsigned long long) h4 * s2);
        d2 = ((unsigned long long) h0 * r2) + ((unsigned long long) h1 * r1) +
             ((unsigned long long) h2 * r0) + ((unsigned long long) h3 * s4) +
             ((unsigned long long) h4 * s3);
        d3 = ((unsigned long long) h0 * r3) + ((unsigned long long) h1 * r2) +
             ((unsigned long long) h2 * r1) + ((unsigned long long) h3 * r0) +
             ((unsigned long long) h4 * s4);
        d4 = ((unsigned long long) h0 * r4) + ((unsigned long long) h1 * r3) +
             ((unsigned long long) h2 * r2) + ((unsigned long long) h3 * r1) +
             ((unsigned long long) h4 * r0);

        /* (partial) h %= p */
        c  = (unsigned long) (d0 >> 26);
        h0 = (unsigned long) d0 & 0x3ffffff;
        d1 += c;
        c  = (unsigned long) (d1 >> 26);
        h1 = (unsigned long) d1 & 0x3ffffff;
        d2 += c;
        c  = (unsigned long) (d2 >> 26);
        h2 = (unsigned long) d2 & 0x3ffffff;
        d3 += c;
        c  = (unsigned long) (d3 >> 26);
        h3 = (unsigned long) d3 & 0x3ffffff;
        d4 += c;
        c  = (unsigned long) (d4 >> 26);
        h4 = (unsigned long) d4 & 0x3ffffff;
        h0 += c * 5;
        c  = (h0 >> 26);
        h0 = h0 & 0x3ffffff;
        h1 += c;

        m += poly1305_block_size;
        bytes -= poly1305_block_size;
    }

    st->h[0] = h0;
    st->h[1] = h1;
    st->h[2] = h2;
    st->h[3] = h3;
    st->h[4] = h4;
}

static POLY1305_NOINLINE void
poly1305_finish(poly1305_state_internal_t *st, unsigned char mac[16])
{
    unsigned long      h0, h1, h2, h3, h4, c;
    unsigned long      g0, g1, g2, g3, g4;
    unsigned long long f;
    unsigned long      mask;

    /* process the remaining block */
    if (st->leftover) {
        unsigned long long i = st->leftover;

        st->buffer[i++] = 1;
        for (; i < poly1305_block_size; i++) {
            st->buffer[i] = 0;
        }
        st->final = 1;
        poly1305_blocks(st, st->buffer, poly1305_block_size);
    }

    /* fully carry h */
    h0 = st->h[0];
    h1 = st->h[1];
    h2 = st->h[2];
    h3 = st->h[3];
    h4 = st->h[4];

    c  = h1 >> 26;
    h1 = h1 & 0x3ffffff;
    h2 += c;
    c  = h2 >> 26;
    h2 = h2 & 0x3ffffff;
    h3 += c;
    c  = h3 >> 26;
    h3 = h3 & 0x3ffffff;
    h4 += c;
    c  = h4 >> 26;
    h4 = h4 & 0x3ffffff;
    h0 += c * 5;
    c  = h0 >> 26;
    h0 = h0 & 0x3ffffff;
    h1 += c;

    /* compute h + -p */
    g0 = h0 + 5;
    c  = g0 >> 26;
    g0 &= 0x3ffffff;
    g1 = h1 + c;
    c  = g1 >> 26;
    g1 &= 0x3ffffff;
    g2 = h2 + c;
    c  = g2 >> 26;
    g2 &= 0x3ffffff;
    g3 = h3 + c;
    c  = g3 >> 26;
    g3 &= 0x3ffffff;
    g4 = h4 + c - (1UL << 26);

    /* select h if h < p, or h + -p if h >= p */
    mask = (g4 >> ((sizeof(unsigned long) * 8) - 1)) - 1;
    g0 &= mask;
    g1 &= mask;
    g2 &= mask;
    g3 &= mask;
    g4 &= mask;
    mask = ~mask;

    h0 = (h0 & mask) | g0;
    h1 = (h1 & mask) | g1;
    h2 = (h2 & mask) | g2;
    h3 = (h3 & mask) | g3;
    h4 = (h4 & mask) | g4;

    /* h = h % (2^128) */
    h0 = ((h0) | (h1 << 26)) & 0xffffffff;
    h1 = ((h1 >> 6) | (h2 << 20)) & 0xffffffff;
    h2 = ((h2 >> 12) | (h3 << 14)) & 0xffffffff;
    h3 = ((h3 >> 18) | (h4 << 8)) & 0xffffffff;

    /* mac = (h + pad) % (2^128) */
    f  = (unsigned long long) h0 + st->pad[0];
    h0 = (unsigned long) f;
    f  = (unsigned long long) h1 + st->pad[1] + (f >> 32);
    h1 = (unsigned long) f;
    f  = (unsigned long long) h2 + st->pad[2] + (f >> 32);
    h2 = (unsigned long) f;
    f  = (unsigned long long) h3 + st->pad[3] + (f >> 32);
    h3 = (unsigned long) f;

    STORE32_LE(mac + 0, (uint32_t) h0);
    STORE32_LE(mac + 4, (uint32_t) h1);
    STORE32_LE(mac + 8, (uint32_t) h2);
    STORE32_LE(mac + 12, (uint32_t) h3);

    /* zero out the state */
    sodium_memzero((void *) st, sizeof *st);
}
#endif

static void
poly1305_update(poly1305_state_internal_t *st, const unsigned char *m,
                unsigned long long bytes)
{
    unsigned long long i;

    /* handle leftover */
    if (st->leftover) {
        unsigned long long want = (poly1305_block_size - st->leftover);

        if (want > bytes) {
            want = bytes;
        }
        for (i = 0; i < want; i++) {
            st->buffer[st->leftover + i] = m[i];
        }
        bytes -= want;
        m += want;
        st->leftover += want;
        if (st->leftover < poly1305_block_size) {
            return;
        }
        poly1305_blocks(st, st->buffer, poly1305_block_size);
        st->leftover = 0;
    }

    /* process full blocks */
    if (bytes >= poly1305_block_size) {
        unsigned long long want = (bytes & ~(poly1305_block_size - 1));

        poly1305_blocks(st, m, want);
        m += want;
        bytes -= want;
    }

    /* store leftover */
    if (bytes) {
        for (i = 0; i < bytes; i++) {
            st->buffer[st->leftover + i] = m[i];
        }
        st->leftover += bytes;
    }
}

static int
crypto_onetimeauth_poly1305_donna(unsigned char *out, const unsigned char *m,
                                  unsigned long long   inlen,
                                  const unsigned char *key)
{
    CRYPTO_ALIGN(64) poly1305_state_internal_t state;

    poly1305_init(&state, key);
    poly1305_update(&state, m, inlen);
    poly1305_finish(&state, out);

    return 0;
}

static int
crypto_onetimeauth_poly1305_donna_init(crypto_onetimeauth_poly1305_state *state,
                                       const unsigned char *key)
{
    COMPILER_ASSERT(sizeof(crypto_onetimeauth_poly1305_state) >=
        sizeof(poly1305_state_internal_t));
    poly1305_init((poly1305_state_internal_t *) (void *) state, key);

    return 0;
}

static int
crypto_onetimeauth_poly1305_donna_update(
    crypto_onetimeauth_poly1305_state *state, const unsigned char *in,
    unsigned long long inlen)
{
    poly1305_update((poly1305_state_internal_t *) (void *) state, in, inlen);

    return 0;
}

static int
crypto_onetimeauth_poly1305_donna_final(
    crypto_onetimeauth_poly1305_state *state, unsigned char *out)
{
    poly1305_finish((poly1305_state_internal_t *) (void *) state, out);

    return 0;
}

static int
crypto_onetimeauth_poly1305_donna_verify(const unsigned char *h,
                                         const unsigned char *in,
                                         unsigned long long   inlen,
                                         const unsigned char *k)
{
    unsigned char correct[16];

    crypto_onetimeauth_poly1305_donna(correct, in, inlen, k);

    return crypto_verify_16(h, correct);
}

struct crypto_onetimeauth_poly1305_implementation
    crypto_onetimeauth_poly1305_donna_implementation = {
        SODIUM_C99(.onetimeauth =) crypto_onetimeauth_poly1305_donna,
        SODIUM_C99(.onetimeauth_verify =)
            crypto_onetimeauth_poly1305_donna_verify,
        SODIUM_C99(.onetimeauth_init =) crypto_onetimeauth_poly1305_donna_init,
        SODIUM_C99(.onetimeauth_update =)
            crypto_onetimeauth_poly1305_donna_update,
        SODIUM_C99(.onetimeauth_final =) crypto_onetimeauth_poly1305_donna_final
    };
#undef MUL
#undef ADD
#undef ADDLO
#undef SHR
#undef LO
#undef POLY1305_NOINLINE
#undef poly1305_block_size

/* ================= sodium_poly1305-sse2.c ================= */
#define poly1305_state_internal_t poly1305_state_internal_t__sse2
#define poly1305_init poly1305_init__sse2
#define poly1305_blocks poly1305_blocks__sse2
#define poly1305_update poly1305_update__sse2
#define poly1305_finish poly1305_finish__sse2
#define poly1305_final poly1305_final__sse2
#define poly1305_state_internal poly1305_state_internal__sse2

#include <stdint.h>
#include <string.h>


#if defined(HAVE_TI_MODE) && defined(HAVE_EMMINTRIN_H)

# ifdef __clang__
#  pragma clang attribute push(__attribute__((target("sse2"))), apply_to = function)
# elif defined(__GNUC__)
#  pragma GCC target("sse2")
# endif

# include <emmintrin.h>

typedef __m128i xmmi;

# if defined(_MSC_VER)
#  define POLY1305_NOINLINE __declspec(noinline)
# elif defined(__clang__) || defined(__GNUC__)
#  define POLY1305_NOINLINE __attribute__((noinline))
# else
#  define POLY1305_NOINLINE
# endif

# define poly1305_block_size 32

enum poly1305_state_flags_t {
    poly1305_started       = 1,
    poly1305_final_shift8  = 4,
    poly1305_final_shift16 = 8,
    poly1305_final_r2_r    = 16, /* use [r^2,r] for the final block */
    poly1305_final_r_1     = 32  /* use [r,1] for the final block */
};

typedef struct poly1305_state_internal_t {
    union {
        uint64_t h[3];
        uint32_t hh[10];
    } H;                                            /*  40 bytes  */
    uint32_t           R[5];                        /*  20 bytes  */
    uint32_t           R2[5];                       /*  20 bytes  */
    uint32_t           R4[5];                       /*  20 bytes  */
    uint64_t           pad[2];                      /*  16 bytes  */
    uint64_t           flags;                       /*   8 bytes  */
    unsigned long long leftover;                    /* 8 bytes */
    unsigned char      buffer[poly1305_block_size]; /* 32 bytes */
} poly1305_state_internal_t;                        /* 164 bytes total */

/*
 * _mm_loadl_epi64() is turned into a simple MOVQ. So, unaligned accesses are
 * totally fine, even though this intrinsic requires a __m128i* input.
 * This confuses dynamic analysis, so force alignment, only in debug mode.
 */
# ifdef DEBUG
static xmmi
_fakealign_mm_loadl_epi64(const void *m)
{
    xmmi tmp;
    memcpy(&tmp, m, 8);

    return _mm_loadl_epi64(&tmp);
}
# define _mm_loadl_epi64(X) _fakealign_mm_loadl_epi64(X)
#endif

/* copy 0-31 bytes */
static inline void
poly1305_block_copy31(unsigned char *dst, const unsigned char *src,
                      unsigned long long bytes)
{
    if (bytes & 16) {
        _mm_store_si128((xmmi *) (void *) dst,
                        _mm_loadu_si128((const xmmi *) (const void *) src));
        src += 16;
        dst += 16;
    }
    if (bytes & 8) {
        memcpy(dst, src, 8);
        src += 8;
        dst += 8;
    }
    if (bytes & 4) {
        memcpy(dst, src, 4);
        src += 4;
        dst += 4;
    }
    if (bytes & 2) {
        memcpy(dst, src, 2);
        src += 2;
        dst += 2;
    }
    if (bytes & 1) {
        *dst = *src;
    }
}

static POLY1305_NOINLINE void
poly1305_init_ext(poly1305_state_internal_t *st, const unsigned char key[32],
                  unsigned long long bytes)
{
    uint32_t          *R;
    uint128_t          d[3];
    uint64_t           r0, r1, r2;
    uint64_t           rt0, rt1, rt2, st2, c;
    uint64_t           t0, t1;
    unsigned long long i;

    if (!bytes) {
        bytes = ~(unsigned long long) 0;
    }
    /* H = 0 */
    _mm_storeu_si128((xmmi *) (void *) &st->H.hh[0], _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) &st->H.hh[4], _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) &st->H.hh[8], _mm_setzero_si128());

    /* clamp key */
    memcpy(&t0, key, 8);
    memcpy(&t1, key + 8, 8);
    r0 = t0 & 0xffc0fffffff;
    t0 >>= 44;
    t0 |= t1 << 20;
    r1 = t0 & 0xfffffc0ffff;
    t1 >>= 24;
    r2 = t1 & 0x00ffffffc0f;

    /* r^1 */
    R    = st->R;
    R[0] = (uint32_t)(r0) &0x3ffffff;
    R[1] = (uint32_t)((r0 >> 26) | (r1 << 18)) & 0x3ffffff;
    R[2] = (uint32_t)((r1 >> 8)) & 0x3ffffff;
    R[3] = (uint32_t)((r1 >> 34) | (r2 << 10)) & 0x3ffffff;
    R[4] = (uint32_t)((r2 >> 16));

    /* save pad */
    memcpy(&st->pad[0], key + 16, 8);
    memcpy(&st->pad[1], key + 24, 8);

    rt0 = r0;
    rt1 = r1;
    rt2 = r2;

    /* r^2, r^4 */
    for (i = 0; i < 2; i++) {
        if (i == 0) {
            R = st->R2;
            if (bytes <= 16) {
                break;
            }
        } else if (i == 1) {
            R = st->R4;
            if (bytes < 96) {
                break;
            }
        }
        st2 = rt2 * (5 << 2);

        d[0] = ((uint128_t) rt0 * rt0) + ((uint128_t)(rt1 * 2) * st2);
        d[1] = ((uint128_t) rt2 * st2) + ((uint128_t)(rt0 * 2) * rt1);
        d[2] = ((uint128_t) rt1 * rt1) + ((uint128_t)(rt2 * 2) * rt0);

        rt0 = (uint64_t) d[0] & 0xfffffffffff;
        c   = (uint64_t)(d[0] >> 44);
        d[1] += c;

        rt1 = (uint64_t) d[1] & 0xfffffffffff;
        c   = (uint64_t)(d[1] >> 44);
        d[2] += c;

        rt2 = (uint64_t) d[2] & 0x3ffffffffff;
        c   = (uint64_t)(d[2] >> 42);
        rt0 += c * 5;
        c   = (rt0 >> 44);
        rt0 = rt0 & 0xfffffffffff;
        rt1 += c;
        c   = (rt1 >> 44);
        rt1 = rt1 & 0xfffffffffff;
        rt2 += c; /* even if rt2 overflows, it will still fit in rp4 safely, and
                     is safe to multiply with */

        R[0] = (uint32_t)(rt0) &0x3ffffff;
        R[1] = (uint32_t)((rt0 >> 26) | (rt1 << 18)) & 0x3ffffff;
        R[2] = (uint32_t)((rt1 >> 8)) & 0x3ffffff;
        R[3] = (uint32_t)((rt1 >> 34) | (rt2 << 10)) & 0x3ffffff;
        R[4] = (uint32_t)((rt2 >> 16));
    }
    st->flags    = 0;
    st->leftover = 0U;
}

static POLY1305_NOINLINE void
poly1305_blocks(poly1305_state_internal_t *st, const unsigned char *m,
                unsigned long long bytes)
{
    CRYPTO_ALIGN(64)
    xmmi HIBIT =
        _mm_shuffle_epi32(_mm_cvtsi32_si128(1 << 24), _MM_SHUFFLE(1, 0, 1, 0));
    const xmmi MMASK = _mm_shuffle_epi32(_mm_cvtsi32_si128((1 << 26) - 1),
                                         _MM_SHUFFLE(1, 0, 1, 0));
    const xmmi FIVE =
        _mm_shuffle_epi32(_mm_cvtsi32_si128(5), _MM_SHUFFLE(1, 0, 1, 0));
    xmmi H0, H1, H2, H3, H4;
    xmmi T0, T1, T2, T3, T4, T5, T6, T7, T8;
    xmmi M0, M1, M2, M3, M4;
    xmmi M5, M6, M7, M8;
    xmmi C1, C2;
    xmmi R20, R21, R22, R23, R24, S21, S22, S23, S24;
    xmmi R40, R41, R42, R43, R44, S41, S42, S43, S44;

    if (st->flags & poly1305_final_shift8) {
        HIBIT = _mm_srli_si128(HIBIT, 8);
    }
    if (st->flags & poly1305_final_shift16) {
        HIBIT = _mm_setzero_si128();
    }
    if (!(st->flags & poly1305_started)) {
        /* H = [Mx,My] */
        T5 = _mm_unpacklo_epi64(
            _mm_loadl_epi64((const xmmi *) (const void *) (m + 0)),
            _mm_loadl_epi64((const xmmi *) (const void *) (m + 16)));
        T6 = _mm_unpacklo_epi64(
            _mm_loadl_epi64((const xmmi *) (const void *) (m + 8)),
            _mm_loadl_epi64((const xmmi *) (const void *) (m + 24)));
        H0 = _mm_and_si128(MMASK, T5);
        H1 = _mm_and_si128(MMASK, _mm_srli_epi64(T5, 26));
        T5 = _mm_or_si128(_mm_srli_epi64(T5, 52), _mm_slli_epi64(T6, 12));
        H2 = _mm_and_si128(MMASK, T5);
        H3 = _mm_and_si128(MMASK, _mm_srli_epi64(T5, 26));
        H4 = _mm_srli_epi64(T6, 40);
        H4 = _mm_or_si128(H4, HIBIT);
        m += 32;
        bytes -= 32;
        st->flags |= poly1305_started;
    } else {
        T0 = _mm_loadu_si128((const xmmi *) (const void *) &st->H.hh[0]);
        T1 = _mm_loadu_si128((const xmmi *) (const void *) &st->H.hh[4]);
        T2 = _mm_loadu_si128((const xmmi *) (const void *) &st->H.hh[8]);
        H0 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(1, 1, 0, 0));
        H1 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(3, 3, 2, 2));
        H2 = _mm_shuffle_epi32(T1, _MM_SHUFFLE(1, 1, 0, 0));
        H3 = _mm_shuffle_epi32(T1, _MM_SHUFFLE(3, 3, 2, 2));
        H4 = _mm_shuffle_epi32(T2, _MM_SHUFFLE(1, 1, 0, 0));
    }
    if (st->flags & (poly1305_final_r2_r | poly1305_final_r_1)) {
        if (st->flags & poly1305_final_r2_r) {
            /* use [r^2, r] */
            T2  = _mm_loadu_si128((const xmmi *) (const void *) &st->R[0]);
            T3  = _mm_cvtsi32_si128(st->R[4]);
            T0  = _mm_loadu_si128((const xmmi *) (const void *) &st->R2[0]);
            T1  = _mm_cvtsi32_si128(st->R2[4]);
            T4  = _mm_unpacklo_epi32(T0, T2);
            T5  = _mm_unpackhi_epi32(T0, T2);
            R24 = _mm_unpacklo_epi64(T1, T3);
        } else {
            /* use [r^1, 1] */
            T0  = _mm_loadu_si128((const xmmi *) (const void *) &st->R[0]);
            T1  = _mm_cvtsi32_si128(st->R[4]);
            T2  = _mm_cvtsi32_si128(1);
            T4  = _mm_unpacklo_epi32(T0, T2);
            T5  = _mm_unpackhi_epi32(T0, T2);
            R24 = T1;
        }
        R20 = _mm_shuffle_epi32(T4, _MM_SHUFFLE(1, 1, 0, 0));
        R21 = _mm_shuffle_epi32(T4, _MM_SHUFFLE(3, 3, 2, 2));
        R22 = _mm_shuffle_epi32(T5, _MM_SHUFFLE(1, 1, 0, 0));
        R23 = _mm_shuffle_epi32(T5, _MM_SHUFFLE(3, 3, 2, 2));
    } else {
        /* use [r^2, r^2] */
        T0  = _mm_loadu_si128((const xmmi *) (const void *) &st->R2[0]);
        T1  = _mm_cvtsi32_si128(st->R2[4]);
        R20 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(0, 0, 0, 0));
        R21 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(1, 1, 1, 1));
        R22 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(2, 2, 2, 2));
        R23 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(3, 3, 3, 3));
        R24 = _mm_shuffle_epi32(T1, _MM_SHUFFLE(0, 0, 0, 0));
    }
    S21 = _mm_mul_epu32(R21, FIVE);
    S22 = _mm_mul_epu32(R22, FIVE);
    S23 = _mm_mul_epu32(R23, FIVE);
    S24 = _mm_mul_epu32(R24, FIVE);

    if (bytes >= 64) {
        T0  = _mm_loadu_si128((const xmmi *) (const void *) &st->R4[0]);
        T1  = _mm_cvtsi32_si128(st->R4[4]);
        R40 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(0, 0, 0, 0));
        R41 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(1, 1, 1, 1));
        R42 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(2, 2, 2, 2));
        R43 = _mm_shuffle_epi32(T0, _MM_SHUFFLE(3, 3, 3, 3));
        R44 = _mm_shuffle_epi32(T1, _MM_SHUFFLE(0, 0, 0, 0));
        S41 = _mm_mul_epu32(R41, FIVE);
        S42 = _mm_mul_epu32(R42, FIVE);
        S43 = _mm_mul_epu32(R43, FIVE);
        S44 = _mm_mul_epu32(R44, FIVE);

        while (bytes >= 64) {
            xmmi v00, v01, v02, v03, v04;
            xmmi v10, v11, v12, v13, v14;
            xmmi v20, v21, v22, v23, v24;
            xmmi v30, v31, v32, v33, v34;
            xmmi v40, v41, v42, v43, v44;
            xmmi T14, T15;

            /* H *= [r^4,r^4], preload [Mx,My] */
            T15 = S42;
            T0  = H4;
            T0  = _mm_mul_epu32(T0, S41);
            v01 = H3;
            v01 = _mm_mul_epu32(v01, T15);
            T14 = S43;
            T1  = H4;
            T1  = _mm_mul_epu32(T1, T15);
            v11 = H3;
            v11 = _mm_mul_epu32(v11, T14);
            T2  = H4;
            T2  = _mm_mul_epu32(T2, T14);
            T0  = _mm_add_epi64(T0, v01);
            T15 = S44;
            v02 = H2;
            v02 = _mm_mul_epu32(v02, T14);
            T3  = H4;
            T3  = _mm_mul_epu32(T3, T15);
            T1  = _mm_add_epi64(T1, v11);
            v03 = H1;
            v03 = _mm_mul_epu32(v03, T15);
            v12 = H2;
            v12 = _mm_mul_epu32(v12, T15);
            T0  = _mm_add_epi64(T0, v02);
            T14 = R40;
            v21 = H3;
            v21 = _mm_mul_epu32(v21, T15);
            v31 = H3;
            v31 = _mm_mul_epu32(v31, T14);
            T0  = _mm_add_epi64(T0, v03);
            T4  = H4;
            T4  = _mm_mul_epu32(T4, T14);
            T1  = _mm_add_epi64(T1, v12);
            v04 = H0;
            v04 = _mm_mul_epu32(v04, T14);
            T2  = _mm_add_epi64(T2, v21);
            v13 = H1;
            v13 = _mm_mul_epu32(v13, T14);
            T3  = _mm_add_epi64(T3, v31);
            T15 = R41;
            v22 = H2;
            v22 = _mm_mul_epu32(v22, T14);
            v32 = H2;
            v32 = _mm_mul_epu32(v32, T15);
            T0  = _mm_add_epi64(T0, v04);
            v41 = H3;
            v41 = _mm_mul_epu32(v41, T15);
            T1  = _mm_add_epi64(T1, v13);
            v14 = H0;
            v14 = _mm_mul_epu32(v14, T15);
            T2  = _mm_add_epi64(T2, v22);
            T14 = R42;
            T5  = _mm_unpacklo_epi64(
                _mm_loadl_epi64((const xmmi *) (const void *) (m + 0)),
                _mm_loadl_epi64((const xmmi *) (const void *) (m + 16)));
            v23 = H1;
            v23 = _mm_mul_epu32(v23, T15);
            T3  = _mm_add_epi64(T3, v32);
            v33 = H1;
            v33 = _mm_mul_epu32(v33, T14);
            T4  = _mm_add_epi64(T4, v41);
            v42 = H2;
            v42 = _mm_mul_epu32(v42, T14);
            T1  = _mm_add_epi64(T1, v14);
            T15 = R43;
            T6  = _mm_unpacklo_epi64(
                _mm_loadl_epi64((const xmmi *) (const void *) (m + 8)),
                _mm_loadl_epi64((const xmmi *) (const void *) (m + 24)));
            v24 = H0;
            v24 = _mm_mul_epu32(v24, T14);
            T2  = _mm_add_epi64(T2, v23);
            v34 = H0;
            v34 = _mm_mul_epu32(v34, T15);
            T3  = _mm_add_epi64(T3, v33);
            M0  = _mm_and_si128(MMASK, T5);
            v43 = H1;
            v43 = _mm_mul_epu32(v43, T15);
            T4  = _mm_add_epi64(T4, v42);
            M1  = _mm_and_si128(MMASK, _mm_srli_epi64(T5, 26));
            v44 = H0;
            v44 = _mm_mul_epu32(v44, R44);
            T2  = _mm_add_epi64(T2, v24);
            T5  = _mm_or_si128(_mm_srli_epi64(T5, 52), _mm_slli_epi64(T6, 12));
            T3  = _mm_add_epi64(T3, v34);
            M3  = _mm_and_si128(MMASK, _mm_srli_epi64(T6, 14));
            T4  = _mm_add_epi64(T4, v43);
            M2  = _mm_and_si128(MMASK, T5);
            T4  = _mm_add_epi64(T4, v44);
            M4  = _mm_or_si128(_mm_srli_epi64(T6, 40), HIBIT);

            /* H += [Mx',My'] */
            T5 = _mm_loadu_si128((const xmmi *) (const void *) (m + 32));
            T6 = _mm_loadu_si128((const xmmi *) (const void *) (m + 48));
            T7 = _mm_unpacklo_epi32(T5, T6);
            T8 = _mm_unpackhi_epi32(T5, T6);
            M5 = _mm_unpacklo_epi32(T7, _mm_setzero_si128());
            M6 = _mm_unpackhi_epi32(T7, _mm_setzero_si128());
            M7 = _mm_unpacklo_epi32(T8, _mm_setzero_si128());
            M8 = _mm_unpackhi_epi32(T8, _mm_setzero_si128());
            M6 = _mm_slli_epi64(M6, 6);
            M7 = _mm_slli_epi64(M7, 12);
            M8 = _mm_slli_epi64(M8, 18);
            T0 = _mm_add_epi64(T0, M5);
            T1 = _mm_add_epi64(T1, M6);
            T2 = _mm_add_epi64(T2, M7);
            T3 = _mm_add_epi64(T3, M8);
            T4 = _mm_add_epi64(T4, HIBIT);

            /* H += [Mx,My]*[r^2,r^2] */
            T15 = S22;
            v00 = M4;
            v00 = _mm_mul_epu32(v00, S21);
            v01 = M3;
            v01 = _mm_mul_epu32(v01, T15);
            T14 = S23;
            v10 = M4;
            v10 = _mm_mul_epu32(v10, T15);
            v11 = M3;
            v11 = _mm_mul_epu32(v11, T14);
            T0  = _mm_add_epi64(T0, v00);
            v20 = M4;
            v20 = _mm_mul_epu32(v20, T14);
            T0  = _mm_add_epi64(T0, v01);
            T15 = S24;
            v02 = M2;
            v02 = _mm_mul_epu32(v02, T14);
            T1  = _mm_add_epi64(T1, v10);
            v30 = M4;
            v30 = _mm_mul_epu32(v30, T15);
            T1  = _mm_add_epi64(T1, v11);
            v03 = M1;
            v03 = _mm_mul_epu32(v03, T15);
            T2  = _mm_add_epi64(T2, v20);
            v12 = M2;
            v12 = _mm_mul_epu32(v12, T15);
            T0  = _mm_add_epi64(T0, v02);
            T14 = R20;
            v21 = M3;
            v21 = _mm_mul_epu32(v21, T15);
            T3  = _mm_add_epi64(T3, v30);
            v31 = M3;
            v31 = _mm_mul_epu32(v31, T14);
            T0  = _mm_add_epi64(T0, v03);
            v40 = M4;
            v40 = _mm_mul_epu32(v40, T14);
            T1  = _mm_add_epi64(T1, v12);
            v04 = M0;
            v04 = _mm_mul_epu32(v04, T14);
            T2  = _mm_add_epi64(T2, v21);
            v13 = M1;
            v13 = _mm_mul_epu32(v13, T14);
            T3  = _mm_add_epi64(T3, v31);
            T15 = R21;
            v22 = M2;
            v22 = _mm_mul_epu32(v22, T14);
            T4  = _mm_add_epi64(T4, v40);
            v32 = M2;
            v32 = _mm_mul_epu32(v32, T15);
            T0  = _mm_add_epi64(T0, v04);
            v41 = M3;
            v41 = _mm_mul_epu32(v41, T15);
            T1  = _mm_add_epi64(T1, v13);
            v14 = M0;
            v14 = _mm_mul_epu32(v14, T15);
            T2  = _mm_add_epi64(T2, v22);
            T14 = R22;
            v23 = M1;
            v23 = _mm_mul_epu32(v23, T15);
            T3  = _mm_add_epi64(T3, v32);
            v33 = M1;
            v33 = _mm_mul_epu32(v33, T14);
            T4  = _mm_add_epi64(T4, v41);
            v42 = M2;
            v42 = _mm_mul_epu32(v42, T14);
            T1  = _mm_add_epi64(T1, v14);
            T15 = R23;
            v24 = M0;
            v24 = _mm_mul_epu32(v24, T14);
            T2  = _mm_add_epi64(T2, v23);
            v34 = M0;
            v34 = _mm_mul_epu32(v34, T15);
            T3  = _mm_add_epi64(T3, v33);
            v43 = M1;
            v43 = _mm_mul_epu32(v43, T15);
            T4  = _mm_add_epi64(T4, v42);
            v44 = M0;
            v44 = _mm_mul_epu32(v44, R24);
            T2  = _mm_add_epi64(T2, v24);
            T3  = _mm_add_epi64(T3, v34);
            T4  = _mm_add_epi64(T4, v43);
            T4  = _mm_add_epi64(T4, v44);

            /* reduce */
            C1 = _mm_srli_epi64(T0, 26);
            C2 = _mm_srli_epi64(T3, 26);
            T0 = _mm_and_si128(T0, MMASK);
            T3 = _mm_and_si128(T3, MMASK);
            T1 = _mm_add_epi64(T1, C1);
            T4 = _mm_add_epi64(T4, C2);
            C1 = _mm_srli_epi64(T1, 26);
            C2 = _mm_srli_epi64(T4, 26);
            T1 = _mm_and_si128(T1, MMASK);
            T4 = _mm_and_si128(T4, MMASK);
            T2 = _mm_add_epi64(T2, C1);
            T0 = _mm_add_epi64(T0, _mm_mul_epu32(C2, FIVE));
            C1 = _mm_srli_epi64(T2, 26);
            C2 = _mm_srli_epi64(T0, 26);
            T2 = _mm_and_si128(T2, MMASK);
            T0 = _mm_and_si128(T0, MMASK);
            T3 = _mm_add_epi64(T3, C1);
            T1 = _mm_add_epi64(T1, C2);
            C1 = _mm_srli_epi64(T3, 26);
            T3 = _mm_and_si128(T3, MMASK);
            T4 = _mm_add_epi64(T4, C1);

            /* Final: H = (H*[r^4,r^4] + [Mx,My]*[r^2,r^2] + [Mx',My']) */
            H0 = T0;
            H1 = T1;
            H2 = T2;
            H3 = T3;
            H4 = T4;

            m += 64;
            bytes -= 64;
        }
    }

    if (bytes >= 32) {
        xmmi v01, v02, v03, v04;
        xmmi v11, v12, v13, v14;
        xmmi v21, v22, v23, v24;
        xmmi v31, v32, v33, v34;
        xmmi v41, v42, v43, v44;
        xmmi T14, T15;

        /* H *= [r^2,r^2] */
        T15 = S22;
        T0  = H4;
        T0  = _mm_mul_epu32(T0, S21);
        v01 = H3;
        v01 = _mm_mul_epu32(v01, T15);
        T14 = S23;
        T1  = H4;
        T1  = _mm_mul_epu32(T1, T15);
        v11 = H3;
        v11 = _mm_mul_epu32(v11, T14);
        T2  = H4;
        T2  = _mm_mul_epu32(T2, T14);
        T0  = _mm_add_epi64(T0, v01);
        T15 = S24;
        v02 = H2;
        v02 = _mm_mul_epu32(v02, T14);
        T3  = H4;
        T3  = _mm_mul_epu32(T3, T15);
        T1  = _mm_add_epi64(T1, v11);
        v03 = H1;
        v03 = _mm_mul_epu32(v03, T15);
        v12 = H2;
        v12 = _mm_mul_epu32(v12, T15);
        T0  = _mm_add_epi64(T0, v02);
        T14 = R20;
        v21 = H3;
        v21 = _mm_mul_epu32(v21, T15);
        v31 = H3;
        v31 = _mm_mul_epu32(v31, T14);
        T0  = _mm_add_epi64(T0, v03);
        T4  = H4;
        T4  = _mm_mul_epu32(T4, T14);
        T1  = _mm_add_epi64(T1, v12);
        v04 = H0;
        v04 = _mm_mul_epu32(v04, T14);
        T2  = _mm_add_epi64(T2, v21);
        v13 = H1;
        v13 = _mm_mul_epu32(v13, T14);
        T3  = _mm_add_epi64(T3, v31);
        T15 = R21;
        v22 = H2;
        v22 = _mm_mul_epu32(v22, T14);
        v32 = H2;
        v32 = _mm_mul_epu32(v32, T15);
        T0  = _mm_add_epi64(T0, v04);
        v41 = H3;
        v41 = _mm_mul_epu32(v41, T15);
        T1  = _mm_add_epi64(T1, v13);
        v14 = H0;
        v14 = _mm_mul_epu32(v14, T15);
        T2  = _mm_add_epi64(T2, v22);
        T14 = R22;
        v23 = H1;
        v23 = _mm_mul_epu32(v23, T15);
        T3  = _mm_add_epi64(T3, v32);
        v33 = H1;
        v33 = _mm_mul_epu32(v33, T14);
        T4  = _mm_add_epi64(T4, v41);
        v42 = H2;
        v42 = _mm_mul_epu32(v42, T14);
        T1  = _mm_add_epi64(T1, v14);
        T15 = R23;
        v24 = H0;
        v24 = _mm_mul_epu32(v24, T14);
        T2  = _mm_add_epi64(T2, v23);
        v34 = H0;
        v34 = _mm_mul_epu32(v34, T15);
        T3  = _mm_add_epi64(T3, v33);
        v43 = H1;
        v43 = _mm_mul_epu32(v43, T15);
        T4  = _mm_add_epi64(T4, v42);
        v44 = H0;
        v44 = _mm_mul_epu32(v44, R24);
        T2  = _mm_add_epi64(T2, v24);
        T3  = _mm_add_epi64(T3, v34);
        T4  = _mm_add_epi64(T4, v43);
        T4  = _mm_add_epi64(T4, v44);

        /* H += [Mx,My] */
        if (m) {
            T5 = _mm_loadu_si128((const xmmi *) (const void *) (m + 0));
            T6 = _mm_loadu_si128((const xmmi *) (const void *) (m + 16));
            T7 = _mm_unpacklo_epi32(T5, T6);
            T8 = _mm_unpackhi_epi32(T5, T6);
            M0 = _mm_unpacklo_epi32(T7, _mm_setzero_si128());
            M1 = _mm_unpackhi_epi32(T7, _mm_setzero_si128());
            M2 = _mm_unpacklo_epi32(T8, _mm_setzero_si128());
            M3 = _mm_unpackhi_epi32(T8, _mm_setzero_si128());
            M1 = _mm_slli_epi64(M1, 6);
            M2 = _mm_slli_epi64(M2, 12);
            M3 = _mm_slli_epi64(M3, 18);
            T0 = _mm_add_epi64(T0, M0);
            T1 = _mm_add_epi64(T1, M1);
            T2 = _mm_add_epi64(T2, M2);
            T3 = _mm_add_epi64(T3, M3);
            T4 = _mm_add_epi64(T4, HIBIT);
        }

        /* reduce */
        C1 = _mm_srli_epi64(T0, 26);
        C2 = _mm_srli_epi64(T3, 26);
        T0 = _mm_and_si128(T0, MMASK);
        T3 = _mm_and_si128(T3, MMASK);
        T1 = _mm_add_epi64(T1, C1);
        T4 = _mm_add_epi64(T4, C2);
        C1 = _mm_srli_epi64(T1, 26);
        C2 = _mm_srli_epi64(T4, 26);
        T1 = _mm_and_si128(T1, MMASK);
        T4 = _mm_and_si128(T4, MMASK);
        T2 = _mm_add_epi64(T2, C1);
        T0 = _mm_add_epi64(T0, _mm_mul_epu32(C2, FIVE));
        C1 = _mm_srli_epi64(T2, 26);
        C2 = _mm_srli_epi64(T0, 26);
        T2 = _mm_and_si128(T2, MMASK);
        T0 = _mm_and_si128(T0, MMASK);
        T3 = _mm_add_epi64(T3, C1);
        T1 = _mm_add_epi64(T1, C2);
        C1 = _mm_srli_epi64(T3, 26);
        T3 = _mm_and_si128(T3, MMASK);
        T4 = _mm_add_epi64(T4, C1);

        /* H = (H*[r^2,r^2] + [Mx,My]) */
        H0 = T0;
        H1 = T1;
        H2 = T2;
        H3 = T3;
        H4 = T4;
    }

    if (m) {
        T0 = _mm_shuffle_epi32(H0, _MM_SHUFFLE(0, 0, 2, 0));
        T1 = _mm_shuffle_epi32(H1, _MM_SHUFFLE(0, 0, 2, 0));
        T2 = _mm_shuffle_epi32(H2, _MM_SHUFFLE(0, 0, 2, 0));
        T3 = _mm_shuffle_epi32(H3, _MM_SHUFFLE(0, 0, 2, 0));
        T4 = _mm_shuffle_epi32(H4, _MM_SHUFFLE(0, 0, 2, 0));
        T0 = _mm_unpacklo_epi64(T0, T1);
        T1 = _mm_unpacklo_epi64(T2, T3);
        _mm_storeu_si128((xmmi *) (void *) &st->H.hh[0], T0);
        _mm_storeu_si128((xmmi *) (void *) &st->H.hh[4], T1);
        _mm_storel_epi64((xmmi *) (void *) &st->H.hh[8], T4);
    } else {
        uint32_t t0, t1, t2, t3, t4, b;
        uint64_t h0, h1, h2, g0, g1, g2, c, nc;

        /* H = H[0]+H[1] */
        T0 = H0;
        T1 = H1;
        T2 = H2;
        T3 = H3;
        T4 = H4;

        T0 = _mm_add_epi64(T0, _mm_srli_si128(T0, 8));
        T1 = _mm_add_epi64(T1, _mm_srli_si128(T1, 8));
        T2 = _mm_add_epi64(T2, _mm_srli_si128(T2, 8));
        T3 = _mm_add_epi64(T3, _mm_srli_si128(T3, 8));
        T4 = _mm_add_epi64(T4, _mm_srli_si128(T4, 8));

        t0 = _mm_cvtsi128_si32(T0);
        b  = (t0 >> 26);
        t0 &= 0x3ffffff;
        t1 = _mm_cvtsi128_si32(T1) + b;
        b  = (t1 >> 26);
        t1 &= 0x3ffffff;
        t2 = _mm_cvtsi128_si32(T2) + b;
        b  = (t2 >> 26);
        t2 &= 0x3ffffff;
        t3 = _mm_cvtsi128_si32(T3) + b;
        b  = (t3 >> 26);
        t3 &= 0x3ffffff;
        t4 = _mm_cvtsi128_si32(T4) + b;

        /* everything except t4 is in range, so this is all safe */
        h0 = (((uint64_t) t0) | ((uint64_t) t1 << 26)) & 0xfffffffffffull;
        h1 = (((uint64_t) t1 >> 18) | ((uint64_t) t2 << 8) |
              ((uint64_t) t3 << 34)) &
             0xfffffffffffull;
        h2 = (((uint64_t) t3 >> 10) | ((uint64_t) t4 << 16));

        c = (h2 >> 42);
        h2 &= 0x3ffffffffff;
        h0 += c * 5;
        c = (h0 >> 44);
        h0 &= 0xfffffffffff;
        h1 += c;
        c = (h1 >> 44);
        h1 &= 0xfffffffffff;
        h2 += c;
        c = (h2 >> 42);
        h2 &= 0x3ffffffffff;
        h0 += c * 5;
        c = (h0 >> 44);
        h0 &= 0xfffffffffff;
        h1 += c;

        g0 = h0 + 5;
        c  = (g0 >> 44);
        g0 &= 0xfffffffffff;
        g1 = h1 + c;
        c  = (g1 >> 44);
        g1 &= 0xfffffffffff;
        g2 = h2 + c - ((uint64_t) 1 << 42);

        c  = (g2 >> 63) - 1;
        nc = ~c;
        h0 = (h0 & nc) | (g0 & c);
        h1 = (h1 & nc) | (g1 & c);
        h2 = (h2 & nc) | (g2 & c);

        st->H.h[0] = h0;
        st->H.h[1] = h1;
        st->H.h[2] = h2;
    }
}

static void
poly1305_update(poly1305_state_internal_t *st, const unsigned char *m,
                unsigned long long bytes)
{
    unsigned long long i;

    /* handle leftover */
    if (st->leftover) {
        unsigned long long want = (poly1305_block_size - st->leftover);

        if (want > bytes) {
            want = bytes;
        }
        for (i = 0; i < want; i++) {
            st->buffer[st->leftover + i] = m[i];
        }
        bytes -= want;
        m += want;
        st->leftover += want;
        if (st->leftover < poly1305_block_size) {
            return;
        }
        poly1305_blocks(st, st->buffer, poly1305_block_size);
        st->leftover = 0;
    }

    /* process full blocks */
    if (bytes >= poly1305_block_size) {
        unsigned long long want = (bytes & ~(poly1305_block_size - 1));

        poly1305_blocks(st, m, want);
        m += want;
        bytes -= want;
    }

    /* store leftover */
    if (bytes) {
        for (i = 0; i < bytes; i++) {
            st->buffer[st->leftover + i] = m[i];
        }
        st->leftover += bytes;
    }
}

static POLY1305_NOINLINE void
poly1305_finish_ext(poly1305_state_internal_t *st, const unsigned char *m,
                    unsigned long long leftover, unsigned char mac[16])
{
    uint64_t h0, h1, h2;

    if (leftover) {
        CRYPTO_ALIGN(16) unsigned char final[32] = { 0 };

        poly1305_block_copy31(final, m, leftover);
        if (leftover != 16) {
            final[leftover] = 1;
        }
        st->flags |=
            (leftover >= 16) ? poly1305_final_shift8 : poly1305_final_shift16;
        poly1305_blocks(st, final, 32);
    }

    if (st->flags & poly1305_started) {
        /* finalize, H *= [r^2,r], or H *= [r,1] */
        if (!leftover || (leftover > 16)) {
            st->flags |= poly1305_final_r2_r;
        } else {
            st->flags |= poly1305_final_r_1;
        }
        poly1305_blocks(st, NULL, 32);
    }

    h0 = st->H.h[0];
    h1 = st->H.h[1];
    h2 = st->H.h[2];

    /* pad */
    h0 = ((h0) | (h1 << 44));
    h1 = ((h1 >> 20) | (h2 << 24));
#ifdef HAVE_AMD64_ASM
    __asm__ __volatile__(
        "addq %2, %0 ;\n"
        "adcq %3, %1 ;\n"
        : "+r"(h0), "+r"(h1)
        : "r"(st->pad[0]), "r"(st->pad[1])
        : "flags", "cc");
#else
    {
        uint128_t h;

        memcpy(&h, &st->pad[0], 16);
        h += ((uint128_t) h1 << 64) | h0;
        h0 = (uint64_t) h;
        h1 = (uint64_t)(h >> 64);
    }
#endif
    _mm_storeu_si128((xmmi *) (void *) st + 0, _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) st + 1, _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) st + 2, _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) st + 3, _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) st + 4, _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) st + 5, _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) st + 6, _mm_setzero_si128());
    _mm_storeu_si128((xmmi *) (void *) st + 7, _mm_setzero_si128());

    memcpy(&mac[0], &h0, 8);
    memcpy(&mac[8], &h1, 8);

    sodium_memzero((void *) st, sizeof *st);
}

static void
poly1305_finish(poly1305_state_internal_t *st, unsigned char mac[16])
{
    poly1305_finish_ext(st, st->buffer, st->leftover, mac);
}

static int
crypto_onetimeauth_poly1305_sse2_init(crypto_onetimeauth_poly1305_state *state,
                                      const unsigned char *key)
{
    COMPILER_ASSERT(sizeof(crypto_onetimeauth_poly1305_state) >=
                    sizeof(poly1305_state_internal_t));
    poly1305_init_ext((poly1305_state_internal_t *) (void *) state, key, 0U);

    return 0;
}

static int
crypto_onetimeauth_poly1305_sse2_update(
    crypto_onetimeauth_poly1305_state *state, const unsigned char *in,
    unsigned long long inlen)
{
    poly1305_update((poly1305_state_internal_t *) (void *) state, in, inlen);

    return 0;
}

static int
crypto_onetimeauth_poly1305_sse2_final(crypto_onetimeauth_poly1305_state *state,
                                       unsigned char *out)
{
    poly1305_finish((poly1305_state_internal_t *) (void *) state, out);

    return 0;
}

static int
crypto_onetimeauth_poly1305_sse2(unsigned char *out, const unsigned char *m,
                                 unsigned long long   inlen,
                                 const unsigned char *key)
{
    CRYPTO_ALIGN(64) poly1305_state_internal_t st;
    unsigned long long                         blocks;

    poly1305_init_ext(&st, key, inlen);
    blocks = inlen & ~31;
    if (blocks > 0) {
        poly1305_blocks(&st, m, blocks);
        m += blocks;
        inlen -= blocks;
    }
    poly1305_finish_ext(&st, m, inlen, out);

    return 0;
}

static int
crypto_onetimeauth_poly1305_sse2_verify(const unsigned char *h,
                                        const unsigned char *in,
                                        unsigned long long   inlen,
                                        const unsigned char *k)
{
    unsigned char correct[16];

    crypto_onetimeauth_poly1305_sse2(correct, in, inlen, k);

    return crypto_verify_16(h, correct);
}

struct crypto_onetimeauth_poly1305_implementation
    crypto_onetimeauth_poly1305_sse2_implementation = {
        SODIUM_C99(.onetimeauth =) crypto_onetimeauth_poly1305_sse2,
        SODIUM_C99(.onetimeauth_verify =)
            crypto_onetimeauth_poly1305_sse2_verify,
        SODIUM_C99(.onetimeauth_init =) crypto_onetimeauth_poly1305_sse2_init,
        SODIUM_C99(.onetimeauth_update =)
            crypto_onetimeauth_poly1305_sse2_update,
        SODIUM_C99(.onetimeauth_final =) crypto_onetimeauth_poly1305_sse2_final
    };

# ifdef __clang__
#  pragma clang attribute pop
# endif

#endif

int poly1305_sse2_link_warning_dummy = 0;
#undef poly1305_state_internal_t
#undef poly1305_init
#undef poly1305_blocks
#undef poly1305_update
#undef poly1305_finish
#undef poly1305_final
#undef poly1305_state_internal
#undef POLY1305_NOINLINE
#undef poly1305_block_size
#undef _mm_loadl_epi64

/* ================= sodium_onetimeauth_poly1305.c ================= */
#define implementation implementation__poly


#if defined(HAVE_TI_MODE) && defined(HAVE_EMMINTRIN_H)
#endif

static const crypto_onetimeauth_poly1305_implementation *implementation =
    &crypto_onetimeauth_poly1305_donna_implementation;

int
crypto_onetimeauth_poly1305(unsigned char *out, const unsigned char *in,
                            unsigned long long inlen, const unsigned char *k)
{
    return implementation->onetimeauth(out, in, inlen, k);
}

int
crypto_onetimeauth_poly1305_verify(const unsigned char *h,
                                   const unsigned char *in,
                                   unsigned long long   inlen,
                                   const unsigned char *k)
{
    return implementation->onetimeauth_verify(h, in, inlen, k);
}

int
crypto_onetimeauth_poly1305_init(crypto_onetimeauth_poly1305_state *state,
                                 const unsigned char *key)
{
    return implementation->onetimeauth_init(state, key);
}

int
crypto_onetimeauth_poly1305_update(crypto_onetimeauth_poly1305_state *state,
                                   const unsigned char *in,
                                   unsigned long long inlen)
{
    return implementation->onetimeauth_update(state, in, inlen);
}

int
crypto_onetimeauth_poly1305_final(crypto_onetimeauth_poly1305_state *state,
                                  unsigned char *out)
{
    return implementation->onetimeauth_final(state, out);
}

size_t
crypto_onetimeauth_poly1305_bytes(void)
{
    return crypto_onetimeauth_poly1305_BYTES;
}

size_t
crypto_onetimeauth_poly1305_keybytes(void)
{
    return crypto_onetimeauth_poly1305_KEYBYTES;
}

size_t
crypto_onetimeauth_poly1305_statebytes(void)
{
    return sizeof(crypto_onetimeauth_poly1305_state);
}

void
crypto_onetimeauth_poly1305_keygen(
    unsigned char k[crypto_onetimeauth_poly1305_KEYBYTES])
{
    randombytes_buf(k, crypto_onetimeauth_poly1305_KEYBYTES);
}

int
_crypto_onetimeauth_poly1305_pick_best_implementation(void)
{
    implementation = &crypto_onetimeauth_poly1305_donna_implementation;
#if defined(HAVE_TI_MODE) && defined(HAVE_EMMINTRIN_H)
    if (sodium_runtime_has_sse2()) {
        #if NETCODE_CRYPTO_LOGS
        printf( "poly1305 -> sse3\n" );
        #endif // #if NETCODE_CRYPTO_LOGS
        implementation = &crypto_onetimeauth_poly1305_sse2_implementation;
        return 0;
    }
#endif
    #if NETCODE_CRYPTO_LOGS
    printf( "poly1305 -> ref\n" );
    #endif // #if NETCODE_CRYPTO_LOGS
    return 0;
}
#undef implementation

/* ================= sodium_aead_chacha20poly1305.c ================= */

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>



static const unsigned char _pad0[16] = { 0 };

int
crypto_aead_chacha20poly1305_encrypt_detached(unsigned char *c,
                                              unsigned char *mac,
                                              unsigned long long *maclen_p,
                                              const unsigned char *m,
                                              unsigned long long mlen,
                                              const unsigned char *ad,
                                              unsigned long long adlen,
                                              const unsigned char *nsec,
                                              const unsigned char *npub,
                                              const unsigned char *k)
{
    crypto_onetimeauth_poly1305_state state;
    unsigned char                     block0[64U];
    unsigned char                     slen[8U];

    (void) nsec;
    crypto_stream_chacha20(block0, sizeof block0, npub, k);
    crypto_onetimeauth_poly1305_init(&state, block0);
    sodium_memzero(block0, sizeof block0);

    crypto_onetimeauth_poly1305_update(&state, ad, adlen);
    STORE64_LE(slen, (uint64_t) adlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    crypto_stream_chacha20_xor_ic(c, m, mlen, npub, 1U, k);

    crypto_onetimeauth_poly1305_update(&state, c, mlen);
    STORE64_LE(slen, (uint64_t) mlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    crypto_onetimeauth_poly1305_final(&state, mac);
    sodium_memzero(&state, sizeof state);

    if (maclen_p != NULL) {
        *maclen_p = crypto_aead_chacha20poly1305_ABYTES;
    }
    return 0;
}

int
crypto_aead_chacha20poly1305_encrypt(unsigned char *c,
                                     unsigned long long *clen_p,
                                     const unsigned char *m,
                                     unsigned long long mlen,
                                     const unsigned char *ad,
                                     unsigned long long adlen,
                                     const unsigned char *nsec,
                                     const unsigned char *npub,
                                     const unsigned char *k)
{
    unsigned long long clen = 0ULL;
    int                ret;

    if (mlen > crypto_aead_chacha20poly1305_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    ret = crypto_aead_chacha20poly1305_encrypt_detached(c,
                                                        c + mlen, NULL,
                                                        m, mlen,
                                                        ad, adlen,
                                                        nsec, npub, k);
    if (clen_p != NULL) {
        if (ret == 0) {
            clen = mlen + crypto_aead_chacha20poly1305_ABYTES;
        }
        *clen_p = clen;
    }
    return ret;
}

int
crypto_aead_chacha20poly1305_ietf_encrypt_detached(unsigned char *c,
                                                   unsigned char *mac,
                                                   unsigned long long *maclen_p,
                                                   const unsigned char *m,
                                                   unsigned long long mlen,
                                                   const unsigned char *ad,
                                                   unsigned long long adlen,
                                                   const unsigned char *nsec,
                                                   const unsigned char *npub,
                                                   const unsigned char *k)
{
    crypto_onetimeauth_poly1305_state state;
    unsigned char                     block0[64U];
    unsigned char                     slen[8U];

    (void) nsec;
    crypto_stream_chacha20_ietf(block0, sizeof block0, npub, k);
    crypto_onetimeauth_poly1305_init(&state, block0);
    sodium_memzero(block0, sizeof block0);

    crypto_onetimeauth_poly1305_update(&state, ad, adlen);
    crypto_onetimeauth_poly1305_update(&state, _pad0, (0x10 - adlen) & 0xf);

    crypto_stream_chacha20_ietf_xor_ic(c, m, mlen, npub, 1U, k);

    crypto_onetimeauth_poly1305_update(&state, c, mlen);
    crypto_onetimeauth_poly1305_update(&state, _pad0, (0x10 - mlen) & 0xf);

    STORE64_LE(slen, (uint64_t) adlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    STORE64_LE(slen, (uint64_t) mlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    crypto_onetimeauth_poly1305_final(&state, mac);
    sodium_memzero(&state, sizeof state);

    if (maclen_p != NULL) {
        *maclen_p = crypto_aead_chacha20poly1305_ietf_ABYTES;
    }
    return 0;
}

int
crypto_aead_chacha20poly1305_ietf_encrypt(unsigned char *c,
                                          unsigned long long *clen_p,
                                          const unsigned char *m,
                                          unsigned long long mlen,
                                          const unsigned char *ad,
                                          unsigned long long adlen,
                                          const unsigned char *nsec,
                                          const unsigned char *npub,
                                          const unsigned char *k)
{
    unsigned long long clen = 0ULL;
    int                ret;

    if (mlen > crypto_aead_chacha20poly1305_ietf_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    ret = crypto_aead_chacha20poly1305_ietf_encrypt_detached(c,
                                                             c + mlen, NULL,
                                                             m, mlen,
                                                             ad, adlen,
                                                             nsec, npub, k);
    if (clen_p != NULL) {
        if (ret == 0) {
            clen = mlen + crypto_aead_chacha20poly1305_ietf_ABYTES;
        }
        *clen_p = clen;
    }
    return ret;
}

int
crypto_aead_chacha20poly1305_decrypt_detached(unsigned char *m,
                                              unsigned char *nsec,
                                              const unsigned char *c,
                                              unsigned long long clen,
                                              const unsigned char *mac,
                                              const unsigned char *ad,
                                              unsigned long long adlen,
                                              const unsigned char *npub,
                                              const unsigned char *k)
{
    crypto_onetimeauth_poly1305_state state;
    unsigned char                     block0[64U];
    unsigned char                     slen[8U];
    unsigned char                     computed_mac[crypto_aead_chacha20poly1305_ABYTES];
    unsigned long long                mlen;
    int                               ret;

    (void) nsec;
    crypto_stream_chacha20(block0, sizeof block0, npub, k);
    crypto_onetimeauth_poly1305_init(&state, block0);
    sodium_memzero(block0, sizeof block0);

    crypto_onetimeauth_poly1305_update(&state, ad, adlen);
    STORE64_LE(slen, (uint64_t) adlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    mlen = clen;
    crypto_onetimeauth_poly1305_update(&state, c, mlen);
    STORE64_LE(slen, (uint64_t) mlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    crypto_onetimeauth_poly1305_final(&state, computed_mac);
    sodium_memzero(&state, sizeof state);

    COMPILER_ASSERT(sizeof computed_mac == 16U);
    ret = crypto_verify_16(computed_mac, mac);
    sodium_memzero(computed_mac, sizeof computed_mac);
    if (m == NULL) {
        return ret;
    }
    if (ret != 0) {
        memset(m, 0, mlen);
        return -1;
    }
    crypto_stream_chacha20_xor_ic(m, c, mlen, npub, 1U, k);

    return 0;
}

int
crypto_aead_chacha20poly1305_decrypt(unsigned char *m,
                                     unsigned long long *mlen_p,
                                     unsigned char *nsec,
                                     const unsigned char *c,
                                     unsigned long long clen,
                                     const unsigned char *ad,
                                     unsigned long long adlen,
                                     const unsigned char *npub,
                                     const unsigned char *k)
{
    unsigned long long mlen = 0ULL;
    int                ret = -1;

    if (clen >= crypto_aead_chacha20poly1305_ABYTES) {
        ret = crypto_aead_chacha20poly1305_decrypt_detached
            (m, nsec,
             c, clen - crypto_aead_chacha20poly1305_ABYTES,
             c + clen - crypto_aead_chacha20poly1305_ABYTES,
             ad, adlen, npub, k);
    }
    if (mlen_p != NULL) {
        if (ret == 0) {
            mlen = clen - crypto_aead_chacha20poly1305_ABYTES;
        }
        *mlen_p = mlen;
    }
    return ret;
}

int
crypto_aead_chacha20poly1305_ietf_decrypt_detached(unsigned char *m,
                                                   unsigned char *nsec,
                                                   const unsigned char *c,
                                                   unsigned long long clen,
                                                   const unsigned char *mac,
                                                   const unsigned char *ad,
                                                   unsigned long long adlen,
                                                   const unsigned char *npub,
                                                   const unsigned char *k)
{
    crypto_onetimeauth_poly1305_state state;
    unsigned char                     block0[64U];
    unsigned char                     slen[8U];
    unsigned char                     computed_mac[crypto_aead_chacha20poly1305_ietf_ABYTES];
    unsigned long long                mlen;
    int                               ret;

    (void) nsec;
    crypto_stream_chacha20_ietf(block0, sizeof block0, npub, k);
    crypto_onetimeauth_poly1305_init(&state, block0);
    sodium_memzero(block0, sizeof block0);

    crypto_onetimeauth_poly1305_update(&state, ad, adlen);
    crypto_onetimeauth_poly1305_update(&state, _pad0, (0x10 - adlen) & 0xf);

    mlen = clen;
    crypto_onetimeauth_poly1305_update(&state, c, mlen);
    crypto_onetimeauth_poly1305_update(&state, _pad0, (0x10 - mlen) & 0xf);

    STORE64_LE(slen, (uint64_t) adlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    STORE64_LE(slen, (uint64_t) mlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    crypto_onetimeauth_poly1305_final(&state, computed_mac);
    sodium_memzero(&state, sizeof state);

    COMPILER_ASSERT(sizeof computed_mac == 16U);
    ret = crypto_verify_16(computed_mac, mac);
    sodium_memzero(computed_mac, sizeof computed_mac);
    if (m == NULL) {
        return ret;
    }
    if (ret != 0) {
        memset(m, 0, mlen);
        return -1;
    }
    crypto_stream_chacha20_ietf_xor_ic(m, c, mlen, npub, 1U, k);

    return 0;
}

int
crypto_aead_chacha20poly1305_ietf_decrypt(unsigned char *m,
                                          unsigned long long *mlen_p,
                                          unsigned char *nsec,
                                          const unsigned char *c,
                                          unsigned long long clen,
                                          const unsigned char *ad,
                                          unsigned long long adlen,
                                          const unsigned char *npub,
                                          const unsigned char *k)
{
    unsigned long long mlen = 0ULL;
    int                ret = -1;

    if (clen >= crypto_aead_chacha20poly1305_ietf_ABYTES) {
        ret = crypto_aead_chacha20poly1305_ietf_decrypt_detached
            (m, nsec,
             c, clen - crypto_aead_chacha20poly1305_ietf_ABYTES,
             c + clen - crypto_aead_chacha20poly1305_ietf_ABYTES,
             ad, adlen, npub, k);
    }
    if (mlen_p != NULL) {
        if (ret == 0) {
            mlen = clen - crypto_aead_chacha20poly1305_ietf_ABYTES;
        }
        *mlen_p = mlen;
    }
    return ret;
}

size_t
crypto_aead_chacha20poly1305_ietf_keybytes(void)
{
    return crypto_aead_chacha20poly1305_ietf_KEYBYTES;
}

size_t
crypto_aead_chacha20poly1305_ietf_npubbytes(void)
{
    return crypto_aead_chacha20poly1305_ietf_NPUBBYTES;
}

size_t
crypto_aead_chacha20poly1305_ietf_nsecbytes(void)
{
    return crypto_aead_chacha20poly1305_ietf_NSECBYTES;
}

size_t
crypto_aead_chacha20poly1305_ietf_abytes(void)
{
    return crypto_aead_chacha20poly1305_ietf_ABYTES;
}

size_t
crypto_aead_chacha20poly1305_ietf_messagebytes_max(void)
{
    return crypto_aead_chacha20poly1305_ietf_MESSAGEBYTES_MAX;
}

void
crypto_aead_chacha20poly1305_ietf_keygen(unsigned char k[crypto_aead_chacha20poly1305_ietf_KEYBYTES])
{
    randombytes_buf(k, crypto_aead_chacha20poly1305_ietf_KEYBYTES);
}

size_t
crypto_aead_chacha20poly1305_keybytes(void)
{
    return crypto_aead_chacha20poly1305_KEYBYTES;
}

size_t
crypto_aead_chacha20poly1305_npubbytes(void)
{
    return crypto_aead_chacha20poly1305_NPUBBYTES;
}

size_t
crypto_aead_chacha20poly1305_nsecbytes(void)
{
    return crypto_aead_chacha20poly1305_NSECBYTES;
}

size_t
crypto_aead_chacha20poly1305_abytes(void)
{
    return crypto_aead_chacha20poly1305_ABYTES;
}

size_t
crypto_aead_chacha20poly1305_messagebytes_max(void)
{
    return crypto_aead_chacha20poly1305_MESSAGEBYTES_MAX;
}

void
crypto_aead_chacha20poly1305_keygen(unsigned char k[crypto_aead_chacha20poly1305_KEYBYTES])
{
    randombytes_buf(k, crypto_aead_chacha20poly1305_KEYBYTES);
}

/* ================= sodium_aead_xchacha20poly1305.c ================= */
#define _pad0 _pad0__x

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>



static const unsigned char _pad0[16] = { 0 };

static int
_encrypt_detached(unsigned char *c,
                  unsigned char *mac,
                  unsigned long long *maclen_p,
                  const unsigned char *m,
                  unsigned long long mlen,
                  const unsigned char *ad,
                  unsigned long long adlen,
                  const unsigned char *nsec,
                  const unsigned char *npub,
                  const unsigned char *k)
{
    crypto_onetimeauth_poly1305_state state;
    unsigned char                     block0[64U];
    unsigned char                     slen[8U];

    (void) nsec;
    crypto_stream_chacha20_ietf_ext(block0, sizeof block0, npub, k);
    crypto_onetimeauth_poly1305_init(&state, block0);
    sodium_memzero(block0, sizeof block0);

    crypto_onetimeauth_poly1305_update(&state, ad, adlen);
    crypto_onetimeauth_poly1305_update(&state, _pad0, (0x10 - adlen) & 0xf);

    crypto_stream_chacha20_ietf_ext_xor_ic(c, m, mlen, npub, 1U, k);

    crypto_onetimeauth_poly1305_update(&state, c, mlen);
    crypto_onetimeauth_poly1305_update(&state, _pad0, (0x10 - mlen) & 0xf);

    STORE64_LE(slen, (uint64_t) adlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    STORE64_LE(slen, (uint64_t) mlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    crypto_onetimeauth_poly1305_final(&state, mac);
    sodium_memzero(&state, sizeof state);

    if (maclen_p != NULL) {
        *maclen_p = crypto_aead_chacha20poly1305_ietf_ABYTES;
    }
    return 0;
}

static int
_decrypt_detached(unsigned char *m,
                  unsigned char *nsec,
                  const unsigned char *c,
                  unsigned long long clen,
                  const unsigned char *mac,
                  const unsigned char *ad,
                  unsigned long long adlen,
                  const unsigned char *npub,
                  const unsigned char *k)
{
    crypto_onetimeauth_poly1305_state state;
    unsigned char                     block0[64U];
    unsigned char                     slen[8U];
    unsigned char                     computed_mac[crypto_aead_chacha20poly1305_ietf_ABYTES];
    unsigned long long                mlen;
    int                               ret;

    (void) nsec;
    crypto_stream_chacha20_ietf_ext(block0, sizeof block0, npub, k);
    crypto_onetimeauth_poly1305_init(&state, block0);
    sodium_memzero(block0, sizeof block0);

    crypto_onetimeauth_poly1305_update(&state, ad, adlen);
    crypto_onetimeauth_poly1305_update(&state, _pad0, (0x10 - adlen) & 0xf);

    mlen = clen;
    crypto_onetimeauth_poly1305_update(&state, c, mlen);
    crypto_onetimeauth_poly1305_update(&state, _pad0, (0x10 - mlen) & 0xf);

    STORE64_LE(slen, (uint64_t) adlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    STORE64_LE(slen, (uint64_t) mlen);
    crypto_onetimeauth_poly1305_update(&state, slen, sizeof slen);

    crypto_onetimeauth_poly1305_final(&state, computed_mac);
    sodium_memzero(&state, sizeof state);

    COMPILER_ASSERT(sizeof computed_mac == 16U);
    ret = crypto_verify_16(computed_mac, mac);
    sodium_memzero(computed_mac, sizeof computed_mac);
    if (m == NULL) {
        return ret;
    }
    if (ret != 0) {
        memset(m, 0, mlen);
        return -1;
    }
    crypto_stream_chacha20_ietf_ext_xor_ic(m, c, mlen, npub, 1U, k);

    return 0;
}

int
crypto_aead_xchacha20poly1305_ietf_encrypt_detached(unsigned char *c,
                                                    unsigned char *mac,
                                                    unsigned long long *maclen_p,
                                                    const unsigned char *m,
                                                    unsigned long long mlen,
                                                    const unsigned char *ad,
                                                    unsigned long long adlen,
                                                    const unsigned char *nsec,
                                                    const unsigned char *npub,
                                                    const unsigned char *k)
{
    unsigned char k2[crypto_core_hchacha20_OUTPUTBYTES];
    unsigned char npub2[crypto_aead_chacha20poly1305_ietf_NPUBBYTES] = { 0 };
    int           ret;

    crypto_core_hchacha20(k2, npub, k, NULL);
    memcpy(npub2 + 4, npub + crypto_core_hchacha20_INPUTBYTES,
           crypto_aead_chacha20poly1305_ietf_NPUBBYTES - 4);
    ret = _encrypt_detached(c, mac, maclen_p, m, mlen, ad, adlen,
                            nsec, npub2, k2);
    sodium_memzero(k2, crypto_core_hchacha20_OUTPUTBYTES);

    return ret;
}

int
crypto_aead_xchacha20poly1305_ietf_encrypt(unsigned char *c,
                                           unsigned long long *clen_p,
                                           const unsigned char *m,
                                           unsigned long long mlen,
                                           const unsigned char *ad,
                                           unsigned long long adlen,
                                           const unsigned char *nsec,
                                           const unsigned char *npub,
                                           const unsigned char *k)
{
    unsigned long long clen = 0ULL;
    int                ret;

    if (mlen > crypto_aead_xchacha20poly1305_ietf_MESSAGEBYTES_MAX) {
        sodium_misuse();
    }
    ret = crypto_aead_xchacha20poly1305_ietf_encrypt_detached
        (c, c + mlen, NULL, m, mlen, ad, adlen, nsec, npub, k);
    if (clen_p != NULL) {
        if (ret == 0) {
            clen = mlen + crypto_aead_xchacha20poly1305_ietf_ABYTES;
        }
        *clen_p = clen;
    }
    return ret;
}

int
crypto_aead_xchacha20poly1305_ietf_decrypt_detached(unsigned char *m,
                                                    unsigned char *nsec,
                                                    const unsigned char *c,
                                                    unsigned long long clen,
                                                    const unsigned char *mac,
                                                    const unsigned char *ad,
                                                    unsigned long long adlen,
                                                    const unsigned char *npub,
                                                    const unsigned char *k)
{
    unsigned char k2[crypto_core_hchacha20_OUTPUTBYTES];
    unsigned char npub2[crypto_aead_chacha20poly1305_ietf_NPUBBYTES] = { 0 };
    int           ret;

    crypto_core_hchacha20(k2, npub, k, NULL);
    memcpy(npub2 + 4, npub + crypto_core_hchacha20_INPUTBYTES,
           crypto_aead_chacha20poly1305_ietf_NPUBBYTES - 4);
    ret = _decrypt_detached(m, nsec, c, clen, mac, ad, adlen, npub2, k2);
    sodium_memzero(k2, crypto_core_hchacha20_OUTPUTBYTES);

    return ret;
}

int
crypto_aead_xchacha20poly1305_ietf_decrypt(unsigned char *m,
                                           unsigned long long *mlen_p,
                                           unsigned char *nsec,
                                           const unsigned char *c,
                                           unsigned long long clen,
                                           const unsigned char *ad,
                                           unsigned long long adlen,
                                           const unsigned char *npub,
                                           const unsigned char *k)
{
    unsigned long long mlen = 0ULL;
    int                ret  = -1;

    if (clen >= crypto_aead_xchacha20poly1305_ietf_ABYTES) {
        ret = crypto_aead_xchacha20poly1305_ietf_decrypt_detached
            (m, nsec,
             c, clen - crypto_aead_xchacha20poly1305_ietf_ABYTES,
             c + clen - crypto_aead_xchacha20poly1305_ietf_ABYTES,
             ad, adlen, npub, k);
    }
    if (mlen_p != NULL) {
        if (ret == 0) {
            mlen = clen - crypto_aead_xchacha20poly1305_ietf_ABYTES;
        }
        *mlen_p = mlen;
    }
    return ret;
}

size_t
crypto_aead_xchacha20poly1305_ietf_keybytes(void)
{
    return crypto_aead_xchacha20poly1305_ietf_KEYBYTES;
}

size_t
crypto_aead_xchacha20poly1305_ietf_npubbytes(void)
{
    return crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
}

size_t
crypto_aead_xchacha20poly1305_ietf_nsecbytes(void)
{
    return crypto_aead_xchacha20poly1305_ietf_NSECBYTES;
}

size_t
crypto_aead_xchacha20poly1305_ietf_abytes(void)
{
    return crypto_aead_xchacha20poly1305_ietf_ABYTES;
}

size_t
crypto_aead_xchacha20poly1305_ietf_messagebytes_max(void)
{
    return crypto_aead_xchacha20poly1305_ietf_MESSAGEBYTES_MAX;
}

void
crypto_aead_xchacha20poly1305_ietf_keygen(unsigned char k[crypto_aead_xchacha20poly1305_ietf_KEYBYTES])
{
    randombytes_buf(k, crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
}
#undef _pad0

