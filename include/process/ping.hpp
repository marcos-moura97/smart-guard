#pragma once
#include <process/process.hpp>
#include <pico/types.h>
#include <device/gpio.hpp>
#include <functional>

#ifdef PICO_BOARD_PICO2_W
#include <boards/pico2_w.h>
#else
#include <boards/pico2.h>
#endif

namespace process
{
    class Ping : public Process
    {
    private:    
        std::function<void()> _on_update;
    public:
        Ping(std::function<void()> on_update = nullptr);

    protected:
        virtual Update update(void) override;
    };
}