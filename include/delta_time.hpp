#pragma once
#include <chrono>
#include <pico/types.h>

class DeltaTime;

class DeltaTime final
{
private:
    using duration_t = std::chrono::microseconds;
    using time_point_t = absolute_time_t;

    duration_t _delta_time;
    time_point_t _time;

public:
    DeltaTime(void);

    void tick(void);
    
    duration_t dt(void) const;
    time_point_t t(void) const;
};