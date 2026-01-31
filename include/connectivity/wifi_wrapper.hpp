// implement the http requests with process.hpp

#ifdef PICO_BOARD_PICO2_W
#include <cstdint> 
#include <cstdio> 
#include <cstring> 

extern "C" { 
    #include "pico/stdlib.h" 
    #include "pico/cyw43_arch.h" 
    #include "lwip/pbuf.h" 
    #include "lwip/udp.h" 
    #include "lwip/ip_addr.h" 
} 

class WifiWrapper { 
public: 
    WifiWrapper(); 
    
    ~WifiWrapper(); 

    // Initialize CYW43 Wi-Fi stack 
    bool init(); 
    // Enable STA mode 
    void enableSta(); 
    
    // Connect using WPA2 AES PSK. Returns true on success. 
    bool connect(const char* ssid, const char* pass, uint32_t timeout_ms); 
    
    
    WifiWrapper(const WifiWrapper&) = delete; 
    WifiWrapper& operator=(const WifiWrapper&) = delete; 
    
private: 
    bool initialized_;
    const int attempts_;
};
#endif