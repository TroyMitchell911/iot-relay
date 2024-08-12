//
// Created by troy on 2024/8/12.
//

#include <cstring>
#include "include/HAL_GPIO.h"

#define DEFAULT_SPEED   0

const static  HAL::GPIO::gpio_cfg_t default_cfg = {
    .direction = HAL::GPIO::GPIO_INPUT,
    .pull = HAL::GPIO::GPIO_NO_PULL,
    .method = HAL::GPIO::GPIO_KAILOU,
    .speed = DEFAULT_SPEED,
};

static void Init_GPIO(const HAL::GPIO::gpio_cfg_t *cfg) {

}

static void Set_GPIO(const HAL::GPIO::gpio_cfg_t  *cfg, HAL::GPIO::GPIO_STATE_T  state) {

}

static HAL::GPIO::GPIO_STATE_T Get_GPIO(const HAL::GPIO::gpio_cfg_t  *cfg) {
    return HAL::GPIO::GPIO_STATE_LOW;
}

HAL::GPIO::GPIO() {
    this->Reconfigure(default_cfg);
}

HAL::GPIO::GPIO(HAL::GPIO::gpio_cfg_t gpiocfg) {
    this->Reconfigure(gpiocfg);
}

HAL::GPIO::~GPIO() {
    Init_GPIO(&default_cfg);
}

void HAL::GPIO::Reconfigure(HAL::GPIO::gpio_cfg_t gpiocfg) {
    memcpy(&this->cfg, &gpiocfg, sizeof(HAL::GPIO::gpio_cfg_t));
    Init_GPIO(&this->cfg);
}

void HAL::GPIO::Set(HAL::GPIO::GPIO_STATE_T gpio_state) {
    if(this->cfg.direction == GPIO_OUTPUT)
        return;
    Set_GPIO(&this->cfg, gpio_state);
    this->state = gpio_state;
}

HAL::GPIO::GPIO_STATE_T HAL::GPIO::Get() {
    if(this->cfg.direction == GPIO_OUTPUT)
        return this->state;
    else
        return Get_GPIO(&this->cfg);
}

