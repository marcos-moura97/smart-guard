#pragma once
#include <array>

namespace dsp
{
    /**
    * An FIR (finite impulse response) discrete-time filter.
    * The filter order is configurable.
    * @param ORDER Filter order.
    */
    template<std::size_t ORDER>
    class FirFilter final
    {
    private:
        static constexpr std::size_t LENGTH {ORDER + 1};

        const std::array<float, LENGTH> _numer;

        std::array<float, ORDER> _buffer;

    public:
        FirFilter(void) = delete;

        /**
        * Initialize a FIR filter with the given coefficients
        * @param numer Numerator coefficients of filter
        */
        constexpr FirFilter(std::array<float, LENGTH> numer);

        /**
        * Runs the FIR filter SISO algorithm
        * @param x Filter input
        * @return Filter output
        */
        float operator()(float x);
    };
}

#include <dsp/fir_filter.tpp>