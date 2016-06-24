Building libyojimbo
===================


## Dependencies 

To build libyojimbo from source you need [premake 5](https://premake.github.io/download.html).

If you are building under MacOSX or Linux, you also need [libsodium](https://github.com/jedisct1/libsodium).

On MacOSX this is most easily done via "brew install libsodium". If you don't have Brew, you can install it from <http://brew.sh>

On Linux, depending on your particular distribution there may be prebuilt packages for libsodium, or you may need to build it from source. 

Windows users don't need to install libsodium because libyojimbo includes prebuilt sodium libraries.


## Building on Windows

If you don't have Visual Studio 2015 you can [download the community edition for free](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx)

Once you have Visual Studio 2015 and premake5 installed, go to the command line under the libyojimbo directory and type:

    premake5 vs2015

This creates Yojimbo.sln and a bunch of project files. Double click Yojimbo.sln to open it in Visual Studio.

Now you can build the library and run individual test programs as you would for any other visual studio solution.


## Building on MacOSX and Linux

Go to the command line under libyojimbo directory and type

    premake5 gmake

This builds makefiles that you can use to build the source via "make all", or if you prefer via premake with shortcuts:

    premake5 test           // build and run unit tests

    premake5 info           // build and run network info (prints list of local network interface IP addresses)

    premake5 cs             // build and run the local client/server testbed with secure connect tokens

    premake5 server         // build run your own yojimbo server on localhost on UDP port 50000

    premake5 client         // build and run the client that connects to the server running on localhost (127.0.0.1:50000)

ps. To make this more convenient I like to alias "pm" to "premake5" in bash, so building and running test programs becomes:

    pm test

    pm client

Which is much less typing. I run these programs a lot.


## Run a libyojimbo server inside Docker

libyojimbo supports Docker on Windows, Mac and Linux.

First install the latest version of Docker from <http://www.docker.com>

Then the command line at the yojimbo directory, type:

    premake5 docker

This builds and runs a Docker container with a yojimbo server inside it (the same as if you ran "premake5 server" on a Linux box).

You can then connect to this server by running a client which connects to 127.0.0.0:5000 (default behavior). For example, "premake5 client" on Mac or Linux, or running the "client" project inside Yojimbo.sln in Visual Studio.

This takes a long time initially, because the docker action has a lot of steps:

1. Start a new docker image from a prebuilt Ubuntu derived base linux

2. apt-get update, apt-get install wget, g++

3. Download, build and install premake5

4. Download, build and install libsodium

5. Build release version of libyojimbo, run tests

6. If all tests pass, clean everything and copy the libyojimbo server to /home dir

7. When the Docker container is run, start the yojimbo server on UDP port 50000.

For details see docker/Dockerfile and the premake5.lua file with commands that build and run the container instance.

What's most impressive is that if no dependencies have changed, the numbered steps above are precached as intermediate
Docker instances and not rebuilt, eg. if you have already downloaded and installed wget, g++, libsodium and premake5
and you run premake5 docker again, these steps are skipped and the cached docker images for these steps are reused.

Try it yourself by running "premake5 docker" once (it should build everything), and run it again, it will go straight
to the server running on port 50000. Similarly, if you change some libyojimbo source it will automatically rebuild
libyojimbo server and run tests before starting the server. Impressive!

ps. If you are running on windows it is necessary to check "Expose container ports on localhost" in Docker Network settings in order to get the client connecting to 127.0.0.1:50000


## Feedback

This is pre-release software so please email me with any feedback you have <glenn.fiedler@gmail.com>

Thanks!

 - Glenn
