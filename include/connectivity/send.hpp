#pragma once

#include <data.hpp>

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
};