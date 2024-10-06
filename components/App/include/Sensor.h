//
// Created by troy on 2024/10/6.
//

#ifndef RELAY_SENSOR_H
#define RELAY_SENSOR_H

#include "HomeAssistant.h"


namespace App {
    class Sensor : private App::HomeAssistant {

    public:
        typedef struct {
            const char *value_name;
            double value;
        }update_data_t;

        typedef unsigned int (*update_func_t)(update_data_t **data);

        typedef enum {
            SENSOR_BATTERY,         // 电池电量（百分比）
            SENSOR_HUMIDITY,        // 湿度（百分比）
            SENSOR_ILLUMINANCE,     // 光照（单位：LX 或 LM）
            SENSOR_TEMPERATURE,     // 温度（单位：°C 或 °F）
            SENSOR_PRESSURE,        // 气压（单位：HPA 或 MBAR）
            SENSOR_POWER,           // 功率（单位：W）
            SENSOR_ENERGY,          // 能量（单位：KWH）
            SENSOR_CURRENT,         // 电流（单位：A）
            SENSOR_VOLTAGE,         // 电压（单位：V）
            SENSOR_SIGNAL_STRENGTH, // 信号强度（单位：DB 或 DBM）
            SENSOR_TIMESTAMP        // 时间戳（ISO 格式）
        } sensor_device_class_t;

        typedef struct {
            /* device class */
            sensor_device_class_t device_class;
            /* Can be set to nullptr */
            const char *unit_of_measurement;
            /* corresponds to value_name in update_data_t */
            const char *value_name;
        }sensor_info_t;

    private:
        /* ms */
        unsigned int update_interval;
        update_func_t update_func;
        sensor_info_t info;
        TaskHandle_t update_task_handler = nullptr;
        unsigned int data_num = 0;
        update_data_t *data = nullptr;
        cJSON *data_json;

    private:
        static void UpdateTask(void *arg);
        static void Update(App::Sensor *sensor);

    public:
        void Init() override;
        Sensor(HAL::WiFiMesh *mesh,
               const char *where,
               const char *name,
               sensor_info_t info,
               unsigned int interval);
        ~Sensor();
        void BindUpdate(update_func_t func);
    };
}


#endif //RELAY_SENSOR_H
