#include <cmath>
#include <data.hpp>
#include <format>

std::array<float, 2> ImuData::accel_angles(void) const
{
    const auto [x, y, z] = this->accel;
    return {
        std::atan2(y, std::sqrt(x*x + z*z)),
        std::atan2(-x, std::sqrt(y*y + z*z))
    };
}

std::string PacketData::format_csv(void) const
{
    const auto& [ax_g, ay_g, az_g] {this->sensor_data.imu.accel};

    // print similar CSV as original
    return std::format(
        "{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:d},{:d},{:d},{:d},{:d}",
        ax_g,
        ay_g,
        az_g,
        this->direction.pitch,
        this->direction.roll,
        this->direction_filtered.pitch, // pitch
        this->direction_filtered.roll,  // roll
        this->free_fall_event,
        this->impact_event,
        this->tilt_change_event,
        this->in_post_fall_monitor,
        this->fall_detected
    );
}

std::string PacketData::format_json() const
{
    std::array<char, 256> buffer{};

    const auto& [ax_g, ay_g, az_g] = this->sensor_data.imu.accel;
    const auto& [gx, gy, gz]       = this->sensor_data.imu.gyro;

    std::snprintf(
        buffer.data(),
        buffer.size(),
        "{"
            "\"imu\":{"
                "\"accel\":[%.3f,%.3f,%.3f],"
                "\"gyro\":[%.3f,%.3f,%.3f]"
            "},"
            "\"dir\":{"
                "\"pitch\":%.3f,"
                "\"roll\":%.3f"
            "},"
            "\"dir_filtered\":{"
                "\"pitch\":%.3f,"
                "\"roll\":%.3f"
            "},"
            "\"events\":{"
                "\"free_fall\":%d,"
                "\"impact\":%d,"
                "\"tilt_change\":%d,"
                "\"post_fall_monitor\":%d,"
                "\"fall_detected\":%d"
            "}"
        "}",
        ax_g, ay_g, az_g,
        gx, gy, gz,
        this->direction.pitch,
        this->direction.roll,
        this->direction_filtered.pitch,
        this->direction_filtered.roll,
        this->free_fall_event,
        this->impact_event,
        this->tilt_change_event,
        this->in_post_fall_monitor,
        this->fall_detected
    );

    return std::string(buffer.data());
}