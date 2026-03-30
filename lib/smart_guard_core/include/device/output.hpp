#pragma once

#include <data.hpp>
#include <functional>

/**
* An interface representing a hardware output terminal
*
* @param Data type of data to write
*/
template<typename Data>
class Output
{
public:
    /**
    * Writes data to output terminal
    *
    * @param data Value to be written
    */
    virtual void write(Data data) = 0;

    virtual ~Output() = default;
};

template<typename Data>
class OutputFn : public Output<Data>
{
public:
    using functor_t = std::function<void(Data)>;

private:
    functor_t _functor;

public:
    OutputFn(functor_t functor):
        _functor(std::move(functor))
    {

    }

    virtual void write(Data data) override
    {
        (this->_functor)(std::move(data));
    }
};