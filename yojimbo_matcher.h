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

#ifndef YOJIMBO_MATCHER_H
#define YOJIMBO_MATCHER_H

#include "yojimbo_config.h"
#include "yojimbo_allocator.h"
#include "yojimbo_tokens.h"

namespace yojimbo
{
    /**
        Matcher status enum.

        Designed for when the matcher will be made non-blocking. Currently the matcher is blocking.
     */

    enum MatcherStatus
    {
        MATCHER_IDLE,                                                       ///< The matcher is idle.
        MATCHER_BUSY,                                                       ///< The matcher is busy requesting a match.
        MATCHER_READY,                                                      ///< The matcher is finished requesting a match. The match response is ready to read with Matcher::GetMatchResponse.
        MATCHER_FAILED                                                      ///< The matcher failed to find a match.
    };

    /** 
        Match response sent back from matcher for match request.

        IMPORTANT: This response is transmitted over HTTPS because it contains encryption keys for packets sent between the client and server.

        @see Client::Connect
        @see ConnectToken
     */

    struct MatchResponse
    {
        MatchResponse()
        {
            numServerAddresses = 0;
            memset( connectTokenData, 0, sizeof( connectTokenData ) );
            memset( connectTokenNonce, 0, sizeof( connectTokenNonce ) );
            memset( clientToServerKey, 0, sizeof( clientToServerKey ) );
            memset( serverToClientKey, 0, sizeof( serverToClientKey ) );
            connectTokenExpireTimestamp = 0;
        }

        int numServerAddresses;                                             ///< The number of server addresses to connect to in [1,MaxServersPerConnect].
        Address serverAddresses[MaxServersPerConnect];                      ///< The array of server addresses that the client can connect to, in order of first to last to try.
        uint8_t connectTokenData[ConnectTokenBytes];                        ///< The encrypted connect token data.
        uint8_t connectTokenNonce[NonceBytes];                              ///< The nonce the connect token was encrypted with.
        uint8_t clientToServerKey[KeyBytes];                                ///< The key for client to server encrypted packets.
        uint8_t serverToClientKey[KeyBytes];                                ///< The key for server to client encrypted packets.
        uint64_t connectTokenExpireTimestamp;                               ///< The timestamp at which this connect token expires.
    };

    /**
        The matcher class.

        This is a stub class for HTTPS requests to matcher.go.

        It will be improved in the future, most importantly to make Matcher::RequestMatch a non-blocking operation.
     */

    class Matcher
    {
    public:

        explicit Matcher( Allocator & allocator );
       
        ~Matcher();

        bool Initialize();

        void RequestMatch( uint64_t protocolId, uint64_t clientId );

        MatcherStatus GetStatus();

        void GetMatchResponse( MatchResponse & matchResponse );

    protected:

        bool ParseMatchResponse( const char * json, MatchResponse & matchResponse );

    private:

        Allocator * m_allocator;
        bool m_initialized;
        MatcherStatus m_status;
        MatchResponse m_matchResponse;
        struct MatcherInternal * m_internal;
    };
}

#endif // #ifndef YOJMIBO_MATCHER_H
