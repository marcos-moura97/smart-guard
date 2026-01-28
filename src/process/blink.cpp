#include <hardware/gpio.h>
#include <process/blink.hpp>

#ifdef PICO_BOARD_PICO2_W
#include "pico/cyw43_arch.h"
#endif

Blink::Blink(void):
    _led(false)
{

}

Process::Update Blink::update(void)
{
    this->_led.toggle();

    return Update::pending;
}