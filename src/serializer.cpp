#include <serializer.hpp>

bool Serializer::toJson(const PacketData& packet, std::string& outJson)
    {
        char buffer[256]; // this can and should be changed, I couldn't think on another way

        const ImuData& imu = packet.sensor_data.imu;
        // the structure might also change, to reduce the size of the packet sent
        int len = std::snprintf(
            buffer,
            sizeof(buffer),
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
            imu.accel[0], imu.accel[1], imu.accel[2],
            imu.gyro[0], imu.gyro[1], imu.gyro[2],

            packet.direction.pitch,
            packet.direction.roll,

            packet.direction_filtered.pitch,
            packet.direction_filtered.roll,

            packet.free_fall_event,
            packet.impact_event,
            packet.tilt_change_event,
            packet.in_post_fall_monitor,
            packet.fall_detected
        );

        if (len <= 0 || len >= static_cast<int>(sizeof(buffer)))
            return false;

        outJson.assign(buffer, len);
        return true;
    }