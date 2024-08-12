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

HAL::GPIO::gpio_state_t last_state{};
HAL::GPIO::gpio_state_t relay_state = HAL::GPIO::GPIO_STATE_LOW;

extern "C" {
__attribute__((noreturn)) void app_main(void) {
    printf("This a iot relay device\n");
    HAL::GPIO::gpio_cfg_t cfg{};
    cfg.pin = GPIO_NUM_20;
    cfg.pull_up = 1;
    cfg.direction = HAL::GPIO::GPIO_INPUT;
    auto key = new HAL::GPIO(cfg);

    last_state = key->Get();

    cfg.pin = GPIO_NUM_9;
    cfg.pull_up = 1;
    cfg.direction = HAL::GPIO::GPIO_OUTPUT;
    auto relay = new HAL::GPIO(cfg);

    relay->Set(HAL::GPIO::GPIO_STATE_LOW);


    for(;;){
        printf("Key value: %d\n", key->Get());
        if(last_state != key->Get()) {
            last_state = key->Get();
            relay_state = relay_state == HAL::GPIO::GPIO_STATE_LOW ? HAL::GPIO::GPIO_STATE_HIGH : HAL::GPIO::GPIO_STATE_LOW;
            printf("relay! %d\n", relay_state);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            relay->Set(relay_state);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    delete key;
}
}