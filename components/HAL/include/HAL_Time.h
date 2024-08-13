//
// Created by troy on 2024/8/5.
//

#ifndef RELAY_HAL_TIME_H
#define RELAY_HAL_TIME_H

#include <ctime>
#include <cstdint>

namespace HAL {
    typedef struct time {
        int	sec;
        int	min;
        int	hour;
    }*time_t;

    typedef struct date {
        int	day;
        int	mon;
        int	year;
    }*date_t;

    typedef struct clock {
        int	sec;
        int	min;
        int	hour;
        int	day;
        int	mon;
        int	year;
        int	wday;
    }*clock_t;

    class Time {

    private:
        struct clock g_clock;
        std::time_t std_time;
        struct tm* tm;

    private:
        Time();
        ~Time();
        Time(const Time &time) = delete;
        const Time &operator=(const Time &time) = delete;
        void update();

    public:
        void GetTime(HAL::time_t *time);
        void GetTime(char **time);
        void GetDate(HAL::date_t *date);
        void GetDate(char **date);
        void GetClock(HAL::clock_t *clock);
        void GetClock(char **clock);
        int GetWeek(char **week, bool full);
        void SyncTime();
        static Time& GetInstance();
    };
};



#endif //RELAY_HAL_TIME_H
