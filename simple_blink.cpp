#include <cstring>
#include <cstdio>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <Kalman.h>


class FallDetector {
public:
    FallDetector()
    : addr(0x68),
      accel_offset_x(-0.3f), accel_offset_y(-0.2f), accel_offset_z(-0.9f),
      gyro_offset_x(-3.5f), gyro_offset_y(0.0f), gyro_offset_z(-1.5f),
      first(true),
      lastSampleTime(0), lastBlinkTime(0), blink_status(false),
      freeFallActive(false), freeFallEvent(false), freeFallStartTime(0),
      impactEvent(false), impactTime(0),
      tiltChangeEvent(false), monitorTiltChange(false), tiltWindowStart(0), wasLyingAtImpact(false),
      inPostFallMonitor(false), postFallStartTime(0), postMinMag(1000.0f), postMaxMag(0.0f),
      alarmActive(false),
      roll_gyro_rate_deg_per_sec(0.0f), pitch_gyro_rate_deg_per_sec(0.0f),
      angleX(0.0f), angleY(0.0f)      
    {
        
    }

    void setup() {
        stdio_init_all();

        // LEDs
        gpio_init(PICO_DEFAULT_LED_PIN);
        gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

        gpio_init(LED_GPIO);
        gpio_set_dir(LED_GPIO, GPIO_OUT);

        // Button
        gpio_init(BUTTON_GPIO);
        gpio_set_dir(BUTTON_GPIO, GPIO_IN);
        gpio_pull_up(BUTTON_GPIO);

        // I2C / MPU6050
        mpu6050_setup();

        printf("Setup done! Starting reading.\n");
        printf("ax_g,ay_g,az_g,pitch,roll,acc_smv,gyro_smv,mag_g,lying\n");
    }

    
    void update() {
        int16_t acceleration[3], gyro[3], temp;
        static int16_t old_accel[3] = {0,0,0};
        static int16_t old_gyro[3]  = {0,0,0};
        int16_t filtered_acceleration[3], filtered_gyro[3];

        // Read sensors
        mpu6050_read_raw(acceleration, gyro, &temp);

        // Simple low-pass on raw LSB values
        for (int i = 0; i < 3; ++i) {
            filtered_acceleration[i] = (int16_t)applyLowPassFilter((float)old_accel[i], (float)acceleration[i]);
            filtered_gyro[i] = (int16_t)applyLowPassFilter((float)old_gyro[i], (float)gyro[i]);
        }

        memcpy(old_accel, acceleration, sizeof(old_accel));
        memcpy(old_gyro, gyro, sizeof(old_gyro));

        // Pre-process thresholds, orientation, Kalman updates and event detection
        threshold_mpu6050_data(filtered_acceleration, filtered_gyro);

        // Fall decision step (post-fall monitor)
        fall_detection(filtered_acceleration, filtered_gyro);

        // check alert button to clear alerts 
        alert_button_handle_check();

        // Simple blink throttling (50 ms)
        delay_with_blink(50);
    }

private:
    // GPIO / I2C constants
    static constexpr uint LED_GPIO = 19;       // stop button
    static constexpr uint BUTTON_GPIO = 9;     // Emergency LED

    // MPU6050 I2C address
    const int addr;

    // Earth constants
    static constexpr float SENSORS_GRAVITY_EARTH = 9.80665F;
    static constexpr float SENSORS_DPS_TO_RADS = 0.017453293F;

    float old_mag_g[5] = {};
    // Sensor offsets
    float accel_offset_x, accel_offset_y, accel_offset_z;
    float gyro_offset_x, gyro_offset_y, gyro_offset_z;

    // state flags & timers
    bool first;
    unsigned long lastSampleTime;
    unsigned long lastBlinkTime;
    bool blink_status;

    // fall detection flags
    bool freeFallActive;
    bool freeFallEvent;
    unsigned long freeFallStartTime;

    bool impactEvent;
    unsigned long impactTime;

    bool tiltChangeEvent;
    bool monitorTiltChange;
    unsigned long tiltWindowStart;
    bool wasLyingAtImpact;

    bool inPostFallMonitor;
    unsigned long postFallStartTime;
    float postMinMag;
    float postMaxMag;

    bool alarmActive;

    // Kalman filters (members)
    Kalman kalmanX;
    Kalman kalmanY;
    Kalman kalman_roll;
    Kalman kalman_pitch;
    float current_pitch;
    float current_roll;

    float roll_gyro_rate_deg_per_sec;
    float pitch_gyro_rate_deg_per_sec;

    float angleX, angleY;

    // CNN stuff

    uint8_t tensor_arena[40*1024];



    // ---------- MPU6050 helpers ----------
    void mpu6050_reset() {
#ifdef i2c_default
        uint8_t buf[] = {0x6B, 0x80};
        i2c_write_blocking(i2c_default, addr, buf, 2, false);
        sleep_ms(100);
        buf[1] = 0x00;
        i2c_write_blocking(i2c_default, addr, buf, 2, false);
        sleep_ms(10);
#endif
    }

    void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
#ifdef i2c_default
        uint8_t buffer[6];

        // Accel registers 0x3B..0x40 (we read 6 bytes)
        uint8_t reg = 0x3B;
        i2c_write_blocking(i2c_default, addr, &reg, 1, true);
        i2c_read_blocking(i2c_default, addr, buffer, 6, false);

        for (int i = 0; i < 3; ++i) {
            accel[i] = (int16_t)((buffer[i*2] << 8) | buffer[i*2 + 1]);
        }

        // Gyro registers 0x43..0x48
        reg = 0x43;
        i2c_write_blocking(i2c_default, addr, &reg, 1, true);
        i2c_read_blocking(i2c_default, addr, buffer, 6, false);

        for (int i = 0; i < 3; ++i) {
            gyro[i] = (int16_t)((buffer[i*2] << 8) | buffer[i*2 + 1]);
        }

        // Temp registers 0x41..0x42
        reg = 0x41;
        i2c_write_blocking(i2c_default, addr, &reg, 1, true);
        i2c_read_blocking(i2c_default, addr, buffer, 2, false);
        *temp = (int16_t)((buffer[0] << 8) | buffer[1]);
#else
        // If i2c_default isn't defined on this build, produce zeros (keeps compilation possible)
        accel[0] = accel[1] = accel[2] = 0;
        gyro[0] = gyro[1] = gyro[2] = 0;
        *temp = 0;
#endif
    }

    void mpu6050_setup() {
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
        #warning i2c/mpu6050_i2c example requires a board with I2C pins
        puts("Default I2C pins were not defined");
#else
        printf("Hello, MPU6050! Reading raw data from registers...\n");
        i2c_init(i2c_default, 400 * 1000);
        gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
        gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
        gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
        gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
        bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
        mpu6050_reset();
#endif
    }

    // ---------- Filters & Kalman ----------
    static float convert_to_deg_per_sec(float a) {
        return a / 131.0f;
    }

    static float applyLowPassFilter(float previousValue, float newReading) {
        return (previousValue * 0.9f) + (newReading * 0.1f);
    }

    static float complementaryFilter(float gyroAngle, float accelAngle) {
        return (gyroAngle * 0.98f) + (accelAngle * 0.02f);
    }

    void updateKalman(float accelRollDeg, float accelPitchDeg, float gyroRawX, float gyroRawY, float dt_seconds) {
        float gyroRateX_deg_s = convert_to_deg_per_sec(gyroRawX);
        float gyroRateY_deg_s = convert_to_deg_per_sec(gyroRawY);

        angleX = kalman_roll.getAngle(accelRollDeg, gyroRateX_deg_s, dt_seconds);
        angleY = kalman_pitch.getAngle(accelPitchDeg, gyroRateY_deg_s, dt_seconds);
    }

    void threshold_mpu6050_data(int16_t accel[3], int16_t gyro[3]) {
        // Convert raw accelerometer to g (corrected by offsets)
        float ax_g = ((accel[0] / 16384.0f) * SENSORS_GRAVITY_EARTH - accel_offset_x) / SENSORS_GRAVITY_EARTH;
        float ay_g = ((accel[1] / 16384.0f) * SENSORS_GRAVITY_EARTH - accel_offset_y) / SENSORS_GRAVITY_EARTH;
        float az_g = ((accel[2] / 16384.0f) * SENSORS_GRAVITY_EARTH - accel_offset_z) / SENSORS_GRAVITY_EARTH;

        float gx = gyro[0] - gyro_offset_x;
        float gy = gyro[1] - gyro_offset_y;
        float gz = gyro[2] - gyro_offset_z;

        float mag_g = sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);

        float absAx = fabsf(ax_g);
        float absAz = fabsf(az_g);
        bool isLyingNow = (absAz < 0.5f && absAx > 0.5f);

        uint32_t now = to_ms_since_boot(get_absolute_time());

        // sampling at ~50 Hz
        if (now - lastSampleTime < 20) {
            return;
        }
        float dt = (now - lastSampleTime) / 1000.0f;
        if (dt <= 0.0f || dt > 0.1f) dt = 0.02f;
        lastSampleTime = now;

        // 3) FREE-FALL detection
        if (mag_g < 0.4f) {
            if (!freeFallActive) {
                freeFallActive = true;
                freeFallStartTime = now;
            } else {
                if (!freeFallEvent && (now - freeFallStartTime >= 200)) {
                    freeFallEvent = true;
                    //printf("Event: FREE FALL\n");
                }
            }
        } else {
            freeFallActive = false;
        }

        // 4) IMPACT detection
        float impactThreshold = 2.5f;
        if (!impactEvent && mag_g > impactThreshold) {
            impactEvent = true;
            impactTime = now;
            //printf("Event: IMPACT  mag=%.2f\n", mag_g);

            monitorTiltChange = true;
            tiltWindowStart = now;
            wasLyingAtImpact = isLyingNow;
        }

        // 5) POSTURE CHANGE within 500ms
        if (monitorTiltChange) {
            if (now - tiltWindowStart <= 500) {
                if (!tiltChangeEvent && (isLyingNow != wasLyingAtImpact)) {
                    tiltChangeEvent = true;
                    //printf("Event: POSTURE CHANGE\n");
                }
            } else {
                monitorTiltChange = false;
            }
        }

        // 6) ENTER POST-FALL MONITORING (2 of 3 events + lying)
        if (!inPostFallMonitor) {
            int eventCount = 0;
            if (freeFallEvent) eventCount++;
            if (impactEvent) eventCount++;
            if (tiltChangeEvent && isLyingNow) eventCount++;

            if (eventCount >= 2 && isLyingNow) {
                inPostFallMonitor = true;
                postFallStartTime = now;
                //printf("Entering post-fall monitoring...\n");
            }
        }

        // store stats
        postMinMag = fminf(postMinMag, mag_g);
        postMaxMag = fmaxf(postMaxMag, mag_g);
        
        float acc_smv  = sqrtf((float)accel[0]*accel[0] + (float)accel[1]*accel[1] + (float)accel[2]*accel[2]);
        float gyro_smv = sqrtf((float)gyro[0]*gyro[0] + (float)gyro[1]*gyro[1] + (float)gyro[2]*gyro[2]);

        // orientation from accelerometer (degrees)
        float pitch = atan2f(-accel[0], sqrtf((float)accel[1]*accel[1] + (float)accel[2]*accel[2])) * 180.0f / M_PI;
        float roll  = atan2f((float)accel[1], (float)accel[2]) * 180.0f / M_PI;

        if (first) {
            kalman_roll.setAngle(roll);
            kalman_pitch.setAngle(pitch);
            first = false;
        }

        updateKalman(roll, pitch, gx, gy, dt);

        // print similar CSV as original
        printf("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d,%d\n",
            ax_g,
            ay_g,
            az_g,
            pitch,
            roll,
            angleY, // pitch
            angleX,  // roll
            freeFallEvent,
            impactEvent,
            tiltChangeEvent,
            inPostFallMonitor,
            fall_detected
        );
        current_pitch = pitch;
        current_roll = roll;
    }

    void fall_detection(int16_t accel[3], int16_t gyro[3]) {
        if (!inPostFallMonitor) return;

        uint32_t now = to_ms_since_boot(get_absolute_time());

        float ax_g = ((accel[0] / 16384.0f) * SENSORS_GRAVITY_EARTH - accel_offset_x) / SENSORS_GRAVITY_EARTH;
        float ay_g = ((accel[1] / 16384.0f) * SENSORS_GRAVITY_EARTH - accel_offset_y) / SENSORS_GRAVITY_EARTH;
        float az_g = ((accel[2] / 16384.0f) * SENSORS_GRAVITY_EARTH - accel_offset_z) / SENSORS_GRAVITY_EARTH;

        float mag_g = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
        bool isLyingNow = (fabsf(current_roll) < 20.0 || fabsf(current_roll) > 150 || fabsf(current_pitch) > 70.0);//(fabsf(az_g) < 0.5f && fabsf(ax_g) > 0.5f);

        if (now - postFallStartTime >= 3000) {
            bool near1g = (mag_g >= 0.75f && mag_g <= 1.3f);

            if (near1g && isLyingNow) {
                //printf("FINAL FALL DETECTED (STEP 1)\n");
                fall_detected = true;
                gpio_put(LED_GPIO, 0);
            } else {
                //printf("Post-fall rejected with mag_g = %.3f and \n",mag_g);
            }

            // Reset state
            freeFallEvent = false;
            impactEvent = false;
            tiltChangeEvent = false;
            monitorTiltChange = false;
            inPostFallMonitor = false;
        }
    }

    // ---------- UI helpers ----------
    void delay_with_blink(uint32_t ms) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - lastBlinkTime < ms) {
            return;
        }
        lastBlinkTime = now;
        blink_status = !blink_status;
        gpio_put(PICO_DEFAULT_LED_PIN, blink_status);
    }

    void alert_button_handle_check() {
        if (fall_detected) {
            if (!gpio_get(BUTTON_GPIO)) {
                gpio_put(LED_GPIO, 0);
                fall_detected = !fall_detected; // toggle to clear
            } else {
                gpio_put(LED_GPIO, 1);
            }
        }
    }

    // ---------- small helpers ----------
    double max_90_deg_correction(double rate, double kalman) {
        if (fabs(kalman) > 90) return -rate;
        else return rate;
    }

    // member to hold the public fall flag (used by alert_button_handle_check)
    bool fall_detected = false;
};

// ----------------- main -----------------
int main() {
    FallDetector detector;
    detector.setup();

    while (true) {
        detector.update();
    }

    return 0; 
}
