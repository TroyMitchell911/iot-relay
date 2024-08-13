//
// Created by troy on 2024/8/13.
//

#ifndef RELAY_HAL_MQTT_H
#define RELAY_HAL_MQTT_H

#include <mqtt_client.h>

namespace HAL {
    class MQTT {
    private:
        esp_mqtt_client_config_t mqtt_cfg{};
    public:
        MQTT(const char *uri);
        MQTT(const char *uri, const char *username, const char *pwd);
        MQTT(const char *uri, const char *username, const char *pwd, const char *ca);
        int Subscribe(const char *topic, uint8_t qos);
        int Unsubscirbe(const char *topic);
        int Publish(const char *topic, const char *str, int qos, int retain);
        int Publish(const char *topic, const char *data, int len, int qos, int retain);
        ~MQTT();
    };
}



#endif //RELAY_HAL_MQTT_H
