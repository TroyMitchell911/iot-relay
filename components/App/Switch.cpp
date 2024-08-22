//
// Created by troy on 2024/8/14.
//

#include "Switch.h"

#define TAG "[App::Switch]"

static void update_status(HAL::WiFiMesh *mesh, char *status_topic, bool status) {
    HAL::MQTT::msg_t msg{};

    strcpy(msg.data, status ? "ON" : "OFF");
    strcpy(msg.topic, status_topic);
    msg.retain = 1;
    msg.qos = 0;

    mesh->Publish(&msg, sizeof(HAL::MQTT::msg_t), HAL::WiFiMesh::MSG_MQTT);
}

App::Switch::Switch(HAL::WiFiMesh *mesh, const char *where, const char *name)
        : HomeAssistant(mesh, where, App::HomeAssistant::SWITCH, name) {
    this->wifi_mesh->BindingCallback(App::Switch::Process, HAL::WiFiMesh::EVENT_DATA, (void*)this);
    this->GetTopic(this->status_topic, "status");
}

void App::Switch::Process(HAL::WiFiMesh::event_t event, void *data, void *arg) {
    auto *sw = (App::Switch*)arg;
    if(event == HAL::WiFiMesh::EVENT_DATA) {
        auto *r_msg = (HAL::MQTT::msg_t*) data;
        if(strncmp(r_msg->topic, sw->command_topic, r_msg->topic_len) == 0) {
            if(strncmp(r_msg->data, "ON", r_msg->len) == 0) {
                sw->sw_status = true;
            } else if(strncmp(r_msg->data, "OFF", r_msg->len) == 0) {
                sw->sw_status = false;
            } else {
                return;
            }

            sw->Act(false);
        }
    }
}

void App::Switch::Act() {
    this->Act(true);
}

void App::Switch::Act(bool set_value) {
    ESP_LOGI(TAG, "Act");
    if(set_value)
        this->sw_status = !this->sw_status;
    update_status(this->wifi_mesh, this->status_topic, this->sw_status);
}

void App::Switch::Init() {
    HomeAssistant::Init();

    cJSON_AddStringToObject(this->discovery_content, "state_topic", this->status_topic);
    cJSON_AddBoolToObject(this->discovery_content, "optimistic", false);
    HAL::MQTT::msg_t msg{};
    strcpy(msg.topic, this->discovery_topic);
    strcpy(msg.data, cJSON_Print(this->discovery_content));
    this->wifi_mesh->Publish(&msg, sizeof(HAL::MQTT::msg_t), HAL::WiFiMesh::MSG_MQTT);
}
