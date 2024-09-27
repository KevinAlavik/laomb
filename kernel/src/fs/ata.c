#include <fs/ata.h>
#include <io.h>
#include <kheap.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <proc/sched.h>
#include <kprintf>

static volatile uint8_t ata_interrupt_occurred_primary = 0;
static volatile uint8_t ata_interrupt_occurred_secondary = 0;

ata_device_t devices[2][2];

static void iowait() {
    inb(0x80);
    inb(0x80);
    inb(0x80);
    inb(0x80);
}

// static uint32_t phys_to_virt(uint32_t paddr) {
//     return paddr + (uint32_t)higher_half_base;
// }

void ata_reset(ata_device_t *device) {
    outb(device->control, ATA_SOFT_RESET);
    iowait();
    outb(device->control, 0);
}

void ata_wait_for_interrupt(ata_device_t *device) {
    if (device->base == devices[0][0].base || device->base == devices[0][1].base) {
        ata_interrupt_occurred_primary = 0;
        while (!ata_interrupt_occurred_primary) {
            yield();
        }
    } else {
        ata_interrupt_occurred_secondary = 0;
        while (!ata_interrupt_occurred_secondary) {
            yield();
        }
    }
}

void ata_wait_BSY(ata_device_t *device) {
    while (inb(device->base + 7) & STATUS_BSY);
}

void ata_wait_DRQ(ata_device_t *device) {
    while (!(inb(device->base + 7) & STATUS_DRQ));
}

static bool ata_identify(ata_device_t *device) {
    outb(device->base + 6, device->dev_select);
    iowait();

    outb(device->base + 7, 0xEC); // Send IDENTIFY command

    if (inb(device->base + 7) == 0) {
        return false;
    }

    uint8_t cl = inb(device->base + 4);
    uint8_t ch = inb(device->base + 5);

    if (cl == 0x14 && ch == 0xEB) {
        device->type = DEVICE_TYPE_ATAPI;
        return true;
    }

    device->type = DEVICE_TYPE_ATA;
    return true;
}

void ata_read_sectors(ata_device_t *device, uint8_t *target, uint32_t LBA, uint8_t sector_count) {
    ata_wait_BSY(device);
    outb(device->base + 6, device->dev_select | ((LBA >> 24) & 0x0F));
    outb(device->base + 2, sector_count);
    outb(device->base + 3, (uint8_t)LBA);
    outb(device->base + 4, (uint8_t)(LBA >> 8));
    outb(device->base + 5, (uint8_t)(LBA >> 16));
    outb(device->base + 7, 0x20); // READ command

    ata_wait_for_interrupt(device);

    uint16_t *target_ptr = (uint16_t *)target;
    for (int i = 0; i < sector_count * 256; i++) {
        target_ptr[i] = inw(device->base);
    }
}

void ata_write_sectors(ata_device_t *device, uint32_t LBA, uint8_t sector_count, uint8_t *source) {
    ata_wait_BSY(device);
    outb(device->base + 6, device->dev_select | ((LBA >> 24) & 0x0F));
    outb(device->base + 2, sector_count);
    outb(device->base + 3, (uint8_t)LBA);
    outb(device->base + 4, (uint8_t)(LBA >> 8));
    outb(device->base + 5, (uint8_t)(LBA >> 16));
    outb(device->base + 7, 0x30); // WRITE command

    ata_wait_for_interrupt(device);

    uint16_t *data_ptr = (uint16_t *)source;
    for (int i = 0; i < sector_count * 256; i++) {
        outw(device->base, data_ptr[i]);
    }
}

void ata_init_device(ata_device_t *device, bool primary, bool master) {
    device->base = primary ? 0x1F0 : 0x170;
    device->control = primary ? 0x3F6 : 0x376;
    device->bmide_base = 0;  // Placeholder for now, DMA not used
    device->dev_select = master ? 0xA0 : 0xB0;
    device->type = DEVICE_TYPE_UNKNOWN;

    if (!ata_identify(device)) {
        kprintf("No ATA/ATAPI device found on %s %s\n",
                primary ? "primary" : "secondary", master ? "master" : "slave");
        return;
    }

    const char *device_type_str = device->type == DEVICE_TYPE_ATAPI ? "ATAPI" :
                                  device->type == DEVICE_TYPE_ATA ? "ATA" :
                                  "Unknown";

    device->dma_buffer = NULL;
    device->dma_buffer_phys = 0;

    uint8_t status = inb(device->base + ATA_REG_STATUS);
    const char *status_str = (status & STATUS_BSY) ? "Busy" :
                             (status & STATUS_RDY) ? "Ready" :
                             (status & STATUS_ERR) ? "Error" :
                             (status & STATUS_DRQ) ? "Data Request" :
                             "Unknown";

    kprintf("Device initialized on %s %s\n", 
            primary ? "primary" : "secondary", master ? "master" : "slave");
    kprintf("  - Device Type: %s\n", device_type_str);
    kprintf("  - I/O Base Address: 0x%x\n", device->base);
    kprintf("  - Control Address: 0x%x\n", device->control);
    kprintf("  - Device Select: 0x%x\n", device->dev_select);
    kprintf("  - Current Status: 0x%x (%s)\n", status, status_str);
}


void ata_primary_interrupt_handler(registers_t *) {
    inb(0x3F6);
    ata_interrupt_occurred_primary = 1;
    pic_sendeoi(14);
}

void ata_secondary_interrupt_handler(registers_t *) {
    inb(0x376);
    ata_interrupt_occurred_secondary = 1;
    pic_sendeoi(15);
}

void ata_init() {
    irq_register_handler(14, ata_primary_interrupt_handler);
    irq_register_handler(15, ata_secondary_interrupt_handler);

    ata_init_device(&devices[0][0], true, true);  // Primary Master
    ata_init_device(&devices[0][1], true, false); // Primary Slave

    ata_init_device(&devices[1][0], false, true);  // Secondary Master
    ata_init_device(&devices[1][1], false, false); // Secondary Slave
}
