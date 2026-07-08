# Fuzz targets

libFuzzer harnesses over yojimbo's untrusted-input parsers — the code that runs on raw,
attacker-controlled bytes off the wire. Standalone from the premake build (their own
sources), so they don't affect the normal build; a CI job builds and runs them under real
libFuzzer on Linux (see `.github/workflows/ci.yml`, job `fuzz`).

Each target is **dual-mode**:
- **Real libFuzzer** (Linux clang, coverage-guided) — the CI mode. Built with
  `-fsanitize=fuzzer,address,undefined`; libFuzzer supplies `main`.
- **Standalone** (`-DFUZZ_STANDALONE`) — an ordinary executable that replays file args or
  feeds pseudo-random buffers (`FUZZ_ITERS` env var, default 200k). Needed because Apple
  clang ships no libFuzzer. Build it with ASan+UBSan to shake out crashes locally.

## Targets
- `fuzz_reliable.c` — `reliable_endpoint_receive_packet` (header parse, ack decode,
  fragment reassembly).
- `fuzz_netcode.c` — `netcode_read_packet` (prefix/type parse, length checks, sequence
  decode, AEAD + connect-token decryption). Includes `netcode.c` directly because the
  reader and its replay-protection / packet-type symbols are internal to that TU.
- `fuzz_netcode_connect_token.c` — `netcode_read_connect_token_private`, the parser for the
  *decrypted* private connect token (client id, timeout, IPv4/IPv6 server-address list, keys,
  user data). A separate target because the AEAD boundary makes this parser unreachable by
  mutation through `fuzz_netcode`: any change to the encrypted token fails the MAC, so the
  address-list loop is only reached by fuzzing the decrypted layout directly.
- `fuzz_connection.cpp` — `yojimbo::Connection::ProcessPacket` (bitpacker ReadStream,
  ConnectionPacket / ChannelPacketData serialization, per-channel message + block-fragment
  reading) over a reliable-ordered and an unreliable-unordered channel.

All three run clean (≥300k standalone inputs under ASan+UBSan). Bugs found and fixed while
bringing `fuzz_connection` up: a message leak on the "block fragment attached to non-block
message" error path; a `bufferSize > 0` assert on a zero-payload packet; and a
`ChannelPacketData` union misread (a block fragment addressed to an unreliable channel was
read as a message-pointer array → wild pointer). Regression tests for the latter two are in
`test.cpp` (`test_connection_reject_empty_packet`,
`test_connection_unreliable_rejects_block_fragment`).

## Build — standalone, sanitized (macOS/Apple clang or Linux)

reliable:
```
clang -DFUZZ_STANDALONE -DRELIABLE_DEBUG -Ireliable -Ifuzz \
  -fsanitize=address,undefined -fno-sanitize-recover=all -g \
  fuzz/fuzz_reliable.c reliable/reliable.c -o /tmp/fz_reliable
FUZZ_ITERS=300000 UBSAN_OPTIONS=halt_on_error=1 /tmp/fz_reliable
```

netcode:
```
clang -DFUZZ_STANDALONE -DNETCODE_DEBUG -Inetcode -Isodium -Ifuzz \
  -fsanitize=address,undefined -fno-sanitize=nonnull-attribute -fno-sanitize-recover=all -g \
  fuzz/fuzz_netcode.c sodium/sodium.c -o /tmp/fz_netcode
FUZZ_ITERS=300000 UBSAN_OPTIONS=halt_on_error=1 /tmp/fz_netcode
```

netcode connect token (swap the source file for the connect-token target):
```
clang -DFUZZ_STANDALONE -DNETCODE_DEBUG -Inetcode -Isodium -Ifuzz \
  -fsanitize=address,undefined -fno-sanitize=nonnull-attribute -fno-sanitize-recover=all -g \
  fuzz/fuzz_netcode_connect_token.c sodium/sodium.c -o /tmp/fz_netcode_connect_token
FUZZ_ITERS=300000 UBSAN_OPTIONS=halt_on_error=1 /tmp/fz_netcode_connect_token
```

connection:
```
clang++ -std=c++11 -DFUZZ_STANDALONE -DYOJIMBO_DEBUG -DNETCODE_DEBUG -DRELIABLE_DEBUG -DSERIALIZE_DEBUG \
  -I. -Iinclude -Isodium -Itlsf -Inetcode -Ireliable -Iserialize -Ifuzz \
  -fsanitize=address,undefined -fno-sanitize=nonnull-attribute -fno-sanitize-recover=all -g \
  fuzz/fuzz_connection.cpp source/*.cpp netcode/netcode.c reliable/reliable.c tlsf/tlsf.c sodium/sodium.c \
  -o /tmp/fz_connection
FUZZ_ITERS=100000 /tmp/fz_connection
```

`-fno-sanitize=nonnull-attribute` is required for any target that links libsodium (netcode,
connection): netcode encrypts zero-length AEAD plaintexts, so libsodium does
`memcpy(dst, NULL, 0)` — benign UB in upstream code.

## Build — real libFuzzer (Linux clang), the CI mode
Swap `-DFUZZ_STANDALONE` for `-fsanitize=fuzzer` (added to the sanitizer set) and pass a
corpus dir + `-max_total_time=N`. Exactly what the `fuzz` CI job does; see it for the
canonical commands.

## Seed corpora (`fuzz/corpus/<target>/`)

Committed seeds of valid packets so the time-boxed CI runs start at inputs that already
reach the post-decrypt / reassembly code instead of rediscovering the wire format from
random bytes. The `fuzz` CI job passes `fuzz/corpus/<target>` as a read-only seed dir
alongside an ephemeral working dir (libFuzzer writes new finds only to the first dir, so the
committed seeds stay pristine). Standalone builds can replay them too:
`./fz_netcode fuzz/corpus/fuzz_netcode/*`.

The seeds are produced by generators under `tools/`, which round-trip every seed through the
matching reader and assert it decodes, so a committed seed is always a valid input:

```
# netcode + connect-token + reliable seeds
# (writes fuzz/corpus/fuzz_netcode, fuzz_netcode_connect_token, fuzz_reliable)
clang -DNETCODE_DEBUG -DRELIABLE_DEBUG -Inetcode -Isodium -Ireliable -Ifuzz -g \
  tools/gen_seed_corpus.c reliable/reliable.c sodium/sodium.c -o /tmp/gen_seed_corpus
/tmp/gen_seed_corpus fuzz/corpus

# connection seeds (writes fuzz/corpus/fuzz_connection)
clang++ -std=c++11 -DYOJIMBO_DEBUG -DNETCODE_DEBUG -DRELIABLE_DEBUG -DSERIALIZE_DEBUG \
  -I. -Iinclude -Isodium -Itlsf -Inetcode -Ireliable -Iserialize -Ifuzz -g \
  tools/gen_seed_corpus_connection.cpp source/*.cpp netcode/netcode.c reliable/reliable.c tlsf/tlsf.c sodium/sodium.c \
  -o /tmp/gen_seed_corpus_connection
/tmp/gen_seed_corpus_connection fuzz/corpus
```

Regenerate and re-commit the seeds if a wire format changes. The netcode generator shares
`fuzz_netcode_params.h` (protocol id + timestamp) with the harness so its packets decrypt
under the harness's keys.

## Ideas for later
- Richer seeds: multi-fragment blocks, every netcode packet type, connection-request packets.
- A target over `netcode_server_process_packet` end-to-end (needs a minimal server without a
  real socket).
