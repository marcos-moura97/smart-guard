#pragma once
#include <concepts>
#include <chrono>
#include <cstddef>
#include <memory>
#include <pico/time.h>
#include <ratio>

// Provides a C ABI callback to be used with raspberry pico hardware-timers
extern "C" bool process_update_timer_callback(repeating_timer_t* timer);

/**
A process which provides an update method that can be ran at a regular interval.

This class can be fed into a Timer class which will run it at a regular given interval using the Raspberry Pico hardware-timers.
*/
class Process
{
protected:
    virtual void update(void) = 0;

    friend bool process_update_timer_callback(repeating_timer_t *timer);
};

/**
A hook for a hardware timer running a Process at a given regular interval using Raspbarry Pico hardware-timers.

The timer will start right away as soon as the object is initialized, and will be stopped when the object exits the scope (i.e. its destructor is called).

To stop a timer early, you can assign nullptr to it.
*/
class Timer final
{
private:
    using microseconds_t = std::chrono::duration<decltype(to_us_since_boot(get_absolute_time())), std::micro>;

    std::unique_ptr<Process> _process;
    std::unique_ptr<repeating_timer_t> _timer;

public:
    Timer(void) noexcept;
    Timer(Timer&& timer) noexcept;
    Timer(const Timer& timer) = delete;
    
    template<std::derived_from<Process> T, typename DRepr, typename DUnit>
    Timer(T process, std::chrono::duration<DRepr, DUnit> delay);
    template<typename DRepr, typename DUnit>
    Timer(std::unique_ptr<Process>&& process, std::chrono::duration<DRepr, DUnit> delay);
    Timer(std::unique_ptr<Process>&& process, microseconds_t delay);

    ~Timer(void) noexcept;

    Timer& operator=(std::nullptr_t);
    Timer& operator=(Timer&& timer);
    Timer& operator=(const Timer& timer) = delete;

    std::unique_ptr<Process> operator*(void) &&;

    microseconds_t get_delay(void) const;
};

template<typename DRepr, typename DUnit>
Timer::Timer(std::unique_ptr<Process>&& process, std::chrono::duration<DRepr, DUnit> delay):
    Timer(std::move(process), std::chrono::duration_cast<microseconds_t>(delay))
{
    
}

template<std::derived_from<Process> T, typename DRepr, typename DUnit>
Timer::Timer(T process, std::chrono::duration<DRepr, DUnit> delay):
    Timer(std::make_unique(std::move(process)), delay)
{

}