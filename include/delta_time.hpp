#pragma once
#include <chrono>

class DeltaTime;

class DeltaTime final
{
private:
    using duration_t = std::chrono::microseconds;
    using time_point_t = std::chrono::system_clock::time_point;

    duration_t _delta_time;
    time_point_t _time;

public:
    DeltaTime(void);

    void tick(void);
    
    duration_t dt(void) const;
    time_point_t t(void) const;
};