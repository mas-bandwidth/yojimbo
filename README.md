[![Travis Build Status](https://travis-ci.org/networkprotocol/yojimbo.svg?branch=master)](https://travis-ci.org/networkprotocol/yojimbo) [![Coverity Scan Build Status](https://scan.coverity.com/projects/11339/badge.svg)](https://scan.coverity.com/projects/11339)

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

After cloning, make sure to run this command to populate the netcode.io and reliable.io submodules:

      git submodule update --init --recursive
   
Alternatively, you can download one of the latest [releases](https://github.com/networkprotocol/yojimbo/releases)

## Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler).

Other open source libraries by the same author include: [netcode](http://netcode.io) and [reliable](https://github.com/networkprotocol/reliable)

Glenn is now the founder and CEO of Network Next. Network Next is a radically new way to link networks together, it's a new internet for games, one where networks compete on performance and price to carry your game's traffic. Check it out at https://networknext.com

## Sponsors

**yojimbo** was generously sponsored by:

* **Gold Sponsors**
    * [Remedy Entertainment](http://www.remedygames.com/)
    * [Cloud Imperium Games](https://cloudimperiumgames.com)
    
* **Silver Sponsors**
    * [Moon Studios](http://www.oriblindforest.com/#!moon-3/)
    * [The Network Protocol Company](http://www.thenetworkprotocolcompany.com)
    
* **Bronze Sponsors**
    * [Kite & Lightning](http://kiteandlightning.la/)
    * [Data Realms](http://datarealms.com)
 
And by individual supporters on Patreon. Thank you. You made this possible!

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
