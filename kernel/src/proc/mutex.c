#include <proc/mutex.h>
#include <proc/sched.h>

void mutex_init(mutex_t *mutex) {
    mutex->locked = 0;
}

void mutex_lock(mutex_t *mutex) {
    while (__sync_lock_test_and_set(&mutex->locked, 1)) {
        yield();
    }
}

void mutex_unlock(mutex_t *mutex) {
    __sync_lock_release(&mutex->locked);
}

bool mutex_trylock(mutex_t *mutex) {
    return __sync_lock_test_and_set(&mutex->locked, 1) == 0;
}
