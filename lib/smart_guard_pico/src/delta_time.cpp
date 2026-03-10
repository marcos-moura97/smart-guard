#include <delta_time.hpp>
#include <pico/time.h>

DeltaTime::DeltaTime(void):
    _delta_time(duration_t::zero()),
    _time(get_absolute_time())
{

}

void DeltaTime::tick(void)
{
    time_point_t time {this->_time};
    this->_time = get_absolute_time();
    this->_delta_time = duration_t(absolute_time_diff_us(time, this->_time));
}
DeltaTime::duration_t DeltaTime::dt(void) const
{
    return this->_delta_time;
}
DeltaTime::time_point_t DeltaTime::t(void) const
{
    return this->_time;
}