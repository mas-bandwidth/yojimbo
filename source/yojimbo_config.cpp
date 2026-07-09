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
#include "yojimbo_platform.h"

#include <math.h>

namespace yojimbo
{
#ifdef YOJIMBO_DEBUG

    // Print what is wrong with the config (with the offending values), then assert. Debug builds
    // only: config validation follows the library-wide contract that asserts enforce correct usage
    // in debug and the programmer has already validated their integration before shipping release.
    #define YOJIMBO_CONFIG_CHECK( condition, ... )                                                  \
        do                                                                                          \
        {                                                                                           \
            if ( !( condition ) )                                                                   \
            {                                                                                       \
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, __VA_ARGS__ );                             \
                yojimbo_assert( condition );                                                        \
            }                                                                                       \
        } while ( 0 )

    void ChannelConfig::Validate( int channelIndex, int maxPacketSize ) const
    {
        YOJIMBO_CONFIG_CHECK( messageSendQueueSize > 0,
            "error: invalid config: channel %d messageSendQueueSize (%d) must be > 0\n", channelIndex, messageSendQueueSize );

        YOJIMBO_CONFIG_CHECK( messageReceiveQueueSize > 0,
            "error: invalid config: channel %d messageReceiveQueueSize (%d) must be > 0\n", channelIndex, messageReceiveQueueSize );

        YOJIMBO_CONFIG_CHECK( maxMessagesPerPacket > 0,
            "error: invalid config: channel %d maxMessagesPerPacket (%d) must be > 0\n", channelIndex, maxMessagesPerPacket );

        if ( !disableBlocks )
        {
            YOJIMBO_CONFIG_CHECK( maxBlockSize > 0,
                "error: invalid config: channel %d maxBlockSize (%d) must be > 0\n", channelIndex, maxBlockSize );
        }

        if ( type == CHANNEL_TYPE_RELIABLE_ORDERED )
        {
            // The reliable-ordered channel stores its state in sequence buffers indexed by
            // sequence % size, which only works when the size divides 65536 (ie. a power of two).
            // Any other size aliases sequence numbers and corrupts channel state.

            YOJIMBO_CONFIG_CHECK( sentPacketBufferSize > 0,
                "error: invalid config: channel %d sentPacketBufferSize (%d) must be > 0\n", channelIndex, sentPacketBufferSize );

            YOJIMBO_CONFIG_CHECK( ( 65536 % sentPacketBufferSize ) == 0,
                "error: invalid config: channel %d sentPacketBufferSize (%d) must be a power of two\n", channelIndex, sentPacketBufferSize );

            YOJIMBO_CONFIG_CHECK( ( 65536 % messageSendQueueSize ) == 0,
                "error: invalid config: channel %d messageSendQueueSize (%d) must be a power of two\n", channelIndex, messageSendQueueSize );

            YOJIMBO_CONFIG_CHECK( ( 65536 % messageReceiveQueueSize ) == 0,
                "error: invalid config: channel %d messageReceiveQueueSize (%d) must be a power of two\n", channelIndex, messageReceiveQueueSize );

            if ( !disableBlocks )
            {
                YOJIMBO_CONFIG_CHECK( blockFragmentSize > 0,
                    "error: invalid config: channel %d blockFragmentSize (%d) must be > 0\n", channelIndex, blockFragmentSize );

                // A block fragment must fit inside a packet, or the channel stalls forever trying
                // to send the block (see ReliableOrderedChannel::GetPacketData).
                YOJIMBO_CONFIG_CHECK( blockFragmentSize <= maxPacketSize,
                    "error: invalid config: channel %d blockFragmentSize (%d) must be <= maxPacketSize (%d)\n", channelIndex, blockFragmentSize, maxPacketSize );

                // Fragment id 0xFFFF is reserved as a sentinel (see GetFragmentToSend).
                YOJIMBO_CONFIG_CHECK( GetMaxFragmentsPerBlock() <= 65535,
                    "error: invalid config: channel %d maxBlockSize (%d) / blockFragmentSize (%d) gives too many fragments per block (max 65535)\n", channelIndex, maxBlockSize, blockFragmentSize );
            }
        }
    }

    void ConnectionConfig::Validate() const
    {
        YOJIMBO_CONFIG_CHECK( numChannels >= 1 && numChannels <= MaxChannels,
            "error: invalid config: numChannels (%d) must be in [1,%d]\n", numChannels, MaxChannels );

        YOJIMBO_CONFIG_CHECK( maxPacketSize > 0,
            "error: invalid config: maxPacketSize (%d) must be > 0\n", maxPacketSize );

        for ( int i = 0; i < numChannels; ++i )
        {
            channel[i].Validate( i, maxPacketSize );
        }
    }

    void ClientServerConfig::Validate() const
    {
        ConnectionConfig::Validate();

        YOJIMBO_CONFIG_CHECK( clientMemory > 0,
            "error: invalid config: clientMemory (%d) must be > 0\n", clientMemory );

        YOJIMBO_CONFIG_CHECK( serverGlobalMemory > 0,
            "error: invalid config: serverGlobalMemory (%d) must be > 0\n", serverGlobalMemory );

        YOJIMBO_CONFIG_CHECK( serverPerClientMemory > 0,
            "error: invalid config: serverPerClientMemory (%d) must be > 0\n", serverPerClientMemory );

        if ( networkSimulator )
        {
            YOJIMBO_CONFIG_CHECK( maxSimulatorPackets > 0,
                "error: invalid config: maxSimulatorPackets (%d) must be > 0\n", maxSimulatorPackets );
        }

        YOJIMBO_CONFIG_CHECK( packetFragmentSize > 0,
            "error: invalid config: packetFragmentSize (%d) must be > 0\n", packetFragmentSize );

        YOJIMBO_CONFIG_CHECK( fragmentPacketsAbove > 0,
            "error: invalid config: fragmentPacketsAbove (%d) must be > 0\n", fragmentPacketsAbove );

        // maxPacketFragments is derived from maxPacketSize inside the ClientServerConfig
        // constructor, which runs *before* your code adjusts any fields. If you increase
        // maxPacketSize you must update maxPacketFragments too, otherwise packets larger than
        // packetFragmentSize * maxPacketFragments cannot be fragmented and are dropped.
        const int neededPacketFragments = (int) ceil( maxPacketSize / (double) packetFragmentSize );

        YOJIMBO_CONFIG_CHECK( maxPacketFragments >= neededPacketFragments,
            "error: invalid config: maxPacketFragments (%d) is too small for maxPacketSize (%d) with packetFragmentSize (%d). It must be at least %d.\n"
            "maxPacketFragments is computed in the ClientServerConfig constructor, so if you increase maxPacketSize afterwards, update maxPacketFragments as well\n",
            maxPacketFragments, maxPacketSize, packetFragmentSize, neededPacketFragments );

        // The reliable endpoint encodes (num fragments - 1) as a uint8_t and asserts max_fragments <= 256.
        YOJIMBO_CONFIG_CHECK( maxPacketFragments <= 256,
            "error: invalid config: maxPacketFragments (%d) must be <= 256\n", maxPacketFragments );

        YOJIMBO_CONFIG_CHECK( packetReassemblyBufferSize > 0,
            "error: invalid config: packetReassemblyBufferSize (%d) must be > 0\n", packetReassemblyBufferSize );

        YOJIMBO_CONFIG_CHECK( ackedPacketsBufferSize > 0,
            "error: invalid config: ackedPacketsBufferSize (%d) must be > 0\n", ackedPacketsBufferSize );

        YOJIMBO_CONFIG_CHECK( receivedPacketsBufferSize > 0,
            "error: invalid config: receivedPacketsBufferSize (%d) must be > 0\n", receivedPacketsBufferSize );
    }

#else // #ifdef YOJIMBO_DEBUG

    void ChannelConfig::Validate( int channelIndex, int maxPacketSize ) const
    {
        (void) channelIndex;
        (void) maxPacketSize;
    }

    void ConnectionConfig::Validate() const {}

    void ClientServerConfig::Validate() const {}

#endif // #ifdef YOJIMBO_DEBUG
}
