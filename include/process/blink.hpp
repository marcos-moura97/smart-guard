#pragma once
#include <process.hpp>
#include <pico/types.h>
#include <gpio.hpp>

#ifdef PICO_BOARD_PICO2_W
#include <boards/pico2_w.h>
#else
#include <boards/pico2.h>
#endif

class Blink : public Process
{
private:
    GpioBuiltinLed _led;
public:
    Blink(void);

protected:
    virtual Update update(void) override;
};