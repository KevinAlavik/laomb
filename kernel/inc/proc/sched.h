#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/idt.h>
#include <sys/mmu.h>
#include <proc/vfs.h>

#define JOB_KERNEL_STACK_SIZE 4096

struct JCB {
    uint64_t pid;
    uint64_t ppid;

    registers_t regs;
    bool fpu_enabled;
    uint8_t fpu_state[108];
    mmu_context_t ctx;

    uint8_t* code_segment_base;
    size_t code_segment_len;
    uint8_t* data_segment_base;
    size_t data_segment_len;
    uint8_t* stack_base;
    size_t stack_len;

    char job_owner_name[70];

    uint8_t exit_code;
    
    uint64_t priority;
    uint64_t base_priority;

    // todo vfs stuff
    // todo IPC stuff

    struct JCB* next;
    struct JCB* parent;
};

enum {
    QUEUE_TYPE_BASIC = 0,
    QUEUE_TYPE_USER = 1,
    QUEUE_TYPE_IO = 2
};

struct SchedulerQueue {
    uint8_t type;
    struct JCB* head;
    struct JCB* tail;
    size_t count;
};

extern struct SchedulerQueue ready_queue;

struct IoQueue {
    struct SchedulerQueue base;
    uint8_t id;
    struct IoQueue* next;
};
extern struct IoQueue* io_queues;

struct UserQueue {
    struct SchedulerQueue base;
    uint64_t id;
    char name[70];
    struct UserQueue* next;
};
extern struct UserQueue* user_queues;

extern struct JCB* current_job;

uintptr_t job_get_jcb(uint64_t pid);

void job_reset_priority(struct JCB* job);
void job_check_priorities(struct SchedulerQueue* queue);

[[noreturn]] void scheduler_init(struct JCB* callback);
void scheduler_create_user_queue(uint64_t id, const char* name);
void scheduler_create_io_queue(uint8_t id);
void scheduler_remove_queue(struct SchedulerQueue* queue);
void scheduler_merge_queues(struct SchedulerQueue* dest, struct SchedulerQueue* src);

void scheduler_add_job(struct JCB* job);

void scheduler_enqueue(struct JCB* job, struct SchedulerQueue* queue);
#define SCHEDULER_ENQ_READY(job) scheduler_enqueue(&job, &ready_queue)

struct JCB* scheduler_create_job(uintptr_t callback, uint64_t priority);
void scheduler_terminate_job(struct JCB* job);

struct JCB* scheduler_pick_next_job(struct SchedulerQueue* queue);
extern void irq_timer_handler(registers_t* r);
extern void __kernel_reaper(); // reaps the VFS, any maybe will do more

static inline void yield() {
    __asm__ volatile ("int $32");
}