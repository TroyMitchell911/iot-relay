//
// Created by troy on 2024/8/14.
//

#include "HAL_WiFiMesh.h"

#define TAG "[HAL::WiFiMesh]"

void HAL::WiFiMesh::Start(HAL::WiFiMesh::cfg_t *config) {
    xTaskCreate(HAL::WiFiMesh::SendTask,
                "mesh send",
                8192,
                this,
                0,
                &this->mesh_send_task_handler);

    xTaskCreate(HAL::WiFiMesh::RecvTask,
                "mesh recv",
                8192,
                this,
                0,
                &this->mesh_recv_task_handler);
    this->mesh_msg_queue = xQueueCreate(MESH_QUEUE_MSG_MAX, sizeof(mesh_data_t));


    /*  create network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, nullptr));
    /*  wifi initialization */
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &HAL::WiFiMesh::WiFiEventHandle,
                                               (void*)this));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               WIFI_EVENT_STA_DISCONNECTED,
                                               &HAL::WiFiMesh::WiFiEventHandle,
                                               (void*)this));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &HAL::WiFiMesh::MeshEventHandle,
                                               (void*)this));
    /*  set mesh topology */
    ESP_ERROR_CHECK(esp_mesh_set_topology(esp_mesh_topology_t(CONFIG_MESH_TOPOLOGY)));
    /*  set mesh max layer according to the topology */
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(config->max_layer));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));

#if CONFIG_MESH_ENABLE_PS
    /* Enable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_enable_ps());
    /* better to increase the associate expired time, if a small duty cycle is set. */
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(60));
    /* better to increase the announcement interval to avoid too much management traffic, if a small duty cycle is set. */
    ESP_ERROR_CHECK(esp_mesh_set_announce_interval(600, 3300));
#else
    /* Disable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_disable_ps());
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
#endif

    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    /* mesh ID */
    memcpy((uint8_t *) &cfg.mesh_id, config->mesh_id, 6);
    /* router */
    cfg.channel = config->mesh_channel;
    cfg.router.ssid_len = strlen(config->router_ssid);
    memcpy((uint8_t *) &cfg.router.ssid, config->router_ssid, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, config->router_pwd,
           strlen(config->router_pwd));
    /* mesh softAP */
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(wifi_auth_mode_t(WIFI_AUTH_WPA2_WPA3_PSK)));
    cfg.mesh_ap.max_connection = config->max_connections;
    cfg.mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, config->mesh_ap_pwd,
           strlen(config->mesh_ap_pwd));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    esp_mesh_set_self_organized(true, true);
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());

#ifdef CONFIG_MESH_ENABLE_PS
    /* set the device active duty cycle. (default:10, MESH_PS_DEVICE_DUTY_REQUEST) */
    ESP_ERROR_CHECK(esp_mesh_set_active_duty_cycle(CONFIG_MESH_PS_DEV_DUTY, CONFIG_MESH_PS_DEV_DUTY_TYPE));
    /* set the network active duty cycle. (default:10, -1, MESH_PS_NETWORK_DUTY_APPLIED_ENTIRE) */
    ESP_ERROR_CHECK(esp_mesh_set_network_duty_cycle(CONFIG_MESH_PS_NWK_DUTY, CONFIG_MESH_PS_NWK_DUTY_DURATION, CONFIG_MESH_PS_NWK_DUTY_RULE));
#endif

    ESP_LOGI(TAG, "Started");
}

HAL::WiFiMesh &HAL::WiFiMesh::GetInstance() {
    static HAL::WiFiMesh wifi_mesh{};

    return wifi_mesh;
}

void HAL::WiFiMesh::BindingCallback(HAL::WiFiMesh::callback_t cb, void *arg) {
    this->BindingCallback(cb, uint32_t(EVENT_CONNECTED | EVENT_GOT_IP), arg);
}

void HAL::WiFiMesh::BindingCallback(HAL::WiFiMesh::callback_t cb, uint32_t event, void *arg) {
    if(!cb)
        return;

    if(event > HAL::WiFiMesh::EVENT_MAX)
        return;

    if(event == HAL::WiFiMesh::EVENT_MAX)
        event -= 1;

    for (auto & it : this->callback) {
        if(it.callback == cb) {
            return;
        }
    }

    s_callback_t s_callback = {.callback = cb, .event_mask = event, .arg = arg, .pthis = (void*)this};

    this->callback.push_back(s_callback);
}

void HAL::WiFiMesh::AttachEvent(HAL::WiFiMesh::callback_t cb, uint32_t event) {
    if(!cb)
        return;
    if(event > HAL::WiFiMesh::EVENT_MAX)
        return;

    if(event == HAL::WiFiMesh::EVENT_MAX)
        event -= 1;

    for (auto & it : this->callback) {
        if(it.callback == cb) {
            it.event_mask |= event;
            return;
        }
    }
}

void HAL::WiFiMesh::SetMQTT(HAL::MQTT *mqtt_client) {
    if(mqtt) {
        ESP_LOGE(TAG, "You have set mqtt client");
        return;
    }

    mqtt = mqtt_client;
    mqtt->BindingCallback(HAL::WiFiMesh::MQTTEventHandle, HAL::MQTT::EVENT_DATA, this);
}

HAL::MQTT &HAL::WiFiMesh::GetMQTT() {
    return *mqtt;
}

void HAL::WiFiMesh::RunCallback(void *arg, HAL::WiFiMesh::event_t event, void *data) {
    auto lst = (std::list<s_callback_t>*)arg;
    for (auto & it : *lst) {
        if(it.event_mask & event) {
            ESP_LOGI(TAG, "Event id: 0x%x", event);
            it.callback(event, data, it.arg);
        }
    }
}

[[noreturn]] void HAL::WiFiMesh::SendTask(void *arg) {
    auto wifi_mesh = (HAL::WiFiMesh*)arg;
    mesh_data_t mesh_msg;
    msg_t *msg;
    for(;;) {
        if(wifi_mesh->is_mesh_connected) {
            if(xQueueReceive(wifi_mesh->mesh_msg_queue, &mesh_msg, portMAX_DELAY) == pdTRUE) {
                /* Even the root node may be executed here, so we should judge carefully */
                msg = (msg_t*)mesh_msg.data;

                if(msg->mac)
                    ESP_LOGI(TAG, "SendTask will send to: " MACSTR "", MAC2STR(msg->mac->addr));
                else
                    ESP_LOGI(TAG, "SendTask will send to root");

                esp_err_t err = esp_mesh_send(msg->mac, &mesh_msg, MESH_DATA_P2P, nullptr, 0);
                if (err) {
                    ESP_LOGE(TAG,
                             "heap:%" PRId32 "[err:0x%x, proto:%d, tos:%d]",
                             esp_get_minimum_free_heap_size(),
                             err, mesh_msg.proto, mesh_msg.tos);
                }
                free(msg);
            }
        } else {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
}

void HAL::WiFiMesh::Publish(HAL::WiFiMesh::msg_t &msg) {
    if(esp_mesh_is_root()) {
        /* mqtt send */
        if(mqtt) {
            if(msg.type == MSG_MQTT && msg.mac == nullptr) {
                mqtt->Publish(*(HAL::MQTT::msg_t*)msg.data);
                goto publish_out;
            } else if(msg.type == MSG_SUBSCRIBE) {
                auto *sub_msg = (subscribe_msg_t *)msg.data;
                mqtt->Subscribe(sub_msg->topic, sub_msg->qos);
                goto publish_out;
            } else if(msg.type == MSG_UNSUBSCRIBE) {
                auto *sub_msg = (subscribe_msg_t *)msg.data;
                mqtt->Unsubscirbe(sub_msg->topic);
                goto publish_out;
            }
            goto publish_mesh;
publish_out:
            ESP_LOGI(TAG, "Publish through MQTT type: 0x%x", msg.type);
            return;
        } else {
            ESP_LOGE(TAG, "MQTT client is a null pointer %p", mqtt);
            return;
        }
    }
publish_mesh:
    ESP_LOGI(TAG, "Publish through Mesh-Network type: 0x%x", msg.type);
//    ESP_LOGI(TAG, "Publish through Mesh-Network type: 0x%x to:" MACSTR"", msg.type, MAC2STR(msg.mac->addr));
    auto *s_msg = (HAL::WiFiMesh::msg_t*)malloc(sizeof(msg_t));
    memcpy(s_msg, &msg, sizeof(msg_t));
    mesh_data_t mesh_msg;

    mesh_msg.data = (uint8_t*)s_msg;
    mesh_msg.size = sizeof(msg_t);
    mesh_msg.proto = MESH_PROTO_BIN;
    mesh_msg.tos = MESH_TOS_P2P;

    xQueueSend(this->mesh_msg_queue, &mesh_msg, MESH_QUEUE_WAIT_TIME_MS / portTICK_PERIOD_MS);

}

void HAL::WiFiMesh::RecvTask(void *arg) {
    mesh_addr_t from;
    mesh_data_t data;
    msg_t msg;
    int flag = 0;
    esp_err_t err;
    auto *wifi_mesh = (HAL::WiFiMesh*)arg;
    data.data = (uint8_t*)&msg;
    data.size = sizeof(msg_t);

    for(;;) {
        if(!wifi_mesh->is_mesh_connected) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }
        err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, nullptr, 0);
        if(err != ESP_OK || data.size != sizeof(msg_t)) {
            ESP_LOGE(TAG, "RecvTask: err:%x data.size:%d", err, data.size);
            continue;
        }

        ESP_LOGD(TAG, "RecvTask: \n\ttype: %d\n\tsize:%d", msg.type, msg.len);
        switch(msg.type) {
            case MSG_MQTT:
                if(esp_mesh_is_root() && memcmp(from.addr, wifi_mesh->root_mac.addr, sizeof(from.addr)) != 0)
                    /* When MQTTEventHandle sends a message through the mesh, there are two cases:
                     * 1. It is the root topic, in which case the root callback function needs to handle it
                     * 2. It is the child topic, in which case it needs to be sent through the mesh */
                    wifi_mesh->Publish(msg);
                else
                    HAL::WiFiMesh::RunCallback(&wifi_mesh->callback, HAL::WiFiMesh::EVENT_DATA, &msg);
                break;
            case MSG_UPLOAD_DEVICE_INFO: {
                if(esp_mesh_is_root()) {
                    auto *device_info = (device_info_t*)malloc(sizeof(device_info_t));
                    memcpy(device_info, msg.data, sizeof(device_info_t));
                    ESP_LOGI(TAG, "device info: %s mac:" MACSTR"",device_info->unique_id, MAC2STR(device_info->mac.addr) );
                    wifi_mesh->device_info_table.push_back(device_info);
                } else {
                    HAL::WiFiMesh::RunCallback(&wifi_mesh->callback, HAL::WiFiMesh::EVENT_UPLOAD_DEVICE_INFO, &msg);
                }
            }
                break;
                /* just root receive */
            case MSG_SUBSCRIBE: {
                auto *sub_msg = (subscribe_msg_t *)msg.data;
                wifi_mesh->mqtt->Subscribe(sub_msg->topic, sub_msg->qos);
            }
                break;
            case MSG_UNSUBSCRIBE: {
                auto *unsub_msg = (subscribe_msg_t *)msg.data;
                wifi_mesh->mqtt->Unsubscirbe(unsub_msg->topic);
            }
                break;
            default:
                /* callback */
                HAL::WiFiMesh::RunCallback((void*)&wifi_mesh->callback, EVENT_DATA, (void*)&msg);
                break;
        }
    }
}

void HAL::WiFiMesh::MQTTEventHandle(HAL::MQTT::event_t event, void *data, void *arg) {
    auto* wifi_mesh = (WiFiMesh*)arg;
    if(event == HAL::MQTT::EVENT_DATA) {
        /* root node also in this list */
        auto *r_msg = (HAL::MQTT::msg_t *) data;
        for (auto & it : wifi_mesh->device_info_table) {
            if (strncmp(r_msg->topic, it->unique_id, strlen(it->unique_id)) == 0) {
                ESP_LOGI(TAG, "MQTTEventHandle probe successful:\n\ttopic:%s", r_msg->topic);
                wifi_mesh->Publish(data, sizeof(HAL::MQTT::msg_t), MSG_MQTT, &it->mac);
                return;
            }
        }
        /* broadcast here */
        ESP_LOGI(TAG, "Broadcast");
//        RunCallback(arg, HAL::WiFiMesh::EVENT_DATA, data);
        wifi_mesh->Broadcast(data, sizeof(HAL::MQTT::msg_t), MSG_MQTT, true);
    }
}

void HAL::WiFiMesh::WiFiEventHandle(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto *event = (ip_event_got_ip_t *) event_data;
    auto *mesh = (HAL::WiFiMesh*) arg;

    if(event_base == IP_EVENT) {
        if(event_id == IP_EVENT_STA_GOT_IP) {
            ESP_LOGI(TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));

            mesh->mqtt->Start();

            RunCallback(arg, HAL::WiFiMesh::EVENT_GOT_IP, &event->ip_info.ip);
        }
    } else if(event_base == WIFI_EVENT) {
        if(event_id == WIFI_EVENT_STA_DISCONNECTED) {
            mesh->mqtt->Stop();
        }
    }
}

void HAL::WiFiMesh::MeshEventHandle(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    mesh_addr_t id{};
    static uint16_t last_layer = 0;
    auto *wifi_mesh = (HAL::WiFiMesh*)arg;
    static mesh_addr_t child_mac;

    switch (event_id) {
        case MESH_EVENT_STARTED: {
            esp_mesh_get_id(&id);
            ESP_LOGI(TAG, "<MESH_EVENT_MESH_STARTED>ID:" MACSTR"", MAC2STR(id.addr));
            wifi_mesh->is_mesh_connected = false;
            wifi_mesh->mesh_layer = esp_mesh_get_layer();
        }
            break;
        case MESH_EVENT_STOPPED: {
            ESP_LOGI(TAG, "<MESH_EVENT_STOPPED>");
            wifi_mesh->is_mesh_connected = false;
            wifi_mesh->mesh_layer = esp_mesh_get_layer();
        }
            break;
        case MESH_EVENT_CHILD_CONNECTED: {
            auto *child_connected = (mesh_event_child_connected_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR"",
                     child_connected->aid,
                     MAC2STR(child_connected->mac));
            memcpy(child_mac.addr, child_connected->mac, sizeof(child_mac.addr));
            wifi_mesh->Publish(&child_mac, sizeof(mesh_addr_t), MSG_UPLOAD_DEVICE_INFO, &child_mac);

        }
            break;
        case MESH_EVENT_CHILD_DISCONNECTED: {
            auto *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR"",
                     child_disconnected->aid,
                     MAC2STR(child_disconnected->mac));
        }
            break;
        case MESH_EVENT_ROUTING_TABLE_ADD: {
            auto *routing_table = (mesh_event_routing_table_change_t *)event_data;
            ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
                     routing_table->rt_size_change,
                     routing_table->rt_size_new, wifi_mesh->mesh_layer);
        }
            break;
        case MESH_EVENT_ROUTING_TABLE_REMOVE: {
            auto *routing_table = (mesh_event_routing_table_change_t *)event_data;
            ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
                     routing_table->rt_size_change,
                     routing_table->rt_size_new, wifi_mesh->mesh_layer);
        }
            break;
        case MESH_EVENT_NO_PARENT_FOUND: {
            auto *no_parent = (mesh_event_no_parent_found_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                     no_parent->scan_times);
        }
            /* TODO handler for the failure */
            break;
        case MESH_EVENT_PARENT_CONNECTED: {
            auto *connected = (mesh_event_connected_t *)event_data;
            esp_mesh_get_id(&id);
            wifi_mesh->mesh_layer = connected->self_layer;
            memcpy(&wifi_mesh->mesh_parent_addr.addr, connected->connected.bssid, 6);
            ESP_LOGI(TAG,
                     "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR"%s, ID:" MACSTR", duty:%d",
                     last_layer, wifi_mesh->mesh_layer, MAC2STR(wifi_mesh->mesh_parent_addr.addr),
                     esp_mesh_is_root() ? "<ROOT>" :
                     (wifi_mesh->mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr), connected->duty);
            wifi_mesh->is_mesh_connected = true;
            if (esp_mesh_is_root()) {
                esp_netif_dhcpc_stop(wifi_mesh->netif_sta);
                esp_netif_dhcpc_start(wifi_mesh->netif_sta);

                msg_t msg{};
                /* record the root mac address */
                esp_read_mac(wifi_mesh->root_mac.addr, ESP_MAC_WIFI_STA);
                memcpy(msg.data, &wifi_mesh->root_mac, sizeof(mesh_addr_t));
                /* root run callback to upload self info directly */
                HAL::WiFiMesh::RunCallback(&wifi_mesh->callback, EVENT_UPLOAD_DEVICE_INFO, &msg);
            } else {
                HAL::WiFiMesh::RunCallback(&wifi_mesh->callback, EVENT_CONNECTED, nullptr);
            }

            if(wifi_mesh->status_led)
                wifi_mesh->status_led->Set(wifi_mesh->status_led_activate);
        }
            break;
        case MESH_EVENT_PARENT_DISCONNECTED: {
            auto *disconnected = (mesh_event_disconnected_t *)event_data;
            ESP_LOGI(TAG,
                     "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                     disconnected->reason);
            wifi_mesh->is_mesh_connected = false;
            wifi_mesh->mesh_layer = esp_mesh_get_layer();

            if(wifi_mesh->status_led)
                wifi_mesh->status_led->Set((GPIO::gpio_state_t )!wifi_mesh->status_led_activate);
        }
            break;
        case MESH_EVENT_LAYER_CHANGE: {
            auto *layer_change = (mesh_event_layer_change_t *)event_data;
            wifi_mesh->mesh_layer = layer_change->new_layer;
            ESP_LOGI(TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                     last_layer, wifi_mesh->mesh_layer,
                     esp_mesh_is_root() ? "<ROOT>" :
                     (wifi_mesh->mesh_layer == 2) ? "<layer2>" : "");
            last_layer = wifi_mesh->mesh_layer;
        }
            break;
        case MESH_EVENT_ROOT_ADDRESS: {
            auto *root_addr = (mesh_event_root_address_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:" MACSTR"",
                     MAC2STR(root_addr->addr));
            mesh_addr_t self_addr;
            esp_read_mac((uint8_t*)&self_addr, ESP_MAC_WIFI_STA);
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>self address:" MACSTR"",
                     MAC2STR(self_addr.addr));
        }
            break;
        case MESH_EVENT_VOTE_STARTED: {
            auto *vote_started = (mesh_event_vote_started_t *)event_data;
            ESP_LOGI(TAG,
                     "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:" MACSTR"",
                     vote_started->attempts,
                     vote_started->reason,
                     MAC2STR(vote_started->rc_addr.addr));
        }
            break;
        case MESH_EVENT_VOTE_STOPPED: {
            ESP_LOGI(TAG, "<MESH_EVENT_VOTE_STOPPED>");
            break;
        }
        case MESH_EVENT_ROOT_SWITCH_REQ: {
            auto *switch_req = (mesh_event_root_switch_req_t *)event_data;
            ESP_LOGI(TAG,
                     "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:" MACSTR"",
                     switch_req->reason,
                     MAC2STR( switch_req->rc_addr.addr));
        }
            break;
        case MESH_EVENT_ROOT_SWITCH_ACK: {
            /* new root */
            wifi_mesh->mesh_layer = esp_mesh_get_layer();
            esp_mesh_get_parent_bssid(&wifi_mesh->mesh_parent_addr);
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:" MACSTR"", wifi_mesh->mesh_layer, MAC2STR(wifi_mesh->mesh_parent_addr.addr));
        }
            break;
        case MESH_EVENT_TODS_STATE: {
            auto *toDs_state = (mesh_event_toDS_state_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
        }
            break;
        case MESH_EVENT_ROOT_FIXED: {
            auto *root_fixed = (mesh_event_root_fixed_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                     root_fixed->is_fixed ? "fixed" : "not fixed");
        }
            break;
        case MESH_EVENT_ROOT_ASKED_YIELD: {
            auto *root_conflict = (mesh_event_root_conflict_t *)event_data;
            ESP_LOGI(TAG,
                     "<MESH_EVENT_ROOT_ASKED_YIELD>" MACSTR", rssi:%d, capacity:%d",
                     MAC2STR(root_conflict->addr),
                     root_conflict->rssi,
                     root_conflict->capacity);

            /* The root will change */
            RunCallback(arg, HAL::WiFiMesh::EVENT_ROOT_WILL_CHANGE, nullptr);
        }
            break;
        case MESH_EVENT_CHANNEL_SWITCH: {
            auto *channel_switch = (mesh_event_channel_switch_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
        }
            break;
        case MESH_EVENT_SCAN_DONE: {
            auto *scan_done = (mesh_event_scan_done_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                     scan_done->number);
        }
            break;
        case MESH_EVENT_NETWORK_STATE: {
            auto *network_state = (mesh_event_network_state_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                     network_state->is_rootless);
        }
            break;
        case MESH_EVENT_STOP_RECONNECTION: {
            ESP_LOGI(TAG, "<MESH_EVENT_STOP_RECONNECTION>");
        }
            break;
        case MESH_EVENT_FIND_NETWORK: {
            auto *find_network = (mesh_event_find_network_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:" MACSTR"",
                     find_network->channel, MAC2STR(find_network->router_bssid));
        }
            break;
        case MESH_EVENT_ROUTER_SWITCH: {
            auto *router_switch = (mesh_event_router_switch_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, " MACSTR"",
                     router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
        }
            break;
        case MESH_EVENT_PS_PARENT_DUTY: {
            auto *ps_duty = (mesh_event_ps_duty_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
        }
            break;
        case MESH_EVENT_PS_CHILD_DUTY: {
            auto *ps_duty = (mesh_event_ps_duty_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, " MACSTR", duty:%d", ps_duty->child_connected.aid-1,
                     MAC2STR(ps_duty->child_connected.mac), ps_duty->duty);
        }
            break;
        default:
            ESP_LOGI(TAG, "unknown id:%" PRId32 "", event_id);
            break;
    }
}

void HAL::WiFiMesh::Publish(void *data, size_t size, HAL::WiFiMesh::msg_type_t type) {
    this->Publish(data, size, type, nullptr);
}

void HAL::WiFiMesh::Publish(void *data, size_t size, HAL::WiFiMesh::msg_type_t type, mesh_addr_t *mac) {
    msg_t msg;
    memcpy(msg.data, data, size);
    msg.type = type;
    msg.mac = mac;
    msg.len = size;
    this->Publish(msg);
}

void HAL::WiFiMesh::Subscribe(char *topic, uint8_t qos) {
    subscribe_msg_t sub_msg{};

    strcpy(sub_msg.topic, topic);
    sub_msg.qos = qos;

    this->Publish(&sub_msg, sizeof(subscribe_msg_t), MSG_SUBSCRIBE);
}

void HAL::WiFiMesh::Subscribe(char *topic) {
    this->Subscribe(topic, 0);
}

void HAL::WiFiMesh::Unsubscribe(char *topic) {
    subscribe_msg_t unsub_msg{};

    strcpy(unsub_msg.topic, topic);

    this->Publish(&unsub_msg, sizeof(subscribe_msg_t), MSG_UNSUBSCRIBE);
}

void HAL::WiFiMesh::Broadcast( void *data, size_t size, msg_type_t type, bool to_root) {
    mesh_addr_t route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
    int route_table_size = 0;
    esp_err_t err;

    esp_mesh_get_routing_table((mesh_addr_t *) &route_table,
                               CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &route_table_size);

    for (int i = to_root ? 0 : 1; i < route_table_size; i++) {
        this->Publish(data, size, type, &route_table[i]);
    }
}

void HAL::WiFiMesh::Broadcast(void *data, size_t size, msg_type_t type) {
    this->Broadcast(data, size, type, false);
}

void HAL::WiFiMesh::SetStatusLed(int gpio_num, HAL::GPIO::gpio_state_t activate_state) {
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

    if(is_mesh_connected)
        this->status_led->Set(this->status_led_activate);
    else
        this->status_led->Set(HAL::GPIO::gpio_state_t(!this->status_led_activate));
}
