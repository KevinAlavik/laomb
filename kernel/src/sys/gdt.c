#include <sys/gdt.h>
#include <kprintf>
#include <string.h>

gdt_t gdt;
gdt_pointer_t gdt_ptr;
tss_t tss = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void gdt_load() {
    kprintf(" -> Loading GDTR and flushing segment selectors\n");
    __asm__ volatile (
        "lgdtl (%0)\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"      
        "pushl $0x08\n"
        "pushl $1f\n"
        "retf\n"
        "1:\n"
        : : "r" (&gdt_ptr)
        : "ax"
    );
    kprintf(" -> Loading TSS into the task register\n");
    __asm__ volatile ("ltr %0" : : "rm" ((uint16_t)0x28) : "memory");
}

void gdt_init() {
    kprintf("<> Initializing GDT\n");
    gdt.entries[0] = (gdt_entry_t){0,0,0,0,0,0};  // Null segment

    gdt.entries[1] = (gdt_entry_t){               // Kernel Code32
        .limit = 0xFFFF,
        .base_low = 0x0000,
        .base_mid = 0x00,
        .access = 0x9A,
        .granularity = 0xC0,
        .base_high = 0x00
    };

    gdt.entries[2] = (gdt_entry_t){               // Kernel Data32
        .limit = 0xFFFF,
        .base_low = 0x0000,
        .base_mid = 0x00,
        .access = 0x92,
        .granularity = 0xC0,
        .base_high = 0x00
    };

    gdt.entries[3] = (gdt_entry_t){               // User Code32
        .limit = 0xFFFF,
        .base_low = 0x0000,
        .base_mid = 0x00,
        .access = 0xFA,
        .granularity = 0xC0,
        .base_high = 0x00
    };

    gdt.entries[4] = (gdt_entry_t){               // User Data32
        .limit = 0xFFFF,
        .base_low = 0x0000,
        .base_mid = 0x00,
        .access = 0xF2,
        .granularity = 0xC0,
        .base_high = 0x00
    };

    uint32_t tss_base = (uint32_t)&tss;
    uint32_t tss_limit = sizeof(tss_t);

    gdt.tss.limit = tss_limit & 0xFFFF;
    gdt.tss.access = 0x89;
    gdt.tss.granularity = ((tss_limit & 0xF0000) >> 16) | (0x00);
    gdt.tss.base_low = tss_base & 0xFFFF;
    gdt.tss.base_mid = (tss_base >> 16) & 0xFF;
    gdt.tss.base_high = (tss_base >> 24) & 0xFF;

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint32_t)&gdt;

    memset(&tss, 0, sizeof(tss));
    tss.ss0 = 0x10;
    uint32_t stackAddr;
    __asm__ volatile ("mov %%esp, %0" : "=r" (stackAddr));
    tss.esp0 = stackAddr;

    gdt_load();
}