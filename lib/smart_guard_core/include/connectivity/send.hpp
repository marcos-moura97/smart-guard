#pragma once

#include <data.hpp>
#include <functional>

/**
* An interface that can send a certain type of Data.
*
* @param Data type of data to send
*/
template<typename Data>
class Send
{
public:
    /**
    * Sends the data to some client.
    *
    * @param data Data payload
    *
    * @return True if successful
    */
    virtual bool send(Data data) = 0;

    virtual ~Send() = default;
};

template<typename Data>
class SendFn : public Send<Data>
{
public:
    using functor_t = std::function<bool(Data)>;

private:
    functor_t _functor;

public:
    SendFn(functor_t functor):
        _functor(std::move(functor))
    {

    }

    virtual bool send(Data data) override
    {
        return (this->_functor)(std::move(data));
    }
};