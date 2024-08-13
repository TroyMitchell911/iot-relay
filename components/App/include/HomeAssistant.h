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
            LIGHT,
            ENTITY_TYPE_MAX
        }entity_type_t;

    private:
        HAL::MQTT *mqtt = nullptr;
        char command_topic[64] = {0};
        char status_topic[64] = {0};
        bool light_status = false;
    public:
        static void Prefix(const char *prefix);
    private:
        void Init(const char *where, entity_type_t type, const char *name, bool discovery);
    public:
        void Process(char *topic, int topic_len, char *data, int data_len);
        HomeAssistant(HAL::MQTT *mqtt, const char *where, entity_type_t type, const char *name);
        HomeAssistant(HAL::MQTT *mqtt, const char *where, entity_type_t type, const char *name, bool discovery);
        ~HomeAssistant();
    };
}



#endif //RELAY_HOMEASSISTANT_H
