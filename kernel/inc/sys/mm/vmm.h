#pragma once

#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4
#define PAGE_SIZE 4096

#define PAGE_MASK (~(PAGE_SIZE - 1))
#define ROUND_DOWN_TO_PAGE(addr) ((addr) & PAGE_MASK)
#define ROUND_UP_TO_PAGE(addr)   (((addr) + PAGE_SIZE - 1) & PAGE_MASK)

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	uint8_t present:1;
	uint8_t readwrite:1;
	uint8_t user:1;
	uint8_t writethru:1;
	uint8_t cachedisable:1;
	uint8_t access:1;
	uint8_t zero:1;
	uint8_t size:1;
	uint8_t ignore:4;
	uint32_t address:20;
} __attribute__((packed)) page_dir_entry;

typedef struct {
	uint8_t present:1;
	uint8_t readwrite:1;
	uint8_t user:1;
	uint8_t writethru:1;
	uint8_t cached:1;
	uint8_t access:1;
	uint8_t dirty:1;
	uint8_t zero:1;
	uint8_t ignore:4;
	uint32_t address:20;
} __attribute__((packed)) page_table_entry;

typedef struct {
    page_table_entry entries[1024];
} __attribute__((packed)) __attribute__((aligned(4096))) PageTable;

typedef struct {
    page_dir_entry entries[1024];
} __attribute__((packed)) __attribute__((aligned(4096))) PageDirectory;

extern char __text_start[], __text_end[], __rodata_start[], __rodata_end[], __data_start[], __data_end[], __bss_start[], __bss_end[]; 

typedef struct {
	PageDirectory* pd;
} __attribute__((packed)) vmm_context_t;

extern vmm_context_t kernel_page_directory;

bool vmm_map_page(vmm_context_t* pageDirectory, uint32_t virtualAddress, size_t size, uint32_t physicalAddress, uint32_t flags);
bool vmm_unmap_page(vmm_context_t* pageDirectory, uint32_t virtualAddress);

void vmm_init_pd(vmm_context_t* pageDirectory);
void vmm_switch_pd(vmm_context_t* pageDirectory);