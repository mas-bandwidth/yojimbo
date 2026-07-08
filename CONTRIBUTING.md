# Contributing to yojimbo

Thanks for your interest in improving yojimbo. This document covers how to build, test,
and submit changes.

## Reporting bugs and requesting features

Open a [GitHub issue](https://github.com/mas-bandwidth/yojimbo/issues) with enough detail to
reproduce: what you did, what you expected, and what happened. For build problems, include
your OS, compiler, and the CMake and command output.

**Security vulnerabilities are different** — please do **not** open a public issue. Follow
the private reporting process in [SECURITY.md](SECURITY.md).

## Building and testing

yojimbo builds with CMake. See [BUILDING.md](BUILDING.md) for the full details; the short
version:

```
cmake -B build                   # bundled libsodium by default
cmake --build build -j
./bin/test                       # must end with "ALL TESTS PASS"
```

Please make sure `./bin/test` passes in **both** a Debug and a Release build
(`-DCMAKE_BUILD_TYPE=Debug` / `Release`) before opening a pull request, and add or update
tests when you change behavior.

## What CI checks

Every pull request runs the GitHub Actions workflow in `.github/workflows/ci.yml`. It must be
green before a change is merged. The jobs are:

- **build & test** on Linux (x86-64) and macOS (Apple Silicon), in debug and release;
- **build & test** on Windows (MSVC x64), debug and release;
- **build & test** on Linux against the **system** libsodium (`--sodium=system`);
- **sanitizers**: a Linux ASan + UBSan + LSan build of the test suite;
- **fuzz**: short libFuzzer runs of the parser targets in `fuzz/`, seeded from
  `fuzz/corpus/`;
- **soak**: a time-boxed, sanitized run of the soak test.

If you touch an untrusted-input parser, consider extending the relevant target in `fuzz/`
(see [fuzz/README.md](fuzz/README.md)); a crashing corpus input makes a great bug report.

## Coding style

The repository ships a `.clang-format` that encodes the existing style (Allman braces,
four-space indent, spaces inside parentheses, `Type * name` pointers). Match the surrounding
code; new or edited code should follow the config, but please **do not** mass-reformat
existing files in a feature PR — it buries the real change.

The vendored third-party code under `sodium/` and `tlsf/` is kept byte-for-byte as upstream
(`sodium/` is verified identical to libsodium) and has its own `DisableFormat` `.clang-format`
— do not reformat or hand-edit it; re-sync from upstream instead.

New source files should carry the same BSD 3-Clause license header as the existing sources.

## Pull requests

- Keep each pull request focused on **one** change; small, self-contained PRs are easier to
  review and to revert.
- Write a clear description of what the change does and why.
- Make sure the full CI matrix is green.

## License

yojimbo is released under the [BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
By contributing, you agree that your contributions are licensed under the same terms.
