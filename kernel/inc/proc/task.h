#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/gdt.h>
#include <kheap.h>
#include <sys/mm/vmm.h>

#define STACK_SIZE 0x1000

enum task_state {
    TASK_RUNNING,
    TASK_READY,
    TASK_WAITING,
    TASK_BLOCKED,
    TASK_TERMINATED
};

struct task {
    struct task *next;                  // linked list baby

    uint32_t pid;                       // Process ID
    uint32_t ppid;                      // Parent Process ID
    uint32_t priority;                  // Task priority
    uint32_t nieche;                    // Default priority to which priority is reset when ran
    vmm_context_t cr3;                  // Pointer to the page directory of the task
    uint32_t registers[7];              // General purpose registers (EAX, EBX, ECX, EDX, ESI, EDI, EBP)
    uint16_t cs, ds, es, fs, gs, ss;    // Segment selectors
    uint32_t eip;
    uint32_t eflags;
    uint32_t esp;                       // Userspace Stack pointer 
    bool fpu_enabled;                   // Is the FPU enabled for userspace tasks?
    uint8_t fpu_state[108];             // FPU/MMX state saved with 'fsave'
    uint32_t kernel_esp;                // Kernel mode stack pointer (userspace shananigans)
    
    enum task_state state;              // Current task state (running, ready, etc.)

    //TODO: Vfs stuff
};

static inline uintptr_t setup_stack() {
    uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
    return stack + STACK_SIZE;
}

static inline struct task task_create(uintptr_t callback, uint32_t pid, uint32_t ppid, uint32_t priority, vmm_context_t cr3) {
    struct task new_task = {
        .pid = pid,
        .ppid = ppid,
        .priority = priority,
        .nieche = priority,
        .cr3 = cr3,
        .registers = {0, 0, 0, 0, 0, 0, 0},
        .cs = 0x08, .ds = 0x10, .es = 0x10, .fs = 0x00, .gs = 0x0 /* gs, fs are reserved */, .ss = 0x10, // all tasks start in kernel mode
        .eip = callback, .eflags = 0x202, .esp = 0x00,
        .fpu_enabled = false,
        .fpu_state = {0},
        .kernel_esp = setup_stack(),
        .state = TASK_READY
    };
    return new_task;
}