#pragma once
#include <hardware/gpio.h>
#include <pico/types.h>

template<uint PIN>
class Gpio
{
public:
    Gpio(const Gpio<PIN>&) = delete;
    Gpio(Gpio<PIN>&&) = delete;

    Gpio(void);

    ~Gpio(void);

    uint pin(void) const;

    Gpio<PIN>& operator=(const Gpio<PIN>&) = delete;
    Gpio<PIN>& operator=(Gpio<PIN>&&) = delete;
};

template<uint PIN>
class GpioOut : public Gpio<PIN>
{
private:
    bool _on = false;

public:
    GpioOut(bool on = false);

    void write(bool on);
    void toggle(void);
};

#ifdef PICO_BOARD_PICO2_W
    using GpioBuiltinLed = GpioOut<CYW43_WL_GPIO_LED_PIN>;
#else
    using GpioBuiltinLed = GpioOut<PICO_DEFAULT_LED_PIN>;
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
    gpio_deinit(PIN);
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