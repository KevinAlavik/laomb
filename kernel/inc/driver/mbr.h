#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <ultra_protocol.h>

#define MBR_SIGNATURE 0xAA55

typedef struct {
    uint8_t boot_indicator;
    uint8_t start_head;
    uint8_t start_sector;
    uint8_t start_cylinder;
    uint8_t partition_type;
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_cylinder;
    uint32_t start_sector_lba;
    uint32_t num_sectors;
} __attribute__((packed)) partition_entry_t;

typedef struct {
    uint8_t boot_code[446];
    partition_entry_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) mbr_t;

struct partition {
    char disk[4]; // 80:0 or 81:0 or 80:1 etc
    uint32_t partition_offset;
    uint32_t partition_size;
};

extern struct partition partitions[4]; // for now only suppporting 80:0, TODO
extern uint8_t boot_partition_index;

void mbr_init(struct ultra_kernel_info_attribute* info);

bool mbr_read_sectors(struct partition partition, uint8_t* buffer, uint32_t num_sectors, uint32_t lba);
bool mbr_write_sectors(struct partition partition, uint8_t* buffer, uint32_t num_sectors, uint32_t lba);