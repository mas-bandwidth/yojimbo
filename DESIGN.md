Design
======

## Building on Windows

Download [premake 5](https://premake.github.io/download.html) and copy the **premake5** executable somewhere in your path.

If you don't have Visual Studio 2015 you can [download the community edition for free](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx).

Go to the command line under the libyojimbo directory and type:

    premake5 solution

This creates Yojimbo.sln and a bunch of project files then opens them in Visual Studio for you.

Now you can build the library and run individual test programs as you would for any other visual studio solution.

## Building on MacOS and Linux

Download [premake 5](https://premake.github.io/download.html) then build and install from source.

Next install libsodium and mbedtls.

On MacOS X, this can be done "brew install libsodium mbedtls". If you don't have Brew, you can install it from <http://brew.sh>.

On Linux, depending on your particular distribution there may be prebuilt packages for libsodium and mbedtls, or you may have to build from source from here [libsodium](https://github.com/jedisct1/libsodium/releases) and here [mbedtls](https://github.com/ARMmbed/mbedtls). Make sure you install the 2.x version of mbedtls. The 1.x version will not work with libyojimbo.

