#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"
#include "data.hpp"
#include <vector>

TEST_GROUP(PacketData)
{

};

/// Test deserialization of PacketData in CSV format
TEST(PacketData, SerdeCSV)
{
    // This might not be a plausible scenario. I just set random values here.
    const PacketData packet_lhs {
        .delta_time {PacketData::duration_t(1)},
        .sensor_data {
            .imu {
                .accel {
                    0.534,
                    0.215,
                    -0.95
                },
                // These are are omitted from the CSV representation sadly.
                // Setting to zero for now...
                // We might need a new data-set that also has the gyroscope data
                .gyro { 0.0, 0.0, 0.0 },

                // Unused, so setting to zero for now...
                .temp = 0.0
            }
        },
        .direction {
            .pitch = 3.150,
            .roll = -1.185
        },
        .direction_filtered {
            .pitch = 1.530,
            .roll = 0.21
        },
        .free_fall_event = true,
        .impact_event = false,
        .tilt_change_event = false,
        .in_post_fall_monitor = true,
        .fall_detected = false
    };

    const std::string csv {packet_lhs.format_csv()};
    std::stringstream csv_ss {csv};
    const std::vector<PacketData> packets_rhs {PacketData::parse_csv(csv_ss)};

    CHECK_EQUAL(1, packets_rhs.size());
    const PacketData& packet_rhs {packets_rhs.front()};

    // It's shame that we have to manually list every member here.
    // Not happy with this, but PacketData won't trivially implement operator== because it contains an std::array
    CHECK_EQUAL(packet_lhs.delta_time.count(), packet_rhs.delta_time.count());

    CHECK_EQUAL(packet_lhs.sensor_data.imu.accel[0], packet_rhs.sensor_data.imu.accel[0]);
    CHECK_EQUAL(packet_lhs.sensor_data.imu.accel[1], packet_rhs.sensor_data.imu.accel[1]);
    CHECK_EQUAL(packet_lhs.sensor_data.imu.accel[2], packet_rhs.sensor_data.imu.accel[2]);

    CHECK_EQUAL(packet_lhs.sensor_data.imu.gyro[0], packet_rhs.sensor_data.imu.gyro[0]);
    CHECK_EQUAL(packet_lhs.sensor_data.imu.gyro[1], packet_rhs.sensor_data.imu.gyro[1]);
    CHECK_EQUAL(packet_lhs.sensor_data.imu.gyro[2], packet_rhs.sensor_data.imu.gyro[2]);

    CHECK_EQUAL(packet_lhs.sensor_data.imu.temp, packet_rhs.sensor_data.imu.temp);

    CHECK_EQUAL(packet_lhs.direction.pitch, packet_rhs.direction.pitch);
    CHECK_EQUAL(packet_lhs.direction.roll, packet_rhs.direction.roll);
    
    CHECK_EQUAL(packet_lhs.direction_filtered.pitch, packet_rhs.direction_filtered.pitch);
    CHECK_EQUAL(packet_lhs.direction_filtered.roll, packet_rhs.direction_filtered.roll);

    CHECK_EQUAL(packet_lhs.free_fall_event, packet_rhs.free_fall_event);
    CHECK_EQUAL(packet_lhs.impact_event, packet_rhs.impact_event);
    CHECK_EQUAL(packet_lhs.tilt_change_event, packet_rhs.tilt_change_event);
    CHECK_EQUAL(packet_lhs.in_post_fall_monitor, packet_rhs.in_post_fall_monitor);
    CHECK_EQUAL(packet_lhs.fall_detected, packet_rhs.fall_detected);
}