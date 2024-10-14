#pragma once

#include <stddef.h>
#include <stdint.h>
#include <ultra_protocol.h>

#define PAGE_SIZE 4096
#define BITMAP_SIZE(memory_size) ((memory_size) / PAGE_SIZE / 8)

#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

extern struct ultra_memory_map_attribute* pmm_memory_map;
extern uint8_t* bitmap;
extern size_t total_pages;
extern size_t bitmap_size;

void pmm_init(struct ultra_boot_context* ctx);
void* pmm_alloc_pages(size_t num_pages);
void pmm_free_pages(void* address, size_t num_pages);   
void pmm_reclaim_bootloader_memory();

static inline void* pmm_alloc() {
    return pmm_alloc_pages(1);
}

static inline void pmm_free(void* ptr) {
    pmm_free_pages(ptr, 1);
}