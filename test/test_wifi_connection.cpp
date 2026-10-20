#include "CppUTest/TestHarness.h"
#include "CppUTest/UtestMacros.h"
#include "CppUTestExt/MockSupport.h"

#include "connectivity/wifi_wrapper.hpp"

extern "C"
{
    int cyw43_arch_wifi_connect_timeout_ms(
        const char* ssid,
        const char* pass,
        uint32_t auth,
        uint32_t timeout_ms);

    void cyw43_arch_enable_sta_mode();
    void cyw43_arch_deinit();
}

extern "C"
{

int cyw43_arch_wifi_connect_timeout_ms(
    const char* ssid,
    const char* pass,
    uint32_t auth,
    uint32_t timeout_ms)
{
    return mock()
        .actualCall("cyw43_arch_wifi_connect_timeout_ms")
        .withStringParameter("ssid", ssid)
        .withStringParameter("pass", pass)
        .withUnsignedIntParameter("auth", auth)
        .withUnsignedIntParameter("timeout_ms", timeout_ms)
        .returnIntValue();
}

void cyw43_arch_enable_sta_mode()
{
    mock().actualCall("cyw43_arch_enable_sta_mode");
}

void cyw43_arch_deinit()
{
    mock().actualCall("cyw43_arch_deinit");
}

} // extern "C"

TEST_GROUP(WifiWrapper)
{
    void teardown() override
    {
        mock().clear();
    }
};

/**
Init returs true

*/
TEST(WifiWrapper, InitReturnsTrue)
{
    WifiWrapper wifi;

    CHECK_TRUE(wifi.init());
}


/**
It won't break if we call it multiple times

*/
TEST(WifiWrapper, InitCanBeCalledMultipleTimes)
{
    WifiWrapper wifi;

    CHECK_TRUE(wifi.init());
    CHECK_TRUE(wifi.init());
}


/**
Enable STA mode

*/
TEST(WifiWrapper, EnableStaCallsDriver)
{
    WifiWrapper wifi;

    mock().expectOneCall("cyw43_arch_enable_sta_mode");

    wifi.enableSta();

    mock().checkExpectations();
}


/**
Connect succeed

IMPORTANT: change the ssid and pass with the correct data, otherwise it will break

*/
TEST(WifiWrapper, ConnectSucceedsOnFirstAttempt)
{
    WifiWrapper wifi;

    mock()
        .expectOneCall("cyw43_arch_wifi_connect_timeout_ms")
        .withStringParameter("ssid", "Autonomous")
        .withStringParameter("pass", "AutoSaiL1")
        .withUnsignedIntParameter("auth", CYW43_AUTH_WPA2_AES_PSK)
        .withUnsignedIntParameter("timeout_ms", 1000)
        .andReturnValue(0);

    bool result = wifi.connect(
        "Autonomous",
        "AutoSaiL1",
        1000);

    CHECK_TRUE(result);

    mock().checkExpectations();
}

/**
It will try to connect until it's successfull

IMPORTANT: change the ssid and pass with the correct data, otherwise it will break

*/
TEST(WifiWrapper, ConnectRetriesUntilSuccess)
{
    WifiWrapper wifi;

    mock()
        .expectOneCall("cyw43_arch_wifi_connect_timeout_ms")
        .andReturnValue(-1);

    mock()
        .expectOneCall("cyw43_arch_wifi_connect_timeout_ms")
        .andReturnValue(-1);

    mock()
        .expectOneCall("cyw43_arch_wifi_connect_timeout_ms")
        .andReturnValue(0);

    bool result = wifi.connect(
        "Autonomous",
        "AutoSaiL1",
        1000);

    CHECK_TRUE(result);

    mock().checkExpectations();
}

/**
After a certain number of tries, it will cail

IMPORTANT: use a wrong ssid/password

*/
TEST(WifiWrapper, ConnectFailsAfterAllRetries)
{
    WifiWrapper wifi;

    constexpr int TOTAL_ATTEMPTS = 10;

    for(int i = 0; i < TOTAL_ATTEMPTS; ++i)
    {
        mock()
            .expectOneCall("cyw43_arch_wifi_connect_timeout_ms")
            .andReturnValue(-1);
    }

    bool result = wifi.connect(
        "ssid",
        "password",
        1000);

    CHECK_FALSE(result);

    mock().checkExpectations();
}

/**
Ensuring lifecycle when the CYW43 library is deinitialized

*/
TEST(WifiWrapper, DestructorCallsDeinitWhenInitialized)
{
    mock().expectOneCall("cyw43_arch_deinit");

    {
        WifiWrapper wifi;
        wifi.init();
    }

    mock().checkExpectations();
}

/**
Ensuring proper destruction

*/
TEST(WifiWrapper, DestructorDoesNotCallDeinitWhenNotInitialized)
{
    {
        WifiWrapper wifi;
    }

    mock().checkExpectations();
}