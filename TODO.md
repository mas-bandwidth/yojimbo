# TODO

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
