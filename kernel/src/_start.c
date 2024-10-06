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

[[noreturn]] void _start(struct ultra_boot_context* ctx, uint32_t)
{
    cli();
    higher_half_base = (uintptr_t)(((struct ultra_platform_info_attribute*)ctx->attributes)->higher_half_base);

    tss_init();
    gdt_load();
    idt_init();

    for(;;) ;
}