#include <chrono>
#include <imu/mpu6050.hpp>
#include <pico/time.h>
#include <device/pico.hpp>
#include <pico/stdlib.h>
#include <process/blink.hpp>

#ifdef PICO_BOARD_PICO2_W
#include "connectivity/wifi_wrapper.hpp" 
#endif
// ----------------- main -----------------
int main()
{

    stdio_init_all();

#ifdef PICO_BOARD_PICO2_W    
    WifiWrapper wifi;
    if (!wifi.init()) {
        return 1;
    }
    wifi.enableSta();
    // The wifi connection can be represented as a LED
    // If it's connected it blinks (or stays on) and if
    // an error happened, then it's off
    if (!wifi.connect(WIFI_SSID, WIFI_PASSWORD, 30000)) {
        return 1;
    }
    
#endif

    Mpu6050 imu(0x68, {-0.3f, -0.2f, -0.9f}, {-3.5f, 0.0f, -1.5f});
    Pico device(std::move(imu));
    Timer blink(Blink(), std::chrono::seconds(1));
    while (blink.status() == Timer::Status::running) {}

    std::printf("Done");
    
    return 0; 
}