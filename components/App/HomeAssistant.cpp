//
// Created by troy on 2024/8/13.
//

#include <esp_log.h>
#include "HomeAssistant.h"

#define TAG "[App::HomeAssistant]"

static const char *type2str[] = {"light"};

static const char *g_prefix = "homeassistant";

void App::HomeAssistant::Init(const char *where, entity_type_t type, const char *name, bool discovery) {
    char buffer[32];

    this->GetTopic(this->command_topic, "command");
    ESP_LOGI(TAG, "command_topic: %s", this->command_topic);
    mqtt->Subscribe(this->command_topic, 0);

    if(discovery) {
        this->discovery_topic = (char*)malloc(TOPIC_MAX_NUM);
        memset(this->discovery_topic, 0, TOPIC_MAX_NUM);
        sprintf(this->discovery_topic, "%s/%s/%s-%s/config", g_prefix, type2str[type], where, name);
        ESP_LOGI(TAG, "discovery_topic: %s", this->discovery_topic);

        this->discovery_content = cJSON_CreateObject();
        cJSON_AddStringToObject(this->discovery_content, "name", buffer);
        cJSON_AddStringToObject(this->discovery_content, "device_class", type2str[type]);
        cJSON_AddStringToObject(this->discovery_content, "command_topic", this->command_topic);
        cJSON_AddBoolToObject(this->discovery_content, "optimistic", false);
        cJSON_AddStringToObject(this->discovery_content, "unique_id", buffer);
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

void App::HomeAssistant::GetTopic(char *dst, const char *suffix) {
    sprintf(dst, "%s/%s/%s/%s", this->entity_where, type2str[this->entity_type], this->entity_name, suffix);
}

void App::HomeAssistant::Process(char *topic, int topic_len, char *data, int data_len) {

}



