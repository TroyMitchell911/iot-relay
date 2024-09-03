//
// Created by troy on 2024/8/14.
//

#ifndef RELAY_SWITCH_H
#define RELAY_SWITCH_H

#include "HomeAssistant.h"
#include "HAL.h"

namespace App {
    class Switch : private App::HomeAssistant{

    private:
        bool sw_status = false;
        char command_topic[MQTT_TOPIC_MAX_NUM] = {0};
        HAL::GPIO *switch_gpio;
        HAL::GPIO::gpio_state_t switch_active_state;

        TaskHandle_t manual_button_task_handler = nullptr;
        HAL::GPIO::gpio_state_t manual_button_state;
        HAL::GPIO *manual_button_gpio;
        HAL::GPIO::gpio_state_t manual_button_active_state;

    private:
        static void Process(HAL::WiFiMesh::event_t event, void *data, void *arg);
        static void ManualButtonTask(void *arg);

    public:
        void Init() override;
        Switch(HAL::WiFiMesh *mesh,
               const char *where,
               const char *name,
               int gpio_num,
               int active_state);
        Switch(HAL::WiFiMesh *mesh,
               const char *where,
               const char *name,
               int switch_gpio_num,
               int switch_active_state,
               int manual_button_gpio_num,
               int manual_button_active_state);
        void Act();
        void Act(bool is_changing_value);
    };
}



#endif //RELAY_SWITCH_H
