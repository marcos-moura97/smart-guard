#pragma once
#include <array>

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

    const std::array<float, LENGTH> numer;

    std::array<float, ORDER> buffer;

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

// IMPL ###############################################################################################################

template<std::size_t ORDER>
constexpr FirFilter<ORDER>::FirFilter(std::array<float, LENGTH> numer):
    numer(std::move(numer)),
    buffer()
{
    // Are arrays zeroed when default-constructed?
    // Zeroing explicitly just to be sure. Not sure if necessary.
    this->buffer.fill(0);
}

template<std::size_t ORDER>
float FirFilter<ORDER>::operator()(float x)
{
    // This is the FIR filter algorithm
    // `x` is pushed into the buffer.
    // Each delayed value of `x` in the buffer is multiplied with its corresponding coefficient, then summed.
    // Should work with any filter order
    float y {0.0};
    auto b {this->numer.crbegin()};
    if(b != this->numer.crend())
    {
        y += x**b;
        for(float& w : this->buffer)
        {   
            ++b;
            std::swap(w, x);
            y += x**b;
        }
    }
    return y;
}