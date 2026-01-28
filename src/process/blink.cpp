#include <hardware/gpio.h>
#include <process/blink.hpp>

#ifdef PICO_BOARD_PICO2_W
#include "pico/cyw43_arch.h"
#endif

Blink::Blink(void):
    status(false)
{

}

Process::Update Blink::update(void)
{
    this->status = !this->status;

#ifdef PICO_BOARD_PICO2_W
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, this->status);
#else
    gpio_put(PICO_DEFAULT_LED_PIN, this->status);
#endif

    return Update::pending;
}