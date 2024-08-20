//
// Created by troy on 2024/8/13.
//

#ifndef RELAY_HAL_MQTT_H
#define RELAY_HAL_MQTT_H

#include <mqtt_client.h>
#include <list>

namespace HAL {
    class MQTT {
#define MQTT_QUEUE_MSG_MAX          10
#define MQTT_QUEUE_WAIT_TIME_MS    500
#define MQTT_TOPIC_MAX_NUM          64
    public:
        typedef enum {
            EVENT_CONNECTED = 0x01,
            EVENT_DISCONNECTED = EVENT_CONNECTED << 1,
            EVENT_SUBSCRIBED = EVENT_DISCONNECTED << 1,
            EVENT_UNSUBSCRIBED = EVENT_SUBSCRIBED << 1,
            MQTT_EVENT_PUBLISHED = EVENT_UNSUBSCRIBED << 1,
            EVENT_DATA = MQTT_EVENT_PUBLISHED << 1,
            EVENT_MAX = EVENT_DATA << 1,
        }event_t;

        typedef struct {
            char topic[MQTT_TOPIC_MAX_NUM];
            char data[512];
            int len;
            /* ↓ just sending use ↓ */
            int qos;
            int retain;
            /* ↓ just reading use ↓ */
            int topic_len;
        }msg_t;

        typedef void (*callback_t)(event_t event, void *data, void *arg);

    private:
        typedef struct {
            callback_t callback;
            uint32_t event_mask;
            void *arg;
        }s_callback_t;

    private:
        esp_mqtt_client_config_t mqtt_cfg{};
        esp_mqtt_client_handle_t mqtt_client;
        std::list<s_callback_t> callback;
        TaskHandle_t mqtt_send_task_handler;
        QueueHandle_t mqtt_msg_queue;

    private:
        static void EventHandle(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
        static void RunCallback(void *arg, HAL::MQTT::event_t event, void *data);
        static void SendTask(void *arg);

    public:
        MQTT(const char *uri);
        MQTT(const char *uri, const char *username, const char *pwd);
        MQTT(const char *uri, const char *username, const char *pwd, const char *ca);
        void Start();
        void BindingCallback(HAL::MQTT::callback_t cb, uint32_t id, void *arg);
        void AttachEvent(HAL::MQTT::callback_t cb, uint32_t id);
        void Subscribe(const char *topic, uint8_t qos);
        void Unsubscirbe(const char *topic);
        void Publish(msg_t &msg);
        void Publish(char *topic, char *data, int len);
        void Publish(char *topic, char *data, int len, int qos);
        void Publish(char *topic, char *data, int len, int qos, int retain);
        ~MQTT();
    };
}



#endif //RELAY_HAL_MQTT_H
