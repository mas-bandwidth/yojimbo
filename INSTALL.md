Installing yojimbo
==================

The Yojimbo library can be installed in several ways,
depending on your needs and preferences.
Below are the instructions for installing yojimbo using package managers.

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

## Installing via Conan

yojimbo is available on [Conan Center](https://conan.io/center/recipes/yojimbo).

First, install the dependencies:

    conan install --requires="yojimbo/[*]" --build=missing

The yojimbo package in Conan Center is maintained by the ConanCenterIndex community.
To report an outdated version or a packaging issue, please open an issue at https://github.com/conan-io/conan-center-index.
