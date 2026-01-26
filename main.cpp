#include "imu/mpu6050.hpp"
#include <memory>
#include <pico/time.h>
#include <device/pico.hpp>
#include <system.hpp>
#include "pico/stdlib.h"

// ----------------- main -----------------
int main() {
    stdio_init_all();

    // IMU error correction shouldn't be hard-coded
    Mpu6050 imu(0x68, {-0.3f, -0.2f, -0.9f}, {-3.5f, 0.0f, -1.5f});
    Pico device(std::move(imu));
    System system(std::move(device));
    
    while (true) {}

    return 0; 
}