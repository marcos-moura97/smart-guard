#pragma once
#include <process/process.hpp>
#include <pico/types.h>
#include <device/gpio.hpp>

namespace process
{
    class Blink : public Process
    {
    private:
        BuiltinLed _led;
    public:
        Blink(void);

    protected:
        virtual Update update(duration_t dt) override;
    };
}