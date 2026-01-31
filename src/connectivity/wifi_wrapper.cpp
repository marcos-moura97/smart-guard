
#ifdef PICO_BOARD_PICO2_W   
#include "connectivity/wifi_wrapper.hpp" 

WifiWrapper::WifiWrapper(): initialized_(false),attempts_(10) {} 

bool WifiWrapper::init() { 
    if (initialized_) 
        return true; 
    if (cyw43_arch_init()) { 
        std::printf("CYW43: failed to initialise\n"); 
        return false; 
    } 
    
    initialized_ = true; 
    return true; 
}
WifiWrapper::~WifiWrapper() { 
    if (initialized_) { 
        cyw43_arch_deinit(); 
    } 
} 

void WifiWrapper::enableSta() { 
    cyw43_arch_enable_sta_mode(); 
} 

bool WifiWrapper::connect(const char* ssid, const char* pass, uint32_t timeout_ms) { 
    std::printf("Connecting to Wi-Fi...\n"); 
    for(int i=0; i<attempts_; i++){
        int rc = cyw43_arch_wifi_connect_timeout_ms( ssid, pass, CYW43_AUTH_WPA2_AES_PSK, timeout_ms ); 
        if (rc) { 
            std::printf("Wi-Fi: failed to connect (rc=%d). Retrying ...\n", rc); 
            continue;
        } 
        std::printf("Wi-Fi: connected to network %s.\n", ssid); 
        return true;
    } 
}
#endif