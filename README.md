## libyojimbo

libyojimbo is a new, easy-to-use software library for creating secure client/server network protocols over UDP.

It is designed around the networking requirements of realtime multiplayer games such as first person shooters and MOBAs and is suitable for networking games with 2-64 players. It requires game developers to host servers in the cloud or in data centers. It does not support player hosted servers or peer-to-peer networking and does not provide NAT punchthrough.

The library is under active development and is is being extended to provide all the standard functionality expected from a client/server network protocols in the game industry such as: time-critical reliable-ordered messages, ability to send large blocks of data quickly and reliably over UDP while staying under MTU, packet aggregation, packet fragmentation and reassembly.

The library is currently in pre-release and provides cryptographically secure authentication, connection management and encryption for packets sent over UDP, as well as a bitpacker and framework for sending and receiving custom packet types.

## Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler), a recognized expert in the field of game network programming with 15 years experience working in the game industry as a network programmer. His credits include: Freedom Force, SWAT 4, L.A. Noire, Mercenaries 2, God of War 3, God of War: Ascension, Playstation: All Stars, Journey and Titanfall.

Glenn is currently writing an article series on [gafferongames.com](http://gafferongames.com) about the development of this library called [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/).

You can support Glenn's work writing articles and open source code on [Patreon](http://www.patreon.com/gafferongames).

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
