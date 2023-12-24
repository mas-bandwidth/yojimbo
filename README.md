[![Build status](https://github.com/networkprotocol/yojimbo/workflows/CI/badge.svg)](https://github.com/networkprotocol/yojimbo/actions?query=workflow%3ACI)

# yojimbo

**yojimbo** is a network library for client/server games written in C++.

It's designed around the networking requirements of competitive multiplayer games like first person shooters. 

It has the following features:

* Cryptographically secure authentication via [connect tokens](https://github.com/networkprotocol/netcode/blob/master/STANDARD.md)
* Client/server connection management and timeouts
* Encrypted and signed packets sent over UDP
* Packet fragmentation and reassembly
* Reliable-ordered messages and data blocks
* Estimates of packet loss, latency and bandwidth usage

yojimbo is stable and production ready.

## Source Code

You can get the latest source code by cloning it from github:

      git clone https://github.com/networkprotocol/yojimbo.git

Alternatively, you can download one of the latest [releases](https://github.com/networkprotocol/yojimbo/releases)

## Author

The author of this library is Glenn Fiedler.

Other open source libraries by the same author include: [netcode](http://netcode.io) and [reliable](https://github.com/networkprotocol/reliable)

## Sponsors

**yojimbo** was generously sponsored by:

* **Gold Sponsors**
    * [Remedy Entertainment](http://www.remedygames.com/)
    * [Cloud Imperium Games](https://cloudimperiumgames.com)
    
* **Silver Sponsors**
    * [Moon Studios](http://www.oriblindforest.com/#!moon-3/)
    * The Network Protocol Company
    
* **Bronze Sponsors**
    * Kite & Lightning
    * [Data Realms](http://datarealms.com)
 
And by individual supporters on Patreon. Thank you. You made this possible!

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
