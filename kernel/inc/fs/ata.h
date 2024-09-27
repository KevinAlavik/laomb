#pragma once

#define STATUS_BSY 0x80
#define STATUS_RDY 0x40
#define STATUS_DRQ 0x08
#define STATUS_DF 0x20
#define STATUS_ERR 0x01

#define ATA_MASTER_BASE 0x1F0
#define ATA_SLAVE_BASE 0x170

#define ATA_MASTER 0xE0
#define ATA_SLAVE 0xF0

#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_FEATURES 0x01
#define ATA_REG_SECCOUNT0 0x02
#define ATA_REG_LBA0 0x03
#define ATA_REG_LBA1 0x04
#define ATA_REG_LBA2 0x05
#define ATA_REG_HDDEVSEL 0x06
#define ATA_REG_COMMAND 0x07
#define ATA_REG_STATUS 0x07
#define ATA_REG_SECCOUNT1 0x08
#define ATA_REG_LBA3 0x09
#define ATA_REG_LBA4 0x0A
#define ATA_REG_LBA5 0x0B
#define ATA_REG_CONTROL 0x0C
#define ATA_REG_ALTSTATUS 0x0C
#define ATA_REG_DEVADDRESS 0x0D

#define ATA_CHANNEL_PRIMARY 0x1F7
#define ATA_CHANNEL_SECONDARY 0x177
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_SECONDARY_CONTROL 0x376


#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DEVICE_TYPE_UNKNOWN,
    DEVICE_TYPE_ATA,
    DEVICE_TYPE_ATAPI
} device_type_t;

typedef struct {
    uint16_t base;
    uint16_t control;
    uint8_t dev_select;
    device_type_t type;
} ata_device_t;

typedef enum {
    DISK_PRIMARY_MASTER = 0x80,
    DISK_PRIMARY_SLAVE = 0x81,
    DISK_SECONDARY_MASTER = 0x82,
    DISK_SECONDARY_SLAVE = 0x83
} disk_type_t;

extern uint8_t in_use_disk;

void ata_init(uint8_t drive);
void ata_set_in_use_disk(disk_type_t disk);
void ata_read_sectors_pio(uint8_t *target_address, uint32_t LBA, uint8_t sector_count);
void ata_write_sectors_pio(uint32_t LBA, uint8_t sector_count, uint8_t *rawBytes);
