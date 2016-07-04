
Monday July 4th, 2016
=====================

Implemented "ReceiveMessage" function and make it safe to call if the current connection is in an error state (return NULL)

Next step now I think is to port the code in "WritePacket" that fills the connection packet with the n most important messages that fit.

Brought this across, but it's really hard to read. Split apart into functions so it's actually readable.

Done. It's pretty good now.

Now need to bring across code that processes packets and inserts received messages into receive queue.

This is a bit tricky, given that there is a situation when the recv queue is full that we have to ignore the packet (drop it)

Need to make this clear in the code what is going on. This code should be readable, not inscrutable.

On that related note, the serialize function is really gross still for the connection packet. It needs some clean up too.

Cleaned up string. Any instances of ToString for address should write to a buffer of length MaxAddressLength

Cleaning up the serialize function for the connection packet. It has to be readable as to what it does.

Next step is to add a function to serialize the ack relative to another sequence: serialize_ack

Cleaned up all the serialize. It's now readable. Much better. No point having something not that complicated look inscrutable when you read it.


Sunday July 3rd, 2016
=====================

Brought across the block message.

For the moment the block message directly owns the pointers to the data block. This is OK, but it's somewhat annoying that a user has to copy across the block data, block size *and* the allocator if they want to copy that block somewhere else and store it. But I guess in most cases, the block message just gets processed in-place as it comes in.

Internal structs for the reliable message system stuck inside "Connection". Now to bring across logic for the reliable message system.

I'd like to keep it all inside the base connection class because:

a) simplicity of just having everything there

b) ability to reuse the same counters, config struct and so on.

c) frankly, it's just not that complicated that it needs to be split up everywhere

If there is anything too complicated that needs to be split up, think about what data structures or helper functions and classes could be created, but keep the core of the logic inside connection I think.

Now bring across member variables and config required for the reliable message system. Done.

Brought across public interface for sending and receiving messages.

Brought across initialization of message data structures and some config.

Now need to have a test message factory for test.cpp. Bring it across from existing code... OK working.

Now I need to free the allocated structures I have created in Connection, because it's reporting memory leaks.

Also, fix up the whole "Reset" and initial setup. I dislike how it seems a bit fragile in the old code. I want it to be less fragile after this cleanup. For example, the way structs are "Reset" means that some logic for freeing the data is elsewhere, which is aware of what should be deleted or freed and some is in the struct itself in the "Reset" function. This is a bit rough, but what is a better option? It's basically because I'm coding this stuff C style, which makes sense because it's pretty low-level.

OK. Memory leak free now.

OK. Code to fill up the send queue in theory is ported. Cut out some of the large block stuff for now, so I can focus on messages first.

Next step now is to extend the connection packet to have the info to serialize a set of messages over the network.

Do I need to setup context for the connection packet to have access to the connection class?

Simplified by making MaxMessagesPerPacket const int rather than dynamic configure. I don't want allocations within each packet just to create an array of pointers to store messages!

Bring across the "Reset" function for the message system.


Saturday July 2nd, 2016
=======================

Next steps, start designing reliability layer.

I still want to use the reliability system I have used in the past, with the ack bits, maybe widened to 64bits this time.

When implemeting this reliability system, make sure it is created out of components that can be reused separately.

Ideally, it should be possible to use the reliability system on its own, or to take the reliability + connection system
and use that without necessarily needing the rest of the library.

Start by bringing across useful code from my previous protocols.

Sequence buffer, sliding window, bit array etc. Done.

Now bring across existing tests for these classes.

All tests pass. Great stuff.

So now need to extract out the core ack protocol into a system.

OK so the code to bring across is the "Connection" class. But I want to strip out the channel stuff from that, because I want to do things differently this time. I think.

I think I want to split out the ack system from "Connection" as "AckSystem". This will make it more reusable.

Nah, it's just so simple. There is nothing to split out!

Cleaned up the connection and it seems to be working fine.

Now to clean up the connection config. Remove the allocator and packet factory from there.

Next steps? Are there some existing tests for the base connection that I could bring across?

It would be good to have a basic set of tests for acks etc.

Yep. Brought across a basic connection test. Passes.

There is another connection ack test, but this requires some callbacks in order to work. Should be reasonably easy to add.

Need to provide some way for the packet factory to create packets and then override their packet types. Do it within the factory, eg: protected method, "SetPacketType( int type )" called by the factory after creating a connection packet to set it to the correct type.

Yes, added "OverridePacketType" and it should be quick flexible. Will allow the same packet type to masequerade as multiple different packet types if users want it (could see a use case for this...).

Acked packets test passes. Yay. :D

What is the next step?

Seems that it is to get the reliable-ordered messages working.

Dependencies of the reliable message system:

1. Message.h
2. Block.h
3. BlockMessage.h
4. MessageFactory.h

Bring these back, then I can bring across the reliable message system.

Put them in yojimbo_message.h

OK. Punting on block messages for now.


Thursday June 30th, 2016
========================

matcher.go was encoding strings as base64 without including the terminating null character, because go strings aren't null terminated. Fixed this, and then also added a check to the base64 decode string to abort the decode if the last character in the decoded string isn't '\0'. This fixes an attack vector for crashing out the C++ code that parses base64 strings in JSON.

Implemented a windows version of "premake5 stress". It does reproduce the same crash in the server. Something is definitely wrong here.

Time to install an evaluation copy of Boundschecker to see what's going on?

Wow. Doesn't seem like boundschecker really exists anymore. Everythings seems acquired a few times by multiple companies and way too expensive. No free eval anymore either. Whatever.

Looks like I'm going to be tracking this one down with open source tools. I'll need to add my own instrumentation. Note that it could also be a logic error. There is no guarantee that it is a memory trash.

Moving back to Mac...

Need to add a mention of "premake5 stress" to the BUILDING.md to show how to connect a bunch of clients. Once it's working of course.

Added a new "premake5 secure_server" option to demonstrate that the client connect going through the matcher is secure, and clients connecting directly (not going through matcher) can't connect to a secure server.

Added information in BUILDING.md about how to run secure server and test it.

Cleaned up logging for connect.cpp to clearly print out client id, requesting match from https://localhost:8080 and so on.

Matcher now logs each time it matches somebody to a server, eg:

    matched client 813ceef4f593ac93 to 127.0.0.1:50000
    matched client f46691038ea667a0 to 127.0.0.1:50000
    matched client 7965cc68d59a2831 to 127.0.0.1:50000
    matched client 5fb2c6ff01c5a11c to 127.0.0.1:50000
    matched client 21368d23955c7346 to 127.0.0.1:50000
    matched client d28c72043d40975c to 127.0.0.1:50000
    matched client e380d13b78d53bff to 127.0.0.1:50000
    matched client 9481d208564f7732 to 127.0.0.1:50000
    matched client ababf0c5e1c78e12 to 127.0.0.1:50000
    matched client 8207bbac50e9866f to 127.0.0.1:50000

Standardized logging of client ids to be 16 wide, eg. so zero padding for client ids is applied, lining up the matcher logs nicely.

Really going to have to go to town with validation and runtime checks to catch this one I think, if it's a memory trash.

First start by adding a magic value in the base packet (debug build only) which is set only if the object is created through the packet factory. That way if somehow a packet is getting passed into free which has not been created via the packet factory, I will pick it up. Also, bad pointers and so on.

Turned on YOJIMBO_DEBUG_PACKET_LEAKS in yojimbo_config.h

Make it store the type as the hash value too, which could be useful.

Added a new feature, #define YOJIMBO_PACKET_MAGIC 1

Packet magic!!!!!one

Now rolling a random uint64_t in the packet factory, I'll stash this on packets that are created from that factory,
then trivially looking at the packet on destroy, I can complain if it looks like the packet wasn't created by that factory,
eg. if it is 0 then it is a manually created packet, and if it is a different non-zero value, it was created by another factory.

OK. I keep adding stuff and it gets stranger. It's definitely a memory trash. It's quite a random one too because the location of the trash shifts around.

I have a crash in the debugger now that indicates that the last variable in the packet processor m_scratchBuffer was overwritten with 0xCC, that's 204 decimal.

Goddamn it. It's the fucking send queue. That's the root cause of the bullshit packets?

Confirmed. The trash is in the send queue. It's not properly handling wrapping. Confirmed by dropping the send queue array size down, and the crashes occur faster. This is definitely it. Fantastic!

Yes, one single place where the element was being written was not wrapping modulus array size. WTF.

Confirmed fixed.

Now to clean up the mess...

Need to remember to switch the mallocs back to alloca once I'm finished with the memory debug stomp tracking.

OK. Everything is cleaned up and we're back to normal.


Wednesday June 29th, 2016
=========================

Fixed race condition in matcher.go found by Egon Elbre. Now use atomic.AddUint64( &MatchNonce, 1 ) to increment nonce.

I wake up today and connect.cpp is failing on MacOSX. I'm pretty 100% sure it worked last night.

It still works today on Linux.

What is going on? 

The error is in mbedtls: 

    MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED

But why is it suddenly failing on MacOSX and no other platforms? What is different?

I tried rebooting my machine. Tried reinstalling mbedtls... same error. But still, it works on Linux.

What is going on?!

Found it. Mismatched headers on macosx. Mac was including the headers from the mbedtls directory for windows.

This happened after I backported to windows. I must not have tested the connect program using mbedtls after adding the mbedtls headers and libs for windows.

To fix this I am going to have to move windows headers and libs specifically under a windows directory, so they do not conflict.

Extended connect.cpp to use the match response to run a secure connect.

Working on getting the actual client connecting through via "connect".

Some issues with server address mismatch potentially. Perhaps it's best to encode a 64bit salt value that only the client knows,
that way the server can replay back, hey, it's me. Sorry if my address looks different to what you expect, but, here is the salt
you are expecting, so you know I'm the real server.

^--- but does this really makes sense when the encryption mapping is based on address? Not really. Hmmm.

There also seems to be a bug in the code to get the socket port from a socket after bind to 0 port. Possible endian swap issue?

I think what is going on now is that the client and server have a different idea of the encryption keys.

This would explain the client not getting packets back from the server after the initial connection request (they are encrypted past that point).

Add some logs to print out the encryption mapping on server and client, and compare? I think the keys are different and it's the backend's fault, maybe I did something stupid like copy paste error somewhere. Checking...

On the client:

    0xc5,0x0c,0x34,0x7d,0x36,0x7b,0x03,0xaf,0x85,0x4f,0x1f,0xd3,0xca,0x7c,0x76,0xf7,
    0x14,0xe8,0x5b,0xb2,0x52,0x49,0x4b,0x09,0x73,0xcb,0xab,0x8a,0xb9,0xea,0x2e,0x06,
    0xed,0x2d,0x2c,0x38,0x46,0x59,0xb1,0x18,0xbb,0x6b,0x14,0x8d,0x3e,0x88,0x6a,0xe7,
    0x5b,0xc9,0x27,0x5c,0xc8,0x5b,0xd0,0x31,0xdf,0x50,0x0c,0x95,0xae,0x38,0x43,0xd5

On the server:

    0xc5,0x0c,0x34,0x7d,0x36,0x7b,0x03,0xaf,0x85,0x4f,0x1f,0xd3,0xca,0x7c,0x76,0xf7,
    0x14,0xe8,0x5b,0xb2,0x52,0x49,0x4b,0x09,0x73,0xcb,0xab,0x8a,0xb9,0xea,0x2e,0x06,
    0xed,0x2d,0x2c,0x38,0x46,0x59,0xb1,0x18,0xbb,0x6b,0x14,0x8d,0x3e,0x88,0x6a,0xe7,
    0x5b,0xc9,0x27,0x5c,0xc8,0x5b,0xd0,0x31,0xdf,0x50,0x0c,0x95,0xae,0x38,0x43,0xd5,

Looks the same to me. What's going on?

I think it's failing because the server is set in insecure mode, and therefore is sending all the packets that should be sent securely, as insecure packets.

In order for secure and insecure mode to coexist on the same server, the server needs to be flexible, and support sending packets as both insecure and secure from the same server instance.

Yes, that's it. As soon as the server is adjusted to not set insecure mode, the secure connect works fine.

I need to fix this. Yojimbo servers set to insecure mode must still allow secure connections.

Noticing some instability in the server as I work. Might be a good idea to spin up a bunch of clients and hammer it.

Yes, it reliably crashes with lots of clients:

    server started on port 50000
    client 0 connected (client address = 127.0.0.1:53725, client id = 688d8f7d4712227b)
    client 1 connected (client address = 127.0.0.1:64053, client id = 4fa420e524ca9d86)
    client 2 connected (client address = 127.0.0.1:56037, client id = eca8d1ef56cd548f)
    client 3 connected (client address = 127.0.0.1:59109, client id = 1dab462abc0020b9)
    client 4 connected (client address = 127.0.0.1:49761, client id = bb3a7ef0c0efa6b9)
    client 5 connected (client address = 127.0.0.1:65310, client id = 576b8ae76e305147)
    client 6 connected (client address = 127.0.0.1:50600, client id = 6dd4fbc4150b2638)
    client 7 connected (client address = 127.0.0.1:59393, client id = 153a3b70f07ad12b)
    client 8 connected (client address = 127.0.0.1:58062, client id = ff00636c65e171d1)
    client 9 connected (client address = 127.0.0.1:53755, client id = e179c1611f8cd242)
    client 10 connected (client address = 127.0.0.1:63179, client id = c11dce58817d4eb6)
    client 11 connected (client address = 127.0.0.1:62638, client id = 42aa917c9c9a5d34)
    client 12 connected (client address = 127.0.0.1:60590, client id = 7f4d55e39ae7c179)
    client 13 connected (client address = 127.0.0.1:54755, client id = 1d930b47a114d8ba)
    client 14 connected (client address = 127.0.0.1:50029, client id = 9f9f76c1025b3e8b)
    client 15 connected (client address = 127.0.0.1:57895, client id = e10dd2deeb1809cc)
    client 16 connected (client address = 127.0.0.1:54650, client id = 1667fbf1a5bf1f7b)
    client 17 connected (client address = 127.0.0.1:63542, client id = 5d54592938a56fdd)
    client 18 connected (client address = 127.0.0.1:65333, client id = 5118ce35b1c6dd73)
    client 19 connected (client address = 127.0.0.1:53070, client id = 9f1f79b3c884c3)
    client 20 connected (client address = 127.0.0.1:61738, client id = ecf84b64fda56bdf)
    client 21 connected (client address = 127.0.0.1:60441, client id = 15e3517b8ee39f40)
    client 22 connected (client address = 127.0.0.1:53608, client id = 498608415cf6d052)
    client 23 connected (client address = 127.0.0.1:58350, client id = 62812141d1a9e421)
    client 24 connected (client address = 127.0.0.1:53555, client id = 1a17200267a2cbac)
    Assertion failed: (m_alloc_map.find( p ) != m_alloc_map.end()), function Free, file yojimbo.cpp, line 142.

Got to track this down. Looks like a memory leak or maybe a bad free somewhere?

I think it's a stack trash.

Yeah. There's definitely a trash going on somewhere.

Alternatively it shows up like this:

    Assertion failed: (type < m_packetFactory->GetNumPacketTypes()), function IsEncryptedPacketType, file yojimbo_interface.cpp, line 363.

Strategy. Replace instances of alloca with malloc and see if I can catch it in valgrind or some other debugger, eg. boundschecker on Windows.

Trying cppcheck. Check runs clean. Found some small bugs, but not the memory trash.

Trying clang static analysis. Found a bug that I had suspected in the base64 public domain code I copied. Ripping it out and replacing with calls to proper functions inside mbedtls library now that it's a dependency, why not. Probably faster too.

Fixed. Bug still occurs though:

    * thread #1: tid = 0x8816a, 0x000000010000182d server`yojimbo::ClientServerPacketFactory::Destroy(this=0x00007fff5fbd9d18, packet=0x0000000102001008) + 93 at yojimbo_client_server.h:352, queue = 'com.apple.main-thread', stop reason = EXC_BAD_ACCESS (code=1, address=0x0)
      * frame #0: 0x000000010000182d server`yojimbo::ClientServerPacketFactory::Destroy(this=0x00007fff5fbd9d18, packet=0x0000000102001008) + 93 at yojimbo_client_server.h:352
        frame #1: 0x000000010001de65 server`yojimbo::PacketFactory::DestroyPacket(this=0x00007fff5fbd9d18, packet=0x0000000102001008) + 133 at yojimbo_packet.cpp:600
        frame #2: 0x000000010001b6c4 server`yojimbo::BaseInterface::DestroyPacket(this=0x00007fff5fbe7878, packet=0x0000000102001008) + 100 at yojimbo_interface.cpp:140
        frame #3: 0x000000010000d843 server`yojimbo::Server::ReceivePackets(this=0x00007fff5fbd9d38) + 227 at yojimbo_client_server.cpp:548
        frame #4: 0x0000000100001243 server`ServerMain() + 579 at server.cpp:83
        frame #5: 0x0000000100001441 server`main + 97 at server.cpp:115
        frame #6: 0x00007fff9c3bd5ad libdyld.dylib`start + 1

What is going on here?! It looks like the packet pointer is incorrectly pointing to a stack location. But I don't think a packet pointer can ever get allocated on the stack.

So how is this happening?!

Got clang static analysis working:

    scan-build make all -j4

Found a few things. Something suspicious in the allocator...

Switched out and radically cut down the allocator. Nothing complicated here just malloc. Everything passes now.

Still happens! !@#($!@($*@!($*!(*))))

It's definitely a memory trash of some kind.

Need to throw boundschecker at this one.


Tuesday June 28th, 2016
=======================

Take the base64 string generated by go and see if the C++ code can decode it into a valid token!

Having some serious fucking trouble getting this to work, checked base64, it's fine. I'm really not sure what is going on?

Something wrong with the chacha poly encrypt getting called from golang maybe?

Maybe print out the JSON and pre-encryption data as bytes, and then the encrypt data, and compare with the C++ version?

Something is mismatched. nonce looks correct (0), private key is the same... hmmm

Found the bug. The private_key static global var as getting clobbered with a different key in other tests, so it was just a different key.

Encrypted connect token now properly base64's, decrypts in C++. Ready to go to the next step.

Now work out how to get the client passing in the parameters of what they want for the match, eg. their clientId
and the protocolId they are looking for.

URL is now: curl http://localhost:8080/match/123125151/2

For a protocol id of 123125151 and a client id of 2

Time to start integrating HTTP in C++ so we can talk to the matcher

Excellent, I can install mbedtls on mac with: "brew install mbedtls"

OK quickly setup an example connect.cpp from mbedtls example

Switched the http response to application/json

How to setup the golang web server to serve over https?

Start here, curl should still work, then once curl works over https, get the mbedtls working over ssl

Seems to be pretty easy to server HTTP over TLS in go: https://gist.github.com/denji/12b3a568f092ab951456

OK so curl is able to connect now, but I think I need to get a real certificate, because I need to run curl in insecure mode: 

    curl https://localhost:8080/match/12314/1 --insecure

Where can I get a free real certificate?

letsencrypt

https://letsencrypt.org/getting-started/

Need to install "certbot"

Should work against staging, otherwise may hit rate limiting: https://acme-staging.api.letsencrypt.org/directory

Looks like this is more like what I want:

https://github.com/kuba/simp_le

Seems like lets encrypt requires actually connecting to the website in order to work, so this rules it out for localhost.

What is the standard process for TLS to localhost during development?

OK. It seems you just used self-signed during development.

Got the TLS connect working from within mbedtls with example. This library is great!

Sketched out matcher interface. For the moment, I will implement on main thread, but the design of the interface is such that it can easily have the HTTPS request done off the main thread and wait until it has finished.

Now extend libyojimbo to have a dependency on mbedtls

Done. For some reason mbedtls headers are found correctly on my own linux box, macosx, but not in ubuntu docker image. No idea what's going on here.

Fixed it by switching to another version. There seems to be two versions released up there. I think the older one is compatible with PolarSSL interfaces.

Implemented a small bit of code to parse the HTTP response and get the JSON.

Work out how to install mbedtls on windows.

Once this is working, windows platform should be working again.

OK. Got cmake building with x64. Headers working fine, linking OK, but CMake by default is going to the wrong c standard libraries. Annoying!

How to make CMake generate a solution with the correct debug and release runtime lib?

CMake documentation says the trick is to add this to the CMakeLists.txt

    if(MSVC)
        set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
        set(CMAKE_C_FLAGS_MINSIZEREL_INIT     "/MT /O1 /Ob1 /D NDEBUG")
        set(CMAKE_C_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /D NDEBUG")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /D NDEBUG")
    endif()

This didn't fucking work, so I had to do it manually. Fuck CMake.



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

Time to switch over to another JSON lib. No point writing more code in libucl when I'm going to replace it.

Remedy guys suggested rapidjson

https://github.com/miloyip/rapidjson

It's pretty good. Very quickly implemented the match reponse parse with it.

Also it is a header only library, so it can just be included in the source tree for windows with no hassle.

Now to remove the rest of the code using libucl and get rid of it.

Now replace the connect token read from JSON with an implementation written in rapidjson.

Now replace the connect token write.

Once this is done, remove libucl entirely.


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

Ok. I think I have the fix.

Yes, also confirming the port endian swap thing is real. Fixing that as well. Fixed.


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
