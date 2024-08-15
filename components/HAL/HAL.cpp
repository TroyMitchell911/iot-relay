//
// Created by troy on 2024/8/14.
//

#include <nvs_flash.h>
#include "HAL.h"

static void storage_init() {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void network_init() {
    ESP_ERROR_CHECK(esp_netif_init());
}

void HAL::Init() {
    storage_init();
    network_init();

    /*  event initialization */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}