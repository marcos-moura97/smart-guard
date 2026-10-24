#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"
#include "connectivity/send.hpp"
#include "data.hpp"
#include "device/input.hpp"
#include "device/output.hpp"
#include "process/fall_detector.hpp"
#include <chrono>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace test_fall_detector
{
    using duration_t = process::FallDetector::duration_t;

    /// One recorded IMU sample, fed into the fall detector as a single update
    struct Sample final
    {
        duration_t delta_time;
        ImuData imu;
    };

    /// What the fall detector reported over a whole recording
    struct Summary final
    {
        std::size_t update_count {0};
        std::size_t packet_count {0};

        // Number of packets with each flag raised
        std::size_t free_fall_count {0};
        std::size_t impact_count {0};
        std::size_t tilt_change_count {0};
        std::size_t post_fall_monitor_count {0};
        std::size_t fall_detected_count {0};

        // Index of the first packet that reported a fall
        std::optional<std::size_t> first_fall_detected {std::nullopt};

        // State of the emergency LED after the last update
        bool emergency_led {false};
    };

    class TestCase final
    {
    private:
        using log_t = std::vector<Sample>;

        // Rate of the 2024 dataset, which has no delta-time column
        static constexpr duration_t LEGACY_DELTA_TIME {std::chrono::milliseconds(20)};

        log_t _log;

    public:
        TestCase(void) = default;
        explicit TestCase(log_t log);

        /**
        Feeds every sample of the recording through a fresh fall detector.

        The button is never pressed, so a detected fall stays latched until the end.

        @param check_packet Called for every packet sent, with the index of the sample that produced it
        */
        Summary run(
            const std::function<void(std::size_t, const PacketData&)>& check_packet = {}
        ) const;

        /**
        Loads a recording from the case data directory.

        Supported row layouts:
        - 7 columns: ax, ay, az, pitch, roll, pitch_filtered, roll_filtered
        - 12 columns: the above, followed by the five event flags
        - 13 columns: delta-time (µs), followed by the 12 columns above

        Any other line (e.g. "Event: IMPACT" log lines from the firmware) is skipped.
        */
        static TestCase from_csv(const std::filesystem::path& csv_filename);

        /// Appends a constant acceleration (in g) for the given time, sampled at the legacy rate
        TestCase& hold(std::array<float, 3> accel, std::chrono::milliseconds time);

    private:
        static std::filesystem::path case_data_directory(void);
        static std::optional<Sample> parse_row(const std::string& line);
    };

    /// Daily activity: the detector must never raise an alarm
    void check_no_fall(const Summary& summary)
    {
        CHECK_EQUAL(summary.update_count, summary.packet_count);
        CHECK_EQUAL(0, summary.fall_detected_count);
        CHECK_FALSE(summary.first_fall_detected.has_value());
        CHECK_FALSE(summary.emergency_led);
    }

    /// Actual fall: the detector must go through the full event chain and raise the alarm
    void check_fall(const Summary& summary)
    {
        CHECK_EQUAL(summary.update_count, summary.packet_count);
        CHECK_TRUE(summary.impact_count > 0);
        CHECK_TRUE(summary.post_fall_monitor_count > 0);
        CHECK_TRUE(summary.fall_detected_count > 0);
        CHECK_TRUE(summary.first_fall_detected.has_value());
        CHECK_TRUE(summary.emergency_led);

        // Once raised, the alarm stays latched since the button is never pressed
        CHECK_EQUAL(summary.packet_count - *summary.first_fall_detected, summary.fall_detected_count);
    }
}

TEST_GROUP(FallDetector)
{

};

/**
Sanity check of the harness: every sample produces exactly one packet,
carrying the delta-time it was updated with.
*/
TEST(FallDetector, OnePacketPerSample)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("walking.csv").run(
        [](std::size_t, const PacketData& output)
        {
            CHECK_EQUAL(
                std::chrono::duration_cast<PacketData::duration_t>(std::chrono::milliseconds(20)).count(),
                output.delta_time.count()
            );
        }
    )};

    CHECK_EQUAL(3144, summary.update_count);
    CHECK_EQUAL(summary.update_count, summary.packet_count);
}

/**
Cleaning test case.

Moving around while cleaning. No fall, and no impacts strong enough to trigger anything.
*/
TEST(FallDetector, TestCaseCleaning)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("cleaning.csv").run()};

    check_no_fall(summary);
    CHECK_EQUAL(0, summary.free_fall_count);
    CHECK_EQUAL(0, summary.impact_count);
    CHECK_EQUAL(0, summary.tilt_change_count);
    CHECK_EQUAL(0, summary.post_fall_monitor_count);
}

/**
Walking test case.

Regular walking. No fall, and no impacts strong enough to trigger anything.
*/
TEST(FallDetector, TestCaseWalking)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("walking.csv").run()};

    check_no_fall(summary);
    CHECK_EQUAL(0, summary.free_fall_count);
    CHECK_EQUAL(0, summary.impact_count);
    CHECK_EQUAL(0, summary.tilt_change_count);
    CHECK_EQUAL(0, summary.post_fall_monitor_count);
}

/**
Playing with Aspira test case.

Playing with a dog. No fall, and no impacts strong enough to trigger anything.
*/
TEST(FallDetector, TestCasePlayingWithDog)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("playing_with_dog.csv").run()};

    check_no_fall(summary);
    CHECK_EQUAL(0, summary.free_fall_count);
    CHECK_EQUAL(0, summary.impact_count);
    CHECK_EQUAL(0, summary.tilt_change_count);
    CHECK_EQUAL(0, summary.post_fall_monitor_count);
}

/**
Running test case.

Foot strikes produce impacts and brief posture changes, which may start post-fall monitoring.
The wearer stays upright though, so the evaluation must reject it and no fall must be reported.
*/
TEST(FallDetector, TestCaseRunning)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("running.csv").run()};

    check_no_fall(summary);
    CHECK_TRUE(summary.impact_count > 0);
}

/**
Jumping test case.

Movement produces free falls, impacts and brief posture changes, which may start post-fall monitoring
(the original firmware did too). The wearer stays upright though, so the evaluation must reject it and no fall must be reported.
*/
TEST(FallDetector, TestCaseJumping)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("jumping.csv").run()};

    check_no_fall(summary);
    CHECK_TRUE(summary.impact_count > 0);
}

/**
Sitting test case.

Sitting down calmly. No fall, and no impacts strong enough to trigger anything.
*/
TEST(FallDetector, TestCaseSitting)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("test_sitting.csv").run()};

    check_no_fall(summary);
    CHECK_EQUAL(0, summary.free_fall_count);
    CHECK_EQUAL(0, summary.impact_count);
    CHECK_EQUAL(0, summary.post_fall_monitor_count);
}

/**
Sitting and standing test case.

Sitting down and standing up again. No fall, and no impacts strong enough to trigger anything.
*/
TEST(FallDetector, TestCaseSittingStanding)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("test_sitting_standing.csv").run()};

    check_no_fall(summary);
    CHECK_EQUAL(0, summary.free_fall_count);
    CHECK_EQUAL(0, summary.impact_count);
    CHECK_EQUAL(0, summary.post_fall_monitor_count);
}

/**
Lying down test case.

Lying down on purpose. The wearer ends up lying still, but without any impact, so no fall must be reported.
*/
TEST(FallDetector, TestCaseLyingDown)
{
    using namespace test_fall_detector;

    const Summary summary {TestCase::from_csv("test_laying_down.csv").run()};

    check_no_fall(summary);
    CHECK_EQUAL(0, summary.free_fall_count);
    CHECK_EQUAL(0, summary.impact_count);
    CHECK_EQUAL(0, summary.post_fall_monitor_count);
}

/**
Pitch fall test case.

Falling forwards/backwards (pitch axis), then lying still. Must be reported as a fall.
*/
TEST(FallDetector, TestCaseFallPitch)
{
    using namespace test_fall_detector;

    check_fall(TestCase::from_csv("test_fall_pitch_2.csv").run());
}

/**
Roll fall test case.

Falling to the side (roll axis), then lying still. Must be reported as a fall.
*/
TEST(FallDetector, TestCaseFallRoll)
{
    using namespace test_fall_detector;

    check_fall(TestCase::from_csv("test_fall2_roll.csv").run());
}

namespace test_fall_detector
{
    // Synthetic postures, in g
    constexpr std::array<float, 3> UPRIGHT {0.0f, 1.0f, 0.0f};
    constexpr std::array<float, 3> FALLING {0.0f, 0.1f, 0.0f};
    constexpr std::array<float, 3> HIT {0.0f, 3.0f, 0.0f};
    constexpr std::array<float, 3> LYING {1.0f, 0.0f, 0.0f};
}

/**
Synthetic fall: free fall, impact, then lying still.

Free fall and impact must count together as two events, even though the free fall has ended by the time of impact.
*/
TEST(FallDetector, SyntheticFreeFallThenImpact)
{
    using namespace test_fall_detector;
    using namespace std::chrono_literals;

    const Summary summary {TestCase()
        .hold(UPRIGHT, 1000ms)
        .hold(FALLING, 300ms)
        .hold(HIT, 20ms)
        .hold(LYING, 5000ms)
        .run()};

    CHECK_TRUE(summary.free_fall_count > 0);
    check_fall(summary);
}

/**
Synthetic false alarm: an impact (e.g. a stomp), then much later the device is dropped and lies still.

The stale impact must have expired, so the drop alone is not enough for a fall.
*/
TEST(FallDetector, SyntheticStaleImpactExpires)
{
    using namespace test_fall_detector;
    using namespace std::chrono_literals;

    const Summary summary {TestCase()
        .hold(UPRIGHT, 1000ms)
        .hold(HIT, 20ms)
        .hold(UPRIGHT, 3000ms)
        .hold(FALLING, 300ms)
        .hold(LYING, 5000ms)
        .run()};

    CHECK_TRUE(summary.impact_count > 0);
    CHECK_TRUE(summary.free_fall_count > 0);
    check_no_fall(summary);
    CHECK_EQUAL(0, summary.post_fall_monitor_count);
}

/**
Synthetic false alarm: a free fall (e.g. a jump), then much later an impact while upright and lying down calmly.

The stale free fall must have expired, so the later impact alone is not enough for a fall.
*/
TEST(FallDetector, SyntheticStaleFreeFallExpires)
{
    using namespace test_fall_detector;
    using namespace std::chrono_literals;

    const Summary summary {TestCase()
        .hold(UPRIGHT, 1000ms)
        .hold(FALLING, 300ms)
        .hold(UPRIGHT, 3000ms)
        .hold(HIT, 20ms)
        .hold(UPRIGHT, 1000ms)
        .hold(LYING, 5000ms)
        .run()};

    CHECK_TRUE(summary.impact_count > 0);
    CHECK_TRUE(summary.free_fall_count > 0);
    check_no_fall(summary);
    CHECK_EQUAL(0, summary.post_fall_monitor_count);
}

test_fall_detector::TestCase& test_fall_detector::TestCase::hold(std::array<float, 3> accel, std::chrono::milliseconds time)
{
    for(duration_t elapsed {duration_t::zero()}; elapsed < time; elapsed += LEGACY_DELTA_TIME)
    {
        this->_log.push_back(Sample {
            .delta_time {LEGACY_DELTA_TIME},
            .imu {
                .accel {accel},
                .gyro {0.0f, 0.0f, 0.0f},
                .temp = 0.0f
            }
        });
    }
    return *this;
}

test_fall_detector::TestCase::TestCase(log_t log):
    _log(std::move(log))
{

}

std::filesystem::path test_fall_detector::TestCase::case_data_directory(void)
{
    // Relative to this source file, so the tests don't depend on the working directory
    return std::filesystem::path(__FILE__).parent_path() / "data" / "cases";
}

std::optional<test_fall_detector::Sample> test_fall_detector::TestCase::parse_row(const std::string& line)
{
    std::vector<float> values;
    std::stringstream line_ss {line};
    std::string token;
    while(std::getline(line_ss, token, ','))
    {
        try
        {
            std::size_t parsed_length {0};
            values.push_back(std::stof(token, &parsed_length));
            if(parsed_length != token.size())
            {
                return std::nullopt;
            }
        }
        catch(const std::logic_error&)
        {
            // Not a number, so not a data row
            return std::nullopt;
        }
    }

    duration_t delta_time {LEGACY_DELTA_TIME};
    auto accel {values.begin()};
    switch(values.size())
    {
        case 7:
        case 12:
            break;
        case 13:
            delta_time = duration_t(static_cast<duration_t::rep>(values.front()));
            ++accel;
            break;
        default:
            return std::nullopt;
    }

    return Sample {
        .delta_time {delta_time},
        .imu {
            .accel {accel[0], accel[1], accel[2]},
            // The recordings don't contain gyroscope data
            .gyro {0.0f, 0.0f, 0.0f},
            .temp = 0.0f
        }
    };
}

test_fall_detector::TestCase test_fall_detector::TestCase::from_csv(const std::filesystem::path& csv_filename)
{
    const std::filesystem::path csv_path {case_data_directory() / csv_filename};
    std::ifstream file {csv_path};
    CHECK_TEXT(file.is_open(), csv_path.c_str());

    log_t log;
    std::string line;
    while(std::getline(file, line))
    {
        if(auto sample {parse_row(line)})
        {
            log.push_back(*sample);
        }
    }
    CHECK_TEXT(!log.empty(), csv_path.c_str());

    return TestCase(std::move(log));
}

test_fall_detector::Summary test_fall_detector::TestCase::run(
    const std::function<void(std::size_t, const PacketData&)>& check_packet
) const
{
    Summary summary {};
    std::size_t index {0};

    process::FallDetector fall_detector(
        // Button is active-low; never pressed
        std::make_unique<InputFn<bool>>(
            []()
            {
                return true;
            }
        ),
        std::make_unique<OutputFn<bool>>(
            [&summary](bool led)
            {
                summary.emergency_led = led;
            }
        ),
        std::make_unique<InputFn<ImuData>>(
            [this, &index]()
            {
                CHECK_TRUE(index < this->_log.size());
                return this->_log[index].imu;
            }
        ),
        std::make_shared<SendFn<PacketData>>(
            [&summary, &index, &check_packet](PacketData packet)
            {
                ++summary.packet_count;
                summary.free_fall_count += packet.free_fall_event;
                summary.impact_count += packet.impact_event;
                summary.tilt_change_count += packet.tilt_change_event;
                summary.post_fall_monitor_count += packet.in_post_fall_monitor;
                summary.fall_detected_count += packet.fall_detected;
                if(packet.fall_detected && !summary.first_fall_detected.has_value())
                {
                    summary.first_fall_detected = index;
                }

                if(check_packet)
                {
                    check_packet(index, packet);
                }
                return true;
            }
        )
    );

    for(; index < this->_log.size(); ++index)
    {
        fall_detector.update(this->_log[index].delta_time);
        ++summary.update_count;
    }

    return summary;
}
