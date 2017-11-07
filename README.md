[![Travis Build Status](https://travis-ci.org/networkprotocol/yojimbo.svg?branch=master)](https://travis-ci.org/networkprotocol/yojimbo) [![Coverity Scan Build Status](https://scan.coverity.com/projects/11339/badge.svg)](https://scan.coverity.com/projects/11339)

# yojimbo

**yojimbo** is a network library for client/server games with dedicated servers.

It's designed around the networking requirements of competitive multiplayer games like first person shooters. 

As such it provides a time critical networking layer on top of UDP, with a client/server architecture supporting up to 64 players per-dedicated server instance.

## Status

**yojimbo** is production ready.

It provides:

* Cryptographically secure authentication via [connect tokens](https://github.com/networkprotocol/netcode.io/blob/master/STANDARD.md)
* Client/server connection management and timeouts
* Encrypted and signed packets sent over UDP
* Reliable-ordered messages and data blocks
* Packet fragmentation and reassembly
* Estimates of packet loss, latency and bandwidth usage

## Source Code

You can get the latest source code by cloning it from github:

      git clone https://github.com/networkprotocol/yojimbo.git

After cloning, make sure to run this command to populate the netcode.io and reliable.io submodules:

      git submodule update --init --recursive
   
Alternatively, you can download one of the latest [releases](https://github.com/networkprotocol/yojimbo/releases)

## Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glennfiedler), a recognized expert in the field of game network programming with over 15 years experience in the industry.

Glenn is now focusing on his new startup [Network Next](https://networknext.com).

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
