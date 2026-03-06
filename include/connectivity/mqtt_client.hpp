// smartguard_mqtt_client.cpp
// Refactored into OOP by encapsulating lwIP MQTT client and callbacks
// Compatible with Raspberry Pi Pico W (CYW43) + lwIP MQTT

#include <cstring>
#include <string>
#include <cstdio>
#include <cmath>
#include <algorithm>

#include <data.hpp>

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

#define MQTT_OUTPUT_RINGBUF_SIZE 300

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
#define MQTT_DATA_TOPIC "/data"

class MqttClient {
public:
    MqttClient();
    ~MqttClient();

    // Set the authorization information used for the client to connect with the broker
    // this is used in "start_client" when doing the connection
    void setAuth(const char* user, const char* pass);
    // Resolve a hostname into an IP address and try to connect by using "start_client"
    // if the hostname resolvs immediately, it imediately calls the "start_client"
    // otherwise it waits and the "start_client" is called in the callback
    bool resolveAndConnect(const char* hostname);

    // publishes a message to a specific topic, it checks if the client is still connected,
    // than handles topic and payload to be added to the "mqtt_publish" function
    bool publish(std::string_view topic,
                 std::string_view payload,
                 u8_t qos,
                 u8_t retain,
                 bool treat_as_basic_topic);

    // converts the packet to json (please check if I did it correctly)
    // then publishes it
    bool publishPayload(const PacketData& packet);
    // simple ping, just publishes a "helloworld"
    void publish_ping();

    // start the subscription of a topic, we will use it when receiving info
    // from the broker / GUI
    bool subscribe(std::string_view topic,
                   u8_t qos = MQTT_SUBSCRIBE_QOS);

    
    bool waitUntilConnected(uint32_t timeout_ms) const;
    void stop();

private:
    static void s_pub_request_cb(void* arg, err_t err); // callback for connection with broker (every data we send)
    static void s_sub_request_cb(void* arg, err_t err); // callback for connection from the broker (every data we receive)
    static void s_dns_found(const char* hostname, const ip_addr_t* ipaddr, void* arg); // callback for resolved hostname, it checks if the IP is valid
                                                                                       // and call "start_client" 
    static void s_connection_cb(mqtt_client_t* client, void* arg, mqtt_connection_status_t status); // callback for connection attempt
    static void s_incoming_publish_cb(void* arg, const char* topic, u32_t tot_len); // callback for handle when we publish data
    static void s_incoming_data_cb(void* arg, const u8_t* data, u16_t len, u8_t flags); // callback for handle every fragment of payload that arrives

    void handle_dns_found(const ip_addr_t* ipaddr);
    void handle_connection_status(mqtt_connection_status_t status);
    void handle_incoming_publish(const char* topic);
    void handle_incoming_data(const u8_t* data, u16_t len);

    void start_client();

    void build_full_topic(std::string_view name,
                          std::array<char, MQTT_TOPIC_LEN>& out) const;

private:
    mqtt_client_t* client_;
    mqtt_connect_client_info_t info_;
    ip_addr_t server_addr_;
    bool connect_done_;
    bool stop_client_;

    std::array<char, 64> client_id_;
    std::array<char, MQTT_TOPIC_LEN> will_topic_;
    std::array<char, MQTT_TOPIC_LEN> topic_;
    std::array<char, MQTT_OUTPUT_RINGBUF_SIZE> data_;
};