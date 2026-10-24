#pragma once
#include "connectivity/send.hpp"
#include "device/input.hpp"
#include "device/output.hpp"
#include <cstdint>
#include <process/process.hpp>
#include <data.hpp>
#include <array>
#include <chrono>
#include <optional>
#include <dsp/fir_filter.hpp>
#include <dsp/kalman.hpp>
#include <delta_time.hpp>

namespace process
{
    class FallDetector final : public Process
    {
    private:
        using microseconds_t = std::chrono::duration<std::uint64_t, std::micro>;

    private:
        static constexpr float IMPACT_THRESHOLD {2.5f};
        static constexpr float IS_LYING_THRESHOLD {0.5f};
        static constexpr microseconds_t FREE_FALL_TRIGGER_TIME {200*1000};
        static constexpr microseconds_t POST_FALL_TRIGGER_TIME {3000*1000};
        static constexpr microseconds_t TILT_WINDOW_TIME {500*1000};
        static constexpr std::array<float, 2> FILTER_COEFFS {0.999f, 0.001f}; // These are just a guess, and need to adjusted better

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

        std::unique_ptr<Input<bool>> _button;
        std::unique_ptr<Output<bool>> _emergency_led;
        std::unique_ptr<Input<ImuData>> _imu;
        std::shared_ptr<Send<PacketData>> _client;

        // Events
        Events _events {};

        // Data envelope
        float _post_min_mag {0.0};
        float _post_max_mag {0.0};

        // Warning status
        bool _fall_detected {false};

        // Filtering
        std::array<dsp::FirFilter<1>, 3> _filter_accel {FILTER_COEFFS, FILTER_COEFFS, FILTER_COEFFS};
        std::array<dsp::FirFilter<1>, 3> _filter_gyro {FILTER_COEFFS, FILTER_COEFFS, FILTER_COEFFS};
        Kalman _kalman_pitch {};
        Kalman _kalman_roll {};
        bool _is_kalman_calibrated {false};

    public:
        //FallDetector(void) = delete;
        //FallDetector(const FallDetector&) = delete;

        FallDetector(
            std::unique_ptr<Input<bool>> button,
            std::unique_ptr<Output<bool>> emergency_led,
            std::unique_ptr<Input<ImuData>> imu,
            std::shared_ptr<Send<PacketData>> client
        );

        // The core algorithm for fall detection
        virtual Update update(duration_t dt) override;

    private:
        // Measures sensor data
        SensorData measure(void);

        // Detects events related to fall detection
        // Pre-collection events
        void analyze(const SensorData& sensor_data, duration_t dt);

        // Post-collection events
        std::optional<bool> evaluate(const PacketData& packet_data) const;

        // Collects the data to be sent to the client / host
        PacketData collect(const SensorData& sensor_data, duration_t dt);

        // Applies a kalman filter to the pitch and roll
        Direction filter_direction(Direction direction, const ImuData& imu_data, duration_t dt);
    };
}