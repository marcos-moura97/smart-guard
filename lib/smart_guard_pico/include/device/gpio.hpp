#pragma once
#include <cstdio>
#include <hardware/gpio.h>
#include <pico/types.h>
#include <device/input.hpp>
#include <device/output.hpp>

#ifdef PICO_BOARD_PICO2_W
#include <boards/pico2_w.h>
#else
#include <boards/pico2.h>
#endif

template<uint PIN>
class Gpio
{
public:
    Gpio(void);

    ~Gpio(void);

    uint pin(void) const;
};

template<uint PIN>
class GpioOut : public Gpio<PIN>, public Output<bool>
{
private:
    bool _on = false;

public:
    GpioOut(bool on = false);

    virtual void write(bool on) override;
    void toggle(void);
};

enum class GpioInMode
{
    PullUp,
    PullDown
};

template<uint PIN, GpioInMode MODE>
class GpioIn : public Gpio<PIN>, public Input<bool>
{
public:
    GpioIn(void);

    virtual bool read(void) const override;
};

using BuiltinLed = GpioOut<
#ifdef PICO_BOARD_PICO2_W
    CYW43_WL_GPIO_LED_PIN
#else
    PICO_DEFAULT_LED_PIN
#endif
>;

using GpioEmergencyLed = GpioOut<19>;
using GpioButton = GpioIn<9, GpioInMode::PullUp>;

#include <device/gpio.tpp>