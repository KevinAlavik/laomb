#pragma once

#include <stdint.h>
#include <stddef.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define BYTES_PER_PIXEL 3
#define FONT_SIZE 8

void font_draw_char(uint16_t x, uint16_t y, char c, uint32_t color);
void font_draw_string(uint16_t x, uint16_t y, const char *str, uint32_t color);