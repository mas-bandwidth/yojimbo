
Sunday November 13th, 2016
==========================

Server wastes time trying to remove encryption mapping for a client who connected via insecure connect

Added a per-client flag on the server "insecure", if this is true, server doesn't try to clean up the encryption mapping for that client on disconnect (it dosen't exist).

Added a test to verify that a client can switch from secure connect, to insecure connect, back to secure connect. There is a lot of different functionality in these connects, so it is important to make sure that the client can switch between these modes dynamically.


Saturday November 12th, 2016
============================

Extended client connect and insecure connect to take an array of server addresses to connect to.

This is the intended usage, eg. request connection from matcher, get a list of servers to connect to in-order, try each one until a connect gets through.

Right now my main concern is how to inform the user that we are in the middle of a large connect, eg. a timeout or a disconnect for some reason, without them thinking that it is a fatal error. How to design this?

First option I guess is to try to suppress/hide the notifications.

Next option is to intercept all errors (eg. timeouts, disconnects) inside "CheckForTimeouts" and redirect any error to connect to the next server, if there is another server in the list.

I think this is the best option. Don't try to hide it, just do the "right" thing, transition directly to connecting to the next server.

I'll have to setup unit tests for both secure and insecure connection with the correct server only being the last in the list to make sure this is working.

OK. I have set up the multiple connects so there is an internal connect and a "ConnectToNextServer" function that is tried on failure, it avoids disconnects/error states, and instead starts connecting to the next server.

This *should* work. I need to setup unit tests to verify this is woring properly, with the correct server to connect to at the end of the list.

Implement unit tests to verify connection works for multiple servers, both secure and insecure.

Insecure connect passes.

Secure connect passes. All done.


Friday November 11th, 2016
==========================

Optimized network simulator so it doesn't have too much overhead. Packets are now read off the simulator as a batch, avoiding an O(n^2) situation, where n is the number of packets to be dequeued this frame. This should make it a lot faster. It's actually simpler to use as well.

Remember to test that the code compiles with #define YOJIMBO_NETWORK_SIMULATOR 0

Did this, but so much code, and all tests rely on network simulator. It's not a good situation to have a gold build with simulator disabled, if this means you cannot run unit tests in gold build to make sure everything is working.

So I removed YOJIMBO_NETWORK_SIMULATOR define. It's faster now, so it's production ready, and it can be disabled in NetworkTransport by passing in allocateNetworkSimulator = false in the constructor.

If a message fails to serialize, tear down the connection with an error. It cannot recover from this.

Added an error code for this:

    CHANNEL_ERROR_FAILED_TO_SERIALIZE

Added a bunch of debug_printfs to print out reasons for debug serialize to fail.

Added ChannelPacketData::Initialize and bitfield "initialize" to make sure that channel packet data is properly cleared.

Previously, the ctor was not getting called in a few cases, so it had uninitialized data. Nailed this down.

Added a new message type that fails to serialize on read.

Used this to implement tests that check that the reliable ordered and unreliable unordered channels properly handle message serialize failure on read.

Add a test for a very large message send queue (and large # of messages per-packet) with a tiny message receive queue.

This would make sure we never send messages that the reciever can't buffer. Seems to be working fine.

Added some bad network conditions to the last few client/server tests. Now the start/stop/restart test is hanging. Why?

I suspect what is happening is that the connect/disconnect is so quick that there are still connection request packets in the simulator queue, which confuse the server, because the old connect is granted, so the new connect gets rejected with address already connected.

To fix this I really need to discard all packets between iterations in the simulator.

Did this, but it's still happening. I think I have a bug when the client connect starts at a non-zero t, eg. far enough away from t=0 and it instantly times out. Wow! This is a terrible bug :)

Yes. This was a real bug. Fixed by deferring timeout until after the first time "AdvanceTime" is called on the client, so we know the real time value.

Add a unit test that intentionally exhausts the client allocator on the server.

Make sure the server detects this and disconnects that client.

Yes. There was an error in the cleanup, good to have tested this. Fixed.

Removed simple messages test program, it's not adding any value. It was added for a specific reason in the past that no longer applies.


Thursday November 10th, 2016
============================

Added virtual CreateAllocator method and YOJIMBO_SERVER_ALLOCATOR / YOJIMBO_CLIENT_ALLOCATOR macro to make it easy to override the allocator used in client/server. 

This lets users hook up their own custom allocator, provided that it has a constructor that accepts:

    void * buffer, size_t bytes

Like the TLSF_Allocator does. By default it is TLSF allocator, but I overrode in shared.h redundantly to demonstrate the macro.

Finished clean up of client/server boilerplate in test.cpp

Memory leak detection was not enabled. Turned it back on. Some small leaks in test.cpp, but doesn't seem to be any in other testbeds.

Confirmed. ShutdownConnection is not getting called for client that failed to connect.

Why not?

Confirmed. The transition to CLIENT_STATE_CONNECTION_DENIED was going through set state, not disconnect, so it was not cleaning up.

Added a destination state via m_shouldDisconnect, m_shouldDisconnectState, so we can transition on next frame to the disconnect state cleanly on receiving a disconnect packet, or a connection denied packet.

Doing the transition immediately craps out the allocators of the client.

Now the memory leak is fixed.

This happened because some cpp files didn't have yojimbo_config.h included, so some #defines were missed.

Solution for now is to make sure that every cpp file includes yojimbo_config.h at the top

Done.

Apply the same "pump" function boilerplate reduction to the connection tests in test.cpp

Add a test to verify that server start/stop/start with a different number of clients works correctly.

I just had a memory leak in this situation that went undetected because no test does this. This should always get picked up.

Added this test, and voila, yes it uncovered an assert on server restart. Good to find it before a client does!

Fixed an assert that showed up on server restart: assert( !m_packetFactory ). Fixed by clearing transport packet factory in Server::Stop.


Wednesday November 9th, 2016
============================

Added m_active bool to simulator, which is set to true when any network condition vars are non zero. This is checked in "UpdateActive".

Cache the "allocateNetworkSimulator" flag passed to ctor for base transport, and if this is true, *and* the simulator is active, then my function "ShouldPacketGoThroughSimulator" returns true. This is also virtual so it can be overridden if necessary.

Separated out code to write packets, so it can be reused between the code that writes packets for the packet simulator and the code that writes packets to pass to the network.

Added code to client_server.cpp so that it runs packets through the network simulator.

Split out code to read a packet from a buffer, so it can be called from multiple context (pumping packets from the simulator, and when pumping packets from the network).

Need to upgrade the network simulator so it is capable of receiving a bunch of packets at a time.

This breaks the O(n^2) bad performance it currently has, turning it into a more reasonable O(n).

Getting started on this...

OK. So what I need to do instead is to pop off all the packets ready to be delivered into an array.

This array could be sorted, but the order of delivery within a frame doesn't really matter. So don't.

Added pending receive buffer to simulator.

Ouch, because multiple receives are done with different "to" addresses on the same simulator, it's still going to be O(n^2) when receiving packets.

For the moment, it's the O(n)+O(m^2) where m is small. Hopefully m is small enough to not be a problem. If it is, then the interface for getting packets on a per-destination address can change, so the transport has an optimized function to call, and it becomes O(n) + O(m*p), where p is the num different addresses, m is the number of packets each frame to be received, and n is the size of the packet entry buffer in simulator.

Next step is to make the regular transport pump packet receive from the simulator.

Renamed "SocketTransport" to "NetworkTransport". Now we have "LocalTransport" and "NetworkTransport" which sounds good. Remove dummy GameNetworkTransport implementation in shared.h. Users shouldn't need to derive their own transport to work.

Implemented it, enabled network conditions in client_server.cpp, but it's not working, packets aren't getting received. Debugging.

Made client_server.cpp return 1 if the connection fails, this way it can be used as part of travis tests.

OK. I've sorted it out, just a plumbing issue. need a way to receive a packet from the simulator for any address, and to get the to address from it, to pass to the internal send packet. Then it will work.

Still some problems. Packet is being correctly sent through simulator, pulled out and sent/received internally, but server is just not receiving packets. 

Why not?!

I've worked out that somehow I've screwed up the wiring and it's sending packets back to itself (across sendto/recvfrom)

This can only happen if the internal receive is sending to its own address, then recvfrom just does what it does.

So how does it have the incorrect address to send to?

Fixed. I had to/from params back the wrong way in the function implementation for ReceivePacket. o_O

I've somehow broken the local transport. It's not getting packets through. What's going on?

Oh I get it, it's pumping the local transport as if it's the simulated network conditions on top of a real transport, so all the packets are being sent back to self.

Fixed.


Tuesday November 8th, 2016
==========================

Removed simulator transport.

Added a network simulator instance to base transport.

Removed get packet factory accessor to transport, you should know the packet factory you pass into it, you don't need to ever get it from the transport.

Sketched out local transport.

I think moving forward there needs to be a flag "useSimulator" set to true in the base transport when network conditions are set, so packets get delayed before being passed into the "InternalSendPacket" and so on, however for the local transport, this bit of code should be disabled, and it should always call the "InternalSendPacket" immediately, which goes through the simulator (for buffering, even if network conditions are set to zero latency etc).

Add a method to the base transport to set network conditions and clear them.

Should be ready to switch over all tests to local transport now.

Oh boy, massive miscalculation. The local transport needs to *share* the same transport between the client and server, so packets can get across between multiple transports (a transport corresponds basically to one source UDP address).

This is unfortunate. I have to rethink how the local transport fits in with the base transport now.

In the case of the base transport, I really do want a network simulator to exist per-transport, but for the local transport, it's ESSENTIAL that the local transport can share a common simulator with another transport in order to work.

OK. Setup the constructor of the local transport to take a reference to transport (shared transport), and put a bool allocateTransport method in the constructor for base transport. default to true.

Convert half of remaining tests to use local transport. 

Removed default value for allocator passed into network simulator. As a rule, never set default parameters for allocators. Require the user to pass it in, that way they won't ever accidentally leave a default allocator when they it to point to an allocator they already passed in.


Monday November 7th, 2016
=========================

Unified the set of packets between shared.h and test

Renamed "GamePacketFactory" to "TestPacketFactory".

This makes a lot of sense because the shared.h currently is the best example of how to extend client/server, so it should show how to take the client/server packets and extend them with custom user packets.

Sketched out counters for client. No counters added yet, but the enum is there and all the functions so they can be added later.

Removed "CreatePacket" from transport. Now that client/server manages the packet factory itself, it makes sense for create packets to be done there instead of transport. If a user is managing their own packet factory, they can still create packets via the packet factory.

Added a "CreatePacket" method to client.

Removed a bunch of old packet creates that were going through transports.

Simplify context.

There is one client/server context on the stream, hard coded to ClientServerContext *, NULL by default.

Also, there is a user context that can setup, and is of type void* so the user can pass in anything.

The current context should become the user context, but also need to add the ClientServerContext.

Most of the work will be plumbing this through client/server through to the packet processor to the packet read/write info, and to the stream.

Scratch this, keep the existing void* context, and add a new void* user context concept.

This is the easiest way, still lots of plumbing required, but existing stuff works fine.

Added user context to all stream instances, as well as SetUserContext/GetUserContext functions.

Add plumbing for user context.

Added user context to PacketReadWriteInfo

Added user context to PacketProcessor (just one global one, not per-client...)

Base transport has a user context.

Added SetUserContext functions to client and server. They just directly shim onto Transport::SetUserContext.

Simplified global and per-client context in yojimbo server, now that they don't need to potentially be derived by the user, user can just use user context instead. Much simpler.


Sunday November 6th, 2016
=========================

Added #define LOGGING 1 inside shared.h and disabled logging for test.cpp include. This way no logs during tests unless I want them.

Added macros:

    YOJIMBO_CLIENT_PACKET_FACTORY( GamePacketFactory );

    YOJIMBO_CLIENT_MESSAGE_FACTORY( GameMessageFactory );

To make it easier to declare client packet factories vs. overriding the function manually. Easier for users of the library.

Doing the same for server packet factories.

    YOJIMBO_CLIENT_PACKET_FACTORY( GamePacketFactory );

    YOJIMBO_CLIENT_MESSAGE_FACTORY( GameMessageFactory );

This is pretty good. Makes it easier to use the library and adapt sample code.

Unified all the client/server functionality from the test.cpp game client/server instances to the shared.h game/server classes.

Added notes for simulator refactor.

Added more notes for client/server cleanup.

Removed passing in uint64_t to callbacks. It would be frustrating to users, since the sequence # is only available with secure packets (libsodium), as it is the nonce. For insecure packets it will always be zero. This will trip somebody up. Best not to pass it in, and keep it to a transport specific concept for security only.

Renamed "ProcessGamePacket" to "ProcessUserPacket". Whatever derives from client/server may not necessarily be a game.

Unified the set of messages between shared.h and test

Renamed GameMessageFactory to TestMessageFactory

Renamed heartbeat packets to keep alive. A heart beat is constant, keep alive packets are not sent unless other packets are sent. Heartbeat is therefore a bad name.


Saturday November 5th, 2016
===========================

Unified the TestMatcher and the LocalMacher. It's all in shared.h now.

Moved LocalMatcher matcher to a static instance, so it doesn't have to be declared in each test that uses it.    

Moved the GamePacket and packet factory into shared.h


Friday November 4th, 2016
=========================

Added a unit test to verify that a client cannot connect with same address as an already connected client.

Added a unit test to verify that a client cannot connect with the same client id as an already connected client.

Cleaned up client/server test with messages, so message processing is done in helper functions. Much smaller now.


Thursday November 3rd, 2016
===========================

Fixed a bug in server where it was not cleaning up the client allocators on each call to stop.

Client should create its own limited allocator with a configured amount of memory for everything underneath it.

Otherwise, the default allocator passed in will be unlimited for tests and will not exercise the limited memory of TLSF.

Done. Hooking up client allocations to use this allocator.

After a bunch of debugging, it's not a good idea to clean up allocators and tidy up in the middle of disconnect.

The reason being is that disconnect can get called in the middle of packet processing.

I could fix this by setting a flag "should disconnect", and next update processing it inside "CheckForTimeouts".

Let's try it...

OK. This works great.

There is an intermittent crash on the server shutdown in "test_connection_client_server".

I'm not sure why it's happening, but I think it's because I'm now destroying the per-client allocators in stop.

Repro is only 1 in 5.

What's going on?

    frame #0: 0x000000010002483c test`yojimbo::BlockMessage::~BlockMessage(this=0x0000000100637488) + 60 at yojimbo_message.h:98
    frame #1: 0x00000001000247a5 test`yojimbo::BlockMessage::~BlockMessage(this=0x0000000100637488) + 21 at yojimbo_message.h:95
    frame #2: 0x00000001000246a5 test`TestBlockMessage::~TestBlockMessage(this=0x0000000100637488) + 21 at test.cpp:1141
    frame #3: 0x00000001000152b8 test`yojimbo::MessageFactory::Release(this=0x00000001006019d0, message=0x0000000100637488) + 840 at yojimbo_message.h:244
    frame #4: 0x000000010002c4f5 test`yojimbo::ReliableOrderedChannel::Reset(this=0x0000000100602778) + 165 at yojimbo_channel.cpp:433
    frame #5: 0x000000010003a48e test`yojimbo::Connection::Reset(this=0x0000000100601a08) + 78 at yojimbo_connection.cpp:203
    frame #6: 0x000000010003a542 test`yojimbo::Connection::~Connection(this=0x0000000100601a08) + 34 at yojimbo_connection.cpp:189
    frame #7: 0x000000010003a765 test`yojimbo::Connection::~Connection(this=0x0000000100601a08) + 21 at yojimbo_connection.cpp:188
    frame #8: 0x00000001000403b0 test`yojimbo::Server::Stop(this=0x00007fff5face708) + 224 at yojimbo_server.cpp:179
    frame #9: 0x000000010001b624 test`test_connection_client_server() + 5796 at test.cpp:5030
    frame #10: 0x000000010001bc4b test`main + 123 at test.cpp:5151
    frame #11: 0x00007fffe3060255 libdyld.dylib`start + 1

I think there is an allocator mixup with the block message, such that the allocator is nuked somehow by the time the message is cleaned up.

But this is weird, because I haven't deleted the allocators yet for the client...

OK. I see what's going on, I was accidentally creating the block for a server message with the client message factory allocator, so the client got cleaned up first and with my latest change, nukes that allocator on disconnect, so that exposed this bug.

The actual bug is not in my client/server code, it's in the test itself.

Fixed.

Tested on windows. Hitting a nasty stack trash in tests because of the larger maximum size for context and encryption mapping.


Wednesday November 2nd, 2016
============================

Reworked the transport so it can be created optionally without a packet factory.

Add a SetPacketFactory method to the transport

Windows has issues with #define SendMessage screwing with my functions, renamed "SendMessage" to "SendMsg" and so on, for all message related functions in channels, connection and client/server. Most pragmatic option.

Thanks Windows =p

Created the packet factory for client internally via a virtual function

Removed all the code that manually set packet factory on client transports in test code

Set the transport packet factory to the client/server packet factory created internally.

Remove the code that should be removed once server packet factory is used.

Create global and per-client allocators on server.

Make sure these allocators are dynamically created according to config via virtual functions. 

Add an interface something like this to the server:

    GetAllocator( ServerResourceType, int clientIndex )

This way the functions that create resources (and delete them) can trivially get the correct allocator.

I have the global allocator hooked up, but objects are not being freed with the correct allocator.

Need to find all cases where the global allocator is used, and make sure those objects are freed with the global allocator as well.

It was the global allocator and the global packet factory. Moved those into lifecycle of start/stop, and hooked them up to the global allocator. All tests pass now.

Added code to create per-client memory and allocators.

Hook up client objects to use per-client allocators.

Setup client/server with limited memory per-client by default, eg. TLSF allocator for the per-client and global allocators.

Make sure all tests pass with limited memory per-client.

Some breakage with the per-client allocators. TSLF complaining about a double free? Can't see how that would happen though.

Oh yeah, just the wrong allocator passed into the connection new. Fixed.

Decided to remove separate stream allocators. the global and per-client allocators are the stream allocators now.

Passed the allocator into the "CreateX" functions in client for consistency w. server.


Sunday October 30th, 2016
=========================

Cleaned up packet types and names.

Cleaned up server counter and connection request / challenge response enums to be consistent.

Implement OnChallengeResponse callback to match the OnConnectionRequest callback implemented yesterday.

Added some notes for how to protoct against replay attacks w. sniffed packets to WOULD BE NICE.


Saturday October 29th, 2016
===========================

All the work below is likely to be disruptive, it should be done in a branch, and then merged from that branch to mainline when completed!

Created branch "development" where I can do my work without dicking up the main branch. This way I can apply fixes to the main branch easily as they come in, even if I'm in the middle of a bunch of disruptive work.

Client should have a virtual to create context. Server does, client needs one too.

Done.

Fix the security hole for replay attacks by putting the 64 bit timestamp in the extra data for the token AEAD

What do I need to do this? First look on the C++ side in client_server.cpp

Converted the local matcher so it returns the timestamp.

Then went through the go matcher, and the code that parses json and added the connectTokenExpireTimestamp entry to JSON.

Should be passed through end-to-end from matcher to client connect now.

So the remaining work is to plumb it through the "connection request" packet now.

Since I upgraded to premake5 beta 10, I need to update building.md, and as well the Dockerfile so it grabs this version for linux, because: Symbols "On" doesn't exist in premake5 beta 8, jeez these guys are just garbage with backwards compatibility and transitioning premake files from old to newer versions... =p

Actually, the building.md was fine, it just points to the download page, added a note to get at least beta 10.

I'm getting sick of having a bunch of stuff in the yojimbo_client_server.*

Did a bunch of cleanup.

Moved the token stuff out into yojimbo_tokens.h/cpp

Moved some constants out to yojimbo_config.h

Moved packet definitions for yojimbo client server into yojimbo_client_server_packets.h/cpp

Split apart yojimbo_client_server.* into yojimbo_client.* and yojimbo_server.*

Now I need to implement the plumbing that takes the connect token expire timestamp and uses it as the additional data for AEAD.

To do this I need to extend the code that encrypts tokens to use the expire timestamp as the additional data.

OK. Passed over everything and it seems to be working in client_server.cpp

Testing in docker with matcher...

Not quite working yet. I'll bet it's not properly passing the timestamp through the json yet. Add some logs to see what's going on.

Ahh. the token is being generated in matcher.go but was not updated with timestamp as additional data, of course it doesn't decrypt. Fixing.

Fixed it. All working now. Trying a stress test. Works fine. Checking in.

Would be nice to move tlsf into yojimbo_tlsf.h/cpp so all the yojimbo source files fit into one directory. This is good for integrators into game projects. Of course, take care to make sure tlsf is identified as its own licenced source code, separately to yojimbo. Done.

Next, I'm fixing this:

    Its frustrating that by default the server doesn't print out any reason why it's ignoring a connection request

    This make it hard to see what is going on when something is wrong, and requires I turn on extra logs with #define YOJIMBO_DEBUG_SPAM 1

    It would much be better if it logged these things all the time during client/server/connect tests. 

    The only place it should not log is in unit tests, where lots of failures will occur intentionally and we don't want to see those.

My solution is to create an enum for all actions taken when processing a connection request, and then calling through to a callback with the action taken and the information required for printing (packet data, address sent from, decrypted connect token, when applicable)

This will allow me to override the GameServer to print out logs when this is called, so I can get the behavior I want, eg. by default logging of ignored connection requests.

This is important because a new user of this library could encounter a bug with connection, much like the timer skew that I had with matcher in docker, and server without, but the current setup of the test programs for libyojimbo failed silently.

Now the user in this situation would at least see the server printing out "ignored connection request from X - connect token timed out", which at least points to the cause of the error!

Failing something like this, the user would just conclude that the library is unreliable.

OK. so back to work, need to hook up the log now. just keep it basic.

Adding some tests for the logs, make sure they fire and provide useful information.

They work quite well. I'm not 100% happy with the formatting.

Found a bug in the rejection of existing connects by address and client id, and fixed that (so neither can be the same to an already connected client). Previous logic was only filtering out connections if they had both same address and client id, but the encryption mapping doesn't support multiple clients by the same address anyway.

This bug triggered a problem in the token reuse check, because it filtered out on same client id already being connected. Updated that test so it disconnects the client before trying to reconnect with the same token again, and that is fixed.

Added notes that I need to add tests for clients connecting with the same address being ignored (eg. a client with same address, but different connect token), and a test for clients connecting with different address but same client id.

Also, added debug_printf to the error cases in process connection response, simply because in unit tests, i just need to see what case is getting hit when I debug something, and the client/server in unit tests aren't setup to print those logs via the callback i just added (and won't even be).

Fixed a small error where the client having "Disconnect" called before destruction was clearing from a timed out disconnect state, to the disconnected state, and losing information. Also, calling "OnDisconnect" twice. Now if a disconnect is called while in a state <= Disconnected, early out.

Need to flesh out the error prints for all connection request cases now...

All done. Works quite well. I like it.


Friday October 21st, 2016
=========================

Creating new preview release 9 with the matcher fix on Windows.

Preparing 0.3 preview 9 release.

Testing the release...

There was some weird issue with make inside "premake5 docker" complaining about certain files being in the future.

Worked around by touching these files in the docker build step.

There is a bug where the "connect" through matcher is not working for server instances not running through docker.

Why?! It connects fine to "pm docker" server, but doesn't work for "pm server" or "pm secure_server". What the fuck.

Insecure connects work, it's just the connect token that's not working. What's going on?!

Turn on spammy logs in yojimbo_config.h to debug this:

    #define YOJIMBO_DEBUG_SPAM                          1

It's saying "connect token expired".

The problem here is that there is some funny clock skew that started happening in my docker instance?!

The same thing that was causing the clock skew in the make. I worked around it with "touch", but this workaround is not a good thing. I'm going to remove it.

Rebooting the Docker VM fixed the clock skew.

Oh docker =p

Some more info here. I suspect it might be a problem with the latest MacOS Sierro.

https://forums.docker.com/t/syncing-clock-with-host/10432/15

OK. Back to testing the release...

Ok. I'm getting the clock skew again. This seems to be a serious issue.

More information here:

https://docs.docker.com/docker-for-mac/troubleshoot/#issues

It seems I need to solve this, because the drift seems to be quite severe on MacOS.

Need to test this on Windows as well, in case a workaround is required there too.

So indeed, on MacOSX fixing the time with this workaround does it:

    docker run --rm --privileged alpine hwclock -s

This is somewhat frustrating, but now I need to run this on each docker action in addition to other steps? Ouch. Docker...

Added the action to sync time on each docker run. Frustrating, but seems I have to do this.

I think it's still possible to desync time if the Mac sleeps while the matcher or server are running.

In a production environment this would be no problem, since the server and matcher would have time synched via NTP.

But for people testing out yojimbo, this is annoying. Make sure to test this on Windows, in case that has gone backwards as well.

Also, added a note to print out connection request deny / challenge response deny logs, so we can see *why* a client connection request is denied by the server.

Right now its a bit too silent and mysterious. This is not good when anything goes wrong, you *must* see why.


Thursday October 20th, 2016
===========================

Investigated the cause of issue where a truncated HTTPS response was getting received from the matcher.

Investigation showed that curl returned a non-truncated version, the bug then is in my code or mbedtls.

Futher investigation showed that the example code for reading the HTTPS response from mbedtls was flawed. 

It was coded in such a way that it expected to read all the data in one call to mbedtls_ssl_read, but the data comes in in multiple reads.

This is probably the thing that changed with the docker upgrade. My guess is it is some sort of MTU split being enforced by the network layer change in Docker. 

I think this is the case because set of reads looks like this:

	1179 bytes (suspiciously close to a 1200 MTU like value...)
	528 bytes
	- negative return (stop reading)

I adjusted the code so it is able to read the response in multiple calls and that fixed the issue.

I wish I had the time to fix this sooner, this wasn't actually that hard to track down in the end, even though it was frustrating that it broke without any changes on my end. It was ultimately a bug in the code on my side, under my control.

Also spent some time optimizing the git repository with this tool:

https://www.theguardian.com/info/developer-blog/2013/apr/29/rewrite-git-history-with-the-bfg

Which allowed me to remove some large files that were committed accidentally from the git repository.

This tool rewrites the github history, so if you have cloned or forked you probably want to do that again.


Sunday August 14, 2016
======================

Should probably take the effort now to integrate proper allocators.

No point doing all the work for separate allocators and factories when the actual allocators just shim to malloc

This is important too because I think I will want to rework the server to have one allocator per-client (for simplicity),
and from this allocator create the message factory, pass in as stream allocator, use for packet factory and so on.

Copied in tlsf allocator from github. It is licenced BSD.

Now set it up in premake5 as a library and make sure it gets included in release zips.

Changed allocator interface to take size_t for bytes because why not.

Added new "TLSFAllocator" implementation. Beautifully simple to integrate this allocator. Love it.

Added new test: "test_allocator_tlsf"

Server needs its own config now. So does the client. Config is required for the # of bytes to create for the allocators.

ServerConfig, ClientConfig.

Passed the server config into the server constructor. A bit awkward with the existing connection config being passed in.

Same for client constructor. Again, awkward.

I think I should move the connection config inside the respective client/server configs, and have a bool in there "bool enableConnection" or something similar.

Moving connection config inside client/server configs.

Anything else doesn't really make sense.

Maybe combine client and server config into the same struct.

Otherwise, there will be duplicates and the possibility of desyncing the client and server configs.

Some desync in the test.cpp with connection config not being setup properly in context.

Was a bug where the connection was being allocated but the packet factory was NULL.

However, if connection packets were processed, this could happen, and would have been a NULL pointer crash triggered by feeding connection packet types into the client/server. Fixed this by checking for message factory, and if NULL return false.

Moved as many variables as possible into the config.


Tuesday August 2nd, 2016
========================

We need multiple packet factories in the server, a global packet factory, and per-client factories.

The problem is that now, when packets are sent or received, at the point where the packet is being destroyed, you don't know where they came from, so how do you find which factory to destroy the packet with?

It's too much to expect the user to know which factory a packet should be destroyed with. Similarly, the transport code and a bunch of other code at the point where it needs to destroy packets, doesn't really have the information it needs passed in to work it which packet factory should be used to destroy the packet.

The simplest solution seems to be to store the packet factory in each packet, so it knows how to destroy itself. eg: packet->Destroy()

This is a bit clumsy, and I'm not a huge fan of it, but the requirements of silo'ing each client set of packet factories from each other is quite important, so even though it is less than perfectly convenient, this seems like the correct choice.

Next, when a packet is being created on the server, add two support functions:

    CreateGlobalPacket( int type )
    CreateClientPacket( int clientIndex, int type )

And call the appropriate packet create function depending on context.

Pass in the per-client packet factory to the connection.

All tests pass.

Actually, they pass sometimes. Othertimes, in client_server_connect I get packet leaks.

What is going on? Ordering issues with deletion of packets relative to the factory getting cleaned up? 

The packet type being leaked is a connection packet (4)

It's probably the transport that needs to have some sort of flush event. eg. flush all packets in send and receive queues.

Yeah. I'm pretty sure the transport just is randomly having some packets still in send/receive queues at the point of stop and they're not getting cleared.

Added a "Reset" method to transport interface. It will clear all packets in queues. Should fix it.

Oh great that definitely fixed it. It was absolutely needed too, because you really don't want anything before Server::Stop spilling across into the next Server::Start.

Next, add the packet factory to the context.

Now when a packet is read in, see if a context exists, and if it does, use the packet factory in the context instead of the global factory on the transport.

Some asserts in test. Somehow a NULL packet factory is getting through? Not sure what is going on.

Enabling debug logs show that the packet type '12' is somehow coming through and failing the packet read.

Something wrong with the packet factory passed in? Is it because I have the default packet factory in the server base class, not the overridden one in test.cpp? I'll bet that's it. It would be great to put a check in there on packet factory, to make sure they have the same # of packet types. They must be compatible.

Yep. That's it. Added an early assert

Added an even earlier assert 

Cleaned up shared.h by removing concept of custom game packets. So no custom factory is required there. It's only done in test.

It's probably a good idea to create a yojimbo release now. Go home and test on windows and release the next preview. There is a bunch of changes in here relative to the last preview, eg. interface changes for packet factories, message factories, different way to destroy packets, plus a bunch of coverity fixes. So get this release out before starting any work on packet fragmentation and reassembly.

Worked through minor todo fixes before the next preview release.

You now have to call Server::Stop before it is destroyed. Reason: stop calls virtuals (OnStop) and they won't work properly if called during a destructor.

Code will assert if you forget to call stop.

Same treatment for client. Realized "Disconnect" was missing in the dtor. You really want to disconnect clean if possible. Again, can't call virtuals in destructor so this is the safest option.


Tuesday July 26th, 2016
=======================

Add concept of error state to to allocator.

Keep the error state free-form, for now. 0 is no error, non-zero is an error. Possible different error types for different types of allocations. Sort this out later when we have real allocator implementations built in to yojimbo.

If any of the per-client message factories or allocators are exhausted, set an error state, if the per-client resource is in error state, disconnect that client with an error.

Made this a general concept: SERVER_CLIENT_ERROR_*

Rolling timeout into this, and connection error. This way we have a single point where all errors server-side that lead to client disconnection can be handled. No point having a separate "OnClientTimedOut" method in this case.

Added error state to message factory. Also, added special case code for checking error state on the allocator owned by the message factory,
because if that allocator is exhausted (eg. block fragments, dynamically allocated message packets), then that client should also be disconnected as soon as possible.

Added counters for each of these so it would be easy to instrument and can see if they are happening via telemetry on live servers.

Add client error states corresponding to stream error, message factory error, packet factory error.

Hook these states up in the client advance time function. Add todo for packet factory error once it is added.

Add the concept of packet factory error, same as message factory basically. Handle allocator exhaustion as well.

Hook up packet factory error in client.

Fixed bug where client was not resetting connection on disconnect.

Added notes for stuff to do for global stream allocator and packet factory error.

Now I need to work out how to get a separate packet factory per-client going.

There needs to be a global packet factory, and a per-client packet factory.

Problem is you cannot create a transport without a packet factory...

So I think moving the global packet factory into the server implies that the transport is created there too.

I'm not sure I like this. Part of what I like about having a separation of aspects between the client/server,
and the transport is that the transport can be updated completely separately. The client/server just generates
packets which are process completely outside of it and serialized and send and received from the network via
the transport.

I like that a client/server can be switched to a different transport trivially, without modifying code, just passing
a transport into the constructor of the client/server. I don't want to lose this.

So I don't want client/server creating or owning the transport.

But the transport *does* need to know the packet factory in order to create packets on receive, and to destroy packets on send.

In fact, it could get quite a bit tricky inside the transport, having to keep track of which particular packet factory created
a packet, maybe packets should back reference to the factory that created them?

This means I think that the *global* packet factory is created externally (passed in with the transport), 
and the per-client packet factories are created inside the server. Which is a bit... inconsistent, but I think
a better option than jamming together the client/server and the transport into one concept.

Also, it's pretty important that transports can be used without client/server. They should be able to stand on their own.


Monday July 25th, 2016
======================

Thought about unifying the encryption manager and the context manager.

But can't. The encryption manage has a different life time, it is active prior to establishing connection, 
eg. at the point of challenge response, while the context only gets set up when the connection is established.

Hence we don't actually need timeout in the context manager, like we do for encryption manager. Simpler.

Context manager 

Create yojimbo_context.cpp copied and adapted from encryption manager source.

Hook up context manager in base transport.

Add code inside yojimbo server to add/remove/clear context mappings. There is no need in client, because there is only one context.

OK should be good.

Add code inside base transport to query context mapping by address before read/write, and override the allocator if a context is found.

Extend to one stream allocator per-client in server. Hook up to transport via context mapping. Done.

Extend server to actually have one message factory per-client.

All tests pass, because there is no test with multiple clients. Should probably add one of those.

Work out how to get this message factory in to the serialization via context mapping.

I think at this point I really need to restructure to have one context per-client. Because context is how the message factory gets passed in.

Need a function to create the client contexts then. Same pattern: CreateBlah( ServerResourceType type, int clientIndex )

So there is a global context (with NULL message factory), and a per-client context with a valid message factory pointer (if that server is configured with a connection)

OK. This should be all setup. Now I just need to set the context on the packet processor prior to packet read/write.

Yeah I think the context is just not pointing at valid data maybe. Add some sanity checks, eg. a magic value uint32_t at the start of the context?

OK. It should be setup now, but I'm getting some errors with the message factory having a NULL allocator.

I'm pretty sure this means the message factory is either pointing to bad data, or we are accessing a message factory after destructor...

Added magic # to the connection context and assert on it in connection packet serialize before any querying of that context is done.

Yes. It's a non-valid, but non-NULL connection context being passed in.

FUCK. Stupid idiot. I was passing in the pointer to the context pointer.

Everything seems to be working. Try profile.cpp to make sure it's OK. Yep. Looks OK.


Sunday July 24th, 2016
======================

Fix client so it creates the message factory lazily on first connect, and move virtual functions out of constructor.

I'm starting to think the correct approach may be to specify per-client allocators. Here is the reasoning.

It would be really nice to have an allocator during serialization so users of the library can allocate dynamic structures.

It would be nice if on release of a message, this allocator was remembered or known (needs to be able to be looked up easily...)

If I use the message factory allocator, then when client/server is setup without a connection (no messages) where does the allocator come from?

It seems that if I want a consistent property that the stream has an allocator on it that can be accessed via "GetAllocator", then this 
allocator should probably be specific to that client, but not tied to either the packet or messages.

So there are three memory pools per-client, potentially to be managed:

    1. Packet factory
    2. Message factory
    3. Stream allocator

This is somewhat frustrating that there are three, but it follows because message factory is optional that it is necessary. :(

It's best to provide the user with the ability to specify exactly how allocators occur, vs. 

So I think we need a CreateStreamAllocator( int clientIndex ) or something similar, to create the allocator that is used.

Then turning it around, it would be nice if message factory is smart enough to have a "Free" function on messages
which is called with that allocator being passed in.

This way messages would not need to store a pointer to the allocator.

Some leaks in soak.cpp. Not sure why. Messages all appear to be getting released.

Could be that these are the messages in the connection send/recv queues, and those connections are getting cleaned up after the message factories are cleaned up, triggering the leak detection.

Yeah, that was it in ~Client.

Stop and think for a bit about how the per-client stream allocators will work, eg. "GetStreamAllocator( int clientIndex )",
and how this relates to the message factory, packet factory stuff being planned.

What needs to be done first to stream and connection/message types to convert across to per-client stream allocators.

Do this first, before generalizing to per-client message factories.

Add concept of an allocator per-stream.

Unless specified, it is the default allocator. Done. Now you can always ask the stream for an allocator, and you will get one.

Convert the complicated message and block allocation so it uses the stream allocator, instead of the message allocator.

This should overall simplify the code, and make it possible to keep the message allocator more single purpose (messages),
while the stream allocator handles more interesting dynamic allocations (blocks, variable size stuff etc.)

Actually, it would be quite complicated to convert all the message channel allocations to stream allocator,
because a lot of the times when the message channel is used it is outside of the stream.

Bottom line is that when messages are serialized, the message allocator will always be there.

The stream allocator is there for other situations, eg. when you are serializing packets that don't have the stream allocator,
eg. packets that may need to exist without a message factory or connection being created.

For now, it seems pragmatic to leave the message channel allocations using the message factory allocator. For the moment.

Maybe later on it would make sense to switch over.

But still, provide inside client/server the ability to create allocators globally, and pass these allocators in to the
packet serialization stream. This is a good bit of functionality, giving the user the ability to perform allocations
from a per-client pool, on a per-packet basis, and inside packet elements. Very useful.

Added a client/server resource type: Global or Client.

This lets me for allocators, packet factories, message factories, specify if they are being created globally, or for a specific client.

Avoids having annoying functions like: "CreateGlobalStreamAllocator", and "CreateClientStreamAllocator" etc.

Moved the default allocator to yojimbo_allocator.h so it can be created from inside CreateStreamAllocator fns in client/server.

Eventually, I would like to out of the box have a nice high quality allocator out of the box for client/server connections, eg. I want my stream allocator per-client to be X bytes maximum, and my message allocator to be a pool, with the following sizes and maximum # per-sizes and so on. I think this is important because I want yojimbo to basically be configured out of the box to be secure, and using a default malloc based alloc out of the box is not (unless it is limited to a maximum amount of allocated memory per-client, which is probably a good idea too as an interim step...)

Now actually allocate and free the stream allocators. One global allocator on the server, and n client allocators in stop start.

On the client, there is only a global allocator, so simplify down the callbacks there, they don't need client index and resource type.

Now hook up client/server stream allocators.

This needs to be done at the transport layer. eg. SetStreamAllocator

Start with just one stream allocator and work it all the way through to the packet processor and packet serialize

Next, there needs to be way to bind a stream allocator, packet factory, and message factory to a particular address.

This way I can bind and clear the per-client allocators on the transport too, as clients connect and disconnect on the server.

Remember, that this needs to not just bind an address to an allocator, but also a message factory (optional), and eventually, also a packet factory.

Probably something internally based on how encryption mapping works. Keep it at the transport layer.

Start with the interface on the transport, and then work inside to implementation.

Settled on calling it a "context mapping". This is a good concept I think. The context in which packets to and from an address will be serialized. Later on, I can override the context void* as well, and stick it in here, then have per-client context data if desired. Clean.

So I think I now make a copy of the encryption manager and call it "context manager".

Done in header: yojimbo_context.h

Added interface for add/remove/clear context mappings in transport interface, sketched out functions need to be implemented in base transport.

Added some todos with stuff that is currently hardcoded, eg. MaxEncryptionMappings. It would be much better as a function of max simulaneous connections expected. This is why I could not get const int MaxClients working above 1024. If it was calculated as a function of max clients (passed to transport on create), it would have adjusted dynamically.


Saturday July 23rd, 2016
========================

One thing I don't like is that the packet factory and message factory are global for all clients.

This means that a rogue client can DOS other clients by sending and allocating too many messages, packets.

This is relevant to the packet fragmentation and reassembly as well, because now fragments if allocated out of the packet factory
would provide another way to easily deny service to other clients the ability to allocate fragments.

The solution I think is to have a global packet factory, one packet factory per-client, one message factory per-client.

This way the only thing a client can do is hurt itself. If it fails to allocate a fragment, message or packet it is disconnected.

I think this is a crucial design element for security moving forward. It is simply not safe until I do this.

I think the correct way forward for this is to provide callbacks in Client/Server:

    CreatePacketFactory
    CreateMessageFactory

And then have the base client/server code call in to these callbacks whenever a factory needs to be created.

This is a necessity for the server at least, because the server can dynamically adjust the # of clients,
and therefore dynamically adjust the # of packet factory and message factories that it needs.

Therefore the callback is really the only way to go about this.

It should also be a bit cleaner. Ideally, the only things that should be maintained separately from the client/server are the transports,
because these need to be logically separate and eventually will be doing work off main thread this makes sense. To maintain and create message and packet factories is just a nuisance though. The server and client can create and own those just fine.

Get started by moving the packet and message factories inside the client and server instances, created via callback.

To do this, I already have good defaults for packet factories, but I need to create a default client server message factory too.

I will use the default allocator for these factories, because I expect if anybody wants to customize yojimbo, they will simply
override the creation of the factories and use their own allocator they want at the same time.

Now keep just one packet factory and message factory per-client server (for the moment), but switch over to creating it
via the new callbacks. Small steps.

On the client, you'll only ever need one. The one with the server, so allocate/free it in ctor/dtor.

On server, the global allocator should be created in ctor/dtor. Once we have allocators per-client, these should be allocated freed in start/stop.

Hmmmm. actually, since this is a virtual I don't think you are supposed to call those from the constructor, the object is not fully formed yet (check)

On client, create it on first use? eg. in connect?

On server, create it on first call to start?

Cleaning up in destructor is fine though.

Yep. is bad. Don't do it. https://www.securecoding.cert.org/confluence/display/cplusplus/OOP50-CPP.+Do+not+invoke+virtual+functions+from+constructors+or+destructors

OK. Time to get started coding it.

Actually, seeing as how transport *needs* a packet factory set on it in creation, seems that global packet factory must still be created by user.

This is annoying. Should there be a way to set the packet factory on a transport dynamically (eg. factory mapping), and if so, should we allow creating a transport initially without a packet factory on it?

For the moment, get started converting across just the message factory, as this is what is needed to avoid message level DOS where one client exhausts memory pools and affects *other* clients.

Keep thinking about how packet factories fit in with the transport. Probably just needs a "SetPacketFactory" on it, and the option to create one without specifying a factory initially.

Removed message factory from client and server ctor.

Made notes for some virtuals called from constructor and destructor in client/server classes. These need to be moved elsewhere.

Need a flag now to indicate that connections should be created. Previously, the m_messageFactory was acting like that.

Interesting. Now to create messages, you have to create them for specific clients.

On send, look I don't really mind if there is a single factory for that... or should you ask for the factory for a particular client.

Or just wrap it all up and add create message ( int clientIndex = -1 )

This is spiralling out to be a big change :)

First key things need to be done to get working again:

1. Client needs to create the connection lazily in connect, if its still NULL, right after it creates the message factory (if m_createConnection is true)

2. Message creation needs to be done via functions on client and server, and these functions need to know which factory they correspond to

This is somewhat frustrating, for the server to release a message, it needs to have the clientIndex passed in.

On the server, when a message is created, you must also specify which client index the message is for, so it gets the right factory.

Otherwise the connection will attempt to free messages from other factories, which won't work.

I think this is the correct approach though. You simply must not allow clients to DOS other clients with message spam.

Clients *must* be limited to exhausting only their own pool of messages, and when exhausted, the connection should be put into an error state and disconnected.

OK. So now we need create message and release message taking client id as next step.

Extended server to do this.

Client also needs functionality to create messages and release them, but without index this time (just one).

Once this is done, can convert tests and example programs over to work this way and remove the external message factory.

Really need a way to get message factory by client index from the outside. For example, you might want access to the allocator.

Added.

Finished converting test and other example programs to new internal client/server message factory.


Wednesday July 20th, 2016
=========================

I extended memory.cpp to have multiple channels and a mix of small and large blocks going through both an unreliable channel (3k),
and a reliable channel (1k). Fixed some minor issues with validation (can't validate unreliable messages based on # messages received).

There seems to be a budgeting bug. This is a good test case. Need to track down what is going on and fix it.

I really need to think of a better name for this example than "memory". That doesn't really communicate what this sample is for.

It was not actually a budgeting bug, but it was the block fragment not fitting in the rest of the packet.

I have adjusted by changing the budget. Conclusion from this i think is that there should be better error reporting and validation
of the budget reported, so people using the library don't get a really nasty, deep callstack assert in the serialization system when 
the budget is setup incorrectly, leading to the packet being written past the end.

For example, some code should be added to test if the fragment will actually fit in the packet, and either warn, or assert that it won't fit.

Now clean up the printf output for memory.cpp and rename it to something else, eg. wrapper.cpp, simple_messages.cpp?

I like simple_messages.cpp. I don't think there is a one word description of this sample that explains it.

Cleaned up output for simple_messages.cpp. Looks good. Done.

There is a bug showing up now in the start/stop/restart test where clients are failing to connect.

This seems like it might be another time related issue, it only shows up after extended time. By adding more iterations, I was able to make it happen 100%.

It seems to happen around t = 15.

I think there is another timeout bug in there, where the encryption mapping is getting added at t = 0, and subsequently times out at t = 15.0

I'll bet the timeout for encryption mapping is 15 seconds...

Yep. That's it.

This is a bad pattern. Similar errors probably exist on the server if you start it at t=1000, and pump packet updates before time is known.

It's even worse, the race condition is on the transport concept of time, since that is what is passed into add/remove encryption mapping.

This concept of each class (client/server/transport) each having their own time concept, and being potentially not updated first before actions are done on the client or server is dangerous. These bugs are really subtle.

What's the best solution here?

One option would be to require the user to pass in the time to client connect, and server start.

This *seems* weird though, user thinks, why does the client/server need to know time for connect?

Alternatively, require the user to call AdvanceTime at least once before connect, but again, seems weird...

Not really sure what the best way to solve this is. I can try to find the cases where it's hurting (eg. connect, encryption mapping), but there may even be others.

It's getting really nasty, transport needs the concept of time, otherwise I need to pass it in to a bunch of functions, because it's used extensively in the encryption manager for timeout encryption mappings.

So bottom line, the same problem exists for the transport. If the transport isn't pumped with AdvanceTime before adding any encryption mapping, same problem exists, even if time is correct for the client. Can't just solve by deferring the set encryption mapping to the AdvanceTime method of the client, this actually has to be solved in some interesting way.

I think I'm coming to the conclusion that you need to specify initial time when creating these objects.

This includes:

    - client
    - server
    - transport

Finished adding time to all these constructors. It's the only safe thing to do. Cannot have a window where m_time is wrong.


Tuesday July 19th, 2016
=======================

Adding a messages example showing how to use yojimbo connection and transport to send messages
without using the client/server layer.

Some clients using this library are interested integrating with it in this way, implementing their own client/server connection layer, and just wanting yojimbo to queue up messages, generate wire packets (that they send), and in reverse for packet reads. read packets in, process and fill message receive queues, deliver messages to the application.

Added messages.cpp example that will show how this is done.

The strategy is to derive a new transport type from BaseTransport that writes out the packet data, then provides accessors for the packet data and packet size after writing the connection packet.

I could even move this transport into yojimbo as a supported feature if it is useful long term, eg. memory transport?

Good progress implementing this. Almost done. 90%

Decided to rename Connection::WritePacket to Connection::GeneratePacket, write packet is confusing, as it does not actually serialize the packet.

Finished in memory connection read and write example.

Extended memory example showing how to send dumb messages, eg. just small/large blocks of memory.


Sunday July 17th, 2016
======================

Need to add an intensive logging mode to help track down what is going on one user's system.

They are only getting ~20 clients connecting to the server in profile.cpp, but the rest of the clients
are sending packets but seemingly not establishing connection. Weird.

Need logs to track down what is going on. It's really strange.

Added some basic logs here, looking good.

OK. no dice. Packets just not getting through.

Theory: on this particular version of windows the default socket buffers are just too small.

Bump up socket buffers to 1mb send and 1mb receive by default (users can specify if they want more or less, but good default...)

This should fix the issue.


Saturday July 16th, 2016
========================

I don't like that it's a virtual for calling "GetChannelId". That's lame.

Put some base functionality in Channel? eg. getting and setting channel Id?

Done. Also put m_error in there.

Implement unreliable unordered channel.

Start with messages alone. Add blocks later.

This channel has a send queue which is just the set of messages to try to send in the next packet. 

If they don't fit any additional messages are dropped.

On the receive side, messages are dequeued from the packet and just put in an unordered queue for receive.

There needs to be some way to identify unreliable messagse with the packet they came from, so we can cross
reference unreliable messages with a snapshot message in the packet, in potentially a different channel, eg.
these unreliable messages correspond to snapshot n

So on receive, call set id on the messages to the sequence # of the packet they arrived in.

This will allow the user to lookup which snapshot they correspond to, although this will be a bit of work,
I think this is the most general solution.

Sketched out UnreliableUnorderedChannel by cutting down ReliableOrderedChannel.

Hook it up to connection create channels based on channel type in config.

Implement send and receive queues

Implemented send and receive message functions. 

Extended message serialize function to have two payloads:

1. serialize ordered messages

2. serialize unordered messages

Use the unordered for reliable messages.

The unordered does not serialize the id per-message, and later when unordered blocks are implemented, will serialize the block in-place (no fragments).

Also moved block flagment serialization into its own function, to keep the channel packet data serialize clean.

Implement code to process packet data for unreliable unordered channel.

On process, assign all message ids to the packet sequence they were sent in.

Moved existing connection tests, eg. test_connection_messages -> test_connection_reliable_ordered_messages

Added a unit test for unreliable unordered messages. Right now no messages are being received. Digging in...

Disabled packet loss, jitter, duplicates, latency etc. so all messages get through.

Fixed some more bugs in the channel code (missing reference in serialize unordered). 

Tests pass now. Time to move on to unreliable unordered blocks.

Extend unreliable unordered channel to serialize blocks in-place (eg. not as fragments, just the whole block following the message as header)

Yes, eventually for large blocks this will require MTU splits, but for now keep those blocks small-ish.

In theory, blocks sent through the unreliable unordered channel should now work.

Unlike with messages and blocks over reliable ordered channels, where they are treated quite differently (fragments),
there is no substantial difference between how messages and blocks are treated in unreliable unordered, so I don't think
a mixed messages and blocks unit test over unreliable unordered channel adds any value.

Add a unit test to verify unordered channels work with blocks. Passes.

Time for another preview release.


Friday July 15th, 2016
======================

Add channel type enum:

    CHANNEL_TYPE_RELIABLE_ORDERED
    CHANNEL_TYPE_UNRELIABLE_UNORDERED

What needs to change to implement the unreliable, unordered channel?

Start with unreliable, unordered messages.

You really need to know what packet they came in on.

Maybe on the receive side, stash the packet sequence number they came in on into the id for the message?

I think this is the best way to give the user some possibility for grouping the messages together, eg. working backwards and going,
oh, these are the messages belonging to packet X, therefore snapshot Y.

While still letting the user just fire off totally unreliable, unordered messages and not caring when they arrive and just processing them.

Start here.

Rename the existing message structures in send queue so it is clear they belong to reliable ordered.

*OR* create a virtual channel interface and implement different channel types.

What's the best way to go here? Is the virtual overhead likely to be significant? No. As long as they share a common message data format that feeds into the packet, I think it's fine.

I think first, the cleanup within the existing channel, to get the interface from the connection -> channel really clean is required before expanding to multiple channel types.

So the key is to work out what the actual interface the connection needs with the channel is.

Really the connection needs to ask:

    Can send message?

    Send message

    Receive message

    What is the channel data you want to send, and conservatively how many bits in the packet will it take up?

    Process this packet level ack

    Process this channel data.

    Serialize the channel data. <---- this can be unifying between all channel types, with some modifications based on the channel config, which it has access to, eg. common data structure.

I think that's really it.

To get here I have to move more aspects of channel processing out of connection and into the channel.

Specifically, the whole loop across channel ids should just call into a single function on each channel, vs. doing all that logic in channel, beacuse that logic could depend on channel type.

Starting by sketching out a virtual channel interface, and then moving the existing channel into ReliableOrderedChannel.

Almost working post-refactor. Messages are working, but something is going wrong with blocks.

Fixed it. Was a bad fuckup on fragment bits, which was returning 0 bits for any fragment id not equal to zero.

This bug was always there, it's just that now I'm deriving "does this channel have data to send" based on the # of bits
it says it has to send.

Soak is working again.


Thursday July 14th, 2016
========================

Next I need to work out how multiple channels store their data in the connection packet.

Is there some channel data structure that gets allocated in an array n times for the packet, for n entries?

Yes. This works. One contiguous block of memory, not individual pointers per-channel.

Since the entries are sparse, each entry needs a channelId at the top to identify it.

OK I think this can work.

Each channel still allocates its own message pointer array, which is frustrating, and its own block

For now, let this be. Later on can allocate a contiguous block for everything, including an array of message pointers at the end.

Create the channel packet data struct.

Make sure it's a union.

Added a function to fill channel packet data with block fragment and optional block message

Add function to fill channel packet data with messages.

Now switch out connection packet to use a single (one) channel packet data, rather than having the data in-place.

Switch over to new functions to fill this packet data.

All tests pass.

Serialize function for connection packet is getting pretttty unweildy...

Would be nice to move that out to a function inside ChannelPacketData somehow.

Successfully moved serialize into ChannelPacketData.

Ran over a cppcheck scan and fixed some minor issues.

OK. Taking a break now.

Calling something a "network interface" is probably bad.

Network interface in this field has a very, very specific meaning.

Renamed to "Transport", eg. "SocketTransport", "SimulatorTransport" and so on.

Extend connection to support multiple channel data objects.

Extend logic in and out of packets to iterate across number of channel data entries.

I chose to keep it simple and serialize each channel with its channel id, and a count of channel entries.

This makes serialize read easier, because I don't need an extra copy MaxChannels size to serialize into on read.

It's slightly more bandwidth but the simplicity is worth it.

It's a bit more complicated on write, but you'd want that anyway... need to go across channels and see if they have data, can't know until you look,
and then only include in the packet entries for channels that do have data.

Technically multiple channels should be working now. Add a unit test to make sure.

Add a test to make sure multiple channels work. Try two channels both with a mix of messages and blocks.

Channel 0 is receiving messages, but channel 1 is not. What is going on?

Argh. Was just a stupid mistake in the test.

You should be able to specify whether a channel allows blocks or not.

For reliable-ordered channels, this will save memory allocating send and receive block data.

Assert *and* set an error if a block is ever sent (or received) over a channel that does not allow blocks.

Update code to only allocate blocks if blocks are enabled in the config for that channel.

Enable blocks by default. This is an optimization the user will have to manually specify. Blocks by default.

Done. Pretty easy.

Add serialization checks to make sure conservative estimates hold for various parts of the packet.

Added some basic checks, that the connection packet header is within the conservative estimate, and that 
message serialiation doesn't go over budget.

Note that there are still other failure cases that could show up, but hopefully the conservative estimates will hold.

Add a unit test with a constrained packet size to make sure the budgeting is working properly.

Need a way to pass a connection config into the client/server.

What's the best way?

Made two versions of ctor for client and server, one that takes a message factory and connection config, and another that doesn't.

With the plain constructor, you take no overhead for messages and can just get a server with entirely your own packets.

With the message and config constructor, you can fully configure the connection setup. Yes! :D

Modify the client/server message test so budgeting comes into play.

Modify the soak test so budgeting comes into play.

I don't trust it yet.

It doesn't trigger, but I think it might be miscalculting and going way under the budget.

I want it to get at least close to the budget, within 10s of bytes.

How to verify channel budgeting is working as expected?

I think it is basically correct, except there is too much overhead per-message. As messages get smaller,
the overhead per-message is overestimated, and they don't converge closer to the limit, in fact they go further away.

Found it. I was double counting the message overhead (type, id) per-message.

Now it's a bit better, but still really underestimating.

My guess is that I'm saving a lot of bits with the relative encoding of the message id, and this is what is throwing off the estimate.

Test it by disabling this...

Yes. This was it. Is it worth refining the # of bits estimate to get it closer? I think it is.

I can do this. It's just a bit of work.

I wonder what the CPU cost will be of running through the serialize measure on the relative 

Well, it's worth it. Messages must be tightly packed, and we can't let it be this inaccurate.

Implement a better estimate of the per-message id overhead.

This estimate must consider the relative serialization of ids in order to be accurate. Do it.

------------------

Why even have a byte limit on the message stream? Why not just measure and check the # of bytes after measurement if necessary? Annoying.

The byte limit exists so "GetTotalBytes" works. But is this really ever used by anybody?

Removed concept. If you are working to a budget, you should know that budget externally to the stream. Its not the stream's job.


Wednesday July 13th, 2016
=========================

Large block of data (eg. snapshot data) -- a large unreliable message at front of packet?

Set of unreliable messages at the end of the packet (eg. effects and sounds?)

Concept of channels? Let user specified channel config? Seems like the correct approach.

Would be a shame to hard-code the packet layout, if it could easily be made flexible.

For example, unifying everything as "messages" is probably a good concept.

Sending a message on a particular channel id is a good metaphor, vs. having functions "SendUnreliableMessage", "SendReliableMessage", "SendSnapshot".

Decision: Yes. Add support for multiple channels. It is the correct approach.

Now work out how to restructure the code so that the message queues right now are part of a "channel".

For now just have one channel, and have that channel be reliable-ordered, but connection should have a general sense of # of channels,
and the channels should be dynamically allocated. Just for the moment, only an array of size one will be created.

Get this working and passing all existing tests.

So far conversion is going well, except I have one tricky aspect, I don't want the channel to know about the connection packet,
but there are helper functions that fill the connection packet with data (eg. with messages etc.)

I'm going to need to clean this up so the channel part can function without knowing packets, and the connection takes that data and puts in in the connection packet.

Added error and counters back to messages.

Convert send block data, receive block data to pointers, so they can be made optional (don't want this overhead, if you don't for example enable blocks for a channel...)

Get messages inserted into packet.

Message test passes.

Get block fragments and message inserted into packet

BOOM. All tests pass! :D

Connection needs to watch channels. If a channel is in an error state, set CONNECTION_ERROR_CHANNEL

Get callback from channel -> connection so we can log fragments being received.

Owwww. how to best do this? Channel listener I think.

A bit naff having a listener that calls up through another listener, but what are you gonna do.

Done. Once I go multi-channel I'm going to want to put the channel id in the listener as well.

Verified soak is back to regular log behavior.

Need to split apart the channel configuration from the connection configuration. Done.

Next start thinking about how channels will be configured.

static const int MaxChannels for the config? 64?

Channel configuration should contain number of channels, and per-channel config.

Pass channel config to channel on creation.

Generalize channel to channels, dynamically allocated according to the # of channels in the config.


Tuesday July 12, 2016
=====================

There is really no utility to forcing people to implement "DestroyPacket" themselves.

Just like with message factory, hell man, if you create a packet via an allocator, it should be freed via that allocator.

There is no situation where we're not going to want YOJIMBO_DELETE called on the packet when it is destroyed. Just bake that in.

Done.

Maybe add helpers to make it easier to define packet factory types.

    YOJIMBO_PACKET_FACTORY_START( TestPacketFactory, ClientServerPacketFactory, TEST_PACKET_NUM_TYPES );

        YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_A, TestPacketA );
        YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_B, TestPacketB );
        YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_C, TestPacketC );
        YOJIMBO_DECLARE_PACKET_TYPE( TEST_PACKET_D, TestPacketD );

    YOJIMBO_PACKET_FACTORY_FINISH();

I mean, it's really boilerplate.

One nice thing about not baking the type in to the constructor and using this boilerplate, is that it becomes simpler to define packet and message structs.

eg. No more assigning the type in the CTOR.

Plus this lets me use the same message struct, for multiple messages, which seems convenient.

I'm sold on the boilerplate. Lets see how this goes, and maybe how it can be extended further to make defining packet and message classes easier.

Added it. It's pretty nice. Especially now that it lets me simplify the packet types.

Now do the same for messages. Much better. Clean.

Make it easier to create blocks, attach to messages, detach. It's ugly right now.

Just renaming it to "AttachBlock" and "DetachBlock" is probably a good start.

Start here.

Good enough for now.

Spotted a bug in the detach block. Fixed.

Removed packet magic. Redundant with the debug packet leaks feature with std::map. Removed message magic as well.

Try jacking up to 64 players connected to a server and exchange messages between in profile.cpp (all local)

Look for bottlenecks. Are there things that can be trivially improved in here?

This code is pretty ugly. It sure would be nice to do a pass to make sure that users can pretty much derive their own client/server
classes and do all their extension in there, eg. network interface, message factory, matchmaker integration in the base client etc.

Doing it this way, it's really ugly and a lot of work to extend the client to do what I want. Tabling this for fixing up later on.

Added code to create a random message.

Bring across code for sending and receiving messages and blocks.

Once this is going, check in and do some profiling in Visual Studio.

Profile results so far:

Network simulator read packet is really, really slow.

yojimbo::calculate_crc32 is slow, 10% of cost, and it really shouldn't be getting used for encrypted packets (the bulk). Fix this.

Whoops. I had encryption disabled for all packet types after I updated the packet factory with the macro.

There should really be a check inside client/server that certain packet types are encrypted. Otherwise, risk of thinking you are safe when you are not.

Fixed this by adding a virtual that enforces all packets to be encrypted for client and server.

Now profile again. I expect the encrypt/decrypt will dominate over the rest of the packet processing, which should mean we are in the perf ballpark.

Still, look for low hanging fruit and things that should basically be free, and aren't.

Low hanging fruit:

    SimulatorInterface::InternalReceivePacket is dog slow due to linear scan.

    SequenceBuffer::RemoveOldEntries is *very* slow. Rework sequence buffer so it doesn't need to scan the whole array to do this.
    
    hash_string appears to be slow (relatively, for what it does). Check usage in serialize_check

    EncryptionManager::GetSendKey/GetReceiveKey should be basically free, but it has a cost because of the linear search and slow(ish) Address == operator

    GenerateAckBits is slower than it should really be. 1% of total time.

Biggest gains would be:

    InternalReceivePacket

    RemoveOldEntries

In that order. The simulator is not really meant for shipping code, but it should at least be fast, 

Make some optimizations to generate ack bits. 

    1) don't get the pointer to the entry, just check exists bit and sequence

    2) don't use variable shift. generate a mask and <<= 1 each iteration

If it's still slow, try removing the exists bitfield, and expand the sequence array to uint32_t, and use 0xFFFFFFFF as an empty sentinal.

This way simply checking if the sequence # matches what you are looking for is enough to know it's there. One less branch. Less shifting too.

Probably worth the memory.

Check this in and measure it...

It's a bit faster now:

    0.75% - GenerateAckBits
    0.42% - GetFragmentToSend

But it can probably be made faster.

Try the idea with the 32bit integer.

A tiny bit faster still.

Optimized away bool first_entry codepath in sequence buffer. Not required.

Optimize RemoveOldEntries away by removing entries as the sequence buffer moves forward.

Bumped sliding window up from 256 to 1024 entries, just because. Soak test passes.

Punt on optimizing InternalReceivePacket for now, because the network simulator is not something intended for use in production.

Convert the profile to use actual socket network interfaces, which will give a more realistic measurement of actual performance, including sendto and recvfrom

Now do a final profile on windows and call it a day for now. Good enough.

Added implementation of platform_time for windows.

Added code on the server and on the client to watch for connection error. If a connection error is spotted, the client is disconnected immediately.


Monday July 11, 2016
====================

Idea of block message having a real serialize function (fragment 0), but also a block attached to it is sexy.

This could be really flexible, for example for a large block you want to send across, but with some free from data you 
also want to send with it, eg. metadata describing that block.

The idea also of having different blocks as message types, so you can distinguish different blocks sent is really strong.

Extend the soak test to have a uint16_t sequence added to the block message

Implemented. Passes test. This is very convenient.

Add callbacks from connection -> client/server so I can print out logs when fragments are received.

Done. Can add more callbacks in the future, but this is enough to get the soak test up and running.

Updated BUILDING.md with information about reliable messages and new soak test.

Make a preview release

Tested on MacOSX and Linux. Fine.

Tested on windows. Some minor compile errors to fix up.

Hold on test.exe runs really slow in debug, what's going on?

YOJIMBO_ALLOCATE_ARRAY was being used for super-large primitive arrays, and is very slow in debug.

Removed. Replaced with allocator.Allocate( sizeof(T) * count )

Perf is back to normal.

Fixed some issues with some tests not having enough iterations and very rarely failing under packet loss.

Really should add a queue test. I broke it and it was hard to track down (breakage only showed up in stress test with 64 clients)

What to do next?

Time for some general cleanup while I think over the next steps.

Added test_queue

Bring across tests for blocks

Converted test.cpp set of messages and factory to match what is in shared.h

Updated test_connection_client_server to send both messages and blocks

Implement test_connection_blocks

Implement test_connection_messages_and_blocks

Everything passes.


Sunday July 10, 2016
====================

Client/server now automatically sets up the connection context. User should not need to do this manually.

Planning. When is 0.3.0 finished?

I need to not only add reliable ordered message and blocks, but also, I have promised packet fragmentation and reassembly and packet aggregation.

So there is a bit more work remaining.

I think once the initial messages and blocks integration is done it is time for the next preview release, 
but it will take at least a month to flesh out and fully complete my vision for the rest of the protocol.

Pushed compressed packets and blocks until a future release. I don't want to add that to 0.3.0

I am wondering if a separate allocator for blocks is worthwhile, or if the same message factory allocator is to be used?

I think it may be best to use the same allocator, because if a large block is requested, you could shunt it off to a separate
heap or whatever, without adding the complexity of different allocators for messages and blocks.

Conclusion: share the same allocator, for now.

Bring across data structures for blocks from example source code.

Some work converting the send block data. Needed to add explicit "Allocate" and "Free" functions to this struct, so it can work with allocator.

Note that there is a bunch of stuff in the config for the blocks that needs to be put in the connection config. eg. MaxBlockSize, MaxFragmentsPerBlock and so on. These must be user configurable.

Next do the same for the block receive structure as well.

OK. So these data structures in theory should be ready to integrate with the connection.

Hook them up.

OK. Send and receive block data are now hooked up nicely, and I've moved the relevant block configuration into ConnectionConfig. Nice!

Add send and receive block data to reset function.

Added block related data to send queue entry, sent packet entry.

Bring across changes to send message that handle blocks.

Next bring across changes to "WritePacket" and all supporting functions (lots of them...)

OK. Brought across the non-block specific stuff for 'WritePacket' plus supporting functions.

Some more stuff to bring across for block messages being sent.

OK. Brought across all of the support functions but had to comment out a bunch of stuff because I don't have the packet structure for block fragments yet. Need to bring that over next.

This requires some adjustments so the blocks are allocated and freed via the packet factory allocator, vs. new and delete.

Bring over connection packet changes to support sending fragments.

Brought over connection packet variables. Adjusted code to free the block data via the message factory allocator.

Now to bring across the adjusted serialize function. Tricky.    

OK. I think I have the functionality in the connection packet that I need.

Now to bring across the remaining helper functions for blocks and it should be functional.

Should theoretically work now.

Extended soak test to also include sending and receiving blocks.

Extended to send blocks, and receive them, but it's getting stuck for some reason on the receive for the first block?

Why is it hanging? Adding logs...

I think the acks are maybe broken? Yeah. Missing some code in process message acks.

Yepppp. OK it's working fine now.

Added code to verify block size and contents on the receive side.


Wednesday July 6th, 2016
========================

Implement listener interface? Client/Server implement connection listener? Yes.

OnConnectionPacketSent, Acked, Received.

Getting the listener callback working with multiple connections is going to be a bit tricky. For example, need to pass in connection instance to work out index for server per-client connections?

Pass in the Connection*

Added a client index to connection object. It's the safest easiest way. That way, const int clientIndex = connection->GetClientIndex()


Tuesday July 5th, 2016
======================

Back port to "Building a Game Network Protocol"

1. Back port a minimal example (soak test) showing how to implement reliable ordered messages as example source code ("Building a Game Network Protocol")

2. Clean up the source code in "Building a Game Network Protocol" as much as possible so that it isn't like a prototypical version of libyojimbo (which the client/server stuff kindof is...). Ideally, each example should only depend on protocol2.h and network2.h. Keep it clean.

3. Where is the line between libyojimbo and "Building a Game Network Protocol". I think it has to become libyojimbo at the point when everything comes together in the secure server with encryption. Otherwise, it's just so much a reimplementation of libyojimbo in a single cpp file. Silly.

More thought required on this one.

Decision has been made. Backport stuff that makes sense and can be standalone.

Stop at the example of the insecure client/server and separate encryption example.

Beyond this it would basically require maintaining yojimbo as an example program -- that *is* what the secure dedicated server stuff has become.

Bumped up soak test to have some higher packet loss (99%), and it gets this:

    client sent message 11563
    Assertion failed: (int64_t(difference) >= int64_t(7)), function serialize_int_relative_internal, file ./yojimbo_serialize.h, line 291.

Need to track this down. It should work flawlessly under worst possible network conditions. It must not break.

Wow. That took a long time. Firstly, there was a bug in the sequence buffer not cleaning up old entries from the previous sequence wrap around. This was incredibly rare, but could happen if very few packets were actually received, leading to false acks.

Fixed by adding "RemoveOldEntries" to sequence buffer and calling it for sequence buffers that need it inside connection.

Removed packet aggregation support. It is not required with new message system.

Implement function to get packet data to send.

This function needs to measure each message as it sends, vs. ahead of time. 

This is OK because we are only ever considering sending messages once, and if they don't fit into the packet they are discarded.

Give up bits is a bad concept for reliable ordered messages, because it assumes that no message will be smaller than some # of bits, so it is dependent on the size of messages being sent. if all messages are big, but there are a lot of them, it will return to worst case behavior.

It would be better to just sent a # of messages to try past the first one that doesnt fit, so it is bounded no matter what the size of messages are, as well as setting a bit level below which it just stops (can't possibly have a message smaller than this).

This way a full send queue of large messages don't revert to worst case (scan entire send queue) if there is a # of bits left that is larger than give up bits, but smaller than the minimum size of all messages in the queue.


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

Now port the code that receives messages and puts them in the receive queue. Done.

Next step is to get the code for processing acks working, eg. ack messages that are in packet entries that were sent.

OK. Should be working now.

Are there tests I can bring across now? Yes. Hacked up a test with messages being sent. Half done.

But it seems that lots of addrefs are being called on the messages, without them being cleaned up. I think these are the messages added to the data structure for the acks. So they need to be addref'd but also, when an ack entry goes stale (eg. old) they also need to be cleaned up.

Where is the mechanism for doing this?

Wait, are the messages actually getting addref'd in the sent packet? I think it's just message *ids*. That would be much better.

The addref is in the connection packet.

But, how does the connection packet know how to clean up messages, when it doesn't know the pointer to the message factory?

Should it cache the message factory pointer? It probably has to. Wow. Fixed. I don't really have a choice here though, you need the message factory to clean up messages, and since the connection packet necessarily adds refs to messages, it needs a way to release those messages.

Setup test for receive messages. It's currently failing. No messages are getting through. Need to step through with logs and work out where it's breaking.

Bunch of stuff. Was forgetting to call read/write packets on the interface. Also, needed to make some fixes to make sure the context set gets through to the packet processor.

I think messages are not getting acked. So the same initial 23 messages are being put in the packet over and over.

Why are the acks broken? Printing on ack message shows no messages being acked.

I was just not calling the "ProcessMessageAck" on the ack call.

But now messages are leaking. Lots of them. Must be messages in packets...

I think I am just forgetting to free the packets. The connection doesn't free the packet, it just reads it. Yeah that fixed it.

Test passes now. Reliable message support is in and working. Fantastic!

Fixed bug in relative serialize of ack from sequence. A bit more efficient now.

Server now accepts an allocator and message factory in constructor. 

If server has a valid message factory, connection objects are create per-client in Server::Start and cleaned up in Server::Stop (and in dtor now)

This makes connection allocation and setup of message factory as optional, which I think makes sense. You may not want to use messages in your client/server protocol. Connection is also quite large, it allocates to big beast, especially if you have 64 players. Users of libyojimbo need a way to avoid taking this overhead, if they don't want it.

Now integrate connection with the client. Client writes connection packets if connection "HasDataToSend" return true.

Now do the same for the server. Done.

Now, client and server should process connection packets back to the connection class, if it is valid.

I think there is a potential race condition server-side.

a) The server needs to wait until it has received the first heartbeat packet from the client, and keep sending
heartbeats down to the client in that case.

b) The client needs to take any packet, heartbeat, connection or game packet received from the server
and trigger it from pending connection -> connected state.

It's a tough choice. I think b) is the most robust, lowest latency option, but could be hard to test.

On the other hand, what happens to a) if you happen to receive other packets before "connection"

I think the correct option is b) but I need to update code to allow transition of client state
via some other function for game packets and other packet types that can trigger connection.

There is also somewhat of an annoying piece of logic that a packet can come through and its processing
is rejected, eg. returns false in process packet, but could still trigger a connect.

Not sure how to handle this. If I process that packet while not connected yet, it could itself cause
logic errors, but if I let any packet coming in trigger a connection, even if it returns false during
processing, then it is possible that a poorly formatted packet will trigger a connection.

In the end I don't think that forming a connection early is a bad thing, and is ideal, so lets go with b)
and if the processing of a packet returns false, it still initiates the connection anyway. It should 
time out later if the processing is truly broken for some reason. rare.

Harden client connect so if any packet comes in that is a heartbeat packet, connection packet or any game type packet, and the client is still in a pending connection state, the connection switches to connected before processing that packet.

^--- No this doesn't work, because the heartbeat contains the clientIndex, and the client needs to know that before considering the client connected.

So the server needs to have a concept "fullyConnected". Once fully connected is true, the server stops sending heartbeat packets to the client unless no other packets have been sent. For it's part, the client must not set "IsConnected" until the client index is known, so no other packets may be sent client -> server until the client knows its client index from a heartbeat packet.

Done. Separated client send packet to server into internal version for connect, which sends packets at any time, and the regular, protected function "SendPacketToServer" which can only send once connected. Now there is a guarantee that the client only sends connection packets once being connected.

Now on the server, add concept of "fullyConnected" per-client, which is false initially and becomes true when the first game packet, heartbeat or connection packet is received.

Now extended the server to only send connect packets to the client once fully connected.

It would still be possible to send non-connect packets to the client and not have the connect complete.

I think it is necessary in this case to have a mode of operation where heartbeat is sent until acked at the given rate, ignoring other packets being sent.

Perhaps packets other than heartbeat are not allowed to be sent until fully connected, alternatively, have a time last heartbeat sent and only consider that. Which is best?

The simplest is to just force sending of heartbeat until fully connected. Implemented. See "lastHeartBeatSendTime".

Client and server now have messages to send and receive packets.

Now implement a simple unit test

Implement a client/server centric unit test for sending and receiving messages.

client -> server messages are getting through, but server -> client messages are not.

Missing connection packet process for client?

Seems that acks are not getting through properly for the client side of the connection. Is it missing advancing time?

Yes. This fixed it.

Sometimes the test is failing. I think the messages are getting stuck. Confirmed.

Need to setup a soak test to prove/disprove this.

Setup a soak test based on client_server.cpp

Why is it always server -> client messages that don't always get through?

Shouldn't they both behave the same? client -> server and server -> client?

OK. Confirmed it does happen for both. It's most likely a conection specific message hang then.

Seems to stop on message #48 a lot for some reason?

Try to work out why it's hanging...

Early indication so far suggests that the sender isn't advancing forward with acks, so is resending the same old messages over and over.

Reduced the test to just have server -> client message stream to make log inspection easier.

Basically, what is going on here is that the receiver is getting messages in a particular packet, but the sender is not seeing that packet acked.

But wait, since it then continues to send the same messages over and over, the sender should eventually see an ack for one of those packets, and move forward. But it's not, so are acks totally broken here somehow?!

Oh great, it's because if it's a single direction of packet, the connection packets aren't sent unless there is packet data to send.

So when the packet sending completes, it stops acking, which fucks up stuff in progress of being sent = race condition.

Solution: need to send connection packets all the time, even if packet data isn't there to be sent, for acks to work.

That was it. Confirmed fixed!


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

First order of business, restricting "max messages per-packet" to a fixed constant # is not appropriate.

Modify this so it is dynamically allocated memory inside the packet. But from which allocator?

I already have a pointer to the message allocator inside the connection packet, and I can use this to get the message allocator.

That seems kind of appropriate, seeing as that block of memory will be storing pointers to messages.

But wait, how will I know max # of messages per-packet during serialization?

Adding connection config struct, and putting this in the context.

Fixed some bugs and now it's working. MaxMessagesPerPacket constant has been removed.


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
