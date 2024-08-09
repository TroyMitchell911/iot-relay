//
// Created by troy on 2024/8/5.
//


#include "HAL_Time.h"
#include <cstring>

/* strlen("hh:mm:ss") + 1 = 9 */
#define TIME_STR_LEN    9
/* strlen("yyyy:MM:dd") + 1 = 11 */
#define DATE_STR_LEN    11
/* strlen("yyyy:MM:dd hh:mm:ss") + 1 = 19 */
#define CLOCK_STR_LEN   19

void HAL::Time::update() {
    std::time(&this->std_time);
    this->tm = localtime(&this->std_time);

    memcpy(&this->g_clock, this->tm, sizeof(struct HAL::clock));
}

void HAL::Time::GetTime(HAL::time_t *time) {
    static struct HAL::time t;

    update();
    memcpy(&t, &this->g_clock, sizeof(struct HAL::time));

    *time = &t;
}

void HAL::Time::GetTime(char **time) {
    static char buf[TIME_STR_LEN];

    update();
    strftime(buf, TIME_STR_LEN, "%T", (struct tm*)&this->g_clock);

    *time = buf;
}

void HAL::Time::GetDate(HAL::date_t *date) {
    static struct HAL::date d;

    update();
    memcpy(&d, &this->g_clock.day, sizeof(struct HAL::date));

    *date = &d;
}

void HAL::Time::GetDate(char **date) {
    static char buf[DATE_STR_LEN];

    update();
    strftime(buf, DATE_STR_LEN, "%Y-%m-%d", (struct tm*)&this->g_clock);

    *date = buf;
}

void HAL::Time::GetClock(HAL::clock_t *clock) {
    static struct HAL::clock c;

    update();
    memcpy(&c, &this->g_clock, sizeof(struct HAL::clock));

    *clock = &c;

}

void HAL::Time::GetClock(char **clock) {
    static char buf[CLOCK_STR_LEN];

    update();
    strftime(buf, CLOCK_STR_LEN, "%Y-%m-%d %T", (struct tm*)&this->g_clock);

    *clock = buf;
}


int HAL::Time::GetWeek(char **week, bool full) {
    static char buf[16];

    update();
    strftime(buf, CLOCK_STR_LEN, full ? "%A" : "%a", (struct tm*)&this->g_clock);

    *week = buf;

    return this->g_clock.wday;
}

void HAL::Time::SyncTime() {

}

HAL::Time &HAL::Time::GetInstance() {
    static Time time;
    return time;
}

HAL::Time::Time() {

}

HAL::Time::~Time() {

}




