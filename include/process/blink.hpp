#pragma once
#include <process.hpp>
#include <boards/pico.h>
#include <pico/types.h>

class Blink : public Process
{
private:
    static constexpr uint BLINK_LED_GPIO {PICO_DEFAULT_LED_PIN};

    bool status;

public:
    Blink(void);

protected:
    virtual Update update(void) override;
};