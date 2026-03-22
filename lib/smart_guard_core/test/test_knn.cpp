#include "CppUTest/TestHarness.h"
#include "ml/knn.hpp"
#include <array>
#include <numeric>

TEST_GROUP(Knn)
{
};

TEST(Knn, ItWorks)
{
    std::array<std::array<float, 3>, 3> variants {
        std::array{-1.0f, 0.0f, 1.0f},
        std::array{1.0f, 0.0f, 1.0f},
        std::array{1.0f, 0.0f, -1.0f}  
    };

    std::array<float, 3> value {1.0, 0.0, 1.0};

    std::array<std::array<std::array<float, 3>, 3>::const_iterator, 3> k;

    using t = std::iter_reference_t<std::array<std::array<float, 3>, 3>::iterator>;
    ml::knn(k.begin(), k.end(), variants.begin(), variants.end(), value,
        [](const auto& x, const auto& y) -> double
        {
            return std::transform_reduce(x.begin(), x.end(), y.begin(), 0.0f,
                [](double a, double b)
                {
                    return a + b;
                },
                [](double a, double b)
                {
                    return (a - b)*(a - b);
                }
            );
        }
    );

    // TODO: how do i assert using CppUTest?

    FAIL("Fail me!");
}