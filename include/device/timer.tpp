#pragma once
#include <device/timer.hpp>

template<std::derived_from<Process> T, typename DRepr, typename DUnit>
Timer::Timer(T process, std::chrono::duration<DRepr, DUnit> delay):
    Timer(std::make_unique<T>(std::move(process)), delay)
{

}

template<typename DRepr, typename DUnit>
Timer::Timer(std::unique_ptr<Process>&& process, std::chrono::duration<DRepr, DUnit> delay):
    Timer(std::move(process), std::chrono::duration_cast<microseconds_t>(delay))
{
    
}