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
#define TOPIC_MAX_NUM   64

    private:
        HAL::MQTT *mqtt = nullptr;
        entity_type_t entity_type = App::HomeAssistant::ENTITY_TYPE_MAX;
        const char *entity_where;
        const char *entity_name;
        bool entity_discovery;

        char command_topic[TOPIC_MAX_NUM] = {0};
        char *discovery_topic;
        cJSON *discovery_content;

    public:
        static void Prefix(const char *prefix);

    private:
        void Init(const char *where, entity_type_t type, const char *name, bool discovery);

    public:
        void GetTopic(char *dst, const char *suffix);
        void Process(char *topic, int topic_len, char *data, int data_len);
        HomeAssistant(HAL::MQTT *mqtt, const char *where, entity_type_t type, const char *name);
        HomeAssistant(HAL::MQTT *mqtt, const char *where, entity_type_t type, const char *name, bool discovery);
        ~HomeAssistant();
    };
}



#endif //RELAY_HOMEASSISTANT_H
