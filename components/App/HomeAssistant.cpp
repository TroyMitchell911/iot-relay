//
// Created by troy on 2024/8/13.
//

#include <esp_log.h>
#include "HomeAssistant.h"

#define TAG "[App::HomeAssistant]"

static const char *type2str[] = {"switch"};

static const char *g_prefix = "homeassistant";

void App::HomeAssistant::Init(const char *where, entity_type_t type, const char *name, bool discovery) {
    char buffer[32];
    char discovery_topic[64];

    sprintf(this->command_topic, "%s/%s/%s/command", where, type2str[type], name);
    sprintf(this->status_topic, "%s/%s/%s/status", where, type2str[type], name);
    ESP_LOGI(TAG, "command_topic: %s", this->command_topic);
    ESP_LOGI(TAG, "status_topic: %s", this->status_topic);
    mqtt->Subscribe(this->command_topic, 0);

    if(discovery) {
        sprintf(buffer, "%s-%s", where, name);

        cJSON* root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "name", buffer);
        cJSON_AddStringToObject(root, "device_class", type2str[type]);
        cJSON_AddStringToObject(root, "command_topic", this->status_topic);
        cJSON_AddStringToObject(root, "unique_id", buffer);

        sprintf(discovery_topic, "%s/%s/%s/config", g_prefix, type2str[type], buffer);
        ESP_LOGI(TAG, "discovery_topic: %s", discovery_topic);

        HAL::MQTT::msg_t msg{};
        msg.data = cJSON_Print(root);
        ESP_LOGI(TAG, "discovery_content: %s", msg.data);
        mqtt->Publish(discovery_topic, msg);
    }
}

App::HomeAssistant::HomeAssistant(HAL::MQTT *mqtt, const char *where, entity_type_t type, const char *name) {
    if(type >= ENTITY_TYPE_MAX)
        return;
    this->mqtt = mqtt;
    this->Init(where, type, name, true);
}

App::HomeAssistant::HomeAssistant(HAL::MQTT *mqtt, const char *where, entity_type_t type, const char *name, bool discovery) {
    if(type >= ENTITY_TYPE_MAX)
        return;
    this->mqtt = mqtt;
    this->Init(where, type, name, discovery);
}

App::HomeAssistant::~HomeAssistant() {

}

void App::HomeAssistant::Prefix(const char *prefix) {
    g_prefix = prefix;
}


