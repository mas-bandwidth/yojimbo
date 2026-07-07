# Vendored libsodium (minimal subset)

This directory is a small, self-contained subset of [libsodium](https://github.com/jedisct1/libsodium),
flattened into one directory (every file prefixed `sodium_`) so it builds without
autoconf/CMake.

## When is this used?

Today `premake5.lua` links this `sodium-builtin` project only under
`filter "system:windows"`; on macOS and Linux yojimbo links the system libsodium
(e.g. from Homebrew or the distro package). However, this subset is written to be
usable on **any** target (Linux, macOS x86-64, macOS Apple Silicon, Windows) —
see "Optimized implementations" below — so it can be linked on other platforms if
desired.

## Baseline

The crypto sources track **libsodium 1.0.20**. netcode/yojimbo only use a tiny
slice of libsodium, and upstream has not changed those sources since 1.0.17 — the
ChaCha20, Poly1305, and AEAD implementations are byte-identical across 1.0.17 →
1.0.20. This subset has been verified to match libsodium 1.0.20 test vectors (see
"Validation" below).

## What is included

Only the primitives netcode calls, plus their support code:

- `crypto_aead_chacha20poly1305_ietf_*`   (ChaCha20-Poly1305, IETF nonce)
- `crypto_aead_xchacha20poly1305_ietf_*`  (XChaCha20-Poly1305)
- `randombytes_buf` / `sodium_init`
- ChaCha20 (ref + SSSE3/AVX2), Poly1305 (donna + SSE2), HChaCha20, verify,
  randombytes (sysrandom), core/runtime/utils.

Everything else from upstream (ed25519, curve25519, box, sign, secretbox, kx,
generichash/blake2, sha512, siphash, the salsa20 family, sandy2x asm, …) has been
removed — it was dead code for yojimbo. `sodium_init()` in `sodium_core.c` was
trimmed to initialise only the kept primitives.

## Optimized implementations

For the kept primitives, **all** of libsodium's optimized implementations are
retained and selected at runtime by CPU feature detection:

- **ChaCha20:** reference, SSSE3, AVX2.
- **Poly1305:** donna (32/64-bit) and SSE2.

(libsodium has no ARM/NEON implementation of ChaCha20 or Poly1305, so on arm64 the
portable donna64 / reference code is the fastest available and is what upstream
uses there too.)

The SSSE3/AVX2/SSE2 code enables its own instruction set per-function via
`#pragma ... target(...)` on GCC/Clang, so no global `-mavx2`/`-mssse3` build flags
are required; the fastest variant the *running* CPU supports is chosen at runtime.
`sodium_private_common.h` enables the intrinsic headers automatically from the
compiler's own target macros (`__x86_64__`, `_MSC_VER`+`__AVX2__`, `__SIZEOF_INT128__`,
…), so the optimized paths light up on any x86 build without extra configuration.
On MSVC, AVX2 additionally requires building with `/arch:AVX2` (which predefines
`__AVX2__`); SSE2/SSSE3 are always on for MSVC x64. `NETCODE_X64`/`NETCODE_AVX`/
`NETCODE_AVX2` may still be predefined to force the x86 set on.

## Local modifications vs upstream

- Includes are flattened and prefixed `sodium_` (mechanical rename).
- Optional `NETCODE_CRYPTO_LOGS` debug prints in the implementation selectors.
- SIMD feature macros (`HAVE_*INTRIN_H`, `HAVE_TI_MODE`) are derived from the
  compiler's own target macros in `sodium_private_common.h` instead of from
  autoconf. The SIMD sources use the same `#pragma clang attribute` / `#pragma GCC
  target` idiom as upstream 1.0.20 so they compile without global `-m` flags.

The constant-time comparison qualifiers in `sodium_utils.c` (`sodium_memcmp`,
`sodium_compare`) and `sodium_verify.c` (`crypto_verify_n`) are kept identical to
upstream (`const volatile unsigned char *volatile`); an earlier local copy had
dropped the pointer-`volatile`, which this tree restores.

## Validation

The crypto here is checked several ways:

1. **libsodium's own known-answer tests** (`test/default/aead_chacha20poly1305` and
   `aead_xchacha20poly1305`) pass against this subset on both arm64 (reference /
   donna64) and x86-64 (SSSE3 ChaCha20 + SSE2 Poly1305 selected at runtime; AVX2
   compiled).
2. Output is **bit-identical** to a stock libsodium 1.0.20 for both AEADs, on both
   architectures.
3. The full yojimbo test suite passes linked against this subset (exercises
   netcode's real connect-token / packet encryption paths).
4. `test.cpp` includes `test_crypto_aead_vectors()`, a known-answer test that runs
   in CI on every platform — so whichever implementation the target CPU selects is
   exercised.

If you change anything in this directory, re-run `bin/test` and make sure
`test_crypto_aead_vectors` passes.
