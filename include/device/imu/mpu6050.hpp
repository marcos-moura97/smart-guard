#pragma once
#include "device/i2c.hpp"
#include <array>
#include <span>
#include <cstdint>
#include <data.hpp>
#include <memory>

class Mpu6050 final
{
public:
    using i2c_t = I2cDefault;

private:
    // MPU6050 I2C address
    const std::shared_ptr<I2cDefault> _i2c;
    const std::uint8_t _addr;

    const std::array<float, 3> _accel_offset;
    const std::array<float, 3> _gyro_offset;
    
public:
    Mpu6050(void) = delete;

    Mpu6050(
        I2cDefault i2c,
        std::uint8_t addr,
        std::array<float, 3> accel_offset,
        std::array<float, 3> gyro_offset
    );
    Mpu6050(
        std::shared_ptr<I2cDefault> i2c,
        std::uint8_t addr,
        std::array<float, 3> accel_offset,
        std::array<float, 3> gyro_offset
    );

    ImuData measure(void) const;

private:
    ImuDataRaw measure_raw(void) const;
    
    /// Acceleration from PCM to g
    std::array<float, 3> accel_from_raw(std::span<std::uint16_t, 3> accel_raw) const;

    // Angular acceleration from PCM to rad/s²
    std::array<float, 3> gyro_from_raw(std::span<std::uint16_t, 3> gyro_raw) const;

    // Note: unused as of writing. Therefore the scaling is just a placeholder.
    float temp_from_raw(std::uint16_t temp_raw) const;

    void reset() const;
};