Installing yojimbo
==================

The Yojimbo library can be installed in several ways,
depending on your needs and preferences.
Below are the instructions for installing yojimbo using package managers.

## Installing from source

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    cmake --install build --prefix /usr/local

This is the **bundled** install and it is self-contained: `libyojimbo` carries the netcode,
reliable, tlsf and libsodium objects, and the `serialize.h`, `netcode.h`, `reliable.h`,
`sodium.h` and `tlsf.h` headers are installed beside yojimbo's own. Nothing else is needed on
the system. Note that it owns those header names in the prefix — if you also install the
netcode, reliable or serialize packages there, use the package-manager build below instead.

Consume it with CMake:

    find_package(Yojimbo REQUIRED)
    target_link_libraries(myapp PRIVATE Yojimbo::yojimbo)

`tools/consumer` is a complete working example of exactly that, and CI builds and runs it
against a clean prefix for both installs.

## Installing for a package manager

    cmake -B build -DCMAKE_BUILD_TYPE=Release -DYOJIMBO_SYSTEM_DEPS=ON
    cmake --build build -j
    cmake --install build --prefix /usr/local

Here serialize, reliable and netcode come from their own packages: their headers are not
installed, and `YojimboConfig.cmake` requires them instead, checking the installed versions
against the floors in `dependencies.manifest` — currently netcode 1.4.0 or later (see
SECURITY.md), reliable 1.4.0 or later and serialize 1.15.0 or later. A prefix holding a netcode
inside a published advisory range fails at configure time rather than at runtime.

`-DYOJIMBO_SYSTEM_SODIUM=ON` links the system libsodium instead of the bundled subset, and
`-DYOJIMBO_SYSTEM_TLSF=ON` the system tlsf. `YojimboConfig.cmake` records which of these the
install was built with and finds them again for the consumer.

## Installing on Debian and Ubuntu (apt)

Packages for Debian 12/13 and Ubuntu 22.04/24.04/26.04 (amd64 and arm64) are
published in the [mas-bandwidth apt repository](https://github.com/mas-bandwidth/apt).
One-time repository setup:

    sudo install -d /etc/apt/keyrings
    sudo curl -fsSL https://mas-bandwidth.github.io/apt/mas-bandwidth-apt.asc -o /etc/apt/keyrings/mas-bandwidth-apt.asc
    echo "deb [signed-by=/etc/apt/keyrings/mas-bandwidth-apt.asc] https://mas-bandwidth.github.io/apt $(. /etc/os-release && echo $VERSION_CODENAME) main" | sudo tee /etc/apt/sources.list.d/mas-bandwidth.list
    sudo apt update

Then:

    sudo apt install yojimbo

This installs libyojimbo-dev and its dependencies (libnetcode-dev,
libreliable-dev, libserialize-dev and the distribution's libsodium). The
packaged libraries are release builds; link with something like:

    g++ -DNDEBUG -O2 -o game game.cpp -lyojimbo -lnetcode -lreliable -lsodium -lpthread -lm

or, with CMake, `find_package(Yojimbo REQUIRED)` and link `Yojimbo::yojimbo`.

## Installing via Conan

yojimbo is available on [Conan Center](https://conan.io/center/recipes/yojimbo).

First, install the dependencies:

    conan install --requires="yojimbo/[*]" --build=missing

The yojimbo package in Conan Center is maintained by the ConanCenterIndex community.
To report an outdated version or a packaging issue, please open an issue at https://github.com/conan-io/conan-center-index.
