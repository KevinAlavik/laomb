#include <proc/mutex.h>
#include <proc/sched.h>
#include <string.h>

void mutex_lock(struct Mutex* mutex) {
    while (__sync_lock_test_and_set(&mutex->lock, 1)) {
        yield();
    }
    mutex->owner_pid = current_job->pid;
}

bool mutex_trylock(struct Mutex* mutex) {
    if (__sync_lock_test_and_set(&mutex->lock, 1) == 0) {
        struct JCB* current_job = current_job;
        return true;
    }
    return false;
}

void mutex_unlock(struct Mutex* mutex) {
    if (mutex->owner_pid == current_job->pid) {
        mutex->owner_pid = 0;
        __sync_lock_release(&mutex->lock);
    }
}

bool mutex_is_locked(struct Mutex* mutex) {
    return mutex->lock != 0;
}
