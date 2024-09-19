//
// Created by troy on 2024/8/14.
//

#ifndef RELAY_HAL_WIFIMESH_H
#define RELAY_HAL_WIFIMESH_H

#include <esp_wifi.h>
#include <esp_mesh.h>
#include <esp_log.h>
#include <esp_mac.h>
#include "HAL_MQTT.h"

namespace HAL {
    class WiFiMesh{
    private:
#define MESH_QUEUE_MSG_MAX          10
#define MESH_QUEUE_WAIT_TIME_MS     500
#define MESH_SEND_DATA_SIZE         1024
    public:
        typedef struct {
            uint8_t mesh_channel;
            uint8_t mesh_id[6];
            const char *mesh_ap_pwd;
            uint8_t max_connections;
            int max_layer;

            const char *router_ssid;
            const char *router_pwd;
        }cfg_t;

        typedef enum {
            EVENT_GOT_IP = 0x01,
            EVENT_DATA = EVENT_GOT_IP << 1,
            EVENT_ROOT_WILL_CHANGE = EVENT_DATA << 1,
            EVENT_UPLOAD_DEVICE_INFO = EVENT_ROOT_WILL_CHANGE << 1,
            EVENT_CONNECTED = EVENT_UPLOAD_DEVICE_INFO << 1,
            EVENT_MAX = EVENT_CONNECTED << 1,
        }event_t;

        typedef struct {
            mesh_addr_t mac;
            /* This unique ID should be generated based on the topic.
             * For example,
             * if you have a light and its command theme is 'main-room/switch/ceiling-light/command',
             * then your unique ID should be 'main-room/switch/ceiling-light/'
             * */
            char unique_id[MQTT_TOPIC_MAX_NUM];
        }device_info_t;

        typedef enum {
            MSG_MQTT = 0,
            MSG_UPLOAD_DEVICE_INFO,
            MSG_SUBSCRIBE,
            MSG_UNSUBSCRIBE,
            MSG_TYPE_MAX,
        }msg_type_t;

        typedef struct {
            char data[MESH_SEND_DATA_SIZE];
            size_t len;
            msg_type_t type;
            mesh_addr_t *mac;
        }msg_t;

        typedef void (*callback_t)(event_t event, void *data, void *arg);

    private:
        typedef struct {
            callback_t callback;
            uint32_t event_mask;
            void *arg;
            void *pthis;
        }s_callback_t;

        typedef struct {
            char topic[MQTT_TOPIC_MAX_NUM];
            uint8_t qos;
        }subscribe_msg_t;

        std::list<s_callback_t> callback;
        TaskHandle_t mesh_send_task_handler = nullptr;
        TaskHandle_t mesh_recv_task_handler = nullptr;
        QueueHandle_t mesh_msg_queue = nullptr;

        int mesh_layer = -1;
        mesh_addr_t mesh_parent_addr{};
        esp_netif_t *netif_sta = nullptr;
        bool is_mesh_connected = false;
        HAL::MQTT* mqtt = nullptr;
        std::list<device_info_t*> device_info_table;

        mesh_addr_t root_mac;

    protected:
        HAL::GPIO::gpio_state_t status_led_activate;
        HAL::GPIO *status_led = nullptr;

    private:
        static void WiFiEventHandle(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data);
        static void MQTTEventHandle(HAL::MQTT::event_t event, void *data, void *arg);
        static void MeshEventHandle(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);
        static void RunCallback(void *arg, HAL::WiFiMesh::event_t event, void *data);
        void Broadcast(void *data, size_t size, msg_type_t type, bool to_root);
        void Broadcast(void *data, size_t size, msg_type_t type);

        [[noreturn]] static void SendTask(void *arg);
        [[noreturn]] static void RecvTask(void *arg);

    public:
        WiFiMesh() = default;
        ~WiFiMesh() = default;
        WiFiMesh(const WiFiMesh &wifi_mesh) = delete;
        const WiFiMesh &operator=(const WiFiMesh &wifi_mesh) = delete;

    public:
        static HAL::WiFiMesh &GetInstance();
        void SetStatusLed(int gpio_num, GPIO::gpio_state_t activate_state);
        void Start(cfg_t *config);
        void SetMQTT(HAL::MQTT* mqtt_client);
        HAL::MQTT &GetMQTT();
        void BindingCallback(callback_t cb, void *arg);
        void BindingCallback(callback_t cb, uint32_t event, void *arg);
        void AttachEvent(callback_t cb, uint32_t event);
        void Publish(msg_t &msg);
        void Publish(void *data, size_t size, msg_type_t type, mesh_addr_t *mac);
        void Publish(void *data, size_t size, msg_type_t type);
        void Subscribe(char *topic);
        void Subscribe(char *topic, uint8_t qos);
        void Unsubscribe(char *topic);
    };
}



#endif //RELAY_HAL_WIFIMESH_H
