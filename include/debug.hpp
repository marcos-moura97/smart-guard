#pragma once

#include <hardware/gpio.h>
#include <pico/time.h>
#ifdef PICO_BOARD_PICO2_W
#include "pico/cyw43_arch.h"
#endif

#include <gpio.hpp>

/**
* Acts as a breakpoint to test if the code is reached.
* Turns the LED on and enters an infinite loop.
*/
void debug_exit(void)
{
    GpioOut<19> led {false};
    
    while(true) {
        sleep_ms(100);
        led.toggle();
    }
}