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

The goal of this release is to extend libyojimbo to support a web backend for clients who want to connect to servers securely with "connect tokens". The connect token and packet encryption feature has already been implemented in 0.1.0, but it is necessary to setup a backend matchmaker and dedicated server infrastruture to show how this would be used in production. 

This release will use Docker to demonstrate how to do setup and run such an example backend. The point is not to tie usage of libyojimbo to Docker based environments. You will be able to use it with whatever backend you wish. Docker is a means to an end.

So far this release has implemented support for running a libyojimbo server inside a container.

I am currently working on extending libyojimbo connect tokens to be based around JSON so that they can be generated from other web languages such as JavaScript, Go, Python, Ruby and so on. Once this is done, I will implement a simple web backend, most likely in Go that provides clients with connect tokens on request. On receiving these tokens, clients will be able to securely connect to dedicated server instances running in a Docker Swarm with encrypted packets. There will also be some communication from dedicated server instances back to the matchmaker over HTTPS, so the matchmaker is aware of the set of servers available for clients to connect to and which servers have free slots to join.

## 0.3.0 - Reliable messages

Now that a demonstration of backend infrastructure and matchmaking is in place, the focus for this release returns to the network protocol. The goal with this release is to get the internal network protocol to a stable API and feature set.

This release will extend the internal UDP-based network protocol to support: 

1. Reliable-ordered time critical messages and data blocks
2. Packet fragmentation and reassembly
3. Packet aggregation
4. Support for compressed packets and data blocks

Once this feature set stabilizes libyojimbo will be ready for public release.

## 0.4.0 - Initial public release

The estimated time for initial public release is early November 2016. At this time I intend to get more people using this library and build a community to determine which features should be implemented next.

One possibility is to continue to extend this system to support entity synchronization, client-side prediction as per-FPS networking models, although it is quite likely that I would create a *new* network library build on top libyojimbo to implement that aspect, so as to not lock people using libyojimbo into one particular network model.

Your feedback is most welcome! 

Please let me know if there are any features missing from the libyojimbo roadmap that would benefit your team.

cheers

- Glenn
