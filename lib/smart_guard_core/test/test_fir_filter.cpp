#include "CppUTest/TestHarness.h"
#include "dsp/fir_filter.hpp"
#include <array>
#include <cstddef>

TEST_GROUP(FirFilter)
{
};

TEST(FirFilter, FirIdentity)
{
    using f_t = float;

    constexpr std::size_t ORDER {1};
    constexpr std::size_t N {ORDER + 1};
    constexpr f_t PRECISION {1e-3}; // how low can we go?

    const std::array<f_t, N> h {1.0, 1.0};
    std::array<f_t, N> x {1.0};

    dsp::FirFilter<ORDER> filter {h};

    for(std::size_t n {0}; n < N; ++n)
    {
        x[n] = filter(x[n]);
        CHECK_TRUE(std::abs(x[n] - h[n]) <= PRECISION);
    }
}