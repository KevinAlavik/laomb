#include <proc/sched.h>
#include <string.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <spinlock.h>
#include <kprintf>
#include <kheap.h>
#include <io.h>

struct JCB* job_list = nullptr;
struct JCB* current_job = nullptr;

struct spinlock sched_lock;

static void kernel_reaper_func() {
    // todo gc
    for (;;) {
        sched_yield();
    }
}

struct JCB kernel_reaper = {0};


void sched_init() {
    kernel_reaper.regs.eip = (uintptr_t)kernel_reaper_func;
    kernel_reaper.regs.cs = 0x8;
    kernel_reaper.regs.ds = 0x10;
    kernel_reaper.regs.eflags = 0x202;
    kernel_reaper.fpu_enabled = false;
    kernel_reaper.state = TASK_READY;
    kernel_reaper.priority = 19;
    kernel_reaper.ctx = kernel_page_directory;
    kernel_reaper.pid = 0xFFFFFFFFFFFFFFF;

    if (!job_list) {
        kprintf("Job list is null\n");
        return;
    }
    kernel_reaper.ctx = kernel_page_directory;
    idt_register_handler(0x20, sched_timer_tick);
    sched_yield();
}

static struct JCB* find_next_ready_job() {
    struct JCB* best_job = &kernel_reaper;
    int lowest_priority = 20;  // Priority ranges from -20 (highest) to 19 (lowest)
    
    if (!job_list) {
        return best_job;
    }

    struct JCB* job = job_list;
    do {
        if (job->state == TASK_READY) {
            if (job->aged_priority < lowest_priority) {
                lowest_priority = job->aged_priority;
                best_job = job;
            }
            
            if (job != current_job && job->aged_priority > -20) {
                job->aged_priority--;
            }
        }
        job = job->next;
    } while (job != job_list);

    if (best_job != &kernel_reaper) {
        best_job->aged_priority = best_job->priority;
    }

    return best_job;
}

static uint64_t gPid = 0;
struct JCB* sched_create_job(uintptr_t callback, uint8_t* code_base, size_t code_len, uint8_t* data_base, size_t data_len, uint32_t uid, uint32_t gid, int priority, struct JCB* parent) {
    struct JCB* new_job = (struct JCB*)kmalloc(sizeof(struct JCB));
    if (!new_job) return nullptr;

    memset(new_job, 0, sizeof(struct JCB));
    new_job->pid = gPid++;
    new_job->ppid = current_job ? current_job->pid : 0;
    new_job->uid = uid;
    new_job->gid = gid;
    new_job->priority = priority;
    new_job->code_segment_base = code_base;
    new_job->code_segment_len = code_len;
    new_job->data_segment_base = data_base;
    new_job->data_segment_len = data_len;
    new_job->stack_base = 0;
    new_job->stack_len = 0;
    new_job->kernel_stack_base = (uintptr_t)kmalloc(JOB_KERNEL_STACK_SIZE);
    new_job->state = TASK_READY;
    new_job->regs.eip = callback;
    new_job->regs.cs = 0x8;
    new_job->regs.ds = 0x10;
    new_job->regs.eflags = 0x202;
    new_job->regs.esp = (uintptr_t)(new_job->kernel_stack_base) + JOB_KERNEL_STACK_SIZE;
    new_job->fpu_enabled = false;
    new_job->ctx = kernel_page_directory;

    new_job->user_time = 0;
    new_job->system_time = 0;
    new_job->first_child = nullptr;

    // new_job->file_descriptors = (struct vnode**)kmalloc(sizeof(struct vnode*) * 20);
    // new_job->num_file_descriptors = 0;
    // new_job->max_file_descriptors = 20;

    spinlock_lock(&sched_lock);
    if (!job_list) {
        job_list = new_job;
        new_job->next = new_job;
        new_job->prev = new_job;
    } else {
        new_job->next = job_list;
        new_job->prev = job_list->prev;
        job_list->prev->next = new_job;
        job_list->prev = new_job;
    }

    if (parent) {
        if (!parent->first_child) {
            parent->first_child = new_job;
        } else {
            struct JCB* head = parent->first_child;
            while (head->next_sibling) {
                head = head->next_sibling;
            }
            head->next_sibling = new_job;
        }
        new_job->parent = parent;
    }
    spinlock_unlock(&sched_lock);

    return new_job;
}

void sched_terminate_job(struct JCB* job) {
    spinlock_lock(&sched_lock);
    struct JCB* parent = job->parent;
    if (parent) {
        if (parent->first_child == job) {
            parent->first_child = job->next_sibling; // can be null, we dont care
        } else {
            struct JCB* head = parent->first_child;
            while (head && head->next_sibling != job) {
                head = head->next_sibling;
            }
            if (head) {
                head->next_sibling = job->next_sibling;
            }
        }
    }
    spinlock_unlock(&sched_lock);

    kfree((void*)job->kernel_stack_base);
    // kfree(job->file_descriptors);
    kfree(job);
}

void sched_yield() {
    __asm__ volatile ("int $0x20");
}

void sched_context_switch(registers_t* r, struct JCB* next_job) {
    size_t size = sizeof(registers_t);
    if (next_job->regs.cs != 0x20) {
        size -= sizeof(uint32_t) * 2;
    }
    memcpy(r, &next_job->regs, size);
    if (next_job->fpu_enabled) {
        __asm__ volatile ("frstor %0" : : "m"(next_job->fpu_state));   
    }

    return;
}

void sched_timer_tick(registers_t* r) {
    if (__builtin_expect(!!(current_job), 1)) { // this is not gonna run ONCE
        current_job->state = TASK_READY;
    }
    struct JCB* next_job = find_next_ready_job(); // this should also adjust the scheduling, so for priority do aging

    if (next_job) {

        if (next_job == current_job) {
            pic_sendeoi(0);
            return;
        }

        if (__builtin_expect(!!(current_job), 1)) {
            if (r->cs == 0x20) {
                current_job->user_time++;
            } else {
                current_job->system_time++;
            }
            size_t size = sizeof(registers_t);
            if (r->cs != 0x20) {
                size -= sizeof(uint32_t) * 2;
            }
            memcpy(&current_job->regs, r, size);
            if (current_job->fpu_enabled) {
                __asm__ volatile ("fsave %0" : : "m"(current_job->fpu_state));   
            }
        }

        current_job = next_job;
        sched_context_switch(r, next_job);

    }
    pic_sendeoi(0);
}

void sched_set_priority(struct JCB* job, int priority) {
    job->priority = priority;
}

struct JCB* sched_get_current_job() {
    return current_job;
}

void sched_block_current() {
    spinlock_lock(&sched_lock);

    if (current_job) {
        current_job->state = TASK_BLOCKED;
        kprintf("Job %llu blocked\n", current_job->pid);
    }

    spinlock_unlock(&sched_lock);

    sched_yield();
}

void sched_unblock_job(struct JCB* job) {
    spinlock_lock(&sched_lock);

    if (job && job->state == TASK_BLOCKED) {
        job->state = TASK_READY;
        kprintf("Job %llu unblocked\n", job->pid);
    }

    spinlock_unlock(&sched_lock);
}