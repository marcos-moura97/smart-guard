#pragma once
#include <array>
#include <cstdint>

struct ImuDataRaw final
{
    std::array<std::uint16_t, 3> accel;
    std::array<std::uint16_t, 3> gyro;
    std::uint16_t temp;
};

struct ImuData final
{
    std::array<float, 3> accel;
    std::array<float, 3> gyro;
    float temp;
};

struct Direction final
{
    float pitch;
    float roll;
};

struct SensorData final
{
    ImuData imu;
};

// The data to be sent to the client or host system
struct PacketData final
{
    SensorData sensor_data;
    Direction direction;
    Direction direction_filtered;
    bool free_fall_event;
    bool impact_event;
    bool tilt_change_event;
    bool in_post_fall_monitor;
    bool fall_detected;

    void post(void) const;
};