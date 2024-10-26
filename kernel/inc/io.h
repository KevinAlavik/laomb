#pragma once
#include <stdint.h>
#include <stddef.h>

void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void insb(uint16_t port, void *buffer, size_t count);
void insw(uint16_t port, void *buffer, size_t count);
void insl(uint16_t port, void *buffer, size_t count);

void outsb(uint16_t port, const void *buffer, size_t count);
void outsw(uint16_t port, const void *buffer, size_t count);
void outsl(uint16_t port, const void *buffer, size_t count);

void cli();
void sti();
void hlt();

int is_interrupts_enabled();

uint64_t read_msr(uint32_t msr);
void write_msr(uint32_t msr, uint64_t value);

void io_wait();
void memory_barrier();
void io_memory_barrier();

void invalidate_cache();
void flush_cache(const void *addr);

uint8_t get_seconds();
uint8_t get_minutes();
uint8_t get_hours();
uint8_t get_day_of_month();
uint8_t get_month();
uint8_t get_year();
uint64_t get_rtc_timestamp();