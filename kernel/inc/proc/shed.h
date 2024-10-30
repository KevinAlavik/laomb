#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/idt.h>
#include <sys/mmu.h>

#define JOB_KERNEL_STACK_SIZE 4096

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_STOPPED,
    TASK_ZOMBIE
} task_state_t;

struct JCB {
    uint64_t pid;                       // Process ID
    uint64_t ppid;                      // Parent process ID
    uint64_t pgid;                      // Process group ID (used for job control)
    uint64_t sid;                       // Session ID

    registers_t regs;
    bool fpu_enabled;
    uint8_t fpu_state[108];
    mmu_context_t ctx;

    uint32_t uid;                       // User ID of the job owner
    uint32_t gid;                       // Group ID of the job owner
    uint32_t permissions;               // Permissions flags

    uint8_t* code_segment_base;
    size_t code_segment_len;
    uint8_t* data_segment_base;
    size_t data_segment_len;
    uint8_t* stack_base;
    size_t stack_len;
    
    // todo posix and unix stuff
    task_state_t state;
    int priority;                       // Unix priority value (-20 to 19)
    uint64_t time_slice;                // Time slice allocated to this task in ms
    uint64_t user_time;                 // CPU time used in user mode (in ticks)
    uint64_t system_time;               // CPU time used in system mode (in ticks)


    // TODO: once IPC is implemented
    // uint64_t signal_mask;               // Mask of blocked signals
    // uint64_t pending_signals;           // Pending signals
    // void (*signal_handlers[32])(int);   // Array of signal handlers
    // int message_queue_id;               // ID of an IPC message queue
    // void* shared_memory_ptr;            // Pointer to shared memory segment

    // TODO: once VFS
    // int* file_descriptors;              // Array of file descriptors

    struct JCB* threads;                // Circular linked list, where last thread points back to the first / master thread
    uint32_t thread_index;

    struct JCB* first_child;
    struct JCB* next_sibling;
    struct JCB* parent;
};

typedef enum {
    SCHED_RR,               // Round Robin scheduling
    SCHED_PRIORITY,         // Priority-based scheduling
    SCHED_REALTIME          // Real-time scheduling policy
} sched_policy_t;

#define SCHED_TIME_SLICE_DEFAULT 10

extern sched_policy_t global_sched_policy;

/**
 * Initialize the scheduler.
 * Should be called once at the start of the system.
 * 
 * @param job0 The first job that will be created in the scheduler, pid 0, parent of all
 */
void sched_init(struct JCB* job0);

/**
 * Create a new job.
 * 
 * @param code_base Pointer to the code segment.
 * @param code_len Length of the code segment.
 * @param data_base Pointer to the data segment.
 * @param data_len Length of the data segment.
 * @param uid User ID owning the job.
 * @param gid Group ID owning the job.
 * @param priority Initial priority of the job.
 * 
 * @return Pointer to the created JCB, or NULL on failure.
 */
struct JCB* sched_create_job(uint8_t* code_base, size_t code_len, uint8_t* data_base, size_t data_len, uint32_t uid, uint32_t gid, int priority);

/**
 * Terminate a job and remove it from the scheduler.
 * 
 * @param job Pointer to the JCB to terminate.
 */
void sched_terminate_job(struct JCB* job);

/**
 * Add a new thread to an existing job.
 * 
 * @param job Pointer to the job to which a thread is added.
 * 
 * @return Pointer to the newly created thread JCB, or NULL on failure.
 */
struct JCB* sched_create_thread(struct JCB* job);

/**
 * Terminate a thread in a job.
 * 
 * @param thread Pointer to the thread JCB to terminate.
 */
void sched_terminate_thread(struct JCB* thread);

/**
 * Set the scheduling policy for a job.
 * 
 * @param policy Scheduling policy (e.g., SCHED_RR, SCHED_PRIORITY).
 */
void sched_set_policy(sched_policy_t policy);

/**
 * Yield the CPU to the next eligible thread.
 */
void sched_yield();

/**
 * Perform a context switch to the next job/thread.
 * -> called by the timer interrupt or scheduler loop.
 */
void sched_context_switch(registers_t* r);

/**
 * Set the priority for a job.
 * 
 * @param job Pointer to the JCB.
 * @param priority Priority value (-20 to 19).
 */
void sched_set_priority(struct JCB* job, int priority);

/**
 * Block the current job.
 * This function places the calling job into a blocked state.
 */
void sched_block_current();

/**
 * Unblock a job.
 * 
 * @param job Pointer to the JCB to unblock.
 */
void sched_unblock_job(struct JCB* job);

/**
 * Get the current job.
 * 
 * @return Pointer to the current JCB.
 */
struct JCB* sched_get_current_job();

/**
 * Get the current thread index
 * 
 * @return Pointer to the current thread JCB.
 */
uint32_t sched_get_current_thread();

/**
 * Get the state of a job.
 * 
 * @param job Pointer to the JCB.
 * @return The current state of the job (TASK_RUNNING, TASK_BLOCKED, etc.).
 */
task_state_t sched_get_job_state(struct JCB* job);

/**
 * Timer tick handler to be called by the system clock interrupt.
 * This function will update time slices and trigger a context switch if necessary.
 */
void sched_timer_tick(registers_t* r);

/**
 * Set the time slice for a job (in ms).
 * Used for Round-Robin scheduling.
 * 
 * @param job Pointer to the JCB.
 * @param time_slice Time slice in milliseconds.
 */
void sched_set_time_slice(struct JCB* job, uint64_t time_slice);