#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>

typedef enum
{
    IDT_FLAG_GATE_TASK              = 0x5,
    IDT_FLAG_GATE_16BIT_INT         = 0x6,
    IDT_FLAG_GATE_16BIT_TRAP        = 0x7,
    IDT_FLAG_GATE_32BIT_INT         = 0xE,
    IDT_FLAG_GATE_32BIT_TRAP        = 0xF,

    IDT_FLAG_RING0                  = (0 << 5),
    IDT_FLAG_RING1                  = (1 << 5),
    IDT_FLAG_RING2                  = (2 << 5),
    IDT_FLAG_RING3                  = (3 << 5),

    IDT_FLAG_PRESENT                = 0x80,
} IDT_FLAGS;

void idt_init();
void idt_disable_gate(int interrupt);
void idt_enable_gate(int interrupt);
void idt_set_gate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags);

typedef struct
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t interrupt, error;
    uint32_t eip, cs, eflags, user_esp, ss;
} __attribute__((packed)) registers_t;

typedef void (*ISRHandler)(registers_t* regs);
typedef void (*IRQHandler)(registers_t* regs);
void idt_register_handler(int interrupt, ISRHandler handler);
void irq_register_handler(int irq, IRQHandler handler);

typedef struct
{
    uint16_t base_low;
    uint16_t segment_selector;
    uint8_t reserved;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed)) idt_pointer_t;

extern idt_pointer_t idt_ptr;

#endif