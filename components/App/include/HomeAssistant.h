//
// Created by troy on 2024/8/13.
//

#ifndef RELAY_HOMEASSISTANT_H
#define RELAY_HOMEASSISTANT_H

#include "cJSON.h"
#include "HAL_WiFiMesh.h"

namespace App {
    class HomeAssistant {
    public:
        typedef enum {
            LIGHT,
            SWITCH,
            ENTITY_TYPE_MAX
        }entity_type_t;

    private:
        const char *online_topic = "homeassistant/status";
        const char *offline_topic = "homeassistant/status";
        const char *online_content = "online";
        const char *offline_content = "offline";

    protected:
        HAL::WiFiMesh *wifi_mesh;
        entity_type_t entity_type = App::HomeAssistant::ENTITY_TYPE_MAX;
        const char *entity_where;
        const char *entity_name;
        bool entity_discovery;

//        char command_topic[MQTT_TOPIC_MAX_NUM] = {0};
        char status_topic[MQTT_TOPIC_MAX_NUM] = {0};
        char *discovery_topic;
        cJSON *discovery_content;

        HAL::WiFiMesh::device_info_t self_info;

    public:
        static void Prefix(const char *prefix);

    private:
        static void Process(HAL::WiFiMesh::event_t event, void *data, void *arg);

    protected:
        void GetTopic(char *dst, const char *suffix);

    public:
        virtual void Init();

        HomeAssistant(HAL::WiFiMesh *mesh, const char *where, entity_type_t type, const char *name);
        HomeAssistant(HAL::WiFiMesh *mesh, const char *where, entity_type_t type, const char *name, bool discovery);
        ~HomeAssistant();
    };
}



#endif //RELAY_HOMEASSISTANT_H
