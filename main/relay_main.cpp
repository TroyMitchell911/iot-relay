/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <esp_event.h>
#include <nvs_flash.h>
#include <soc/gpio_num.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "HAL_WiFi.h"

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
    wifi.Sta("troyself-wifi", "troy888666", nullptr);

    for(;;){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
}