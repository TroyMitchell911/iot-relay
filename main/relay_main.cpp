/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <nvs_flash.h>
#include <soc/gpio_num.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_log.h>
#include <mqtt_client.h>
#include <esp_ota_ops.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "HAL_WiFi.h"
#include "HAL_MQTT.h"
#include "HomeAssistant.h"
#include "Switch.h"
#include "HAL_GPIO.h"
#include "HAL.h"
#include "BinarySensor.h"
#include "Sensor.h"

#define TAG "[main]"

static HAL::MQTT *mqtt;
static App::Switch *sw;
static App::BinarySensor *bs;
static HAL::GPIO::gpio_state_t key_state;
static App::Sensor *sensor;

static void mqtt_event(HAL::MQTT::event_t event, void *data, void *arg) {
    if(event == HAL::MQTT::EVENT_CONNECTED) {
        sw->Init();
        bs->Init();
        sensor->Init();
    }
}
#include "ca_emqx_troy_home.crt"
static void wifi_event(HAL::WiFiMesh::event_t event_id, void *event_data, void *arg) {
    if(event_id == HAL::WiFiMesh::EVENT_GOT_IP) {
        mqtt->BindingCallback(mqtt_event, HAL::MQTT::EVENT_CONNECTED, nullptr);
        mqtt->Start();
    } else if(event_id == HAL::WiFiMesh::EVENT_CONNECTED) {
        delete mqtt;
        sw->Init();
        bs->Init();
        sensor->Init();
    }
}

static void test_func(void)
{

}

static unsigned int test_update(App::Sensor::update_data_t **data)
{
    static App::Sensor::update_data_t update_data = {"humi", 1.0};

    update_data.value += 0.1;

    *data = &update_data;

    ESP_LOGI(TAG, "update data: %f\n", update_data.value);

    return 1;
}

extern "C" {
__attribute__((noreturn)) void app_main(void) {

    ESP_LOGI(TAG, "This a iot relay device");

    HAL::Init();

    test_func();

    HAL::WiFiMesh &mesh = HAL::WiFiMesh::GetInstance();
    sw = new App::Switch(&mesh, CONFIG_SWITCH_DEVICE_WHERE,
                         CONFIG_SWITCH_DEVICE_NAME,
                         CONFIG_SWITCH_GPIO_NUM,
                         CONFIG_SWITCH_ACTIVE_STATE,
                         CONFIG_SWITCH_MANUAL_BUTTON_GPIO_NUM,
                         CONFIG_SWITCH_MANUAL_BUTTON_ACTIVE_STATE);

    bs = new App::BinarySensor(&mesh, CONFIG_BODY_SENSOR_WHERE,
                         CONFIG_BODY_SENSOR_NAME,
                         CONFIG_BODY_SENSOR_GPIO_NUM,
                         CONFIG_BODY_SENSOR_ACTIVE_STATE,
                         App::BinarySensor::PRESENCE);

    App::Sensor::sensor_info_t info;
    info.device_class = App::Sensor::SENSOR_HUMIDITY;
    info.value_name = "humi";
    info.unit_of_measurement = nullptr;

    sensor = new App::Sensor(&mesh, "room", "test3", info, 1000);
    sensor->BindUpdate(test_update);

    mesh.BindingCallback(wifi_event, nullptr);
    HAL::WiFiMesh::cfg_t mesh_cfg{};
    mesh_cfg.max_connections = CONFIG_MESH_AP_CONNECTIONS;
    mesh_cfg.mesh_ap_pwd = CONFIG_MESH_AP_PASSWD;
    mesh_cfg.router_ssid = "troyself-wifi";
    mesh_cfg.router_pwd = "troy888666";
//    mesh_cfg.router_ssid = "HBDT-23F";
//    mesh_cfg.router_pwd = "hbishbis";
//    mesh_cfg.router_ssid = "troy-phone";
//    mesh_cfg.router_pwd = "jianglin998";
//    mesh_cfg.router_ssid = "jl-wifi";
//    mesh_cfg.router_pwd = "jianglin2022";
    mesh_cfg.mesh_channel = CONFIG_MESH_CHANNEL;
    mesh_cfg.max_layer = CONFIG_MESH_MAX_LAYER;
    for(unsigned char & i : mesh_cfg.mesh_id) {
        i = 0x77;
    }
    mqtt = new HAL::MQTT(CA_EMQX_TROY_HOME_URI,
                         CA_EMQX_TROY_HOME_USER,
                         CA_EMQX_TROY_HOME_PWD,
                         ca_emqx_troy_home);
    ESP_LOGI(TAG, "mqtt client pointer: %p", mqtt);
    mesh.SetMQTT(mqtt);
    mesh.Start(&mesh_cfg);

    for(;;){
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
}