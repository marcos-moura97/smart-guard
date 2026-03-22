#pragma once

#include <cassert>
#include <ranges>
#include <algorithm>
#include <concepts>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace ml
{
    template<
        std::forward_iterator K,
        std::forward_iterator Y,
        typename X,
        std::invocable<const X&, const std::iter_value_t<Y>&> D,
        std::invocable<const std::invoke_result_t<D, const X&, const std::iter_value_t<Y>&>&, const std::invoke_result_t<D, const X&, const std::iter_value_t<Y>&>&> C
    >
    K knn(
        K k_begin,
        K k_end,
        Y y_begin,
        Y y_end,
        const X& x,
        D&& distance_function,
        C&& distance_cmp
    )
    {
        using distance_t = std::invoke_result_t<D, const X&, std::iter_value_t<Y>>;

        K k_full {k_begin};
        std::vector<distance_t> distances;
        distances.reserve(std::distance(k_begin, k_end));
        for(; y_begin != y_end; ++y_begin)
        {
            distance_t distance {distance_function(x, *y_begin)};
            K k_iter {k_begin};
            if(k_iter != k_full)
            {
                typename std::vector<distance_t>::iterator distance_iter {distances.begin()};
                typename std::vector<distance_t>::iterator max_distance {distance_iter};
                K max_k {k_iter};
                ++distance_iter;
                ++k_iter;
                while(k_iter != k_full)
                {
                    if((distance_cmp)(static_cast<const distance_t&>(*max_distance), static_cast<const distance_t&>(*distance_iter)) < 0)
                    {
                        max_distance = distance_iter;
                        max_k = k_iter;
                    }
                    ++distance_iter;
                    ++k_iter;
                }
                if((distance_cmp)(static_cast<const distance_t&>(distance), static_cast<const distance_t&>(*max_distance)) < 0)
                {
                    *max_distance = distance;
                    *max_k = y_begin;
                    continue;
                }
            }
            distances.push_back(distance);
            ++k_full;
        }
        return k_full;
    }

    template<
        std::forward_iterator K,
        std::forward_iterator Y,
        typename X,
        std::invocable<const X&, const std::iter_value_t<Y>&> D
    >
    K knn(
        K k_begin,
        K k_end,
        Y y_begin,
        Y y_end,
        const X& x,
        D&& distance_function
    )
    {
        return knn(
            std::move(k_begin),
            std::move(k_end),
            std::move(y_begin),
            std::move(y_end),
            x,
            std::forward<D>(distance_function),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs <=> rhs;
            }
        );
    }

    template<
        std::ranges::range K,
        std::forward_iterator Y,
        typename X,
        std::invocable<const X&, const std::iter_value_t<Y>&> D,
        std::invocable<const std::invoke_result_t<D, const X&, const std::iter_value_t<Y>&>&, const std::invoke_result_t<D, const X&, const std::iter_value_t<Y>&>&> C
    >
    std::ranges::iterator_t<K> knn(
        K& k,
        Y y_begin,
        Y y_end,
        const X& x,
        D&& distance_function,
        C&& distance_cmp
    )
    {
        return knn(
            k.begin(),
            k.end(),
            std::move(y_begin),
            std::move(y_end),
            x,
            std::forward<D>(distance_function),
            std::forward<C>(distance_cmp)
        );
    }

    template<
        std::ranges::range K,
        std::forward_iterator Y,
        typename X,
        std::invocable<const X&, const std::iter_value_t<Y>&> D
    >
    std::ranges::iterator_t<K> knn(
        K& k,
        Y y_begin,
        Y y_end,
        const X& x,
        D&& distance_function
    )
    {
        return knn(
            k.begin(),
            k.end(),
            std::move(y_begin),
            std::move(y_end),
            x,
            std::forward<D>(distance_function)
        );
    }

    template<
        std::forward_iterator K,
        std::ranges::range Y,
        typename X,
        std::invocable<const X&, const std::ranges::range_value_t<Y>&> D,
        std::invocable<const std::invoke_result_t<D, const X&, const std::ranges::range_value_t<Y>&>&, const std::invoke_result_t<D, const X&, const std::ranges::range_value_t<Y>&>&> C
    >
    K knn(
        K k_begin,
        K k_end,
        const Y& y,
        const X& x,
        D&& distance_function,
        C&& distance_cmp
    )
    {
        return knn(
            std::move(k_begin),
            std::move(k_end),
            y.cbegin(),
            y.cend(),
            x,
            std::forward<D>(distance_function),
            std::forward<C>(distance_cmp)
        );
    }

    template<
        std::forward_iterator K,
        std::ranges::range Y,
        typename X,
        std::invocable<const X&, const std::ranges::range_value_t<Y>&> D
    >
    K knn(
        K k_begin,
        K k_end,
        const Y& y,
        const X& x,
        D&& distance_function
    )
    {
        return knn(
            std::move(k_begin),
            std::move(k_end),
            y.cbegin(),
            y.cend(),
            x,
            std::forward<D>(distance_function)
        );
    }

    template<
        std::ranges::range K,
        std::ranges::range Y,
        typename X,
        std::invocable<const X&, const std::ranges::range_value_t<Y>&> D,
        std::invocable<const std::invoke_result_t<D, const X&, const std::ranges::range_value_t<Y>&>&, const std::invoke_result_t<D, const X&, const std::ranges::range_value_t<Y>&>&> C
    >
    std::ranges::iterator_t<K> knn(
        K& k,
        const Y& y,
        const X& x,
        D&& distance_function,
        C&& distance_cmp
    )
    {
        return knn(
            k.begin(),
            k.end(),
            y.cbegin(),
            y.cend(),
            x,
            std::forward<D>(distance_function),
            std::forward<C>(distance_cmp)
        );
    }

    template<
        std::ranges::range K,
        std::ranges::range Y,
        typename X,
        std::invocable<const X&, const std::ranges::range_value_t<Y>&> D
    >
    std::ranges::iterator_t<K> knn(
        K& k,
        const Y& y,
        const X& x,
        D&& distance_function
    )
    {
        return knn(
            k.begin(),
            k.end(),
            y.cbegin(),
            y.cend(),
            x,
            std::forward<D>(distance_function)
        );
    }
}