#pragma once

#include <stdatomic.h>
#include <stdint.h>

struct spinlock {
    atomic_int lock;
};

void spinlock_lock(struct spinlock *lock);
void spinlock_unlock(struct spinlock *lock);