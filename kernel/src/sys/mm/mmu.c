#include <sys/mm/mmu.h>
#include <sys/mm/pmm.h>
#include <string.h>

PageDirectory* kernel_page_directory;
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

void vmm_init_pd(PageDirectory* page_directory) {
    page_directory = pmm_alloc();
    page_directory += platform_info_attrb->higher_half_base;
}