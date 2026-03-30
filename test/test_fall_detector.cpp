#include "CppUTest/TestHarness.h"

TEST_GROUP(FallDetector)
{
    
};

void test_triggers(
    std::string log,
    bool want_free_fall_event,
    bool want_impact_event,
    bool want_tilt_change_event,
    bool want_in_post_fall_monitor,
    bool want_fall_detected
)
{
    
}

/// Runs the fall-detection algorithm on a long list of recorded test-cases and verifies that the right triggers are activated for each test-case.
TEST(FallDetector, VerifyTriggers)
{
    
}