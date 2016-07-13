Building libyojimbo
===================

## Building on Windows

Download [premake 5](https://premake.github.io/download.html) and copy the **premake5** executable somewhere in your path.

If you don't have Visual Studio 2015 you can [download the community edition for free](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx).

Go to the command line under the libyojimbo directory and type:

    premake5 solution

This creates Yojimbo.sln and a bunch of project files then opens them in Visual Studio for you.

Now you can build the library and run individual test programs as you would for any other visual studio solution.

## Building on MacOSX and Linux

Download [premake 5](https://premake.github.io/download.html) then build and install from source.

Next install libsodium and mbedtls.

On MacOS X, this can be done "brew install libsodium mbedtls". If you don't have Brew, you can install it from <http://brew.sh>.

On Linux, depending on your particular distribution there may be prebuilt packages for libsodium and mbedtls, or you may have to build from source from here [libsodium](https://github.com/jedisct1/libsodium/releases) and here [mbedtls](https://github.com/ARMmbed/mbedtls). Make sure you install the 2.x version of mbedtls. The 1.x version will not work with libyojimbo.

Next go to the command line under the libyojimbo directory and enter:

    premake5 gmake

This creates makefiles which you can use to build the source via "make all", or if you prefer, via the following shortcuts:

    premake5 test           // build and run unit tests

    premake5 info           // build and run network info (prints list of local network interface IP addresses)

    premake5 cs             // build and run the local client/server testbed with secure connect tokens

    premake5 server         // build run your own yojimbo server on localhost on UDP port 50000

    premake5 client         // build and run the client that connects to the server running on localhost

## Run a yojimbo server inside Docker

**libyojimbo** supports Docker on Windows, Mac and Linux.

First, install the latest version of Docker from <http://www.docker.com>

If you are running on Windows go into Docker network settings and enable "Expose container ports on localhost".

Now go to the command line at the yojimbo directory and enter:

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

This should return the match response in JSON which looks something like this:

    {"connectToken":"y2R7Sqej9HFg7Y3sBqr9XbK6tyMicrmk13TLsGksxAqniu5LaY89AKhgKDGlQ/mIpxukwFdDwPBtMa5KRPpFUlIds2dD+gNGOjH636Vxh5Svb1Ul8AzajQoiamC1w2TN/qAQjW2+bFp/k6ifDKoEwcchHSlqbgzzIxctgr1iODJADyMb7YdHq7TCWApxeWAIrWnHfapTD1uVJU0oDjAqak/QtSLG6GAMCi9Qxgd66aQlK+V/2nm7bMQ00ubXZC8mSUI5xRssNeoFQsZF68rJcxsZgEIumLg16NOkZiX/K7DGbHR2If+1hqNUJqf7S8N30mNoI1yXMKNSlfiqts7Eze40NjVxiYzseibPfZt6uOXGgkeEFjQeTs9xMB78BzpOn5Z4I6JpqC/1TKewW5S4LpK8mWz3w8z7Px6leo4g0SqOxo/0Beqim6JoelMJe0nan2T+XKkX5GeaTrqnkVzKyDda+RD+22KLyMD3vda89RCq3KJUhpbyBAC7CHUrHGxj9jufzLebaVrsQN+KaZlKJytMbRee3G+NGjuilXiHbAJzY101fqDfGL9vNMgPNQiaiWDbbIdXyGp+H3yhzCnUbALOeHPDhhFnXUyB+XwBjqq/w2JbNIXei3AN+vydjmhSKav7SnSFO8MpMlOv/0ztI/eJPR7CUx/XEFPlfr6EK5UhUWKoPcrUs5pLOohqByckvrkrasfhftAHcr82kQiYo1jKFiKX8fE1/dspdXirkWd26aoT8RQFWIs48z/Rmvv609oaHlQGo3pBj6ogjU82aMyCPGoao2QYRdd2HTjWqBevkDn+O6/YFeJLFkonaa5GsQjpJfi+CrFdJTNldMcLfB7mAuq9tIwhKT3ivHEYsZvnlV45NtZuwXhjLEms2z/YrTgP0OiVC9s6utRG8loJoj+nzDMfnr567Si3VLXJAHwCZ/tJT+fnJvzGXE+T0gw+WKHz8dKVuRLb16FAvRlBHekbs2dj9///djLvXQPp4dDr1KcXDsSt/nBf5d/wnaU1EvltKv8y6oclu195OTqWSmzeJYH2f+Gn0RP9xVVKxTliKcoMz84/h+IQXD2Qd3DR7dtJfQkoZRB/zCEoRHIXLTA6N0BDxqpsG916Fg7fC4c5GDDvmNU0NZV0Vwz2E1ydXsDuS9Q1vxIpuQbqlhfckjRuuHOY4dlQfDOqTEPoSQxIJhUlhsPr4zImhbvhUfqdCZKonjQura62BdJHEE/aTF8KavvNgm5JEyRz8H3b2Aqrjuzj6tQJYpe8TZ3eMspGd7o/S5mu5JS/WcPhlQYiruzSNTuASwPwCxWjHzwNQwyDhM7ZjkHOrEYCp3uj9RYAIah7R7w4gwxeAoQIjkEKFA==","connectNonce":"1","serverAddresses":["MTI3LjAuMC4xOjUwMDAw"],"clientToServerKey":"8oRGFHDQo+Z38lhyKQ+1OjDDlFwjg6o1HkeR+fgw4z0=","serverToClientKey":"D0g58nVdAx0jmT+duZwcWdbHUuFBKWGyj2SZ8g+Ak0o="}

## Run the connect program

If you have both a matcher and server running you can run the "connect" program to connect a client to that server through the matchmaker.

On MacOSX or Linux, run:

    premake5 connect

If you are building under Visual Studio, run the "connect" project from the IDE.

## Running a secure server

Now lets switch over to running secure servers. You can run a secure server like this:

    premake5 secure_server

If you are building under Visual Studio, run the "secure_server" project from the IDE.

Now that the secure server is running, you will see that connecting via "connect" works, but regular connects via "client" do not.

This is the entire point of secure servers. **Secure servers only allow connections that come from the matcher.**

This is accomplished via an encrypted connect token that the matchmaker generates and passes back to the client. This connect token is valid only for a particular globally unique 64bit client id (of your choice), for a limited period of time, and for a limited whitelist of server addresses. 

Connect tokens cannot be decrypted or forged by clients because they are encrypted and signed using a shared private key known only to the matcher and the dedicated server instances. This is why libyojimbo is designed only for games that host dedicated servers. The private key must be known only to the matcher and the dedicated server instances or the security model breaks down.

## Reliable Messages and Blocks

The latest preview release of yojimbo has support for reliable-ordered messages and blocks.

Messages can be sent in both directions. Client to server **and** server to client. Messages are reliable, ordered and time critical. They are resent rapidly until acked.

Blocks of data larger than MTU may be sent in the same reliable ordered stream. They are sliced up and reassembled on the other side and injected into the same reliable ordered stream of messages. This is very convenient for example when you need to send down a large block of initial state (eg. initial delta baseline, or state of the world) on client connect. 

For more details see soak.cpp.

To see the blocks and messages in action, run:

    premake5 soak

This soak program looks forever sending messages and reliable blocks over very bad network conditions.

The soak test is self validating. As long as it keeps receiving messages it's working. CTRL-C to stop.

## Profiling

There is now a profiling testbed for libyojimbo. 

This testbed connects local 64 clients to a secure server in the same process and exchanges packets, messages and blocks continuously between them. Packets are still encrypted and sent over sockets so performance is representative of real world usage of the library.

The good news: The performance is very good, and after fixing some minor hotspots (see CHANGES.md) the total cost of libyojimbo library is smaller than the time it takes to call sendto/recvfrom, and much, much less than the time spent encrypting and decrypting packets inside libsodium. 

This means that the actual performance cost of libyojimbo connection management and serializing packets is neglible.

If you'd like to look at the profile results yourself, you'll need a copy of Visual Studio 2015 (Community edition is fine). 

First it is necessary to make some modifications to premake5.lua so you have debug symbols in release build for the profiler:

In the solution "Yojimbo" section near the top of premake5, add symbols to the release configuration:

    configuration "Release"
        flags { "Symbols" }             --  add this line!
        optimize "Speed"
        defines { "NDEBUG" }

Now run "premake5 solution", switch to "Release" configuration and rebuild all. 

Right click on "profile" project an set it as the startup project. 

Press ALT-F2, check "CPU Usage" and click the "Start" to begin profiling.

After about minute you should have enough samples. Close the profile process and view the results. Enjoy.

## Feedback

This is pre-release software so please email me with any feedback you have <glenn.fiedler@gmail.com>

Thanks!

 - Glenn
