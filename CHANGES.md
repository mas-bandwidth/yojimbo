
Sunday June 26th, 2016
======================

Tracked down why releases were missing Docker directory. Stupid mistake in the premake5 release action. I must have been too tired.

Added a simple matcher in golang that returns some JSON on HTTP request. To run "premake5 matcher".

Will want to run this matcher *inside* Docker, not natively, so find out how to do this before moving any further.

Created a simple go docker instance that builds and runs the matcher.go

This is *not* an ideal deployment, just like the server, because the docker instances are fat (they contain SDK, building and so on... not just the end result application). But it is the easiest way to demonstrate and get everything up and running for people who want to try out libyojimbo backend matcher.

For example, for distributing go applications, this is better approach: http://blog.dimroc.com/2015/08/20/cross-compiled-go-with-docker/

Install libsodium in the docker container for the matcher. We're going to need it.

Update go web server to use: http://www.gorillatoolkit.org/pkg/mux

Hack up a dummy connect token and marshall it to json in the web response

Now get a correct timestamp for expiry in the token.

Continued filling out the connect token JSON. Now everything is filled in, and I need to start integrating with libsodium.

Updated the web response to be on /match so I can put something else on / (eg. a stats web page or something)

Working out how to get access to libsodium in go.

Wow. It's really easy to call into C from golang: http://akrennmair.github.io/golang-cgo-slides/#1 https://github.com/golang/go/wiki/cgo

Got sodium initialize working from inside the matcher. Fantastic.

Now to call the libsodium function to generate a random key. Working!

Spotted a potential buffer truncation in EncryptConnectToken and EncryptChallengeToken. Fixed.

I need the ability to create a nonce and then increase it with each web request. Done. Global uint64

Implemented the equivalent in Go of my Encrypt_AEAD function

Added a MatcherResponse struct in Go which contains the stuff which should JSON in the http://localhost:8080/match response.

Need to store a private key in a global somewhere.

Save the same private key in the shared.h so C++ has it in common.

Client was having trouble connecting to server outside of docker or within, solution was to not bind the server socket to any particular address.

Encrypt the connect token with the private key via libsodium before it is base64'd

Added nonce to the match response. It needs to be passed back to the client, because the server needs it to decrypt the connect token.


Saturday June 25th, 2016
========================

Implement base64_encode_data and base64_decode_data

Finish writing connect token to JSON.

Added tests for base64 encode/decode data in test.cpp

Now implement the code to parse the connect token from JSON.

Add test for read/write connect token to JSON. Make sure it reads back identically.

Convert the client/server token generation to use the JSON serialization of the token, instead of the C++ specific binary serialization (which is not compatible with web backends...)

Removed old json test. Not required now

Switched server to run on 127.0.0.1 so local client can connect. Previously, was not working unless you used docker.

Plan: going need to setup an insecure example on 127.0.0.1 or an insecure mode for running client/server example.

Then the secure mode will go through the matchmaker to find, so it won't matter that servers are running on docker instances on 127.0.0.1 or real addresses.

Update BUILDING.md with instructions on how to build libucl

Integrate libucl with Docker image. 

Update description of container build steps for docker to include libucl.

Seem to now have some problems connecting to the docker server from outside the container. Trying --net=host

Nope. Need to level up my understanding of how Docker netorking works. Something is weird.

Still can't get host networking working. Falling back to -p 50000:50000/udp. OK looks good.

Good info about Docker networking commands here: https://www.ctl.io/developers/blog/post/docker-networking-rules/

Looks like libucl is poorly maintained and does not compile clean under Visual Studio. Shame.

I am thinking of switching to jsmn for parsing, and just pooping out the format with sprintf and strcat on write.


Friday June 24th, 2016
======================

Investigating whether to continue using JSON or to switch to something else

MsgPack looks nice, and has support for a lot of languages out there.

http://msgpack.org

Meanwhile, there is a small JSON library that could be hacked to do simple JSON, but seems shitty to take some JSON code an adapt it.

People basically say that JSON + bin64 is really the way to go. Lingua franca.

I tried out a simple library for JSON, but it doesn't really seem right to take some random unmaintained crap to read secure JSON tokens.

I have pestered the author of libucl to fix various issues that are stopping me from adopting libucl and hopefully he will fix them.

Seems like critical libucl APIs for building structures are:

    ucl_object_typed_new
    ucl_object_insert_key
    ucl_array_append

Looks like I should be able to cobble this together now.

OK. Got a JSON test passing. Don't really want to leave this test in, but it's a good start for the connect token JSON read/write.

Should be unblocked now.

Sketched out functions:

    bool WriteConnectTokenToJSON( const ConnectToken & connectToken, char * output, int outputSize );

    bool ReadConnectTokenFromJSON( char * json, ConnectToken & connectToken );

Mostly implemented write connect token to JSON, but I need versions of encode/decode base64 that operate on data blocks, not on strings.

This is necessary to send the keys over JSON, as they are not NULL terminated.


Thursday June 23rd, 2016
========================

Cleaning up for a 0.2.0 release with preliminary docker support, eg. run a server inside docker.

Some fixes on Windows by moving to static msvc runtime and having separate debug and release sodium libraries.

Made test output nicer by adding "*** ALL TESTS PASS ***" at the bottom.

In order to move forward with the next step I need to get libyojimbo able to read JSON, and open HTTPS connections.

For JSON, I am selecting: libucl

It can be installed trivially on MacOSX via:

    brew install libucl

It also seems to be reasonably mature, and is JSON compatible:

https://github.com/vstakhov/libucl

For HTTPS I am going to evaluate: mbed TLS

https://tls.mbed.org

First I will start with JSON, and work out how to get a connect token into JSON.

I think I might need some way to encode strings as base64 in order to stash data in JSON.

Evaluating this library to rip:

http://libb64.sourceforge.net

It is public domain, which is perfect.

Getting on it... integrated into yojimbo_common.h/cpp

Now to actually test it.

Added a test and pegged it to an external base64 string encoding so it is going to be correct for base64 strings encoded by other programs.

Fixed a bug in base64 encode. It was not adding terminating '\0'. Linux picked it up.

Now to get started with https://github.com/vstakhov/libucl

Write code to can serialize read and write a connect token to JSON such that tokens can easily be created in golang or any other web lang that would implement a REST API for the backend.

Documentation is here: https://github.com/vstakhov/libucl/blob/master/doc/api.md

Study the API and see if it's worth using.

API seems decent. The documentation is not complete, so digging into the header is necessary to work out how to use it, especially on the parsing side, which seems to be deemphasized.

Next: Add ucl code to actually build up an equivalent JSON-like structure and emit.

The emmision part is easy, but I don't see how to build up a complex structure just yet.

Once again, time to study the header...

Actually, first check that I can reasonably install ucl on Debian. make -f Makefile.unix does not build out of the box. 

Created an issue on github about this. Hopefully the author can fix it and create a release that actually builds.


Tuesday June 21st, 2016
========================

Bunch of docker work. Managed to get a docker instance setup that builds libsodium from source, runs tests, deletes the source code and leaves with just a binary containing the server to run.

It's pretty cool, and certainly points the way as to how I can build and distribute a web application demonstrating the secure client connect. Now I should trivially with docker be able to set this up and distribute it in a runnable form to yojimbo users as part of the source. Awesome!


Friday June 17th, 2016
======================

Migrate to github organization

Update to 0.2.0 version

Split out yojimbo_socket.h and yojimbo_address.h from yojimbo_network.h

Bunch of file shuffling and clean up.


Monday June 13th, 2016
======================

Seems there is a bunch of stuff getting stuck here.

High packet loss is exposing some not quite correct stuff...

There seems to be some sort of rogue "disconnect" packet getting through after the client is disconnected.

This should not be possible, because the client disconnects are sent encrypted, and the encryption mapping should block them post disconnect.

I think there are some bugs in the network simulator where old packets are coming back for a second round, somehow.

Sure enough, "Resetting" the simulator before the reconnect makes the bug go away.

So there is some way that packets are getting across from the previous session, even though there should 
be an encryption mapping that blocks this from happening...

What is going on?!

Confirmed that it's the "duplicate" code with +/- 10 seconds in delivery.

But I would expect that old duplicates put into the simulator would not decode without the correct encryption mapping...

So how is it that old duplicates are decrypting?!

I think that the bad encryption mapping is setup by the client getting an old (duplicate) connection request from the prior session.

This connection request is not properly being ignored (as it has already been used!) and appears to be setting up the encryption mapping for *old* packets, so old packets get through for a short period until the client times out.

So the question really is why is the stale connection request (with an old connect token) being let through?!

Some progress made, it seems best for a connect token to only be allowed to open an encryption mapping *once*

This way if an old connection request gets through, it will not put a new encryption mapping in, and the encryption mapping in the normal case gets maintained by encrypted packets being sent and received across it.

So now the old packets seem to have "decrypt failed" and "null key" and I'm not getting phantom disconnect packets.

But for some reason, still, the reconnect still fucks up unless I reset the simulator.

What's going on?!

What I think is going on is that the token expiry is not working, because the unit tests are not in
real time, so the older token is not expiring, *but* the entry for it is going away (timing out).

The bug was that the oldest token was not getting properly overwritten, and in fact it was always
deleting the first token mac in the array meant to stop mac reuse.

Fixing this issue makes the reconnect work properly under soak.

There is now a bug in test_client_server_server_is_full

Sometimes the client is not getting the server full response, and instead is getting a timeout on connection request.

It was a time problem, because that client was not advancing time forward, so it immediately timed out post connect.

Ok. All tests pass in soak now. Good stuff

Done for tonight. Take a break.

Need to make a decision about allocators.

I like the basic allocator approach, and it would be a shame to remove in and rewrite it from scratch

There is some value in it, but the core allocator is actually extremely simple. Keep just the malloc allocator?

But I don't like the array, queue, hash data structures... and I don't trust the implementation of the scratch allocator.

To remove these, write a replacement queue, and then cut down as much as possible just to the malloc allocator.

Queue replacement was actually trivial to write. 5 minutes...

OK. All good.

Now do my best to strip down and rewrite the allocators.

Done. It's much nicer.

Now, there is no need to have separate initialization for the network from initialize libyojimbo, is there?

No there isn't. Combining.

OK. Converted across and everything looks good. Turned on memory leak checking by default as well.

Pass across all remaining allocations and convert them to use the allocator.

Converted queue over to use allocator.

Any other news or deletes? 

Yes. There are a bunch of news and deletes. Mostly in test programs, but be consistent now and convert everything to use the allocator.

Switching to the allocator is important because it provides memory leak detection. You don't want to be leaking memory.

Convert remaining new/deletes over to allocator.

Implement signal break for test, so it can be broken out of when tests are soaking

Marked all allocator stuff with // todo

Converted all allocator stuff over. Left the windows address walking stuff alone (malloc)

Works fine on linux.

Test under windows and linux.

Works great under windows, mac and linux.

All platforms connect to linux just fine.


Sunday June 12th, 2016
======================

Setup the socket interface so that it can bind to a specific IP address, not just a port.

Modify the linux server program to get the first local address and bind the server network interface to that (IPV4...)

Set this address on the server.

Now on the client, modify so it connects to the linux server IP address: 173.255.195.190

Verify that the MacOSX client is able to connect to the linux server IP address.

Nope! Doesn't connect. Don't know why.

Possibly because wrong socket type...?

Really want to make it so the local address is passed in to the socket interface for binding, so the correct socket type can be inferred from the address.

This is the most flexible. Will allow IPV4 addresses and IPV6 addresses to coexist in the same program.

OK. It works now. Almost certainly incorrect socket type.

Fix up socket and socket interface to accept an address instead of just a port.

This is important so that the socket interface can infer the correct socket type from the address.

Otherwise, it will be extremely difficult to mix IPv4 and IPv6 functionality in the same library.

Ideally, *everything* should just work, whether IPv4 or IPv6.    

To do this, the socket must adapt to the address it is bound to.

Seems to work.

Now actually pass in the correct address in bind.

See how sendto gets the address. Do the same.

Done. Seems to work fine.

Update the socket to get its port # after bind to 0 port.

Later on, might want to support multiple interfaces per-server.

This way can have one socket for IPv4 and another for IPv6... supporting both in the same server.

Something to consider later, but this would be very nice, because it would transparently use IPv6 when available, and fall back to IPv4 when it's not.

The only question is should the socket interface handle the dual IPv4 IPv6 internally, *or* should there be two interfaces managed by the client/server? Hard to say. Seems to me that there should probably be only one address per-interface.

Client needs to know their client index.

To implement this, stick it in the heartbeat packet.

Add an accessor and print it out on client connect.

Works in client/server testbed.

Now test with multiple clients connecting to linux server.

Fixed some bugs where the heartbeat packet was getting created with 0 client index.

To avoid having this problem again, created a function on the server to create a heartbeat packet that requires a client index passed in.

Port the network interface code to windows.

Port the platform_time and platform_sleep to windows. (must have some old code around for this...)

So far just ported platform_sleep, time is not used yet (but will be soon...)

Implement the network interface walk, find first local address on windows.

    GetAdaptersInfo()
    GetAdaptersAddresses()

Fucking windows uses wide strings for "friendly name" for network interfaces.

I don't want to deal with unicode in my library, so stripped back to just addresses without names.

Client and server work on windows. Also, with Toredo tunnelling it seems that I can test out IPv6 stuff.

Seems like I should definitely add two addresses per-interface, eg. IPv6 address and IPv4 address.

Maybe combine the function to get the IPv4 and IPv6 addresses into one function, and then pass *both* into the interface, so it can create two sockets, one for IPv6 and another for IPv4... if the addresses are valid. (pass in invalid addresses to disable...) ?

Make sure both the client and server can compile and run on Windows.

Fix up the mac/linux to only use addresses not names.

Now test on linux. Works fine.

Verify cross connects: eg. windows -> linux, mac -> linux.

Passes. Time for a break!

Clean up the simulator a bit.

Make it so you can read back packets only to a specific address, this is required to wrap an interface around the simulator.

Wrap an interface around the simulator.

Note that there is a lot of logic still in the simulator interface that I'd actually like to have moved out to something else so it can easily be shared. How about a base interface implementation shared between simulator and socket?

OK. There is soooo much common functionality between simulator and socket interface it must be moved to a base class.

Moved the counters into the network interface in prep.

Next, move the bulk of the socket interface implementation into yojimbo_network_implementation.*

Now derive only the packet send/receive and m_sockets bit into SocketInterface.

Done. Seems to work.

Now wrap a new "SimulatorInterface" around the base interface, but instead of piping packet sends through a socket, pipe them through the simulator.

Should be easy.

OK done in theory.

Some annoyance based on how the simulator bends over backwards to avoid packet copies.

In order to make the simulator match with the socket behavior I'll have to add a copy on top, which is annoying.

But the simulator is for debug only so whatever...

OK. It adds two extra copies, one on each side... ahahah so inefficient. May want to revisit.

Should be working now. Try it on a few tests.

Amazing. Switched over the tests to the simulator interface and everything just worked.

Idea: when a simulator interface shuts down, it should flush anything to or from its address out of the simulator, so packets are not buffered in the shared network simulator across different simulator interface instances.

Seems like a good idea. Implemented.
    
Bump up the packet loss and network conditions to ridiculous levels and make sure all tests pass.

Looks like there is a bug where the challenge response is not resent back if the initial one is lost.

It should be replied back to the client each time the client sends a connection request...

No it was a bug because the simulator wasn't advancing time forward.

Next, there was a bug because simulator time was going backwards. Changed it so there is one simulator instance per-test.

Passes now.

Seems there are some edge cases that need to be caught in client connect and especially reconnect.

This will be quite painful...

Made some progress by ensuring that the encryption mapping for a client is cleared immediately on client disconnect.

Now it is getting quite a bit further before failing...


Saturday June 11th, 2016
========================

Now implement the server-side processing of the insecure packet. Basically, take that packet, find a free client slot and immediately accept or deny.

What to do if the client is already connected?

I think the insecure connect needs a salt.

If the same address is connected but with a different salt, ignore the connect.

No need to do the challenge response tho. That's for security and we don't need that here.

Add insecure connect state.

Add salt to the insecure connect packet.

Idea: Instead of adding salt, make the client id a random number, and then stash per-client an insecure flag.

Why not add a concept of client flags, eg. uint32_t clientFlags[MaxClients]?

Or, make insecure clients have a client id of 0 and use the salt?

It's difficult to say what is best.

On one hand, a random 64bit number is extremely unlikely to collide.

On the other, what value is a client id anyway, if it is a random number for fake clients? Why not just make it zero with salt?

What if somebody during testing wanted to connect a client with a *known* client id, but insecurely. 

Complicated. What exactly is the use case for insecure connect?

For the moment, assume that insecure connect just won't establish a valid client id, eg. client id will be zero

Removed client id from connect function. Not used.

Implement client "InsecureConnect" that takes just the address, and goes into insecure connect state.

Generate a random salt on each call to insecure connect.

Make sure all this is wrapped with #if YOJIMBO_INSECURE_CONNECT

Get it to the point where the client sends the insecure connect packets w. the salt

Next on the server side process the insecure connect packets

Look for a matching client slot with the address and salt.

If the address and salt match, reply with a heartbeat.

Otherwise, if the address does not exist, add it, reply with heartbeat.

If the address exists, but the salt is different, ignore the connect request.

Now on the client take the heartbeat and go direct from insecure connect to connected if it's received.

Had to set both server and client/server interfaces into insecure mode in order to get this to work.

Good! It *should* be hard to get the insecure mode working.

Test passes. Ready to move on to the next stuff.

Choice as to what to work on now. It's complicated.

For example, could work on getting the reliable protocol + data blocks working inside the client/server.

This would be a large amount of work, but it *would* transform the library into something actually useful instead of just being an auth client/server layer over UDP.

The other choice is to hack up a matchmaker web interface that distributes tokens

This would allow setting up *secure* connect end-to-end demonstrating the auth.

In reality I need both.

But which is the most important to work on next?

I think the most important is to implement the actual protocol side, even if insecure, because teams can use insecure during LAN and start using this library if it has the support for reliable-ordered messages.

But if it doesn't have reliable ordered messages, the library is basically just a wrapper over UDP packets.

Another thing that needs to be done, is setup the proper packet fragmentation and re-assembly at the packet level, otherwise this system is not useful for large delta encoded state, which is the primary network model I work with these days.

So the order of operations seems to be:

1. Create an insecure client.cpp and server.cpp and verify they can actually connect over sockets on all platforms

2. Pass over network simulator and add simulator interface on top of it

3. Switch over all unit tests to use the network simulator instead of sockets

4. Bring across the complicated message and data block code and get it working inside client/server

5. Unit test and soak test the crap out of the reliable message and data block system (bring tests across)

6. Bring across packet fragmentation and reassembly support in such a way that it can be used across multiple network interface implementations (eg. part of packet processor)

7. Implement test program demonstrating lots of clients connecting via matchmaker in one C++ program

8. Implement actual functional matchmaker backend and demonstrate secure connect (lots of work...)

In terms of what I can actually do this month, it seems I should be able to make good progress up to #4.

I think if I tried to implement the matchmaker framework as well, I would run out of time.

Split out a bunch of shared code into shared.h

Added defines to control what gets pulled in, eg:

    #define CLIENT 1
    #define SERVER 0
    #define MATCHER 0

This way the code can be reused in multiple contexts, which is important.

Brought across some old platform code I have for platform_sleep and platform_time functions.

Need to implement a Windows version of these platform functions btw!

Now setup an actual server (insecure mode) including a sleep 0.1 seconds between update pumps.

Done. It needs to be updated to properly sleep to the next frame and detect dropped frames, once an actual game simulation is running, but right now it's OK for it to just sleep 0.1.

Next setup an client.

Make the client port 0 so the client can connect from any port, eg. multiple clients per-machine.

Ahah. The client connect isn't timing out. insecure client connect isn't implemented.

Added at test for insecure connect timeout, and a new client state for that.

Test fails. Now make it pass.

Passes. Moving on!

Verify that client.cpp times out after 5 seconds with no server running.

Now set the server into insecure mode so the client can connect to it.

Verify multiple clients can connect to the server

Server compiles and runs on linux but obviously its hardcoded address is wrong...

Need a way to get the address of a network interface after the fact

And then that address can be automatically passed to the server, so it knows its address.

No need for "SetServerAddress" anymore, but leave it in in case the user wants to override.

Update. You can't query the local IP address from a socket. Instead you need to walk the set of network interfaces using platform specific APIs, and then you can bind a socket to 0.0.0.0 *or* bind it to a specific IP address in that list of interface IP addresses.

Windows:

    GetAdaptersInfo()
    GetAdaptersAddresses()

MacOSX:

    ?

Linux:

    getifaddrs

More info: http://linux.die.net/man/2/bind

Seems that I need to extend network interfaces so they can be bound to a specific IP address.

If you have multiple network interfaces, especially for dedicated servers, you're going to only want the packets sent to particular IP (eg. the interface you are running your dedi on)

Since localhost connections are going to be, by definition, insecure... as long as non-secure connections work to the localhost it's fine. Don't sweat too much the whole ::1 vs. 127.0.0.1 vs real IP thing that sometimes happens.

Added a bunch of functions, eg. get network interface info. get first IPv4 address, get first IPv6 address...

Now, IPv6 addresses prefixed with f8e0 are "link local" and cannot be routed.

This means I believe that I should filter these out... if the are listed in the network interfaces, they are not actualy IPv6 addresses that I can share with the rest of the world to connect to.

So they should be filtered out.

This way, if you ask for the first local address, and only a link local IPv6 address is specified, then that address will be ignored.

Extended to support all IPv6 address types, multicast, link local, site local, global unicast.

Only global unicast addresses are relevant when searching for a valid IPv6 network interface addr.


Tuesday June 7th, 2016
======================

Added test for token being sent to a different server than the one initially sent to.

Removing any and all complexity from challenge token.

The client has already authenticated. The encryption mapping means that only that particular address/port can respond with that challenge token, there was no timeout in the challenge token, so the encryption mapping timeout was the thing that would expire the challenge response.

Point is all the challenge response does is say, OK, I'm the client, I actually got the data you sent to me, and here is my response that can't really be authenticated.

You can never go wide on a challenge response because it needs a setup of the encryption mapping.

Encryption mapping setup can *only* happen with a connect token.

So any going wide hack with challenge token would necessitate an attack at the connect token level first.

So removing all this complexity, it doesn't add any security at all!

Extend the packet factory so additional packet types above the client/server packet set can be sent and received.

This needs to be tested, and easy to do, as this is the common situation, extend the client/server packet types with your own that must be processed.

What needs to be done to client and server classes so they can let derived classes handle custom packet types?

To get started there must be a base set of packet enums, packet factory in yojimbo_client_server.h/cpp

After this is going it must be possible to *extend* this packet factory to add new user packets.

Get this working with existing client/server harness and tests.

There is already a ClientServerPacketFactory inside yojimbo_client_server.h

Derive a GamePacketFactory inside test.cpp and add "game packet".

Rename PACKET_* in yojimbo client server to CLIENT_SERVER_PACKET_* so they cannot conflict with user packet types.

Made ClientServerPacketFactory ctor take in # of packet types, so the user can add more packet types.

Now add a test that verifies game packet payload gets through in both directions (eg. send 10 packets each way)

Extended game packet so it has sequence and 3 dependent variables that can be checked vs. that sequence this way I know that the game packets are actaully getting through uncorrupted.

Added function to look up connected client by address

Extended GameClient and GameServer in test.cpp to have framework for sending and receiving game packets and counting # received.

Added code to actually send game packets from client -> server and server -> client.

Now need to implement a process packet for client and server, which return false if the packet is to be rejected.

This way if it returns false, don't consider that packet towards any timeout credits (eg. time last packet recv'd)

Call the base process packet for all packets inside yojimbo client/server and adjust so all the timeout stuff is done on the return code for that process packet function.

Next add a ProcessGamePacket for both, that is an override, returns false by default. Override it and return true for any packets the game recognizes, but only call it for packet types not recognized by base client/server.

Implement client and server side and now test passes. Game packets are getting across.

When receiving game packets in the test make sure all encrypted packets have sequence > 0 when received. eg. correct nonce.

Added a test for unencrypted packets not getting through if encrypted.

Implement own htons - test_htons, so I don't have to include a bunch of stuff in test.cpp for manipulating addresses.

Added concept of server flags. Now can easily set custom behavior with a flag. Added flags to ignore connection requests and another to ignore challenge responses.

Implemented test for connection response timeout.

Design in the concept of both secure and insecure connects at the same time.

Eg. should be secure by default, but the server can be switched into insecure mode.

In this mode, packets that are normally encrypted are globally allowed to be received unencrypted.

Does the network interface also need to be put into an insecure mode?

I think it would need it, in order to know to always send packets insecure, otherwise it could get weird.

Start here.

Implement network interface flags, easy to extend for additional behavior.

Added flag for insecure mode: NETWORK_INTERFACE_FLAG_INSECURE_MODE

Now implement the insecure mode on the client and server.

I think the client should switch into insecure mode automatically on calling insecure connect.

But the server needs to explicitly enable insecure connects. via a flag?

Yes via a flag, but also needs to be completely compiled out of retail servers.

Make sure insecure connects are a separate packet, so they can be completely removed and also so they have zero impact on normal connection codepath.


Monday June 6th, 2016
=====================

test client can't connect because full (eg. connect 4/4 clients, try to connect another...)

test was incorrect because it tried to call "connect" again on an already connected client, with the same token.

should this be supported? not really. each connect requires a new token.

It's a fact however that you can kick off an undefined reconnect with the same token already grabbed, but that connect will risk having a client join into an state that is undefined, eg. the client will have to indicate to the server that it wants to start again from the lowest defined state, eg. build up from state 0.

So it could be supported at the high level if necessary.

It may also have some implications on reliable ordered events though, so perhaps it's *not* worth supporting.

Added test for detecting connect token reuse.

Added test for client connect token expiry.

Added test for server address being in connect token whitelist.

Added test for client connect token being invalid (random data).

Add tests for typical challenge token exploits.

To do this I will need to be able to create a modified version of the client that gets the challenge token and then saves it off, so it can be stored and then reused by another client with a different address.

Attacks to protect against are:

1. reuse of the same challenge token, from a different client address (impersonation)

2. reuse of the same challenge token, on a different server (going wide and trying to join servers)

3. using an expired challenge token

4. getting through with an invalid challenge token.

It's really difficult to see how I can keep the client/server code clean in Yojimbo, while still provide enough flexibility to override behavior and test these cases above.

Need to think about this for a while!  

It seems that the least disruptive way to handle this is for the derived client and server to be able to access data members of the parent class, so it can be inspected, modified and messed with, allowing the derived classes to read data and changed behavior.

Two key pieces of tech are needed:

1. a client needs to try to connect, stash the challenge token when it receives it, and stop connect (without clearing connect token)

2. a client needs to be *forced* into a connection challenge response state with a specified server address and challenge token

3. a way for the server to construct an older challenge token, eg. with an already expired timestamp

If I can implement these, I should be able to implement all the tests required for challenge token.

Made data members in yojimbo client/server protected rather than private.

Implement client connect, stash token and abort.

Add accessor to grab challenge token.

Add function "ForceIntoConnectionChallengeResponse" that takes challenge token, server address etc.

Did all this, but the challenge response token doesn't get through because the server doesn't have an encryption mapping added for the address.

The only way to get a valid encryption mapping setup is to have a valid connect token for your address and the server...

So I don't actually think it is possible in the real world to do challenge token attacks, because the challenge token must follow a gated connect token...

Is this worth testing futher given this fact?!

It seems I would need to actually hack up the server so that for some reason it has an encryption mapping for the second client.

I guess it's worth testing, but really practically I don't think this makes stuff any more secure

It is just like another layer of security if somehow the connect tokens are broken...

But if connect tokens are broken, the whole system breaks down....

OK. Hacked up an encryption mapping on the server interface somehow. Now the codepath is exercised, but this is still I think not actually a valid attack vector in the wild.


Sunday June 5th, 2016
=====================

Added test for client reconnect 

Added tests for client side disconnect and server side disconnect

Renamed "IsConnected" functions in the server to "FindClientId" and "FindAddressAndClientId" because it had already created a bug where I thought "IsConnected" meant "IsConnected( int clientIndex )"

Added code to check client slots and iterate across only up to m_maxClients rather than MaxClients

This allows the server start to spocify a max clients less than MaxClients (64), which would allow for a subclassed server to restrict allocation of expensive per-client data to smaller client counts on start/stop life cycle.

Extended OnStart so it accepts int maxClients to make stuff easy for allocation on start.

Made sure OnStart and OnStop are called (they weren't...)

Added tests for client and server-side timeout after connection established


Saturday June 4th, 2016
=======================

added tests:

1. client connect

2. client connection request timeout

extended client/server to have start/stop concept.

Most importantly, start allows you to specify the # of clients you want.

Added a bunch of checks "assert( IsRunning() )" where appropriate in server.

Now get test working with new start as well.

Standardized passing in time in first parameter to function in client/server (public) where required.

I like keeping time separate because you'll probably wrap the client/server with your own struct or class, and probably have separate global concepts of time. The client/server don't OWN time. You want the time somewhere up above and passed in.

Add callbacks for OnStart and OnStop

Added time to all callbacks because if you want it, how else will you get it?!

OK. The time thing is annoying. Adding cached copy of time in client/server.

"AdvanceTime" function sets it. You must advance time!

This cleans up stuff nicely. I think I want this in the network interface as well. It's annoying to pass in time redundantly.

Yes. Adjusted client interface as well and everything has cleared up nicely.


Tuesday 31st May, 2016
======================

Idea: maybe a flag on send packet, "immediate"?

eg. as an alternative to complicated queue rewrite with packets being sent out of order, eg. flush all for this address.

Just send some packets and marke their sends as *immediate*, = no queue, serialize this and send it right away.

Sounds like a good feature to add.

Found some bugs in the client/server connection. They were not increasing the sequence number for client/server packets on sends.

This means they were not encrypting with nonce, hence the code was broken from a security point of view. Fixed.

This fix should be brought across to the open source example source code for the article series.

Added the interface, now need to restructure the socket interface to be able to write and send a single packet at a time when necessary.

Done works well.

It's annoying that the server when procesing a client side disconnect sends a disconnect packet to the client that has already disconnected.

So I extended DisconnectClient to take a bool parameter, true by default, to send a disconnect packet to the client.

And then I pass false into DisconnectClient for timeouts and when processing a disconnect packet sent from that client, because it is redundant.

Make the same change vice versa for the client, so if a server-side connect happens, or the client times out, the client does not send a redundant disconnect packet to the server.

Implement encryption manager.

Hook up the socket interface to use it.

It needs time passed in to do timeouts for encryption mappings, so extended send/receive packets to take time parameters.

(This will come in super handy when implementing a network interface wrapper around the network simulator)

Start with something simple that is O(n) but hot/cold split so it is reasonably efficient, eg. array with holes and max.

Consider. How does the encryption mapping deal with connection requests that setup an encryption mapping but don't complete all the way to connect. Do encryption mappings have timeouts? eg. 10 seconds?

Each time a packet is sent or received on the encryption mapping, the encryption mapping is updated so the MRU time is the current time.

Then if an encryption mapping is old, or if we are looking for an empty encryption mapping slot, the old slots are thrown out and reused automatically, which should fill holes. Works well.

Implement a basic timeout, eg. if an entry is older than 15 seconds, it will be reused for another address.

Now need to implement encryption mapping remove.

The only tricky part of this is that it has to be smart enough when it is the last entry to search to the left and find the next valid entry which has not already expired (time).

This means that remove encryption mapping also needs time passed in.

Extended interface to add this.

Now to implement the function. Do it.

Verify basic client/server test still passes.

All good.


Sunday 29th May, 2016
=====================

Convert protocol2.h

Convert network2.h

Split out parts of yojimbo_protocol.h / network.h to other files

Especially, generally useful functions into yojimbo_util.h or yojimbo_common.h

I like yojimbo_common.h better. Fuck util.

Finish conversion of yojimbo_common.h inlines into yojimbo_common.cpp

Move bitpacker into yojimbo_bitpack.h

Move stream into yojimbo_stream.h

Move serialization functions into yojimbo_serialize.h

Now repurpose yojimbo_protocol.h into yojimbo_packet.h - packet header, packet class, packet factory, read/write packet functions.

Split up network into multiple headers.

yojimbo_network.h is good for address and sockets. Great. Leave that.

Move the network simulator out to yojimbo_network_simulator.h

Remove InitCrypto, instead put it inside InitializeYojimbo / ShutdownYojimbo instead.

Move the token stuff into yojimbo_client_server.h

Unit test the token stuff (based on 008 starter...)

Remove the testing harness stuff from client_server.cpp

Move the rest of the client/server stuff into yojimbo_client_server.h/cpp

Eventually, going to want to make a bunch of this stuff more *configurable*

eg. let the server restrict the number of clients he wants to allow to connect.

even though the maximum of 64 is enforced statically at compile time...

But for now, keep it static, and focus on implementing enough callbacks to extend the base class.

eg. you extend the client/server to create your own versions, and demonstrate this by adding new packet types, implementing all client/server logs I want via callbacks (not the base class) etc.

Inherited custom GameClient and GameServer classes from base client and server.

Did the same for GameNetworkInterface and GamePacketFactory.

There seems to be some sort of weirdness with the disconnect packet set from client -> server?

Ah. There was a packet leak if the packet failed to decrypt (eg. broken encryption mapping)

Look for interesting counters to add on the server, eg. a counter for each invisible thing that could happen, but indicates something maybe going wrong, eg. connect token reject etc.

Done. A bunch of counters. Will be more.

Comment out logs that don't make sense to become callbacks.

Commented out logs that I might want to bring back (#if CLIENT_SERVER_DEBUG_LOGS or something)

Add callbacks for packets sent on server (by type)

Unified all packet send to go through one function, don't directly call on the interface (keep it internal)

Implement logs so I can see the traffic going back and forth for debugging. Works well!

Now implement callbacks for packet receive on server.

Same treatment for client:

1. Replace logs with callbacks where possible 

2. Callback for state changes

3. Add callback for packet send

4. Add callback for packet receive

Verify logs are now acceptable. Yes.

Just need to add client state names, and it's all good.
