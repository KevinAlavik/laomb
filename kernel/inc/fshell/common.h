#pragma once

#include <stdint.h>
#include <sys/idt.h>
#include <proc/sched.h>
#include <proc/mutex.h>
#include <kheap.h>

#define FSHELL_BUFFER_SIZE 512
#define WRITE_MODE_TRUNCATE 0
#define WRITE_MODE_APPEND   1

struct fshell_ctx {
    char buffer[FSHELL_BUFFER_SIZE];// text buffer
    uint32_t buffer_index;          // index of the next character to be written
    int count;                      // number of characters in the buffer
    int shift_pressed;              // is shift pressed
    int ctrl_pressed;               // is ctrl pressed
    char pwd[256];                  // current working directory
};

extern struct fshell_ctx fshell_ctx;

void fshell_interrupt_handler(registers_t*);
char getchar_locking();