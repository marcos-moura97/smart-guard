#pragma once
#include <chrono>
#include <memory>
#include <process/fall_detector.hpp>
#include <pico/time.h>
#include <device.hpp>

class System final
{
private:
    using duration_t = std::chrono::duration<std::int64_t, std::micro>;

    static constexpr float FALL_DETECTOR_SAMPLE_RATE {60.0f};
    static constexpr std::chrono::duration<float> FALL_DETECTOR_SAMPLE_PERIOD {1.0f/FALL_DETECTOR_SAMPLE_RATE};

    std::shared_ptr<Device> device;

    FallDetector fall_detector;
    repeating_timer_t fall_detector_timer;

public:
    System(void) = delete;
    System(const System&) = delete;
    System(System&& system) = delete;

    System(std::shared_ptr<Device> device);

    template<std::derived_from<Device> D>
    System(D&& device);

    // Hardware timers need to be stopped before the object is destroyed
    ~System();

    System& operator=(System&& system) = delete;
};

template<std::derived_from<Device> D>
System::System(D&& device):
    System(std::make_unique<D>(std::move(device)))
{

}