//
// Created by troy on 2024/8/14.
//

#ifndef RELAY_HAL_WIFIMESH_H
#define RELAY_HAL_WIFIMESH_H

#include "HAL_MQTT.h"

namespace HAL {
    class WiFiMesh{
    public:
        typedef struct {
            uint8_t mesh_channel;
            uint8_t mesh_id[6];
            const char *mesh_ap_pwd;
            uint8_t max_connections;
            int max_layer;

            const char *router_ssid;
            const char *router_pwd;
        }wifi_mesh_cfg_t;

        typedef enum {
            EVENT_GOT_IP = 0x01,
            EVENT_MAX = EVENT_GOT_IP << 1,
        }event_t;

        typedef void (*callback_t)(event_t event, void *data, void *arg);

    private:
        typedef struct {
            callback_t callback;
            uint32_t event_mask;
            void *arg;
            void *pthis;
        }s_callback_t;
        std::list<s_callback_t> callback;
    public:
        WiFiMesh() = default;
        ~WiFiMesh() = default;
        WiFiMesh(const WiFiMesh &wifi_mesh) = delete;
        const WiFiMesh &operator=(const WiFiMesh &wifi_mesh) = delete;

    public:
        static HAL::WiFiMesh &GetInstance();
        void Start(wifi_mesh_cfg_t *config);
        void SetMQTT(HAL::MQTT* mqtt_client);
        HAL::MQTT &GetMQTT();
        void BindingCallback(callback_t cb, void *arg);
        void BindingCallback(callback_t cb, uint32_t event, void *arg);
        void AttachEvent(callback_t cb, uint32_t event);
        void Publish(HAL::MQTT::msg_t msg);
    };
}



#endif //RELAY_HAL_WIFIMESH_H
