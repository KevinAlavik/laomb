#include <stdint.h>
#include <kprintf>
#include <io.h>
#include <string.h>

#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <ultra_protocol.h>

void key(registers_t* ) {
    kprintf("Key!\n");
    pic_sendeoi(1);
}

[[noreturn]] void _start(struct ultra_boot_context *, uint32_t magic)
{
    cli();
    kprintf("Hyper Bootloader Magic: 0x%08x\n", magic);

    gdt_init();
    idt_init();

    pic_mask(0);
    irq_register_handler(1, key);
    
    sti();
    for(;;) hlt();
}