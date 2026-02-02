#pragma once
#include <dsp/fir_filter.hpp>

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