//
// Created by troy on 2024/8/13.
//

#include <esp_log.h>
#include "HomeAssistant.h"

#define TAG "[App::HomeAssistant]"

static const char *type2str[] = {"light", "switch"};

static const char *g_prefix = "homeassistant";

void App::HomeAssistant::Init(const char *where, entity_type_t type, const char *name, bool discovery) {
    char buffer[32];

    this->entity_where = where;
    this->entity_name = name;
    this->entity_type = type;

    this->GetTopic(this->command_topic, "command");
    ESP_LOGI(TAG, "command_topic: %s", this->command_topic);

    if(discovery) {
        this->discovery_topic = (char*)malloc(TOPIC_MAX_NUM);

        memset(this->discovery_topic, 0, TOPIC_MAX_NUM);
        sprintf(buffer, "%s-%s", where, name);
        sprintf(this->discovery_topic, "%s/%s/%s/config", g_prefix, type2str[type], buffer);
        ESP_LOGI(TAG, "discovery_topic: %s", this->discovery_topic);

        this->discovery_content = cJSON_CreateObject();
        cJSON_AddStringToObject(this->discovery_content, "name", buffer);
        cJSON_AddStringToObject(this->discovery_content, "device_class", type2str[type]);
        cJSON_AddStringToObject(this->discovery_content, "command_topic", this->command_topic);
        cJSON_AddStringToObject(this->discovery_content, "unique_id", buffer);

        wifi_mesh->GetMQTT().Subscribe(this->online_topic, 0);
        wifi_mesh->GetMQTT().Subscribe(this->offline_topic, 0);
    }

    wifi_mesh->GetMQTT().Subscribe(this->command_topic, 0);
}

App::HomeAssistant::HomeAssistant(HAL::WiFiMesh *mesh, const char *where, entity_type_t type, const char *name)
                    :  HomeAssistant(mesh, where, type, name, true) {

}

App::HomeAssistant::HomeAssistant(HAL::WiFiMesh *mesh, const char *where, entity_type_t type, const char *name, bool discovery) {
    if(type >= ENTITY_TYPE_MAX)
        return;
    this->wifi_mesh = mesh;
    this->Init(where, type, name, discovery);
    this->wifi_mesh->BindingCallback(App::HomeAssistant::Process, HAL::WiFiMesh::EVENT_DATA, (void*)this);
}

App::HomeAssistant::~HomeAssistant() {
    if(this->entity_discovery) {
        wifi_mesh->GetMQTT().Unsubscirbe(this->online_topic);
        wifi_mesh->GetMQTT().Unsubscirbe(this->offline_topic);
        wifi_mesh->GetMQTT().Unsubscirbe(this->discovery_topic);
        if(this->discovery_topic)
            free(this->discovery_topic);
        if(this->discovery_content)
            cJSON_Delete(this->discovery_content);
    }

    wifi_mesh->GetMQTT().Unsubscirbe(this->command_topic);
}

void App::HomeAssistant::Prefix(const char *prefix) {
    g_prefix = prefix;
}

void App::HomeAssistant::GetTopic(char *dst, const char *suffix) {
    sprintf(dst, "%s/%s/%s/%s", this->entity_where, type2str[this->entity_type], this->entity_name, suffix);
}

void App::HomeAssistant::Process(HAL::WiFiMesh::event_t event, void *data, void *arg) {
    auto *ha = (App::HomeAssistant*)arg;
    HAL::MQTT::msg_t msg{};
    if(event == HAL::WiFiMesh::EVENT_DATA) {
        auto *r_msg = (HAL::MQTT::msg_t*)data;

        if(strncmp(ha->online_topic, r_msg->topic, r_msg->topic_len) == 0) {
            if(strncmp(ha->online_content, r_msg->data, r_msg->len) == 0) {
                printf("online\n");
                msg.topic = ha->discovery_topic;
                msg.data = cJSON_Print(ha->discovery_content);
                msg.qos = 0;
                ha->wifi_mesh->Publish(&msg, sizeof(HAL::MQTT::msg_t));
            }
        } else if(strncmp(ha->offline_topic, r_msg->topic, r_msg->topic_len) == 0) {
            if(strncmp(ha->offline_content, r_msg->data, r_msg->len) == 0) {
                printf("offline\n");
            }
        }
    }
}



