#include <stdint.h>
#include <stddef.h>
#include <kprintf>
#include <io.h>
#include <string.h>
#include <ultra_protocol.h>

uintptr_t higher_half_base;

struct ultra_platform_info_attribute* platform_info_attrb = NULL;
struct ultra_kernel_info_attribute* kernel_info_attrb = NULL;
struct ultra_memory_map_attribute* memory_map = NULL;
struct ultra_framebuffer_attribute* framebuffer = NULL;

#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <sys/pmm.h>

[[noreturn]] void _start(struct ultra_boot_context* ctx, uint32_t)
{
    cli();
    higher_half_base = (uintptr_t)(((struct ultra_platform_info_attribute*)ctx->attributes)->higher_half_base);

    tss_init();
    gdt_load();
    idt_init();
    pmm_init(ctx);

    // 200 allocations for testing
    uintptr_t* ptrarr = (uintptr_t*)pmm_alloc_pages((sizeof(void*) * 200) / PAGE_SIZE);
    for (int i = 0; i < 200; i++) {
        void* ptr = pmm_alloc_pages(2);
        kprintf("ptr: 0x%p\n", ptr);
        ptrarr[i] = (uintptr_t)ptr;
    }
    for (int i = 0; i < 200; i++) {
        pmm_free_pages((void*)ptrarr[i], 2);
    }

    for(;;) ;
}