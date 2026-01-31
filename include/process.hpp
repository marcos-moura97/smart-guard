#pragma once
#include <pico/time.h>

// Provides a C ABI callback to be used with raspberry pico hardware-timers
extern "C" bool process_update_timer_callback(repeating_timer_t* timer);

/**
A process which provides an update method that can be ran at a regular interval.

This class can be fed into a Timer class which will run it at a regular given interval using the Raspberry Pico hardware-timers.
*/
class Process
{
public:
    enum class Update
    {
        pending,
        done
    };

protected:
    virtual Update update(void) = 0;

    friend bool process_update_timer_callback(repeating_timer_t *timer);
};