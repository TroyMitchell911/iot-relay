//
// Created by troy on 2024/9/3.
//

#include "BinarySensor.h"

static const char *type2str[] = {"presence"};

App::BinarySensor::BinarySensor(HAL::WiFiMesh *mesh,
                                const char *where,
                                const char *name,
                                int gpio_num,
                                int active_state,
                                sensor_type_t type)
                                : App::HomeAssistant(mesh, where, App::HomeAssistant::BINARY_SENSOR, name){

    this->sensor_type = type;

    HAL::GPIO::gpio_cfg_t cfg{};
    cfg.pin = gpio_num;
    if(this->active_state == HAL::GPIO::GPIO_STATE_HIGH)
        cfg.pull_down = 1;
    else if(this->active_state == HAL::GPIO::GPIO_STATE_LOW)
        cfg.pull_up = 1;
    cfg.direction = HAL::GPIO::GPIO_INPUT;
    cfg.mode = HAL::GPIO::GPIO_PP;
    this->gpio = new HAL::GPIO(cfg);

    this->gpio_state = this->gpio->Get();

    xTaskCreate(App::BinarySensor::DetectTask,
                "detect task",
                8192,
                (void*)this,
                0,
                &this->detect_task_handler);
}

void App::BinarySensor::Init() {
    HomeAssistant::Init();

    cJSON_ReplaceItemInObject(this->discovery_content,
                              "device_class",
                              cJSON_CreateString(type2str[int(this->sensor_type)]));
    cJSON_AddStringToObject(this->discovery_content, "payload_on", "ON");
    cJSON_AddStringToObject(this->discovery_content, "payload_off", "OFF");

    this->Discovery();

    /* Update the first value to avoid the ha displays `unknown` */
    this->Update();
}

[[noreturn]] void App::BinarySensor::DetectTask(void *arg) {
    auto *bs = (App::BinarySensor*)arg;

    for(;;) {
        if(bs->gpio_state != bs->gpio->Get()) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            if(bs->gpio_state != bs->gpio->Get()) {
                bs->gpio_state = bs->gpio->Get();
                bs->Update();
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void App::BinarySensor::Update() {
    if(!this->inited)
        return;

    HAL::MQTT::msg_t msg{};

    strcpy(msg.data, this->gpio_state == this->active_state ? "ON" : "OFF");
    strcpy(msg.topic, status_topic);
    msg.retain = 0;
    msg.qos = 0;

    this->wifi_mesh->Publish(&msg, sizeof(HAL::MQTT::msg_t), HAL::WiFiMesh::MSG_MQTT);
}
