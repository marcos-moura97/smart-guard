#pragma once
#include <data.hpp>

class Device
{
public:
    // Returns scaled acceleration and gyroscope measurements
    // Acceleration is measured in g
    // Angular acceleration is measured in rad/s² (i think...)
    virtual ImuData measure_imu(void) = 0;

    virtual ~Device() = default;
};