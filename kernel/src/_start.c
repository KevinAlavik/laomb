#include <stdint.h>
#include <stddef.h>
#include <kprintf>
#include <io.h>
#include <string.h>
#include <ultra_protocol.h>
#include <kheap.h>

uintptr_t higher_half_base;

struct ultra_platform_info_attribute* platform_info_attrb = NULL;
struct ultra_kernel_info_attribute* kernel_info_attrb = NULL;
struct ultra_memory_map_attribute* memory_map = NULL;
struct ultra_framebuffer_attribute* framebuffer = NULL;
struct ultra_module_info_attribute* initrd_module = NULL;

#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <sys/pmm.h>
#include <sys/mmu.h>

#include <proc/sched.h>
#include <proc/vfs.h>
#include <time.h>

[[noreturn]] void main() {
    g_Vfs = vfs_initialize();

    for (;;) { }
}

[[noreturn]] void _start(struct ultra_boot_context* ctx, uint32_t)
{
    cli();
    higher_half_base = (uintptr_t)(((struct ultra_platform_info_attribute*)ctx->attributes)->higher_half_base);

    tss_init();
    gdt_load(); 
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
            case ULTRA_ATTRIBUTE_MODULE_INFO:
                initrd_module = kmalloc(head->size);
                memcpy(initrd_module, head, head->size);
                break;
            default:
                break;
        }
    }
    pmm_memory_map = memory_map;
    mmu_init_pd(&kernel_page_directory);

    mmu_switch_pd(&kernel_page_directory);
    pmm_reclaim_bootloader_memory();

    struct JCB* callback = scheduler_create_job((uintptr_t)main, 1000000);
    scheduler_init(callback);
    __builtin_unreachable();
}