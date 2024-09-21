#include <sys/mm/pmm.h>
#include <string.h>
#include <kprintf>

#define PAGE_SIZE 4096
#define BITMAP_SIZE(memory_size) ((memory_size) / PAGE_SIZE / 8)

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

extern struct ultra_platform_info_attribute* platform_info_attrb;
struct ultra_memory_map_attribute* memory_map;
uint8_t* bitmap;
size_t total_pages;
size_t bitmap_size;

static inline void set_bit(size_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void clear_bit(size_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline int test_bit(size_t bit) {
    return bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(struct ultra_boot_context* ctx) {
    struct ultra_attribute_header* head = ctx->attributes;
    uint32_t type = head->type;
    
    while (type != ULTRA_ATTRIBUTE_MEMORY_MAP) {
        head = ULTRA_NEXT_ATTRIBUTE(head);
        type = head->type;
    }
    memory_map = (struct ultra_memory_map_attribute*)head;

    total_pages = 0;
    for (size_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(memory_map->header); i++) {
        struct ultra_memory_map_entry* entry = &memory_map->entries[i];
        kprintf("Memory region: start=0x%llx, size=0x%llx, type=0x%08x\n",
                entry->physical_address, entry->size, entry->type);

        if (entry->type == ULTRA_MEMORY_TYPE_FREE || entry->type == ULTRA_MEMORY_TYPE_RECLAIMABLE) {
            total_pages += entry->size / PAGE_SIZE;
        }
    }

    bitmap_size = BITMAP_SIZE(total_pages * PAGE_SIZE);
    kprintf("Bitmap size: %lu bytes\n", bitmap_size);   
    
    uintptr_t bitmap_start_page = 0;
    uintptr_t bitmap_end_page = 0;
    for (size_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(memory_map->header); i++) {
        struct ultra_memory_map_entry* entry = &memory_map->entries[i];
        if ((entry->type == ULTRA_MEMORY_TYPE_FREE || entry->type == ULTRA_MEMORY_TYPE_RECLAIMABLE)) {

            uintptr_t start_page = ALIGN_UP(entry->physical_address, PAGE_SIZE);
            uintptr_t num_pages = entry->size / PAGE_SIZE;

            kprintf("Checking region for bitmap placement: start=0x%lx, num_pages=%lu\n", start_page, num_pages);
            
            if (num_pages * PAGE_SIZE >= bitmap_size) {                    
                bitmap = (uint8_t*)start_page;

                memset(bitmap, 0xFF, bitmap_size);
                
                bitmap_start_page = ALIGN_DOWN(start_page, PAGE_SIZE) / PAGE_SIZE;
                bitmap_end_page = ALIGN_UP(start_page + bitmap_size, PAGE_SIZE) / PAGE_SIZE;
                bitmap += platform_info_attrb->higher_half_base; // map higher half

                kprintf("Bitmap placed at: 0x%p->0x%p\n", bitmap, bitmap + bitmap_size);
                for (uintptr_t page = bitmap_start_page; page < bitmap_end_page; page++) {
                    set_bit(page);
                }

                break;
            }
        }
    }

    set_bit(0);

    for (size_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(memory_map->header); i++) {
        struct ultra_memory_map_entry* entry = &memory_map->entries[i];
        if (entry->type == ULTRA_MEMORY_TYPE_FREE || entry->type == ULTRA_MEMORY_TYPE_RECLAIMABLE) {
            uintptr_t aligned_address = ALIGN_UP(entry->physical_address, PAGE_SIZE);
            uintptr_t end_address = entry->physical_address + entry->size;
            uintptr_t num_pages = (end_address - aligned_address) / PAGE_SIZE;

            for (size_t j = 0; j < num_pages; j++) {
                uintptr_t page_index = (aligned_address / PAGE_SIZE) + j;
                
                if (page_index >= bitmap_start_page && page_index < bitmap_end_page) {
                    continue;
                }

                clear_bit(page_index);
            }
        }
    }
}


void* pmm_alloc() {
    for (size_t i = 0; i < total_pages; i++) {  
        if (!test_bit(i)) {
            set_bit(i);
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void* pmm_alloc_pages(size_t num_pages) {
    size_t start_page = 0;
    size_t contiguous_count = 0;

    for (size_t i = 0; i < total_pages; i++) {
        if (!test_bit(i)) {
            if (contiguous_count == 0) {
                start_page = i;
            }
            contiguous_count++;

            if (contiguous_count == num_pages) {
                for (size_t j = 0; j < num_pages; j++) {
                    set_bit(start_page + j);
                }
                return (void*)(start_page * PAGE_SIZE);
            }
        } else {
            contiguous_count = 0;
        }
    }
    return NULL;  
}

void pmm_free_pages(void* address, size_t num_pages) {
    size_t start_page = (size_t)address / PAGE_SIZE;

    for (size_t i = 0; i < num_pages; i++) {
        clear_bit(start_page + i);
    }
}

void pmm_free(void* ptr) {
    size_t page = (uintptr_t)ptr / PAGE_SIZE;
    clear_bit(page);
}

void pmm_reclaim_bootloader_memory() {
    for (size_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(memory_map->header); i++) {
        struct ultra_memory_map_entry* entry = &memory_map->entries[i];

        if (entry->type == ULTRA_MEMORY_TYPE_LOADER_RECLAIMABLE) {
            uintptr_t aligned_address = ALIGN_UP(entry->physical_address, PAGE_SIZE);
            uintptr_t end_address = entry->physical_address + entry->size;
            uintptr_t num_pages = (end_address - aligned_address) / PAGE_SIZE;

            kprintf("Reclaiming bootloader memory: aligned_address=0x%lx, num_pages=%lu\n", aligned_address, num_pages);

            for (size_t j = 0; j < num_pages; j++) {
                uintptr_t page_index = (aligned_address / PAGE_SIZE) + j;
                clear_bit(page_index);
            }
        }
    }

    kprintf("Bootloader reclaimable memory has been reclaimed.\n");
}

// this will be called by the kernel heap manager AFTER it saves what pages are usable (aka some are used by bitmap, some by bootloader)
void reserve_low_memory_for_heap() {
    uintptr_t low_memory_end = 0x9FC00;  // Limit for usable low memory

    for (uintptr_t addr = 0x0; addr < low_memory_end; addr += PAGE_SIZE) {
        size_t page_index = addr / PAGE_SIZE;
        
        // Mark this page as used in the PMM so it won't be allocated elsewhere
        set_bit(page_index);
    }

    kprintf("Low memory (0x0 to 0x9FC00) reserved for kernel heap.\n");
}
