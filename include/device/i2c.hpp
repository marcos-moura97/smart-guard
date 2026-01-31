#pragma once
#include <bit>
#include <cstdint>
#include <device/gpio.hpp>
#include <hardware/i2c.h>
#include <pico/binary_info/code.h>
#include <span>

template<i2c_inst_t* I2C, uint PIN_SDA, uint PIN_SCL, uint BAUD_RATE = 400 * 1000>
class I2c
{
public:
    using addr_t = std::uint8_t;
private:
    template<uint PIN>
    class GpioI2c : public Gpio<PIN>
    {
    public:
        GpioI2c(void):
            Gpio<PIN>()
        {
            gpio_set_function(PIN, GPIO_FUNC_I2C);
            gpio_pull_up(PIN);
        }
    };

    using sda_t = GpioI2c<PIN_SDA>;
    using scl_t = GpioI2c<PIN_SDA>;

    sda_t _sda;
    scl_t _scl;

public:
    I2c(void):
        _sda(),
        _scl()
    {
        i2c_init(I2C, BAUD_RATE);
        bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    }

    template<typename T>
    void write_blocking(addr_t addr, const T& src, bool nostop) const
    {
        i2c_write_blocking(
            i2c_default,
            addr,
            reinterpret_cast<const std::uint8_t*>(src),
            sizeof(T),
            nostop
        );
    }
    template<typename T>
    void write_blocking(addr_t addr, std::span<const T> src, bool nostop) const
    {
        i2c_write_blocking(
            i2c_default,
            addr,
            std::as_bytes(src).data(),
            src.size_bytes(),
            nostop
        );
    }

    template<typename T>
    void read_blocking(addr_t addr, T dst, bool nostop) const
    {
        i2c_read_blocking(
            i2c_default,
            addr,
            reinterpret_cast<std::uint8_t*>(dst),
            sizeof(T),
            nostop
        );
    }
    template<typename T>
    void read_blocking(addr_t addr, std::span<T> dst, bool nostop) const
    {
        i2c_read_blocking(
            i2c_default,
            addr,
            std::as_writable_bytes(dst).data(),
            dst.size_bytes(),
            nostop
        );
    }
};

#if defined(i2c_default) && defined(PICO_DEFAULT_I2C_SDA_PIN) && defined(PICO_DEFAULT_I2C_SCL_PIN)
using I2cDefault = I2c<i2c_default, PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN>;
#elif
#warning i2c/mpu6050_i2c example requires a board with I2C pins
#endif