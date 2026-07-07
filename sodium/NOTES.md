# Vendored libsodium (minimal subset)

This directory is a small, self-contained subset of [libsodium](https://github.com/jedisct1/libsodium),
flattened into one directory (every file prefixed `sodium_`) so it builds without
autoconf/CMake.

## When is this used?

**Windows only.** `premake5.lua` links this `sodium-builtin` project only under
`filter "system:windows"`. On macOS and Linux, yojimbo links the system libsodium
(e.g. from Homebrew or the distro package). So changes here affect Windows builds
only; on other platforms `libsodium-builtin.a` contains just `dummy.c`.

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

## Local modifications vs upstream

- Includes are flattened and prefixed `sodium_` (mechanical rename).
- Optional `NETCODE_CRYPTO_LOGS` debug prints in the implementation selectors.
- SIMD feature macros (`HAVE_*INTRIN_H`) are derived from `NETCODE_X64/AVX/AVX2`
  in `sodium_private_common.h` instead of from autoconf.

The constant-time comparison qualifiers in `sodium_utils.c` (`sodium_memcmp`,
`sodium_compare`) and `sodium_verify.c` (`crypto_verify_n`) are kept identical to
upstream (`const volatile unsigned char *volatile`); an earlier local copy had
dropped the pointer-`volatile`, which this tree restores.

## Validation

The crypto here is checked three ways:

1. **libsodium's own known-answer tests** (`test/default/aead_chacha20poly1305` and
   `aead_xchacha20poly1305`) pass against this subset, for both the reference and
   the x86_64 SSE2/SSSE3/AVX2 builds.
2. Output is **bit-identical** to a stock libsodium 1.0.20 for both AEADs.
3. `test.cpp` includes `test_crypto_aead_vectors()`, a known-answer test that runs
   in CI on every platform — so the Windows build exercises this vendored code.

If you change anything in this directory, re-run `bin/test` and make sure
`test_crypto_aead_vectors` passes.
