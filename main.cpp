#include "device/gpio.hpp"
#include "device/i2c.hpp"
#include "process/fall_detector.hpp"
#include <chrono>
#include <cstdio>
#include <device/imu/mpu6050.hpp>
#include <device/timer.hpp>
#include <memory>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <process/blink.hpp>

#ifdef PICO_BOARD_PICO2_W
#include "pico/cyw43_arch.h"
#endif

// ----------------- main -----------------
int main()
{
    stdio_init_all();

#ifdef PICO_BOARD_PICO2_W
    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43");
    }
#endif
    auto i2c {std::make_shared<I2cDefault<Mpu6050::BAUD_RATE>>()};
    
    // IMU error correction shouldn't be hard-coded
    // Unless we create a 'calibration mode', it has to be like this
    // This is a could have, not a must have, by the end of the semester we can think about it
    Mpu6050 imu(i2c, 0x68, {-0.3f, -0.2f, -0.9f}, {-3.5f, 0.0f, -1.5f});

    Timer blink(
        Blink(),
        std::chrono::seconds(1) // 1 Hz
    );

    /*Timer fall_detector(
        FallDetector(GpioButton(), GpioEmergencyLed(), std::move(imu)), 
        std::chrono::microseconds(1000000/60) // 60 Hz
    );*/

    while (blink.status() == Timer::Status::running/* || fall_detector.status() == Timer::Status::running*/)
    {
        
    }

    std::printf("Done");
    
    return 0; 
}