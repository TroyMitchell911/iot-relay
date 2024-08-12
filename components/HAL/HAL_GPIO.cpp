//
// Created by troy on 2024/8/12.
//

#include <cstring>
#include "HAL_GPIO.h"

/* -----------------------For porting start------------------------- */
#include <driver/gpio.h>
#include <esp_log.h>

#define GPIO_LOG(format, ... ) ESP_LOGI("GPIO", format, ##__VA_ARGS__)

static void GPIO_InitDirection(const HAL::GPIO::gpio_cfg_t *cfg, void* port_cfg) {
    auto *config = (gpio_config_t*)port_cfg;

    if(cfg->direction == HAL::GPIO::GPIO_INPUT) {
        config->mode = GPIO_MODE_INPUT;
    } else if(cfg->direction == HAL::GPIO::GPIO_OUTPUT) {
        config->mode = GPIO_MODE_OUTPUT;
    } else if(cfg->direction == HAL::GPIO::GPIO_BOTH) {
        config->mode = GPIO_MODE_INPUT_OUTPUT;
    }else {
        config->mode = GPIO_MODE_DISABLE;
    }
}

static void GPIO_InitMode(const HAL::GPIO::gpio_cfg_t *cfg, void* port_cfg) {
    auto *config = (gpio_config_t*)port_cfg;

    if(cfg->direction != HAL::GPIO::GPIO_DISABLE && cfg->direction != HAL::GPIO::GPIO_INPUT) {
        if(cfg->mode == HAL::GPIO::GPIO_OD) {
            config->mode = gpio_mode_t (config->mode | GPIO_MODE_DEF_OD);
        }
    }
}

static void GPIO_InitPull(const HAL::GPIO::gpio_cfg_t *cfg, void* port_cfg) {
    auto *config = (gpio_config_t*)port_cfg;

    config->pull_up_en = gpio_pullup_t(cfg->pull_up);
    config->pull_down_en = gpio_pulldown_t(cfg->pull_down);
}

static void GPIO_Init(const HAL::GPIO::gpio_cfg_t *cfg) {
    gpio_config_t config{};

    config.intr_type = GPIO_INTR_DISABLE;
    config.pin_bit_mask = 1ull << cfg->pin;
    GPIO_InitDirection(cfg, &config);
    GPIO_InitMode(cfg, &config);
    GPIO_InitPull(cfg, &config);

    gpio_config(&config);
}

static void GPIO_Set(const HAL::GPIO::gpio_cfg_t  *cfg, HAL::GPIO::gpio_state_t state) {
    gpio_set_level(gpio_num_t(cfg->pin), state == HAL::GPIO::GPIO_STATE_HIGH ? 1 : 0);

}

static HAL::GPIO::gpio_state_t GPIO_Get(const HAL::GPIO::gpio_cfg_t  *cfg) {
    int state = gpio_get_level(gpio_num_t(cfg->pin));
    return state == 1 ? HAL::GPIO::GPIO_STATE_HIGH : HAL::GPIO::GPIO_STATE_LOW;
}
/* -----------------------For porting end------------------------- */

#define DEFAULT_SPEED   0

const static  HAL::GPIO::gpio_cfg_t default_cfg = {
#if GPIO_USE_GROUP
    .group = 0,
#endif
    .pin = 0,
    .direction = HAL::GPIO::GPIO_DISABLE,
#if GPIO_USE_SPEED
    .speed = DEFAULT_SPEED,
#endif
};

#if GPIO_USE_GROUP
HAL::GPIO::GPIO(uint32_t group, uint32_t pin) {
    gpio_cfg_t gpiocfg;
    memcpy(&cfg, &default_cfg, sizeof(gpio_cfg_t));
    gpiocfg.group = group;
    gpiocfg.pin = pin;
    this->Reconfigure(gpiocfg);
}
#else
HAL::GPIO::GPIO(uint32_t pin) {
    gpio_cfg_t gpiocfg;
    memcpy(&gpiocfg, &default_cfg, sizeof(gpio_cfg_t));
    gpiocfg.pin = pin;
    this->Reconfigure(gpiocfg);
}
#endif

HAL::GPIO::GPIO(HAL::GPIO::gpio_cfg_t gpiocfg) {
    this->Reconfigure(gpiocfg);
}

HAL::GPIO::~GPIO() {
    GPIO_Init(&default_cfg);
}

HAL::GPIO::gpio_cfg_t HAL::GPIO::GetConfig() {
    return this->cfg;
}

void HAL::GPIO::Reconfigure(HAL::GPIO::gpio_cfg_t gpiocfg) {
    memcpy(&this->cfg, &gpiocfg, sizeof(HAL::GPIO::gpio_cfg_t));
    GPIO_Init(&this->cfg);
}

void HAL::GPIO::Set(HAL::GPIO::gpio_state_t gpio_state) {
    if(this->cfg.direction == HAL::GPIO::GPIO_INPUT || this->cfg.direction == HAL::GPIO::GPIO_DISABLE)
        return;
    GPIO_Set(&this->cfg, gpio_state);
    this->state = gpio_state;
}

HAL::GPIO::gpio_state_t HAL::GPIO::Get() {
    if(!inited || this->cfg.direction == GPIO_DISABLE)
        return HAL::GPIO::GPIO_STATE_NONE;
    if(this->cfg.direction == HAL::GPIO::GPIO_OUTPUT)
        return this->state;
    else
        return GPIO_Get(&this->cfg);
}



