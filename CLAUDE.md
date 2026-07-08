# Working notes for yojimbo

Context for AI agents (and humans) working on this repo. yojimbo is the official
client/server networking library by Glenn Fiedler / Mas Bandwidth. It vendors three
sibling libraries in-tree: **netcode** (`netcode/`), **reliable** (`reliable/`),
**serialize** (`serialize/`), plus **tlsf** (allocator) and a pruned **libsodium**
(`sodium/`). The Go **matcher** (`matcher/`) is a reference matchmaker.

## Build & test

The build is **CMake** (`CMakeLists.txt`); the `build*/` dirs are gitignored.

```
cmake -B build                 # bundled libsodium (the default); Debug by default
cmake --build build -j
./bin/test                     # the unit/integration suite — must end with "ALL TESTS PASS"
./bin/soak                     # long-running soak at high packet loss (Ctrl-C / kill)
```

- **Configs:** default is Debug; pass `-DCMAKE_BUILD_TYPE=Release` for the optimized build.
  The `_DEBUG`/`_RELEASE` mode of yojimbo/netcode/reliable/serialize is derived from `NDEBUG`
  (set by CMake in Release, absent in Debug), so no explicit defines are needed.
- **libsodium backend:** bundled by default; `-DYOJIMBO_SYSTEM_SODIUM=ON` links the system
  `sodium` lib instead (needs `libsodium-dev` / `brew install libsodium`). The bundled
  `sodium/sodium.h` is always on the include path, so system mode uses that subset header and
  links the system implementation.
- **Windows:** `cmake -B build -G "Visual Studio 17 2022" -A x64` then
  `cmake --build build --config Debug|Release` (or open `build\Yojimbo.sln`).
- Executables land in `bin/` regardless of the out-of-source build dir.

### Sanitizers (how CI does it, and how to reproduce locally)
Inject flags via CMake — this is what found several bugs this session:
```
SAN="-fsanitize=address,undefined -fno-sanitize=nonnull-attribute -fno-sanitize-recover=all -g"
cmake -B build-san -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="$SAN" -DCMAKE_CXX_FLAGS="$SAN" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-san -j --target test
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ./bin/test
```
- **`-fno-sanitize=nonnull-attribute` is required**: netcode encrypts zero-length AEAD
  plaintexts, so libsodium does `memcpy(dst, NULL, 0)` — benign UB in upstream code, not
  a real bug. Leaving it in makes UBSan abort.
- macOS ASan has **no leak detection** (LSan is Linux-only). Do leak-hunting on Linux CI.

## CI

`.github/workflows/ci.yml` (GitHub Actions). Jobs: build+test on **Linux (ubuntu-24.04)**
and **macOS Apple Silicon (macos-14)** in debug+release; **Windows MSVC** (Debug+Release);
**`--sodium=system`** on Linux; a **Linux ASan+UBSan+LSan** job; a **libFuzzer** job (the
`fuzz/` targets, ASan+UBSan, seeded from `fuzz/corpus/`); a **time-boxed sanitized soak**; and
a **libFuzzer + MemorySanitizer** job (uninitialized-read detection — the C++ targets build
with `-DYOJIMBO_RELEASE` so the debug `std::map` leak trackers aren't compiled in and MSan
needs no instrumented libc++; an uninstrumented libstdc++ `std::map` traversal MSan-false
-positives, which is how the first attempt failed). A separate scheduled workflow
`fuzz-nightly.yml` runs each target far longer (matrix, persisted+growing corpus via the
Actions cache, per-target `fuzz/dict/*.dict`), uploading any crash reproducer as an artifact;
it never gates PRs and can be run on demand via `workflow_dispatch`. macOS **Intel (macos-13)** is commented out in the matrix (slow to allocate). Badge
is in the README. **MSan is Linux-only — it cannot be reproduced on the macOS dev machine;
CI is the only validator.**

## Vendored libsodium (`sodium/`) — it is generated

`sodium/` is a **pruned + amalgamated** subset of libsodium 1.0.20: just `sodium.h` +
`sodium.c` + `NOTES.md`. Only the primitives netcode uses are kept (ChaCha20-Poly1305 IETF
+ XChaCha20, randombytes, sodium_init) with **all** their optimized impls (ChaCha20
ref/SSSE3/AVX2, Poly1305 donna/SSE2) selected at runtime. Key facts:
- The two files were produced by an **amalgamation generator** (concatenate pruned upstream
  files; inline the dolbeau `u*.h` / poly1305 `donna32/64.h` implementation-body headers;
  bracket the SIMD variants that reuse the same `static` names with `#define`/`#undef`;
  `#undef` each file's local macros at its section end). The generator was **not committed**
  (the pre-amalgamation per-file sources are in git history). If re-syncing libsodium, redo
  the amalgamation; see `sodium/NOTES.md`.
- SIMD is enabled by compiler-macro auto-detection in `sodium.h` (was dormant before: gated on
  never-defined `NETCODE_*` macros). Uses `#pragma clang attribute push(target(...))` /
  `#pragma GCC target` per function, so no global `-m` flags needed.
- Verified **bit-identical to stock libsodium 1.0.20** and passes libsodium's own AEAD KAT
  `.exp` vector tests, on arm64 (ref/donna64) and x86-64 (SSSE3+SSE2; AVX2 compiled).
- `test.cpp` has `test_crypto_aead_vectors()` (a KAT) so the bundled crypto is exercised in CI.

## Gotchas learned this session

- **BitReader over-read:** the serialize BitReader reads whole 32-bit words, so it may touch
  up to 3 bytes past the logical packet end. Real receive buffers always have slack; any
  harness/caller passing an exact-size buffer must pad (round up to 4 / zero-fill) or ASan
  will (correctly) flag a heap over-read. (The alignment-UB fix in #251 made unaligned reads
  safe but did NOT change this over-read.)
- **Mixing debug/release objects → SIGSEGV in `GetPacketData` (addr 0x10).** If you hand-link
  objects built with different `YOJIMBO_DEBUG`/`YOJIMBO_RELEASE` settings (Debug vs Release)
  you get a struct-layout mismatch that crashes deep in yojimbo, *not* a sodium problem. With
  CMake, use a separate build dir per config (e.g. `build-debug`/`build-release`) so objects
  never mix; this matters most for the hand-rolled fuzz/coverage build commands.
- **The Bash tool runs zsh**, which does NOT word-split unquoted variables. Use `bash <<'EOF'`
  for loops over `$var` lists. `grep -c` exit code 1 on zero matches breaks `&&` chains.
- **Stray files to never commit:** libsodium test harness drops `*.res` in cwd; the Go build
  drops `matcher/matcher`. Both are now gitignored-ish / removed — watch for them.

## Work done this session (all merged to main unless noted)

- #251 BitReader unaligned dword reads (UB) — memcpy-based loads in `serialize.h`.
- #252/#253 libsodium prune → 1.0.20 align → constant-time hardening → keep all optimized
  impls on all platforms → bundle by default (`--sodium=system` opt-out) → amalgamate to
  `sodium.h`+`sodium.c`. (Note: #252 only partially merged; #253 re-landed the rest.)
- #254 network-simulator duplicate-packet leak + block fragment-count overflow
  (`GetMaxFragmentsPerBlock` floor→ceil) + regression test.
- #255 GitHub Actions CI. #256 README badge. #257 minor audit nits (bounds assert `<=`→`<`,
  reliable reassembly NULL-alloc check, InitializeYojimbo error-path leak). #258 remove dead
  `SODIUM_STATIC`. #259 fix `netcode_enable_packet_tagging` prototype (`int`→`void`). #260
  `yojimbo.cpp` include netcode.h/reliable.h instead of hand-declared prototypes.
- #261 matcher `expireSeconds` uint64→int64 (always-true expiry check) + safer
  `verboseError`/key defaults. #262 CLAUDE.md working notes + first fuzz harnesses.

## Fuzzing (`fuzz/`)

libFuzzer harnesses over the untrusted-input parsers, dual-mode (`-DFUZZ_STANDALONE` for an
ASan/UBSan driver since Apple clang has no libFuzzer; `-fsanitize=fuzzer` on Linux CI). See
`fuzz/README.md` for the canonical build/run commands. All three run clean at ≥300k
standalone inputs under ASan+UBSan, and a `fuzz` CI job builds + time-boxes them.
- `fuzz/fuzz_reliable.c` — `reliable_endpoint_receive_packet`.
- `fuzz/fuzz_netcode.c` — `netcode_read_packet` (`#include "netcode.c"` — the reader and its
  replay-protection / packet-type symbols are internal to that TU; tests are off by default).
- `fuzz/fuzz_netcode_connect_token.c` — `netcode_read_connect_token_private` (the decrypted
  private-token parser: address-list loop, keys, user data). Separate target because the AEAD
  boundary makes this parser unreachable by mutating bytes through `fuzz_netcode`.
- `fuzz/fuzz_connection.cpp` — `yojimbo::Connection::ProcessPacket`. An earlier "exits 1
  with no output" was the `TestMessageFactory` dtor's `exit(1)` leak check firing (its report
  was swallowed by the default log level, hence silent). It was a REAL bug the fuzzer found,
  not a harness artifact. Bringing it to clean turned up **three library bugs, now fixed:**
  1. **Message leak** in `SerializeBlockFragment` (`source/yojimbo_channel.cpp`): the "block
     fragment attached to non-block message" error path returned without releasing the just
     -created message.
  2. **`bufferSize > 0` assert** in `Connection::ProcessPacket` (`yojimbo_connection.cpp`) on a
     zero-payload packet — reachable because a packet that is exactly a reliable header hands
     the reader 0 payload bytes. Now rejected up front as a read failure.
  3. **`ChannelPacketData` union misread** (`yojimbo_unreliable_unordered_channel.cpp`):
     `ProcessPacketData` read `packetData.message` without checking `blockMessage`, so a block
     fragment addressed to an unreliable channel was reinterpreted as a message-pointer array
     → wild pointer deref (SEGV in `Message::SetId`). The unreliable channel never *sends* a
     top-level block fragment (its blocks go inline via `SerializeMessageBlock`), so receiving
     one is malformed; now treated as a serialize failure like the reliable channel does.
  Regression tests for (2) and (3) are in `test.cpp`
  (`test_connection_reject_empty_packet`, `test_connection_unreliable_rejects_block_fragment`);
  both verified to fail without their fix. (1) is covered by the fuzzer.
  `fuzz_connection` was later made **stateful** (input = `[u8 config selector][u16 len][pkt]...`
  fed to one long-lived Connection, so block reassembly + the out-of-order receive queue are
  reachable), given a **config selector** (`fuzz_config.h`) and a **rich message factory**
  (`fuzz_messages.h`, the whole `serialize_*` vocabulary). That combination found a 4th bug:
  4. **Uninitialized `block.message` deref** (`source/yojimbo_channel.cpp`
     `ChannelPacketData::Initialize`): it only zeroed the low 4 bytes of the message/block
     union, so a block fragment arriving on a `disableBlocks` channel (early return before the
     block pointers are nulled) left `block.message` half-garbage, and the packet destructor's
     `Free()` dereferenced it → SEGV. Fixed by zeroing both union arms; regression test
     `test_connection_reliable_block_fragment_on_disabled_blocks`.
  `fuzz/fuzz_connection_structured.cpp` complements the byte-level target: it drives a
  sender/receiver pair from a *script* (send message / tick-deliver-or-drop), so every packet is
  valid and the receive state machine (reassembly, in-order delivery under loss) is reached
  directly rather than by chance.
  `tools/fuzz_coverage.sh` reports `llvm-cov` over the deserialization sources (the channel
  `.cpp` percentages are dominated by unreachable send-side code — look at the read-path
  functions and `serialize/serialize.h`, which the rich messages lifted from ~15%→~24% region
  coverage on the seed corpus).

Seed corpora live in `fuzz/corpus/<target>/` (committed). The `fuzz` CI job passes them as a
read-only seed dir alongside an ephemeral working dir so runs start at inputs that already
reach the post-decrypt / reassembly code. Seeds are produced by generators under `tools/`
(`gen_seed_corpus.c` for netcode+reliable, `gen_seed_corpus_connection.cpp`) that round-trip
each seed through the matching reader and assert it decodes; the netcode generator shares
`fuzz/fuzz_netcode_params.h` (protocol id + timestamp) with the harness. Regenerate + recommit
if a wire format changes.

## Suggested next improvements (from the audit)

Done: fuzzing (harnesses incl. the connect-token target, CI job, seed corpora); time-boxed
sanitized `soak` in CI; `SECURITY.md`; `.clang-format`; `CONTRIBUTING.md`; markdown
consistency pass.

Remaining / open:
- **libsodium amalgamation generator** under `tools/` — the one real gap. The generator that
  produced `sodium/sodium.{h,c}` was never committed; the per-file pre-amalgamation sources
  live at `4a745d6^` and the amalgamation left `/* ===== <file> ===== */` section markers, so
  it's reconstructable but non-trivial (byte-exact reproduction of 6.5k lines with the SIMD
  `#define`/`#undef` bracketing). Glenn deprioritized this — don't start it unless asked.
- Ideas only: a `netcode_server_process_packet` end-to-end fuzz target (needs a socketless
  server); richer message-serialization tests.

## Assessment of yojimbo (my opinion, for future me and anyone curious)

Written after a session spent fuzzing the parsers, fixing the bugs that surfaced, and reading
most of the core. This is my honest read, not marketing.

**Verdict: a genuinely good, production-grade library for its niche** — dedicated-server
competitive multiplayer (FPS-style) where a web backend hands clients signed *connect tokens*
and they connect to game servers over authenticated, encrypted UDP. The README's "stable and
production ready" is fair. I'd trust it for a real game.

**What it gets right**
- *Clean layering.* Three focused libraries with real separation: **netcode** (connection +
  crypto + connect-token auth + replay protection), **reliable** (acks, RTT, fragmentation),
  **serialize** (the bitpacker). yojimbo is a thin, coherent client/server/message layer on
  top. Each piece is understandable on its own.
- *Crypto done the right way.* It doesn't roll its own — it vendors libsodium and uses real
  AEAD (ChaCha20-Poly1305) with a sound token model and replay protection. The vendored sodium
  subset is unusually careful: pruned, amalgamated, and *verified bit-identical to upstream*
  with the KAT vectors run in CI. That's more rigor than most projects give a vendored crypto
  dep.
- *The unified read/write serialization* (one `Serialize` template drives measure/write/read)
  is elegant and eliminates a whole class of send/receive-mismatch bugs. It's the best idea in
  the codebase.
- *Allocator-aware and deterministic* — TLSF, explicit per-connection memory budgets, no
  hidden global allocation. Good for consoles / shipping titles.

**Where it's rough (be honest)**
- *The untrusted-input parsing layer had not been fuzzed before this session, and it showed.*
  Fuzzing `Connection::ProcessPacket` turned up three real, post-authentication,
  attacker-reachable bugs in short order — a `ChannelPacketData` union misread that
  dereferenced attacker bytes as a pointer (SEGV), a message leak on an error path, and an
  assert on a zero-payload packet. These sat latent for years. They're fixed and now fuzzed in
  CI, but the lesson stands: the crypto/auth boundary was solid, the *post-decrypt parsing*
  behind it was under-tested. Treat that layer with suspicion when changing it.
- *`ChannelPacketData` is a hand-managed tagged union* whose `Initialize()` only zeroes one
  arm; correctness depends on careful `blockMessage` sequencing and the reader nulling the
  block pointers. It works, but it's a footgun — exactly where bug #3 lived.
- *Debug invariants are blunt* — the message factory calls `exit(1)` on a leak, asserts
  `__builtin_trap()`. Effective at catching mistakes, hostile to embedding/tooling (this is
  why the fuzzer looked "silent" until the log level was raised).
- *Ergonomics are low-level by design.* You hand-write serialization, manually ref-count and
  release messages, run your own fixed-timestep loop, and must remember `InitializeYojimbo()`.
  Right for the audience (engine programmers), not friendly to newcomers — and the docs had
  drifted (USAGE.md never mentioned the mandatory init; BUILDING.md referenced VS2019). Now
  fixed.
- *Build friction:* now on CMake (was premake with gitignored makefiles); the one remaining
  build-tooling gap is that the clever amalgamated libsodium has no committed generator.

**Net:** the core protocol and crypto are sound and thoughtfully built; the safety floor was
raised materially this session by adding fuzzing + sanitizer/soak CI and fixing the parser
bugs. My residual caution is entirely about the message/channel *deserialization* code — it's
the least-defended, most-attacker-exposed surface, and it's where I'd keep looking if I were
hunting for the next bug.
