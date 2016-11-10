/*
    Yojimbo Client/Server Network Protocol Library.

    Copyright © 2016, The Network Protocol Company, Inc.

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
#include "yojimbo_context.h"

namespace yojimbo
{
    ContextManager::ContextManager()
    {
        ResetContextMappings();
    }

    bool ContextManager::AddContextMapping( const Address & address, Allocator & streamAllocator, PacketFactory & packetFactory, void * contextData )
    {
        for ( int i = 0; i < m_numContextMappings; ++i )
        {
            if ( m_address[i] == address )
            {
                m_context[i].streamAllocator = &streamAllocator;
                m_context[i].packetFactory = &packetFactory;
                m_context[i].contextData = contextData;
                return true;
            }
        }

        for ( int i = 0; i < MaxContextMappings; ++i )
        {
            if ( m_context[i].streamAllocator == NULL )
            {
                m_address[i] = address;
                m_context[i].streamAllocator = &streamAllocator;
                m_context[i].packetFactory = &packetFactory;
                m_context[i].contextData = contextData;
                if ( i + 1 > m_numContextMappings )
                    m_numContextMappings = i + 1;
                return true;
            }
        }

#if YOJIMBO_DEBUG_SPAM
        char addressString[MaxAddressLength];
        address.ToString( addressString, MaxAddressLength );
        debug_printf( "failed to add context mapping for %s\n", addressString );
#endif // #if YOJIMBO_DEBUG_SPAM

        return false;
    }

    bool ContextManager::RemoveContextMapping( const Address & address )
    {
        for ( int i = 0; i < m_numContextMappings; ++i )
        {
            if ( m_address[i] == address )
            {
                m_address[i] = Address();
                m_context[i] = Context();

                if ( i + 1 == m_numContextMappings )
                {
                    int index = i - 1;
                    while ( index >= 0 )
                    {
                        if ( m_context[index].streamAllocator )
                            break;
                        index--;
                    }
                    m_numContextMappings = index + 1;
                }

                return true;
            }
        }

#if YOJIMBO_DEBUG_SPAM
        char addressString[MaxAddressLength];
        address.ToString( addressString, MaxAddressLength );
        debug_printf( "failed to remove context mapping for %s\n", addressString );
#endif // #if YOJIMBO_DEBUG_SPAM

        return false;
    }

    void ContextManager::ResetContextMappings()
    {
        m_numContextMappings = 0;
        
        for ( int i = 0; i < MaxContextMappings; ++i )
            m_address[i] = Address();

        memset( m_context, 0, sizeof( m_context ) );
    }

    const Context * ContextManager::GetContext( const Address & address ) const
    {
        for ( int i = 0; i < m_numContextMappings; ++i )
        {
            if ( m_address[i] == address )
                return &m_context[i];
        }
        return NULL;
    }
}
