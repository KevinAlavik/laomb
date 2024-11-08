#include <video/print.h>
#include <stdarg.h>
#include <stdint.h>
#include <kprintf>
#include <kheap.h>

static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;

void print_init(uint16_t x, uint16_t y) {
    cursor_x = x;
    cursor_y = y;
}

void print_set_position(uint16_t x, uint16_t y) {
    cursor_x = x;
    cursor_y = y;
}

uint16_t print_get_x() {
    return cursor_x;
}

uint16_t print_get_y() {
    return cursor_y;
}

static void output_func(const char* buffer) {
    for (const char* p = buffer; *p != '\0'; ++p) {
        char character = *p;
        if (character == '\n') {
            cursor_x = 0;
            cursor_y += 1;
        } else if (character == '\t') {
            cursor_x += 4;
        } else if (character == '\b') {
            if (cursor_x > 0) {
                cursor_x -= 1;
            } else if (cursor_y > 0) {
                cursor_y -= 1;
                cursor_x = SCREEN_WIDTH - 1;
            }
            font_draw_char(cursor_x, cursor_y, '~'+1, 0x000000);
        } else {
            font_draw_char(cursor_x, cursor_y, character, 0xFFFFFF);
            cursor_x += 1;
            if (cursor_x >= (SCREEN_WIDTH / 8)) {
                cursor_x = 0;
                cursor_y += 1;
            }
        }
    }
}

int print(const char* format, ...) {
    va_list args;
    va_start(args, format);

    int count = kvsnprintf(nullptr, 0, format, args);
    if (count < 0) {
        va_end(args);
        return -1; 
    }

    char* buffer = (char*)kmalloc(count + 1);
    if (!buffer) {
        va_end(args);
        return -1;
    }

    va_start(args, format);
    kvsnprintf(buffer, count + 1, format, args);
    va_end(args);

    output_func(buffer);

    kfree(buffer);

    return count;
}
