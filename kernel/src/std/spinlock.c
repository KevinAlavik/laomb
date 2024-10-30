#include <spinlock.h>

void spinlock_lock(struct spinlock *lock) {
    while (atomic_exchange(&lock->lock, 1) == 1)
        ;
}

void spinlock_unlock(struct spinlock *lock) {
    atomic_store(&lock->lock, 0);
}