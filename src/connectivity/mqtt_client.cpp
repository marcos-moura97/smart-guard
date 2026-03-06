#include "connectivity/mqtt_client.hpp"

MqttClient::MqttClient()
    : client_(nullptr),
      connect_done_(false),
      stop_client_(false)
{
    // Build a unique client_id from Pico's unique ID
    pico_unique_board_id_t bid;
    pico_get_unique_board_id(&bid);
    std::snprintf(client_id_.data(),
                  client_id_.size(),
                  "%s-%02x%02x%02x%02x%02x%02x%02x%02x",
                  MQTT_DEVICE_NAME,
                  bid.id[0], bid.id[1], bid.id[2], bid.id[3],
                  bid.id[4], bid.id[5], bid.id[6], bid.id[7]);

    this->info_.client_id  = client_id_.data();
    this->info_.keep_alive = MQTT_KEEP_ALIVE_S;
    this->info_.client_user = nullptr;
    this->info_.client_pass = nullptr;

    this->build_full_topic(MQTT_WILL_TOPIC, this->will_topic_);
    this->info_.will_topic   = this->will_topic_.data();
    this->info_.will_msg     = MQTT_WILL_MSG;
    this->info_.will_msg_len = strlen(MQTT_WILL_MSG);
    this->info_.will_qos     = MQTT_WILL_QOS;
    this->info_.will_retain  = 1;
}

MqttClient::~MqttClient() {
    stop();
}

void MqttClient::setAuth(const char* user, const char* pass) {
    this->info_.client_user = user;
    this->info_.client_pass = pass;
}

bool MqttClient::resolveAndConnect(const char* hostname) {
    
    std::printf("[DEBUG] DNS server: %s\n", ipaddr_ntoa(dns_getserver(0)));
    std::printf("Resolving MQTT server '%s'...\n", hostname);
    
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(hostname,
                                &this->server_addr_,
                                &MqttClient::s_dns_found,
                                this);
    cyw43_arch_lwip_end();
    
    if (err == ERR_OK) {
        // IP available immediately
        this->start_client();
        return true;
    } else if (err == ERR_INPROGRESS) {
        // Will complete in s_dns_found
        return true;
    } else {
        std::printf("dns_gethostbyname failed: %d\n", err);
        return false;
    }
    while (this->server_addr_.addr == 0) {
        cyw43_arch_poll();
        sleep_ms(1);
    }
}

void MqttClient::publish_ping(){
    bool status = this->publish("/ping", "helloworld", 0, MQTT_PUBLISH_RETAIN, /*treat_as_basic_topic=*/true);
}


bool MqttClient::publish(std::string_view topic,
                         std::string_view payload,
                         u8_t qos,
                         u8_t retain,
                         bool treat_as_basic_topic)
{
    cyw43_arch_lwip_begin();
    bool ok = this->client_ && mqtt_client_is_connected(this->client_);
    cyw43_arch_lwip_end();
    if (!ok) return false;

    std::array<char, MQTT_TOPIC_LEN> full_topic;
    const char* topic_ptr = topic.data();

    if (treat_as_basic_topic) {
        this->build_full_topic(topic, full_topic);
        topic_ptr = full_topic.data();
    }

    cyw43_arch_lwip_begin();
    err_t err = mqtt_publish(this->client_,
                             topic_ptr,
                             payload.data(),
                             payload.size(),
                             qos,
                             retain,
                             &MqttClient::s_pub_request_cb,
                             this);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        std::printf("mqtt_publish failed: %d\n", err);
        return false;
    }

    return true;
}
bool MqttClient::publishPayload(const PacketData& packet)
{
    std::string json {packet.format_json()};

    return this->publish(
        MQTT_DATA_TOPIC,
        json.c_str(),
        0,
        MQTT_PUBLISH_RETAIN,
        true
    );
}

bool MqttClient::subscribe(std::string_view name, u8_t qos)
{
    cyw43_arch_lwip_begin();
    if (!this->client_ || !mqtt_client_is_connected(this->client_)) {
        cyw43_arch_lwip_end();
        return false;
    }
    cyw43_arch_lwip_end();

    std::array<char, MQTT_TOPIC_LEN> full_topic;
    this->build_full_topic(name, full_topic);

    cyw43_arch_lwip_begin();
    err_t err = mqtt_subscribe(this->client_,
                               full_topic.data(),
                               qos,
                               &MqttClient::s_sub_request_cb,
                               this);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        std::printf("mqtt_subscribe failed: %d\n", err);
        return false;
    }

    return true;
}
bool MqttClient::waitUntilConnected(uint32_t timeout_ms) const {
    absolute_time_t until = make_timeout_time_ms(timeout_ms);
    while (!this->connect_done_) {
        if (absolute_time_diff_us(get_absolute_time(), until) <= 0) {
            return false;
        }
        sleep_ms(10);
    }
    return true;
}

void MqttClient::stop() {
    if (this->client_) {
        this->client_ = nullptr;
    }
    this->stop_client_ = true;
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
        this->server_addr_ = *ipaddr;
        this->start_client();
    } else {
        std::printf("dns request failed");
    }
}

void MqttClient::handle_connection_status(mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        this->connect_done_ = true;

        if (this->info_.will_topic) {
            cyw43_arch_lwip_begin();
            mqtt_publish(this->client_,
                            this->info_.will_topic,
                            "1", 1, MQTT_WILL_QOS, /*retain=*/1,
                            &MqttClient::s_pub_request_cb, this);
            cyw43_arch_lwip_end();
        }
        std::printf("MQTT connected\n");
    } else if (status == MQTT_CONNECT_DISCONNECTED) {
        if (!this->connect_done_) {
        std::printf("Failed to connect to mqtt server\n");
        }
    } else {
        std::printf("Unexpected MQTT status with flat %d\n", status);
    }
}

void MqttClient::handle_incoming_publish(const char* topic)
{
    std::snprintf(this->topic_.data(),
                  this->topic_.size(),
                  "%s",
                  topic);
}

void MqttClient::handle_incoming_data(const u8_t* data, u16_t len)
{
#if MQTT_UNIQUE_TOPIC
    const char* basic_topic = this->topic_.data() + std::strlen(client_id_.data()) + 1;
#else
    const char* basic_topic = this->topic_.data();
#endif

    std::size_t copy_len = std::min<std::size_t>(len, this->data_.size() - 1);

    std::memcpy(this->data_.data(), data, copy_len);
    this->data_[copy_len] = '\0';

    std::printf("Topic: %s, Message: %s\n",
                this->topic_.data(),
                this->data_.data());

    if (std::strcmp(basic_topic, "/ping") == 0) {
        char buf[16];
        std::snprintf(buf,
                      sizeof(buf),
                      "%u",
                      to_ms_since_boot(get_absolute_time()) / 1000);

        this->publish("/uptime",
                buf,
                MQTT_PUBLISH_QOS,
                MQTT_PUBLISH_RETAIN,
                true);
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
    this->client_ = mqtt_client_new();
    cyw43_arch_lwip_end();
    if (!this->client_) std::printf("MQTT client instance creation error\n");

    std::printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    std::printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&this->server_addr_));

    cyw43_arch_lwip_begin();
    err_t err = mqtt_client_connect(this->client_,
                                    &this->server_addr_,
                                    port,
                                    &MqttClient::s_connection_cb,
                                    this,
                                    &this->info_);
                                    
    if (err != ERR_OK) std::printf("MQTT broker connection error with status %d\n", err);
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    if (this->client_ && this->client_->conn) {
        mbedtls_ssl_set_hostname(altcp_tls_context(this->client_->conn), MQTT_SERVER);
    }
#endif

    mqtt_set_inpub_callback(this->client_,
                            &MqttClient::s_incoming_publish_cb,
                            &MqttClient::s_incoming_data_cb,
                            this);
    cyw43_arch_lwip_end();
    
    
}

void MqttClient::build_full_topic(std::string_view name,
                                  std::array<char, MQTT_TOPIC_LEN>& out) const
{
#if MQTT_UNIQUE_TOPIC
    std::snprintf(out.data(),
                  out.size(),
                  "/%s%.*s",
                  client_id_.data(),
                  static_cast<int>(name.size()),
                  name.data());
#else
    std::snprintf(out.data(),
                  out.size(),
                  "%.*s",
                  static_cast<int>(name.size()),
                  name.data());
#endif

    out.back() = '\0';
}