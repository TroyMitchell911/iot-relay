//
// Created by troy on 2024/8/13.
//

#include <esp_log.h>
#include "HAL_MQTT.h"

#define TAG "[HAL::MQTT]"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    auto event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno));
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGE(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGW(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void mqtt_init(esp_mqtt_client_config_t *cfg, esp_mqtt_client_handle_t *mqtt_client) {
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(cfg);
    *mqtt_client = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, esp_mqtt_event_id_t(ESP_EVENT_ANY_ID), mqtt_event_handler, nullptr);
    esp_mqtt_client_start(client);
}

HAL::MQTT::MQTT(const char *uri) {
    this->mqtt_cfg.broker.address.uri = uri;
    mqtt_init(&this->mqtt_cfg, &this->mqtt_client);
}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd) {
    this->mqtt_cfg.credentials.username = username;
    this->mqtt_cfg.credentials.authentication.password = pwd;
    this->mqtt_cfg.broker.address.uri = uri;
    mqtt_init(&this->mqtt_cfg, &this->mqtt_client);
}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd, const char *ca) {
    this->mqtt_cfg.credentials.username = username;
    this->mqtt_cfg.credentials.authentication.password = pwd;
    this->mqtt_cfg.broker.address.uri = uri;
    this->mqtt_cfg.broker.verification.certificate = (const char*) ca;
    mqtt_init(&this->mqtt_cfg, &this->mqtt_client);
}

HAL::MQTT::~MQTT() {

}

void HAL::MQTT::Subscribe(const char *topic, uint8_t qos) {
    esp_mqtt_client_subscribe(this->mqtt_client, topic, qos);
}

void HAL::MQTT::Unsubscirbe(const char *topic) {
    esp_mqtt_client_unsubscribe(this->mqtt_client, topic);
}

int HAL::MQTT::Publish(const char *topic, const char *str, int qos, int retain) {
    return esp_mqtt_client_publish(this->mqtt_client, topic, str, strlen(str), qos, retain);
}

int HAL::MQTT::Publish(const char *topic, const char *data, int len, int qos, int retain) {
    return esp_mqtt_client_publish(this->mqtt_client, topic, data, len, qos, retain);
}
