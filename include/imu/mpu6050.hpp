#pragma once
#include <array>
#include <span>
#include <cstdint>
#include <data.hpp>

class Mpu6050 final
{
private:
    const std::array<float, 3> accel_offset;
    const std::array<float, 3> gyro_offset;

    // MPU6050 I2C address
    const std::uint8_t addr;

public:
    Mpu6050(void) = delete;

    Mpu6050(std::uint8_t addr, std::array<float, 3>&& accel_offset, std::array<float, 3>&& gyro_offset);

    ImuData measure(void);

private:
    ImuDataRaw measure_raw(void);
    
    /// Acceleration from PCM to g
    std::array<float, 3> accel_from_raw(std::span<std::uint16_t, 3> accel_raw) const;

    // Angular acceleration from PCM to rad/s²
    std::array<float, 3> gyro_from_raw(std::span<std::uint16_t, 3> gyro_raw) const;

    // Note: unused as of writing. Therefore the scaling is just a placeholder.
    float temp_from_raw(std::uint16_t temp_raw) const;

    void reset();
};