#include <sys/mm/vmm.h>
#include <sys/mm/pmm.h>
#include <io.h>
#include <string.h>
#include <kprintf>
#include <stdbool.h>

vmm_context_t kernel_page_directory;
extern uintptr_t higher_half_base;

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
    pageDirectory->cr3 = ((uintptr_t)pageDirectory->pd - higher_half_base);
    kprintf("Loading PD at paddr: 0x%p, vaddr: 0x%lx\n", pageDirectory->pd, ((uintptr_t)pageDirectory->pd + higher_half_base));
    __asm__ volatile(
        "mov %0, %%cr3\n"
        : :"r"(pageDirectory->cr3)
        : "memory"
    );
    kprintf("Switched to PD at paddr: 0x%p, vaddr: 0x%lx\n", pageDirectory->pd, ((uintptr_t)pageDirectory->pd + higher_half_base));
}

void vmm_init_pd(vmm_context_t* page_directory) {
    page_directory->pd = pmm_alloc();
    page_directory->pd = (PageDirectory*)((uintptr_t)page_directory->pd + higher_half_base);

    memset(page_directory->pd, 0, PAGE_SIZE);

    kprintf(
        "Kernel Adresses: text 0x%p - 0x%p, rodata 0x%p - 0x%p, data 0x%p - 0x%p, bss 0x%p - 0x%p\n",
        __text_start, __text_end, __rodata_start, __rodata_end, __data_start, __data_end, __bss_start, __bss_end
    );

    for (uint32_t addr = 0; addr < 0x40000000; addr += PAGE_SIZE) {
        bool should_map = false;
        for (size_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(pmm_memory_map->header); i++) {
            struct ultra_memory_map_entry* entry = &pmm_memory_map->entries[i];

            if (entry->type == ULTRA_MEMORY_TYPE_FREE || 
                entry->type == ULTRA_MEMORY_TYPE_RECLAIMABLE || 
                entry->type == ULTRA_MEMORY_TYPE_NVS ||
                entry->type == ULTRA_MEMORY_TYPE_LOADER_RECLAIMABLE || 
                entry->type == ULTRA_MEMORY_TYPE_KERNEL_STACK || 
                entry->type == ULTRA_MEMORY_TYPE_MODULE || 
                entry->type == ULTRA_MEMORY_TYPE_KERNEL_BINARY) {
                
                uintptr_t entry_start = entry->physical_address;
                uintptr_t entry_end = entry->physical_address + entry->size;

                if (addr >= entry_start && addr < entry_end) {
                    should_map = true;
                    break;
                }
            }
        }
        if (should_map) {
            if (!vmm_map_page(page_directory, higher_half_base + addr, PAGE_SIZE, addr, PAGE_PRESENT | PAGE_RW)) {
                kprintf("Failed to map 0x%p\n", addr);
                cli(); for(;;) hlt();
            }
        } else {
            if (!vmm_map_page(page_directory, higher_half_base + addr, PAGE_SIZE, addr, 0x0)) {
                kprintf("Failed to map 0x%p not present!\n", addr);
            }
        }
    }
    vmm_map_page(page_directory, (uintptr_t)__text_start, __text_end - __text_start, (uintptr_t)__text_start - higher_half_base, PAGE_PRESENT);
    vmm_map_page(page_directory, (uintptr_t)__rodata_start, __rodata_end - __rodata_start, (uintptr_t)__rodata_start - higher_half_base, PAGE_PRESENT);
    vmm_map_page(page_directory, (uintptr_t)__data_start, __data_end - __data_start, (uintptr_t)__data_start - higher_half_base, PAGE_PRESENT | PAGE_RW);
    vmm_map_page(page_directory, (uintptr_t)__bss_start, __bss_end - __bss_start, (uintptr_t)__bss_start - higher_half_base, PAGE_PRESENT | PAGE_RW);
}

bool vmm_map_page(vmm_context_t* pageDirectory, uint32_t vaddr, size_t size, uint32_t paddr, uint32_t flags) {
    size = ROUND_UP_TO_PAGE(size);

    while (size > 0) {
        uint32_t pageDirIndex = vaddr >> 22;
        uint32_t pageTableIndex = (vaddr >> 12) & 0x03FF;

        page_dir_entry* pageDirEntry = &pageDirectory->pd->entries[pageDirIndex];

        if (!is_page_present((page_table_entry* /* both have present at same offset */)pageDirEntry)) {
            PageTable* newPageTable = pmm_alloc();
            if (!newPageTable) {
                kprintf("Failed to allocate a new page table for vaddr=0x%x\n", vaddr);
                return false;
            }

            set_page_entry((page_table_entry*)pageDirEntry, (uint32_t)newPageTable, PAGE_PRESENT | PAGE_RW);
        }

        PageTable* pageTable = (PageTable*)(pageDirEntry->address << 12);

        page_table_entry* pageEntry = &pageTable->entries[pageTableIndex];

        set_page_entry(pageEntry, paddr, flags);

        vaddr += PAGE_SIZE;
        paddr += PAGE_SIZE;
        size -= PAGE_SIZE;

        __asm__ volatile("invlpg (%0)" : :"r"(vaddr) : "memory");
    }

    return true;
}

static const char pnp_text[] = "Page not present at 0x%x\n";
bool vmm_unmap_page(vmm_context_t* pageDirectory, uint32_t virtualAddress) {
    uint32_t vaddr = ROUND_DOWN_TO_PAGE(virtualAddress);
    uint32_t pageDirIndex = vaddr >> 22;
    uint32_t pageTableIndex = (vaddr >> 12) & 0x03FF;

    page_dir_entry* pageDirEntry = &pageDirectory->pd->entries[pageDirIndex];

    if (!is_page_present((page_table_entry* /* both have present at same offset */)pageDirEntry)) {
        kprintf(pnp_text, vaddr);
        return false;
    }

    PageTable* pageTable = (PageTable*)(pageDirEntry->address << 12);
    page_table_entry* pageEntry = &pageTable->entries[pageTableIndex];

    if (!is_page_present(pageEntry)) { 
        kprintf(pnp_text, vaddr);
        return false;
    }

    pmm_free((void*)(pageEntry->address << 12));
    pageEntry->present = 0;
    __asm__ volatile(
        "invlpg (%0)" : :"r"(virtualAddress) : "memory"
    );

    return true;
}