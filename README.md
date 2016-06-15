## libyojimbo
==========

libyojimbo is a new, easy-to-use software library for creating secure client/server network protocols over UDP.

The library is designed around the networking requirements of realtime multiplayer games such as first person shooters and action games like MOBAs. It is suitable for networking games with 2-64 players that require dedicated servers, rather than servers which are hosted on player's machines or peer-to-peer network topologies.

It's current functionality is to provide provides cryptographically secure authentication, client/server connection management and encryption for packets sent over UDP, as well as a bitpacker and framework for sending and receiving custom game packets.

The library is under active development and is is being extended to provide all the standard functionality expected from a client/server network protocols in the game industry such as time-critical reliable-ordered messages, ability to send large blocks of data quickly and reliably over UDP while staying under MTU, packet aggregation, packet fragmentation and reassembly, 
## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
