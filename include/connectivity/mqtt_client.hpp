// smartguard_mqtt_client.cpp
// Refactored into OOP by encapsulating lwIP MQTT client and callbacks
// Compatible with Raspberry Pi Pico W (CYW43) + lwIP MQTT

#include <cstring>
#include <cstdio>
#include <cmath>

#include "pico/unique_id.h"
#include "pico/cyw43_arch.h"

#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"   
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

#if LWIP_ALTCP && LWIP_ALTCP_TLS
#include "mbedtls/ssl.h"
#endif

#ifndef MQTT_SERVER
#error Need to define MQTT_SERVER
#endif

#ifdef MQTT_CERT_INC
#include MQTT_CERT_INC
#endif

#ifndef MQTT_TOPIC_LEN
#define MQTT_TOPIC_LEN 100
#endif

#ifndef MQTT_OUTPUT_RINGBUF_SIZE
#define MQTT_OUTPUT_RINGBUF_SIZE 1024
#endif

#ifndef MQTT_KEEP_ALIVE_S
#define MQTT_KEEP_ALIVE_S 60
#endif

#ifndef MQTT_SUBSCRIBE_QOS
#define MQTT_SUBSCRIBE_QOS 1
#endif
#ifndef MQTT_PUBLISH_QOS
#define MQTT_PUBLISH_QOS 1
#endif
#ifndef MQTT_PUBLISH_RETAIN
#define MQTT_PUBLISH_RETAIN 0
#endif

#ifndef MQTT_DEVICE_NAME
#define MQTT_DEVICE_NAME "smartguard"
#endif

#ifndef MQTT_UNIQUE_TOPIC
#define MQTT_UNIQUE_TOPIC 0
#endif

// LWT
#define MQTT_WILL_TOPIC "/online"
#define MQTT_WILL_MSG   "0"
#define MQTT_WILL_QOS   2

// Our data topic base
#define MQTT_BASE_TOPIC "/smartguard"
#define MQTT_DATA_TOPIC MQTT_BASE_TOPIC "/data"

class MqttClient {
public:
    MqttClient();

    ~MqttClient();

    void setAuth(const char* user, const char* pass);
    bool resolveAndConnect(const char* hostname = MQTT_SERVER);
    bool publish(const char* name_or_full_topic,
                 const char* payload,
                 u8_t qos = MQTT_PUBLISH_QOS,
                 u8_t retain = MQTT_PUBLISH_RETAIN,
                 bool treat_as_basic_topic = true);
    bool publishPayload(const char* payload);
    bool subscribe(const char* name, u8_t qos = MQTT_SUBSCRIBE_QOS);
    bool waitUntilConnected(uint32_t timeout_ms) const;
    void stop();

private:
    static void s_pub_request_cb(void* arg, err_t err);
    static void s_sub_request_cb(void* arg, err_t err);
    static void s_dns_found(const char* hostname, const ip_addr_t* ipaddr, void* arg);
    static void s_connection_cb(mqtt_client_t* client, void* arg, mqtt_connection_status_t status);
    static void s_incoming_publish_cb(void* arg, const char* topic, u32_t tot_len);
    static void s_incoming_data_cb(void* arg, const u8_t* data, u16_t len, u8_t flags);

    void handle_dns_found(const ip_addr_t* ipaddr);
    void handle_connection_status(mqtt_connection_status_t status);
    void handle_incoming_publish(const char* topic);
    void handle_incoming_data(const u8_t* data, u16_t len);

    void start_client();
    void build_full_topic(const char* name, char* out, std::size_t out_len) const;

private:
    mqtt_client_t* client_;
    mqtt_connect_client_info_t info_;
    ip_addr_t server_addr_;
    bool connect_done_;
    bool stop_client_;

    // Buffers
    char client_id_[64];
    char will_topic_[MQTT_TOPIC_LEN];
    char topic_[MQTT_TOPIC_LEN];
    char data_[MQTT_OUTPUT_RINGBUF_SIZE];
};
