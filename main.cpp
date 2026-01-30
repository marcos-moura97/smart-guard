#include <chrono>
#include <imu/mpu6050.hpp>
#include <pico/time.h>
#include <device/pico.hpp>
#include <pico/stdlib.h>
#include <process/blink.hpp>
#include <debug.hpp>

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
    // IMU error correction shouldn't be hard-coded
    // Unless we create a 'calibration mode', it has to be like this
    // This is a could have, not a must have, by the end of the semester we can think about it
    //Mpu6050 imu(0x68, {-0.3f, -0.2f, -0.9f}, {-3.5f, 0.0f, -1.5f});
    //Pico device(std::move(imu));

    Timer blink(Blink(), std::chrono::seconds(1));
    while (blink.status() == Timer::Status::running) {}

    std::printf("Done");
    
    return 0; 
}