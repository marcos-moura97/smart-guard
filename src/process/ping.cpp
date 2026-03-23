#include <cstdio>
#include <hardware/gpio.h>
#include <process/ping.hpp>

process::Ping::Ping(std::function<void()> on_update): 
    _on_update(on_update)
{
    std::printf("Ping process started!");
}

process::Update process::Ping::update(void)
{
    if(this->_on_update)
    {
        this->_on_update();
    }
    return Update::pending;
}