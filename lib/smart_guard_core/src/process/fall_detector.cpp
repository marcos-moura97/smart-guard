#include "connectivity/send.hpp"
#include <cstdio>
#include <data.hpp>
#include <delta_time.hpp>
#include <cassert>
#include <chrono>
#include <cmath>
#include <numbers>
#include <optional>
#include <process/fall_detector.hpp>

process::FallDetector::FallDetector(
    std::unique_ptr<Input<bool>> button,
    std::unique_ptr<Output<bool>> emergency_led,
    std::unique_ptr<Input<ImuData>> imu,
    std::shared_ptr<Send<PacketData>> client
):
    _button(std::move(button)),
    _emergency_led(std::move(emergency_led)),
    _imu(std::move(imu)),
    _client(std::move(client))
{
    
}

process::Update process::FallDetector::update(duration_t dt)
{
    const SensorData sensor_data {this->measure()};

    this->analyze(sensor_data, dt);

    const PacketData packet_data {this->collect(sensor_data, dt)};
    this->_client->send(packet_data);

    const std::optional<bool> evaluation {this->evaluate(packet_data)};
    if(evaluation.has_value()) // An evaluation has been made
    {
        // Was it a fall?
        if(*evaluation)
        {
            //printf("FINAL FALL DETECTED (STEP 1)\n");
            this->_fall_detected = true;
            this->_emergency_led->write(false);
        }
        else
        {
            //printf("Post-fall rejected with mag_g = %.3f and \n",mag_g);
        }

        // Reset state
        this->_events = {};
    }

    // alert button handle check
    if(this->_fall_detected)
    {
        if(!this->_button->read())
        {
            this->_emergency_led->write(false);
            this->_fall_detected = false;
        }
        else
        {
            this->_emergency_led->write(true);
        }
    }

    return Update::pending;
}

SensorData process::FallDetector::measure(void)
{
    // Read from IMU
    ImuData imu_dat {this->_imu->read()};

    // Low pass on floating point values
    // NOTE: Previous implementation did this on the PCM values, so this implementation may lead to slightly different results.
    // TODO: Investigate: why was PCM used previously for the filtering stage? Maybe there was a good reason.
    for(std::size_t i {0}; i < 3; ++i)
    {
        imu_dat.accel[i] = this->_filter_accel[i](imu_dat.accel[i]);
        imu_dat.gyro[i] = this->_filter_gyro[i](imu_dat.gyro[i]);
    }

    return {
        .imu {std::move(imu_dat)}
    };
}

// Detects events related to fall detection
// Pre-collection events
void process::FallDetector::analyze(const SensorData& sensor_data, duration_t dt)
{
    const auto [ax_g, ay_g, az_g] = sensor_data.imu.accel;
    const auto [gx, gy, gz] = sensor_data.imu.gyro;
    const float mag_g {
        std::sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g)
    };

    // Events are kept while monitoring post-fall, until the fall has been evaluated
    const bool may_expire {!this->_events.is_monitoring_post_fall()};

    // 3) FREE-FALL detection
    if(this->_events.is_free_fall())
    {
        // Event stays latched, so it can be combined with the impact that follows
        this->_events.free_fall->elapsed += dt;
        if(may_expire && this->_events.free_fall->elapsed >= this->EVENT_TIMEOUT_TIME)
        {
            this->_events.free_fall = std::nullopt;
        }
    }
    else if (!(mag_g < 0.4f))
    {
        // Reset free fall counter
        this->_events.free_fall = std::nullopt;
    }
    else if (!this->_events.free_fall.has_value())
    {
        // Start free fall counter
        this->_events.free_fall = {
            .elapsed {microseconds_t::zero()},
            .is_free_fall = false
        };
    }
    else
    {
        this->_events.free_fall->elapsed += dt;
        if(this->_events.free_fall->elapsed >= this->FREE_FALL_TRIGGER_TIME)
        {
            // Restart the counter to time the event's expiry
            this->_events.free_fall = {
                .elapsed {microseconds_t::zero()},
                .is_free_fall = true
            };
            std::printf("Event: FREE FALL\n");
        }
    }

    // 4) IMPACT detection
    const bool is_lying_now {std::abs(az_g) < IS_LYING_THRESHOLD && std::abs(ax_g) > IS_LYING_THRESHOLD};
    if(this->_events.impact.has_value())
    {
        this->_events.impact->elapsed += dt;
        if(may_expire && this->_events.impact->elapsed >= this->EVENT_TIMEOUT_TIME)
        {
            this->_events.impact = std::nullopt;
            this->_events.tilt_change = std::nullopt;
        }
    }
    else if(mag_g > this->IMPACT_THRESHOLD)
    {
        this->_events.impact = {
            .elapsed {microseconds_t::zero()}
        };
        this->_events.tilt_change = {
            .elapsed {microseconds_t::zero()},
            .was_lying_at_impact = is_lying_now,
            .is_tilt_change = false
        };
        std::printf("Event: IMPACT  mag=%.2f\n", mag_g);
    }

    // 5) POSTURE CHANGE within 500ms
    if(this->_events.tilt_change.has_value())
    {
        // Only look for a change within the window, but keep the event once it has happened
        if(this->_events.tilt_change->elapsed <= this->TILT_WINDOW_TIME
            && !this->_events.tilt_change->is_tilt_change
            && is_lying_now != this->_events.tilt_change->was_lying_at_impact)
        {
            this->_events.tilt_change->is_tilt_change = true;
            std::printf("Event: POSTURE CHANGE\n");
        }
        this->_events.tilt_change->elapsed += dt;
    }

    // 6) ENTER POST-FALL MONITORING (2 of 3 events + lying)
    if (this->_events.post_fall.has_value())
    {
        this->_events.post_fall->elapsed += dt;
    }
    else
    {
        const int event_count = 
            static_cast<std::uint8_t>(this->_events.is_free_fall())
            + static_cast<std::uint8_t>(this->_events.is_impact())
            + static_cast<std::uint8_t>(this->_events.is_tilt_change());
        if(event_count >= 2 && is_lying_now) {
            this->_events.post_fall = {
                .elapsed {microseconds_t::zero()}
            };
            std::printf("Entering post-fall monitoring...\n");
        }
    }
}

// Post-collection events
std::optional<bool> process::FallDetector::evaluate(const PacketData& packet_data) const
{
    // 7) FALL DETECTION (TRIGGER WARNING)
    if (this->_events.post_fall.has_value())
    {
        const auto [ax_g, ay_g, az_g] = packet_data.sensor_data.imu.accel;
        const auto [gx, gy, gz] = packet_data.sensor_data.imu.gyro;
        // Acceleration magnitude
        const float mag_g {
            std::sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g)
        };
        
        // TODO: try this alternate one later:
        //const bool is_lying_now = std::sqrt((packet_data.direction.roll - 90.0f)*(packet_data.direction.roll - 90.0f) + packet_data.direction.pitch*packet_data.direction.pitch) > 70.0f;
        const bool is_lying_now = std::abs(packet_data.direction.roll) < 20.0f
            || std::abs(packet_data.direction.roll) > 150.0f
            || std::abs(packet_data.direction.pitch) > 70.0f; // rectangular (original #2)
            //(fabsf(az_g) < 0.5f && fabsf(ax_g) > 0.5f);

        if (this->_events.post_fall->elapsed >= POST_FALL_TRIGGER_TIME)
        {
            const bool near1g = (mag_g >= 0.75f && mag_g <= 1.3f);

            return near1g && is_lying_now;
        }
    }
    return std::nullopt;
}

// Collects the data to be sent to the client / host
PacketData process::FallDetector::collect(const SensorData& sensor_data, duration_t dt)
{
    const auto [ax_g, ay_g, az_g] = sensor_data.imu.accel;
    const auto [gx, gy, gz] = sensor_data.imu.gyro;
    const float mag_g {
        std::sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g)
    };

    // store stats
    this->_post_min_mag = std::min(this->_post_min_mag, mag_g);
    this->_post_max_mag = std::max(this->_post_max_mag, mag_g);

    static constexpr float DEG_PER_RAD {180.0f / std::numbers::pi_v<float>};

    // orientation from accelerometer (degrees)
    const Direction direction {
        .pitch = std::atan2(-ax_g, std::sqrt(ay_g*ay_g + az_g*az_g)) * DEG_PER_RAD,
        .roll = std::atan2(ay_g, az_g) * DEG_PER_RAD
    };

    const Direction direction_filtered {this->filter_direction(direction, sensor_data.imu, dt)};

    return PacketData {
        .delta_time {dt},
        .sensor_data {sensor_data},
        .direction {direction},
        .direction_filtered {direction_filtered},
        
        .free_fall_event = this->_events.is_free_fall(),
        .impact_event = this->_events.is_impact(),
        .tilt_change_event = this->_events.is_tilt_change(),
        .in_post_fall_monitor = this->_events.is_monitoring_post_fall(),
        .fall_detected = this->_fall_detected
    };
}

// Applies a kalman filter to the pitch and roll
Direction process::FallDetector::filter_direction(Direction direction, const ImuData& imu_data, duration_t dt)
{
    static constexpr double KALMAN_TORQUE_SCALING {131.0}; // What does this number correspond to?

    // Set initial value to avoid transient
    if(!this->_is_kalman_calibrated) {
        this->_kalman_roll.setAngle(static_cast<double>(direction.roll));
        this->_kalman_pitch.setAngle(static_cast<double>(direction.pitch));
        this->_is_kalman_calibrated = true;
    }

    const auto [gx, gy, gz] = imu_data.gyro;

    /*const*/ double dt_seconds {std::chrono::duration_cast<std::chrono::duration<double>>(dt).count()};

    // NOTE: I assume this helps with floating-point overflow/underflow, but wouldn't a std::clamp make more sense? Keeping it as is for now.
    if (dt_seconds <= 0.0 || dt_seconds > 0.1) dt_seconds = 0.02;

    direction.pitch = static_cast<float>(this->_kalman_pitch.getAngle(
        static_cast<double>(direction.pitch),
        static_cast<double>(gy)*KALMAN_TORQUE_SCALING,
        dt_seconds
    ));
    direction.roll = static_cast<float>(this->_kalman_roll.getAngle(
        static_cast<double>(direction.roll),
        static_cast<double>(gx)*KALMAN_TORQUE_SCALING,
        dt_seconds
    ));

    return direction;
}

bool process::FallDetector::Events::is_free_fall(void) const
{
    return this->free_fall.has_value() && this->free_fall->is_free_fall;
}
bool process::FallDetector::Events::is_impact(void) const
{
    return this->impact.has_value();
}
bool process::FallDetector::Events::is_tilt_change(void) const
{
    return this->tilt_change.has_value() && this->tilt_change->is_tilt_change;
}
bool process::FallDetector::Events::is_monitoring_post_fall(void) const
{
    return this->post_fall.has_value();
}