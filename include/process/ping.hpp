#pragma once
#include <process.hpp>
#include <pico/types.h>
#include <device/gpio.hpp>
#include <functional>

#ifdef PICO_BOARD_PICO2_W
#include <boards/pico2_w.h>
#else
#include <boards/pico2.h>
#endif

class Ping : public Process
{
private:    
    std::function<void()> _onUpdate;
public:
    Ping(std::function<void()> onUpdate = nullptr);

protected:
    virtual Update update(void) override;
};