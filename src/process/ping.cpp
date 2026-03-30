#include <cstdio>
#include <hardware/gpio.h>
#include <process/ping.hpp>

process::Ping::Ping(std::function<void()> on_update): 
    _on_update(std::move(on_update))
{
    std::printf("Ping process started!");
}

process::Update process::Ping::update(duration_t dt)
{
    if(this->_on_update)
    {
        (this->_on_update)();
    }
    return Update::pending;
}