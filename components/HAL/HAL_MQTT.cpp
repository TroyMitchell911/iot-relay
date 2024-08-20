//
// Created by troy on 2024/8/13.
//

#include <esp_log.h>
#include "HAL_MQTT.h"

#define TAG "[HAL::MQTT]"

HAL::MQTT::MQTT(const char *uri) : MQTT(uri, nullptr, nullptr, nullptr) {

}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd) : MQTT(uri, username, pwd, nullptr) {

}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd, const char *ca) {
    this->mqtt_msg_queue = xQueueCreate(MQTT_QUEUE_MSG_MAX, sizeof(HAL::MQTT::msg_t));
    xTaskCreate(HAL::MQTT::SendTask,
                "mesh send",
                8192,
                (void*)this,
                0,
                &this->mqtt_send_task_handler);

    this->mqtt_cfg.credentials.username = username;
    this->mqtt_cfg.credentials.authentication.password = pwd;
    this->mqtt_cfg.broker.address.uri = uri;
    this->mqtt_cfg.broker.verification.certificate = (const char*) ca;
    this->mqtt_cfg.task.stack_size = 8192;
}

HAL::MQTT::~MQTT() {
    vTaskDelete(this->mqtt_send_task_handler);
}

void HAL::MQTT::Subscribe(const char *topic, uint8_t qos) {
    esp_mqtt_client_subscribe(this->mqtt_client, topic, qos);
}

void HAL::MQTT::Unsubscirbe(const char *topic) {
    esp_mqtt_client_unsubscribe(this->mqtt_client, topic);
}

void HAL::MQTT::BindingCallback(HAL::MQTT::callback_t cb, uint32_t id, void *arg) {
    if(id >= HAL::MQTT::EVENT_MAX)
        return;

    for (auto it = this->callback.begin(); it != this->callback.end(); it++) {
        if(it->callback == cb) {
            return;
        }
    }

    s_callback_t s_callback = {.callback = cb, .event_mask = id, .arg = arg};
    this->callback.push_back(s_callback);
}

void HAL::MQTT::AttachEvent(HAL::MQTT::callback_t cb, uint32_t id) {
    if(id >= HAL::MQTT::EVENT_MAX)
        return;

    for (auto & it : this->callback) {
        if(it.callback == cb) {
            it.event_mask |= id;
            return;
        }
    }
}

void HAL::MQTT::Publish(HAL::MQTT::msg_t &msg) {
    ESP_LOGD(TAG, "msg.data: %s", msg.data);
    ESP_LOGD(TAG, "msg.topic: %s", msg.topic);
    ESP_LOGD(TAG, "msg.len: %d", msg.len);
    ESP_LOGD(TAG, "msg.qos: %d", msg.qos);
    ESP_LOGD(TAG, "msg.retain: %d", msg.retain);
    xQueueSend(this->mqtt_msg_queue, &msg, MQTT_QUEUE_WAIT_TIME_MS / portTICK_PERIOD_MS);
}

void HAL::MQTT::SendTask(void *arg) {
    auto mqtt = (HAL::MQTT*)arg;
    HAL::MQTT::msg_t msg;
    for(;;) {
        if(xQueueReceive(mqtt->mqtt_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "xQueueReceived");
            esp_mqtt_client_publish(mqtt->mqtt_client,
                                    msg.topic,
                                    msg.data,
                                    int(msg.len == 0 ? strlen(msg.data) : msg.len),
                                    msg.qos,
                                    msg.retain);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void HAL::MQTT::RunCallback(void *arg, HAL::MQTT::event_t event, void *data) {
    auto lst = (std::list<s_callback_t>*)arg;
    for (auto & it : *lst) {
        if(it.event_mask & event) {
            it.callback(event, data, it.arg);
        }
    }
}

void HAL::MQTT::EventHandle(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    auto event = (esp_mqtt_event_handle_t)event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            RunCallback(handler_args, HAL::MQTT::EVENT_CONNECTED, nullptr);
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
        case MQTT_EVENT_DATA:{
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
            HAL::MQTT::msg_t msg{};
            memcpy(msg.data, event->data, event->data_len);
            memcpy(msg.topic, event->topic, event->topic_len);
            msg.len = event->data_len;
            msg.topic_len = event->topic_len;
            RunCallback(handler_args, HAL::MQTT::EVENT_DATA, &msg);
            break;
        }
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

void HAL::MQTT::Publish(char *topic, char *data, int len) {
    this->Publish(topic, data, len, 0);
}

void HAL::MQTT::Publish(char *topic, char *data, int len, int qos) {
    this->Publish(topic, data, len, qos, 0);
}

void HAL::MQTT::Publish(char *topic, char *data, int len, int qos, int retain) {
    msg_t msg{};
    msg.len = len;
    msg.qos = qos;
    msg.retain = retain;
    strcpy(msg.topic, topic);
    strcpy(msg.data, data);
    this->Publish(msg);
}

void HAL::MQTT::Start() {
    this->mqtt_client = esp_mqtt_client_init(&this->mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(this->mqtt_client,
                                   esp_mqtt_event_id_t(ESP_EVENT_ANY_ID),
                                   HAL::MQTT::EventHandle,
                                   (void*)&this->callback);
    esp_mqtt_client_start(this->mqtt_client);
}
