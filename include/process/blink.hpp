#pragma once
#include <process.hpp>
#include <pico/types.h>

#ifdef PICO_BOARD_PICO2_W
#include <boards/pico2_w.h>
#else
#include <boards/pico2.h>
#endif

class Blink : public Process
{
private:
#ifdef PICO_BOARD_PICO2_W
    static constexpr uint BLINK_LED_GPIO {CYW43_WL_GPIO_LED_PIN};
#else
    static constexpr uint BLINK_LED_GPIO {PICO_DEFAULT_LED_PIN};
#endif
    bool status;

public:
    Blink(void);

protected:
    virtual Update update(void) override;
};