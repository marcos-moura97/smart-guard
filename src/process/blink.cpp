#include <cstdio>
#include <hardware/gpio.h>
#include <process/blink.hpp>

process::Blink::Blink(void):
    _led(false)
{
    std::printf("Blink process started!");
}

process::Update process::Blink::update(duration_t dt)
{
    this->_led.toggle();

    std::printf("Blinking!");

    return Update::pending;
}