#pragma once
#include "device/gpio.hpp"
#include "device/imu/mpu6050.hpp"
#include <cstdint>
#include <process.hpp>
#include <data.hpp>
#include <array>
#include <chrono>
#include <optional>
#include <pico/time.h>
#include <pico/types.h>
#include <dsp/fir_filter.hpp>
#include <Kalman.h>
#include <device/gpio.hpp>
#include <delta_time.hpp>

class FallDetector final : public Process
{
private:
    using microseconds_t = std::chrono::duration<std::uint64_t, std::micro>;

private:
    static constexpr float IMPACT_THRESHOLD {2.5f};
    static constexpr float IS_LYING_THRESHOLD {0.5f};
    static constexpr microseconds_t FREE_FALL_TRIGGER_TIME {200};
    static constexpr microseconds_t POST_FALL_TRIGGER_TIME {3000};
    static constexpr microseconds_t TILT_WINDOW_TIME {500};
    static constexpr std::array<float, 2> FILTER_COEFFS {0.9f, 0.1f};

    // State pertaining to fall detection events (flags and timestamps)
    struct Events final
    {
        struct FreeFall final
        {
            microseconds_t elapsed;
            bool is_free_fall {false};
        };
        // Free fall detection
        std::optional<FreeFall> free_fall {std::nullopt};

        struct Impact final
        {
            microseconds_t elapsed;
        };
        // Impact detection
        std::optional<Impact> impact {std::nullopt};

        struct TiltChange final
        {
            microseconds_t elapsed;
            bool was_lying_at_impact;
            bool is_tilt_change {false};
        };
        // Tilt change detection
        std::optional<TiltChange> tilt_change {std::nullopt};

        // Post-fall
        struct PostFall final
        {
            microseconds_t elapsed;
        };
        // Post-fall
        std::optional<PostFall> post_fall {std::nullopt};

        bool is_free_fall(void) const;
        bool is_impact(void) const;
        bool is_tilt_change(void) const;
        bool is_monitoring_post_fall(void) const;
    };

    GpioButton button;
    GpioEmergencyLed emergency_led;
    Mpu6050 imu;
    DeltaTime delta_time;

    // Events
    Events events {};

    // Data envelope
    float post_min_mag {0.0};
    float post_max_mag {0.0};

    // Warning status
    bool fall_detected {false};

    // Filtering
    std::array<FirFilter<1>, 3> filter_accel {FILTER_COEFFS, FILTER_COEFFS, FILTER_COEFFS};
    std::array<FirFilter<1>, 3> filter_gyro {FILTER_COEFFS, FILTER_COEFFS, FILTER_COEFFS};
    Kalman kalman_pitch {};
    Kalman kalman_roll {};
    bool is_kalman_calibrated {false};

public:
    //FallDetector(void) = delete;
    //FallDetector(const FallDetector&) = delete;

    FallDetector(GpioButton button, GpioEmergencyLed emergency_led, Mpu6050 imu);

protected:
    // The core algorithm for fall detection
    virtual Update update(void) override;

private:
    // Measures sensor data
    SensorData measure(void);

    // Detects events related to fall detection
    // Pre-collection events
    void analyze(const SensorData& sensor_data);

    // Post-collection events
    std::optional<bool> evaluate(const PacketData& packet_data) const;

    // Collects the data to be sent to the client / host
    PacketData collect(const SensorData& sensor_data);

    // Applies a kalman filter to the pitch and roll
    Direction filter_direction(Direction direction, const ImuData& imu_data);
};