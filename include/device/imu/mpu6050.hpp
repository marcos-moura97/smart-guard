#pragma once
#include <device/i2c.hpp>
#include <data.hpp>
#include <array>
#include <memory>

class Mpu6050 final
{
public:
    static constexpr uint BAUD_RATE {400 * 1000};

    using i2c_t = I2cDefault<BAUD_RATE>;

private:
    // MPU6050 I2C address
    std::shared_ptr<i2c_t> _i2c;
    std::uint8_t _addr;

    std::array<float, 3> _accel_offset;
    std::array<float, 3> _gyro_offset;
    
public:
    Mpu6050(
        i2c_t i2c,
        std::uint8_t addr,
        std::array<float, 3> accel_offset,
        std::array<float, 3> gyro_offset
    );
    Mpu6050(
        std::shared_ptr<i2c_t> i2c,
        std::uint8_t addr,
        std::array<float, 3> accel_offset,
        std::array<float, 3> gyro_offset
    );

    ImuData measure(void) const;

    ImuDataRaw measure_raw(void) const;
    
private:
    /// Acceleration from PCM to g
    std::array<float, 3> accel_from_raw(std::span<std::uint16_t, 3> accel_raw) const;

    // Angular acceleration from PCM to rad/s²
    std::array<float, 3> gyro_from_raw(std::span<std::uint16_t, 3> gyro_raw) const;

    // Note: unused as of writing. Therefore the scaling is just a placeholder.
    float temp_from_raw(std::uint16_t temp_raw) const;

    void reset() const;

    static void swap_msb_lsb(std::uint16_t& x);
};