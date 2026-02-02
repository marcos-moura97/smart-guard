#include "data.hpp"
#include <device/imu/mpu6050.hpp>
#include <pico/time.h>

Mpu6050::Mpu6050(i2c_t i2c, std::uint8_t addr, std::array<float, 3> accel_offset, std::array<float, 3> gyro_offset):
    Mpu6050(std::make_shared<i2c_t>(std::move(i2c)), addr, std::move(accel_offset), std::move(gyro_offset))
{
    
}
Mpu6050::Mpu6050(std::shared_ptr<i2c_t> i2c, std::uint8_t addr, std::array<float, 3> accel_offset, std::array<float, 3> gyro_offset):
    _i2c(std::move(i2c)),
    _addr(addr),
    _accel_offset(std::move(accel_offset)),
    _gyro_offset(std::move(gyro_offset))
{
    std::printf("Hello, MPU6050! Reading raw data from registers...\n");
    this->reset();
    std::printf("Setup done! Starting reading.\n");
    std::printf("ax_g,ay_g,az_g,pitch,roll,acc_smv,gyro_smv,mag_g,lying\n");
}

ImuDataRaw Mpu6050::measure_raw(void) const
{
    if(!this->_i2c)
    {
        // Return all zeroes if no i2c interface exists
        return ImuDataRaw {
            .accel = {0, 0, 0},
            .gyro = {0, 0 , 0},
            .temp = 0
        };
    }

    ImuDataRaw data; // Initialize with garbage, it will be overwritten

    // Accel registers 0x3B..0x40 (we read 6 bytes)
    static constexpr std::uint8_t REG_ACCEL {0x3B};
    this->_i2c->write_blocking(this->_addr, REG_ACCEL, true);
    this->_i2c->read_blocking(this->_addr, data.accel, false);
    for(std::uint16_t& a : data.accel)
    {
        this->swap_msb_lsb(a);
    }

    // Gyro registers 0x43..0x48
    static constexpr std::uint8_t REG_GYRO {0x43};
    this->_i2c->write_blocking(this->_addr, REG_GYRO, true);
    this->_i2c->read_blocking(this->_addr, data.gyro, false);
    for(std::uint16_t& g : data.gyro)
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

std::array<float, 3> Mpu6050::accel_from_raw(std::span<std::uint16_t, 3> accel_raw) const
{
    static constexpr float SENSORS_GRAVITY_EARTH = 9.80665F;

    std::array<float, 3> accel;
    
    for(std::size_t i {0}; i < 3; ++i)
    {
        accel[i] = static_cast<float>(accel_raw[i]) / 16384.0f - this->_accel_offset[i] / SENSORS_GRAVITY_EARTH;
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
        gyro[i] = (static_cast<float>(gyro_raw[i]) - this->_gyro_offset[i]) * SENSORS_DPS_TO_RADS;
    }
    
    return gyro;
}

float Mpu6050::temp_from_raw(std::uint16_t temp_raw) const
{
    // TODO: Temperature is not yet used. Set to a nonzero value when needed. Idk what this would be. Check datasheet maybe.
    static constexpr float DEG_C_PER_DPS = 0.0;

    return static_cast<float>(temp_raw) * DEG_C_PER_DPS;
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

void Mpu6050::swap_msb_lsb(std::uint16_t& x)
{
    std::uint8_t* msb {reinterpret_cast<std::uint8_t*>(&x)};
    std::uint8_t* lsb {msb + 1};
    std::swap(*msb, *lsb);
}