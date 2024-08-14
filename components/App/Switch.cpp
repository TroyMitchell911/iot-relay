//
// Created by troy on 2024/8/14.
//

#include "Switch.h"

App::Switch::Switch(HAL::MQTT *mqtt, const char *where, App::HomeAssistant::entity_type_t type, const char *name)
        : HomeAssistant(mqtt, where, type, name) {
    this->mqtt->BindingCallback(App::Switch::Process, HAL::MQTT::EVENT_DATA, (void*)this);
    this->GetTopic(this->status_topic, "status");

    cJSON_AddStringToObject(this->discovery_content, "state_topic", this->status_topic);
    cJSON_AddBoolToObject(this->discovery_content, "optimistic", false);

    HAL::MQTT::msg_t msg{};
    msg.topic = this->discovery_topic;
    msg.data = cJSON_Print(this->discovery_content);
    msg.qos = 0;
    printf("%s\n", msg.data);
    mqtt->Publish(msg);
}

void App::Switch::Process(HAL::MQTT::event_t event, void *data, void *arg) {
    auto *sw = (App::Switch*)arg;
    if(event == HAL::MQTT::EVENT_DATA) {
        auto *r_msg = (HAL::MQTT::msg_t*) data;
        if(strncmp(r_msg->topic, sw->command_topic, r_msg->topic_len) == 0) {
            if(strncmp(r_msg->data, "ON", r_msg->len) == 0) {
                sw->sw_status = true;
            } else if(strncmp(r_msg->data, "OFF", r_msg->len) == 0) {
                sw->sw_status = false;
            }

            HAL::MQTT::msg_t msg{};

            msg.data = (char*)(sw->sw_status ? "ON" : "OFF");
            msg.retain = 1;
            msg.qos = 0;
            msg.topic = sw->status_topic;
            sw->mqtt->Publish(msg);
        }
    }
}
