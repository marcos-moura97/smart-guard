#include <delta_time.hpp>

DeltaTime::DeltaTime(void):
    _delta_time(duration_t::zero()),
    _time(std::chrono::system_clock::now())
{

}

void DeltaTime::tick(void)
{
    time_point_t time {this->_time};
    this->_time = std::chrono::system_clock::now();
    this->_delta_time = std::chrono::duration_cast<duration_t>(time - this->_time);
}
DeltaTime::duration_t DeltaTime::dt(void) const
{
    return this->_delta_time;
}
DeltaTime::time_point_t DeltaTime::t(void) const
{
    return this->_time;
}