/* Amalgamated libsodium subset for yojimbo/netcode — generated; do not edit by hand. */
#ifndef SODIUM_AMALGAM_H
#define SODIUM_AMALGAM_H

/* ================= sodium_stream_chacha20.h ================= */

#ifndef stream_chacha20_H
#define stream_chacha20_H

#include <stdint.h>

typedef struct crypto_stream_chacha20_implementation {
    int (*stream)(unsigned char *c, unsigned long long clen,
                  const unsigned char *n, const unsigned char *k);
    int (*stream_ietf_ext)(unsigned char *c, unsigned long long clen,
                           const unsigned char *n, const unsigned char *k);
    int (*stream_xor_ic)(unsigned char *c, const unsigned char *m,
                         unsigned long long mlen,
                         const unsigned char *n, uint64_t ic,
                         const unsigned char *k);
    int (*stream_ietf_ext_xor_ic)(unsigned char *c, const unsigned char *m,
                                  unsigned long long mlen,
                                  const unsigned char *n, uint32_t ic,
                                  const unsigned char *k);
} crypto_stream_chacha20_implementation;

#endif
/* ================= sodium_export.h ================= */

#ifndef sodium_export_H
#define sodium_export_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#if !defined(__clang__) && !defined(__GNUC__)
# ifdef __attribute__
#  undef __attribute__
# endif
# define __attribute__(a)
#endif

#ifndef CRYPTO_ALIGN
# if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#  define CRYPTO_ALIGN(x) __declspec(align(x))
# else
#  define CRYPTO_ALIGN(x) __attribute__ ((aligned(x)))
# endif
#endif

#define SODIUM_MIN(A, B) ((A) < (B) ? (A) : (B))
#define SODIUM_SIZE_MAX SODIUM_MIN(UINT64_MAX, SIZE_MAX)

#endif
/* ================= sodium_crypto_stream_chacha20.h ================= */
#ifndef crypto_stream_chacha20_H
#define crypto_stream_chacha20_H

/*
 *  WARNING: This is just a stream cipher. It is NOT authenticated encryption.
 *  While it provides some protection against eavesdropping, it does NOT
 *  provide any security against active attacks.
 *  Unless you know what you're doing, what you are looking for is probably
 *  the crypto_box functions.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

#define crypto_stream_chacha20_KEYBYTES 32U
size_t crypto_stream_chacha20_keybytes(void);

#define crypto_stream_chacha20_NONCEBYTES 8U
size_t crypto_stream_chacha20_noncebytes(void);

#define crypto_stream_chacha20_MESSAGEBYTES_MAX SODIUM_SIZE_MAX
size_t crypto_stream_chacha20_messagebytes_max(void);

/* ChaCha20 with a 64-bit nonce and a 64-bit counter, as originally designed */

int crypto_stream_chacha20(unsigned char *c, unsigned long long clen,
                           const unsigned char *n, const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_stream_chacha20_xor(unsigned char *c, const unsigned char *m,
                               unsigned long long mlen, const unsigned char *n,
                               const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_stream_chacha20_xor_ic(unsigned char *c, const unsigned char *m,
                                  unsigned long long mlen,
                                  const unsigned char *n, uint64_t ic,
                                  const unsigned char *k)
            __attribute__ ((nonnull));

void crypto_stream_chacha20_keygen(unsigned char k[crypto_stream_chacha20_KEYBYTES])
            __attribute__ ((nonnull));

/* ChaCha20 with a 96-bit nonce and a 32-bit counter (IETF) */

#define crypto_stream_chacha20_ietf_KEYBYTES 32U
size_t crypto_stream_chacha20_ietf_keybytes(void);

#define crypto_stream_chacha20_ietf_NONCEBYTES 12U
size_t crypto_stream_chacha20_ietf_noncebytes(void);

#define crypto_stream_chacha20_ietf_MESSAGEBYTES_MAX \
    SODIUM_MIN(SODIUM_SIZE_MAX, 64ULL * (1ULL << 32))
size_t crypto_stream_chacha20_ietf_messagebytes_max(void);

int crypto_stream_chacha20_ietf(unsigned char *c, unsigned long long clen,
                                const unsigned char *n, const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_stream_chacha20_ietf_xor(unsigned char *c, const unsigned char *m,
                                    unsigned long long mlen, const unsigned char *n,
                                    const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_stream_chacha20_ietf_xor_ic(unsigned char *c, const unsigned char *m,
                                       unsigned long long mlen,
                                       const unsigned char *n, uint32_t ic,
                                       const unsigned char *k)
            __attribute__ ((nonnull));

void crypto_stream_chacha20_ietf_keygen(unsigned char k[crypto_stream_chacha20_ietf_KEYBYTES])
            __attribute__ ((nonnull));

/* Aliases */

#define crypto_stream_chacha20_IETF_KEYBYTES crypto_stream_chacha20_ietf_KEYBYTES
#define crypto_stream_chacha20_IETF_NONCEBYTES crypto_stream_chacha20_ietf_NONCEBYTES
#define crypto_stream_chacha20_IETF_MESSAGEBYTES_MAX crypto_stream_chacha20_ietf_MESSAGEBYTES_MAX

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_chacha20-ref.h ================= */

#include <stdint.h>


extern struct crypto_stream_chacha20_implementation
    crypto_stream_chacha20_ref_implementation;
/* ================= sodium_core.h ================= */

#ifndef sodium_core_H
#define sodium_core_H


#ifdef __cplusplus
extern "C" {
#endif

int sodium_init(void)
            __attribute__ ((warn_unused_result));

/* ---- */

int sodium_set_misuse_handler(void (*handler)(void));

void sodium_misuse(void)
            __attribute__ ((noreturn));

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_crypto_aead_chacha20poly1305.h ================= */
#ifndef crypto_aead_chacha20poly1305_H
#define crypto_aead_chacha20poly1305_H

#include <stddef.h>


#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

/* -- IETF ChaCha20-Poly1305 construction with a 96-bit nonce and a 32-bit internal counter -- */

#define crypto_aead_chacha20poly1305_ietf_KEYBYTES 32U
size_t crypto_aead_chacha20poly1305_ietf_keybytes(void);

#define crypto_aead_chacha20poly1305_ietf_NSECBYTES 0U
size_t crypto_aead_chacha20poly1305_ietf_nsecbytes(void);

#define crypto_aead_chacha20poly1305_ietf_NPUBBYTES 12U

size_t crypto_aead_chacha20poly1305_ietf_npubbytes(void);

#define crypto_aead_chacha20poly1305_ietf_ABYTES 16U
size_t crypto_aead_chacha20poly1305_ietf_abytes(void);

#define crypto_aead_chacha20poly1305_ietf_MESSAGEBYTES_MAX \
    SODIUM_MIN(SODIUM_SIZE_MAX - crypto_aead_chacha20poly1305_ietf_ABYTES, \
               (64ULL * ((1ULL << 32) - 1ULL)))
size_t crypto_aead_chacha20poly1305_ietf_messagebytes_max(void);

int crypto_aead_chacha20poly1305_ietf_encrypt(unsigned char *c,
                                              unsigned long long *clen_p,
                                              const unsigned char *m,
                                              unsigned long long mlen,
                                              const unsigned char *ad,
                                              unsigned long long adlen,
                                              const unsigned char *nsec,
                                              const unsigned char *npub,
                                              const unsigned char *k)
            __attribute__ ((nonnull(1, 8, 9)));

int crypto_aead_chacha20poly1305_ietf_decrypt(unsigned char *m,
                                              unsigned long long *mlen_p,
                                              unsigned char *nsec,
                                              const unsigned char *c,
                                              unsigned long long clen,
                                              const unsigned char *ad,
                                              unsigned long long adlen,
                                              const unsigned char *npub,
                                              const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(4, 8, 9)));

int crypto_aead_chacha20poly1305_ietf_encrypt_detached(unsigned char *c,
                                                       unsigned char *mac,
                                                       unsigned long long *maclen_p,
                                                       const unsigned char *m,
                                                       unsigned long long mlen,
                                                       const unsigned char *ad,
                                                       unsigned long long adlen,
                                                       const unsigned char *nsec,
                                                       const unsigned char *npub,
                                                       const unsigned char *k)
            __attribute__ ((nonnull(1, 2, 9, 10)));

int crypto_aead_chacha20poly1305_ietf_decrypt_detached(unsigned char *m,
                                                       unsigned char *nsec,
                                                       const unsigned char *c,
                                                       unsigned long long clen,
                                                       const unsigned char *mac,
                                                       const unsigned char *ad,
                                                       unsigned long long adlen,
                                                       const unsigned char *npub,
                                                       const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(3, 5, 8, 9)));

void crypto_aead_chacha20poly1305_ietf_keygen(unsigned char k[crypto_aead_chacha20poly1305_ietf_KEYBYTES])
            __attribute__ ((nonnull));

/* -- Original ChaCha20-Poly1305 construction with a 64-bit nonce and a 64-bit internal counter -- */

#define crypto_aead_chacha20poly1305_KEYBYTES 32U
size_t crypto_aead_chacha20poly1305_keybytes(void);

#define crypto_aead_chacha20poly1305_NSECBYTES 0U
size_t crypto_aead_chacha20poly1305_nsecbytes(void);

#define crypto_aead_chacha20poly1305_NPUBBYTES 8U
size_t crypto_aead_chacha20poly1305_npubbytes(void);

#define crypto_aead_chacha20poly1305_ABYTES 16U
size_t crypto_aead_chacha20poly1305_abytes(void);

#define crypto_aead_chacha20poly1305_MESSAGEBYTES_MAX \
    (SODIUM_SIZE_MAX - crypto_aead_chacha20poly1305_ABYTES)
size_t crypto_aead_chacha20poly1305_messagebytes_max(void);

int crypto_aead_chacha20poly1305_encrypt(unsigned char *c,
                                         unsigned long long *clen_p,
                                         const unsigned char *m,
                                         unsigned long long mlen,
                                         const unsigned char *ad,
                                         unsigned long long adlen,
                                         const unsigned char *nsec,
                                         const unsigned char *npub,
                                         const unsigned char *k)
            __attribute__ ((nonnull(1, 8, 9)));

int crypto_aead_chacha20poly1305_decrypt(unsigned char *m,
                                         unsigned long long *mlen_p,
                                         unsigned char *nsec,
                                         const unsigned char *c,
                                         unsigned long long clen,
                                         const unsigned char *ad,
                                         unsigned long long adlen,
                                         const unsigned char *npub,
                                         const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(4, 8, 9)));

int crypto_aead_chacha20poly1305_encrypt_detached(unsigned char *c,
                                                  unsigned char *mac,
                                                  unsigned long long *maclen_p,
                                                  const unsigned char *m,
                                                  unsigned long long mlen,
                                                  const unsigned char *ad,
                                                  unsigned long long adlen,
                                                  const unsigned char *nsec,
                                                  const unsigned char *npub,
                                                  const unsigned char *k)
            __attribute__ ((nonnull(1, 2, 9, 10)));

int crypto_aead_chacha20poly1305_decrypt_detached(unsigned char *m,
                                                  unsigned char *nsec,
                                                  const unsigned char *c,
                                                  unsigned long long clen,
                                                  const unsigned char *mac,
                                                  const unsigned char *ad,
                                                  unsigned long long adlen,
                                                  const unsigned char *npub,
                                                  const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(3, 5, 8, 9)));

void crypto_aead_chacha20poly1305_keygen(unsigned char k[crypto_aead_chacha20poly1305_KEYBYTES])
            __attribute__ ((nonnull));

/* Aliases */

#define crypto_aead_chacha20poly1305_IETF_KEYBYTES         crypto_aead_chacha20poly1305_ietf_KEYBYTES
#define crypto_aead_chacha20poly1305_IETF_NSECBYTES        crypto_aead_chacha20poly1305_ietf_NSECBYTES
#define crypto_aead_chacha20poly1305_IETF_NPUBBYTES        crypto_aead_chacha20poly1305_ietf_NPUBBYTES
#define crypto_aead_chacha20poly1305_IETF_ABYTES           crypto_aead_chacha20poly1305_ietf_ABYTES
#define crypto_aead_chacha20poly1305_IETF_MESSAGEBYTES_MAX crypto_aead_chacha20poly1305_ietf_MESSAGEBYTES_MAX

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_crypto_aead_xchacha20poly1305.h ================= */
#ifndef crypto_aead_xchacha20poly1305_H
#define crypto_aead_xchacha20poly1305_H

#include <stddef.h>


#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

#define crypto_aead_xchacha20poly1305_ietf_KEYBYTES 32U
size_t crypto_aead_xchacha20poly1305_ietf_keybytes(void);

#define crypto_aead_xchacha20poly1305_ietf_NSECBYTES 0U
size_t crypto_aead_xchacha20poly1305_ietf_nsecbytes(void);

#define crypto_aead_xchacha20poly1305_ietf_NPUBBYTES 24U
size_t crypto_aead_xchacha20poly1305_ietf_npubbytes(void);

#define crypto_aead_xchacha20poly1305_ietf_ABYTES 16U
size_t crypto_aead_xchacha20poly1305_ietf_abytes(void);

#define crypto_aead_xchacha20poly1305_ietf_MESSAGEBYTES_MAX \
    (SODIUM_SIZE_MAX - crypto_aead_xchacha20poly1305_ietf_ABYTES)
size_t crypto_aead_xchacha20poly1305_ietf_messagebytes_max(void);

int crypto_aead_xchacha20poly1305_ietf_encrypt(unsigned char *c,
                                               unsigned long long *clen_p,
                                               const unsigned char *m,
                                               unsigned long long mlen,
                                               const unsigned char *ad,
                                               unsigned long long adlen,
                                               const unsigned char *nsec,
                                               const unsigned char *npub,
                                               const unsigned char *k)
            __attribute__ ((nonnull(1, 8, 9)));

int crypto_aead_xchacha20poly1305_ietf_decrypt(unsigned char *m,
                                               unsigned long long *mlen_p,
                                               unsigned char *nsec,
                                               const unsigned char *c,
                                               unsigned long long clen,
                                               const unsigned char *ad,
                                               unsigned long long adlen,
                                               const unsigned char *npub,
                                               const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(4, 8, 9)));

int crypto_aead_xchacha20poly1305_ietf_encrypt_detached(unsigned char *c,
                                                        unsigned char *mac,
                                                        unsigned long long *maclen_p,
                                                        const unsigned char *m,
                                                        unsigned long long mlen,
                                                        const unsigned char *ad,
                                                        unsigned long long adlen,
                                                        const unsigned char *nsec,
                                                        const unsigned char *npub,
                                                        const unsigned char *k)
            __attribute__ ((nonnull(1, 2, 9, 10)));

int crypto_aead_xchacha20poly1305_ietf_decrypt_detached(unsigned char *m,
                                                        unsigned char *nsec,
                                                        const unsigned char *c,
                                                        unsigned long long clen,
                                                        const unsigned char *mac,
                                                        const unsigned char *ad,
                                                        unsigned long long adlen,
                                                        const unsigned char *npub,
                                                        const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(3, 5, 9, 9)));

void crypto_aead_xchacha20poly1305_ietf_keygen(unsigned char k[crypto_aead_xchacha20poly1305_ietf_KEYBYTES])
            __attribute__ ((nonnull));

/* Aliases */

#define crypto_aead_xchacha20poly1305_IETF_KEYBYTES         crypto_aead_xchacha20poly1305_ietf_KEYBYTES
#define crypto_aead_xchacha20poly1305_IETF_NSECBYTES        crypto_aead_xchacha20poly1305_ietf_NSECBYTES
#define crypto_aead_xchacha20poly1305_IETF_NPUBBYTES        crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
#define crypto_aead_xchacha20poly1305_IETF_ABYTES           crypto_aead_xchacha20poly1305_ietf_ABYTES
#define crypto_aead_xchacha20poly1305_IETF_MESSAGEBYTES_MAX crypto_aead_xchacha20poly1305_ietf_MESSAGEBYTES_MAX

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_crypto_core_hchacha20.h ================= */
#ifndef crypto_core_hchacha20_H
#define crypto_core_hchacha20_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define crypto_core_hchacha20_OUTPUTBYTES 32U
size_t crypto_core_hchacha20_outputbytes(void);

#define crypto_core_hchacha20_INPUTBYTES 16U
size_t crypto_core_hchacha20_inputbytes(void);

#define crypto_core_hchacha20_KEYBYTES 32U
size_t crypto_core_hchacha20_keybytes(void);

#define crypto_core_hchacha20_CONSTBYTES 16U
size_t crypto_core_hchacha20_constbytes(void);

int crypto_core_hchacha20(unsigned char *out, const unsigned char *in,
                          const unsigned char *k, const unsigned char *c)
            __attribute__ ((nonnull(1, 2, 3)));

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_crypto_onetimeauth_poly1305.h ================= */
#ifndef crypto_onetimeauth_poly1305_H
#define crypto_onetimeauth_poly1305_H

#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>


typedef struct CRYPTO_ALIGN(16) crypto_onetimeauth_poly1305_state {
    unsigned char opaque[256];
} crypto_onetimeauth_poly1305_state;

size_t crypto_onetimeauth_poly1305_statebytes(void);

#define crypto_onetimeauth_poly1305_BYTES 16U
size_t crypto_onetimeauth_poly1305_bytes(void);

#define crypto_onetimeauth_poly1305_KEYBYTES 32U
size_t crypto_onetimeauth_poly1305_keybytes(void);

int crypto_onetimeauth_poly1305(unsigned char *out,
                                const unsigned char *in,
                                unsigned long long inlen,
                                const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_onetimeauth_poly1305_verify(const unsigned char *h,
                                       const unsigned char *in,
                                       unsigned long long inlen,
                                       const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

int crypto_onetimeauth_poly1305_init(crypto_onetimeauth_poly1305_state *state,
                                     const unsigned char *key)
            __attribute__ ((nonnull));

int crypto_onetimeauth_poly1305_update(crypto_onetimeauth_poly1305_state *state,
                                       const unsigned char *in,
                                       unsigned long long inlen)
            __attribute__ ((nonnull));

int crypto_onetimeauth_poly1305_final(crypto_onetimeauth_poly1305_state *state,
                                      unsigned char *out)
            __attribute__ ((nonnull));

void crypto_onetimeauth_poly1305_keygen(unsigned char k[crypto_onetimeauth_poly1305_KEYBYTES])
            __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_crypto_onetimeauth.h ================= */
#ifndef crypto_onetimeauth_H
#define crypto_onetimeauth_H

#include <stddef.h>


#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

typedef crypto_onetimeauth_poly1305_state crypto_onetimeauth_state;

size_t  crypto_onetimeauth_statebytes(void);

#define crypto_onetimeauth_BYTES crypto_onetimeauth_poly1305_BYTES
size_t  crypto_onetimeauth_bytes(void);

#define crypto_onetimeauth_KEYBYTES crypto_onetimeauth_poly1305_KEYBYTES
size_t  crypto_onetimeauth_keybytes(void);

#define crypto_onetimeauth_PRIMITIVE "poly1305"
const char *crypto_onetimeauth_primitive(void);

int crypto_onetimeauth(unsigned char *out, const unsigned char *in,
                       unsigned long long inlen, const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_onetimeauth_verify(const unsigned char *h, const unsigned char *in,
                              unsigned long long inlen, const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

int crypto_onetimeauth_init(crypto_onetimeauth_state *state,
                            const unsigned char *key) __attribute__ ((nonnull));

int crypto_onetimeauth_update(crypto_onetimeauth_state *state,
                              const unsigned char *in,
                              unsigned long long inlen)
            __attribute__ ((nonnull));

int crypto_onetimeauth_final(crypto_onetimeauth_state *state,
                             unsigned char *out) __attribute__ ((nonnull));

void crypto_onetimeauth_keygen(unsigned char k[crypto_onetimeauth_KEYBYTES])
            __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_crypto_verify_16.h ================= */
#ifndef crypto_verify_16_H
#define crypto_verify_16_H

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#define crypto_verify_16_BYTES 16U
size_t crypto_verify_16_bytes(void);

int crypto_verify_16(const unsigned char *x, const unsigned char *y)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_crypto_verify_32.h ================= */
#ifndef crypto_verify_32_H
#define crypto_verify_32_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define crypto_verify_32_BYTES 32U
size_t crypto_verify_32_bytes(void);

int crypto_verify_32(const unsigned char *x, const unsigned char *y)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_crypto_verify_64.h ================= */
#ifndef crypto_verify_64_H
#define crypto_verify_64_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define crypto_verify_64_BYTES 64U
size_t crypto_verify_64_bytes(void);

int crypto_verify_64(const unsigned char *x, const unsigned char *y)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_dolbeau_chacha20-avx2.h ================= */

#include <stdint.h>


extern struct crypto_stream_chacha20_implementation
    crypto_stream_chacha20_dolbeau_avx2_implementation;
/* ================= sodium_dolbeau_chacha20-ssse3.h ================= */

#include <stdint.h>


extern struct crypto_stream_chacha20_implementation
    crypto_stream_chacha20_dolbeau_ssse3_implementation;
/* ================= sodium_onetimeauth_poly1305.h ================= */

#ifndef onetimeauth_poly1305_H
#define onetimeauth_poly1305_H


typedef struct crypto_onetimeauth_poly1305_implementation {
    int (*onetimeauth)(unsigned char *out, const unsigned char *in,
                       unsigned long long inlen, const unsigned char *k);
    int (*onetimeauth_verify)(const unsigned char *h, const unsigned char *in,
                              unsigned long long inlen, const unsigned char *k);
    int (*onetimeauth_init)(crypto_onetimeauth_poly1305_state *state,
                            const unsigned char *              key);
    int (*onetimeauth_update)(crypto_onetimeauth_poly1305_state *state,
                              const unsigned char *              in,
                              unsigned long long                 inlen);
    int (*onetimeauth_final)(crypto_onetimeauth_poly1305_state *state,
                             unsigned char *                    out);
} crypto_onetimeauth_poly1305_implementation;

#endif
/* ================= sodium_poly1305-sse2.h ================= */
#ifndef poly1305_sse2_H
#define poly1305_sse2_H

#include <stddef.h>


extern struct crypto_onetimeauth_poly1305_implementation
    crypto_onetimeauth_poly1305_sse2_implementation;

#endif /* poly1305_sse2_H */
/* ================= sodium_poly1305_donna.h ================= */
#ifndef poly1305_donna_H
#define poly1305_donna_H

#include <stddef.h>


extern struct crypto_onetimeauth_poly1305_implementation
    crypto_onetimeauth_poly1305_donna_implementation;

#endif /* poly1305_donna_H */
/* ================= sodium_private_chacha20_ietf_ext.h ================= */
#ifndef chacha20_ietf_ext_H
#define chacha20_ietf_ext_H

#include <stdint.h>

/* The ietf_ext variant allows the internal counter to overflow into the IV */

int crypto_stream_chacha20_ietf_ext(unsigned char *c, unsigned long long clen,
                                    const unsigned char *n, const unsigned char *k);

int crypto_stream_chacha20_ietf_ext_xor_ic(unsigned char *c, const unsigned char *m,
                                           unsigned long long mlen,
                                           const unsigned char *n, uint32_t ic,
                                           const unsigned char *k);
#endif

/* ================= sodium_private_common.h ================= */
#ifndef common_H
#define common_H 1

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined( _MSC_VER )
#pragma warning(disable:4127)
#pragma warning(disable:4244)
#pragma warning(disable:4668)
#pragma warning(disable:4456)
#endif

#define COMPILER_ASSERT(X) (void) sizeof(char[(X) ? 1 : -1])

/* 128-bit integer support: enables the Poly1305 donna64 and SSE2 code paths.
   Available on 64-bit GCC/Clang targets (x86-64, arm64, ...); not on MSVC. */
#if !defined(HAVE_TI_MODE) && defined(__SIZEOF_INT128__)
# define HAVE_TI_MODE 1
#endif

#ifdef HAVE_TI_MODE
# if defined(__SIZEOF_INT128__)
typedef unsigned __int128 uint128_t;
# else
typedef unsigned uint128_t __attribute__((mode(TI)));
# endif
#endif

#define ROTL32(X, B) rotl32((X), (B))
static inline uint32_t
rotl32(const uint32_t x, const int b)
{
    return (x << b) | (x >> (32 - b));
}

#define ROTL64(X, B) rotl64((X), (B))
static inline uint64_t
rotl64(const uint64_t x, const int b)
{
    return (x << b) | (x >> (64 - b));
}

#define ROTR32(X, B) rotr32((X), (B))
static inline uint32_t
rotr32(const uint32_t x, const int b)
{
    return (x >> b) | (x << (32 - b));
}

#define ROTR64(X, B) rotr64((X), (B))
static inline uint64_t
rotr64(const uint64_t x, const int b)
{
    return (x >> b) | (x << (64 - b));
}

#define LOAD64_LE(SRC) load64_le(SRC)
static inline uint64_t
load64_le(const uint8_t src[8])
{
#ifdef NATIVE_LITTLE_ENDIAN
    uint64_t w;
    memcpy(&w, src, sizeof w);
    return w;
#else
    uint64_t w = (uint64_t) src[0];
    w |= (uint64_t) src[1] <<  8;
    w |= (uint64_t) src[2] << 16;
    w |= (uint64_t) src[3] << 24;
    w |= (uint64_t) src[4] << 32;
    w |= (uint64_t) src[5] << 40;
    w |= (uint64_t) src[6] << 48;
    w |= (uint64_t) src[7] << 56;
    return w;
#endif
}

#define STORE64_LE(DST, W) store64_le((DST), (W))
static inline void
store64_le(uint8_t dst[8], uint64_t w)
{
#ifdef NATIVE_LITTLE_ENDIAN
    memcpy(dst, &w, sizeof w);
#else
    dst[0] = (uint8_t) w; w >>= 8;
    dst[1] = (uint8_t) w; w >>= 8;
    dst[2] = (uint8_t) w; w >>= 8;
    dst[3] = (uint8_t) w; w >>= 8;
    dst[4] = (uint8_t) w; w >>= 8;
    dst[5] = (uint8_t) w; w >>= 8;
    dst[6] = (uint8_t) w; w >>= 8;
    dst[7] = (uint8_t) w;
#endif
}

#define LOAD32_LE(SRC) load32_le(SRC)
static inline uint32_t
load32_le(const uint8_t src[4])
{
#ifdef NATIVE_LITTLE_ENDIAN
    uint32_t w;
    memcpy(&w, src, sizeof w);
    return w;
#else
    uint32_t w = (uint32_t) src[0];
    w |= (uint32_t) src[1] <<  8;
    w |= (uint32_t) src[2] << 16;
    w |= (uint32_t) src[3] << 24;
    return w;
#endif
}

#define STORE32_LE(DST, W) store32_le((DST), (W))
static inline void
store32_le(uint8_t dst[4], uint32_t w)
{
#ifdef NATIVE_LITTLE_ENDIAN
    memcpy(dst, &w, sizeof w);
#else
    dst[0] = (uint8_t) w; w >>= 8;
    dst[1] = (uint8_t) w; w >>= 8;
    dst[2] = (uint8_t) w; w >>= 8;
    dst[3] = (uint8_t) w;
#endif
}

/* ----- */

#define LOAD64_BE(SRC) load64_be(SRC)
static inline uint64_t
load64_be(const uint8_t src[8])
{
#ifdef NATIVE_BIG_ENDIAN
    uint64_t w;
    memcpy(&w, src, sizeof w);
    return w;
#else
    uint64_t w = (uint64_t) src[7];
    w |= (uint64_t) src[6] <<  8;
    w |= (uint64_t) src[5] << 16;
    w |= (uint64_t) src[4] << 24;
    w |= (uint64_t) src[3] << 32;
    w |= (uint64_t) src[2] << 40;
    w |= (uint64_t) src[1] << 48;
    w |= (uint64_t) src[0] << 56;
    return w;
#endif
}

#define STORE64_BE(DST, W) store64_be((DST), (W))
static inline void
store64_be(uint8_t dst[8], uint64_t w)
{
#ifdef NATIVE_BIG_ENDIAN
    memcpy(dst, &w, sizeof w);
#else
    dst[7] = (uint8_t) w; w >>= 8;
    dst[6] = (uint8_t) w; w >>= 8;
    dst[5] = (uint8_t) w; w >>= 8;
    dst[4] = (uint8_t) w; w >>= 8;
    dst[3] = (uint8_t) w; w >>= 8;
    dst[2] = (uint8_t) w; w >>= 8;
    dst[1] = (uint8_t) w; w >>= 8;
    dst[0] = (uint8_t) w;
#endif
}

#define LOAD32_BE(SRC) load32_be(SRC)
static inline uint32_t
load32_be(const uint8_t src[4])
{
#ifdef NATIVE_BIG_ENDIAN
    uint32_t w;
    memcpy(&w, src, sizeof w);
    return w;
#else
    uint32_t w = (uint32_t) src[3];
    w |= (uint32_t) src[2] <<  8;
    w |= (uint32_t) src[1] << 16;
    w |= (uint32_t) src[0] << 24;
    return w;
#endif
}

#define STORE32_BE(DST, W) store32_be((DST), (W))
static inline void
store32_be(uint8_t dst[4], uint32_t w)
{
#ifdef NATIVE_BIG_ENDIAN
    memcpy(dst, &w, sizeof w);
#else
    dst[3] = (uint8_t) w; w >>= 8;
    dst[2] = (uint8_t) w; w >>= 8;
    dst[1] = (uint8_t) w; w >>= 8;
    dst[0] = (uint8_t) w;
#endif
}

#define XOR_BUF(OUT, IN, N) xor_buf((OUT), (IN), (N))
static inline void
xor_buf(unsigned char *out, const unsigned char *in, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        out[i] ^= in[i];
    }
}


#if !defined(__clang__) && !defined(__GNUC__)
# ifdef __attribute__
#  undef __attribute__
# endif
# define __attribute__(a)
#endif

#ifndef CRYPTO_ALIGN
# if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#  define CRYPTO_ALIGN(x) __declspec(align(x))
# else
#  define CRYPTO_ALIGN(x) __attribute__ ((aligned(x)))
# endif
#endif

/*
 * SIMD / intrinsics feature selection.
 *
 * The optimized implementations kept for the primitives yojimbo uses
 * (ChaCha20 SSSE3/AVX2, Poly1305 SSE2) enable their own instruction set
 * per-function with #pragma target on GCC/Clang, and the fastest variant the
 * running CPU actually supports is chosen at runtime (see sodium_runtime.c and
 * the *_pick_best_implementation() functions). So on any x86 target we enable
 * the intrinsic headers unconditionally and let runtime dispatch decide; targets
 * with no specialized implementation (e.g. arm64) fall back to the portable
 * reference code, which is correct on every platform. A build may still predefine
 * NETCODE_X64 / NETCODE_AVX / NETCODE_AVX2 to force the x86 set on.
 *
 * Note: the amd64/AVX *assembly* paths (HAVE_AMD64_ASM / HAVE_AVX_ASM) are left
 * off on purpose. They only accelerated primitives that have been pruned from
 * this copy; the kept SSE2/SSSE3/AVX2 code is pure intrinsics.
 */

#if defined(__clang__) || defined(__GNUC__)
# if defined(__x86_64__) || defined(__amd64__) || defined(__i386__) || \
     NETCODE_X64 || NETCODE_AVX || NETCODE_AVX2
#  define HAVE_MMINTRIN_H   1
#  define HAVE_EMMINTRIN_H  1   /* SSE2   */
#  define HAVE_PMMINTRIN_H  1   /* SSE3   */
#  define HAVE_TMMINTRIN_H  1   /* SSSE3  */
#  define HAVE_SMMINTRIN_H  1   /* SSE4.1 */
#  define HAVE_AVXINTRIN_H  1   /* AVX    */
#  define HAVE_WMMINTRIN_H  1
#  define HAVE_AVX2INTRIN_H 1   /* AVX2   */
#  define HAVE_CPUID        1
# endif
#endif

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86))

# include <intrin.h>

# define HAVE_INTRIN_H    1
# define HAVE_MMINTRIN_H  1
# define HAVE_EMMINTRIN_H 1
# define HAVE_PMMINTRIN_H 1
# define HAVE_TMMINTRIN_H 1
# define HAVE_SMMINTRIN_H 1

# if _MSC_VER >= 1600
#  define HAVE_WMMINTRIN_H 1
# endif

/* MSVC has no per-function target pragma, so AVX/AVX2 codegen requires the
   matching /arch: switch (which predefines __AVX__ / __AVX2__). Honor that, and
   still allow an explicit NETCODE_AVX/NETCODE_AVX2 override. */
# if defined(__AVX__) || NETCODE_AVX || NETCODE_AVX2
#  define HAVE_AVXINTRIN_H 1
# endif
# if (defined(__AVX2__) || NETCODE_AVX2) && _MSC_VER >= 1700 && defined(_M_X64)
#  define HAVE_AVX2INTRIN_H 1
# endif

#elif defined(HAVE_INTRIN_H)

# include <intrin.h>

#endif

#ifdef HAVE_LIBCTGRIND
extern void ct_poison  (const void *, size_t);
extern void ct_unpoison(const void *, size_t);
# define POISON(X, L)   ct_poison((X), (L))
# define UNPOISON(X, L) ct_unpoison((X), (L))
#else
# define POISON(X, L)   (void) 0
# define UNPOISON(X, L) (void) 0
#endif

#endif
/* ================= sodium_private_implementations.h ================= */
#ifndef implementations_H
#define implementations_H

int _crypto_generichash_blake2b_pick_best_implementation(void);
int _crypto_onetimeauth_poly1305_pick_best_implementation(void);
int _crypto_scalarmult_curve25519_pick_best_implementation(void);
int _crypto_stream_chacha20_pick_best_implementation(void);
int _crypto_stream_salsa20_pick_best_implementation(void);

#endif
/* ================= sodium_private_mutex.h ================= */
#ifndef mutex_H
#define mutex_H 1

extern int sodium_crit_enter(void);
extern int sodium_crit_leave(void);

#endif
/* ================= sodium_private_sse2_64_32.h ================= */
#ifndef sse2_64_32_H
#define sse2_64_32_H 1


#ifdef HAVE_INTRIN_H
# include <intrin.h>
#endif

#if defined(HAVE_EMMINTRIN_H) && \
    !(defined(__amd64) || defined(__amd64__) || defined(__x86_64__) || \
      defined(_M_X64) || defined(_M_AMD64))

# include <emmintrin.h>
# include <stdint.h>

# ifndef _mm_set_epi64x
#  define _mm_set_epi64x(Q0, Q1) sodium__mm_set_epi64x((Q0), (Q1))
static inline __m128i
sodium__mm_set_epi64x(int64_t q1, int64_t q0)
{
    union { int64_t as64; int32_t as32[2]; } x0, x1;
    x0.as64 = q0; x1.as64 = q1;
    return _mm_set_epi32(x1.as32[1], x1.as32[0], x0.as32[1], x0.as32[0]);
}
# endif

# ifndef _mm_set1_epi64x
#  define _mm_set1_epi64x(Q) sodium__mm_set1_epi64x(Q)
static inline __m128i
sodium__mm_set1_epi64x(int64_t q)
{
    return _mm_set_epi64x(q, q);
}
# endif

# ifndef _mm_cvtsi64_si128
#  define _mm_cvtsi64_si128(Q) sodium__mm_cvtsi64_si128(Q)
static inline __m128i
sodium__mm_cvtsi64_si128(int64_t q)
{
    union { int64_t as64; int32_t as32[2]; } x;
    x.as64 = q;
    return _mm_setr_epi32(x.as32[0], x.as32[1], 0, 0);
}
# endif

#endif

#endif
/* ================= sodium_randombytes.h ================= */

#ifndef randombytes_H
#define randombytes_H

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>


#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

typedef struct randombytes_implementation {
    const char *(*implementation_name)(void); /* required */
    uint32_t    (*random)(void);              /* required */
    void        (*stir)(void);                /* optional */
    uint32_t    (*uniform)(const uint32_t upper_bound); /* optional, a default implementation will be used if NULL */
    void        (*buf)(void * const buf, const size_t size); /* required */
    int         (*close)(void);               /* optional */
} randombytes_implementation;

#define randombytes_BYTES_MAX SODIUM_MIN(SODIUM_SIZE_MAX, 0xffffffffUL)

#define randombytes_SEEDBYTES 32U
size_t randombytes_seedbytes(void);

void randombytes_buf(void * const buf, const size_t size)
            __attribute__ ((nonnull));

void randombytes_buf_deterministic(void * const buf, const size_t size,
                                   const unsigned char seed[randombytes_SEEDBYTES])
            __attribute__ ((nonnull));

uint32_t randombytes_random(void);

uint32_t randombytes_uniform(const uint32_t upper_bound);

void randombytes_stir(void);

int randombytes_close(void);

int randombytes_set_implementation(randombytes_implementation *impl)
            __attribute__ ((nonnull));

const char *randombytes_implementation_name(void);

/* -- NaCl compatibility interface -- */

void randombytes(unsigned char * const buf, const unsigned long long buf_len)
            __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_randombytes_sysrandom.h ================= */

#ifndef randombytes_sysrandom_H
#define randombytes_sysrandom_H


#ifdef __cplusplus
extern "C" {
#endif

extern struct randombytes_implementation randombytes_sysrandom_implementation;

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_runtime.h ================= */

#ifndef sodium_runtime_H
#define sodium_runtime_H


#ifdef __cplusplus
extern "C" {
#endif

int sodium_runtime_has_neon(void);

int sodium_runtime_has_sse2(void);

int sodium_runtime_has_sse3(void);

int sodium_runtime_has_ssse3(void);

int sodium_runtime_has_sse41(void);

int sodium_runtime_has_avx(void);

int sodium_runtime_has_avx2(void);

int sodium_runtime_has_avx512f(void);

int sodium_runtime_has_pclmul(void);

int sodium_runtime_has_aesni(void);

int sodium_runtime_has_rdrand(void);

/* ------------------------------------------------------------------------- */

int _sodium_runtime_get_cpu_features(void);

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_utils.h ================= */

#ifndef sodium_utils_H
#define sodium_utils_H

#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef SODIUM_C99
# if defined(__cplusplus) || !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#  define SODIUM_C99(X)
# else
#  define SODIUM_C99(X) X
# endif
#endif

void sodium_memzero(void * const pnt, const size_t len) __attribute__ ((nonnull));

void sodium_stackzero(const size_t len);

/*
 * WARNING: sodium_memcmp() must be used to verify if two secret keys
 * are equal, in constant time.
 * It returns 0 if the keys are equal, and -1 if they differ.
 * This function is not designed for lexicographical comparisons.
 */
int sodium_memcmp(const void * const b1_, const void * const b2_, size_t len)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

/*
 * sodium_compare() returns -1 if b1_ < b2_, 1 if b1_ > b2_ and 0 if b1_ == b2_
 * It is suitable for lexicographical comparisons, or to compare nonces
 * and counters stored in little-endian format.
 * However, it is slower than sodium_memcmp().
 */
int sodium_compare(const unsigned char *b1_, const unsigned char *b2_,
                   size_t len)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

int sodium_is_zero(const unsigned char *n, const size_t nlen);

void sodium_increment(unsigned char *n, const size_t nlen);

void sodium_add(unsigned char *a, const unsigned char *b, const size_t len)
            __attribute__ ((nonnull));

void sodium_sub(unsigned char *a, const unsigned char *b, const size_t len)
            __attribute__ ((nonnull));

char *sodium_bin2hex(char * const hex, const size_t hex_maxlen,
                     const unsigned char * const bin, const size_t bin_len)
            __attribute__ ((nonnull));

int sodium_hex2bin(unsigned char * const bin, const size_t bin_maxlen,
                   const char * const hex, const size_t hex_len,
                   const char * const ignore, size_t * const bin_len,
                   const char ** const hex_end)
            __attribute__ ((nonnull(1, 3)));

#define sodium_base64_VARIANT_ORIGINAL            1
#define sodium_base64_VARIANT_ORIGINAL_NO_PADDING 3
#define sodium_base64_VARIANT_URLSAFE             5
#define sodium_base64_VARIANT_URLSAFE_NO_PADDING  7

/*
 * Computes the required length to encode BIN_LEN bytes as a base64 string
 * using the given variant. The computed length includes a trailing \0.
 */
#define sodium_base64_ENCODED_LEN(BIN_LEN, VARIANT) \
    (((BIN_LEN) / 3U) * 4U + \
    ((((BIN_LEN) - ((BIN_LEN) / 3U) * 3U) | (((BIN_LEN) - ((BIN_LEN) / 3U) * 3U) >> 1)) & 1U) * \
     (4U - ((0U - (((VARIANT) & 2U) >> 1)) & (3U - ((BIN_LEN) - ((BIN_LEN) / 3U) * 3U)))) + 1U)

size_t sodium_base64_encoded_len(const size_t bin_len, const int variant);

char *sodium_bin2base64(char * const b64, const size_t b64_maxlen,
                        const unsigned char * const bin, const size_t bin_len,
                        const int variant) __attribute__ ((nonnull));

int sodium_base642bin(unsigned char * const bin, const size_t bin_maxlen,
                      const char * const b64, const size_t b64_len,
                      const char * const ignore, size_t * const bin_len,
                      const char ** const b64_end, const int variant)
            __attribute__ ((nonnull(1, 3)));

int sodium_mlock(void * const addr, const size_t len)
            __attribute__ ((nonnull));

int sodium_munlock(void * const addr, const size_t len)
            __attribute__ ((nonnull));

/* WARNING: sodium_malloc() and sodium_allocarray() are not general-purpose
 * allocation functions.
 *
 * They return a pointer to a region filled with 0xd0 bytes, immediately
 * followed by a guard page.
 * As a result, accessing a single byte after the requested allocation size
 * will intentionally trigger a segmentation fault.
 *
 * A canary and an additional guard page placed before the beginning of the
 * region may also kill the process if a buffer underflow is detected.
 *
 * The memory layout is:
 * [unprotected region size (read only)][guard page (no access)][unprotected pages (read/write)][guard page (no access)]
 * With the layout of the unprotected pages being:
 * [optional padding][16-bytes canary][user region]
 *
 * However:
 * - These functions are significantly slower than standard functions
 * - Each allocation requires 3 or 4 additional pages
 * - The returned address will not be aligned if the allocation size is not
 *   a multiple of the required alignment. For this reason, these functions
 *   are designed to store data, such as secret keys and messages.
 *
 * sodium_malloc() can be used to allocate any libsodium data structure.
 *
 * The crypto_generichash_state structure is packed and its length is
 * either 357 or 361 bytes. For this reason, when using sodium_malloc() to
 * allocate a crypto_generichash_state structure, padding must be added in
 * order to ensure proper alignment. crypto_generichash_statebytes()
 * returns the rounded up structure size, and should be prefered to sizeof():
 * state = sodium_malloc(crypto_generichash_statebytes());
 */

void *sodium_malloc(const size_t size)
            __attribute__ ((malloc));

void *sodium_allocarray(size_t count, size_t size)
            __attribute__ ((malloc));

void sodium_free(void *ptr);

int sodium_mprotect_noaccess(void *ptr) __attribute__ ((nonnull));

int sodium_mprotect_readonly(void *ptr) __attribute__ ((nonnull));

int sodium_mprotect_readwrite(void *ptr) __attribute__ ((nonnull));

int sodium_pad(size_t *padded_buflen_p, unsigned char *buf,
               size_t unpadded_buflen, size_t blocksize, size_t max_buflen)
            __attribute__ ((nonnull(2)));

int sodium_unpad(size_t *unpadded_buflen_p, const unsigned char *buf,
                 size_t padded_buflen, size_t blocksize)
            __attribute__ ((nonnull(2)));

/* -------- */

int _sodium_alloc_init(void);

#ifdef __cplusplus
}
#endif

#endif
/* ================= sodium_version.h ================= */
#ifndef sodium_version_H
#define sodium_version_H


#define SODIUM_VERSION_STRING "1.0.20"

#define SODIUM_LIBRARY_VERSION_MAJOR 26
#define SODIUM_LIBRARY_VERSION_MINOR 2

#ifdef __cplusplus
extern "C" {
#endif

const char *sodium_version_string(void);

int         sodium_library_version_major(void);

int         sodium_library_version_minor(void);

int         sodium_library_minimal(void);

#ifdef __cplusplus
}
#endif

#endif

#endif /* SODIUM_AMALGAM_H */
