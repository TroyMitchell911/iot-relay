//
// Created by troy on 2024/8/13.
//

#ifndef RELAY_HOMEASSISTANT_H
#define RELAY_HOMEASSISTANT_H

#include "cJSON.h"
#include "HAL_MQTT.h"

namespace App {
    class HomeAssistant {
    public:
        typedef enum {
            SWITCH,
            ENTITY_TYPE_MAX
        }entity_type_t;

    private:
        HAL::MQTT *mqtt = nullptr;
        char command_topic[64] = {0};
        char status_topic[64] = {0};
    public:
        static void Prefix(const char *prefix);
    private:
        void Init(const char *where, entity_type_t type, const char *name, bool discovery);
    public:
        HomeAssistant(HAL::MQTT *mqtt, const char *where, entity_type_t type, const char *name);
        HomeAssistant(HAL::MQTT *mqtt, const char *where, entity_type_t type, const char *name, bool discovery);
        ~HomeAssistant();
    };
}



#endif //RELAY_HOMEASSISTANT_H
