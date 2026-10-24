#include "connectivity/mqtt_client.hpp"
#include "data.hpp"
#include <lwip/apps/mqtt.h>
#include <lwip/dns.h>
#include <pico/cyw43_arch.h>
#include <cstring>
#include <pico/unique_id.h>

MqttClient::MqttClient(void):
    _client(nullptr),
    _connect_done(false),
    _stop_client(false)
{
    // Build a unique client_id from Pico's unique ID
    pico_unique_board_id_t bid;
    pico_get_unique_board_id(&bid);
    std::snprintf(
        this->_client_id.data(),
        this->_client_id.size(),
        "%s-%02x%02x%02x%02x%02x%02x%02x%02x",
        MQTT_DEVICE_NAME,
        bid.id[0], bid.id[1], bid.id[2], bid.id[3],
        bid.id[4], bid.id[5], bid.id[6], bid.id[7]
    );

    this->_info.client_id  = this->_client_id.data();
    this->_info.keep_alive = MQTT_KEEP_ALIVE_S;
    this->_info.client_user = nullptr;
    this->_info.client_pass = nullptr;

    this->build_full_topic(MQTT_WILL_TOPIC, this->_will_topic);
    this->_info.will_topic   = this->_will_topic.data();
    this->_info.will_msg     = MQTT_WILL_MSG;
    this->_info.will_msg_len = strlen(MQTT_WILL_MSG);
    this->_info.will_qos     = MQTT_WILL_QOS;
    this->_info.will_retain  = 1;
}

MqttClient::~MqttClient() {
    this->stop();
}

void MqttClient::setAuth(const char* user, const char* pass) {
    this->_info.client_user = user;
    this->_info.client_pass = pass;
}

bool MqttClient::resolveAndConnect(const char* hostname) {
    
    std::printf("[DEBUG] DNS server: %s\n", ipaddr_ntoa(dns_getserver(0)));
    std::printf("Resolving MQTT server '%s'...\n", hostname);
    
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(hostname,
                                &this->_server_addr,
                                &MqttClient::s_dns_found,
                                this);
    cyw43_arch_lwip_end();
    
    if(err == ERR_OK)
    {
        // IP available immediately
        this->start_client();
        return true;
    }
    else if(err == ERR_INPROGRESS)
    {
        // Will complete in s_dns_found
        return true;
    }
    else
    {
        std::printf("dns_gethostbyname failed: %d\n", err);
        return false;
    }

    // Asyncronously waits for `handle_dns_found` to be called
    // and server address to be fetched successfully
    while(this->_server_addr.addr == 0)
    {
        cyw43_arch_poll();
        sleep_ms(1);
    }
}

bool MqttClient::send(PingData)
{
    return this->publish("/ping", "helloworld", 0, MQTT_PUBLISH_RETAIN, /*treat_as_basic_topic=*/true);
}


bool MqttClient::publish(std::string_view topic,
                         std::string_view payload,
                         u8_t qos,
                         u8_t retain,
                         bool treat_as_basic_topic)
{
    if(!this->_client)
    {
        return false;
    }
    cyw43_arch_lwip_begin();
    bool ok = mqtt_client_is_connected(this->_client);
    cyw43_arch_lwip_end();
    if(!ok)
    {
        return false;
    }

    std::array<char, MQTT_TOPIC_LEN> full_topic;
    const char* topic_ptr = topic.data();

    if (treat_as_basic_topic) {
        this->build_full_topic(topic, full_topic);
        topic_ptr = full_topic.data();
    }

    cyw43_arch_lwip_begin();
    err_t err = mqtt_publish(this->_client,
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
bool MqttClient::send(PacketData packet)
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
    if(!this->_client)
    {
        return false;
    }
    cyw43_arch_lwip_begin();
    if (!mqtt_client_is_connected(this->_client))
    {
        cyw43_arch_lwip_end();
        return false;
    }
    cyw43_arch_lwip_end();

    std::array<char, MQTT_TOPIC_LEN> full_topic;
    this->build_full_topic(name, full_topic);

    cyw43_arch_lwip_begin();
    err_t err = mqtt_subscribe(this->_client,
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
bool MqttClient::waitUntilConnected(uint32_t timeout_ms) const
{
    absolute_time_t until = make_timeout_time_ms(timeout_ms);
    while(!this->_connect_done)
    {
        if(absolute_time_diff_us(get_absolute_time(), until) <= 0)
        {
            return false;
        }
        sleep_ms(10);
    }
    return true;
}

void MqttClient::stop()
{
    if(this->_client)
    {
        this->_client = nullptr;
    }
    this->_stop_client = true;
}

void MqttClient::s_pub_request_cb(void* arg, err_t err)
{
    std::ignore = arg;

    if(err != ERR_OK)
    {
        std::printf("pub_request_cb failed %d\n", err);
    }
}

void MqttClient::s_sub_request_cb(void* arg, err_t err)
{
    std::ignore = arg;

    if(err != ERR_OK)
    {
        std::printf("sub_request_cb failed %d\n", err);
    }
}

void MqttClient::s_dns_found(const char* hostname, const ip_addr_t* ipaddr, void* arg)
{
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

void MqttClient::handle_dns_found(const ip_addr_t* ipaddr)
{
    if(!ipaddr)
    {
        std::printf("dns request failed");
        return;
    }

    this->_server_addr = *ipaddr;
    this->start_client();
}

void MqttClient::handle_connection_status(mqtt_connection_status_t status)
{
    switch(status)
    {
        case MQTT_CONNECT_ACCEPTED:
        {
            this->_connect_done = true;

            if (this->_info.will_topic) {
                cyw43_arch_lwip_begin();
                mqtt_publish(this->_client,
                                this->_info.will_topic,
                                "1", 1, MQTT_WILL_QOS, /*retain=*/1,
                                &MqttClient::s_pub_request_cb, this);
                cyw43_arch_lwip_end();
            }
            std::printf("MQTT connected\n");
            break;
        }
        case MQTT_CONNECT_DISCONNECTED:
        {
            if(!this->_connect_done)
            {
                std::printf("Failed to connect to mqtt server\n");
            }
            break;
        }
        default:
        {
            std::printf("Unexpected MQTT status with flat %d\n", status);
            break;
        }
    }
}

void MqttClient::handle_incoming_publish(const char* topic)
{
    std::snprintf(
        this->_topic.data(),
        this->_topic.size(),
        "%s",
        topic
    );
}

void MqttClient::handle_incoming_data(const u8_t* data, u16_t len)
{
    // NOTE: this might need documentation, the MQTT_UNIQUE_TOPIC thing
#if MQTT_UNIQUE_TOPIC
    const char* basic_topic = this->_topic.data() + std::strlen(this->_client_id.data()) + 1;
#else
    const char* basic_topic = this->_topic.data();
#endif

    std::size_t copy_len = std::min<std::size_t>(len, this->_data.size() - 1);

    std::memcpy(this->_data.data(), data, copy_len);
    this->_data[copy_len] = '\0';

    std::printf(
        "Topic: %s, Message: %s\n",
        this->_topic.data(),
        this->_data.data()
    );

    if(std::strcmp(basic_topic, "/ping") == 0)
    {
        char buf[16];
        std::snprintf(
            buf,
            sizeof(buf),
            "%u",
            to_ms_since_boot(get_absolute_time()) / 1000
        );

        this->publish(
            "/uptime",
            buf,
            MQTT_PUBLISH_QOS,
            MQTT_PUBLISH_RETAIN,
            true
        );
    }
}

void MqttClient::start_client()
{
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    const int port = MQTT_TLS_PORT;
    std::printf("Using TLS\n");
#else
    const int port = MQTT_PORT_;
    std::printf("Warning: Not using TLS\n");
#endif
    cyw43_arch_lwip_begin();
    this->_client = mqtt_client_new();
    cyw43_arch_lwip_end();
    
    if(!this->_client)
    {
        std::printf("MQTT client instance creation error\n");
    }

    std::printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    std::printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&this->_server_addr));

    cyw43_arch_lwip_begin();
    err_t err = mqtt_client_connect(
        this->_client,
        &this->_server_addr,
        port,
        &MqttClient::s_connection_cb,
        this,
        &this->_info
    );
                                    
    if(err != ERR_OK)
    {
        std::printf("MQTT broker connection error with status %d\n", err);
    }

#if LWIP_ALTCP && LWIP_ALTCP_TLS
    if(this->_client && this->_client->conn)
    {
        mbedtls_ssl_set_hostname(altcp_tls_context(this->_client->conn), MQTT_SERVER);
    }
#endif

    mqtt_set_inpub_callback(
        this->_client,
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