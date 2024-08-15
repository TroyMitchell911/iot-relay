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
            EVENT_DATA = EVENT_GOT_IP << 1,
            EVENT_ROOT_WILL_CHANGE = EVENT_DATA << 1,
            EVENT_MAX = EVENT_ROOT_WILL_CHANGE << 1,
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
        TaskHandle_t mesh_send_task_handler;
        TaskHandle_t mesh_recv_task_handler;
        QueueHandle_t mesh_msg_queue;

    private:
        static void IPEventHandle(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data);
        static void MQTTEventHandle(HAL::MQTT::event_t event, void *data, void *arg);
        static void MeshEventHandle(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);
        static void RunCallback(void *arg, HAL::WiFiMesh::event_t event, void *data);

        [[noreturn]] static void SendTask(void *arg);
        [[noreturn]] static void RecvTask(void *arg);

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
        void Publish(void *data, size_t len);
    };
}



#endif //RELAY_HAL_WIFIMESH_H
