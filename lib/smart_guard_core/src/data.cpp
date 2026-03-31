#include <cassert>
#include <cstddef>
#include <data.hpp>
#include <cmath>
#include <format>
#include <iterator>
#include <string>
#include <vector>

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
        "{:},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:d},{:d},{:d},{:d},{:d}",
        this->delta_time.count(),
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

std::vector<PacketData> PacketData::parse_csv(std::istream& input_stream)
{
    using istream_iter_t = std::istreambuf_iterator<char>;

    static constexpr std::size_t EXPECTED_TOKEN_COUNT {13};

    std::vector<PacketData> packets;
    std::vector<std::string> tokens;
    tokens.reserve(EXPECTED_TOKEN_COUNT);
    for(;;)
    {
        bool done {!input_stream};
        char c;
        if(!done)
        {
            input_stream >> c;
        }
        if(done || c == '\n')
        {
            if(!tokens.empty())
            {
                assert(tokens.size() == 13);

                auto token_iter {tokens.begin()};
                auto token_end {tokens.end()};

                auto next_token {
                    [&token_iter]()
                    {
                        assert(token_iter != token_end);
                        return *token_iter++;
                    }
                };

                PacketData packet {
                    .delta_time {std::stol(next_token())},
                    .sensor_data {
                        .imu {
                            .accel {
                                std::stof(next_token()),
                                std::stof(next_token()),
                                std::stof(next_token())
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
                        .pitch = std::stof(next_token()),
                        .roll = std::stof(next_token())
                    },
                    .direction_filtered {
                        .pitch = std::stof(next_token()),
                        .roll = std::stof(next_token())
                    },
                    .free_fall_event = std::stoi(next_token()) != 0,
                    .impact_event = std::stoi(next_token()) != 0,
                    .tilt_change_event = std::stoi(next_token()) != 0,
                    .in_post_fall_monitor = std::stoi(next_token()) != 0,
                    .fall_detected = std::stoi(next_token()) != 0
                };

                assert(token_iter == token_end);
                tokens.clear();
                packets.push_back(std::move(packet));
            }

            if(done)
            {
                break;
            }
        }
        else if(c == ',')
        {
            tokens.push_back(std::string());
        }
        else
        {
            if(tokens.empty())
            {
                tokens.push_back(std::string());
            }
            tokens.back().push_back(c);
        }
    }
    return packets;
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
            "\"delta_time\": %li"
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
        this->delta_time.count(),
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