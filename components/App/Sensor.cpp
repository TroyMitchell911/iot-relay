//
// Created by troy on 2024/10/6.
//

#include "Sensor.h"

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

App::Sensor::Sensor(HAL::WiFiMesh *mesh, const char *where, const char *name, App::Sensor::sensor_info_t info)
        : HomeAssistant(mesh, where, App::HomeAssistant::SENSOR, name) {
    this->info = info;
}

App::Sensor::~Sensor() {

}
