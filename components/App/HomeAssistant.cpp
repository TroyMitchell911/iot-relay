//
// Created by troy on 2024/8/13.
//

#include <esp_log.h>
#include "HomeAssistant.h"

#define TAG "[App::HomeAssistant]"

static const char *type2str[] = {"light", "switch", "binary_sensor", "sensor"};

static const char *g_prefix = "homeassistant";

void App::HomeAssistant::Init() {
    char buffer[32];

    this->GetTopic(this->status_topic, "status");

    if(this->entity_discovery) {
        this->discovery_topic = (char*)malloc(MQTT_TOPIC_MAX_NUM);

        memset(this->discovery_topic, 0, MQTT_TOPIC_MAX_NUM);
        sprintf(buffer, "%s-%s", this->entity_where, this->entity_name);
        sprintf(this->discovery_topic, "%s/%s/%s/config", g_prefix, type2str[this->entity_type], buffer);
        ESP_LOGI(TAG, "discovery_topic: %s", this->discovery_topic);

        this->discovery_content = cJSON_CreateObject();
        cJSON_AddStringToObject(this->discovery_content, "name", this->entity_name);
        cJSON_AddStringToObject(this->discovery_content, "device_class", type2str[this->entity_type]);
        cJSON_AddStringToObject(this->discovery_content, "state_topic", this->status_topic);
        cJSON_AddStringToObject(this->discovery_content, "unique_id", buffer);

        /* just need to judge it */
        if(this->device_identifiers) {
            cJSON *identifiers_json = cJSON_CreateArray();
            cJSON_AddItemToArray(identifiers_json, cJSON_CreateString(this->device_identifiers));
            cJSON *device_info = cJSON_CreateObject();
            cJSON_AddItemToObject(device_info, "identifiers", identifiers_json);
            cJSON_AddStringToObject(device_info, "name", this->device_name);
            cJSON_AddStringToObject(device_info, "model", this->device_model);
            cJSON_AddStringToObject(device_info, "manufacturer", this->device_manufacturer);
            cJSON_AddItemToObject(this->discovery_content, "device", device_info);
            ESP_LOGI(TAG, "%s", cJSON_Print(this->discovery_content));
        }

        wifi_mesh->Subscribe((char*)this->online_topic, 0);
        wifi_mesh->Subscribe((char*)this->offline_topic, 0);
    }
}

App::HomeAssistant::HomeAssistant(HAL::WiFiMesh *mesh, const char *where, entity_type_t type, const char *name)
                    :  HomeAssistant(mesh, where, type, name, true) {

}

App::HomeAssistant::HomeAssistant(HAL::WiFiMesh *mesh, const char *where, entity_type_t type, const char *name, bool discovery) {
    if(type >= ENTITY_TYPE_MAX)
        return;
    this->wifi_mesh = mesh;
    this->wifi_mesh->BindingCallback(App::HomeAssistant::Process,
                                     HAL::WiFiMesh::EVENT_DATA | HAL::WiFiMesh::EVENT_UPLOAD_DEVICE_INFO,
                                     (void*)this);

    this->entity_where = where;
    this->entity_name = name;
    this->entity_type = type;
    this->entity_discovery = discovery;
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
}

void App::HomeAssistant::Prefix(const char *prefix) {
    g_prefix = prefix;
}

void App::HomeAssistant::GetTopic(char *dst, const char *suffix) {
    sprintf(dst, "%s/%s/%s/%s", this->entity_where, type2str[this->entity_type], this->entity_name, suffix);
}

void App::HomeAssistant::Process(HAL::WiFiMesh::event_t event, void *data, void *arg) {
    auto *ha = (App::HomeAssistant *) arg;
    auto *mesh_msg = (HAL::WiFiMesh::msg_t *)data;
    if (event == HAL::WiFiMesh::EVENT_DATA) {
        HAL::MQTT::msg_t msg{};
        auto *r_msg = (HAL::MQTT::msg_t *) data;

        if (strncmp(ha->online_topic, r_msg->topic, r_msg->topic_len) == 0) {
            if (strncmp(ha->online_content, r_msg->data, r_msg->len) == 0) {
                ESP_LOGI(TAG, "online");
                strcpy(msg.topic, ha->discovery_topic);
                strcpy(msg.data, cJSON_Print(ha->discovery_content));
                msg.qos = 0;

                ha->wifi_mesh->Publish(&msg, sizeof(HAL::MQTT::msg_t), HAL::WiFiMesh::MSG_MQTT);
            }
        } else if (strncmp(ha->offline_topic, r_msg->topic, r_msg->topic_len) == 0) {
            if (strncmp(ha->offline_content, r_msg->data, r_msg->len) == 0) {
                ESP_LOGI(TAG, "offline");
            }
        }
    } else if(event == HAL::WiFiMesh::EVENT_UPLOAD_DEVICE_INFO) {
        ESP_LOGI(TAG, "I need to upload self info");
        /* upload self info so that the root can send
        * message to this device through some topic that this device has */
        /* mesh rely on mac address to send */
        memcpy((uint8_t*)&ha->self_info.mac, mesh_msg->data, sizeof(mesh_addr_t));
        ha->GetTopic(ha->self_info.unique_id, "");
        ha->wifi_mesh->Publish( &ha->self_info, sizeof(HAL::WiFiMesh::device_info_t), HAL::WiFiMesh::MSG_UPLOAD_DEVICE_INFO);
    }
}

void App::HomeAssistant::Discovery() {
    if(!this->entity_discovery)
        return;

    HAL::MQTT::msg_t msg{};
    strcpy(msg.topic, this->discovery_topic);
    strcpy(msg.data, cJSON_Print(this->discovery_content));
    this->wifi_mesh->Publish(&msg, sizeof(HAL::MQTT::msg_t), HAL::WiFiMesh::MSG_MQTT);

    this->inited = true;
}

App::HomeAssistant::HomeAssistant(HAL::WiFiMesh *mesh,
                                  const char *where,
                                  App::HomeAssistant::entity_type_t type,
                                  const char *name,
                                  const char *device_identifiers,
                                  const char *device_name,
                                  const char *device_model,
                                  const char *device_manufacturer)
                                  :HomeAssistant(mesh, where, type, name) {
    this->device_identifiers = device_identifiers;
    this->device_manufacturer = device_manufacturer;
    this->device_model = device_model;
    this->device_name = device_name;
}


