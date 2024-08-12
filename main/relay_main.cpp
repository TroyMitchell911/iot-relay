/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <esp_event.h>
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"

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

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    for (;;) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
}