# Roadmap

## 0.1.0 - Initial release

The goal of this release was to take the source code written for [Building a Game Network Protocol](http://gafferongames.com/building-a-game-network-protocol/) and harden it so that it is unit tested, secure and ready to be used as a library.

Features implemented for this release:

1. Client/server connection management and timeouts
2. Secure client connect with connect token
3. Insecure connect for use in development (connection to server IP without authentication or encryption)
4. Packet encryption (requires connection via connect token)
5. Packet serialization with bitpacker
6. Can extend protocol to add new packet types
7. Unit tests for all of the functionality above
8. Integrated client/server test program that demonstrates secure connect with tokens
9. Separate client/server test programs that connect insecure connection (dev mode)
10. Network simulator support with latency, packet loss, jitter and duplicate packets.
11. Socket network interface (send packets over UDP)

## 0.2.0 - Docker integration

The goal of this release is to extend libyojimbo to support a web backend for clients who need to securely connect to dedicated servers with "connect tokens". The connect token and packet encryption feature has already been implemented in 0.1.0, but it is necessary to setup a backend matchmaker and dedicated server infrastructure to show how this would be used in production. 

Docker is being used in this release as an example, not to tie libyojimbo to any particular architecture, but because it is a really convenient way to distribute and run a web backend across Mac, Windows and Linux. It's super convenient for this purpose.

So far this release has added support for running a libyojimbo server inside a container "premake5 docker", as well as a simple matcher web service in golang: "premake5 matcher".

This release is very close to being finished, just as soon as the connect example hits the matcher service and uses the match response JSON to securely connect to the running server, I'm calling this release done.

## 0.3.0 - Reliable messages

Now that a demonstration of backend infrastructure is in place, the focus for this release returns to the network protocol. The goal with this release is to get the internal network protocol to a stable API and feature set.

This release will extend the internal UDP-based network protocol to support: 

1. Reliable-ordered time critical messages and data blocks
2. Packet fragmentation and reassembly
3. Packet aggregation
4. Support for compressed packets and data blocks

## 0.4.0 - Matchmaker

In this release I will extend matcher.go to support multiple dedicated servers reporting to it via HTTPS.

Then I will and setup a docker swarm of libyojimbo servers, all reporting to the web backend, and now clients can connect to this swarm of instances.

The matcher will then track the set of all servers registered with it and satisfy client requests to join matches by directing clients towards servers with empty slots, sorting servers in the order of servers with the least free slots (1) to the most free slots (maxClients)

I may even setup a swarm of fake clients as well for stress test behavior, this could be very interesting. As a stretch goal, it would be cool to implement a web backend showing the behavior of clients and servers in this simulated system so you can see everything that is going on.

## 0.5.0 - Initial public release

The estimated time for initial public release is November 2016. At this time I intend to get more people using this library and build a community to determine which features should be implemented next.

One possibility is to continue to extend this system to support entity synchronization, client-side prediction as per-FPS networking models, although it is quite likely that I would create a *new* network library build on top libyojimbo to implement that aspect, so as to not lock people using libyojimbo into one particular network model.

Your feedback is most welcome! 

Please let me know if there are any features missing from the libyojimbo roadmap that would benefit your team.

cheers

- Glenn
