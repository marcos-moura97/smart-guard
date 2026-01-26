#pragma once
#include <concepts>
#include <chrono>
#include <memory>
#include <pico/time.h>

#include "data.hpp"

// Provides a C ABI callback to be used with raspberry pico hardware-timers
extern "C" bool process_update_timer_callback(repeating_timer_t* timer);

class Process
{
protected:
    virtual void update(void) = 0;

    friend bool process_update_timer_callback(repeating_timer_t *timer);
};

class Timer final
{
private:
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
    Timer(std::unique_ptr<Process>&& process, duration_t delay);

    ~Timer(void) noexcept;

    Timer& operator=(Timer&& timer);
    Timer& operator=(const Timer& timer) = delete;

    std::unique_ptr<Process> operator*(void) &&;

    duration_t get_delay(void) const;
};

template<typename DRepr, typename DUnit>
Timer::Timer(std::unique_ptr<Process>&& process, std::chrono::duration<DRepr, DUnit> delay):
    Timer(std::move(process), std::chrono::duration_cast<duration_t>(delay))
{
    
}

template<std::derived_from<Process> T, typename DRepr, typename DUnit>
Timer::Timer(T process, std::chrono::duration<DRepr, DUnit> delay):
    Timer(std::make_unique(std::move(process)), delay)
{

}