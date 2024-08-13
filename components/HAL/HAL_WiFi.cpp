//
// Created by troy on 2024/8/9.
//

#include <cstring>
#include <esp_log.h>
#include "include/HAL_WiFi.h"

#define TAG "[HAL::WiFi]"

using namespace HAL;

static bool sta_status = false;

static void WiFi_Event(void* event_handler_arg,
                       esp_event_base_t event_base,
                       int32_t event_id,
                       void* event_data) {
    if(event_base == WIFI_EVENT) {
        switch(event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

HAL::WiFi& HAL::WiFi::GetInstance() {
    static WiFi wifi;

    return wifi;
}

esp_err_t WiFi::Sta(char *ssid, char *pwd, wifi_callback_t callback) {
    if(!sta_status && inited) {
        wifi_config_t wifi_config{};

        if(callback)
            this->sta_callback = callback;

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, WiFi_Event, nullptr);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, WiFi_Event, nullptr);

        strcpy((char*)wifi_config.sta.ssid, ssid);
        strcpy((char*)wifi_config.sta.password, pwd);

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        ESP_ERROR_CHECK(esp_wifi_start());
        sta_status = true;
    }
    return 0;
}

void WiFi::Init() {
    ESP_ERROR_CHECK(esp_netif_init());

    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();

    this->inited = true;
}
