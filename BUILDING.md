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

## Using the system libsodium instead of the bundled one

To link the system-installed libsodium rather than the bundled subset, install it first:

    sudo apt install libsodium-dev   # Linux
    brew install libsodium           # macOS

then configure with `-DYOJIMBO_SYSTEM_SODIUM=ON`:

    cmake -B build -DYOJIMBO_SYSTEM_SODIUM=ON
    cmake --build build -j

cheers

 - Glenn
