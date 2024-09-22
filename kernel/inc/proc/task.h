#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/gdt.h>
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
    uint32_t pid;                       // Process ID
    uint32_t ppid;                      // Parent Process ID
    uint32_t priority;                  // Task priority
    uint32_t nieche;                    // Default priority to which priority is reset when ran
    vmm_context_t cr3;                  // Pointer to the page table or directory
    uint32_t registers[7];              // General purpose registers (EAX, EBX, ECX, EDX, ESI, EDI, EBP)
    uint16_t cs, ds, es, fs, gs, ss;    // Segment selectors
    uint32_t eip;                       // Instruction pointer
    uint32_t eflags;                    // Flags register
    uint32_t esp;                       // Stack pointer  
    bool fpu_enabled;                   // Is the FPU enabled 
    uint8_t fpu_state[108];             // FPU/MMX state saved with 'fsave'
    uint32_t kernel_esp;                // Kernel mode stack pointer (for privilege level switches)

    
    enum task_state state;              // Current task state (running, ready, etc.)

    struct task* next;                  // Pointer to the next task

    //TODO: Vfs stuff
};
extern struct task* current_task;

struct task* task_create(uint32_t pid, uint32_t ppid, uint32_t priority, vmm_context_t page_directory, uintptr_t callback);
bool task_delete(uint32_t pid);
struct task* task_get(uint32_t pid);
