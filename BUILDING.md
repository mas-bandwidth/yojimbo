Building libyojimbo
===================

## Building on Windows

Download [premake 5](https://premake.github.io/download.html) and copy the **premake5** executable somewhere in your path. Please make sure you have at least premake5 alpha 10.

If you don't have Visual Studio 2015 you can [download the community edition for free](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx).

Go to the command line under the libyojimbo directory and type:

    premake5 solution

This creates Yojimbo.sln and a bunch of project files and opens them in Visual Studio for you.

Now you can build the library and run individual test programs as you would for any other Visual Studio solution.

## Building on MacOS and Linux

First, download and install [premake 5](https://premake.github.io/download.html).

Next, install libsodium and mbedtls.

On MacOS X, this can be done most easily with `brew install libsodium mbedtls`. If you don't have Brew, you can install it from <http://brew.sh>.

On Linux, depending on your particular distribution there may be prebuilt packages for libsodium and mbedtls, or you may have to build from source from here [libsodium](https://github.com/jedisct1/libsodium/releases) and here [mbedtls](https://github.com/ARMmbed/mbedtls). Make sure you install the 2.x version of mbedtls as the 1.x version will not work with libyojimbo.

Now go to the command line under the libyojimbo directory and enter:

    premake5 gmake

Which creates makefiles which you can use to build the source via:

    make all

Alternatively, you can use the following shortcuts to build and run test programs directly:

    premake5 test           // build and run unit tests

    premake5 server         // build run a yojimbo server on localhost on UDP port 40000

    premake5 client         // build and run a yojimbo client that connects to the server running on localhost 
   
## Run a yojimbo server inside Docker

**libyojimbo** supports Docker on Windows, Mac and Linux.

First, install the latest version of Docker from <http://www.docker.com>

Now go to the command line at the yojimbo directory and enter:

    premake5 docker

This builds and runs a Docker container with a yojimbo server inside it (exactly the same as if you ran "premake5 server" on a Linux box). You can now connect to this server by running a client which connects to 127.0.0.0:40000. For example, "premake5 client" on Mac or Linux, or running the "client" project inside the Yojimbo.sln in Visual Studio.

IMPORTANT: The premake docker action takes a long time initially, because it has a lot of work to do:

1. Start a new docker image from an [ubuntu derived base image](https://github.com/phusion/baseimage-docker)

2. `apt-get update, apt-get install wget, g++`

3. Download, build and install premake5

4. Download, build and install libsodium

5. Build release version of libyojimbo, run tests

6. If all tests pass, clean everything and copy the libyojimbo server to the /home dir

7. When the Docker container is run, start the yojimbo server /home/server on UDP port 40000.

For details see docker/Dockerfile and the premake5.lua file with commands that build and run the container instance.

What's most impressive is that if no dependencies have changed, the numbered steps above are precached as intermediate Docker instances are not rebuilt unless necessary. For example, if you have already downloaded and installed wget, g++, libsodium and premake5 and you run "premake5 docker" again, these steps are skipped.

Try it yourself by running "premake5 docker" once (it should build everything), then run it again. It will go straight to the server running on port 40000. Similarly, if you change some libyojimbo source it automatically rebuilds libyojimbo server and runs tests before starting the server. Impressive!

## Run a yojimbo matcher inside Docker

In order to demonstrate authentication and a secure connection between client and server, yojimbo provides an example backend written in golang.

You can run this backend in Docker via this command:

    premake5 matcher

This builds and runs a linux docker instance with matcher.go running on port 8080.

You can verify the matcher instance is working correctly as follows:

    curl https://localhost:8080/match/12345/1 --insecure

Which should return a match response in JSON that looks something like this:

    {"connectToken":"y2R7Sqej9HFg7Y3sBqr9XbK6tyMicrmk13TLsGksxAqniu5LaY89AKhgKDGlQ/mIpxukwFdDwPBtMa5KRPpFUlIds2dD+gNGOjH636Vxh5Svb1Ul8AzajQoiamC1w2TN/qAQjW2+bFp/k6ifDKoEwcchHSlqbgzzIxctgr1iODJADyMb7YdHq7TCWApxeWAIrWnHfapTD1uVJU0oDjAqak/QtSLG6GAMCi9Qxgd66aQlK+V/2nm7bMQ00ubXZC8mSUI5xRssNeoFQsZF68rJcxsZgEIumLg16NOkZiX/K7DGbHR2If+1hqNUJqf7S8N30mNoI1yXMKNSlfiqts7Eze40NjVxiYzseibPfZt6uOXGgkeEFjQeTs9xMB78BzpOn5Z4I6JpqC/1TKewW5S4LpK8mWz3w8z7Px6leo4g0SqOxo/0Beqim6JoelMJe0nan2T+XKkX5GeaTrqnkVzKyDda+RD+22KLyMD3vda89RCq3KJUhpbyBAC7CHUrHGxj9jufzLebaVrsQN+KaZlKJytMbRee3G+NGjuilXiHbAJzY101fqDfGL9vNMgPNQiaiWDbbIdXyGp+H3yhzCnUbALOeHPDhhFnXUyB+XwBjqq/w2JbNIXei3AN+vydjmhSKav7SnSFO8MpMlOv/0ztI/eJPR7CUx/XEFPlfr6EK5UhUWKoPcrUs5pLOohqByckvrkrasfhftAHcr82kQiYo1jKFiKX8fE1/dspdXirkWd26aoT8RQFWIs48z/Rmvv609oaHlQGo3pBj6ogjU82aMyCPGoao2QYRdd2HTjWqBevkDn+O6/YFeJLFkonaa5GsQjpJfi+CrFdJTNldMcLfB7mAuq9tIwhKT3ivHEYsZvnlV45NtZuwXhjLEms2z/YrTgP0OiVC9s6utRG8loJoj+nzDMfnr567Si3VLXJAHwCZ/tJT+fnJvzGXE+T0gw+WKHz8dKVuRLb16FAvRlBHekbs2dj9///djLvXQPp4dDr1KcXDsSt/nBf5d/wnaU1EvltKv8y6oclu195OTqWSmzeJYH2f+Gn0RP9xVVKxTliKcoMz84/h+IQXD2Qd3DR7dtJfQkoZRB/zCEoRHIXLTA6N0BDxqpsG916Fg7fC4c5GDDvmNU0NZV0Vwz2E1ydXsDuS9Q1vxIpuQbqlhfckjRuuHOY4dlQfDOqTEPoSQxIJhUlhsPr4zImhbvhUfqdCZKonjQura62BdJHEE/aTF8KavvNgm5JEyRz8H3b2Aqrjuzj6tQJYpe8TZ3eMspGd7o/S5mu5JS/WcPhlQYiruzSNTuASwPwCxWjHzwNQwyDhM7ZjkHOrEYCp3uj9RYAIah7R7w4gwxeAoQIjkEKFA==","connectNonce":"1","serverAddresses":["MTI3LjAuMC4xOjUwMDAw"],"clientToServerKey":"8oRGFHDQo+Z38lhyKQ+1OjDDlFwjg6o1HkeR+fgw4z0=","serverToClientKey":"D0g58nVdAx0jmT+duZwcWdbHUuFBKWGyj2SZ8g+Ak0o="}

## Connect a secure client

You can run the "secure_client" program to securely connect a client to that server through the matcher. 

Connect this way requires having the matcher running, eg. "premake5 matcher"

The benefit of secure connect is that it enables packet encryption between the client and server.

On MacOS or Linux, run:

    premake5 secure_client

If everything is working correctly you should see something like:

    client id is cbfbce8542016791
    requesting match from https://localhost:8080
    received match response
    client started on port 59160
    client connecting to 127.0.0.1:40000
    client changed state from 'disconnected' to 'sending connection request'
    client changed state from 'sending connection request' to 'sending challenge response'
    client changed state from 'sending challenge response' to 'connected'
    client connected as client 0

## Running a secure server

Up to this point we have been running insecure servers with "premake5 server".

Servers running in this way allow either secure or insecure connects. When you go into production with yojimbo, generally you want to disable this and only allow secure connects from clients.

You can run a secure server like this on MacOS and Linux:

    premake5 secure_server

Or, if you are building under Visual Studio, run the "secure_server" project from the IDE.

When a secure server is running, you will notice that connecting via "secure_client" works, but regular (insecure) connects via "client" do not.

This is the entire point of secure servers. **Secure servers only allow connections that come from the matcher.**

This is accomplished via an encrypted connect token that the matchmaker generates and passes back to the client. This connect token is valid only for a particular globally unique 64bit client id (of your choice), for a limited period of time, and for a limited whitelist of server addresses. 

Connect tokens cannot be decrypted or forged by clients because they are encrypted and signed with a private key known only to the matcher and the dedicated server instances. This is why libyojimbo is designed only for games that host dedicated servers. The private key must be known only to the matcher and the dedicated server instances for the security model to work.

## Documentation and Support

libyojimbo now has reference documentation built from code comments with [doxygen](http://www.stack.nl/~dimitri/doxygen/).

To build the documentation first install doxygen on your platform.

Once you have doxygen installed and in y our path, you can build and view the documentation with this command:

    premake5 docs
    
More documentation including usage documentation is coming shortly. 

Until then, if you have questions and you don't find the answer you need in the documentation, please create an issue at http://www.libyojimbo.com and I'll do my best to help you out.

cheers

 - Glenn
