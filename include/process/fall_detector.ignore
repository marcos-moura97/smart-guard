#pragma once
#include <process.hpp>
#include <data.hpp>
#include <array>
#include <chrono>
#include <concepts>
#include <memory>
#include <optional>
#include <pico/time.h>
#include <pico/types.h>
#include <device.hpp>
#include <filter.hpp>
#include <ratio>
#include <Kalman.h>

class FallDetector final : public Process
{
private:
    // GPIO / I2C constants
    static constexpr uint LED_GPIO {19}; // stop button
    static constexpr uint BUTTON_GPIO {9}; // Emergency LED

    static constexpr float IMPACT_THRESHOLD {2.5f};
    static constexpr float IS_LYING_THRESHOLD {0.5f};
    static constexpr std::chrono::microseconds POST_FALL_TRIGGER_TIME {3000};

    // Blinking LED
    static constexpr std::chrono::milliseconds BLINK_PERIOD {50};

    std::shared_ptr<Device> device;
    absolute_time_t time;

    // Filtering
    static constexpr std::array<float, 2> FILTER_COEFFS {0.9f, 0.1f};
    std::array<FirFilter<1>, 3> filter_accel {FILTER_COEFFS, FILTER_COEFFS, FILTER_COEFFS};
    std::array<FirFilter<1>, 3> filter_gyro {FILTER_COEFFS, FILTER_COEFFS, FILTER_COEFFS};
    Kalman kalman_pitch {};
    Kalman kalman_roll {};
    bool is_kalman_calibrated {false};

    struct Events final
    {
        struct FreeFall final
        {
            duration_t elapsed;
            bool is_free_fall {false};
        };
        // Free fall detection
        std::optional<FreeFall> free_fall {std::nullopt};

        struct Impact final
        {
            duration_t elapsed;
        };
        // Impact detection
        std::optional<Impact> impact {std::nullopt};

        struct TiltChange final
        {
            duration_t elapsed;
            bool was_lying_at_impact;
            bool is_tilt_change {false};
        };
        // Tilt change detection
        std::optional<TiltChange> tilt_change {std::nullopt};

        // Post-fall
        struct PostFall final
        {
            duration_t elapsed;
        };
        // Post-fall
        std::optional<PostFall> post_fall {std::nullopt};

        void increment_time(duration_t dt);

        bool is_free_fall(void) const;
        bool is_impact(void) const;
        bool is_tilt_change(void) const;
        bool is_monitoring_post_fall(void) const;
    };

    // State pertaining to fall detection events (flags and timestamps)
    Events events;

    // Data
    float post_min_mag;
    float post_max_mag;

    // Warning status
    bool fall_detected;

public:
    FallDetector(void) = delete;
    FallDetector(const FallDetector&) = delete;

    FallDetector(std::shared_ptr<Device> device);

private:
    // The core algorithm for fall detection
    void update(void);

    // Measures sensor data
    SensorData measure(void);

    // Detects events related to fall detection
    // Pre-collection events
    void analyze(const SensorData& sensor_data);

    // Post-collection events
    std::optional<bool> evaluate(const PacketData& packet_data) const;

    // Collects the data to be sent to the client / host
    PacketData collect(const SensorData& sensor_data, duration_t dt);

    // Applies a kalman filter to the pitch and roll
    Direction filter_direction(Direction direction, const ImuData& imu_data, duration_t dt);

    // Returns duration since last call of the method in seconds
    duration_t delta_time(void);
};