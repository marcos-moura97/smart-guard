#include "device/gpio.hpp"
#include "device/i2c.hpp"
#include "process/fall_detector.hpp"
#include <chrono>
#include <device/imu/mpu6050.hpp>
#include <device/timer.hpp>
#include <memory>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <process/blink.hpp>
#include <process/ping.hpp>

#ifdef PICO_BOARD_PICO2_W
#include "connectivity/wifi_wrapper.hpp" 
#include "connectivity/mqtt_client.hpp"
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
    sleep_ms(2000);

    MqttClient mqtt;
    mqtt.setAuth(MQTT_USERNAME, MQTT_PASSWORD);
    if (!mqtt.resolveAndConnect(MQTT_SERVER)) {
        std::printf("DNS/Connect sequence failed to start\n");
    }
    if (!mqtt.waitUntilConnected(10000)) {
        std::printf("Timeout waiting for MQTT connect\n");
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
    Timer fall_detector(
        FallDetector(GpioButton(), GpioEmergencyLed(), std::move(imu)), 
        std::chrono::milliseconds(20) // 60 Hz
    );
#ifdef PICO_BOARD_PICO2_W
    Timer ping(
        Ping([&mqtt]() { mqtt.publish_ping(); }),
        std::chrono::seconds(1)
    );
#endif

    while (blink.status() == Timer::Status::running || fall_detector.status() == Timer::Status::running) {}

    std::printf("Done");
    
    return 0; 
}