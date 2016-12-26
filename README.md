[![Travis Build Status](https://travis-ci.org/networkprotocol/libyojimbo.svg?branch=master)](https://travis-ci.org/networkprotocol/libyojimbo) [![Coverity Scan Build Status](https://scan.coverity.com/projects/9652/badge.svg)](https://scan.coverity.com/projects/9652)

# libyojimbo

**libyojimbo** is a new, easy-to-use library for creating secure client/server network protocols over UDP.

It is designed around the networking requirements of competitive realtime multiplayer games such as first person shooters and action games. As such it provides the absolute fastest, most time critical networking layer over UDP, with a client/server architecture supporting up to 64 players per-dedicated server instance.

## Status

**libyojimbo** is currently in pre-release for early feedback. Right now it provides support for: cryptographically secure authentication, client/server connection management, encryption for packets sent over UDP, reliable-ordered messages and data blocks, plus a serialization framework for extending the protocol with custom messages and packet types.

Current work is foecused on documenting the library and finalizing the API. Once the library is sufficiently documented, it will be extended to support large unreliable-unordered snapshot messages with MTU fragmentation and reassembly at the yojimbo wire protocol level. After this, the next milestone is to extend the matcher to support dynamically registered server instances vs. the current example matcher implementation which just directs clients to a fixed server IP address.

For more details please refer to the [roadmap](https://github.com/networkprotocol/libyojimbo/blob/master/ROADMAP.md).

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
