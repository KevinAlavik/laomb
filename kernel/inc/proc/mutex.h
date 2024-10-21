#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <proc/sched.h>

#define MUTEX_NAME_SIZE 64

struct Mutex {
    volatile uint8_t lock;
    uint64_t owner_pid;
};

void mutex_lock(struct Mutex* mutex);
bool mutex_trylock(struct Mutex* mutex);
void mutex_unlock(struct Mutex* mutex);
bool mutex_is_locked(struct Mutex* mutex);
