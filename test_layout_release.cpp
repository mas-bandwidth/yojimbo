/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2026, Más Bandwidth LLC.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// One half of the cross-configuration layout test (YJ-04). This translation unit compiles the
// public headers with YOJIMBO_RELEASE forced on, whatever the build type is, and reports the
// sizes of the two classes consumers derive from. Its twin does the same for the other
// configuration, and test.cpp checks the two agree with each other and with the build's own.
//
// Nothing here is instantiated, so forcing the configuration costs no code: sizeof needs the
// class definitions and nothing else.

#define YOJIMBO_RELEASE

// serialize.h's own test suite defines free functions at namespace scope, so it must be
// compiled into exactly one translation unit of the test binary -- test.cpp, which calls it.
#undef SERIALIZE_ENABLE_TESTS

#include "yojimbo_allocator.h"
#include "yojimbo_message.h"

#include <stddef.h>

void yojimbo_test_layout_release( size_t & allocatorSize, size_t & messageFactorySize )
{
    allocatorSize = sizeof( yojimbo::Allocator );
    messageFactorySize = sizeof( yojimbo::MessageFactory );
}
