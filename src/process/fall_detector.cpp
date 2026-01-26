#include "data.hpp"
#include <cassert>
#include <chrono>
#include <cmath>
#include <exception>
#include <hardware/gpio.h>
#include <numbers>
#include <optional>
#include <pico/time.h>
#include <pico/types.h>
#include <ratio>
#include <stdexcept>
#include <process/fall_detector.hpp>

FallDetector::FallDetector(std::shared_ptr<Device> device):
    device(std::move(device)),
    time(get_absolute_time())
{
    
}

void FallDetector::update(void)
{
    const SensorData sensor_data {this->measure()};
    
    const duration_t dt = this->delta_time();

    this->analyze(sensor_data);

    const PacketData packet_data {this->collect(sensor_data, dt)};
    packet_data.post();

    const std::optional<bool> evaluation {this->evaluate(packet_data)};
    if(evaluation.has_value()) // An evaluation has been made
    {
        // Was it a fall?
        if(*evaluation)
        {
            //printf("FINAL FALL DETECTED (STEP 1)\n");
            this->fall_detected = true;
            gpio_put(LED_GPIO, 0);
        }
        else
        {
            //printf("Post-fall rejected with mag_g = %.3f and \n",mag_g);
        }

        // Reset state
        this->events = {
            .free_fall {std::nullopt},
            .impact {std::nullopt},
            .post_fall {std::nullopt}
        };
    }

    // alert button handle check
    if (this->fall_detected) {
        if (!gpio_get(BUTTON_GPIO)) {
            gpio_put(LED_GPIO, 0);
            this->fall_detected = false;
        } else {
            gpio_put(LED_GPIO, 1);
        }
    }

    // delay with blink
    // TODO: either remove it (if we don't need it) or encapsulate the logic + state into a seperate class.
    if (this->blink.elapsed >= BLINK_PERIOD)
    {
        this->blink.status = !blink.status;
        gpio_put(PICO_DEFAULT_LED_PIN, blink.status);
    }
    this->blink.elapsed += dt;
}

FallDetector::SensorData FallDetector::measure(void)
{
    // Read from IMU
    ImuData imu_dat {this->device->measure_imu()};

    // Low pass on floating point values
    // NOTE: Previous implementation did this on the raw LSB values, so this implementation may lead to different results.
    // TODO: Investigate: why was LSB used previously for the filtering stage? Maybe there was a good reason.
    for(std::size_t i {0}; i < 3; ++i)
    {
        imu_dat.accel[i] = this->filter_accel[i](imu_dat.accel[i]);
        imu_dat.gyro[i] = this->filter_gyro[i](imu_dat.gyro[i]);
    }

    return {
        .imu {std::move(imu_dat)}
    };
}

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
        this->events.free_fall = std::nullopt;
    }
    else if (!this->events.free_fall.has_value())
    {
        // Start free fall counter
        this->events.free_fall = {
            .elapsed = duration_t::zero(),
            .is_free_fall = false
        };
    }
    else
    {
        if(!this->events.free_fall->is_free_fall && this->events.free_fall->elapsed >= this->FREE_FALL_TRIGGER_TIME)
        {
            this->events.free_fall->is_free_fall = true;
            //printf("Event: FREE FALL\n");
        }
    }

    // 4) IMPACT detection
    //const bool is_lying_now {std::abs(ax_g) < IS_LYING_THRESHOLD && std::abs(az_g) > IS_LYING_THRESHOLD}; // rectangular (original)
    const bool is_lying_now {ax_g*ax_g + az_g*az_g > IS_LYING_THRESHOLD*IS_LYING_THRESHOLD}; // circular (not tested)
    if(!this->events.impact.has_value() && mag_g > IMPACT_THRESHOLD)
    {
        this->events.impact = {
            .elapsed = duration_t::zero()
        };
        this->events.tilt_change = {
            .elapsed = duration_t::zero(),
            .was_lying_at_impact = is_lying_now,
            .is_tilt_change = false
        };
        //printf("Event: IMPACT  mag=%.2f\n", mag_g);
    }

    // 5) POSTURE CHANGE within 500ms
    // NOTE: the logic here has changed slightly from the 2025 version. I noticed the impact event never gets unset. Maybe revert if it doesn't work...
    if(this->events.tilt_change.has_value())
    {
        if(this->events.tilt_change->elapsed <= TILT_WINDOW_TIME)
        {
            if(!this->events.tilt_change->is_tilt_change && (is_lying_now != this->events.tilt_change->was_lying_at_impact)) // TODO: Double check this logic
            {
                this->events.tilt_change->is_tilt_change = true;
                //printf("Event: POSTURE CHANGE\n");
            }
        }
        else
        {
            this->events.impact = std::nullopt;
        }
    }

    // 6) ENTER POST-FALL MONITORING (2 of 3 events + lying)
    if (!this->events.post_fall.has_value())
    {
        const int event_count = 
            static_cast<std::uint8_t>(this->events.is_free_fall())
            + static_cast<std::uint8_t>(this->events.is_impact())
            + static_cast<std::uint8_t>(this->events.is_tilt_change());
        if(event_count >= 2 && is_lying_now) {
            this->events.post_fall = {
                .elapsed {duration_t::zero()}
            };
            //printf("Entering post-fall monitoring...\n");
        }
    }
}

FallDetector::PacketData FallDetector::collect(const SensorData& sensor_data, const duration_t dt)
{
    const auto [ax_g, ay_g, az_g] = sensor_data.imu.accel;
    const auto [gx, gy, gz] = sensor_data.imu.gyro;
    const float mag_g {
        std::sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g)
    };

    // store stats
    this->post_min_mag = std::min(this->post_min_mag, mag_g);
    this->post_max_mag = std::max(this->post_max_mag, mag_g);

    static constexpr float DEG_PER_RAD {180.0f / std::numbers::pi_v<float>};

    // orientation from accelerometer (degrees)
    const Direction direction {
        .pitch = std::atan2(-ax_g, std::sqrt(ay_g*ay_g + az_g*az_g)) * DEG_PER_RAD,
        .roll = std::atan2(ay_g, az_g) * DEG_PER_RAD
    };

    const Direction direction_filtered {this->filter_direction(direction, sensor_data.imu, dt)};

    return PacketData {
        .sensor_data {sensor_data},
        .direction {direction},
        .direction_filtered {direction_filtered},
        
        .free_fall_event {this->events.is_free_fall()},
        .impact_event {this->events.is_impact()},
        .tilt_change_event {this->events.is_tilt_change()},
        .in_post_fall_monitor {this->events.is_monitoring_post_fall()},
        .fall_detected {this->fall_detected}
    };
}

std::optional<bool> FallDetector::evaluate(const PacketData& packet_data) const
{
    // 7) FALL DETECTION (TRIGGER WARNING)
    if (this->events.post_fall.has_value())
    {
        const auto [ax_g, ay_g, az_g] = packet_data.sensor_data.imu.accel;
        const auto [gx, gy, gz] = packet_data.sensor_data.imu.gyro;
        // Acceleration magnitude
        const float mag_g {
            std::sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g)
        };
        
        const bool is_lying_now = std::abs(packet_data.direction.roll) < 20.0f
            || std::abs(packet_data.direction.roll) > 150.0f
            || std::abs(packet_data.direction.pitch) > 70.0f; // rectangular (original #2)
            //(fabsf(az_g) < 0.5f && fabsf(ax_g) > 0.5f);

        if (this->events.post_fall->elapsed >= POST_FALL_TRIGGER_TIME)
        {
            const bool near1g = (mag_g >= 0.75f && mag_g <= 1.3f);

            return near1g && is_lying_now;
        }
    }
    return std::nullopt;
}

FallDetector::Direction FallDetector::filter_direction(Direction direction, const ImuData& imu_data, const duration_t dt)
{
    static constexpr double KALMAN_TORQUE_SCALING {131.0}; // What does this number correspond to?

    // Set initial value to avoid transient
    if(!this->is_kalman_calibrated) {
        kalman_roll.setAngle(static_cast<double>(direction.roll));
        kalman_pitch.setAngle(static_cast<double>(direction.pitch));
        this->is_kalman_calibrated = true;
    }

    const auto [gx, gy, gz] = imu_data.gyro;

    /*const*/ double dt_seconds {std::chrono::duration_cast<std::chrono::duration<double>>(dt).count()};

    // NOTE: I assume this helps with floating-point overflow/underflow, but wouldn't a std::clamp make more sense? Keeping it as is for now.
    if (dt_seconds <= 0.0 || dt_seconds > 0.1) dt_seconds = 0.02;

    direction.pitch = static_cast<float>(kalman_pitch.getAngle(
        static_cast<double>(direction.pitch),
        static_cast<double>(gy)*KALMAN_TORQUE_SCALING,
        dt_seconds
    ));
    direction.roll = static_cast<float>(kalman_roll.getAngle(
        static_cast<double>(direction.roll),
        static_cast<double>(gx)*KALMAN_TORQUE_SCALING,
        dt_seconds
    ));

    return direction;
}

FallDetector::duration_t FallDetector::delta_time(void)
{
    // TODO: handle infinite/invalid values (see documentation in pico/time.h)
    const absolute_time_t now {get_absolute_time()};
    const std::int64_t dt_us {absolute_time_diff_us(this->time, now)}; // WARNING: susceptable to numeric overflow
    this->time = now;
    const duration_t dt {dt_us};
    this->events.increment_time(dt);
    return dt;
}