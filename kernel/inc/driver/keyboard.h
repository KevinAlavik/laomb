#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/idt.h>

#define MAX_INPUT_BUFFER_SIZE 256

void keyboard_init(void);
char keyboard_getchar(void);
size_t keyboard_gets(char* buffer, size_t max_length);
void keyboard_clear_buffer(void);
int keyboard_buffer_size(void);
int keyboard_key_pressed(void);
void keyboard_set_repeat_rate(uint8_t rate);
void keyboard_reset(void);
