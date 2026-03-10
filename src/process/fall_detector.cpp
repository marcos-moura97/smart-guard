#include "connectivity/send.hpp"
#include "device/imu/mpu6050.hpp"
#include <cstdio>
#include <data.hpp>
#include <delta_time.hpp>
#include <cassert>
#include <chrono>
#include <cmath>
#include <hardware/gpio.h>
#include <numbers>
#include <optional>
#include <pico/time.h>
#include <pico/types.h>
#include <process/fall_detector.hpp>

FallDetector::FallDetector(GpioButton button, GpioEmergencyLed emergency_led, Mpu6050 imu, std::shared_ptr<Send<PacketData>> client):
    _button(std::move(button)),
    _emergency_led(std::move(emergency_led)),
    _imu(std::move(imu)),
    _client(std::move(client)),
    _delta_time()
{
    
}

Process::Update FallDetector::update(void)
{
    const SensorData sensor_data {this->measure()};
    
    this->_delta_time.tick();

    this->analyze(sensor_data);

    const PacketData packet_data {this->collect(sensor_data)};
    this->_client->send(packet_data);

    const std::optional<bool> evaluation {this->evaluate(packet_data)};
    if(evaluation.has_value()) // An evaluation has been made
    {
        // Was it a fall?
        if(*evaluation)
        {
            //printf("FINAL FALL DETECTED (STEP 1)\n");
            this->_fall_detected = true;
            this->_emergency_led.write(false);
        }
        else
        {
            //printf("Post-fall rejected with mag_g = %.3f and \n",mag_g);
        }

        // Reset state
        this->_events = {
            .free_fall {std::nullopt},
            .impact {std::nullopt},
            .post_fall {std::nullopt}
        };
    }

    // alert button handle check
    if(this->_fall_detected)
    {
        if(!this->_button.read())
        {
            this->_emergency_led.write(false);
            this->_fall_detected = false;
        }
        else
        {
            this->_emergency_led.write(true);
        }
    }

    return Update::pending;
}

SensorData FallDetector::measure(void)
{
    // Read from IMU
    ImuData imu_dat {this->_imu.measure()};

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
void FallDetector::analyze(const SensorData& sensor_data)
{
    const auto [ax_g, ay_g, az_g] = sensor_data.imu.accel;
    const auto [gx, gy, gz] = sensor_data.imu.gyro;
    const float mag_g {
        std::sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g)
    };

    // 3) FREE-FALL detection
    if (!(mag_g < 0.4f))
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
        this->_events.free_fall->elapsed += this->_delta_time.dt();
        if(!this->_events.free_fall->is_free_fall && this->_events.free_fall->elapsed >= this->FREE_FALL_TRIGGER_TIME)
        {
            this->_events.free_fall->is_free_fall = true;
            std::printf("Event: FREE FALL\n");
        }
    }

    // 4) IMPACT detection
    const bool is_lying_now {std::abs(ax_g) < IS_LYING_THRESHOLD && std::abs(az_g) > IS_LYING_THRESHOLD};
    if(this->_events.impact.has_value())
    {
        this->_events.impact->elapsed += this->_delta_time.dt();
    }
    else if(mag_g > this->IMPACT_THRESHOLD)
    {
        this->_events.impact = {
            .elapsed {microseconds_t::zero()}
        };
        this->_events.tilt_change = {
            .elapsed {microseconds_t::zero()},
            .was_lying_at_impact = is_lying_now,
            .is_tilt_change = this->_events.tilt_change.has_value() && this->_events.tilt_change->is_tilt_change
        };
        std::printf("Event: IMPACT  mag=%.2f\n", mag_g);
    }

    // 5) POSTURE CHANGE within 500ms
    if(this->_events.tilt_change.has_value())
    {
        this->_events.tilt_change->elapsed += this->_delta_time.dt();
        if(this->_events.tilt_change->elapsed <= this->TILT_WINDOW_TIME)
        {
            if(!this->_events.tilt_change->is_tilt_change && (is_lying_now != this->_events.tilt_change->was_lying_at_impact)) // TODO: Double check this logic
            {
                this->_events.tilt_change->is_tilt_change = true;
                std::printf("Event: POSTURE CHANGE\n");
            }
            else
            {
                this->_events.tilt_change = std::nullopt;
            }
        }
    }

    // 6) ENTER POST-FALL MONITORING (2 of 3 events + lying)
    if (this->_events.post_fall.has_value())
    {
        this->_events.post_fall->elapsed += this->_delta_time.dt();
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
std::optional<bool> FallDetector::evaluate(const PacketData& packet_data) const
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
PacketData FallDetector::collect(const SensorData& sensor_data)
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

    const Direction direction_filtered {this->filter_direction(direction, sensor_data.imu)};

    return PacketData {
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
Direction FallDetector::filter_direction(Direction direction, const ImuData& imu_data)
{
    static constexpr double KALMAN_TORQUE_SCALING {131.0}; // What does this number correspond to?

    // Set initial value to avoid transient
    if(!this->_is_kalman_calibrated) {
        this->_kalman_roll.setAngle(static_cast<double>(direction.roll));
        this->_kalman_pitch.setAngle(static_cast<double>(direction.pitch));
        this->_is_kalman_calibrated = true;
    }

    const auto [gx, gy, gz] = imu_data.gyro;

    /*const*/ double dt_seconds {std::chrono::duration_cast<std::chrono::duration<double>>(this->_delta_time.dt()).count()};

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

bool FallDetector::Events::is_free_fall(void) const
{
    return this->free_fall.has_value() && this->free_fall->is_free_fall;
}
bool FallDetector::Events::is_impact(void) const
{
    return this->impact.has_value();
}
bool FallDetector::Events::is_tilt_change(void) const
{
    return this->tilt_change.has_value() && this->tilt_change->is_tilt_change;
}
bool FallDetector::Events::is_monitoring_post_fall(void) const
{
    return this->post_fall.has_value();
}