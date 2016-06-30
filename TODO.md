# DONE

matcher.go was encoding strings as base64 without including the terminating null character, because go strings aren't null terminated. Fixed this, and then also added a check to the base64 decode string to abort the decode if the last character in the decoded string isn't '\0'. This fixes an attack vector for crashing out the C++ code that parses base64 strings in JSON.

Implemented a windows version of "premake5 stress". It does reproduce the same crash in the server. Something is definitely wrong here.


# TODO

Time to install an evaluation copy of Boundschecker to see what's going on?

Wow. Doesn't seem like boundschecker really exists anymore. Everythings seems acquired a few times by multiple companies and way too expensive. No free eval anymore either. Whatever.

Looks like I'm going to be tracking this one down with open source tools. I'll need to add my own instrumentation. Note that it could also be a logic error. There is no guarantee that it is a memory trash.

First start by adding a magic value in the base packet (debug build only) which is set only if the object is created through the packet factory. That way if somehow a packet is getting passed into free which has not been created via the packet factory, I will pick it up. Also, bad pointers and so on.

Moving back to Mac...