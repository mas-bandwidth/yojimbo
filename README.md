[![Build status](https://github.com/networkprotocol/yojimbo/workflows/CI/badge.svg)](https://github.com/networkprotocol/yojimbo/actions?query=workflow%3ACI)

# yojimbo

**yojimbo** is a network library for client/server games written in C++.

It's designed around the networking requirements of competitive multiplayer games like first person shooters. 

Generally speaking, if you have a custom game engine written in C++ and you want to network your game, yojimbo is a really good choice.

![image](https://github.com/mas-bandwidth/yojimbo/assets/696656/098935f2-ba2b-4540-8d7f-474acc7f2cd8)

It has the following features:

* Cryptographically secure authentication via [connect tokens](https://github.com/networkprotocol/netcode/blob/master/STANDARD.md)
* Client/server connection management and timeouts
* Encrypted and signed packets sent over UDP
* Packet fragmentation and reassembly
* Bitpacker and serialization system
* Unreliable-unordered messages for time sensitive data
* Reliable-ordered messages with aggressive resend until ack
* Data blocks larger than maximum packet size can be attached to reliable-ordered messages
* Estimates of latency, jitter, packet loss, bandwidth sent, received and acked per-connection

yojimbo is stable and production ready.

## Source Code

You can get the latest source code by cloning it from github:

      git clone https://github.com/mas-bandwidth/yojimbo.git

Alternatively, you can download the latest [release](https://github.com/mas-bandwidth/yojimbo/releases).

## Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glenn-fiedler-11b735302/).

Yojimbo is built on top of other open source libraries by the same author: [netcode](https://github.com/mas-bandwidth/netcode), [reliable](https://github.com/mas-bandwidth/reliable), and [serialize](https://github.com/mas-bandwidth/serialize)

If you find this software useful, please consider [sponsoring it](https://github.com/sponsors/mas-bandwidth). Thanks!

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
