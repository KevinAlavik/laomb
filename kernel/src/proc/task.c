#include <proc/task.h>
#include <stdbool.h>
#include <kheap.h>
#include <kprintf>
#include <string.h>

struct task* current_task;

static uintptr_t setup_stack() {
    uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
    return stack + STACK_SIZE;
}

struct task* task_create(uint32_t pid, uint32_t ppid, uint32_t priority, vmm_context_t page_directory, uintptr_t callback)
{
    struct task* new_task = (struct task*)kmalloc(sizeof(struct task));
    new_task->pid = pid;
    new_task->ppid = ppid;
    new_task->priority = priority;
    new_task->nieche = priority;
    new_task->cr3 = page_directory;

    memset(new_task->registers, 0, sizeof(new_task->registers));

    new_task->eip = (uint32_t)callback;
    new_task->esp = setup_stack();
    
    new_task->state = TASK_READY;
    new_task->next = nullptr;

    if (current_task == nullptr) {
        current_task = new_task;
    } else if (current_task->next == nullptr) {
        current_task->next = new_task;
    } else {
        struct task* temp = current_task;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_task;
    }

    return new_task;
}

bool task_delete(uint32_t pid) {
    struct task* target = task_get(pid);
    
    if (!target) {
        return false;
    }
    
    if (current_task->pid == pid) {
        struct task* to_delete = current_task;
        current_task = current_task->next;
        kfree(to_delete);
        return true;
    }

    struct task* prev = current_task;
    struct task* temp = current_task->next;

    while (temp != NULL) {
        if (temp->pid == pid) {
            prev->next = temp->next;
            kfree(temp);
            return true;
        }
        prev = temp;
        temp = temp->next;
    }
    
    kfree(target);
    return true;
}

struct task* task_get(uint32_t pid) {
    struct task* t = current_task;
    
    while (t != NULL) {
        if (t->pid == pid) {
            return t;
        }
        t = t->next;
    }
    
    return NULL;
}