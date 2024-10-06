//
// Created by troy on 2024/10/6.
//

#include "Sensor.h"

#define TAG "[App::Sensor]"

static const char *sensor_device_class_str[32] = {
        "battery",              // SENSOR_BATTERY
        "humidity",             // SENSOR_HUMIDITY
        "illuminance",          // SENSOR_ILLUMINANCE
        "temperature",          // SENSOR_TEMPERATURE
        "pressure",             // SENSOR_PRESSURE
        "power",                // SENSOR_POWER
        "energy",               // SENSOR_ENERGY
        "current",              // SENSOR_CURRENT
        "voltage",              // SENSOR_VOLTAGE
        "signal_strength",      // SENSOR_SIGNAL_STRENGTH
        "timestamp",           // SENSOR_TIMESTAMP
};

void App::Sensor::Init() {
    char buf[128];

    HomeAssistant::Init();

    sprintf(buf, "{{ value_json.%s }}", this->info.value_name);
    cJSON_AddStringToObject(this->discovery_content, "value_template", buf);
    cJSON_AddNumberToObject(this->discovery_content, "suggested_display_precision", 1);
    cJSON_AddStringToObject(this->discovery_content, "device_class", sensor_device_class_str[int(info.device_class)]);

    if(this->info.unit_of_measurement)
        cJSON_AddStringToObject(this->discovery_content, "unit_of_measurement", this->info.unit_of_measurement);

    printf("%s\n", cJSON_Print(this->discovery_content));
    this->Discovery();
}

App::Sensor::Sensor(HAL::WiFiMesh *mesh, const char *where, const char *name, App::Sensor::sensor_info_t info,
                    unsigned int interval)
        : HomeAssistant(mesh, where, App::HomeAssistant::SENSOR, name) {
    this->info = info;
    this->update_interval = interval;

    xTaskCreate(App::Sensor::UpdateTask,
                "update task",
                8192,
                (void*)this,
                0,
                &this->update_task_handler);

    this->data_json = cJSON_CreateObject();

    cJSON_AddNumberToObject(this->data_json, this->info.value_name, 0);
}

App::Sensor::~Sensor() {

}

void App::Sensor::UpdateTask(void *arg) {
    auto *sensor = (App::Sensor*)arg;

    for(;;) {
        if(!sensor->inited || !sensor->update_func)
            goto fail;

        sensor->data_num = sensor->update_func(&sensor->data);

        if(!sensor->data_num || !sensor->data)
            goto fail;

        App::Sensor::Update(sensor);

        vTaskDelay(sensor->update_interval / portTICK_PERIOD_MS);
        continue;
fail:
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void App::Sensor::Update(App::Sensor *sensor) {
    cJSON *item;
    HAL::MQTT::msg_t msg{};
    item = cJSON_GetObjectItem(sensor->data_json, sensor->info.value_name);
    cJSON_SetNumberValue(item, sensor->data->value);
    ESP_LOGI(TAG, "%s", cJSON_Print(sensor->data_json));

    strcpy(msg.topic, sensor->status_topic);
    strcpy(msg.data, cJSON_Print(sensor->data_json));
    msg.retain = 0;
    msg.qos = 0;
    ESP_LOGI(TAG, "msg.size:%d\n", sizeof(HAL::MQTT::msg_t));
    sensor->wifi_mesh->Publish(&msg, sizeof(HAL::MQTT::msg_t), HAL::WiFiMesh::MSG_MQTT);
}

void App::Sensor::BindUpdate(App::Sensor::update_func_t func) {
    this->update_func = func;
}
