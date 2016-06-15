## libyojimbo

libyojimbo is a new, easy-to-use software library for creating secure client/server network protocols over UDP.

It is designed around the networking requirements of realtime multiplayer games such as first person shooters and action games like MOBAs. It is suitable for networking games with 2-64 players that require dedicated servers. It is not intended for games that host game servers on player's machines or network via peer-to-peer topologies.

It's current functionality is to provide provides cryptographically secure authentication, connection management and encryption for packets sent over UDP, as well as a bitpacker and framework for sending and receiving custom packet types.

The library is under active development and is is being extended to provide all the standard functionality expected from a client/server network protocols in the game industry such as time-critical reliable-ordered messages, ability to send large blocks of data quickly and reliably over UDP while staying under MTU, packet aggregation, packet fragmentation and reassembly.

## Author

The author Glenn Fiedler has over 15 years of experience working in the game industry as a game network programmer and is a recognized expert in the field of game networking. 

He is currently writing an article series [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/) where he describes the development of this network protocol library from scratch.

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
