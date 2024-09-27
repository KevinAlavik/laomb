#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MBR_SIGNATURE 0xAA55

typedef struct mbr_partition {
    uint8_t boot_indicator;  // Boot indicator
    uint8_t start_head;      // Starting head
    uint8_t start_sector;    // Starting sector
    uint8_t start_cylinder;  // Starting cylinder
    uint8_t partition_type;  // Partition type
    uint8_t end_head;        // Ending head
    uint8_t end_sector;      // Ending sector
    uint8_t end_cylinder;    // Ending cylinder
    uint32_t start_sector_lba; // Starting sector (LBA)
    uint32_t num_sectors;    // Number of sectors in partition
} __attribute__((packed)) mbr_partition_t;

typedef struct mbr_header {
    uint8_t boot_code[446];
    mbr_partition_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) mbr_header_t;

typedef struct {
	uint32_t lba_offset;
    uint32_t part_size;
} partition_t;


extern partition_t mbr[4];
extern uint8_t boot_mbr;

void mbr_parse();
bool mbr_read_sector(partition_t* disk, uint32_t lba, uint8_t sectors, uint8_t* buffer);
bool mbr_write_sector(partition_t* disk, uint32_t lba, uint8_t sectors, uint8_t* buffer);
