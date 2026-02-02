#pragma once
#include <device/i2c.hpp>
#include <pico/binary_info/code.h>

template<uint PIN>
GpioI2c<PIN>::GpioI2c(void):
    Gpio<PIN>()
{
    gpio_set_function(PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PIN);
}

template<i2c_inst_t* I2C, uint PIN_SDA, uint PIN_SCL, uint BAUD_RATE>
I2c<I2C, PIN_SDA, PIN_SCL, BAUD_RATE>::I2cDevice::I2cDevice(void)
{
    i2c_init(I2C, BAUD_RATE);
}

template<i2c_inst_t* I2C, uint PIN_SDA, uint PIN_SCL, uint BAUD_RATE>
I2c<I2C, PIN_SDA, PIN_SCL, BAUD_RATE>::I2c(void):
    _i2c_dev(),
    _sda(),
    _scl()
{
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
}

template<i2c_inst_t* I2C, uint PIN_SDA, uint PIN_SCL, uint BAUD_RATE>
template<typename T>
void I2c<I2C, PIN_SDA, PIN_SCL, BAUD_RATE>::write_blocking(addr_t addr, const T& src, bool nostop) const
{
    i2c_write_blocking(
        i2c_default,
        addr,
        reinterpret_cast<const std::uint8_t*>(src),
        sizeof(T),
        nostop
    );
}

template<i2c_inst_t* I2C, uint PIN_SDA, uint PIN_SCL, uint BAUD_RATE>
template<typename T>
void I2c<I2C, PIN_SDA, PIN_SCL, BAUD_RATE>::write_blocking(addr_t addr, std::span<const T> src, bool nostop) const
{
    i2c_write_blocking(
        i2c_default,
        addr,
        reinterpret_cast<const std::uint8_t*>(src.data()),
        src.size_bytes(),
        nostop
    );
}

template<i2c_inst_t* I2C, uint PIN_SDA, uint PIN_SCL, uint BAUD_RATE>
template<typename T>
void I2c<I2C, PIN_SDA, PIN_SCL, BAUD_RATE>::read_blocking(addr_t addr, T dst, bool nostop) const
{
    i2c_read_blocking(
        i2c_default,
        addr,
        reinterpret_cast<std::uint8_t*>(dst),
        sizeof(T),
        nostop
    );
}

template<i2c_inst_t* I2C, uint PIN_SDA, uint PIN_SCL, uint BAUD_RATE>
template<typename T>
void I2c<I2C, PIN_SDA, PIN_SCL, BAUD_RATE>::read_blocking(addr_t addr, std::span<T> dst, bool nostop) const
{
    i2c_read_blocking(
        i2c_default,
        addr,
        reinterpret_cast<std::uint8_t*>(dst.data()),
        dst.size_bytes(),
        nostop
    );
}