#include <system.hpp>

System::System(std::shared_ptr<Device> device):
    device(std::move(device)),
    fall_detector(this->device),
    fall_detector_timer()
{
    // Adding hardware timers. I assume this should work but it needs testing
    if(!(
        add_repeating_timer_us(
            std::chrono::duration_cast<duration_t>(FALL_DETECTOR_SAMPLE_PERIOD).count(), // should convert into microseconds
            process_update_timer_callback,
            &this->fall_detector,
            &this->fall_detector_timer
        )
    ))
    {
        panic("Attaching hardware timer failed. Timers are busy. Cannot add more hardware timers.");
    }
}

System::~System()
{
    if(this->fall_detector_timer.)
    cancel_repeating_timer(&this->fall_detector_timer);
}