/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <nvs_flash.h>
#include <soc/gpio_num.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_log.h>
#include <mqtt_client.h>
#include <esp_ota_ops.h>
#include "freertos/FreeRTOS.h"
#include "HAL_WiFi.h"
#include "HAL_MQTT.h"

#define TAG "[main]"

static void wifi_event(HAL::WiFi::wifi_event_t event_id, void *event_data) {
    if(event_id == HAL::WiFi::WiFi_DISCONNECTED) {
    } else if(event_id == HAL::WiFi::WiFi_GOT_IP) {
        auto *data = (esp_ip4_addr_t*)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(data));
        auto mqtt = new HAL::MQTT("uri", "root", "123", ca);
    }
}

extern "C" {
__attribute__((noreturn)) void app_main(void) {
    printf("This a iot relay device\n");

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    HAL::WiFi& wifi = HAL::WiFi::GetInstance();

    wifi.Init();
//    wifi.Sta("troyself-wifi", "troy888666", wifi_event);
    wifi.Sta("HBDT-23F", "hbishbis", wifi_event);
//    wifi.Sta("troy-phone", "jianglin998", wifi_event);

    for(;;){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
}