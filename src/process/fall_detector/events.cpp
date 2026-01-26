#include <optional>
#include <system.hpp>

void System::Events::increment_time(duration_t dt)
{
    // Using chrono durations should mitigate numeric overflow
    if(this->free_fall.has_value())
    {
        this->free_fall->elapsed += dt;
    }
    if(this->impact.has_value())
    {
        this->impact->elapsed += dt;
    }
    if(this->tilt_change.has_value())
    {
        this->tilt_change->elapsed += dt;
    }
    if(this->post_fall.has_value())
    {
        this->post_fall->elapsed += dt;
    }
}

bool System::Events::is_free_fall(void) const
{
    return this->free_fall.has_value() && this->free_fall->is_free_fall;
}
bool System::Events::is_impact(void) const
{
    return this->impact.has_value();
}
bool System::Events::is_tilt_change(void) const
{
    return this->tilt_change.has_value() && this->tilt_change->is_tilt_change;
}
bool System::Events::is_monitoring_post_fall(void) const
{
    return this->post_fall.has_value();
}