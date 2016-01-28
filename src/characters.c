#include <pebble.h>
#include "characters.h"

char chinese_numbers[12] = "A&'()*+,-.A";

void chinese_date_string(char* date_buffer, struct tm *tick_time) {
    #define month (tick_time->tm_mon+1)
    #define day (tick_time->tm_mday)
    //#define month 12
    //#define day 27
    int ptr = 0;
    if(month <= 10) {
        date_buffer[ptr++] = chinese_numbers[month];
    } else {
        date_buffer[ptr++] = chinese_numbers[0];
        date_buffer[ptr++] = chinese_numbers[month - 10];
    }
    date_buffer[ptr++] = YUE;
    #ifndef PBL_ROUND
    if(day > 20 && month > 10)
    #endif
        date_buffer[ptr++] = '\n';//wordwrap
        
    if(day <= 10) {
        date_buffer[ptr++] = chinese_numbers[day];
    } else {
        if(day >= 30)
            date_buffer[ptr++] = chinese_numbers[3];
        else if(day >= 20)
            date_buffer[ptr++] = chinese_numbers[2];
        date_buffer[ptr++] = chinese_numbers[0]; //SHI
        if(day % 10 != 0)
            date_buffer[ptr++] = chinese_numbers[day % 10];
    }
    date_buffer[ptr++] = RI;
    date_buffer[ptr++] = '\0';
}

void chinese_time_string(char* time_buffer, struct tm *tick_time) {
    #define minute (tick_time->tm_min)
    int hour = clock_is_24h_style() ? (tick_time->tm_hour) : (tick_time->tm_hour % 12);
    if(hour == 0) hour = 12;
    
    if(minute == 0)
        snprintf(time_buffer, 4, "%d%c", hour, DIAN);
    else if(minute == 30)
        snprintf(time_buffer, 5, "%d%c%c", hour, DIAN, BAN);
}

char chinese_weekday_number(struct tm *tick_time) {
    #define wday (tick_time->tm_wday)
    if(wday == 0)
        return TIAN;
    else
        return chinese_numbers[wday];
}