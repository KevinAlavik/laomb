#pragma once

#include <stdint.h>

typedef struct {
    uint16_t limit;
    uint16_t base_low;  
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;  
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    gdt_entry_t entries[5];
    gdt_entry_t tss;
} __attribute__((packed)) gdt_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_pointer_t;

typedef struct {
    uint32_t prev_tss;   // Previous TSS (if hardware task switching is used)
    uint32_t esp0;       // Stack pointer to load when changing to kernel mode
    uint32_t ss0;        // Stack segment to load when changing to kernel mode
    uint32_t esp1;       // Stack pointer to load when changing to ring 1
    uint32_t ss1;        // Stack segment to load when changing to ring 1
    uint32_t esp2;       // Stack pointer to load when changing to ring 2
    uint32_t ss2;        // Stack segment to load when changing to ring 2
    uint32_t cr3;        // Page directory base register (CR3)
    uint32_t eip;        // Instruction pointer
    uint32_t eflags;     // Flags register
    uint32_t eax;        // General purpose registers
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;        // Stack pointer
    uint32_t ebp;        // Base pointer
    uint32_t esi;        // Source index
    uint32_t edi;        // Destination index
    uint32_t es;         // Segment selectors
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;        // Local Descriptor Table selector
    uint16_t trap;       // Trap on task switch
    uint16_t iomap_base; // I/O map base address
} __attribute__((packed)) tss_t;

extern gdt_t gdt;
extern gdt_pointer_t gdt_ptr;
extern tss_t tss;

void gdt_init();
void gdt_load();
