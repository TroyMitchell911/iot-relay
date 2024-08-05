//
// Created by troy on 2024/8/5.
//

#ifndef RELAY_HAL_TIME_H
#define RELAY_HAL_TIME_H

#include "../../../../../../../.espressif/tools/riscv32-esp-elf/esp-13.2.0_20240530/riscv32-esp-elf/riscv32-esp-elf/include/c++/13.2.0/ctime"
#include "../../../../../../../.espressif/tools/riscv32-esp-elf/esp-13.2.0_20240530/riscv32-esp-elf/riscv32-esp-elf/include/c++/13.2.0/cstdint"

namespace HAL {
    typedef struct time {
        int	sec;
        int	min;
        int	hour;
    }*time_t;

    typedef struct date {
        struct time time;
        int	day;
        int	mon;
        int	year;
        int	wday;
        char wday_str[16];
    }*date_t;
    class Time {

    private:
        struct date t;
        std::time_t std_time;
        struct tm* tm;

    private:
        Time();
        ~Time();
        Time(const Time &time) = delete;
        const Time &operator=(const Time &time) = delete;
        void update();

    public:
        void getTime(time_t time);
        void getTimeStr(char *buf);
        void getDate(date_t date);
        void getDateStr(char *buf);
        void updateTime();
        static Time& getInstance();
    };
};



#endif //RELAY_HAL_TIME_H
