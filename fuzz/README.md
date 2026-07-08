# Fuzz targets (work in progress)

libFuzzer harnesses over yojimbo's untrusted-input parsers. Standalone (not part of
the premake build), so they don't affect the normal build or CI.

Each target is **dual-mode**:
- **Real libFuzzer** (Linux clang, coverage-guided) — the intended CI mode.
- **Standalone** (`-DFUZZ_STANDALONE`) — an ordinary executable that replays file
  args or feeds pseudo-random buffers (`FUZZ_ITERS` env var). Needed because Apple
  clang ships no libFuzzer. Build it with ASan+UBSan to shake out crashes.

## Targets
- `fuzz_reliable.c` — `reliable_endpoint_receive_packet` (header parse, ack decode,
  fragment reassembly). **Working**: 300k standalone inputs clean under ASan+UBSan.
- `fuzz_connection.cpp` — `yojimbo::Connection::ProcessPacket` (ReadStream +
  channel/message/block deserialization). **WIP — standalone build exits 1 with no
  output; needs debugging.** Prime suspect: `TestMessageFactory` dtor calls `exit(1)`
  on message leaks in debug builds. See CLAUDE.md "CURRENT TASK".

## Build — standalone, sanitized (macOS/Apple clang or Linux)

reliable:
```
clang -DFUZZ_STANDALONE -DRELIABLE_DEBUG -I reliable -I fuzz \
  -fsanitize=address,undefined -fno-sanitize-recover=all -g \
  fuzz/fuzz_reliable.c reliable/reliable.c -o /tmp/fz_reliable
FUZZ_ITERS=300000 UBSAN_OPTIONS=halt_on_error=1 /tmp/fz_reliable
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

## Build — real libFuzzer (Linux clang), intended CI mode
Swap `-DFUZZ_STANDALONE` for `-fsanitize=fuzzer` (added to the sanitizer set), drop the
standalone `main`, and pass a corpus dir + `-max_total_time=N`. Not yet wired into CI.

## TODO
- Fix `fuzz_connection` (see CLAUDE.md).
- Add a `netcode_read_packet` target.
- Add a CI job: Linux clang `-fsanitize=fuzzer,address,undefined`, short `-max_total_time`
  per target, seeded from a small corpus.
