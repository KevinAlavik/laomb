#include <stdint.h>
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

struct ultra_boot_context* boot_context;
struct ultra_platform_info_attribute* platform_info_attrb;
uintptr_t higher_half_base;

[[noreturn]] void _start(struct ultra_boot_context* ctx, uint32_t magic)
{
    cli();
    memcpy(&boot_context, ctx, sizeof(boot_context));

    uint32_t type = ctx->attributes->type;
    if (type == ULTRA_ATTRIBUTE_PLATFORM_INFO) {
        platform_info_attrb = (struct ultra_platform_info_attribute*)ctx->attributes;
    }
    kprintf("Bootloader: %s\n", platform_info_attrb->loader_name);
    kprintf("Hyper Bootloader Magic: 0x%08x\n", magic);
    
    kprintf("Kernel Base: 0x%lx\n", (uintptr_t)platform_info_attrb->higher_half_base);
    higher_half_base = (uintptr_t)platform_info_attrb->higher_half_base;
    kprintf("ACPI RSDP: 0x%llx\n", platform_info_attrb->acpi_rsdp_address);
    kprintf("DTB: 0x%llx\n", platform_info_attrb->dtb_address);
    kprintf("SMBios: 0x%llx\n", platform_info_attrb->smbios_address);
    kprintf("Page Table Depth: %d\n", platform_info_attrb->page_table_depth);

    gdt_init();
    idt_init();
    pmm_init(ctx);
    vmm_init_pd(&kernel_page_directory);
    vmm_switch_pd(&kernel_page_directory);

    sti();
    for(;;) ;
}