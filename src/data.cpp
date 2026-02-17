#include "connectivity/client/usart.hpp"
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

void PacketData::post(void) const
{
    Usart().send(*this);
}