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

## Ideas for later
- Seed corpora (a captured valid packet per target) so the time-boxed CI runs reach the
  post-decrypt / reassembled-payload paths faster.
- A `netcode` connect-token / server-side read target.
