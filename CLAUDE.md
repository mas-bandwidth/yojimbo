# CLAUDE.md — Audit of yojimbo

*An honest code audit written by Claude (July 2026), covering the yojimbo library proper
(`include/`, `source/`), its vendored dependencies (`netcode/`, `reliable/`, `serialize/`,
`sodium/`, `tlsf/`), tests, fuzzing, CI, and documentation.*

## Orientation

yojimbo is a C++ client/server network library for real-time multiplayer games. It layers
cleanly: **yojimbo** (messages, channels, connection, allocators) sits on **reliable**
(acks, packet fragmentation), **netcode** (encrypted UDP, connect-token authentication),
and a pruned **libsodium** (crypto), with **serialize** (bitpacker) and **tlsf**
(per-client heaps) alongside. Each dependency is vendored as an amalgamated one- or
two-file library maintained by the same author. The yojimbo layer itself is ~9,300 lines
of source plus well-documented headers — small enough to hold in your head, which is a
feature.

## Design contract (per the author)

These are deliberate design decisions, not defects — reviews and changes should work
within them:

- **Asserts enforce programmer contracts.** Correct usage and configuration are validated
  by asserts in debug builds; release builds pay zero overhead because the programmer is
  expected to have validated their integration in debug before shipping. Any added
  validation must follow this pattern (debug-only, fails loudly, compiles away).
- **The config must be identical on client and server** or they won't work. Each side can
  only validate locally; keeping the two sides in sync is the integrator's
  responsibility.
- **Single-threaded by design.** All client/server/message APIs are called from one
  thread.
- **Designed for client/server games with ~100 players or less** — the memory model,
  `MaxClients = 64`, and per-client heap sizing all assume that scale.
- **Log level semantics:** `ERROR` is an error, `INFO` is infrequent but important
  ("client connected"), `DEBUG` is too frequent for info, and per-packet spam goes below
  that. `LOG_LEVEL_NONE` is not a severity — it's a glorified printf for emitting data to
  library users without log-level filtering interfering (e.g. the assert handler's
  output).

## Verdict

This is a mature, genuinely well-engineered library, and the README's claim of
"stable and production ready" is credible. Ten years of history show in the right ways:
the hard-won invariants are written down where they matter (the sequence-buffer
wraparound war story, the attacker-controlled-fragment bounds check), the error paths are
handled rather than hoped away, and the recent hardening pass — fuzzers over every
untrusted parser, sanitizers and a sanitized soak test in CI, allocation-failure
injection tests — puts it well above the norm for open-source game networking. My honest
overall opinion: I would trust this library in production. The criticisms below are real
but they are footguns and polish, not rot.

## What's genuinely good

- **Security posture.** Encryption and authentication are on by default, not opt-in. The
  connect-token model pushes unauthenticated-traffic rejection down into netcode before
  any per-client state is allocated. The untrusted read path is disciplined: every
  wire-read value is range-bounded by the serializer, validation happens *before* the
  memcpy (see the over-long-final-fragment check in
  `ReliableOrderedChannel::ProcessPacketFragment`), and malformed input degrades to a
  channel error level and a disconnect — never an assert, never trust. Debug asserts are
  deliberately kept off the read path so a hostile peer can't crash a debug server.
- **The fuzzing setup is exemplary.** Five libFuzzer targets cover each parsing layer,
  including the post-decrypt layers that naive fuzzing can't reach (the decrypted
  connect-token parser gets its own target because the AEAD boundary blocks mutation; the
  stateful connection target threads multiple packets through one live `Connection` so
  block reassembly is reachable; the structured target drives the real write path so the
  receiver never bails early). CI runs them per-PR, a nightly runs them long with a
  persistent corpus and dictionaries, and `fuzz/README.md` documents actual bugs found —
  with regression tests in `test.cpp`. That is evidence of a working process, not
  security theater.
- **Memory architecture.** Everything allocates through a pluggable `Allocator`;
  the server silos each client into its own TLSF heap, so one client exhausting its
  budget cannot starve another or the global heap — the right containment model for a
  game server. Allocation failure is handled deliberately on essentially every path, with
  ownership reasoning written at the site (who holds the reference, what must be freed).
  Debug builds track leaks for both memory and messages.
- **The serialization design.** One templated serialize function per message, compiled
  three ways (read/write/measure), with packet budgeting driven by the measure stream.
  It's elegant, it's fast, and the unified path makes read/write mismatches structurally
  harder to write.
- **Asserts as the contract-enforcement mechanism.** The library's explicit design
  contract: asserts enforce correct usage and configuration in debug builds, and release
  builds pay zero overhead because the programmer has already validated their integration
  by running it in debug. The assert placement makes that contract actually work — config
  invariants (power-of-two buffer sizes, channel counts) assert in constructors, so every
  debug run validates the config at startup; usage contracts deep in the stack are
  asserted where they're exercised (e.g. `reliable.c` asserts the fragment count fits
  `max_fragments` at send time); API misuse paths additionally degrade gracefully in
  release (`yojimbo_assert(CanSendMessage())` followed by a handled error). Meanwhile
  asserts are deliberately kept *off* the untrusted network read path, so the full-trust
  rule for programmer inputs is never confused with the zero-trust rule for wire data.
  This is the classic game-engine contract, applied consistently.
- **Tests and CI.** 37 functional tests including adversarial cases (empty packets,
  fragment overflow, over-budget packets, receive-queue overflow, allocation-failure
  injection), Linux/macOS/Windows builds, ASan+UBSan+LSan, a separate MSan job with a
  written rationale for its build flags, and a sanitized 20k-iteration soak run per PR.
- **Documentation.** Header comment density is exceptional, `USAGE.md` is a real
  integration walkthrough rather than API listing, and `SECURITY.md` is clear about
  scope, reporting, and the vendored-sodium liability. Most commercial codebases don't
  document this well.

## Honest criticisms

Ordered by how much I think they matter. (Two items from the first draft of this audit —
"config validation is assert-only" and "`maxPacketFragments` goes stale" — were revised
after the author clarified the assert design contract: debug-only validation is the
intended mechanism, not a defect. What survived is captured in the note below and the
list that follows.)

*Note on config validation:* the one genuine gap the original criticism found was that
`maxPacketFragments` is derived from `maxPacketSize` inside the `ClientServerConfig`
constructor, so raising `maxPacketSize` afterwards (the documented config pattern)
silently leaves it too small — the debug assert for that case fired three layers down in
`reliable.c`, at send time, only when a large enough packet was actually generated, and
nothing pointed at the config field. `soak.cpp` itself had this latent bug. This is now
closed within the contract: `ClientServerConfig::Validate()`
(`source/yojimbo_config.cpp`) runs at `Server::Start` and on every client connect path,
asserts each config invariant at startup with a message naming the field and the fix, and
compiles away entirely in release.

1. **Disconnect diagnostics** — *addressed on both sides.* Server:
   `Server::GetClientDisconnectReason(clientIndex)` returns a per-slot
   `ServerClientDisconnectReason` (kicked, transport disconnect/timeout, serialize
   failure, desync, out of memory, …), recorded before
   `Adapter::OnServerClientDisconnected` fires so it can be queried from the callback.
   Client: `Client::GetDisconnectReason()` returns a `ClientDisconnectReason` that
   preserves the detailed netcode failure states (connection denied, connect token
   expired/invalid, request/response/connection timed out, disconnected by server) plus
   the same connection-error causes, so the game can tell the player "server is full"
   versus "update your client" versus "network error".

2. **The single-threaded, ≤100-player contract is under-signposted.** Both are deliberate
   design decisions (see the design contract above), and the author is comfortable with
   them being surfaced — the criticism is only that the codebase barely states them: one
   line in `yojimbo_allocator.h` is the sole thread-safety statement. Contracts enforced
   by programmer responsibility only work when they're written down where integrators
   will read them; a prominent paragraph in README/USAGE would do it, because violating
   the threading contract produces rare refcount races in production, not a clean
   failure.

3. **Manual message refcounting is easy to misuse.** Create/Send transfers ownership;
   Receive obligates a Release; the compiler enforces none of it. This is consistent with
   the library's contract style, and the debug leak checker is its enforcement mechanism
   — though `exit(1)` from the factory destructor is hostile if the library is embedded
   in an editor or tool. This is the part of the API where integrators will make their
   first mistake.

4. **`alloca` sized by config in packet and tick paths.** With default configs the sizes
   are trivial, but `Client::AdvanceTime` puts ~12 bytes × `maxSimulatorPackets` (48KB at
   the default 4096) on the stack per tick when the simulator is active, and a user who
   cranks `maxMessagesPerPacket` or `maxSimulatorPackets` converts a config choice into a
   silent stack-overflow risk. Fixed member buffers or the channel allocator would be
   sturdier.

5. **The C++ dialect is dated — deliberately, but with a couple of leaks.** C++03 idioms
   (NULL, private copy constructors instead of `=delete`, no `override`, `std::map` in
   debug trackers) are a fair trade for portability into engine codebases with exceptions
   and RTTI off. Two things do leak out of that choice, though:
   `#pragma warning(disable: 4244)` in `yojimbo_config.h` suppresses narrowing warnings
   in *user* translation units that include yojimbo headers, and the `#undef SendMessage`
   in the public headers silently deletes the Win32 macro for users who include
   `windows.h` first. Both deserve at least a documentation note.

6. **Vendored-crypto maintenance burden.** The pruned libsodium subset means upstream
   CVEs must be tracked and re-vendored by hand. `SECURITY.md` acknowledges this honestly
   and `-DYOJIMBO_SYSTEM_SODIUM=ON` exists as an escape hatch, which is the right
   mitigation — but it remains a standing liability inherent to the amalgamation
   approach, and it's the one place where the single-maintainer model concentrates risk.

## Bottom line

The architecture is sound, the security engineering is unusually serious for the genre,
and the code reads like it's been maintained by someone who has debugged it at 90% packet
loss — because it has. The design contract is coherent: zero trust for anything off the
wire, full trust plus debug-time assert enforcement for the programmer's own inputs, and
zero overhead in release. With startup config validation now in place, the remaining asks
I'd weight most are disconnect-cause telemetry and writing the threading/scale contract
into the front-page docs. If I were choosing a C++ client/server layer for a competitive
multiplayer game today, this would be on my shortlist, and the source being small and
readable enough to audit in an afternoon is a large part of why.
