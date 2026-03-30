#include <chrono>
#include <delta_time.hpp>

TimePoint::TimePoint(repr_t repr) noexcept:
    _repr(std::move(repr))
{

}

TimePoint::duration_t TimePoint::duration_since(TimePoint previous_instant) const
{
#ifdef PICO_BOARD
    return duration_t(absolute_time_diff_us(this->_repr, previous_instant._repr))
#else
    return std::chrono::duration_cast<duration_t>(this->_repr - previous_instant._repr);
#endif
}

TimePoint TimePoint::now(void)
{
#ifdef PICO_BOARD
    return TimePoint(get_absolute_time());
#else
    return TimePoint(std::chrono::system_clock::now());
#endif
}

DeltaTime::DeltaTime(void):
    DeltaTime(TimePoint::now())
{

}
DeltaTime::DeltaTime(TimePoint now):
    _delta_time(duration_t::zero()),
    _time(std::move(now))
{

}

void DeltaTime::tick(void)
{
    TimePoint time {this->_time};
    this->_time = TimePoint::now();
    this->_delta_time = this->_time.duration_since(time);
}
DeltaTime::duration_t DeltaTime::dt(void) const
{
    return this->_delta_time;
}
DeltaTime::time_point_t DeltaTime::t(void) const
{
    return this->_time;
}