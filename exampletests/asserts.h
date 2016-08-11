#include "gtest/gtest.h"

testing::AssertionResult isAddress6Match(const uint16_t * actual, const uint16_t * expected)
{
    for ( int i = 0; i < 8; ++i ) {
        uint16_t fragActual = actual[i];
        uint16_t fragExpected = htons(expected[i]);

        if(fragActual != fragExpected)
            return testing::AssertionFailure()
                << "ip fragment " << (i+1) << " does not match "
                << fragActual << " != " << fragExpected;
    }

    return testing::AssertionSuccess();
}
