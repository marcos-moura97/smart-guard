#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "connectivity/mqtt_client.hpp"

#define private public
#include "connectivity/mqtt_client.hpp"
#undef private

extern "C"
{
#include "lwip/apps/mqtt.h"
#include "lwip/err.h"
}

static mqtt_client_t g_client;
static bool g_connected = true;

/*
Mocks
*/
extern "C"
{

mqtt_client_t* mqtt_client_new(void)
{
    mock().actualCall("mqtt_client_new");
    return &g_client;
}

u8_t mqtt_client_is_connected(mqtt_client_t*)
{
    mock()
        .actualCall("mqtt_client_is_connected");

    return g_connected;
}

err_t mqtt_publish(
    mqtt_client_t* client,
    const char* topic,
    const void* payload,
    u16_t payload_length,
    u8_t qos,
    u8_t retain,
    mqtt_request_cb_t cb,
    void* arg)
{
    return mock()
        .actualCall("mqtt_publish")
        .withStringParameter("topic", topic)
        .withMemoryBufferParameter(
            "payload",
            static_cast<const unsigned char*>(payload),
            payload_length)
        .returnIntValue();
}

err_t mqtt_subscribe(
    mqtt_client_t* client,
    const char* topic,
    u8_t qos,
    mqtt_request_cb_t cb,
    void* arg)
{
    return mock()
        .actualCall("mqtt_subscribe")
        .withStringParameter("topic", topic)
        .withUnsignedIntParameter("qos", qos)
        .returnIntValue();
}

void mqtt_set_inpub_callback(
    mqtt_client_t*,
    mqtt_incoming_publish_cb_t,
    mqtt_incoming_data_cb_t,
    void*)
{
}

void cyw43_arch_lwip_begin(void)
{
}

void cyw43_arch_lwip_end(void)
{
}

}

/*
Fixtures

*/
TEST_GROUP(MqttClientTests)
{
    void setup() override
    {
        g_connected = true;
    }

    void teardown() override
    {
        mock().clear();
    }
};

/*
Publish fails when client is null

*/
TEST(MqttClientTests, PublishReturnsFalseWithoutClient)
{
    MqttClient mqtt;

    bool result =
        mqtt.publish(
            "/test",
            "hello",
            0,
            0,
            true);

    CHECK_FALSE(result);
}

/*
Publish fails when disconnected

*/

TEST(MqttClientTests, PublishReturnsFalseWhenDisconnected)
{
    MqttClient mqtt;

    mqtt._client = &g_client;

    g_connected = false;

    mock()
        .expectOneCall("mqtt_client_is_connected");

    bool result =
        mqtt.publish(
            "/test",
            "hello",
            0,
            0,
            true);

    CHECK_FALSE(result);
}

/*
Publish success

*/
TEST(MqttClientTests, PublishReturnsTrueWhenBrokerAcceptsMessage)
{
    MqttClient mqtt;

    mqtt._client = &g_client;

    mock()
        .expectOneCall("mqtt_client_is_connected");

    mock()
        .expectOneCall("mqtt_publish")
        .withStringParameter("topic", "/test")
        .withMemoryBufferParameter(
            "payload",
            reinterpret_cast<const unsigned char*>("hello"),
            5)
        .andReturnValue(ERR_OK);

    bool result =
        mqtt.publish(
            "/test",
            "hello",
            0,
            0,
            false);

    CHECK_TRUE(result);

    mock().checkExpectations();
}

/*
Publish failure from MQTT stack
*/

TEST(MqttClientTests, PublishReturnsFalseWhenMqttPublishFails)
{
    MqttClient mqtt;

    mqtt._client = &g_client;

    mock()
        .expectOneCall("mqtt_client_is_connected");

    mock()
        .expectOneCall("mqtt_publish")
        .andReturnValue(ERR_CONN);

    bool result =
        mqtt.publish(
            "/test",
            "hello",
            0,
            0,
            false);

    CHECK_FALSE(result);
}

/*
Subscribe Success
*/

TEST(MqttClientTests, SubscribeReturnsTrue)
{
    MqttClient mqtt;

    mqtt._client = &g_client;

    mock()
        .expectOneCall("mqtt_client_is_connected");

    mock()
        .expectOneCall("mqtt_subscribe")
        .withStringParameter("topic", "/ping")
        .withUnsignedIntParameter("qos", 1)
        .andReturnValue(ERR_OK);

    CHECK_TRUE(mqtt.subscribe("/ping"));

    mock().checkExpectations();
}

/*
Subscribe fails when disconnected
*/
TEST(MqttClientTests, SubscribeReturnsFalseWhenDisconnected)
{
    MqttClient mqtt;

    mqtt._client = &g_client;
    g_connected = false;

    mock()
        .expectOneCall("mqtt_client_is_connected");

    CHECK_FALSE(mqtt.subscribe("/ping"));
}

/*
Ensure stop function clears client
*/
TEST(MqttClientTests, StopClearsClient)
{
    MqttClient mqtt;

    mqtt._client = &g_client;

    mqtt.stop();

    POINTERS_EQUAL(nullptr, mqtt._client);
}

/*
Ensuring PingData sends expected payload
*/
TEST(MqttClientTests, SendPingPublishesHelloWorld)
{
    MqttClient mqtt;

    mqtt._client = &g_client;

    mock()
        .expectOneCall("mqtt_client_is_connected");

    mock()
        .expectOneCall("mqtt_publish")
        .withStringParameter("topic", "/ping")
        .withMemoryBufferParameter(
            "payload",
            reinterpret_cast<const unsigned char*>("helloworld"),
            10)
        .andReturnValue(ERR_OK);

    CHECK_TRUE(mqtt.send(PingData{}));

    mock().checkExpectations();
}