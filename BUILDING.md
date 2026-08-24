Building yojimbo
================

yojimbo builds with [CMake](https://cmake.org/) (3.16 or newer). By default it uses the
bundled minimal libsodium in `sodium/`, so there is nothing else to install.

## Building on macOS and Linux

From the yojimbo directory:

    cmake -B build
    cmake --build build -j

This produces the static libraries and the sample programs / tests in `bin/`. Run them with:

    ./bin/test           # unit + integration tests — must print "ALL TESTS PASS"
    ./bin/server         # run a server on localhost on UDP port 40000
    ./bin/client         # run a client that connects to the local server
    ./bin/soak           # long-running soak test at high packet loss (Ctrl-C to stop)

The default is a debug build. For an optimized build, pass the build type:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j

## Floating point: yojimbo requires -ffp-contract=off

yojimbo's wire arithmetic lives in `serialize.h`, whose compressed float quantizes with two
distinct roundings on write and on read: the product must round to float32 *before* the
constant is added. A compiler allowed to contract fuses the multiply and the add into a
single FMA and rounds once, which shifts the encoded integer and the decoded float by one
ulp — a change to what goes on the wire.

This build sets the right flag on every target it compiles, so nothing is required of you to
build yojimbo itself:

  - GCC/Clang: `-ffp-contract=off`. Not merely the absence of `-ffast-math` — GCC's default
    is `-ffp-contract=fast`, which contracts across statement boundaries, and clang's default
    `=on` still fuses within a single expression.
  - MSVC: `/fp:precise`.

**If you compile `serialize.h` in your own translation units, set the same flag there.** The
flag is `PRIVATE` to yojimbo's targets and deliberately does not leak into your build, so
this one is yours to set. It is easy to miss because the symptom is architecture-dependent:
FMA is in the aarch64 baseline and absent from the x86-64 one, so an unflagged build is
typically wrong on arm64 and accidentally right on amd64.

## Building on Windows

Install [CMake](https://cmake.org/download/) and Visual Studio (the free
[Community edition](https://visualstudio.microsoft.com/downloads/) works). Then, from the
yojimbo directory:

    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Debug

Open the generated `build\Yojimbo.sln` in Visual Studio if you prefer, or pass
`--config Release` for an optimized build. The executables are written to `bin\`.

### Windows header side effects

Two things to be aware of when including yojimbo headers in your own Windows code:

- The yojimbo headers `#undef SendMessage`, because `windows.h` defines it as a macro that
  would otherwise mangle the `SendMessage` methods on the client and server. If your code
  uses the Win32 API after including yojimbo, call `SendMessageA` / `SendMessageW`
  explicitly instead of relying on the macro.
- `yojimbo_config.h` defines `_CRT_SECURE_NO_WARNINGS` and disables MSVC warnings 4127
  (conditional expression is constant) and 4244 (narrowing conversion) via `#pragma`.
  These apply to any translation unit that includes yojimbo headers, so code in those
  files won't get narrowing-conversion warnings even at `/W4`.

## Using the system libsodium instead of the bundled one

To link the system-installed libsodium rather than the bundled subset, install it first:

    sudo apt install libsodium-dev   # Linux
    brew install libsodium           # macOS

then configure with `-DYOJIMBO_SYSTEM_SODIUM=ON`:

    cmake -B build -DYOJIMBO_SYSTEM_SODIUM=ON
    cmake --build build -j

## Building against system-installed dependencies

By default the build uses the vendored copies of serialize, reliable and netcode. For
package managers (e.g. homebrew), configure with `-DYOJIMBO_SYSTEM_DEPS=ON` to build
against system-installed copies of all three instead. They must be installed where CMake
can find them — pass `-DCMAKE_PREFIX_PATH=/path/to/prefix` for a custom location. In this
configuration the bundled libsodium isn't used at all (the system netcode supplies its own
crypto), and `cmake --install` installs the yojimbo headers and library:

    cmake -B build -DYOJIMBO_SYSTEM_DEPS=ON
    cmake --build build -j
    ./bin/test
    cmake --install build

Note that the test suite skips the embedded netcode and reliable self-test sections in
this configuration — system libraries are built without their test hooks.

## Building against a system-installed tlsf

tlsf is a private implementation detail of yojimbo's per-client allocators, and by
default the vendored copy is compiled directly into `libyojimbo` so the archive is
self-contained. Package managers that ship tlsf as its own package (e.g. vcpkg)
configure with `-DYOJIMBO_SYSTEM_TLSF=ON`: yojimbo then embeds nothing and links the
system tlsf library publicly instead. tlsf must be installed where CMake can find it
(`tlsf.h` plus a `tlsf` library — pass `-DCMAKE_PREFIX_PATH` for a custom location):

    cmake -B build -DYOJIMBO_SYSTEM_TLSF=ON
    cmake --build build -j
    ./bin/test

Combines freely with `-DYOJIMBO_SYSTEM_DEPS=ON`. This option exists so packages carry
no patches — the design came out of the vcpkg port review (thanks, vicroms).
