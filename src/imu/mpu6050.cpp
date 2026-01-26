#include <array>
#include <cstdio>
#include <span>

#include <pico/binary_info.h>
#include <hardware/i2c.h>
#include <hardware/gpio.h>

#include <data.hpp>
#include <imu/mpu6050.hpp>

Mpu6050::Mpu6050(std::uint8_t addr, std::array<float, 3>&& accel_offset, std::array<float, 3>&& gyro_offset):
    accel_offset(std::move(accel_offset)),
    gyro_offset(std::move(gyro_offset)),
    addr(addr)
{
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/mpu6050_i2c example requires a board with I2C pins
    std::puts("Default I2C pins were not defined");
#else
    std::printf("Hello, MPU6050! Reading raw data from registers...\n");
    
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    this->reset();
#endif
}

ImuDataRaw Mpu6050::measure_raw(void)
{
#ifdef i2c_default
    uint8_t buffer[6];
    ImuDataRaw data; // Initialize with garbage, it will be overwritten

    // Accel registers 0x3B..0x40 (we read 6 bytes)
    static constexpr uint8_t REG_ACCEL {0x3B};
    i2c_write_blocking(i2c_default, this->addr, &REG_ACCEL, 1, true);
    i2c_read_blocking(i2c_default, this->addr, buffer, 6, false);

    // TODO: What is the endianness here? Is the endianness defined behaviour? Can memcpy/reinterpret-cast/bitcast be used?
    for (std::size_t i = 0; i < 3; ++i) {
        // C++'s weird implicit conversions make this work without overflow. It looks a bit concerning, but it should work. Is there a more explicit way?
        data.accel[i] = (int16_t)((buffer[i*2] << 8) | buffer[i*2 + 1]);
    }

    // Gyro registers 0x43..0x48
    static constexpr uint8_t REG_GYRO {0x43};
    i2c_write_blocking(i2c_default, this->addr, &REG_GYRO, 1, true);
    i2c_read_blocking(i2c_default, this->addr, buffer, 6, false);

    for (std::size_t i = 0; i < 3; ++i) {
        data.gyro[i] = (int16_t)((buffer[i*2] << 8) | buffer[i*2 + 1]);
    }

    // Temp registers 0x41..0x42
    static constexpr uint8_t REG_TEMP {0x41};
    i2c_write_blocking(i2c_default, this->addr, &REG_TEMP, 1, true);
    i2c_read_blocking(i2c_default, this->addr, buffer, 2, false);
    data.temp = (int16_t)((buffer[0] << 8) | buffer[1]);

    return data;
#else
    // If i2c_default isn't defined on this build, produce zeros (keeps compilation possible)
    return ImuDataRaw {
        .accel {0, 0, 0},
        .gyro {0, 0, 0},
        .temp {0}
    };
#endif
}

ImuData Mpu6050::measure(void)
{
    ImuDataRaw raw {this->measure_raw()};

    return {
        .accel {this->accel_from_raw(raw.accel)},
        .gyro {this->gyro_from_raw(raw.gyro)},
        .temp {this->temp_from_raw(raw.temp)}
    };
}

std::array<float, 3> Mpu6050::accel_from_raw(std::span<std::uint16_t, 3> accel_raw) const
{
    static constexpr float SENSORS_GRAVITY_EARTH = 9.80665F;

    std::array<float, 3> accel;
    
    for(std::size_t i {0}; i < 3; ++i)
    {
        accel[i] = static_cast<float>(accel_raw[i]) / 16384.0f - this->accel_offset[i] / SENSORS_GRAVITY_EARTH;
    }
    
    return accel;
}

std::array<float, 3> Mpu6050::gyro_from_raw(std::span<std::uint16_t, 3> gyro_raw) const
{
    // TODO: I assume this is the right constant. Maybe double check
    static constexpr float SENSORS_DPS_TO_RADS = 0.017453293F;

    std::array<float, 3> gyro;
    
    for(std::size_t i {0}; i < 3; ++i)
    {
        gyro[i] = (static_cast<float>(gyro_raw[i]) - this->gyro_offset[i]) * SENSORS_DPS_TO_RADS;
    }
    
    return gyro;
}

float Mpu6050::temp_from_raw(std::uint16_t temp_raw) const
{
    // TODO: Temperature is not yet used. Set to a nonzero value when needed. Idk what this would be. Check datasheet maybe.
    static constexpr float DEG_C_PER_DPS = 0.0;

    return static_cast<float>(temp_raw) * DEG_C_PER_DPS;
}

void Mpu6050::reset()
{
#ifdef i2c_default
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(i2c_default, addr, buf, 2, false);
    sleep_ms(100);
    buf[1] = 0x00;
    i2c_write_blocking(i2c_default, addr, buf, 2, false);
    sleep_ms(10);
#endif
}