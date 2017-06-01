[![Travis Build Status](https://travis-ci.org/networkprotocol/yojimbo.svg?branch=master)](https://travis-ci.org/networkprotocol/yojimbo) [![Coverity Scan Build Status](https://scan.coverity.com/projects/11339/badge.svg)](https://scan.coverity.com/projects/11339)

# yojimbo

**yojimbo** is a library for client/server games that host dedicated servers in private or public clouds.

It's designed around the networking requirements of competitive real-time multiplayer games such as first person shooters. As such it provides a time critical networking layer over UDP, with a client/server architecture supporting up to 64 players per-dedicated server instance.

## Status

**yojimbo** is currently in pre-release for early feedback. 

Right now it provides support for: 

* Cryptographically secure authentication via [netcode.io connect tokens](https://github.com/networkprotocol/netcode.io/blob/master/STANDARD.md)
* Client/server connection management and timeouts
* Encryption for packets sent over UDP
* Reliable-ordered messages and data blocks
* Packet fragmentation and reassembly
* A serialization framework for extending the protocol with custom message types

Work over the past month has been focused on rebuilding the library on top of [netcode.io](http://netcode.io) and [reliable.io](https://github.com/networkprotocol/reliable.io)

For more details please refer to the [roadmap](https://github.com/networkprotocol/yojimbo/blob/master/ROADMAP.md).

## Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler), a recognized expert in the field of game network programming with over 15 years experience in the game industry.

Glenn is currently writing an article series about the development of this library called [Building a Game Network Protocol](http://gafferongames.com/2016/05/10/building-a-game-network-protocol/).

You can support Glenn's work writing articles and open source code via [Patreon](http://www.patreon.com/gafferongames).

## Sponsors

**yojimbo** is generously sponsored by:

* **Gold Sponsors**
    * [Remedy Entertainment](http://www.remedygames.com/)
    * [Cloud Imperium Games](https://cloudimperiumgames.com)
    
* **Silver Sponsors**
    * [Moon Studios](http://www.oriblindforest.com/#!moon-3/)
    * [The Network Protocol Company](http://www.thenetworkprotocolcompany.com)
    
* **Bronze Sponsors**
    * [Kite & Lightning](http://kiteandlightning.la/)
    * [Data Realms](http://datarealms.com)
 
And by individual supporters on [Patreon](http://www.patreon.com/gafferongames). Thank you. You make this possible!

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
