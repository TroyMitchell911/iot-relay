//
// Created by troy on 2024/8/14.
//

#ifndef RELAY_SWITCH_H
#define RELAY_SWITCH_H

#include "HomeAssistant.h"

namespace App {
    class Switch : private App::HomeAssistant{

    private:
        bool sw_status = false;
        char status_topic[TOPIC_MAX_NUM];
    private:
        static void Process(HAL::MQTT::event_t event, void *data, void *arg);
    public:
        Switch(HAL::MQTT *mqtt, const char *where, const char *name);
        void Act();
    };
}



#endif //RELAY_SWITCH_H
