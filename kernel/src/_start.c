#include <stdint.h>
#include <stddef.h>
#include <kprintf>
#include <io.h>
#include <string.h>

#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <sys/pci.h>
#include <sys/mm/pmm.h>
#include <sys/mm/vmm.h>
#include <kheap.h>
#include <ultra_protocol.h>
#include <proc/task.h>
#include <proc/sched.h>

#include <rbtree.h>
#include <proc/vfs.h>
#include <proc/ramfs.h>

#include <fs/ata.h>
#include <fs/mbr.h>

uint8_t ctrl_pressed = 0;
void key_interrupt_handler(registers_t* regs) {
    uint8_t scancode = inb(0x60);

    if (scancode == 0x1D) {
        ctrl_pressed = 1;  // Left CTRL is pressed
    } else if (scancode == (0x1D | 0x80)) {
        ctrl_pressed = 0;  // Left CTRL is released
    } else if (scancode == 0x1F && ctrl_pressed) {
        kprintf("User triggered register dump!\n");
        
        kprintf("  eax = 0x%lx  ebx = 0x%lx  ecx = 0x%lx  edx = 0x%lx  esi = 0x%lx  edi = 0x%lx\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

        kprintf("  esp = 0x%lx  ebp = 0x%lx  eip = 0x%lx  eflags = 0x%lx\n",
               regs->esp, regs->esp, regs->eip, regs->eflags);

        kprintf("  cs = 0x%x  ds = 0x%x  es = 0x%x  fs = 0x%x  gs = 0x%x\n", 
               regs->cs, regs->ds, regs->es, regs->fs, regs->gs);

        uint32_t cr2;
        uint32_t cr0;
        uint32_t cr3;
        __asm__ volatile ("mov %%cr2, %0" : "=r" (cr2));
        __asm__ volatile ("mov %%cr0, %0" : "=r" (cr0));
        __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
        kprintf("  cr2 = 0x%lx  cr0 = 0x%lx  cr3 = 0x%lx\n", cr2, cr0, cr3);
    }
}

uintptr_t higher_half_base;

struct ultra_platform_info_attribute* platform_info_attrb = NULL;
struct ultra_kernel_info_attribute* kernel_info_attrb = NULL;
struct ultra_memory_map_attribute* memory_map = NULL;
struct ultra_framebuffer_attribute* framebuffer = NULL;

struct vfs_tree *g_Vfs = NULL;

void main() {
    g_Vfs = vfs_initialize();
    ata_init();
    mbr_parse();

    for(;;) hlt();
}

[[noreturn]] void _start(struct ultra_boot_context* ctx, uint32_t)
{
    cli();

    higher_half_base = (uintptr_t)(((struct ultra_platform_info_attribute*)ctx->attributes)->higher_half_base);

    gdt_init();
    idt_init();
    pmm_init(ctx);
    pci_init();

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

    vmm_init_pd(&kernel_page_directory);

    size_t framebuffer_size = framebuffer->fb.width * framebuffer->fb.height * 4;
    vmm_unmap_page(&kernel_page_directory, 0xfc000000);
    vmm_map_page(&kernel_page_directory, 0xfc000000, framebuffer_size, framebuffer->fb.physical_address, PAGE_PRESENT | PAGE_RW);
    vmm_switch_pd(&kernel_page_directory);
    memset((uint8_t*)0xfc000000, 0x12345432, framebuffer_size);
    
    pmm_reclaim_bootloader_memory();
    
    irq_register_handler(1, key_interrupt_handler);

    struct task callback_task = task_create((uintptr_t)main, 0, 0, 10000, kernel_page_directory);
    sched_init(&callback_task);

    for(;;) ;
}