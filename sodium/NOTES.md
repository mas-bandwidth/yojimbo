# Vendored libsodium (minimal subset)

`sodium.h` and `sodium.c` are a small, amalgamated subset of
[libsodium](https://github.com/jedisct1/libsodium) — the ChaCha20 / Poly1305 / AEAD slice
that netcode uses, and nothing else.

    Tracks: libsodium 1.0.22
    Source: mas-bandwidth/netcode, arriving here with the netcode re-vendor
    Currently vendored netcode: 1.4.2  (see netcode/netcode.h, NETCODE_VERSION_FULL)

## This is not an independent copy

These two files are **byte-identical** to `sodium/` in
[mas-bandwidth/netcode](https://github.com/mas-bandwidth/netcode), and they are meant to
stay that way. yojimbo's crypto *is* netcode's crypto: the connect-token and packet
encryption live in netcode, and yojimbo carries the same slice so it can build standalone.

They update when netcode is re-vendored, not on their own schedule. Do not patch them
here — patch upstream in netcode, re-vendor netcode, and this follows.

**`.github/workflows/sodium-parity.yml` enforces that.** It fetches the vendored netcode
version from `netcode/netcode.h`, pulls that tag's `sodium/` from mas-bandwidth/netcode,
and fails if the bytes differ. Without it, "byte-identical to netcode's" is a sentence in
a file that nothing checks — it would keep reading as true for exactly as long as it took
someone to edit one of the two copies.

## The upstream review record lives in netcode, on purpose

`mas-bandwidth/netcode`'s `sodium/NOTES.md` holds the full account: how the amalgamation
is generated, what is included and what was pruned, the optimized implementations kept,
the validation, and the **review log** — every upstream release read, what was inside the
included slice, and what was deliberately not applied.

That is deliberately **not** duplicated here. Two copies of a review log is two copies of
one truth, and the copy nobody updates is the one people read. One record, in the repo
that owns the vendoring.

## Console platforms need their own RNG

On `__ORBIS__` / `__PROSPERO__` the `randombytes_sysrandom` backend in `sodium.c` is a set
of stubs — upstream libsodium ships no system RNG for those platforms, so
`randombytes_sysrandom_buf` returns without writing a byte of the buffer it was handed.
Nothing warns at build time. netcode draws every key and nonce from `randombytes_buf`, and
yojimbo's connect tokens and session keys are netcode's, so on a console build with the
default backend they are whatever was in that memory, which is not a key.

**A console port must register a real RNG before `netcode_init` runs** — before
`InitializeYojimbo`, which calls it — using the platform's own cryptographic random source:

    randombytes_set_implementation( &my_console_randombytes_implementation );

This is a porting requirement, not a defect in the vendored slice: it matches upstream
libsodium's behaviour on those platforms, and the platforms yojimbo builds and tests on
(Linux, macOS, Windows) all use a real system RNG. netcode's `sodium/NOTES.md` and
`IMPLEMENTERS.md` state it for netcode and its ports, from netcode 1.4.7; this is the same
requirement seen from yojimbo.

## Why yojimbo has it at all

So yojimbo builds with no external libsodium dependency on any platform. To link the
system-installed libsodium instead:

    cmake -B build -DYOJIMBO_SYSTEM_SODIUM=ON

That skips the bundled subset entirely. See `BUILDING.md`.

## Licence

libsodium is ISC-licensed; the licence text travels in the amalgamation header.
