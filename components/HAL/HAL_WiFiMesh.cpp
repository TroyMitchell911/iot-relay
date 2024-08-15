//
// Created by troy on 2024/8/14.
//

#include <esp_wifi.h>
#include <esp_mesh.h>
#include <esp_log.h>
#include <esp_mac.h>
#include "HAL_WiFiMesh.h"

#define TAG "[HAL::WiFiMesh]"

static int mesh_layer = -1;
static mesh_addr_t mesh_parent_addr;
static esp_netif_t *netif_sta = NULL;
static bool is_mesh_connected = false;

typedef struct {
    HAL::WiFiMesh::callback_t callback;
    uint32_t event_mask;
    void *arg;
}s_callback_t;

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    mesh_addr_t id{};
    static uint16_t last_layer = 0;
    auto *wifi_mesh = (HAL::WiFiMesh*)arg;

    switch (event_id) {
        case MESH_EVENT_STARTED: {
            esp_mesh_get_id(&id);
            ESP_LOGI(TAG, "<MESH_EVENT_MESH_STARTED>ID:" MACSTR"", MAC2STR(id.addr));
            is_mesh_connected = false;
            mesh_layer = esp_mesh_get_layer();
        }
            break;
        case MESH_EVENT_STOPPED: {
            ESP_LOGI(TAG, "<MESH_EVENT_STOPPED>");
            is_mesh_connected = false;
            mesh_layer = esp_mesh_get_layer();
        }
            break;
        case MESH_EVENT_CHILD_CONNECTED: {
            mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR"",
                     child_connected->aid,
                     MAC2STR(child_connected->mac));
        }
            break;
        case MESH_EVENT_CHILD_DISCONNECTED: {
            mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR"",
                     child_disconnected->aid,
                     MAC2STR(child_disconnected->mac));
        }
            break;
        case MESH_EVENT_ROUTING_TABLE_ADD: {
            auto *routing_table = (mesh_event_routing_table_change_t *)event_data;
            ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
                     routing_table->rt_size_change,
                     routing_table->rt_size_new, mesh_layer);
        }
            break;
        case MESH_EVENT_ROUTING_TABLE_REMOVE: {
            auto *routing_table = (mesh_event_routing_table_change_t *)event_data;
            ESP_LOGW(TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
                     routing_table->rt_size_change,
                     routing_table->rt_size_new, mesh_layer);
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
            mesh_layer = connected->self_layer;
            memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
            ESP_LOGI(TAG,
                     "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR"%s, ID:" MACSTR", duty:%d",
                     last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                     esp_mesh_is_root() ? "<ROOT>" :
                     (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr), connected->duty);
            if (esp_mesh_is_root()) {
                esp_netif_dhcpc_stop(netif_sta);
                esp_netif_dhcpc_start(netif_sta);
            }
        }
            break;
        case MESH_EVENT_PARENT_DISCONNECTED: {
            auto *disconnected = (mesh_event_disconnected_t *)event_data;
            ESP_LOGI(TAG,
                     "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                     disconnected->reason);
            is_mesh_connected = false;
            mesh_layer = esp_mesh_get_layer();
        }
            break;
        case MESH_EVENT_LAYER_CHANGE: {
            auto *layer_change = (mesh_event_layer_change_t *)event_data;
            mesh_layer = layer_change->new_layer;
            ESP_LOGI(TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                     last_layer, mesh_layer,
                     esp_mesh_is_root() ? "<ROOT>" :
                     (mesh_layer == 2) ? "<layer2>" : "");
            last_layer = mesh_layer;
        }
            break;
        case MESH_EVENT_ROOT_ADDRESS: {
            auto *root_addr = (mesh_event_root_address_t *)event_data;
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:" MACSTR"",
                     MAC2STR(root_addr->addr));
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
            mesh_layer = esp_mesh_get_layer();
            esp_mesh_get_parent_bssid(&mesh_parent_addr);
            ESP_LOGI(TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:" MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
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

void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    auto *callback = (s_callback_t*)arg;

    auto *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));

    printf("%lx\n", callback->event_mask);
    if(callback->event_mask & HAL::WiFiMesh::EVENT_GOT_IP) {

        callback->callback(HAL::WiFiMesh::EVENT_GOT_IP, &event->ip_info.ip, callback->arg);
    }

}

void HAL::WiFiMesh::Start(HAL::WiFiMesh::wifi_mesh_cfg_t *config) {
    /*  create network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, nullptr));
    /*  wifi initialization */
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, (void*)&this->callback));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, (void*)&this->callback));
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
    /* better to increase the announce interval to avoid too much management traffic, if a small duty cycle is set. */
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
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(wifi_auth_mode_t(CONFIG_MESH_AP_AUTHMODE)));
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
}

HAL::WiFiMesh &HAL::WiFiMesh::GetInstance() {
    static HAL::WiFiMesh wifi_mesh{};

    return wifi_mesh;
}

void HAL::WiFiMesh::BindingEvent(HAL::WiFiMesh::callback_t cb, void *arg) {
    this->BindingEvent(cb, arg, EVENT_GOT_IP);
}

void HAL::WiFiMesh::BindingEvent(HAL::WiFiMesh::callback_t cb, void *arg, int event) {
    if(!cb)
        return;
    if(event >= HAL::WiFiMesh::EVENT_MAX)
        return;

    this->callback.callback = cb;
    this->callback.arg = arg;
    this->callback.event_mask = HAL::WiFiMesh::event_t(event);
    this->callback.pthis = (void*)this;
}

void HAL::WiFiMesh::AttachEvent(HAL::WiFiMesh::callback_t cb, int event) {
    if(!cb)
        return;
    if(event >= HAL::WiFiMesh::EVENT_MAX)
        return;

    this->callback.event_mask |= HAL::WiFiMesh::event_t(event);
}