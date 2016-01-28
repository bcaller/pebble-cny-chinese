#pragma once
#include <pebble.h>

#define YUE 'Y'
#define RI 'R'
#define XING 'X'
#define QI 'Q'
#define DIAN '['
#define BAN 'b'
#define TIAN 'T'

extern char chinese_numbers[12];
void chinese_date_string(char* date_buffer, struct tm *tick_time);
void chinese_time_string(char* time_buffer, struct tm *tick_time);
char chinese_weekday_number(struct tm *tick_time);