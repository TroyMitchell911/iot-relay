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

#define TAG "[main]"

static HAL::MQTT *mqtt;
static App::Switch *sw;
static HAL::GPIO::gpio_state_t key_state;

static void mqtt_event(HAL::MQTT::event_t event, void *data, void *arg) {
    if(event == HAL::MQTT::EVENT_CONNECTED) {
        sw->Init();
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
    }
}

extern "C" {
__attribute__((noreturn)) void app_main(void) {
    ESP_LOGI(TAG, "This a iot relay device");

    HAL::Init();

    HAL::GPIO::gpio_cfg_t cfg;
    cfg.pin = GPIO_NUM_20;
    cfg.pull_up = 1;
    cfg.direction = HAL::GPIO::GPIO_INPUT;
    auto key = new HAL::GPIO(cfg);

    key_state = key->Get();

    HAL::WiFiMesh &mesh = HAL::WiFiMesh::GetInstance();
    sw = new App::Switch(&mesh, CONFIG_DEVICE_WHERE,
                         CONFIG_DEVICE_NAME,
                         CONFIG_SWITCH_GPIO_NUM,
                         CONFIG_SWITCH_ACTIVE_STATE);
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
        if(key->Get() != key_state) {

            key_state = key->Get();
            sw->Act();
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
}