#pragma once
#include <array>
#include <cstdint>
#include <string>

struct ImuData;

struct ImuDataRaw final
{
    std::array<std::int16_t, 3> accel;
    std::array<std::int16_t, 3> gyro;
    std::int16_t temp;
};

struct ImuData final
{
    std::array<float, 3> accel;
    std::array<float, 3> gyro;
    float temp;

    std::array<float, 2> accel_angles(void) const;
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

struct PingData final
{
    
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

    std::string format_csv(void) const;
    std::string format_json(void) const
    {
        // TODO: Add JSON formatting here later
        return "{\"error\": \"Placeholder... JSON implementation missing\"}";
    }

    [[deprecated]]
    void post(void) const;
};