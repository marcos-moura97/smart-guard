#include <device/timer.hpp>

Timer::Timer(void) noexcept:
    _process(nullptr),
    _timer(nullptr)
{

}
Timer::Timer(Timer&& timer) noexcept:
    _process(std::move(timer._process)),
    _timer(std::move(timer._timer))
{
    assert(!timer._process);
    assert(!timer._timer);
}

Timer::Timer(std::unique_ptr<Process>&& process, microseconds_t delay):
    _process(std::move(process)),
    _timer(this->_process ? std::make_unique<repeating_timer_t>(repeating_timer_t(0)) : nullptr)
{
    if(this->_process)
    {
        assert(this->_timer);
        add_repeating_timer_us(
            delay.count(), // should convert into microseconds
            process_update_timer_callback,
            this->_process.get(),
            this->_timer.get()
        );
    }
}

Timer::~Timer(void) noexcept
{
    if(this->_timer)
    {
        cancel_repeating_timer(this->_timer.get());
        this->_timer = nullptr;
    }
}

Timer& Timer::operator=(std::nullptr_t)
{
    if(this->_timer)
    {
        cancel_repeating_timer(this->_timer.get());
        this->_timer = nullptr;
    }

    return *this;
}
Timer& Timer::operator=(Timer&& timer)
{
    if(this->_timer)
    {
        cancel_repeating_timer(this->_timer.get());
    }
    this->_process = std::move(timer._process);
    this->_timer = std::move(timer._timer);
    assert(!timer._process);
    assert(!timer._timer);

    return *this;
}

std::unique_ptr<Process> Timer::operator*(void) &&
{
    if(this->_timer)
    {
        cancel_repeating_timer(this->_timer.get());
        this->_timer = nullptr;
    }

    std::unique_ptr<Process> process {std::move(this->_process)};
    assert(!this->_process);
    assert(!this->_timer);
    
    return process;
}

Timer::microseconds_t Timer::delay(void) const
{
    if(!this->_timer)
    {
        return microseconds_t::zero();
    }
    return microseconds_t(this->_timer->delay_us);
}
Timer::Status Timer::status(void) const
{
    if(this->_timer)
    {
        if(this->_timer->alarm_id == 0)
        {
            return Status::done;
        }
        return Status::running;
    }
    return Status::stopped;
}