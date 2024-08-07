/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"

#include "../components/HAL/include/HAL_Time.h"

extern "C" {
__attribute__((noreturn)) void app_main(void) {
    HAL::Time &time = HAL::Time::GetInstance();

    printf("This a iot relay device\n");

    for (;;) {
        char *buf_date, *buf_time, *buf_clock, *buf_week;
        HAL::date_t date;
        HAL::time_t t;
        HAL::clock_t clock;

        time.GetDate(&date);
        time.GetTime(&t);
        printf("GetDate: %d-%d-%d  %d:%d:%d\n", date->year, date->mon, date->day, t->hour, t->min, t->sec);

        time.GetDate(&buf_date);
        time.GetTime(&buf_time);
        printf("GetDateStr: %s %s\n", buf_date, buf_time);

        time.GetClock(&clock);
        printf("GetClock: %d-%d-%d  %d:%d:%d %d\n", clock->year, clock->mon, clock->day, clock->hour, clock->min, clock->sec, clock->wday);

        time.GetClock(&buf_clock);
        printf("GetClockStr: %s\n", buf_clock);

        time.GetWeek(&buf_week, false);
        printf("GetWeek: %s ", buf_week);

        time.GetWeek(&buf_week, true);
        printf("%s\n", buf_week);


        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
}