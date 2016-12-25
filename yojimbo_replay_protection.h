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

#ifndef YOJIMBO_REPLAY_PROTECTION_H
#define YOJIMBO_REPLAY_PROTECTION_H

#include "yojimbo_config.h"
#include "yojimbo_allocator.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

namespace yojimbo
{
    // todo: document this file

    class ReplayProtection
    {
    public:

        ReplayProtection()
        {
            Reset( 0 );
        }

        void Reset( uint64_t mostRecentSequence = 0LL )
        {
            m_mostRecentSequence = mostRecentSequence;
            memset( m_receivedPacket, 0xFF, sizeof( m_receivedPacket ) );
        }

        bool PacketAlreadyReceived( uint64_t sequence )
        {
            // IMPORTANT: Global server packets don't follow the same sequencing as per-client packets
            // These packets are marked with high bit of sequence set to 1. Ignore them completely.
            if ( sequence & ( 1LL << 63 ) )
                return false;

            if ( sequence + ReplayProtectionBufferSize <= m_mostRecentSequence )
                return true;
            
            if ( sequence > m_mostRecentSequence )
                m_mostRecentSequence = sequence;

            const int index = (int) ( sequence % ReplayProtectionBufferSize );

            if ( m_receivedPacket[index] == 0xFFFFFFFFFFFFFFFFLL )
            {
                m_receivedPacket[index] = sequence;
                return false;
            }

            if ( m_receivedPacket[index] >= sequence )
                return true;
            
            m_receivedPacket[index] = sequence;

            return false;
        }

        uint64_t GetMostRecentSequence() const { return m_mostRecentSequence; }

    protected:

        uint64_t m_mostRecentSequence;
        uint64_t m_receivedPacket[ReplayProtectionBufferSize];
    };
}

#endif // #ifndef YOJIMBO_REPLAY_PROTECTION_H
