#include <cstdio>
#include <hardware/gpio.h>
#include <process/ping.hpp>

#ifdef PICO_BOARD_PICO2_W
#include "pico/cyw43_arch.h"
#endif

Ping::Ping(std::function<void()> onUpdate)
    : _onUpdate(onUpdate)
{
    printf("Ping process started!");
}

Process::Update Ping::update(void)
{

    if (_onUpdate)
        _onUpdate();

    return Update::pending;
}