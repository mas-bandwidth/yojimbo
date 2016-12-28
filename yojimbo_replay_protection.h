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
    /**
        Provides protection against packets being sniffed and replayed.
        
        Basically a ring buffer that stores packets indexed by packet sequence number modulo replay buffer size.

        The logic is pretty much:

            1. If a packet is older than what can be stored in the buffer, ignore it. 

            2. If an entry in the reply buffer exists and matches the packet sequence, it's already been received, so ignore it.

            3. Otherwise, this is the first time the packet has been received, so let it in.

        The whole point is to avoid the possibility of an attacker capturing and replaying encrypted packets, in an attempt to break some internal protocol state, like packet level acks or reliable-messages.

        Without this protection, that would be reasonably easy to do. Just capture a packet and then keep replaying it. Eventually the packet or message sequence number would wrap around and you'd corrupt the connection state for that client.
     */

    class ReplayProtection
    {
    public:

        /**
            Replay protection constructor.

            Starts at packet sequence number 0.
         */

        ReplayProtection()
        {
            Reset( 0 );
        }

        /**
            Reset the replay protection buffer at a particular sequence number.

            @param mostRecentSequence The sequence number to start at, defaulting to zero.
         */

        void Reset( uint64_t mostRecentSequence = 0LL )
        {
            m_mostRecentSequence = mostRecentSequence;
            memset( m_receivedPacket, 0xFF, sizeof( m_receivedPacket ) );
        }

        /**
            Has a packet already been received with this sequence?

            IMPORTANT: Global packets sent by the server (packets that don't correspond to any connected client) have the high bit of the sequence number set to 1. These packets should NOT have replay protection applied to them.

            This is not a problem in practice, as modification of the packet sequence number by an attacker would cause it to fail to decrypt (the sequence number is the nonce).

            @param sequence The packet sequence number.

            @returns True if you should ignore this packet, because it's potentially a replay attack. False if it's OK to process the packet.
         */

        bool PacketAlreadyReceived( uint64_t sequence )
        {
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

        /**
            Get the most recent packet sequence number received.

            Packets older than this sequence number minus yojimbo::ReplayProtectionBufferSize are discarded. This way we can keep the replay protection buffer small (about one seconds worth).

            @returns The most recent packet sequence number.
         */

        uint64_t GetMostRecentSequence() const { return m_mostRecentSequence; }

    protected:

        uint64_t m_mostRecentSequence;                                  ///< The most recent sequence number received.

        uint64_t m_receivedPacket[ReplayProtectionBufferSize];          ///< The ring buffer that stores packet sequence numbers at index sequence modulo buffer size.
    };
}

#endif // #ifndef YOJIMBO_REPLAY_PROTECTION_H
