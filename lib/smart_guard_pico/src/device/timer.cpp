#include "delta_time.hpp"
#include "process/process.hpp"
#include <cassert>
#include <device/timer.hpp>

Timer::Timer(void) noexcept:
    _timer_data(nullptr),
    _timer(nullptr)
{
    
}
Timer::Timer(Timer&& timer) noexcept:
    _timer_data(std::move(timer._timer_data)),
    _timer(std::move(timer._timer))
{
    assert(!timer._timer_data);
    assert(!timer._timer);
}

Timer::Timer(std::unique_ptr<process::Process>&& process, microseconds_t delay):
    _timer_data(process ? std::make_unique<TimerData>(TimerData {
        .delta_time {},
        .process {std::move(process)}
    }) : nullptr),
    _timer(this->_timer_data ? std::make_unique<repeating_timer_t>(repeating_timer_t(0)) : nullptr)
{
    if(this->_timer_data && this->_timer_data->process)
    {
        assert(this->_timer);
        add_repeating_timer_us(
            delay.count(), // should convert into microseconds
            Timer::__process_update_timer_callback,
            this->_timer_data.get(),
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
    this->_timer_data = std::move(timer._timer_data);
    this->_timer = std::move(timer._timer);
    assert(!timer._process);
    assert(!timer._timer);

    return *this;
}

std::unique_ptr<process::Process> Timer::operator*(void) &&
{
    if(this->_timer)
    {
        cancel_repeating_timer(this->_timer.get());
        this->_timer = nullptr;
    }

    std::unique_ptr<process::Process> process {std::move(this->_timer_data->process)};
    assert(!this->_timer_data->process);
    this->_timer_data = nullptr;
    assert(!this->_timer_data);
    assert(!this->_timer);
    
    return process;
}

Timer::microseconds_t Timer::get_delay(void) const
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

bool Timer::__process_update_timer_callback(repeating_timer_t* timer)
{
    // WARNING: There could be nullpointer exceptions happening here, but i want to test nullability first by using assertions.
    // Getting the behaviour of the destructor right could need a bit of trial and error (with my limited knowledge)
    // Update: this seems to work fine. No problems yet! :)
    assert(timer);
    assert(timer->user_data);

    void* timer_data_untyped {timer->user_data};
    TimerData* timer_data {static_cast<TimerData*>(timer_data_untyped)};
    process::Process& process {*timer_data->process};
    DeltaTime& delta_time {timer_data->delta_time};

    process::Update result {process.update(delta_time.dt())};
    delta_time.tick();
    
    switch(result)
    {
        case process::Update::done:
        {
            return false;
        }
        case process::Update::pending:
        {
            return true;
        }
    }
    return false;
}