#pragma once
#include <chrono>
#include <concepts>
#include <memory>
#include <pico/time.h>
#include <process.hpp>

/**
A hook for a hardware timer running a Process at a given regular interval using Raspbarry Pico hardware-timers.

The timer will start right away as soon as the object is initialized, and will be stopped when the object exits the scope (i.e. its destructor is called).

To stop a timer early, you can assign nullptr to it.
*/
class Timer final
{
private:
    using microseconds_t = std::chrono::duration<decltype(to_us_since_boot(get_absolute_time())), std::micro>;

public:
    enum class Status
    {
        stopped,
        running,
        done,
    };

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
    Timer(std::unique_ptr<Process>&& process, microseconds_t delay);

    ~Timer(void) noexcept;

    Timer& operator=(std::nullptr_t);
    Timer& operator=(Timer&& timer);
    Timer& operator=(const Timer& timer) = delete;

    std::unique_ptr<Process> operator*(void) &&;

    microseconds_t delay(void) const;
    Status status(void) const;

    /**
    * Waits for process to finish before continuing
    */
    void join(void) const;
};

#include <device/timer.tpp>