#include <cstdio>
#include <hardware/gpio.h>
#include <process/blink.hpp>

Blink::Blink(void):
    _led(false)
{
    std::printf("Blink process started!");
}

Process::Update Blink::update(void)
{
    this->_led.toggle();

    std::printf("Blinking!");

    return Update::pending;
}