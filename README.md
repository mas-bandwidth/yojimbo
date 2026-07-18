# yojimbo

[![CI](https://github.com/mas-bandwidth/yojimbo/actions/workflows/ci.yml/badge.svg)](https://github.com/mas-bandwidth/yojimbo/actions/workflows/ci.yml)

**yojimbo** is a network library for client/server games written in C++.

It's designed around the networking requirements of competitive multiplayer games like first person shooters. 

Generally speaking, if you have a game engine written in C++ and you want to network your game, yojimbo is a really good choice.

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

## Design Assumptions

yojimbo is designed for **client/server games with 100 players or less**.

The client and server must use the **same configuration**. Make sure the `ClientServerConfig`, channels and message factory are identical on both sides, or the client and server won't work together. Keeping the two sides in sync is your responsibility.

yojimbo is **single-threaded**. Call all client and server functions from the same thread. If you want to run networking on its own thread, that works fine — just make sure every yojimbo call happens on that one thread.

## Source Code

You can get the latest source code by cloning it from github:

      git clone https://github.com/mas-bandwidth/yojimbo.git

Alternatively, you can download the latest [release](https://github.com/mas-bandwidth/yojimbo/releases).

## Author

The author of this library is [Glenn Fiedler](https://www.linkedin.com/in/glenn-fiedler-11b735302/).

Yojimbo is built on top of other open source libraries by the same author: [netcode](https://github.com/mas-bandwidth/netcode), [reliable](https://github.com/mas-bandwidth/reliable), and [serialize](https://github.com/mas-bandwidth/serialize)

If you find this software useful, please consider [sponsoring it](https://github.com/sponsors/mas-bandwidth). Thanks!

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md) for how to build, test, and submit changes. To report a security vulnerability, please follow [SECURITY.md](SECURITY.md) rather than opening a public issue.

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).

## Crediting

yojimbo is built on top of netcode, reliable, and serialize — shipping yojimbo
means shipping all four. If you use yojimbo in a product, please credit them
together in your product credits:

> **Más Bandwidth LLC**
> yojimbo — Glenn Fiedler
> netcode — Glenn Fiedler
> reliable — Glenn Fiedler
> serialize — Glenn Fiedler

The license doesn't require this. It's an official request, and honoring it is
appreciated. Fair credit keeps open source honest.

yojimbo also bundles [libsodium](https://libsodium.org) and
[tlsf](https://github.com/mattconte/tlsf) — their own license notices apply as
usual.
