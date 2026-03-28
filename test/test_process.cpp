#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"
#include "process/process.hpp"
#include "process/repeat_until.hpp"
#include <cstdint>
#include <limits>

TEST_GROUP(Process)
{

};

/// Tests wether or not the process class works at all
TEST(Process, ItWorks)
{
    using i_t = std::int32_t;
    constexpr i_t N {std::numeric_limits<i_t>::max()};

    i_t n {0};
    process::RepeatUntil incrementor {
        [&n]()
        {
            if(n == N)
            {
                return process::Update::done;
            }
            ++n;
            return process::Update::pending;
        }
    };

    i_t i {0};
    while(incrementor.update() != process::Update::done)
    {
        ++i;
        CHECK_EQUAL(i, n);
    }
    CHECK_EQUAL(i, N);
    CHECK_EQUAL(n, N);
}