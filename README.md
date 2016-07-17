# libyojimbo

**libyojimbo** is a new, easy-to-use library for creating secure client/server network protocols over UDP.

It is designed around the networking requirements of competitive realtime multiplayer games such as first person shooters and action games. As such it provides the absolute fastest, most time critical networking layer over UDP, with a client/server architecture supporting up to 64 players per-dedicated server instance.

**libyojimbo** is currently in pre-release. It provides support for cryptographically secure authentication, client/server connection management and encryption for packets sent over UDP, as well as a bitpacker and framework for extending the protocol with custom packet types. Preliminary support for reliable-ordered messages and data blocks is included in the latest release.

The library is under active development and is being extended to provide all standard functionality expected from a professional grade network protocol such as: packet level acks, reliable-ordered messages and data blocks, unreliable-unordered messages and blocks, multiple message channels, user configurable packet budget per-channel, packet fragmentation and reassembly, automatic compression and decompression of large data blocks.

## Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler), a recognized expert in the field of game network programming with over 15 years experience in the game industry.

Glenn is currently writing an article series about the development of this library called [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/).

You can support Glenn's work writing articles and open source code via [Patreon](http://www.patreon.com/gafferongames).

## Sponsors

**libyojimbo** is generously sponsored by:

* Gold Sponsors
 - [Cloud Imperium Games](https://cloudimperiumgames.com)
 
* Silver Sponsors
 - [The Network Protocol Company](http://www.thenetworkprotocolcompany.com)

* Bronze Sponsors
 - [Kite & Lightning](http://kiteandlightning.la/)
 - [Data Realms](http://datarealms.com)
 
And by individual supporters on [Patreon](http://www.patreon.com/gafferongames). Thank you. You make this possible!

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
