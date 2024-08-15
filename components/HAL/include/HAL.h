//
// Created by troy on 2024/8/14.
//

#ifndef RELAY_HAL_H
#define RELAY_HAL_H

#include "HAL_GPIO.h"
#include "HAL_MQTT.h"
#include "HAL_WiFi.h"
#include "HAL_WiFiMesh.h"
#include "HAL_Time.h"

namespace HAL {
    void Init();
}

#endif //RELAY_HAL_H
