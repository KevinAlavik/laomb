#pragma once

#include <stdint.h>
#include <stdbool.h>

// ATA device types
typedef enum {
    DEVICE_TYPE_UNKNOWN,
    DEVICE_TYPE_ATA,
    DEVICE_TYPE_ATAPI
} device_type_t;

typedef enum {
    DISK_PRIMARY_MASTER = 0x80,
    DISK_PRIMARY_SLAVE = 0x81,
    DISK_SECONDARY_MASTER = 0x82,
    DISK_SECONDARY_SLAVE = 0x83
} disk_type_t;

// Status flags
#define STATUS_BSY  0x80
#define STATUS_RDY  0x40
#define STATUS_DRQ  0x08
#define STATUS_DF   0x20
#define STATUS_ERR  0x01
#define ATA_REG_STATUS 0x07

// Control flags
#define ATA_SOFT_RESET 0x4

// DMA commands
#define DMA_CMD_START  0x1
#define DMA_CMD_STOP   0x0
#define DMA_CMD_READ   0x8

// Sizes
#define SECTOR_SIZE 512
#define PRDT_MAX_ENTRIES 16

typedef struct prdt {
    uint32_t buffer_phys;
    uint16_t transfer_size;
    uint16_t mark_end;
} __attribute__((packed)) prdt_t;

typedef struct ata_device {
    uint16_t base;
    uint16_t control;
    uint16_t bmide_base;       // Bus Master IDE base address
    uint8_t dev_select;
    device_type_t type;

    uint32_t prdt_count;
    prdt_t prdt[PRDT_MAX_ENTRIES];
    uint32_t prdt_phys_addr;

    uint8_t *dma_buffer;
    uint32_t dma_buffer_phys;

    bool dma_enabled;
} ata_device_t;

void ata_init_device(ata_device_t *device, bool primary, bool master);
void ata_read_sectors(ata_device_t *device, uint8_t *target, uint32_t LBA, uint8_t sector_count);
void ata_write_sectors(ata_device_t *device, uint32_t LBA, uint8_t sector_count, uint8_t *source);

// void ata_enable_dma(ata_device_t *device);
// void ata_disable_dma(ata_device_t *device);
// void ata_prepare_dma(ata_device_t *device, uint8_t *buffer, uint32_t size);
// void ata_start_dma(ata_device_t *device);
// void ata_stop_dma(ata_device_t *device);
// void ata_dma_interrupt_handler(ata_device_t *device);

void ata_reset(ata_device_t *device);
void ata_wait_for_interrupt(ata_device_t *device);
void ata_wait_BSY(ata_device_t *device);
void ata_wait_DRQ(ata_device_t *device);
void ata_init();

extern ata_device_t devices[2][2]; // 2 primary/secondary channels, 2 master/slave devices each
