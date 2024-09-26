#pragma once

#include <proc/task.h>
#include <sys/idt.h>
#include <stdint.h>

void sched_init(struct task *callback_task);

void sched_add_task(struct task *new_task);
void sched_remove_task(uint32_t pid);

void timer_interrupt_handler(registers_t* regs);

static inline void yield() {
    __asm__ volatile ("int $0x20");
}
