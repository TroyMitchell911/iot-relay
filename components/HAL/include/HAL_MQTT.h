//
// Created by troy on 2024/8/13.
//

#ifndef RELAY_HAL_MQTT_H
#define RELAY_HAL_MQTT_H

#include <mqtt_client.h>
#include <list>
#include "HAL_GPIO.h"

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

        typedef struct {
            char topic[MQTT_TOPIC_MAX_NUM];
            int qos : 1;
            bool is_sub;
        }mqtt_topic_t;

    protected:
        HAL::GPIO::gpio_state_t status_led_activate;
        HAL::GPIO *status_led;

    private:
        esp_mqtt_client_config_t mqtt_cfg{};
        esp_mqtt_client_handle_t mqtt_client = nullptr;
        std::list<s_callback_t> callback;
        TaskHandle_t mqtt_send_task_handler = nullptr;
        TaskHandle_t mqtt_topic_task_handler = nullptr;
        QueueHandle_t mqtt_msg_queue = nullptr;
        QueueHandle_t mqtt_topic_queue = nullptr;
        EventGroupHandle_t mqtt_event_group = nullptr;
        bool started = false;



    private:
        static void EventHandle(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
        static void RunCallback(void *arg, HAL::MQTT::event_t event, void *data);

        [[noreturn]] static void SendTask(void *arg);
        static void SubTask(void *arg);

        void InitLed(int gpio_num, GPIO::gpio_state_t  activate_state);

    public:
        MQTT(const char *uri);
        MQTT(const char *uri, const char *username, const char *pwd);
        MQTT(const char *uri, const char *username, const char *pwd, const char *ca);
        MQTT(const char *uri, int gpio_num, GPIO::gpio_state_t activate_state);
        MQTT(const char *uri, const char *username, const char *pwd, int gpio_num, GPIO::gpio_state_t activate_state);
        MQTT(const char *uri, const char *username, const char *pwd, const char *ca, int gpio_num, GPIO::gpio_state_t activate_state);
        void Start();
        void Stop();
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
