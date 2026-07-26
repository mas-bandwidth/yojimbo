<!-- HOT:BEGIN -->
## HOT — read before reasoning about this repo

WHAT: the C++ client/server networking library built on netcode + reliable + serialize,
all three VENDORED in-tree (netcode/, reliable/, serialize/) plus a pruned libsodium subset
in sodium/.

**THE INTERFACE DOES NOT CHANGE.** The library is over ten years old and deliberately
stable. Do not propose interface changes.

**VENDORED CODE IS NOT EDITED HERE.** sodium/ is byte-identical to netcode's copy and
`.github/workflows/sodium-parity.yml` fails if it drifts; see sodium/NOTES.md. Patch
upstream, re-vendor, let it flow down.

DECISIONS THAT READ AS BUGS (they are not — do not "fix" them)
- **Debug asserts are deliberately kept OFF the untrusted network read path**, so a hostile
  peer cannot crash a debug server. This is the single most important invariant to preserve
  when adding validation: the full-trust rule for programmer inputs must never be confused
  with the zero-trust rule for wire data.
- **Config invariants assert in constructors** (power-of-two buffer sizes, channel counts)
  so every debug run validates config at startup; usage contracts assert where exercised;
  API misuse ALSO degrades gracefully in release. That layering is intentional.
- **Malformed wire input degrades to a channel error and a disconnect — never an assert.**
  Validation happens before the memcpy (see the over-long-final-fragment check in
  `ReliableOrderedChannel::ProcessPacketFragment`).
- **Allocation failure is handled on essentially every path** deliberately, because this is
  a game server.

SECURITY: yojimbo 1.6.3 and earlier vendor a netcode missing the AEAD nonce-reuse fix
(netcode 1.4.0); 1.7.0 was the first with it. See netcode's SECURITY.md.

THE WRITE/READ RULE — read this BEFORE reporting any assert as a missing bounds check
Glenn, 2026-07-26: "intention is on write, user is responsible to not crash or do undefined
behavior. asserts are there to help. callers responsibility. on read, obviously, we must
check." Plus Postel: "be conservative in what you send, permissive in what you receive."
  WRITE / caller-supplied -> the CALLER validates. Assert-only is the DESIGN; -DNDEBUG
    removing it is correct. Do NOT add runtime checks here.
  READ / off the wire     -> the library checks at runtime, for safety.
yojimbo states this in its own code: yojimbo_config.cpp:34-36 explains that YOJIMBO_CONFIG_CHECK
logs-then-asserts and the whole Validate() family is compiled to nothing in release
(yojimbo_config.cpp:163-175). That is deliberate, not an oversight.
DELIBERATELY ASSERT-ONLY, do NOT "fix" -- a future audit that finds these has found the
contract: oversized block-message send (reliable_ordered_channel.cpp:562/611 -- SendMessage
exempts block messages at :168 on purpose); BaseServer::Start's maxClients
(base_server.cpp:48-50); Client::GenerateInsecureConnectToken (client.cpp:72); the whole
clientIndex/channelIndex public surface.
The WIRE path is properly checked and was verified clean by two independent audits: wire
channelIndex (channel.cpp:446), numChannelEntries (connection.cpp:58), numFragments/fragmentId
(reliable_ordered_channel.cpp:712-724) and the receive memcpy bounds test at :733-738.
yojimbo also satisfies serialize's bytes%8 write contract deliberately at connection.cpp:248
(maxPacketBytes &= ~7) -- do not "simplify" that line.
FIXED 2026-07-26 (#320): Address::IsMulticast/IsLinkLocal/IsSiteLocal compared the first
16-bit group for EXACT equality instead of masking the prefix, and IsGlobalUnicast is their
negation, so ff02::1 (all nodes) reported as GLOBAL UNICAST. Now masked: /8 for multicast,
/10 for link and site local. There had been ZERO test coverage of these predicates, which is
how they drifted -- test_address_classification now covers the range boundaries.
<!-- HOT:END -->

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
- **The interface does not change.** The library is over ten years old, deliberately
  keeps the C++ dialect it was written in, and will not break the API its users have come
  to expect. This includes the manual message refcounting contract (Create/Send transfers
  ownership, Receive obligates a Release — the rules are stated explicitly in USAGE.md,
  and the debug leak checker enforces them). Do not propose interface changes,
  modernization, or RAII/smart-pointer wrappers.
- **Sodium vendoring follows netcode.** The process for generating and validating the
  pruned libsodium subset is documented in the **netcode repository** under
  `sodium/NOTES.md`, intentionally not duplicated into this repo — don't "fix" or copy
  it. netcode's nightly CI tracks upstream libsodium releases; to update yojimbo's copy,
  follow that process in netcode, then re-vendor. `-DYOJIMBO_SYSTEM_SODIUM=ON` links the
  system library instead.

## Verdict

This is a mature, genuinely well-engineered library, and the README's claim of
"stable and production ready" is credible. Ten years of history show in the right ways:
the hard-won invariants are written down where they matter (the sequence-buffer
wraparound war story, the attacker-controlled-fragment bounds check), the error paths are
handled rather than hoped away, and the recent hardening pass — fuzzers over every
untrusted parser, sanitizers and a sanitized soak test in CI, allocation-failure
injection tests — puts it well above the norm for open-source game networking. My honest
overall opinion: I would trust this library in production. The July 2026 audit raised a
set of concerns; every one of them has since been fully addressed — fixed in code, or
confirmed with the author as deliberate design and documented — so this audit carries no
open concerns.

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
- **Interface stability as a feature.** The library is over ten years old and
  deliberately keeps the C++ dialect it was written in and the API its users have come
  to expect — no interface churn chasing language fashion. That is exactly what you want
  from a networking layer a shipped game depends on: yojimbo is a stable, mature library
  that values compatibility, for serious people to get things done with. (The two header
  side effects that reach user translation units on Windows — the `#undef SendMessage`
  and the MSVC warning pragmas — are documented in BUILDING.md.)
- **Operational visibility, within the contract.** `ClientServerConfig::Validate()` runs
  at server start and on every client connect, asserting each config invariant with a
  message naming the field and the fix — debug builds only, compiling away in release
  (this is what catches the classic mistake of raising `maxPacketSize` without updating
  the derived `maxPacketFragments`). Both sides report *why* a connection ended:
  `Server::GetClientDisconnectReason(clientIndex)` distinguishes kicked, clean
  disconnect, timed out, and the specific connection errors (serialize failure, desync,
  out of memory, …), recorded before `Adapter::OnServerClientDisconnected` fires so it
  can be queried from the callback; `Client::GetDisconnectReason()` preserves the
  detailed netcode failure states, so the game can tell the player "server is full"
  versus "update your client" versus "network error". Stack usage never scales with
  config values — packet-path scratch lives on the heap or in fixed-size batches.

## Bottom line

The architecture is sound, the security engineering is unusually serious for the genre,
and the code reads like it's been maintained by someone who has debugged it at 90% packet
loss — because it has. The design contract is coherent: zero trust for anything off the
wire, full trust plus debug-time assert enforcement for the programmer's own inputs, zero
overhead in release, and an interface that does not move underneath its users. The July
2026 audit's findings were worked through with the author to full resolution — fixed in
code (startup config validation, disconnect-cause telemetry on both sides including the
timed-out vs clean-disconnect split via netcode v1.3.2, no config-scaled stack usage), or
confirmed as deliberate design and documented (the ownership rules in USAGE.md, the
design assumptions in the README, the Windows header notes in BUILDING.md). If I were
choosing a C++ client/server layer for a competitive multiplayer game today, this would
be on my shortlist, and the source being small and readable enough to audit in an
afternoon is a large part of why.
