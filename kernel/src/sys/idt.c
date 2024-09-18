#include <sys/idt.h>
#include <sys/gdt.h>
#include <sys/pic.h>
#include <kprintf>
#include <string.h>
#include <io.h>

#define PIC_REMAP_OFFSET        0x20

__attribute__((aligned(16))) idt_entry_t idt[256];

idt_pointer_t idt_ptr = { sizeof(idt) - 1, (uintptr_t)&idt };

ISRHandler handlers[256];
IRQHandler irq_handlers[16];

static const char* const g_Exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};

extern void *isr_table[];
void irq_default_handler(registers_t* regs);

void idt_init()
{
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, isr_table[i], 0x08 /* GDT Kernel Code */, IDT_FLAG_RING0 | IDT_FLAG_GATE_32BIT_INT);
        idt_enable_gate(i);
    }
    pic_init(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    for (int i = 0; i < 16; i++)
        idt_register_handler(PIC_REMAP_OFFSET + i, irq_default_handler);
    
    __asm__ volatile ("lidt %0" : : "m" (idt_ptr) : "memory");
}

void idt_default_handler(registers_t* regs)
{
    if (handlers[regs->interrupt] != NULL)
        handlers[regs->interrupt](regs);

    else if (regs->interrupt >= 32)
        kprintf("Unhandled interrupt %d!\n", regs->interrupt);

    else
    {
        if (regs->cs == 0x18) {
            // we came from usermode
            kprintf("USERMODE!!!\n");
            kprintf("SS=%x, ESP=%x\n", regs->ss, regs->user_esp);
        }

        kprintf("Unhandled exception %d %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
        
        kprintf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

        kprintf("  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x\n",
               regs->esp, regs->esp, regs->eip, regs->eflags, regs->cs, regs->ds);

        kprintf("  interrupt=%x  errorcode=%x\n", regs->interrupt, regs->error);

        kprintf("KERNEL PANIC!\n");
        cli();

        for(;;) hlt();
    }
}
void irq_default_handler(registers_t* regs)
{
    int irq = regs->interrupt - PIC_REMAP_OFFSET;
    
    uint8_t pic_isr = pic_read_isr();
    uint8_t pic_irr = pic_read_irqrr();

    if (irq_handlers[irq] != NULL)
    {
        irq_handlers[irq](regs);
    }
    else
    {
        if (irq != 0)
            kprintf("Unhandled IRQ %d  ISR=%x  IRR=%x...\n", irq, pic_isr, pic_irr);
        // Stupid message
    }

    pic_sendeoi(irq);
}

void idt_set_gate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags)
{
    idt[interrupt].base_low = ((uint32_t)base) & 0xFFFF;
    idt[interrupt].segment_selector = segmentDescriptor;
    idt[interrupt].reserved = 0;
    idt[interrupt].flags = flags;
    idt[interrupt].base_high = ((uint32_t)base >> 16) & 0xFFFF;
}

void idt_enable_gate(int interrupt)
{
    idt[interrupt].flags |= IDT_FLAG_PRESENT;
}

void idt_disable_gate(int interrupt)
{
    idt[interrupt].flags &= ~IDT_FLAG_PRESENT;
}

void idt_register_handler(int interrupt, ISRHandler handler)
{
    handlers[interrupt] = handler;
    idt_enable_gate(interrupt);
}

void irq_register_handler(int irq, IRQHandler handler)
{
    irq_handlers[irq] = handler;
}