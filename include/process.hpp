#pragma once
#include <concepts>
#include <device.hpp>
#include <chrono>
#include <memory>
#include <pico/time.h>

// Provides a C ABI callback to be used with raspberry pico hardware-timers
extern "C" bool process_update_timer_callback(repeating_timer_t* timer);

class Process
{
protected:
    using duration_t = std::chrono::duration<std::int64_t, std::micro>;

protected:
    virtual void update(void) = 0;

    friend bool process_update_timer_callback(repeating_timer_t *timer);
};

class Timer
{
private:
    std::unique_ptr<Process> process;

public:
    Timer(std::unique_ptr<Process>&& process);
    template<std::derived_from<Process> T>
    Timer(T&& process);
};

template<std::derived_from<Process> T>
Timer::Timer(T&& process):
    Timer(std::make_unique(std::move(process)))
{

}