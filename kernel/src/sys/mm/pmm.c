#include <sys/mm/pmm.h>
#include <string.h>
#include <kprintf>

#define PAGE_SIZE 4096
#define BITMAP_SIZE(memory_size) ((memory_size) / PAGE_SIZE / 8)

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

static struct ultra_memory_map_attribute memory_map;
static uint8_t* bitmap;
static size_t total_pages;
static size_t bitmap_size;

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
    kprintf("Initializing PMM...\n");
    struct ultra_attribute_header* head = ctx->attributes;
    uint32_t type = head->type;
    
    while (type != ULTRA_ATTRIBUTE_MEMORY_MAP) {
        head = ULTRA_NEXT_ATTRIBUTE(head);
        type = head->type;
    }
    memcpy(&memory_map, head, head->size);
    kprintf("Memory map found, number of entiries: %d\n", ULTRA_MEMORY_MAP_ENTRY_COUNT(memory_map.header));

    total_pages = 0;
    for (size_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(memory_map.header); i++) {
        struct ultra_memory_map_entry* entry = &memory_map.entries[i];
        kprintf("Memory region: start=0x%llx, size=0x%llx, type=0x%08x\n",
                entry->physical_address, entry->size, entry->type);

        if (entry->type == ULTRA_MEMORY_TYPE_FREE || entry->type == ULTRA_MEMORY_TYPE_RECLAIMABLE) {
            total_pages += entry->size / PAGE_SIZE;
        }
    }
    kprintf("Total pages: %d\n", total_pages);

    bitmap_size = BITMAP_SIZE(total_pages * PAGE_SIZE);
    kprintf("Bitmap size: %lu bytes\n", bitmap_size);   
    
    for (size_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(memory_map.header); i++) {
        struct ultra_memory_map_entry* entry = &memory_map.entries[i];
        if (entry->type == ULTRA_MEMORY_TYPE_FREE || entry->type == ULTRA_MEMORY_TYPE_RECLAIMABLE) {

            uintptr_t start_page = ALIGN_UP(entry->physical_address, PAGE_SIZE);
            uintptr_t num_pages = entry->size / PAGE_SIZE;

            kprintf("Checking region for bitmap placement: start=0x%lx, num_pages=%lu\n", start_page, num_pages);
            
            if (num_pages * PAGE_SIZE >= bitmap_size) {
                bitmap = (uint8_t*)start_page;
                kprintf("Bitmap placed at: 0x%p\n", bitmap);    

                memset(bitmap, 0xFF, bitmap_size);
                break;
            }
        }
    }

    set_bit(0);

    for (size_t i = 0; i < ULTRA_MEMORY_MAP_ENTRY_COUNT(memory_map.header); i++) {
        struct ultra_memory_map_entry* entry = &memory_map.entries[i];
        if (entry->type == ULTRA_MEMORY_TYPE_FREE || entry->type == ULTRA_MEMORY_TYPE_RECLAIMABLE) {
            uintptr_t aligned_address = ALIGN_UP(entry->physical_address, PAGE_SIZE);
            uintptr_t end_address = entry->physical_address + entry->size;
            uintptr_t num_pages = (end_address - aligned_address) / PAGE_SIZE;


            kprintf("Freeing pages: aligned_address=0x%lx, num_pages=%lu end_address=0x%lx\n", aligned_address, num_pages, end_address);

            for (size_t j = 0; j < num_pages; j++) {
                uintptr_t page_index = (aligned_address / PAGE_SIZE) + j;
                clear_bit(page_index);
            }
        }
    }

    kprintf("PMM initialization complete.\n");
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

void pmm_free(void* ptr) {
    size_t page = (uintptr_t)ptr / PAGE_SIZE;
    clear_bit(page);
}
