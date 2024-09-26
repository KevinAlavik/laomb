#include <proc/sched.h>
#include <proc/task.h>
#include <stdbool.h>
#include <kheap.h>
#include <sys/pic.h>
#include <kprintf>
#include <io.h>
#include <string.h>

static struct task *current_task = NULL;
static struct task *task_list = NULL;

void sched_add_task(struct task *new_task) {
    new_task->next = task_list;
    task_list = new_task;
}

void sched_remove_task(uint32_t pid) {
    struct task **indirect = &task_list;
    while (*indirect && (*indirect)->pid != pid) {
        indirect = &(*indirect)->next;
    }
    if (*indirect) {
        struct task *removed = *indirect;
        *indirect = removed->next;
        kfree(removed);
    }
}

void schedule() {
    struct task *next_task = current_task ? current_task->next : task_list;

    if (next_task == NULL) {
        next_task = task_list;
    }

    while (next_task != NULL && next_task->state != TASK_READY) {
        next_task = next_task->next;

        if (next_task == NULL) {
            next_task = task_list;
        }

        if (next_task == current_task) {
            break;
        }
    }

    if (next_task != NULL && next_task != current_task) {
        // kprintf("Switching to task %d, EIP: 0x%lx\n", next_task->pid, next_task->eip);
        current_task = next_task;
    } else {
        // kprintf("No task to switch to\n");
    }
}


void timer_interrupt_handler(registers_t *regs) {
    if (current_task) {
        uintptr_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        current_task->cr3 = (vmm_context_t){ .pd = (PageDirectory*)cr3 };
        current_task->cs = regs->cs; current_task->ds = regs->ds; current_task->es = regs->es; current_task->fs = regs->fs; current_task->gs = regs->gs;
        if (regs->cs != 0x08) {
            // coming from usaspac!
            current_task->ss = regs->ss;
            current_task->esp = regs->user_esp;
        }
        current_task->eflags = regs->eflags;
        current_task->eip = regs->eip;
        current_task->kernel_esp = regs->esp;
        
        if (current_task->fpu_enabled) {
            __asm__ volatile("fsave (%0)" : : "r"(&current_task->fpu_state));
        }
        current_task->registers[0] = regs->eax;
        current_task->registers[1] = regs->ebx;
        current_task->registers[2] = regs->ecx;
        current_task->registers[3] = regs->edx;
        current_task->registers[4] = regs->esi;
        current_task->registers[5] = regs->edi;
        current_task->registers[6] = regs->ebp; 

        current_task->state = TASK_READY;
    }

    schedule();

    if (current_task) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(current_task->cr3.pd));
        regs->cs = current_task->cs; regs->ds = current_task->ds; regs->es = current_task->es; regs->fs = current_task->fs; regs->gs = current_task->gs;
        if (current_task->cs != 0x08) {
            // go to usaspac!
            regs->ss = current_task->ss;
            regs->user_esp = current_task->esp;
        }
        regs->eflags = current_task->eflags;
        regs->eip = current_task->eip;
        regs->esp = current_task->kernel_esp;

        if (current_task->fpu_enabled) {
            __asm__ volatile("frstor (%0)" : : "r"(&current_task->fpu_state));
        }
        regs->eax = current_task->registers[0];
        regs->ebx = current_task->registers[1];
        regs->ecx = current_task->registers[2];
        regs->edx = current_task->registers[3];
        regs->esi = current_task->registers[4];
        regs->edi = current_task->registers[5];
        regs->ebp = current_task->registers[6];

        current_task->state = TASK_RUNNING;
    }
    pic_sendeoi(0);
}

void sched_init(struct task *callback_task) {
    sti();
    sched_add_task(callback_task);

    idt_register_handler(32, timer_interrupt_handler);
}