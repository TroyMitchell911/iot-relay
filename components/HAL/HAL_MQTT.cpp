//
// Created by troy on 2024/8/13.
//

#include <esp_log.h>
#include "HAL_MQTT.h"

#define TAG "[HAL::MQTT]"

#define MQTT_CONNECTED_BIT  BIT0

void HAL::MQTT::Subscribe(const char *topic, uint8_t qos) {
    mqtt_topic_t mqtt_topic{};
    mqtt_topic.qos = qos;
    mqtt_topic.is_sub = true;
    strcpy(mqtt_topic.topic, topic);
    ESP_LOGI(TAG, "Subscribe %s", topic);
    xQueueSend(this->mqtt_topic_queue, &mqtt_topic, MQTT_QUEUE_WAIT_TIME_MS / portTICK_PERIOD_MS);
}

void HAL::MQTT::Unsubscirbe(const char *topic) {
    mqtt_topic_t mqtt_topic{};
    mqtt_topic.is_sub = false;
    strcpy(mqtt_topic.topic, topic);
    ESP_LOGI(TAG, "Unsubscirbe %s", topic);
    xQueueSend(this->mqtt_topic_queue, &mqtt_topic, MQTT_QUEUE_WAIT_TIME_MS / portTICK_PERIOD_MS);
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
    ESP_LOGI(TAG, "Publish: \n\tdata: %s\n\t"\
                "topic: %s\n\t"\
                "len: %d\n\t"\
                "qos: %d\n\t"\
                "retain: %d", msg.data, msg.topic, msg.len, msg.qos, msg.retain);
    xQueueSend(this->mqtt_msg_queue, &msg, MQTT_QUEUE_WAIT_TIME_MS / portTICK_PERIOD_MS);
}

[[noreturn]] void HAL::MQTT::SendTask(void *arg) {
    auto mqtt = (HAL::MQTT*)arg;
    HAL::MQTT::msg_t msg;
    EventBits_t bits = xEventGroupWaitBits(mqtt->mqtt_event_group,
                                           MQTT_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    if(bits & MQTT_CONNECTED_BIT) {
        for (;;) {
            if (xQueueReceive(mqtt->mqtt_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
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
}

void HAL::MQTT::RunCallback(void *arg, HAL::MQTT::event_t event, void *data) {
    auto lst = (std::list<s_callback_t>*)arg;
    for (auto & it : *lst) {
        if(it.event_mask & event) {
            ESP_LOGI(TAG, "Event id: 0x%x", event);
            it.callback(event, data, it.arg);
        }
    }
}

void HAL::MQTT::EventHandle(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    auto event = (esp_mqtt_event_handle_t)event_data;
    auto *mqtt = (HAL::MQTT*)handler_args;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            if(mqtt->status_led)
                mqtt->status_led->Set(mqtt->status_led_activate);
            xEventGroupSetBits(mqtt->mqtt_event_group, MQTT_CONNECTED_BIT);
            RunCallback((void*)&mqtt->callback, HAL::MQTT::EVENT_CONNECTED, nullptr);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            if(mqtt->status_led)
                mqtt->status_led->Set((GPIO::gpio_state_t )!mqtt->status_led_activate);
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
            RunCallback((void*)&mqtt->callback, HAL::MQTT::EVENT_DATA, &msg);
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
    if(this->started)
        return;

    ESP_LOGI(TAG, "Start");
    if(!this->mqtt_client) {

        /* First start */
        this->mqtt_client = esp_mqtt_client_init(&this->mqtt_cfg);
        /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
        esp_mqtt_client_register_event(this->mqtt_client,
                                       esp_mqtt_event_id_t(ESP_EVENT_ANY_ID),
                                       HAL::MQTT::EventHandle,
                                       (void*)this);
    }

    esp_mqtt_client_start(this->mqtt_client);

    if(!this->mqtt_event_group)
        this->mqtt_event_group = xEventGroupCreate();

    if(!this->mqtt_send_task_handler)
        xTaskCreate(HAL::MQTT::SendTask,
                    "mesh send",
                    8192,
                    (void*)this,
                    0,
                    &this->mqtt_send_task_handler);
    else
        vTaskResume(this->mqtt_send_task_handler);

    if(!this->mqtt_topic_task_handler)
        xTaskCreate(HAL::MQTT::SubTask,
                    "mesh send",
                    2048,
                    (void*)this,
                    0,
                    &this->mqtt_topic_task_handler);
    else
        vTaskResume(this->mqtt_topic_task_handler);

    this->started = true;

}

void HAL::MQTT::Stop() {
    if(!this->started)
        return;

    ESP_LOGW(TAG, "Stop");
    if(mqtt_client)
        esp_mqtt_client_stop(this->mqtt_client);
    if(this->mqtt_send_task_handler)
        vTaskSuspend(this->mqtt_send_task_handler);
    if(this->mqtt_topic_task_handler)
        vTaskSuspend(this->mqtt_topic_task_handler);

    this->started = false;
}

void HAL::MQTT::SubTask(void *arg) {
    auto mqtt = (HAL::MQTT*)arg;
    mqtt_topic_t mqtt_topic;
    EventBits_t bits = xEventGroupWaitBits(mqtt->mqtt_event_group,
                                           MQTT_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    if(bits & MQTT_CONNECTED_BIT) {
        for(;;) {
            if(xQueueReceive(mqtt->mqtt_topic_queue, &mqtt_topic, portMAX_DELAY) == pdTRUE) {
                if(mqtt_topic.is_sub)
                    esp_mqtt_client_subscribe(mqtt->mqtt_client, mqtt_topic.topic, mqtt_topic.qos);
                else
                    esp_mqtt_client_unsubscribe(mqtt->mqtt_client, mqtt_topic.topic);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

HAL::MQTT::MQTT(const char *uri) : MQTT(uri, nullptr, nullptr, nullptr) {

}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd) : MQTT(uri, username, pwd, nullptr) {

}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd, const char *ca) {
    this->mqtt_msg_queue = xQueueCreate(MQTT_QUEUE_MSG_MAX, sizeof(HAL::MQTT::msg_t));
    this->mqtt_topic_queue = xQueueCreate(MQTT_QUEUE_MSG_MAX, sizeof(mqtt_topic_t));

    this->mqtt_cfg.credentials.username = username;
    this->mqtt_cfg.credentials.authentication.password = pwd;
    this->mqtt_cfg.broker.address.uri = uri;
    this->mqtt_cfg.broker.verification.certificate = (const char*) ca;
    this->mqtt_cfg.task.stack_size = 8192;
    this->mqtt_cfg.session.keepalive = 60;
}

HAL::MQTT::~MQTT() {
    if(this->mqtt_send_task_handler)
        vTaskDelete(this->mqtt_send_task_handler);
    if(this->mqtt_msg_queue)
        vQueueDelete(this->mqtt_msg_queue);
    if(this->mqtt_topic_task_handler)
        vTaskDelete(this->mqtt_topic_task_handler);
    if(this->mqtt_topic_queue)
        vQueueDelete(this->mqtt_topic_queue);
}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd, const char *ca, int gpio_num,
                HAL::GPIO::gpio_state_t activate_state) :
        MQTT(uri, username, pwd, ca){
    this->InitLed(gpio_num, activate_state);
}

HAL::MQTT::MQTT(const char *uri, int gpio_num, HAL::GPIO::gpio_state_t activate_state) :
        MQTT(uri) {
    this->InitLed(gpio_num, activate_state);
}

HAL::MQTT::MQTT(const char *uri, const char *username, const char *pwd, int gpio_num,
                HAL::GPIO::gpio_state_t activate_state) {
    this->InitLed(gpio_num, activate_state);
}

void HAL::MQTT::InitLed(int gpio_num, HAL::GPIO::gpio_state_t activate_state) {
    this->status_led_activate = HAL::GPIO::gpio_state_t(activate_state);

    HAL::GPIO::gpio_cfg_t cfg{};
    cfg.pin = gpio_num;
    if(this->status_led_activate == HAL::GPIO::GPIO_STATE_HIGH)
        cfg.pull_down = 1;
    else if(this->status_led_activate == HAL::GPIO::GPIO_STATE_LOW)
        cfg.pull_up = 1;
    cfg.direction = HAL::GPIO::GPIO_OUTPUT;
    cfg.mode = HAL::GPIO::GPIO_PP;
    this->status_led = new HAL::GPIO(cfg);


    this->status_led->Set(HAL::GPIO::gpio_state_t(!this->status_led_activate));
}
