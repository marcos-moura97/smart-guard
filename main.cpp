#include <chrono>
#include <imu/mpu6050.hpp>
#include <pico/time.h>
#include <device/pico.hpp>
#include <pico/stdlib.h>
#include <process/blink.hpp>

// ----------------- main -----------------
int main()
{
    stdio_init_all();

    // IMU error correction shouldn't be hard-coded
    Mpu6050 imu(0x68, {-0.3f, -0.2f, -0.9f}, {-3.5f, 0.0f, -1.5f});
    Pico device(std::move(imu));

    Timer blink(Blink(), std::chrono::seconds(1));
    
    while (blink.status() == Timer::Status::running) {}

    std::printf("Done");
    
    return 0; 
}