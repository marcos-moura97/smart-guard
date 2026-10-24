#pragma once
#include <cstdint>
#include <device/input.hpp>
#include <device/i2c.hpp>
#include <data.hpp>
#include <array>
#include <memory>
#include <numbers>

class Mpu6050 final : public Input<ImuData>
{
public:
    static constexpr std::uint8_t DEFAULT_SLAVE_ADDRESS {0x68};
    static constexpr uint BAUD_RATE {400 * 1000};

    using i2c_t = I2cDefault<BAUD_RATE>;

    static constexpr std::array<float, 4> ACCEL_SENSITIVITY {16384., 8192., 4096., 2048.};
    static constexpr std::array<float, 4> GYRO_SENSITIVITY {131., 65.5, 32.8, 16.4};
    static constexpr float TEMP_OFFSET {36.53};
    static constexpr float TEMP_SENSITIVITY {340.};

    enum class AccelRange : std::uint8_t
    {
        G2 = 0,
        G4 = 1,
        G8 = 2,
        G16 = 3
    };

    enum class GyroRange : std::uint8_t
    {
        D250 = 0,
        D500 = 1,
        D1000 = 2,
        D2000 = 3
    };

    enum class AccelHpf {
        /// Cut off frequency: None
        Reset = 0,
        /// Cut off frequency: 5 Hz
        F5Hz = 1,
        /// Cut off frequency: 2.5 Hz
        F2_5Hz = 2,
        /// Cut off frequency: 1.25 Hz
        F1_25Hz = 3,
        /// Cut off frequency: 0.63 Hz
        F0_63Hz = 4,
        /// When triggered, the filter holds the present sample. The filter output will be the
        /// difference between the input sample and the held sample
        Hold = 7
    };

private:
    static constexpr float RADS_PER_DEGREE {std::numbers::pi/180.0};

private:
    // MPU6050 I2C address
    std::shared_ptr<i2c_t> _i2c;
    std::uint8_t _addr;
    float _accel_sensitivity;
    float _gyro_sensitivity;
    
public:
    Mpu6050(
        i2c_t i2c,
        std::uint8_t addr,
        AccelRange accel_range,
        GyroRange gyro_range
    );
    Mpu6050(
        std::shared_ptr<i2c_t> i2c,
        std::uint8_t addr,
        AccelRange accel_range,
        GyroRange gyro_range
    );

    virtual ImuData read(void) override;
    ImuDataRaw read_raw(void);

    void set_accel_range(AccelRange range);
    void set_accel_hpf(AccelHpf hpf);
    void set_gyro_range(GyroRange range);
    
    AccelRange get_accel_range(void) const;
    AccelHpf get_accel_hpf(void) const;
    GyroRange get_gyro_range(void) const;

    /**
    * Returns a 6 bit number corresponding to the device id
    */
    std::uint8_t device_id(void) const;
    /**
    * Tests if connection has been successful
    *
    * NOTE: currently not working
    */
    bool is_connected(void) const;
    
private:
    /// Acceleration from PCM to g
    std::array<float, 3> accel_from_raw(std::span<std::int16_t, 3> accel_raw) const;

    // Angular acceleration from PCM to rad/s²
    std::array<float, 3> gyro_from_raw(std::span<std::int16_t, 3> gyro_raw) const;

    // Note: unused as of writing. Therefore the scaling is just a placeholder.
    float temp_from_raw(std::int16_t temp_raw) const;

    void reset() const;

    static void swap_msb_lsb(std::int16_t& x);

    static float accel_sensitivity(AccelRange range);
    static float gyro_sensitivity(GyroRange range);
};