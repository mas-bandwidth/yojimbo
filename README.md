[libyojimbo]
============

Yojimbo is a new, easy-to-use software library for creating client/server network protocols in C++.

The library is currently in active development and in early access release.

It currently provides authentication, connection management and encryption for packets sent over UDP. 

In the future it will be extended to support packet fragmentation and reassembly, packet aggregation, and time critical reliable-ordered messages and data blocks.

It's goal is to provide all the functionality required to create a secure client/server game with dedicated servers, 
as well as non-game applications that could learn a thing or two from how the game industry does things.

It currently supports 
as well as core functionality needed to create your protocol such as packet fragmentation and reassembly, 
support for sending large data blocks, a bitpacker and a reliable-ordered message system.

It is designed for games written in C++ that 

Its goal is to provide all of the core functionatily you need to implement a secure client/server game protocol with dedicated servers.

Yojimbo supports a variety of compilers and operating systems, including Windows (with MingW or Visual Studio, x86 and x64), iOS and Android.

## Documentation

The documentation is a work-in-progress, and is being written using
Gitbook:

* [libsodium documentation](https://download.libsodium.org/doc/) -
online, requires Javascript.
* [offline documentation](https://www.gitbook.com/book/jedisct1/libsodium/details)
in PDF, MOBI and ePUB formats.

## Integrity Checking

The integrity checking instructions (including the signing key for libsodium)
are available in the [installation](https://download.libsodium.org/doc/installation/index.html#integrity-checking)
section of the documentation.

## License

[BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause).
