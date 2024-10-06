//
// Created by troy on 2024/8/14.
//

#include "Switch.h"

#define TAG "[App::Switch]"

static void update_status(HAL::WiFiMesh *mesh, char *status_topic, bool status) {
    HAL::MQTT::msg_t msg{};

    strcpy(msg.data, status ? "ON" : "OFF");
    strcpy(msg.topic, status_topic);
    msg.retain = 0;
    msg.qos = 0;

    mesh->Publish(&msg, sizeof(HAL::MQTT::msg_t), HAL::WiFiMesh::MSG_MQTT);
}

App::Switch::Switch(HAL::WiFiMesh *mesh,
                    const char *where,
                    const char *name,
                    int gpio_num,
                    int active_state)
        : HomeAssistant(mesh, where, App::HomeAssistant::SWITCH, name) {
    this->switch_active_state = HAL::GPIO::gpio_state_t(active_state);

    HAL::GPIO::gpio_cfg_t cfg{};
    cfg.pin = gpio_num;
    if(this->switch_active_state == HAL::GPIO::GPIO_STATE_HIGH)
        cfg.pull_down = 1;
    else if(this->switch_active_state == HAL::GPIO::GPIO_STATE_LOW)
        cfg.pull_up = 1;
    cfg.direction = HAL::GPIO::GPIO_OUTPUT;
    cfg.mode = HAL::GPIO::GPIO_PP;
    this->switch_gpio = new HAL::GPIO(cfg);


    this->switch_gpio->Set(HAL::GPIO::gpio_state_t(!this->switch_active_state));

    this->wifi_mesh->BindingCallback(App::Switch::Process, HAL::WiFiMesh::EVENT_DATA, (void*)this);
    this->GetTopic(this->command_topic, "command");
}

App::Switch::Switch(HAL::WiFiMesh *mesh, const char *where, const char *name, int switch_gpio_num,
                    int switch_active_state, int manual_button_gpio_num, int manual_button_active_state)
        : Switch(mesh, where, name, switch_gpio_num, switch_active_state){
    this->manual_button_active_state = HAL::GPIO::gpio_state_t(manual_button_active_state);

    HAL::GPIO::gpio_cfg_t cfg{};
    cfg.pin = manual_button_gpio_num;
    if(this->manual_button_active_state == HAL::GPIO::GPIO_STATE_HIGH)
        cfg.pull_down = 1;
    else if(this->manual_button_active_state == HAL::GPIO::GPIO_STATE_LOW)
        cfg.pull_up = 1;
    cfg.direction = HAL::GPIO::GPIO_INPUT;
    cfg.mode = HAL::GPIO::GPIO_PP;
    this->manual_button_gpio = new HAL::GPIO(cfg);

    xTaskCreate(App::Switch::ManualButtonTask,
                "manual task",
                8192,
                (void*)this,
                0,
                &this->manual_button_task_handler);

    this->manual_button_state = this->manual_button_gpio->Get();
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

void App::Switch::Act(bool is_changing_value) {
    ESP_LOGI(TAG, "Act");
    if(is_changing_value)
        this->sw_status = !this->sw_status;
    switch_gpio->Set(this->sw_status ? switch_active_state : HAL::GPIO::gpio_state_t(!switch_active_state));
    /* send message when the remote is ok */
    if(this->inited)
        update_status(this->wifi_mesh, this->status_topic, this->sw_status);
}

void App::Switch::Init() {
    HomeAssistant::Init();

    cJSON_AddStringToObject(this->discovery_content, "command_topic", this->command_topic);
    cJSON_AddBoolToObject(this->discovery_content, "optimistic", false);

    this->Discovery();

    this->wifi_mesh->Subscribe(this->command_topic);
}

void App::Switch::ManualButtonTask(void *arg) {
    auto *sw = (App::Switch*)arg;

    for(;;) {
        if(sw->manual_button_state != sw->manual_button_gpio->Get()) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            if(sw->manual_button_state != sw->manual_button_gpio->Get()) {
                sw->Act();
                sw->manual_button_state = sw->manual_button_gpio->Get();
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}




