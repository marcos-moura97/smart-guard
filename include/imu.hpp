#pragma once
#include <array>
#include <cstdint>

struct ImuDataRaw
{
    std::array<std::uint16_t, 3> accel;
    std::array<std::uint16_t, 3> gyro;
    std::uint16_t temp;
};

struct ImuData
{
    std::array<float, 3> accel;
    std::array<float, 3> gyro;
    float temp;
};