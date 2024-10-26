#include <io.h>
#include <stdint.h>

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void cli() {
    __asm__ volatile ("cli");
}

void sti() {
    __asm__ volatile ("sti");
}

void hlt() {
    __asm__ volatile ("hlt");
}

int is_interrupts_enabled() {
    uint32_t eflags;
    __asm__ volatile (
        "pushf\n"
        "pop %0"
        : "=r"(eflags)
        :
        : "memory"
    );
    return (eflags & 0x200) != 0;
}

uint64_t read_msr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

void write_msr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile ("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

void io_wait() {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

void invalidate_cache() {
    __asm__ volatile ("wbinvd");
}

void flush_cache(const void *addr) {
    __asm__ volatile ("clflush (%0)" : : "r"(addr));
}

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static uint8_t read_rtc_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static int is_updating() {
    outb(CMOS_ADDRESS, 0x0A);
    return inb(CMOS_DATA) & 0x80;
}

static uint8_t bcd_to_binary(uint8_t value) {
    return (value & 0x0F) + ((value / 16) * 10);
}

uint8_t get_seconds() {
    while (is_updating());
    return bcd_to_binary(read_rtc_register(0x00));
}

uint8_t get_minutes() {
    while (is_updating());
    return bcd_to_binary(read_rtc_register(0x02));
}

uint8_t get_hours() {
    while (is_updating());
    return bcd_to_binary(read_rtc_register(0x04));
}

uint8_t get_day_of_month() {
    while (is_updating());
    return bcd_to_binary(read_rtc_register(0x07));
}

uint8_t get_month() {
    while (is_updating());
    return bcd_to_binary(read_rtc_register(0x08));
}

uint8_t get_year() {
    while (is_updating());
    return bcd_to_binary(read_rtc_register(0x09));
}

uint64_t get_rtc_timestamp() {
    uint64_t timestamp = 0;
    timestamp |= (uint64_t)(get_year() + 2000) << 40;
    timestamp |= (uint64_t)get_month() << 32;
    timestamp |= (uint64_t)get_day_of_month() << 24;
    timestamp |= (uint64_t)get_hours() << 16;
    timestamp |= (uint64_t)get_minutes() << 8;
    timestamp |= (uint64_t)get_seconds();
    return timestamp;
}

void insb(uint16_t port, void *buffer, size_t count) {
    __asm__ volatile (
        "rep insb" 
        : "=D" (buffer), "=c" (count)
        : "d" (port), "0" (buffer), "1" (count)
        : "memory"
    );
}

void insw(uint16_t port, void *buffer, size_t count) {
    __asm__ volatile (
        "rep insw" 
        : "=D" (buffer), "=c" (count)
        : "d" (port), "0" (buffer), "1" (count)
        : "memory"
    );
}

void insl(uint16_t port, void *buffer, size_t count) {
    __asm__ volatile (
        "rep insl" 
        : "=D" (buffer), "=c" (count)
        : "d" (port), "0" (buffer), "1" (count)
        : "memory"
    );
}

void outsb(uint16_t port, const void *buffer, size_t count) {
    __asm__ volatile (
        "rep outsb" 
        : "=S" (buffer), "=c" (count)
        : "d" (port), "0" (buffer), "1" (count)
        : "memory"
    );
}

void outsw(uint16_t port, const void *buffer, size_t count) {
    __asm__ volatile (
        "rep outsw" 
        : "=S" (buffer), "=c" (count)
        : "d" (port), "0" (buffer), "1" (count)
        : "memory"
    );
}

void outsl(uint16_t port, const void *buffer, size_t count) {
    __asm__ volatile (
        "rep outsl" 
        : "=S" (buffer), "=c" (count)
        : "d" (port), "0" (buffer), "1" (count)
        : "memory"
    );
}
