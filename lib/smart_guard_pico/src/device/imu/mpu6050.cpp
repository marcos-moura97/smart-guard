#include "data.hpp"
#include <bit>
#include <cstdio>
#include <device/imu/mpu6050.hpp>
#include <pico/time.h>

Mpu6050::Mpu6050(i2c_t i2c, std::uint8_t addr, AccelRange accel_range, GyroRange gyro_range):
    Mpu6050(std::make_shared<i2c_t>(std::move(i2c)), addr, accel_range, gyro_range)
{
    
}
Mpu6050::Mpu6050(std::shared_ptr<i2c_t> i2c, std::uint8_t addr, AccelRange accel_range, GyroRange gyro_range):
    _i2c(std::move(i2c)),
    _addr(addr),
    _accel_sensitivity(0.0),
    _gyro_sensitivity(0.0)
{
    this->reset();
    this->set_accel_range(accel_range);
    this->set_gyro_range(gyro_range);
    this->set_accel_hpf(AccelHpf::Reset);
}

ImuDataRaw Mpu6050::measure_raw(void) const
{
    if(!this->_i2c)
    {
        std::printf("Warning: no I2C interface attached.\n");
        // Return all zeroes if no i2c interface exists
        return ImuDataRaw {
            .accel = {0, 0, 0},
            .gyro = {0, 0 , 0},
            .temp = 0
        };
    }
    if(!this->is_connected())
    {
        std::printf("Warning: not connected!\n");
        /*return ImuDataRaw {
            .accel = {0, 0, 0},
            .gyro = {0, 0 , 0},
            .temp = 0
        };*/
    }

    std::printf("Info: measuring...\n");

    ImuDataRaw data; // Initialize with garbage, it will be overwritten

    // Accel registers 0x3B..0x40 (we read 6 bytes)
    static constexpr std::uint8_t REG_ACCEL {0x3B};
    this->_i2c->write_blocking(this->_addr, REG_ACCEL, true);
    this->_i2c->read_blocking(this->_addr, data.accel, false);
    for(std::int16_t& a : data.accel)
    {
        this->swap_msb_lsb(a);
    }

    // Gyro registers 0x43..0x48
    static constexpr std::uint8_t REG_GYRO {0x43};
    this->_i2c->write_blocking(this->_addr, REG_GYRO, true);
    this->_i2c->read_blocking(this->_addr, data.gyro, false);
    for(std::int16_t& g : data.gyro)
    {
        this->swap_msb_lsb(g);
    }

    // Temp registers 0x41..0x42
    static constexpr std::uint8_t REG_TEMP {0x41};
    this->_i2c->write_blocking(this->_addr, REG_TEMP, true);
    this->_i2c->read_blocking(this->_addr, data.temp, false);
    this->swap_msb_lsb(data.temp);
    
    // In case of mismatched endianness, a simple reinterpret cast is not sufficient
    if constexpr (std::endian::native == std::endian::big)
    {
        // TODO: handle wrong endian (all bits need to be reversed!)
        panic("MPU6050 support for big endian systems is not implemented");
    }
    
    return data;
}

ImuData Mpu6050::measure(void) const
{
    ImuDataRaw raw {this->measure_raw()};

    return {
        .accel = this->accel_from_raw(raw.accel),
        .gyro = this->gyro_from_raw(raw.gyro),
        .temp = this->temp_from_raw(raw.temp)
    };
}

void Mpu6050::set_accel_range(AccelRange range)
{
    static constexpr std::uint8_t REG_ACCEL_CONFIG {0x1C};

    std::uint8_t accel_config;
    this->_i2c->write_blocking(this->_addr, REG_ACCEL_CONFIG, true);
    this->_i2c->read_blocking(this->_addr, accel_config, false);

    static constexpr std::uint8_t SHIFT {3};
    static constexpr std::uint8_t MASK {0x3};
    accel_config &= ~(MASK << SHIFT);
    accel_config |= (static_cast<std::uint8_t>(range) & MASK) << SHIFT;

    this->_i2c->write_blocking(this->_addr, std::array{REG_ACCEL_CONFIG, accel_config}, false);

    this->_accel_sensitivity = this->accel_sensitivity(range);
}
void Mpu6050::set_accel_hpf(AccelHpf hpf)
{
    static constexpr std::uint8_t REG_ACCEL_CONFIG {0x1C};

    std::uint8_t accel_config;
    this->_i2c->write_blocking(this->_addr, REG_ACCEL_CONFIG, true);
    this->_i2c->read_blocking(this->_addr, accel_config, false);

    static constexpr std::uint8_t MASK {0x7};
    accel_config &= ~MASK;
    accel_config |= static_cast<std::uint8_t>(hpf) & MASK;

    this->_i2c->write_blocking(this->_addr, std::array{REG_ACCEL_CONFIG, accel_config}, false);
}
void Mpu6050::set_gyro_range(GyroRange range)
{
    static constexpr std::uint8_t REG_GYRO_CONFIG {0x1B};

    std::uint8_t gyro_config;
    this->_i2c->write_blocking(this->_addr, REG_GYRO_CONFIG, true);
    this->_i2c->read_blocking(this->_addr, gyro_config, false);

    static constexpr std::uint8_t SHIFT {3};
    static constexpr std::uint8_t MASK {0x3};
    gyro_config &= ~(MASK << SHIFT);
    gyro_config |= (static_cast<std::uint8_t>(range) & MASK) << SHIFT;

    this->_i2c->write_blocking(this->_addr, std::array{REG_GYRO_CONFIG, gyro_config}, false);

    this->_gyro_sensitivity = this->gyro_sensitivity(range);
}

Mpu6050::AccelRange Mpu6050::get_accel_range(void) const
{
    static constexpr std::uint8_t REG_ACCEL_CONFIG {0x1C};

    std::uint8_t accel_config;
    this->_i2c->write_blocking(this->_addr, REG_ACCEL_CONFIG, true);
    this->_i2c->read_blocking(this->_addr, accel_config, false);

    static constexpr std::uint8_t SHIFT {3};
    static constexpr std::uint8_t MASK {0x3};
    return static_cast<AccelRange>((accel_config >> SHIFT) & MASK);
}
Mpu6050::AccelHpf Mpu6050::get_accel_hpf(void) const
{
    static constexpr std::uint8_t REG_ACCEL_CONFIG {0x1C};

    std::uint8_t accel_config;
    this->_i2c->write_blocking(this->_addr, REG_ACCEL_CONFIG, true);
    this->_i2c->read_blocking(this->_addr, accel_config, false);

    static constexpr std::uint8_t MASK {0x7};
    return static_cast<AccelHpf>(accel_config & MASK);
}
Mpu6050::GyroRange Mpu6050::get_gyro_range(void) const
{
    static constexpr std::uint8_t REG_GYRO_CONFIG {0x1B};

    std::uint8_t gyro_config;
    this->_i2c->write_blocking(this->_addr, REG_GYRO_CONFIG, true);
    this->_i2c->read_blocking(this->_addr, gyro_config, false);

    static constexpr std::uint8_t SHIFT {3};
    static constexpr std::uint8_t MASK {0x3};
    return static_cast<GyroRange>((gyro_config >> SHIFT) & MASK);
}

std::array<float, 3> Mpu6050::accel_from_raw(std::span<std::int16_t, 3> accel_raw) const
{
    std::array<float, 3> accel;
    
    for(std::size_t i {0}; i < 3; ++i)
    {
        accel[i] = static_cast<float>(accel_raw[i]) / this->_accel_sensitivity;
    }
    
    return accel;
}

std::array<float, 3> Mpu6050::gyro_from_raw(std::span<std::int16_t, 3> gyro_raw) const
{
    // TODO: I assume this is the right constant. Maybe double check
    static constexpr float SENSORS_DPS_TO_RADS = 0.017453293F;

    std::array<float, 3> gyro;
    
    for(std::size_t i {0}; i < 3; ++i)
    {
        gyro[i] = static_cast<float>(gyro_raw[i]) * (RADS_PER_DEGREE / this->_gyro_sensitivity);
    }
    
    return gyro;
}

float Mpu6050::temp_from_raw(std::int16_t temp_raw) const
{
    return static_cast<float>(temp_raw) / TEMP_SENSITIVITY + TEMP_OFFSET;
}

void Mpu6050::reset() const
{
    if(!this->_i2c)
    {
        // Do nothing if no I2C interface exists
        return;
    }

    // Resets
    static constexpr std::array<std::uint8_t, 2> RESET {0x6B, 0x80};
    this->_i2c->write_blocking(this->_addr, RESET, false);
    sleep_ms(1000);

    // Configures internal clock
    static constexpr std::array<std::uint8_t, 2> RUN_INTERNAL_CLOCK {0x6B, 0x00};
    this->_i2c->write_blocking(this->_addr, RUN_INTERNAL_CLOCK, false);
    sleep_ms(10);
}
std::uint8_t Mpu6050::device_id(void) const
{
    static constexpr std::uint8_t WHOAMI {0x75};
    std::uint8_t device_id;
    this->_i2c->write_blocking(this->_addr, WHOAMI, true);
    this->_i2c->read_blocking(this->_addr, device_id, false);
    return device_id;
}
bool Mpu6050::is_connected(void) const
{
    std::uint8_t device_id {this->device_id()};
    return device_id == DEFAULT_SLAVE_ADDRESS;
}

void Mpu6050::swap_msb_lsb(std::int16_t& x)
{
    if(std::endian::native == std::endian::little)
    {
        std::uint8_t* msb {reinterpret_cast<std::uint8_t*>(&x)};
        std::uint8_t* lsb {msb + 1};
        std::swap(*msb, *lsb);
    }
}

float Mpu6050::accel_sensitivity(AccelRange range)
{
    return ACCEL_SENSITIVITY[static_cast<std::uint8_t>(range)];
}
float Mpu6050::gyro_sensitivity(GyroRange range)
{
    return GYRO_SENSITIVITY[static_cast<std::uint8_t>(range)];
}