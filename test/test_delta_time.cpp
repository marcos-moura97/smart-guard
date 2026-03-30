#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"
#include "process/process.hpp"
#include "process/repeat_until.hpp"
#include "delta_time.hpp"
#include <chrono>
#include <cstdint>
#include <limits>
#include <thread>
#include <print>

TEST_GROUP(DeltaTime)
{

};

/// Asserts that delta-time gives a value roughly corresponding to the sleep delay in the loop.
TEST(DeltaTime, ItWorks)
{
    constexpr std::size_t TOTAL_STEPS {10};
    constexpr std::chrono::milliseconds DT_TARGET {1000};

    /// It can miss the target time by this much
    constexpr std::chrono::milliseconds DT_MISS {200};

    DeltaTime delta_time;
    for(std::size_t i; i < TOTAL_STEPS; ++i)
    {
        std::this_thread::sleep_for(DT_TARGET);
        delta_time.tick();
        std::chrono::microseconds dt {delta_time.dt()};
        std::chrono::microseconds dt_error {std::chrono::abs(dt - DT_TARGET)};
        std::printf("%li", dt.count());
        CHECK(DT_TARGET > dt_error);
    }
}