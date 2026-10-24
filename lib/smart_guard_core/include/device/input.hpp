#pragma once

#include <data.hpp>
#include <functional>

/**
* An interface representing a hardware input terminal
*
* @param Data type of data to read
*/
template<typename Data>
class Input
{
public:
    /**
    * Reads data from input terminal
    *
    * @return Input value
    */
    virtual Data read(void) = 0;

    virtual ~Input() = default;
};

template<typename Data>
class InputFn : public Input<Data>
{
public:
    using functor_t = std::function<Data()>;

private:
    functor_t _functor;

public:
    InputFn(functor_t functor):
        _functor(std::move(functor))
    {

    }

    virtual Data read(void) override
    {
        return (this->_functor)();
    }
};