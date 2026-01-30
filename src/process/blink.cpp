#include <cstdio>
#include <hardware/gpio.h>
#include <process/blink.hpp>

#ifdef PICO_BOARD_PICO2_W
#include "pico/cyw43_arch.h"
#endif

Blink::Blink(void):
    _led(false)
{
    printf("Blink process started!");
}

Process::Update Blink::update(void)
{
    this->_led.toggle();

    printf("Blinking!");

    return Update::pending;
}