#pragma once

#include <stdint.h>
#include <ultra_protocol.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define SCREEN_TAB_SIZE 4

extern struct ultra_framebuffer_attribute* framebuffer;

void putc(char c);
void puts(const char* str);
void cls();
void set_foreground_color(uint32_t color);
void set_background_color(uint32_t color);
void set_colors(uint32_t fg_color, uint32_t bg_color);

void set_pixel(uint32_t x, uint32_t y, uint32_t color);  
void set_cursor(uint32_t x, uint32_t y);