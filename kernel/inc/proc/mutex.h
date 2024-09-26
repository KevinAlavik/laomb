#pragma once

#include <stdbool.h>

typedef struct {
    volatile int locked;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);
bool mutex_trylock(mutex_t *mutex);
