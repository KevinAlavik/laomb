#include <proc/sched.h>
#include <kheap.h>
#include <kprintf>
#include <sys/mmu.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <string.h>
#include <io.h>

struct SchedulerQueue ready_queue = {QUEUE_TYPE_BASIC, NULL, NULL, 0};
struct IoQueue* io_queues = NULL;
struct UserQueue* user_queues = NULL;
static size_t io_queue_count = 0;
static size_t user_queue_count = 0;

struct JCB* current_job = NULL;

uintptr_t job_get_jcb(uint64_t pid)
{
    struct JCB* head = ready_queue.head;
    while (head != NULL) {
        if (head->pid == pid) {
            return (uintptr_t)head;
        }
        head = head->next;
    }
    for (size_t i = 0; i < io_queue_count; i++) {
        for (size_t j = 0; j < io_queues[i].base.count; j++) {
            head = io_queues[i].base.head;
            while (head != NULL) {
                if (head->pid == pid) {
                    return (uintptr_t)head;
                }
                head = head->next;
            }
        }
    }
    for (size_t i = 0; i < user_queue_count; i++) {
        for (size_t j = 0; j < user_queues[i].base.count; j++) {
            head = user_queues[i].base.head;
            while (head != NULL) {
                if (head->pid == pid) {
                    return (uintptr_t)head;
                }
                head = head->next;
            }
        }
    }
    return 0;
}

void job_reset_priority(struct JCB* job) {
    job->priority = job->base_priority;
}

void job_check_priorities(struct SchedulerQueue* queue) {
    struct JCB* current = queue->head;
    while (current) {
        current->priority--;
        current = current->next;
    }
}

struct JCB kernel_reaper = (struct JCB) {
    .pid = 0,
    .ppid = 0,
    .regs = (registers_t){0},
    .fpu_enabled = false,
    .fpu_state = {0},
    .ctx = {0},
    .code_segment_base = NULL,
    .code_segment_len = 0,
    .data_segment_base = NULL,
    .data_segment_len = 0,
    .stack_base = NULL,
    .stack_len = 0,
    .exit_code = 0,
    .priority = 0,
    .base_priority = 0,
    .next = NULL,
    .parent = NULL
};

void scheduler_init(struct JCB* callback)
{
    kernel_reaper.ctx = kernel_page_directory;
    kernel_reaper.stack_base = kmalloc(JOB_KERNEL_STACK_SIZE);
    kernel_reaper.stack_len = JOB_KERNEL_STACK_SIZE;
    kernel_reaper.regs.eip = (uintptr_t)__kernel_reaper;
    kernel_reaper.regs.esp = (uintptr_t)kernel_reaper.stack_base + JOB_KERNEL_STACK_SIZE;
    ready_queue.head = callback;
    ready_queue.tail = callback;
    ready_queue.count = 1;

    idt_register_handler(32, irq_timer_handler);
    sti();

    yield();

    // should be unreachable
    cli();
    kprintf("Scheduler initialization failed!\n");
    for (;;);
    __builtin_unreachable();
}

void scheduler_create_user_queue(uint64_t id, const char* name)
{
    user_queues = (struct UserQueue*)krealloc(user_queues, sizeof(struct UserQueue) * (user_queue_count + 1));
    user_queues[user_queue_count].id = id;
    int len = strlen(name) + 1;
    strncpy(user_queues[user_queue_count].name, name, len > 70 ? 70 : len);
    user_queues[user_queue_count].base.head = NULL;
    user_queues[user_queue_count].base.tail = NULL;
    user_queues[user_queue_count].base.count = 0;
    user_queue_count++;
}

void scheduler_create_io_queue(uint8_t id)
{
    io_queues = (struct IoQueue*)krealloc(io_queues, sizeof(struct IoQueue) * (io_queue_count + 1));
    io_queues[io_queue_count].id = id;
    io_queues[io_queue_count].base.head = NULL;
    io_queues[io_queue_count].base.tail = NULL;
    io_queues[io_queue_count].base.count = 0;
    io_queue_count++;
}

void scheduler_remove_queue(struct SchedulerQueue* queue)
{
    for (size_t i = 0; i < io_queue_count; i++) {
        if (io_queues[i].id == ((struct IoQueue*)queue)->id) {
            io_queues[i] = io_queues[io_queue_count - 1];
            if (i < io_queue_count - 1) {
                for (size_t j = i; j < io_queue_count - 1; j++) {
                    io_queues[j] = io_queues[j + 1];
                }
            }
            io_queue_count--;
            return;
        }
    }
    for (size_t i = 0; i < user_queue_count; i++) {
        if (user_queues[i].id == ((struct UserQueue*)queue)->id) {
            user_queues[i] = user_queues[user_queue_count - 1];
            if (i < user_queue_count - 1) {
                for (size_t j = i; j < user_queue_count - 1; j++) {
                    user_queues[j] = user_queues[j + 1];
                }
            }
            user_queue_count--;
            return;
        }
    }
    kprintf("Queue %p not found\n", queue);
    return;
}

void scheduler_merge_queues(struct SchedulerQueue* dest, struct SchedulerQueue* src)
{
    if (dest->tail) {
        dest->tail->next = src->head;
    } else {
        dest->head = src->head;
    }
    if (src->head) {
        src->head->parent = dest->tail;
    }
    dest->tail = src->tail;
    src->head = NULL;
    src->tail = NULL;
    dest->count += src->count;
}


void scheduler_add_job(struct JCB* job)
{
    scheduler_enqueue(job, &ready_queue);
}

void scheduler_enqueue(struct JCB* job, struct SchedulerQueue* queue)
{
    job->next = NULL;
    if (queue->tail) {
        queue->tail->next = job;
    } else {
        queue->head = job;
    }
    queue->tail = job;
    queue->count++;
}

static void remove_job_from_queue(struct JCB** head, struct JCB* job) {
    struct JCB* current = *head;
    struct JCB* prev = NULL;

    while (current) {
        if (current == job) {
            if (prev) {
                prev->next = current->next;
            } else {
                *head = current->next;
            }
            return;
        }
        prev = current;
        current = current->next;
    }
}

static uint64_t last_pid = 1;
struct JCB* scheduler_create_job(uintptr_t callback, uint64_t priority)
{
    struct JCB* job = (struct JCB*)kmalloc(sizeof(struct JCB));
    job->pid = last_pid++;
    job->ppid = 0;
    memset(&job->regs, 0, sizeof(job->regs));
    job->fpu_enabled = false;
    memset(job->fpu_state, 0, sizeof(job->fpu_state));
    strncpy(job->job_owner_name, "root", 70);
    job->ctx = kernel_page_directory;
    job->code_segment_base = NULL;
    job->code_segment_len = 0;
    job->data_segment_base = NULL;
    job->data_segment_len = 0;
    job->stack_base = kmalloc(JOB_KERNEL_STACK_SIZE);
    job->stack_len = JOB_KERNEL_STACK_SIZE;
    job->exit_code = 0;
    job->priority = priority;
    job->base_priority = priority;
    job->next = NULL;
    job->parent = NULL;
    job->regs.esp = (uintptr_t)job->stack_base + JOB_KERNEL_STACK_SIZE;
    job->regs.eip = (uint32_t)callback;
    job->regs.cs = 0x8;
    job->regs.ds = 0x10;
    job->regs.eflags = 0x202;
    return job;
}

void scheduler_terminate_job(struct JCB* job)
{
    remove_job_from_queue(&ready_queue.head, job);
    for (size_t i = 0; i < io_queue_count; i++) {
        remove_job_from_queue(&io_queues[i].base.head, job);
    }
    for (size_t i = 0; i < user_queue_count; i++) {
        remove_job_from_queue(&user_queues[i].base.head, job);
    }
    kfree(job->stack_base);
    kfree(job);
}

struct JCB* scheduler_pick_next_job(struct SchedulerQueue* queue) {
    struct JCB* lowest_priority_job = NULL;
    uint64_t lowest_priority = UINT64_MAX;

    job_check_priorities(&ready_queue);

    struct JCB* current = queue->head;
    
    while (current) {
        if (current->priority < lowest_priority) {
            lowest_priority = current->priority;
            lowest_priority_job = current;
        }
        current = current->next;
    }

    if (lowest_priority_job) {
        job_reset_priority(lowest_priority_job);
    } else {
        lowest_priority_job = &kernel_reaper;
    }

    return lowest_priority_job;
}

extern void irq_timer_handler(registers_t* r)
{
    job_check_priorities(&ready_queue);
    struct JCB* job = scheduler_pick_next_job(&ready_queue);
    if (job) {
        if (!current_job) {
            current_job = job;
            size_t size = sizeof(registers_t);
            if (job->regs.cs != 0x20) {
                size -= sizeof(uint32_t)*2;
            }
            memcpy(r, &job->regs, size);
            if (job->fpu_enabled) {
                __asm__ volatile ("frstor %0" : : "m"(job->fpu_state));   
            }
            pic_sendeoi(0);
            return;
        }
        else if (job->pid == current_job->pid) {
            pic_sendeoi(0);
            return;
        } else {
            job_reset_priority(job);

            size_t size = sizeof(registers_t);
            if (current_job->regs.cs != 0x20) {
                size -= sizeof(uint32_t)*2;
            }
            memcpy(&current_job->regs, r, size);
            if (current_job->fpu_enabled) {
                __asm__ volatile ("fxsave %0" : : "m"(current_job->fpu_state));   
            }
            
            current_job = job;

            size = sizeof(registers_t);
            if (job->regs.cs != 0x20) {
                size -= sizeof(uint32_t)*2;
            }
            memcpy(r, &job->regs, size);
            if (job->fpu_enabled) {
                __asm__ volatile ("frstor %0" : : "m"(job->fpu_state));   
            }
        }
    } else {
        kprintf("Find job somehow returned NULL\n");
        cli();
        for(;;) hlt();
    }
    pic_sendeoi(0);
}

extern void __kernel_reaper()
{
    // perform garbage collection
    for(;;) yield();
}