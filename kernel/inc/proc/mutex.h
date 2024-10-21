#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <proc/sched.h>

#define MUTEX_NAME_SIZE 64

struct Mutex {
    volatile uint8_t lock;
    char name[MUTEX_NAME_SIZE]; // Descriptive name for debugging purposes
    uint64_t owner_pid; // PID of the job currently holding the mutex
};

void mutex_init(struct Mutex* mutex, const char* name);
void mutex_lock(struct Mutex* mutex);
bool mutex_trylock(struct Mutex* mutex);
void mutex_unlock(struct Mutex* mutex);
bool mutex_is_locked(struct Mutex* mutex);
