Building libyojimbo
===================

## Building on Windows

Download [premake 5](https://premake.github.io/download.html) and copy the **premake5** executable somewhere in your path.

If you don't have Visual Studio 2015 you can [download the community edition for free](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx).

Go to the command line under the libyojimbo directory and type:

    premake5 vs2015

This creates Yojimbo.sln and a bunch of project files. Double click Yojimbo.sln to open it in Visual Studio.

Now you can build the library and run individual test programs as you would for any other visual studio solution.

## Building on MacOSX and Linux

Download [premake 5](https://premake.github.io/download.html) then build and install from source.

Next install libsodium and mbedtls.

On MacOS X, this can be done "brew install libsodium mbedtls". If you don't have Brew, you can install it from <http://brew.sh>.

On Linux, depending on your particular distribution there may be prebuilt packages for libsodium and mbedtls, or you may have to build from source from here: [libsodium](https://github.com/jedisct1/libsodium/releases) and here [mbedtls](https://github.com/ARMmbed/mbedtls). Make sure you install the 2.x version of mbedtls. The 1.x version will not work with libyojimbo.

Next go to the command line under the libyojimbo directory and enter:

    premake5 gmake

This creates makefiles which you can use to build the source via "make all", or if you prefer, via the following shortcuts:

    premake5 test           // build and run unit tests

    premake5 info           // build and run network info (prints list of local network interface IP addresses)

    premake5 cs             // build and run the local client/server testbed with secure connect tokens

    premake5 server         // build run your own yojimbo server on localhost on UDP port 50000

    premake5 client         // build and run the client that connects to the server running on localhost

To make this more convenient I like to alias "pm" to "premake5" in bash, so building and running test programs becomes:

    pm test

    pm client

And so on, which is much less typing (I run these shortcuts a lot!)

## Run a yojimbo server inside Docker

**libyojimbo** supports Docker on Windows, Mac and Linux.

First, install the latest version of Docker from <http://www.docker.com>

Once Docker is installed, go to the command line at the yojimbo directory and enter:

    premake5 docker

This builds and runs a Docker container with a yojimbo server inside it (exactly the same as if you ran "premake5 server" on a Linux box). You can now connect to this server by running a client which connects to 127.0.0.0:50000. For example, "premake5 client" on Mac or Linux, or running the "client" project inside the Yojimbo.sln in Visual Studio.

IMPORTANT: The premake docker action takes a long time initially, because it has a lot of work to do:

1. Start a new docker image from an [ubuntu derived base image](https://github.com/phusion/baseimage-docker)

2. apt-get update, apt-get install wget, g++

3. Download, build and install premake5

4. Download, build and install libsodium and libucl

5. Build release version of libyojimbo, run tests

6. If all tests pass, clean everything and copy the libyojimbo server to the /home dir

7. When the Docker container is run, start the yojimbo server /home/server on UDP port 50000.

For details see docker/Dockerfile and the premake5.lua file with commands that build and run the container instance.

What's most impressive is that if no dependencies have changed, the numbered steps above are precached as intermediate Docker instances are not rebuilt unless necessary. For example, if you have already downloaded and installed wget, g++, libsodium and premake5 and you run "premake5 docker" again, these steps are skipped.

Try it yourself by running "premake5 docker" once (it should build everything), then run it again. It will go straight to the server running on port 50000. Similarly, if you change some libyojimbo source it automatically rebuilds libyojimbo server and runs tests before starting the server. Impressive!

## Run a yojimbo matcher inside Docker

In order to support authentication and a secure connection between client and server, yojimbo provides a backend written in golang.

The purpose of this backend is to take a client request for connection and provide a connection token which the client uses to securely connect to a dedicated server.

To run this matchmaker backend, run this command:

    premake5 matcher

This builds and runs a linux docker instance with matcher.go running on port 8080.

You can verify the matcher instance is working correctly as follows:

    curl https://localhost:8080/match/12345/1 --insecure

Where 12345 is the protocolId and 1 is the client id. This should return the match response in JSON which looks like this:

    {"connectToken":"y2R7Sqej9HFg7Y3sBqr9XbK6tyMicrmk13TLsGksxAqniu5LaY89AKhgKDGlQ/mIpxukwFdDwPBtMa5KRPpFUlIds2dD+gNGOjH636Vxh5Svb1Ul8AzajQoiamC1w2TN/qAQjW2+bFp/k6ifDKoEwcchHSlqbgzzIxctgr1iODJADyMb7YdHq7TCWApxeWAIrWnHfapTD1uVJU0oDjAqak/QtSLG6GAMCi9Qxgd66aQlK+V/2nm7bMQ00ubXZC8mSUI5xRssNeoFQsZF68rJcxsZgEIumLg16NOkZiX/K7DGbHR2If+1hqNUJqf7S8N30mNoI1yXMKNSlfiqts7Eze40NjVxiYzseibPfZt6uOXGgkeEFjQeTs9xMB78BzpOn5Z4I6JpqC/1TKewW5S4LpK8mWz3w8z7Px6leo4g0SqOxo/0Beqim6JoelMJe0nan2T+XKkX5GeaTrqnkVzKyDda+RD+22KLyMD3vda89RCq3KJUhpbyBAC7CHUrHGxj9jufzLebaVrsQN+KaZlKJytMbRee3G+NGjuilXiHbAJzY101fqDfGL9vNMgPNQiaiWDbbIdXyGp+H3yhzCnUbALOeHPDhhFnXUyB+XwBjqq/w2JbNIXei3AN+vydjmhSKav7SnSFO8MpMlOv/0ztI/eJPR7CUx/XEFPlfr6EK5UhUWKoPcrUs5pLOohqByckvrkrasfhftAHcr82kQiYo1jKFiKX8fE1/dspdXirkWd26aoT8RQFWIs48z/Rmvv609oaHlQGo3pBj6ogjU82aMyCPGoao2QYRdd2HTjWqBevkDn+O6/YFeJLFkonaa5GsQjpJfi+CrFdJTNldMcLfB7mAuq9tIwhKT3ivHEYsZvnlV45NtZuwXhjLEms2z/YrTgP0OiVC9s6utRG8loJoj+nzDMfnr567Si3VLXJAHwCZ/tJT+fnJvzGXE+T0gw+WKHz8dKVuRLb16FAvRlBHekbs2dj9///djLvXQPp4dDr1KcXDsSt/nBf5d/wnaU1EvltKv8y6oclu195OTqWSmzeJYH2f+Gn0RP9xVVKxTliKcoMz84/h+IQXD2Qd3DR7dtJfQkoZRB/zCEoRHIXLTA6N0BDxqpsG916Fg7fC4c5GDDvmNU0NZV0Vwz2E1ydXsDuS9Q1vxIpuQbqlhfckjRuuHOY4dlQfDOqTEPoSQxIJhUlhsPr4zImhbvhUfqdCZKonjQura62BdJHEE/aTF8KavvNgm5JEyRz8H3b2Aqrjuzj6tQJYpe8TZ3eMspGd7o/S5mu5JS/WcPhlQYiruzSNTuASwPwCxWjHzwNQwyDhM7ZjkHOrEYCp3uj9RYAIah7R7w4gwxeAoQIjkEKFA==","connectNonce":"1","serverAddresses":["MTI3LjAuMC4xOjUwMDAw"],"clientToServerKey":"8oRGFHDQo+Z38lhyKQ+1OjDDlFwjg6o1HkeR+fgw4z0=","serverToClientKey":"D0g58nVdAx0jmT+duZwcWdbHUuFBKWGyj2SZ8g+Ak0o="}

Next, you can run the "connect" program to HTTPS to the matcher directly from C++:

    premake5 connect

Or if you are building under Visual Studio, run the "connect" project from the IDE.

The connect program requests a match from the macher over HTTPS, parses this JSON, extracts the data and uses it to securely connect to the server with encrypted traffic. This is secure because the matcher and the dedicated server instances share a private key that the client does not know, so the client can receive the encrypted connect token, but cannot read its contents or create connect tokens to pass to servers.

This with libyojimbo, you can deploy dedicated server instances that integrate with your own matchmaker backend by passing connect tokens.

If a client doesn't have a connect token, a client **cannot connect to your dedicated servers**.

## Feedback

This is pre-release software so please email me with any feedback you have <glenn.fiedler@gmail.com>

Thanks!

 - Glenn
