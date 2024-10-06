#include <sys/gdt.h>
#include <string.h>

tss_t g_tss = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

gdt_t g_gdt = {
    .entries = {
        {
            .limit = 0,
            .base_low = 0,
            .base_mid = 0,
            .access = 0,
            .granularity = 0,
            .base_high = 0,
        },
        {
            .limit = 0xFFFF,
            .base_low = 0x0000,
            .base_mid = 0x00,
            .access = 0x9A,
            .granularity = 0xC0,
            .base_high = 0x00
        },
        {
            .limit = 0xFFFF,
            .base_low = 0x0000,
            .base_mid = 0x00,
            .access = 0x92,
            .granularity = 0xC0,
            .base_high = 0x00
        },
        {
            .limit = 0xFFFF,
            .base_low = 0x0000,
            .base_mid = 0x00,
            .access = 0xFA,
            .granularity = 0xC0,
            .base_high = 0x00
        },
        {
            .limit = 0xFFFF,
            .base_low = 0x0000,
            .base_mid = 0x00,
            .access = 0xF2,
            .granularity = 0xC0,
            .base_high = 0x00
        },
    },
    .tss = {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .base_high = 0,
        .access = 0x89,
        .granularity = 0x00,
    },
};

gdt_pointer_t g_gdt_ptr = {
    .limit = sizeof(g_gdt) - 1,
    .base = (uint32_t) &g_gdt
};

void tss_init()
{
    uint32_t tss_base = (uint32_t)&g_tss;
    uint32_t tss_limit = sizeof(tss_t);

    g_gdt.tss.limit = tss_limit & 0xFFFF;
    g_gdt.tss.granularity = ((tss_limit & 0xF0000) >> 16) | (0x00);
    g_gdt.tss.base_low = tss_base & 0xFFFF;
    g_gdt.tss.base_mid = (tss_base >> 16) & 0xFF;
    g_gdt.tss.base_high = (tss_base >> 24) & 0xFF;
    memset(&g_tss, 0, sizeof(tss_t));
    g_tss.ss0 = 0x10;
    uint32_t stackAddr;
    __asm__ volatile ("mov %%esp, %0" : "=r" (stackAddr));
    g_tss.esp0 = stackAddr;
}

void gdt_load()
{
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
        : : "r" (&g_gdt_ptr)
        : "ax"
    );
}