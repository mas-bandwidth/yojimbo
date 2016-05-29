/*
    Yojimbo Client/Server Network Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    All rights reserved.
*/

#include "yojimbo_crypto.h"
#include <sodium.h>
#include <assert.h>

bool InitializeYojimbo()
{
    assert( yojimbo::NonceBytes == crypto_aead_chacha20poly1305_NPUBBYTES );
    assert( yojimbo::KeyBytes == crypto_aead_chacha20poly1305_KEYBYTES );
    assert( yojimbo::KeyBytes == crypto_secretbox_KEYBYTES );
    assert( yojimbo::AuthBytes == crypto_aead_chacha20poly1305_ABYTES );
    assert( yojimbo::MacBytes == crypto_secretbox_MACBYTES );

    return sodium_init() != -1;
}

void ShutdownYojimbo()
{
    // ...
}
