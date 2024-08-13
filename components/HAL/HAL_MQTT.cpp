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
    auto callback = *((HAL::MQTT::callback_t*)handler_args);
    uint32_t callback_event_mask = *(uint32_t*)((uint32_t)handler_args + sizeof(HAL::MQTT::callback_t));

    printf("callback: %p, event_mask: %ld\n", callback, callback_event_mask);

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            if((callback_event_mask & HAL::MQTT::EVENT_CONNECTED) && callback) {
                callback(HAL::MQTT::EVENT_CONNECTED, nullptr);
            }
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
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
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

static void mqtt_init(esp_mqtt_client_config_t *cfg, esp_mqtt_client_handle_t *mqtt_client, void *arg) {
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(cfg);
    *mqtt_client = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, esp_mqtt_event_id_t(ESP_EVENT_ANY_ID), mqtt_event_handler, arg);
    esp_mqtt_client_start(client);
}

HAL::MQTT::MQTT(const char *uri, callback_t cb) {
    this->mqtt_cfg.broker.address.uri = uri;
    this->callback.callback = cb;
    mqtt_init(&this->mqtt_cfg, &this->mqtt_client, (void*)&this->callback);
}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd, callback_t cb) {
    this->mqtt_cfg.credentials.username = username;
    this->mqtt_cfg.credentials.authentication.password = pwd;
    this->mqtt_cfg.broker.address.uri = uri;

    this->callback.callback = cb;
    mqtt_init(&this->mqtt_cfg, &this->mqtt_client, (void*)&this->callback);
}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd, const char *ca, callback_t cb) {
    this->mqtt_cfg.credentials.username = username;
    this->mqtt_cfg.credentials.authentication.password = pwd;
    this->mqtt_cfg.broker.address.uri = uri;
    this->mqtt_cfg.broker.verification.certificate = (const char*) ca;

    this->callback.callback = cb;
    mqtt_init(&this->mqtt_cfg, &this->mqtt_client, (void*)&this->callback);
}

HAL::MQTT::~MQTT() {

}

void HAL::MQTT::Subscribe(const char *topic, uint8_t qos) {
    esp_mqtt_client_subscribe(this->mqtt_client, topic, qos);
}

void HAL::MQTT::Unsubscirbe(const char *topic) {
    esp_mqtt_client_unsubscribe(this->mqtt_client, topic);
}

int HAL::MQTT::Publish(const char *topic, HAL::MQTT::msg_t &msg) {
    return esp_mqtt_client_publish(this->mqtt_client,
                                   topic,
                                   msg.data,
                                   int(msg.len == 0 ? strlen(msg.data) : msg.len),
                                   msg.qos,
                                   msg.retain);
}

void HAL::MQTT::BindingEvent(uint32_t id) {
    if(id < HAL::MQTT::EVENT_MAX) {
        this->callback.event_mask |= id;
    }
}
