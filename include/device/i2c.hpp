#pragma once
#include <cstdint>
#include <device/gpio.hpp>
#include <hardware/i2c.h>
#include <span>

template<uint PIN>
class GpioI2c : public Gpio<PIN>
{
public:
    GpioI2c(void);
};

template<i2c_inst_t* I2C, uint PIN_SDA, uint PIN_SCL, uint BAUD_RATE>
class I2c
{
public:
    using addr_t = std::uint8_t;
private:

    struct I2cDevice
    {
        I2cDevice();
    };

    using sda_t = GpioI2c<PIN_SDA>;
    using scl_t = GpioI2c<PIN_SCL>;

    I2cDevice _i2c_dev;
    sda_t _sda;
    scl_t _scl;

public:
    I2c(void);

    template<typename T>
    void write_blocking(addr_t addr, const T& src, bool nostop) const;
    template<typename T>
    void write_blocking(addr_t addr, std::span<const T> src, bool nostop) const;

    template<typename T>
    void read_blocking(addr_t addr, T dst, bool nostop) const;
    template<typename T>
    void read_blocking(addr_t addr, std::span<T> dst, bool nostop) const;
};

#if defined(i2c_default) && defined(PICO_DEFAULT_I2C_SDA_PIN) && defined(PICO_DEFAULT_I2C_SCL_PIN)
template<uint BAUD_RATE>
using I2cDefault = I2c<i2c_default, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, BAUD_RATE>;
#elif
#warning i2c/mpu6050_i2c example requires a board with I2C pins
#endif

#include <device/i2c.tpp>