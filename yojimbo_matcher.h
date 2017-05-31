/*
    Yojimbo Network Library.
    
    Copyright © 2016 - 2017, The Network Protocol Company, Inc.
*/

#ifndef YOJIMBO_MATCHER_H
#define YOJIMBO_MATCHER_H

#include "yojimbo_config.h"
#include "yojimbo_allocator.h"

/** @file */

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
            @param clientId A unique client identifier that identifies each client to your back end services. If you don't have this yet, just roll a random 64 bit number.

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

        void GetMatchResponse( uint8_t * matchResponse );

    private:

        Allocator * m_allocator;                                ///< The allocator passed into the constructor.
        bool m_initialized;                                     ///< True if the matcher was successfully initialized. See Matcher::Initialize.
        MatchStatus m_matchStatus;                              ///< The current match status.
        struct MatcherInternal * m_internal;                    ///< Internals are in here to avoid spilling details of mbedtls library outside of yojimbo_matcher.cpp
        uint8_t m_matchResponse[ConnectTokenBytes];             ///< The match response data from the last call to Matcher::RequestMatch once the match status is MATCH_READY.
    };
}

#endif // #ifndef YOJMIBO_MATCHER_H
