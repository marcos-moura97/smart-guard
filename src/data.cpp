#include <cmath>
#include <cstdio>
#include <data.hpp>

std::array<float, 2> ImuData::accel_angles(void) const
{
    const auto [x, y, z] = this->accel;
    return {
        std::atan2(y, std::sqrt(x*x + z*z)),
        std::atan2(-x, std::sqrt(y*y + z*z))
    };
}

void PacketData::post(void) const
{
    const auto& [ax_g, ay_g, az_g] {this->sensor_data.imu.accel};

    // print similar CSV as original
    std::printf("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d,%d\n",
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