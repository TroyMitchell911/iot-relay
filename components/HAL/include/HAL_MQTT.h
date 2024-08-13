//
// Created by troy on 2024/8/13.
//

#ifndef RELAY_HAL_MQTT_H
#define RELAY_HAL_MQTT_H

#include <mqtt_client.h>

namespace HAL {
    class MQTT {
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
            const char *data;
            int len;
            /* ↓ just sending use ↓ */
            int qos;
            int retain;
        }msg_t;

        typedef void (*callback_t)(event_t event, void *data);

    private:
        typedef struct {
            callback_t callback;
            uint32_t event_mask;
        }s_callback_t;
    private:
        esp_mqtt_client_config_t mqtt_cfg{};
        esp_mqtt_client_handle_t mqtt_client;
        s_callback_t callback = {
                nullptr,
                EVENT_DATA
        };

    public:
        /* default: the callback just subscribe EVENT_DATA */
        MQTT(const char *uri, callback_t callback);
        MQTT(const char *uri, const char *username, const char *pwd, callback_t callback);
        MQTT(const char *uri, const char *username, const char *pwd, const char *ca, callback_t callback);
        void BindingEvent(uint32_t id);
        void Subscribe(const char *topic, uint8_t qos);
        void Unsubscirbe(const char *topic);
        int Publish(const char *topic, msg_t &msg);
        ~MQTT();
    };
}



#endif //RELAY_HAL_MQTT_H
