#include <sys/idt.h>
#include <sys/pic.h>
#include <kprintf>
#include <io.h>

#define PIC_REMAP_OFFSET        0x20

__attribute__((aligned(16))) idt_entry_t idt[256];
idt_pointer_t idt_ptr = { sizeof(idt) - 1, (uintptr_t)&idt };

handler handlers[256];

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

void idt_set_gate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags)
{
    idt[interrupt].base_low = ((uint32_t)base) & 0xFFFF;
    idt[interrupt].segment_selector = segmentDescriptor;
    idt[interrupt].reserved = 0;
    idt[interrupt].flags = flags;
    idt[interrupt].base_high = ((uint32_t)base >> 16) & 0xFFFF;
}

void idt_init()
{
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, isr_table[i], 0x08 /* GDT Kernel Code */, IDT_FLAG_RING0 | IDT_FLAG_GATE_32BIT_INT);
        idt[i].flags |= IDT_FLAG_PRESENT;
    }
    pic_init(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);
    
    __asm__ volatile ("lidt %0" : : "m" (idt_ptr) : "memory");
}

void idt_default_handler(registers_t* regs)
{
    if (handlers[regs->interrupt] != nullptr) {
        handlers[regs->interrupt](regs);
        return;
    }

    if (regs->interrupt < 32) {
        kprintf("Unhandled exception %d: %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
        kprintf("EAX=%08x EBX=%08x ECX=%08x EDX=%08x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
        kprintf("ESI=%08x EDI=%08x EBP=%08x ESP=%08x\n", regs->esi, regs->edi, regs->ebp, regs->esp);
        kprintf("EIP=%08x EFL=%08x\n", regs->eip, regs->eflags);
        kprintf("CS=%04x DS=%04x ES=%04x FS=%04x GS=%04x\n", regs->cs, regs->ds, regs->es, regs->fs, regs->gs);
        
        if (regs->cs == 0x20)
            kprintf("SS =%04x ESP_USER=%08x\n", regs->ss, regs->user_esp);

        kprintf("CR2=%08x CR3=%08x\n", regs->cr2, regs->cr3);
        cli();
        for (;;) hlt();
    } else if (regs->interrupt >= PIC_REMAP_OFFSET && regs->interrupt < PIC_REMAP_OFFSET + 0x20) {
        // ..? It doesnt work
        pic_sendeoi(regs->interrupt - PIC_REMAP_OFFSET);
    }
}

void idt_register_handler(int interrupt, handler handler)
{
    handlers[interrupt] = handler;
    idt[interrupt].flags |= IDT_FLAG_PRESENT;
}