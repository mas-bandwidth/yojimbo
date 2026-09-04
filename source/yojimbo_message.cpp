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

#include "yojimbo_config.h"
#include "yojimbo_message.h"

#if YOJIMBO_DEBUG_MESSAGE_LEAKS
#include <map>
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS

namespace yojimbo
{
    // The message leak tracker lives here, behind MessageFactory::m_debug, instead of in the
    // public header. MessageFactory is a base class consumers derive from (usually through
    // YOJIMBO_MESSAGE_FACTORY_START), so a member whose presence depends on NDEBUG gave the class
    // two layouts: one for a debug build of the library and another for a release consumer that
    // included the same header. One pointer, always present, one layout.
    class MessageFactoryDebugState
    {
    public:

#if YOJIMBO_DEBUG_MESSAGE_LEAKS
        std::map<void*,int> allocated_messages;
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
    };

    MessageFactoryDebugState * yojimbo_message_factory_debug_create()
    {
#if YOJIMBO_DEBUG_MESSAGE_LEAKS
        // The tracker's own std::map allocates from the global heap, and always did; the state
        // object joins it there rather than routing through the factory's allocator.
        return new MessageFactoryDebugState();
#else // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
        return NULL;
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
    }

    void yojimbo_message_factory_debug_destroy( MessageFactoryDebugState * debug )
    {
#if YOJIMBO_DEBUG_MESSAGE_LEAKS
        yojimbo_assert( debug );
        if ( debug->allocated_messages.size() )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "you leaked messages!\n" );
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "%d messages leaked\n", (int) debug->allocated_messages.size() );
            typedef std::map<void*,int>::iterator itor_type;
            for ( itor_type i = debug->allocated_messages.begin(); i != debug->allocated_messages.end(); ++i )
            {
                Message * message = (Message*) i->first;
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "leaked message %p (type %d, refcount %d)\n", message, message->GetType(), message->GetRefCount() );
            }
            yojimbo_assert( false && "Message leaks detected, see log" );
        }
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
        delete debug;
    }

    void yojimbo_message_factory_debug_track_create( MessageFactoryDebugState * debug, Message * message )
    {
        (void) debug;
        (void) message;
#if YOJIMBO_DEBUG_MESSAGE_LEAKS
        yojimbo_assert( debug );
        debug->allocated_messages[message] = 1;
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
    }

    void yojimbo_message_factory_debug_track_release( MessageFactoryDebugState * debug, Message * message )
    {
        (void) debug;
        (void) message;
#if YOJIMBO_DEBUG_MESSAGE_LEAKS
        yojimbo_assert( debug );
        yojimbo_assert( debug->allocated_messages.find( message ) != debug->allocated_messages.end() );
        debug->allocated_messages.erase( message );
#endif // #if YOJIMBO_DEBUG_MESSAGE_LEAKS
    }
}
