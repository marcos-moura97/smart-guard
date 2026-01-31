#pragma once
#include <process.hpp>
#include <pico/types.h>
#include <device/gpio.hpp>

class Blink : public Process
{
private:
    GpioBuiltinLed _led;
public:
    Blink(void);

protected:
    virtual Update update(void) override;
};