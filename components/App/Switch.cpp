//
// Created by troy on 2024/8/14.
//

#include "Switch.h"

static void update_status(HAL::WiFiMesh *mesh, char *status_topic, bool status) {
    HAL::MQTT::msg_t msg{};

    msg.data = (char*)(status ? "ON" : "OFF");
    msg.retain = 1;
    msg.qos = 0;
    msg.topic = status_topic;

    HAL::WiFiMesh::msg_t mesh_msg{};
    mesh_msg.data = &msg;
    mesh_msg.len = sizeof(HAL::MQTT::msg_t);
    mesh_msg.type = HAL::WiFiMesh::MSG_MQTT;
    mesh->Publish(mesh_msg);
}

App::Switch::Switch(HAL::WiFiMesh *mesh, const char *where, const char *name)
        : HomeAssistant(mesh, where, App::HomeAssistant::SWITCH, name) {
    this->wifi_mesh->BindingCallback(App::Switch::Process, HAL::WiFiMesh::EVENT_DATA, (void*)this);
    this->GetTopic(this->status_topic, "status");

    cJSON_AddStringToObject(this->discovery_content, "state_topic", this->status_topic);
    cJSON_AddBoolToObject(this->discovery_content, "optimistic", false);

    HAL::MQTT::msg_t msg{};
    msg.topic = this->discovery_topic;
    msg.data = cJSON_Print(this->discovery_content);
    msg.qos = 0;

    printf("%s\n", msg.data);
    
    HAL::WiFiMesh::msg_t mesh_msg{};
    mesh_msg.data = &msg;
    mesh_msg.len = sizeof(HAL::MQTT::msg_t);
    mesh_msg.type = HAL::WiFiMesh::MSG_MQTT;
    mesh->Publish(mesh_msg);
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

            update_status(sw->wifi_mesh, sw->status_topic, sw->sw_status);
        }
    }
}

void App::Switch::Act() {
    this->sw_status = this->sw_status ? false : true;
    update_status(this->wifi_mesh, this->status_topic, this->sw_status);
}
