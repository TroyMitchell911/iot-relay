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
#include "HAL_GPIO.h"

extern "C" {
__attribute__((noreturn)) void app_main(void) {
    printf("This a iot relay device\n");
    HAL::GPIO::gpio_cfg_t cfg;
    cfg.pin = GPIO_NUM_9;
    cfg.pull_up = 1;
    cfg.direction = HAL::GPIO::GPIO_INPUT;
    auto key = new HAL::GPIO(cfg);

    for(;;){
        printf("Key value: %d\n", key->Get());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    delete key;
}
}