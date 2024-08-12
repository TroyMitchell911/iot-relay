//
// Created by troy on 2024/8/12.
//

#ifndef RELAY_HAL_GPIO_H
#define RELAY_HAL_GPIO_H

#include <cstdint>

namespace HAL{
    class GPIO {
    public:
        enum {
            GPIO_INPUT,
            GPIO_OUTPUT,
        };

        enum {
            GPIO_KAILOU,
            GPIO_TUIWAN,
        };

        enum {
            GPIO_PULLUP,
            GPIO_PULLDOWN,
            GPIO_NO_PULL,
        };

        typedef struct {
            uint8_t group;
            uint8_t pin;
            uint8_t direction;
            uint8_t pull;
            uint8_t method;
            uint32_t speed;
        }gpio_cfg_t;

    public:
        typedef enum {
            GPIO_STATE_HIGH,
            GPIO_STATE_LOW,
        }GPIO_STATE_T;

    private:
        gpio_cfg_t cfg{};
        GPIO_STATE_T state;

    public:
        GPIO();
        explicit GPIO(HAL::GPIO::gpio_cfg_t gpiocfg);
        ~GPIO();

    public:
        void Reconfigure(HAL::GPIO::gpio_cfg_t gpiocfg);
        void Set(HAL::GPIO::GPIO_STATE_T gpio_state);
        HAL::GPIO::GPIO_STATE_T Get();
    };
}


#endif //RELAY_HAL_GPIO_H
