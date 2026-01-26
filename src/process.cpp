#include "data.hpp"
#include <memory>
#include <process.hpp>
#include <assert.h>

extern "C" bool process_update_timer_callback(repeating_timer_t* const timer)
{
    // WARNING: There could be nullpointer exceptions happening here, but i want to test nullability first by using assertions.
    // Getting the behaviour of the destructor right could need a bit of trial and error (with my limited knowledge)
    // TODO: make this more robust later
    assert(timer);
    assert(timer->user_data);

    void* system_untyped {timer->user_data};
    Process* system {static_cast<Process*>(system_untyped)};
    system->update();

    return true;
}

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

Timer::Timer(std::unique_ptr<Process>&& process, duration_t delay):
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

duration_t Timer::get_delay(void) const
{
    if(!this->_timer)
    {
        return duration_t::zero();
    }
    return duration_t(this->_timer->delay_us);
}