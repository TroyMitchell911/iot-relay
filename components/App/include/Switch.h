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
        HAL::GPIO *gpio;
        HAL::GPIO::gpio_state_t active_state;

    private:
        void InitGPIO(int gpio_num, int _active_state);
        static void Process(HAL::WiFiMesh::event_t event, void *data, void *arg);

    public:
        void Init() override;
        Switch(HAL::WiFiMesh *mesh,
               const char *where,
               const char *name,
               int gpio_num,
               int active_state);
        void Act();
        void Act(bool is_changing_value);
    };
}



#endif //RELAY_SWITCH_H
