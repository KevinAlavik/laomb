#pragma once

#include <video/font.h>
#include <io.h>

void print_init(uint16_t x, uint16_t y);
void print_set_position(uint16_t x, uint16_t y);
uint16_t print_get_x();
uint16_t print_get_y();
int print(const char* format, ...);

#define DEBUG(format, ...) \
    do { \
        print("[%02u-%02u-%02u ", get_day_of_month(), get_month(), get_year()); \
        print("%02u:%02u:%02u] ", get_hours(), get_minutes(), get_seconds()); \
        print("[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
        print(format "\n", ##__VA_ARGS__); \
    } while(0)

