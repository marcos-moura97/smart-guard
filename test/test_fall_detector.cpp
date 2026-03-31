#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"
#include "connectivity/send.hpp"
#include "data.hpp"
#include "device/input.hpp"
#include "process/fall_detector.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <vector>

TEST_GROUP(FallDetector)
{
    
};

namespace test_fall_detector
{
    class TestCase
    {
    private:
        using log_t = std::vector<PacketData>;

        static constexpr std::string_view CASE_DATA_DIRECTORY {"../../test/data/cases"};

        log_t _log;
        
    public:
        TestCase(void) = default;
        explicit TestCase(log_t log);

        void test(
            std::function<void(const PacketData&, const PacketData&)> check_packet
        ) const;

        static TestCase from_csv(std::filesystem::path csv_filename);
    };
}

/**
Cleaning test case.

TODO: describe test case more precisely

Shouldn't trigger any flags. Right? We're gonna check anyways
*/
TEST(FallDetector, TestCaseCleaning)
{
    using namespace test_fall_detector;

    TestCase::from_csv("cleaning.csv").test(
        [](const PacketData& output, const PacketData& log)
        {
            CHECK_FALSE(output.fall_detected);
            CHECK_FALSE(output.free_fall_event);
            CHECK_FALSE(output.impact_event);
            CHECK_FALSE(output.in_post_fall_monitor);
            CHECK_FALSE(output.tilt_change_event);
        }
    );
}

// TODO: add more cases

test_fall_detector::TestCase::TestCase(log_t log):
    _log(std::move(log))
{

}

test_fall_detector::TestCase test_fall_detector::TestCase::from_csv(std::filesystem::path csv_filename)
{
    std::filesystem::path case_data_directory {CASE_DATA_DIRECTORY};
    std::filesystem::path csv_path {case_data_directory / csv_filename};
    std::ifstream file {csv_path, std::fstream::in};
    CHECK_TRUE(file);

    return TestCase(PacketData::parse_csv(file));
}

void test_fall_detector::TestCase::test(
    std::function<void(const PacketData&, const PacketData&)> check_packet
) const
{
    auto iter = this->_log.begin();
    const auto end = this->_log.end();

    process::FallDetector fall_detector(
        nullptr,
        nullptr,
        std::make_unique<InputFn<ImuData>>(
            [&iter, &end]()
            {
                CHECK_TRUE(iter != end);
                ImuData imu_data {iter->sensor_data.imu};
                return imu_data;
            }
        ),
        std::make_shared<SendFn<PacketData>>(
            [&iter, &end, &check_packet](PacketData data)
            {
                CHECK_TRUE(iter != end);
                check_packet(data, *iter);
                return true;
            }
        )
    );

    for(; iter != end; ++iter)
    {
        fall_detector.update(
            std::chrono::duration_cast<process::FallDetector::duration_t>(std::chrono::milliseconds(20))
        );
    }
}