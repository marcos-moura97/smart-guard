#pragma once
#include <device/gpio.hpp>
#include <hardware/gpio.h>

#ifdef PICO_BOARD_PICO2_W
#include <pico/cyw43_arch.h>
#endif

template<uint PIN>
Gpio<PIN>::Gpio(void)
{
    gpio_init(PIN);
}

template<uint PIN>
uint Gpio<PIN>::pin(void) const
{
    return PIN;
}

template<uint PIN>
Gpio<PIN>::~Gpio(void)
{
    // TEMP: we might want to deinit, however this may cause weird behaviour with move semantics. may need to be revisited later
    // for now, don't deinit
    //gpio_deinit(PIN);
}

template<uint PIN>
GpioOut<PIN>::GpioOut(bool on):
    Gpio<PIN>(),
    _on(on)
{
    gpio_set_dir(PIN, GPIO_OUT);
}

template<uint PIN>
void GpioOut<PIN>::write(bool on)
{
    this->_on = on;
    #ifdef PICO_BOARD_PICO2_W
        cyw43_arch_gpio_put(PIN, on);
    #else
        gpio_put(PIN, on);
    #endif
}
template<uint PIN>
void GpioOut<PIN>::toggle(void)
{
    this->write(!this->_on);
}

template<uint PIN, GpioInMode MODE>
GpioIn<PIN, MODE>::GpioIn(void):
    Gpio<PIN>()
{
    gpio_set_dir(PIN, GPIO_IN);
    switch(MODE)
    {
        case GpioInMode::PullUp:
        {
            gpio_pull_up(PIN);
            break;
        }
        case GpioInMode::PullDown:
        {
            gpio_pull_down(PIN);
            break;
        }
    }
}

template<uint PIN, GpioInMode MODE>
bool GpioIn<PIN, MODE>::read(void) const
{
    return gpio_get(PIN);
}