//
// Created by troy on 2024/8/5.
//


#include "HAL_Time.h"
#include <cstdio>
#include <cstring>

void HAL::Time::update() {
    std::time(&this->std_time);
    this->tm = localtime(&this->std_time);

    memcpy(&this->t, this->tm, sizeof(struct HAL::date));
}

void HAL::Time::getTime(HAL::time_t time) {
    update();
    memcpy(time, &this->t.time, sizeof(struct HAL::time));
}

void HAL::Time::getTimeStr(char *buf) {
    update();
    strftime(buf, strlen("hh:mm:ss") + 1, "%T", (struct tm*)&this->t);
}

void HAL::Time::getDate(HAL::date_t date) {
    update();
    memcpy(date, &this->t, sizeof(struct HAL::date));
}

void HAL::Time::getDateStr(char *buf) {
    update();
    strftime(buf, strlen("yyyy:MM:dd") + 1, "%Y-%m-%d", (struct tm*)&this->t);
}

void HAL::Time::updateTime() {

}

HAL::Time &HAL::Time::getInstance() {
    static Time time;
    return time;
}

HAL::Time::Time() {

}

HAL::Time::~Time() {

}


