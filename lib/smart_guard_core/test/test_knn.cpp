#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"
#include "ml/knn.hpp"
#include <array>
#include <numeric>

TEST_GROUP(Knn)
{

};

/// Tests a single case for the K-Nearest-Neighbour algorithm
TEST(Knn, ItWorks)
{
    constexpr std::size_t K {3};
    constexpr std::size_t N {3};

    using x_t = std::array<float, N>;
    using y_t = std::array<x_t, K>;
    using k_t = std::array<y_t::const_iterator, K>;

    auto distance_fn {
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
    };
    auto distance_cmp {
        [](const auto& lhs, const auto& rhs)
        {
            return lhs <=> rhs;
        }
    };

    y_t variants {
        std::array{-1.0f, 0.0f, 1.0f},
        std::array{1.0f, 0.0f, 1.0f},
        std::array{0.0f, 0.0f, 1.0f}  
    };

    x_t value {1.0, 0.0, 1.0};

    k_t ranking;
    std::array<std::ptrdiff_t, K> correct_ranking {
        1,
        2,
        0
    };

    auto test {
        [&](auto end)
        {
            CHECK_TRUE(end == ranking.end());
            auto begin {variants.cbegin()};
            for(std::size_t k {0}; k < K; ++k)
            {
                std::ptrdiff_t distance {std::distance(begin, ranking[k])};
                CHECK_TRUE(distance == correct_ranking[k]);
            }
        }
    };

    test(ml::knn(ranking, variants, value, distance_fn));
    test(ml::knn(ranking, variants, value, distance_fn, distance_cmp));
    test(ml::knn(ranking.begin(), ranking.end(), variants, value, distance_fn));
    test(ml::knn(ranking.begin(), ranking.end(), variants, value, distance_fn, distance_cmp));
    test(ml::knn(ranking, variants.begin(), variants.end(), value, distance_fn));
    test(ml::knn(ranking, variants.begin(), variants.end(), value, distance_fn, distance_cmp));
    test(ml::knn(ranking.begin(), ranking.end(), variants.begin(), variants.end(), value, distance_fn));
    test(ml::knn(ranking.begin(), ranking.end(), variants.begin(), variants.end(), value, distance_fn, distance_cmp));
}