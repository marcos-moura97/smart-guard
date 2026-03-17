#include "data.hpp"
#include "device/gpio.hpp"
#include "device/i2c.hpp"
#include "process/fall_detector.hpp"
#include <chrono>
#include <cstdio>
#include <device/imu/mpu6050.hpp>
#include <device/timer.hpp>
#include <functional>
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
    // it's ugly, I know, fell free to refactor as you want
    if (cyw43_arch_init()) { 
        std::printf("CYW43: failed to initialise\n"); 
        return 1; 
    } 
    WifiWrapper wifi;

    //auto i2c {std::make_shared<I2cDefault<Mpu6050::BAUD_RATE>>()};
    
    //Mpu6050 imu(i2c, Mpu6050::DEFAULT_SLAVE_ADDRESS, Mpu6050::AccelRange::G2, Mpu6050::GyroRange::D250);

    Timer blink(
        Blink(),
        std::chrono::seconds(1) // 1 Hz
    );

    /*Timer fall_detector(
        FallDetector(GpioButton(), GpioEmergencyLed(), std::move(imu)), 
        std::chrono::milliseconds(20) // 50 Hz
    );*/

    while(blink.status() == Timer::Status::running /*|| fall_detector.status() == Timer::Status::running*/)
    {
        sleep_ms(1000);
    }
    
    return 0;
}

int test_imu()
{
    stdio_init_all();
    
    auto i2c {std::make_shared<I2cDefault<Mpu6050::BAUD_RATE>>()};

    Mpu6050 imu(i2c, Mpu6050::DEFAULT_SLAVE_ADDRESS, Mpu6050::AccelRange::G2, Mpu6050::GyroRange::D250);
    
    while(true)
    {
        auto [accel, gyro, temp] = imu.measure();

        auto [ax, ay, az] = accel;
        auto [gx, gy, gz] = gyro;

        std::printf("[%f, %f, %f], [%f, %f, %f], %f\n", ax, ay, az, gx, gy, gz, temp);

        sleep_ms(1000);
    }

    return 0;
}

#ifdef PICO_BOARD_PICO2_W
int test_wifi()
{
    stdio_init_all();
    
    if (cyw43_arch_init()) { 
        std::printf("CYW43: failed to initialise\n"); 
        return false; 
    } 
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

    Timer ping(
        Ping([&mqtt]() { mqtt.send(PingData()); }),
        std::chrono::seconds(1)
    );

    while(ping.status() == Timer::Status::running)
    {
        sleep_ms(1000);
    }

    return 0;
}

int test_wifi_w_imu(){
    stdio_init_all();
    if (cyw43_arch_init()) { 
        std::printf("CYW43: failed to initialise\n"); 
        return false; 
    } 
    
    auto i2c {std::make_shared<I2cDefault<Mpu6050::BAUD_RATE>>()};

    Mpu6050 imu(i2c, Mpu6050::DEFAULT_SLAVE_ADDRESS, Mpu6050::AccelRange::G2, Mpu6050::GyroRange::D250);

    WifiWrapper wifi;
    if (!wifi.init()) {
        return 1;
    }
    wifi.enableSta();
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
    
    while(true)
    {
        SensorData sensor_data = {};
        sensor_data.imu = {imu.measure()};
        Direction direction = {};

        //std::printf("[%f, %f, %f], [%f, %f, %f], %f\n", ax, ay, az, gx, gy, gz, temp);

        
        mqtt.send(
            PacketData {
                .sensor_data {sensor_data},
                .direction {direction},
                .direction_filtered {direction},
                
                .free_fall_event = false,
                .impact_event = false,
                .tilt_change_event = false,
                .in_post_fall_monitor = false,
                .fall_detected = false
            }
        );

        sleep_ms(100);
    }

    return 0;
}

#endif