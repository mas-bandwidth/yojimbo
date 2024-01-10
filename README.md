[![Build status](https://github.com/networkprotocol/yojimbo/workflows/CI/badge.svg)](https://github.com/networkprotocol/yojimbo/actions?query=workflow%3ACI)

# yojimbo

https://www.google.com/url?sa=i&url=https%3A%2F%2Fen.wikipedia.org%2Fwiki%2FYojimbo&psig=AOvVaw3nh6QfHQnTVDN1n9NCJj-T&ust=1704945425105000&source=images&cd=vfe&opi=89978449&ved=0CBEQjRxqFwoTCLC_mOj20YMDFQAAAAAdAAAAABAD![image](https://github.com/mas-bandwidth/yojimbo/assets/696656/984e8591-3dba-4f93-b22c-426646518796)

**yojimbo** is a network library for client/server games written in C++.

It's designed around the networking requirements of competitive multiplayer games like first person shooters. 

It has the following features:

* Cryptographically secure authentication via [connect tokens](https://github.com/networkprotocol/netcode/blob/master/STANDARD.md)
* Client/server connection management and timeouts
* Encrypted and signed packets sent over UDP
* Packet fragmentation and reassembly
* Bitpacker and serialization system
* Reliable-ordered messages and data blocks
* Estimates of packet loss, latency and bandwidth usage

yojimbo is stable and production ready.

## Source Code

You can get the latest source code by cloning it from github:

      git clone https://github.com/mas-bandwidth/yojimbo.git

Alternatively, you can download the latest [release](https://github.com/mas-bandwidth/yojimbo/releases).

## Author

The author of this library is Glenn Fiedler.

Other open source libraries by the same author include: [netcode](https://github.com/mas-bandwidth/netcode), [reliable](https://github.com/mas-bandwidth/reliable), and [serialize](https://github.com/mas-bandwidth/serialize)

If you find this software useful, please consider [sponsoring it](https://github.com/sponsors/mas-bandwidth). Thanks!

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
