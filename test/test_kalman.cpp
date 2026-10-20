#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"

#include <dsp/kalman.hpp>

#include <cmath>

TEST_GROUP(KalmanFilter)
{
};

/*
Constructor initializes tuning parameters
*/
TEST(KalmanFilter, DefaultParameters)
{
    Kalman k;

    DOUBLES_EQUAL(0.001, k.getQangle(), 1e-12);
    DOUBLES_EQUAL(0.003, k.getQbias(), 1e-12);
    DOUBLES_EQUAL(0.03,  k.getRmeasure(), 1e-12);
}

/*
Setters / getters
*/
TEST(KalmanFilter, ParameterSetters)
{
    Kalman k;

    k.setQangle(0.1);
    k.setQbias(0.2);
    k.setRmeasure(0.3);

    DOUBLES_EQUAL(0.1, k.getQangle(), 1e-12);
    DOUBLES_EQUAL(0.2, k.getQbias(), 1e-12);
    DOUBLES_EQUAL(0.3, k.getRmeasure(), 1e-12);
}

/*
Initial angle is preserved
*/

TEST(KalmanFilter, SetAngleInitializesState)
{
    Kalman k;

    k.setAngle(45.0);

    double angle = k.getAngle(
        45.0,  // accel
        0.0,   // gyro
        0.01); // dt

    DOUBLES_EQUAL(45.0, angle, 0.1);
}

/*
Stationary system remains stable, the filter should not drift when the system is stationary
*/
TEST(KalmanFilter, StationaryAngleRemainsStable)
{
    Kalman k;

    k.setAngle(30.0);

    double estimate = 0.0;

    for(int i = 0; i < 100; ++i)
    {
        estimate = k.getAngle(
            30.0,
            0.0,
            0.01);
    }

    DOUBLES_EQUAL(30.0, estimate, 0.05);
}

/*
Constant integration test, if a gyro reports 90 deg/s for 1 second, the estimated angle should be near 90 deg
*/
TEST(KalmanFilter, IntegratesAngularRate)
{
    Kalman k;

    k.setAngle(0.0);

    double estimate = 0.0;

    constexpr double dt = 0.01;
    constexpr double rate = 90.0;

    for(int i = 0; i < 100; ++i)
    {
        estimate = k.getAngle(
            estimate,
            rate,
            dt);
    }

    DOUBLES_EQUAL(90.0, estimate, 3.0);
}

/*
The filter should move toward the measurement even if started incorrectly
*/

TEST(KalmanFilter, ConvergesTowardMeasurement)
{
    Kalman k;

    k.setAngle(0.0);

    double estimate = 0.0;

    for(int i = 0; i < 500; ++i)
    {
        estimate = k.getAngle(
            45.0,
            0.0,
            0.01);
    }

    CHECK(estimate > 40.0);
    CHECK(estimate < 46.0);
}

/*
Rate reflects unbiased gyro rate
*/

TEST(KalmanFilter, ReturnsCurrentRate)
{
    Kalman k;

    k.setAngle(0.0);

    k.getAngle(
        0.0,
        123.0,
        0.01);

    DOUBLES_EQUAL(
        123.0,
        k.getRate(),
        1e-6);
}

/*
Handles zero timestep
*/

TEST(KalmanFilter, ZeroTimeStepDoesNotExplode)
{
    Kalman k;

    k.setAngle(10.0);

    double angle =
        k.getAngle(
            10.0,
            100.0,
            0.0);

    CHECK(std::isfinite(angle));
}

/*
Noisy measurements are smoothed

IMPORTANT: This is the most important test, the whole point of using kalman is to smooth noise
*/

TEST(KalmanFilter, SmoothsNoisyMeasurements)
{
    Kalman k;

    k.setAngle(20.0);

    const double samples[] =
    {
        19.5,
        20.7,
        18.9,
        20.4,
        21.1,
        19.8,
        20.2
    };

    double estimate = 20.0;

    for(double sample : samples)
    {
        estimate =
            k.getAngle(
                sample,
                0.0,
                0.01);
    }

    CHECK(estimate > 19.0);
    CHECK(estimate < 21.0);
}