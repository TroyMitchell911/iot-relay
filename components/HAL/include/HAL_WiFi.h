//
// Created by troy on 2024/8/9.
//

#ifndef RELAY_HAL_WIFI_H
#define RELAY_HAL_WIFI_H

#include <stdint.h>
#include <esp_wifi.h>
#include <esp_event.h>

namespace HAL {
    class WiFi {

    public:
    typedef enum{
        WiFi_CONNECTED,
        WiFi_DISCONNECTED,
        WiFi_GOT_IP,
    }wifi_event_t;
    typedef void (*wifi_callback_t)(wifi_event_t event, void *data);

    private:
        uint8_t sta_ssid[32]{};
        uint8_t sta_pwd[32]{};
        wifi_callback_t sta_callback;
        bool inited;

    public:
        static HAL::WiFi& GetInstance();
        void Init();
        esp_err_t Sta(const char *ssid, const char *pwd, wifi_callback_t callback);
        esp_err_t Sta(const char *ssid, const char *pwd, wifi_callback_t callback, uint8_t retry_num);

    public:
        WiFi() = default;
        ~WiFi() = default;
        WiFi(const WiFi &wifi) = delete;
        const WiFi &operator=(const WiFi &wifi) = delete;

    };
}


#endif //RELAY_HAL_WIFI_H
