//
// Created by troy on 2024/8/9.
//

#include <cstring>
#include <esp_log.h>
#include "HAL_WiFi.h"

#define TAG "[HAL::WiFi]"

using namespace HAL;

static bool g_sta_status = false;

static uint8_t g_retry_num = 5;

static void WiFi_Event(void* event_handler_arg,
                       esp_event_base_t event_base,
                       int32_t event_id,
                       void* event_data) {
    static uint8_t retry_count = 0;
    auto  callback = (HAL::WiFi::wifi_callback_t)event_handler_arg;

    if(event_base == WIFI_EVENT) {
        switch(event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if(retry_count != g_retry_num) {
                    retry_count ++;
                    esp_wifi_connect();
                    ESP_LOGI(TAG, "retry connect");
                } else {
                    ESP_LOGI(TAG, "connect failed");
                    if(callback) {
                        callback(HAL::WiFi::WiFi_DISCONNECTED, &retry_count);
                    }
                }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = (ip_event_got_ip_t*) event_data;
        if(callback) {
            callback(HAL::WiFi::WiFi_GOT_IP, &event->ip_info.ip);
        }
    }
}

HAL::WiFi& HAL::WiFi::GetInstance() {
    static WiFi wifi;

    return wifi;
}

esp_err_t WiFi::Sta(const char *ssid, const char *pwd, wifi_callback_t callback) {
    if(!g_sta_status && inited) {
        wifi_config_t wifi_config{};


        this->sta_callback = callback;

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, WiFi_Event, (void*)this->sta_callback);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, WiFi_Event, (void*)this->sta_callback);

        strcpy((char*)wifi_config.sta.ssid, ssid);
        strcpy((char*)wifi_config.sta.password, pwd);

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        ESP_ERROR_CHECK(esp_wifi_start());
        g_sta_status = true;
    }
    return 0;
}

esp_err_t WiFi::Sta(const char *ssid, const char *pwd, WiFi::wifi_callback_t callback, uint8_t retry_num) {
    g_sta_status = retry_num;
    return this->Sta(ssid, pwd, callback);
}

void WiFi::Init() {
    ESP_ERROR_CHECK(esp_netif_init());

    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();

    this->inited = true;
}


