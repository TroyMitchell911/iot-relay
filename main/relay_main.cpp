/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"

#include "HAL/HAL_Time.h"

extern "C" {
void app_main(void) {
    char buffer[32];
    HAL::Time &time = HAL::Time::getInstance();

    printf("This a iot relay device\n");

    for (;;) {
        struct HAL::date date{};
        time.getDate(&date);
        printf("getDate: %d-%d-%d %d:%d:%d\n", date.year, date.mon, date.day,
               date.time.hour,
               date.time.min,
               date.time.sec);
        time.getTime(&date.time);
        printf("getTime: %d:%d:%d\n",  date.time.hour,
               date.time.min,
               date.time.sec);
        time.getDateStr(buffer);
        printf("getDateStr:%s\n", buffer);
        time.getTimeStr(buffer);
        printf("getTimeStr:%s\n", buffer);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
}