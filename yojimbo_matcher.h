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

        Designed for when the matcher will be made non-blocking. The matcher is currently blocking in Matcher::RequestMatch
     */

    enum MatchStatus
    {
        MATCH_IDLE,                                                         ///< The matcher is idle.
        MATCH_BUSY,                                                         ///< The matcher is busy requesting a match.
        MATCH_READY,                                                        ///< The matcher is finished requesting a match. The match response is ready to read with Matcher::GetMatchResponse.
        MATCH_FAILED                                                        ///< The matcher failed to find a match.
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
        Communicates with the matcher web service over HTTPS.

        See docker/matcher/matcher.go for details. Launch the matcher via "premake5 matcher".

        This class will be improved in the future, most importantly to make Matcher::RequestMatch a non-blocking operation.
     */

    class Matcher
    {
    public:

        /**
            Matcher constructor.

            @param allocator The allocator to use for allocations.
         */

        explicit Matcher( Allocator & allocator );
       
        /**
            Matcher destructor.
         */

        ~Matcher();

        /**
            Initialize the matcher. 

            @returns True if the matcher initialized successfully, false otherwise.
         */

        bool Initialize();

        /** 
            Request a match.

            This is how clients get connect tokens from matcher.go. 

            They request a match and the server replies with a set of servers to connect to, and a connect token to pass to that server.

            IMPORTANT: This function is currently blocking. It will be made non-blocking in the near future.

            @param protocolId The protocol id that we are using. Used to filter out servers with different protocol versions.
            @param clientId A unique client identifier that identifies each client to your back end services. If you don't have this yet, just roll a random 64bit number.

            @see Matcher::GetMatchStatus
            @see Matcher::GetMatchResponse
         */

        void RequestMatch( uint64_t protocolId, uint64_t clientId );

        /**
            Get the current match status.

            Because Matcher::RequestMatch is currently blocking this will be MATCH_READY or MATCH_FAILED immediately after that function returns.

            If the status is MATCH_READY you can call Matcher::GetMatchResponse to get the match response data corresponding to the last call to Matcher::RequestMatch.

            @returns The current match status.
         */

        MatchStatus GetMatchStatus();

        /**
            Get match response data.

            This can only be called if the match status is MATCH_READY.

            @param matchResponse The match response data to fill [out].

            @see Matcher::RequestMatch
            @see Matcher::GetMatchStatus
         */

        void GetMatchResponse( MatchResponse & matchResponse );

    protected:

        /**
            Helper function to parse the match response JSON into the MatchResponse struct.

            @param json The JSON match response string to parse.
            @param matchResponse The match response structure to fill [out].

            @returns True if the match response JSON was successfully parsed, false otherwise.
         */

        bool ParseMatchResponse( const char * json, MatchResponse & matchResponse );

    private:

        Allocator * m_allocator;                                ///< The allocator passed into the constructor.

        bool m_initialized;                                     ///< True if the matcher was successfully initialized. See Matcher::Initialize.
        
		MatchStatus m_matchStatus;                              ///< The current match status.
        
		MatchResponse m_matchResponse;                          ///< The match response status from the last call to Matcher::RequestMatch if the match status is MATCH_READY.
        
		struct MatcherInternal * m_internal;                    ///< Internal match data is contained in this structure here so we don't have to spill details of mbedtls library outside yojimbo_matcher.cpp
    };
}

#endif // #ifndef YOJMIBO_MATCHER_H
