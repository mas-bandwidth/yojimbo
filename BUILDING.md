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

cheers

 - Glenn
