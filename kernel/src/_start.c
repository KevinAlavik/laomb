#include <stdint.h>
#include <stddef.h>
#include <kprintf>
#include <io.h>
#include <string.h>

#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <sys/mm/pmm.h>
#include <sys/mm/vmm.h>
#include <kheap.h>
#include <ultra_protocol.h>
#include <proc/task.h>
#include <proc/sched.h>

#include <rbtree.h>
#include <proc/vfs.h>
#include <proc/ramfs.h>

uintptr_t higher_half_base;

struct ultra_platform_info_attribute* platform_info_attrb = NULL;
struct ultra_kernel_info_attribute* kernel_info_attrb = NULL;
struct ultra_memory_map_attribute* memory_map = NULL;
struct ultra_framebuffer_attribute* framebuffer = NULL;

struct vfs_tree *vfs = NULL;

void main() {
    vfs = vfs_initialize();

    for(;;) hlt();
}

[[noreturn]] void _start(struct ultra_boot_context* ctx, uint32_t)
{
    cli();

    higher_half_base = (uintptr_t)(((struct ultra_platform_info_attribute*)ctx->attributes)->higher_half_base);

    gdt_init();
    idt_init();
    pmm_init(ctx);

    vmm_init_pd(&kernel_page_directory);
    vmm_switch_pd(&kernel_page_directory);

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
    pmm_reclaim_bootloader_memory();

    struct task callback_task = task_create((uintptr_t)main, 0, 0, 10000, kernel_page_directory);
    sched_init(&callback_task);

    for(;;) ;
}