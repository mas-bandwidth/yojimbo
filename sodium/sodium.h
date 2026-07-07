
#ifndef sodium_H
#define sodium_H

/*
    Minimal libsodium subset for yojimbo / netcode.

    This vendored copy is compiled only on Windows (see premake5.lua); other
    platforms link the system libsodium. It has been pruned to just the
    primitives netcode uses: ChaCha20-Poly1305 (IETF and XChaCha20 variants),
    randombytes, and the supporting core/runtime/utils. The crypto matches
    upstream libsodium 1.0.20 test vectors. See NOTES.md in this directory.
*/

#include "sodium_version.h"
#include "sodium_core.h"
#include "sodium_crypto_aead_chacha20poly1305.h"
#include "sodium_crypto_aead_xchacha20poly1305.h"
#include "sodium_crypto_core_hchacha20.h"
#include "sodium_crypto_onetimeauth.h"
#include "sodium_crypto_onetimeauth_poly1305.h"
#include "sodium_crypto_stream_chacha20.h"
#include "sodium_crypto_verify_16.h"
#include "sodium_crypto_verify_32.h"
#include "sodium_crypto_verify_64.h"
#include "sodium_randombytes.h"
#include "sodium_randombytes_sysrandom.h"
#include "sodium_runtime.h"
#include "sodium_utils.h"

#endif
