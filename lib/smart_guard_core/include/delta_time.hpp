#pragma once
#include <chrono>

#ifdef PICO_BOARD
    #include <pico/types.h>
#endif

class TimePoint final
{
public:
    using duration_t = std::chrono::microseconds;

private:
    using repr_t = 
#ifdef PICO_BOARD
        absolute_time_t
#else
        std::chrono::system_clock::time_point
#endif
    ;

    repr_t _repr;

private:
    explicit TimePoint(repr_t repr) noexcept;

public:
    duration_t duration_since(TimePoint previous_instant) const;

    static TimePoint now(void);
};

class DeltaTime final
{
public:
    using duration_t = TimePoint::duration_t;
    using time_point_t = TimePoint;

private:
    time_point_t _time;
    duration_t _delta_time;

public:
    DeltaTime(void);
    explicit DeltaTime(time_point_t time);

    void tick(void);
    
    duration_t dt(void) const;
    time_point_t t(void) const;
};