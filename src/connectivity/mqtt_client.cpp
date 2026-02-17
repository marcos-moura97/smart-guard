#include "connectivity/mqtt_client.hpp"

MqttClient::MqttClient()
    : client_(nullptr),
      connect_done_(false),
      stop_client_(false)
{
    std::memset(&info_, 0, sizeof(info_));
    std::memset(&server_addr_, 0, sizeof(server_addr_));
    std::memset(topic_, 0, sizeof(topic_));
    std::memset(data_, 0, sizeof(data_));

    // Build a unique client_id from Pico's unique ID
    pico_unique_board_id_t bid;
    pico_get_unique_board_id(&bid);
    std::snprintf(client_id_, sizeof(client_id_), "%s-%02x%02x%02x%02x%02x%02x%02x%02x",
                    MQTT_DEVICE_NAME,
                    bid.id[0], bid.id[1], bid.id[2], bid.id[3],
                    bid.id[4], bid.id[5], bid.id[6], bid.id[7]);

    info_.client_id     = client_id_;
    info_.keep_alive    = MQTT_KEEP_ALIVE_S;
    info_.client_user   = nullptr; 
    info_.client_pass   = nullptr;

    
    build_full_topic(MQTT_WILL_TOPIC, will_topic_, sizeof(will_topic_));
    info_.will_topic    = will_topic_;
    info_.will_msg      = MQTT_WILL_MSG;
    info_.will_msg_len  = strlen(MQTT_WILL_MSG);
    info_.will_qos      = MQTT_WILL_QOS;
    info_.will_retain   = 1;
}

MqttClient::~MqttClient() {
    stop();
}

void MqttClient::setAuth(const char* user, const char* pass) {
    info_.client_user = user;
    info_.client_pass = pass;
}

bool MqttClient::resolveAndConnect(const char* hostname) {
    
    std::printf("[DEBUG] DNS server: %s\n", ipaddr_ntoa(dns_getserver(0)));
    std::printf("Resolving MQTT server '%s'...\n", hostname);
    
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(hostname,
                                &server_addr_,
                                &MqttClient::s_dns_found,
                                this);
    cyw43_arch_lwip_end();
    
    if (err == ERR_OK) {
        // IP available immediately
        start_client();
        return true;
    } else if (err == ERR_INPROGRESS) {
        // Will complete in s_dns_found
        return true;
    } else {
        std::printf("dns_gethostbyname failed: %d\n", err);
        return false;
    }
    while (server_addr_.addr == 0) {
        cyw43_arch_poll();
        sleep_ms(1);
    }
}

void MqttClient::publish_ping(){
    bool status = publish("/ping", "helloworld", 0, MQTT_PUBLISH_RETAIN, /*treat_as_basic_topic=*/true);
}

bool MqttClient::publish(const char* name_or_full_topic,
                const char* payload,
                u8_t qos,
                u8_t retain,
                bool treat_as_basic_topic)
{

    cyw43_arch_lwip_begin();
    bool ok = client_ && mqtt_client_is_connected(client_);
    cyw43_arch_lwip_end();
    if (!ok) return false;

    char topic_buf[MQTT_TOPIC_LEN];
    const char* topic_to_use = name_or_full_topic;

    if (treat_as_basic_topic) {
        build_full_topic(name_or_full_topic, topic_buf, sizeof(topic_buf));
        topic_to_use = topic_buf;
    }

    cyw43_arch_lwip_begin();
    err_t err = mqtt_publish(client_, topic_to_use,
                                payload, strlen(payload),
                                qos, retain,
                                &MqttClient::s_pub_request_cb, this);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        std::printf("mqtt_publish failed: %d\n", err);
        return false;
    }
    return true;
}

bool MqttClient::publishPayload(const PacketData& packet)
{
    std::string json;

    if (!Serializer::toJson(packet, json))
    {
        std::printf("JSON serialization failed\n");
        return false;
    }

    return publish(
        MQTT_DATA_TOPIC,
        json.c_str(),
        0,
        MQTT_PUBLISH_RETAIN,
        true
    );
}


bool MqttClient::subscribe(const char* name, u8_t qos) {
    cyw43_arch_lwip_begin();
    if (!client_ || !mqtt_client_is_connected(client_)) return false;
    cyw43_arch_lwip_end();
    char topic_buf[MQTT_TOPIC_LEN];
    build_full_topic(name, topic_buf, sizeof(topic_buf));
    cyw43_arch_lwip_begin();
    err_t err = mqtt_subscribe(client_, topic_buf, qos, &MqttClient::s_sub_request_cb, this);
    cyw43_arch_lwip_end();
    if (err != ERR_OK) {
        std::printf("mqtt_subscribe failed: %d\n", err);
        return false;
    }
    return true;
}

bool MqttClient::waitUntilConnected(uint32_t timeout_ms) const {
    absolute_time_t until = make_timeout_time_ms(timeout_ms);
    while (!connect_done_) {
        if (absolute_time_diff_us(get_absolute_time(), until) <= 0) {
            return false;
        }
        sleep_ms(10);
    }
    return true;
}

void MqttClient::stop() {
    if (client_) {
        client_ = nullptr;
    }
    stop_client_ = true;
}

void MqttClient::s_pub_request_cb(void* arg, err_t err) {
    if (err != ERR_OK) std::printf("pub_request_cb failed %d\n", err);
    (void)arg;
}

void MqttClient::s_sub_request_cb(void* arg, err_t err) {
    if (err != ERR_OK) std::printf("sub_request_cb failed %d\n", err);
    (void)arg;
}

void MqttClient::s_dns_found(const char* hostname, const ip_addr_t* ipaddr, void* arg) {
    auto* self = static_cast<MqttClient*>(arg);
    self->handle_dns_found(ipaddr);
}

void MqttClient::s_connection_cb(mqtt_client_t* client, void* arg, mqtt_connection_status_t status) {
    auto* self = static_cast<MqttClient*>(arg);
    self->handle_connection_status(status);
}

void MqttClient::s_incoming_publish_cb(void* arg, const char* topic, u32_t tot_len) {
    auto* self = static_cast<MqttClient*>(arg);
    self->handle_incoming_publish(topic);
}

void MqttClient::s_incoming_data_cb(void* arg, const u8_t* data, u16_t len, u8_t flags) {
    auto* self = static_cast<MqttClient*>(arg);
    self->handle_incoming_data(data, len);
}

void MqttClient::handle_dns_found(const ip_addr_t* ipaddr) {
    if (ipaddr) {
        server_addr_ = *ipaddr;
        start_client();
    } else {
        std::printf("dns request failed");
    }
}

void MqttClient::handle_connection_status(mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        connect_done_ = true;

        if (info_.will_topic) {
            cyw43_arch_lwip_begin();
            mqtt_publish(client_,
                            info_.will_topic,
                            "1", 1, MQTT_WILL_QOS, /*retain=*/1,
                            &MqttClient::s_pub_request_cb, this);
            cyw43_arch_lwip_end();
        }
        std::printf("MQTT connected\n");
    } else if (status == MQTT_CONNECT_DISCONNECTED) {
        if (!connect_done_) {
        std::printf("Failed to connect to mqtt server\n");
        }
    } else {
        std::printf("Unexpected MQTT status with flat %d\n", status);
    }
}

void MqttClient::handle_incoming_publish(const char* topic) {
    std::strncpy(topic_, topic, sizeof(topic_) - 1);
    topic_[sizeof(topic_) - 1] = '\0';
}

void MqttClient::handle_incoming_data(const u8_t* data, u16_t len) {
#if MQTT_UNIQUE_TOPIC
    const char* basic_topic = topic_ + std::strlen(info_.client_id) + 1;
#else
    const char* basic_topic = topic_;
#endif

    std::size_t copy_len = (len < sizeof(data_) - 1) ? len : (sizeof(data_) - 1);
    std::memcpy(data_, data, copy_len);
    data_[copy_len] = '\0';

    std::printf("Topic: %s, Message: %s\n", topic_, data_);

    if (std::strcmp(basic_topic, "/ping") == 0) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%u",
                        to_ms_since_boot(get_absolute_time()) / 1000);
        publish("/uptime", buf, MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, /*treat_as_basic_topic=*/true);
    }
}

void MqttClient::start_client() {
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    const int port = MQTT_TLS_PORT;
    std::printf("Using TLS\n");
#else
    const int port = MQTT_PORT_;
    std::printf("Warning: Not using TLS\n");
#endif
    cyw43_arch_lwip_begin();
    client_ = mqtt_client_new();
    cyw43_arch_lwip_end();
    if (!client_) std::printf("MQTT client instance creation error\n");

    std::printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    std::printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&server_addr_));

    cyw43_arch_lwip_begin();
    err_t err = mqtt_client_connect(client_,
                                    &server_addr_,
                                    port,
                                    &MqttClient::s_connection_cb,
                                    this,
                                    &info_);
                                    
    if (err != ERR_OK) std::printf("MQTT broker connection error with status %d\n", err);
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    if (client_ && client_->conn) {
        mbedtls_ssl_set_hostname(altcp_tls_context(client_->conn), MQTT_SERVER);
    }
#endif

    mqtt_set_inpub_callback(client_,
                            &MqttClient::s_incoming_publish_cb,
                            &MqttClient::s_incoming_data_cb,
                            this);
    cyw43_arch_lwip_end();
    
    
}

void MqttClient::build_full_topic(const char* name, char* out, std::size_t out_len) const {
#if MQTT_UNIQUE_TOPIC
    std::snprintf(out, out_len, "/%s%s", info_.client_id, name);
#else
    std::snprintf(out, out_len, "%s", name);
#endif
    out[out_len - 1] = '\0';
}