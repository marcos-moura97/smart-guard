#pragma once
#include <device.hpp>
#include <imu/mpu6050.hpp>
#include <hardware/gpio.h>

class Pico final : public Device
{
private:
    // GPIO / I2C constants
    static constexpr uint LED_GPIO = 19;       // stop button
    static constexpr uint BUTTON_GPIO = 9;     // Emergency LED

    Mpu6050 imu;
public:
    Pico(Pico&&) = default;

    explicit Pico(Mpu6050&& imu):
        imu(std::move(imu))
    {
        // LEDs
#ifdef PICO_BOARD_PICO2_W
        gpio_init(CYW43_WL_GPIO_LED_PIN);
        gpio_set_dir(CYW43_WL_GPIO_LED_PIN, GPIO_OUT);
#else
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
        gpio_init(LED_GPIO);
        gpio_set_dir(LED_GPIO, GPIO_OUT);

        // Button
        gpio_init(BUTTON_GPIO);
        gpio_set_dir(BUTTON_GPIO, GPIO_IN);
        gpio_pull_up(BUTTON_GPIO);

        std::printf("Setup done! Starting reading.\n");
        std::printf("ax_g,ay_g,az_g,pitch,roll,acc_smv,gyro_smv,mag_g,lying\n");
    }

public:
    virtual ImuData measure_imu(void) override
    {
        return this->imu.measure();
    }
};