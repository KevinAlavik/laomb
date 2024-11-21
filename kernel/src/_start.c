#include <stdint.h>
#include <stddef.h>
#include <kprintf>
#include <io.h>
#include <string.h>
#include <ultra_protocol.h>
#include <kheap.h>
#include <string.h>

uintptr_t higher_half_base;

struct ultra_platform_info_attribute* platform_info_attrb = NULL;
struct ultra_kernel_info_attribute* kernel_info_attrb = NULL;
struct ultra_memory_map_attribute* memory_map = NULL;
struct ultra_framebuffer_attribute* framebuffer = NULL;

#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <sys/pmm.h>
#include <sys/mmu.h>

#include <proc/sched.h>
#include <video/print.h>
#include <driver/keyboard.h>

[[noreturn]] void job0() {
    DEBUG("Scheduler initialised");

    keyboard_init();
    DEBUG("Keyboard initialised");

    DEBUG("Enabling the Programable Interrupt Controller");
    pic_enable();
    while (1) {
        char c = keyboard_getchar();
        if (c == -1) {
            continue;
        }
        print("%c", c);
    }

    for (;;) ;
}


[[noreturn]] void _start(struct ultra_boot_context* ctx, uint32_t)
{
    cli();
    higher_half_base = (uintptr_t)(((struct ultra_platform_info_attribute*)ctx->attributes)->higher_half_base);

    gdt_init();
    idt_init();
    pmm_init(ctx);

    struct ultra_attribute_header* head = ctx->attributes;
    for (size_t i = 0; i < ctx->attribute_count; i++, head = ULTRA_NEXT_ATTRIBUTE(head)) {
        switch (head->type) {
            case ULTRA_ATTRIBUTE_PLATFORM_INFO:
                platform_info_attrb = kmalloc(head->size);
                memcpy(platform_info_attrb, head, head->size);
                break;
            case ULTRA_ATTRIBUTE_KERNEL_INFO:
                kernel_info_attrb = kmalloc(head->size);
                memcpy(kernel_info_attrb, head, head->size);
                break;
            case ULTRA_ATTRIBUTE_MEMORY_MAP:
                memory_map = kmalloc(head->size);
                memcpy(memory_map, head, head->size);
                break;
            case ULTRA_ATTRIBUTE_FRAMEBUFFER_INFO:
                framebuffer = kmalloc(head->size);
                memcpy(framebuffer, head, head->size);
                break;
            default:
                break;
        }
    }
    pmm_memory_map = memory_map;
    mmu_init_pd(&kernel_page_directory);

    mmu_switch_pd(&kernel_page_directory);
    DEBUG("Switched to new kernel page directory");
    pmm_reclaim_bootloader_memory();
    DEBUG("Reclaimed bootloader memory");

    sched_create_job((uintptr_t)job0, (uint8_t*)__text_start, (uintptr_t)(__text_start - __text_end), (uint8_t*)__data_start, (uintptr_t)(__data_end - __data_start)
                    , 0, 0, 0, nullptr);

    sched_init();

    for(;;) ;
}