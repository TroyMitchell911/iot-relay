//
// Created by troy on 2024/9/3.
//

#ifndef RELAY_BINARYSENSOR_H
#define RELAY_BINARYSENSOR_H

#include "HomeAssistant.h"
#include "HAL_GPIO.h"

namespace App {
    class BinarySensor : private App::HomeAssistant{

    public:
        typedef enum {
            PRESENCE,
            ENTITY_TYPE_MAX
        }sensor_type_t;

    private:
        bool bs_status = false;
        HAL::GPIO *gpio;
        HAL::GPIO::gpio_state_t active_state;
        HAL::GPIO::gpio_state_t gpio_state;
        TaskHandle_t detect_task_handler = nullptr;

        sensor_type_t sensor_type;

    private:
        void Update();
        [[noreturn]] static void DetectTask(void *arg);

    public:
        void Init() override;
        BinarySensor(HAL::WiFiMesh *mesh,
                     const char *where,
                     const char *name,
                     int gpio_num,
                     int active_state,
                     sensor_type_t type);
    };
}



#endif //RELAY_BINARYSENSOR_H
