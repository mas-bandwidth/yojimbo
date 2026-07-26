# Security Policy

yojimbo is a networking library that parses untrusted data straight off the wire —
encrypted UDP packets, connect tokens, and the message/block streams inside them — so we
take memory-safety and protocol-parsing bugs seriously.

## Reporting a vulnerability

**Please do not report security issues in public GitHub issues or pull requests.**

Report privately through either channel:

- **GitHub private vulnerability reporting** (preferred): on this repository, go to the
  **Security** tab → **Report a vulnerability**. This opens a private advisory visible only
  to the maintainers.
- **Email**: glenn@mas-bandwidth.com.

Please include enough detail to reproduce: the affected component and version/commit, a
description of the flaw, and — where possible — a proof-of-concept input or a small patch.
Fuzzing crash artifacts (a crashing input file plus the target name) are ideal.

We will acknowledge your report, keep you updated on our assessment, and coordinate
disclosure timing with you. We prefer coordinated disclosure and will credit reporters who
wish to be named.

## Scope

In scope — bugs in this repository, including the vendored components maintained here:

- the yojimbo library itself (`source/`, `include/`);
- the in-tree **netcode** (`netcode/`), **reliable** (`reliable/`), and **serialize**
  (`serialize/`) sources;
- the pruned **libsodium** subset under `sodium/` **as vendored** (e.g. an amalgamation or
  pruning mistake). Vulnerabilities in upstream libsodium itself should be reported to the
  [libsodium project](https://github.com/jedisct1/libsodium); we track upstream and pull in
  fixes.

Especially of interest: memory-safety issues (out-of-bounds read/write, use-after-free,
overflow) reachable from a received packet, connect token, or message stream; and protocol
flaws that let a peer bypass authentication, encryption, or replay protection.

Out of scope: bugs in your own game code built on top of yojimbo, denial-of-service that
requires an already-authenticated malicious peer flooding traffic, and issues in build
tooling or example/test code that are not reachable at runtime.

## Known issue: AEAD nonce reuse in vendored netcode (fixed in 1.7.0)

**Advisory: [GHSA-hqp3-fj6v-hrpc](https://github.com/mas-bandwidth/yojimbo/security/advisories/GHSA-hqp3-fj6v-hrpc)** (published 2026-07-26; a CVE has been requested and is pending assignment).

**Affected: yojimbo 1.6.3 and earlier. Fixed in 1.7.0.**

yojimbo vendors netcode. Releases up to and including 1.6.3 carry a netcode at or below
1.3.5, which is affected by an AEAD nonce reuse issue: the server's global packet sequence
was seeded only on *create* and not on *start*, so a server stopped and restarted in the
same process could emit global packets (connection challenge, connection denied) at
sequence numbers already used under the same server-to-client key. netcode uses the packet
sequence as the AEAD nonce, so this is nonce reuse under ChaCha20-Poly1305 — which voids
both confidentiality and integrity for the affected packets.

Fixed upstream in netcode 1.4.0; **yojimbo 1.7.0** is the first release to vendor it.

### If you are using an affected version

Upgrade to 1.7.0 or later. If you cannot, avoid restarting a server in-process — a fresh
process is unaffected, because the sequence is seeded at creation.

### Where affected versions can still be obtained

The legacy Conan remote `center.conan.io` serves `yojimbo/1.2.1`, which vendors an affected
netcode. That remote is frozen: nobody, including us, can update or withdraw it. This is
recorded because we cannot remove it and a user has no other way to find out.

### Why this has its own advisory

yojimbo carries netcode as vendored source rather than as a declared dependency. Vendored
code is invisible to dependency scanners, so a yojimbo user would not learn they are
affected from an advisory filed only against netcode. Upstream: `netcode` <= 1.3.5, fixed in
1.4.0 ([GHSA-3x95-24j9-7448](https://github.com/mas-bandwidth/netcode/security/advisories/GHSA-3x95-24j9-7448)).

## Supported versions

Fixes land on `main` and ship in the next tagged release. Please test against `main` before
reporting, in case the issue is already fixed. Older tagged releases are not maintained; we
generally do not backport security fixes.

## Hardening in this repository

The parsers that handle untrusted input are exercised by libFuzzer targets under `fuzz/`
(reliable packet receive, netcode packet read, and the yojimbo connection deserializer) and
by an address/undefined-sanitizer build and a time-boxed sanitized soak test in CI. If you
are investigating a suspected issue, those targets are a good place to start.
