#include <sys/mm/vmm.h>
#include <sys/mm/pmm.h>
#include <string.h>
#include <kprintf>
#include <stdbool.h>

#ifndef bool // vscode is on drugs once again
#define bool	_Bool
#define true	1
#define false	0
#endif

vmm_context_t kernel_page_directory;
extern struct ultra_platform_info_attribute* platform_info_attrb;

static inline uint32_t get_page_index(uint32_t addr) {
    return addr / PAGE_SIZE;
}

static inline bool is_page_present(page_table_entry* entry) {
    return entry->present == 1;
}

static inline void set_page_entry(page_table_entry* entry, uint32_t addr, uint32_t flags) {
    entry->present = (flags & PAGE_PRESENT) ? 1 : 0;
    entry->readwrite = (flags & PAGE_RW) ? 1 : 0;
    entry->user = (flags & PAGE_USER) ? 1 : 0;
    entry->address = addr >> 12;
}

void vmm_switch_pd(vmm_context_t* pageDirectory) {
    kprintf("Switching to Page Directory: 0x%p (before math)\n", pageDirectory->pd);
    pageDirectory->pd = (PageDirectory*)((uintptr_t)pageDirectory->pd - (uintptr_t)platform_info_attrb->higher_half_base);
    kprintf("Switching to Page Directory: 0x%p (after math)\n", pageDirectory->pd);
    __asm__ volatile(
        "mov %0, %%cr3\n"
        : :"r"(pageDirectory)
        : "memory"
    );
}

void vmm_init_pd(vmm_context_t* page_directory) {
    page_directory->pd = pmm_alloc();
    kprintf("Page Directory: 0x%p\n", page_directory->pd);
    page_directory->pd = (PageDirectory*)((uintptr_t)page_directory->pd + (uintptr_t)platform_info_attrb->higher_half_base);
    kprintf("Higher Half Page Directory: 0x%p\n", page_directory->pd);

    memset(page_directory, 0, PAGE_SIZE);

    uint32_t higher_half_base = platform_info_attrb->higher_half_base;
    kprintf(
        "Kernel Adresses: text 0x%p - 0x%p, rodata 0x%p - 0x%p, data 0x%p - 0x%p, bss 0x%p - 0x%p\n",
        __text_start, __text_end, __rodata_start, __rodata_end, __data_start, __data_end, __bss_start, __bss_end
    );

    for (uint32_t addr = 0; addr < 0x40000000; addr += PAGE_SIZE) {
        vmm_map_page(page_directory, higher_half_base + addr, PAGE_SIZE, addr, PAGE_PRESENT | PAGE_RW);
    }
    vmm_map_page(page_directory, higher_half_base + (uintptr_t)__text_start, __text_end - __text_start, (uintptr_t)__text_start - higher_half_base, PAGE_PRESENT);
    vmm_map_page(page_directory, higher_half_base + (uintptr_t)__rodata_start, __rodata_end - __rodata_start, (uintptr_t)__rodata_start - higher_half_base, PAGE_PRESENT);
    vmm_map_page(page_directory, higher_half_base + (uintptr_t)__data_start, __data_end - __data_start, (uintptr_t)__data_start - higher_half_base, PAGE_PRESENT | PAGE_RW);
    vmm_map_page(page_directory, higher_half_base + (uintptr_t)__bss_start, __bss_end - __bss_start, (uintptr_t)__bss_start - higher_half_base, PAGE_PRESENT | PAGE_RW);
}

bool vmm_map_page(vmm_context_t* pageDirectory, uint32_t virtualAddress, size_t size, uint32_t physicalAddress, uint32_t flags) {
    uint32_t vaddr = ROUND_DOWN_TO_PAGE(virtualAddress);
    uint32_t paddr = ROUND_DOWN_TO_PAGE(physicalAddress);
    size = ROUND_UP_TO_PAGE(size);

    while (size > 0) {
        uint32_t pageDirIndex = vaddr >> 22;
        uint32_t pageTableIndex = (vaddr >> 12) & 0x03FF;

        page_dir_entry* pageDirEntry = &pageDirectory->pd->entries[pageDirIndex];


        if (!is_page_present((page_table_entry* /* both have present at same offset */)pageDirEntry)) {
            PageTable* newPageTable = pmm_alloc();
            if (!newPageTable) return false;

            set_page_entry((page_table_entry*)pageDirEntry, (uint32_t)newPageTable, PAGE_PRESENT | PAGE_RW | PAGE_USER);
        }

        PageTable* pageTable = (PageTable*)(pageDirEntry->address << 12);
        set_page_entry(&pageTable->entries[pageTableIndex], paddr, flags);

        vaddr += PAGE_SIZE;
        paddr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }

    __asm__ volatile(
        "invlpg (%0)" : :"r"(virtualAddress) : "memory"
    );
    return true;
}

bool vmm_unmap_page(vmm_context_t* pageDirectory, uint32_t virtualAddress) {
    uint32_t vaddr = ROUND_DOWN_TO_PAGE(virtualAddress);
    uint32_t pageDirIndex = vaddr >> 22;
    uint32_t pageTableIndex = (vaddr >> 12) & 0x03FF;

    page_dir_entry* pageDirEntry = &pageDirectory->pd->entries[pageDirIndex];

    if (!is_page_present((page_table_entry* /* both have present at same offset */)pageDirEntry)) return false;

    PageTable* pageTable = (PageTable*)(pageDirEntry->address << 12);
    page_table_entry* pageEntry = &pageTable->entries[pageTableIndex];

    if (!is_page_present(pageEntry)) return false;

    pmm_free((void*)(pageEntry->address << 12));
    pageEntry->present = 0;
    __asm__ volatile(
        "invlpg (%0)" : :"r"(virtualAddress) : "memory"
    );

    return true;
}