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
    if (handlers[regs->interrupt] != nullptr)
        handlers[regs->interrupt](regs);

    else if (regs->interrupt >= 32)
        kprintf("Unhandled interrupt %d!\n", regs->interrupt);

    else
    {
        if (regs->cs != 0x08) {
            kprintf("Unhandled exception %d %s from usermode\n", regs->interrupt, g_Exceptions[regs->interrupt]);
            kprintf("KERNEL PANIC!\n");
            // TODO: Have the userspace manager kill it?
            cli();
            for(;;) hlt();
        }

        kprintf("Unhandled exception %d %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
        
        kprintf("  eax = 0x%lx  ebx = 0x%lx  ecx = 0x%lx  edx = 0x%lx  esi = 0x%lx  edi = 0x%lx\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

        kprintf("  esp = 0x%lx  ebp = 0x%lx  eip = 0x%lx  eflags = 0x%lx\n",
               regs->esp, regs->esp, regs->eip, regs->eflags);

        kprintf("  cs = 0x%x  ds = 0x%x  es = 0x%x  fs = 0x%x  gs = 0x%x\n", 
               regs->cs, regs->ds, regs->es, regs->fs, regs->gs);

        uint32_t cr2;
        uint32_t cr0;
        uint32_t cr3;
        __asm__ volatile ("mov %%cr2, %0" : "=r" (cr2));
        __asm__ volatile ("mov %%cr0, %0" : "=r" (cr0));
        __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
        kprintf("  cr2 = 0x%lx  cr0 = 0x%lx  cr3 = 0x%lx\n", cr2, cr0, cr3);

        kprintf("  interrupt = 0x%x  errorcode = 0x%x\n", regs->interrupt, regs->error);

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

    if (irq_handlers[irq] != nullptr)
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