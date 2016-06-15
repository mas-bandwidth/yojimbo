## libyojimbo

libyojimbo is a new, easy-to-use library for creating secure client/server network protocols over UDP.

It is designed around the networking requirements of realtime multiplayer games such as first person shooters and MOBAs and is suitable for networking games with 2-64 players. It requires games to host servers in the cloud or in data centers. It does not support player hosted servers or peer-to-peer networking.

The library is under active development and is is being extended to provide all the standard functionality expected from a professional grade network protocol in the game industry such as: time-critical reliable-ordered messages, sending large blocks of data quickly and reliably over UDP, compressed packets, packet aggregation, packet fragmentation and reassembly.

The library is currently in pre-release and provides support for cryptographically secure authentication, client/server connection management and encryption for packets sent over UDP, as well as a bitpacker and framework for extending the protocol with custom packet types.

## Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler), a recognized expert in the field of game network programming with almost 20 years experience working in the game industry. His credits include: Freedom Force, SWAT 4, L.A. Noire, Mercenaries 2, God of War 3, God of War: Ascension, Playstation: All Stars, Journey and Titanfall.

Glenn is currently writing an article series about the development of this library: [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/).

You can support Glenn's work writing articles and open source code on [Patreon](http://www.patreon.com/gafferongames).

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
