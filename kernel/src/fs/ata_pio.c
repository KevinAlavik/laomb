#include <fs/ata.h>
#include <kprintf>
#include <io.h>
#include <kheap.h>
#include <sys/idt.h>
#include <proc/sched.h>
#include <sys/pic.h>

static uint8_t atapi_read_command[12] = {
    0xA8,          // READ (12) command code
    0, 0, 0, 0,    // Logical block address (LBA) placeholder
    0, 0,          // Transfer length in blocks (to be filled in later)
    0, 0, 0, 0,    // Reserved
    0              // Control
};

static ata_device_t devices[2][2]; 
static ata_device_t *current_device;
volatile uint8_t ata_interrupt_occurred_primary = 0;
volatile uint8_t ata_interrupt_occurred_secondary = 0;

void ata_set_in_use_disk(disk_type_t disk)
{
    bool primary = (disk == DISK_PRIMARY_MASTER) || (disk == DISK_PRIMARY_SLAVE);
    bool master = (disk == DISK_PRIMARY_MASTER) || (disk == DISK_SECONDARY_MASTER);
    current_device = &devices[primary][master];
    current_device->base = primary ? ATA_MASTER_BASE : ATA_SLAVE_BASE;
    current_device->control = primary ? ATA_PRIMARY_CONTROL : ATA_SECONDARY_CONTROL;
    current_device->dev_select = master ? 0xA0 : 0xB0;
}

static void ATA_wait_for_interrupt()
{
    if (current_device->base == ATA_MASTER_BASE) {
        ata_interrupt_occurred_primary = 0;
        while (!ata_interrupt_occurred_primary)
            yield();
    } else {
        ata_interrupt_occurred_secondary = 0;
        while (!ata_interrupt_occurred_secondary)
            yield();
    }
}

static void ATA_wait_BSY()
{
    while (inb(current_device->base + ATA_REG_STATUS) & STATUS_BSY)
        ;
}

static void ATA_wait_DRQ()
{
    while (!(inb(current_device->base + ATA_REG_STATUS) & STATUS_DRQ))
        ;
}

static bool ata_identify(bool primary, bool master)
{
    uint16_t base = primary ? ATA_MASTER_BASE : ATA_SLAVE_BASE;
    uint8_t dev_select = master ? 0xA0 : 0xB0;

    outb(base + ATA_REG_HDDEVSEL, dev_select);
    yield();

    outb(base + ATA_REG_COMMAND, 0xEC);

    if (inb(base + ATA_REG_STATUS) == 0)
        return false;

    uint8_t cl = inb(base + ATA_REG_LBA1);
    uint8_t ch = inb(base + ATA_REG_LBA2);

    if (cl == 0x14 && ch == 0xEB) {
        kprintf("ATAPI found on: \'%s\'\\\'%s\'\n", primary ? "primary" : "secondary", master ? "master" : "slave");
        devices[primary][master].type = DEVICE_TYPE_ATAPI;
        return true;
    }

    kprintf("ATA found on: \'%s\'\\\'%s\'\n", primary ? "primary" : "secondary", master ? "master" : "slave");
    devices[primary][master].type = DEVICE_TYPE_ATA;
    return true;
}

void atapi_read_sectors_pio(uint8_t *target_address, uint32_t LBA, uint8_t sector_count)
{
    outb(current_device->base + ATA_REG_HDDEVSEL, current_device->dev_select);
    yield();

    ATA_wait_BSY();
    outb(current_device->base + ATA_REG_COMMAND, 0xA0);
    ATA_wait_DRQ();

    atapi_read_command[2] = (LBA >> 24) & 0xFF;
    atapi_read_command[3] = (LBA >> 16) & 0xFF;
    atapi_read_command[4] = (LBA >> 8) & 0xFF;
    atapi_read_command[5] = LBA & 0xFF;
    atapi_read_command[7] = 0;
    atapi_read_command[8] = sector_count;

    for (int i = 0; i < 6; i++)
        outw(current_device->base + ATA_REG_DATA, ((uint16_t *)atapi_read_command)[i]);

    ATA_wait_for_interrupt();
    for (int j = 0; j < sector_count; j++) {
        ATA_wait_DRQ();
        for (int i = 0; i < 2048 / 2; i++)
            ((uint16_t *)target_address)[i] = inw(current_device->base + ATA_REG_DATA);
        target_address += 2048;
    }

    pic_sendeoi(current_device->base == ATA_MASTER_BASE ? 14 : 15);
}

void ata_read_sectors_pio(uint8_t *target_address, uint32_t LBA, uint8_t sector_count)
{
    ATA_wait_BSY();
    outb(current_device->base + ATA_REG_HDDEVSEL, current_device->dev_select | ((LBA >> 24) & 0x0F));
    outb(current_device->base + ATA_REG_SECCOUNT0, sector_count);
    outb(current_device->base + ATA_REG_LBA0, (uint8_t)LBA);
    outb(current_device->base + ATA_REG_LBA1, (uint8_t)(LBA >> 8));
    outb(current_device->base + ATA_REG_LBA2, (uint8_t)(LBA >> 16));
    outb(current_device->base + ATA_REG_COMMAND, 0x20);

    uint16_t *target = (uint16_t *)target_address;

    for (int j = 0; j < sector_count; j++) {
        ATA_wait_for_interrupt();
        ATA_wait_DRQ();
        for (int i = 0; i < 256; i++)
            target[i] = inw(current_device->base + ATA_REG_DATA);
        target += 256;
    }
}

void ata_write_sectors_pio(uint32_t LBA, uint8_t sector_count, uint8_t *rawBytes)
{
    ATA_wait_BSY();
    outb(current_device->base + ATA_REG_HDDEVSEL, current_device->dev_select | ((LBA >> 24) & 0x0F));
    outb(current_device->base + ATA_REG_SECCOUNT0, sector_count);
    outb(current_device->base + ATA_REG_LBA0, (uint8_t)LBA);
    outb(current_device->base + ATA_REG_LBA1, (uint8_t)(LBA >> 8));
    outb(current_device->base + ATA_REG_LBA2, (uint8_t)(LBA >> 16));
    outb(current_device->base + ATA_REG_COMMAND, 0x30);

    uint16_t *data = (uint16_t *)rawBytes;

    for (int j = 0; j < sector_count; j++) {
        ATA_wait_for_interrupt();
        ATA_wait_DRQ();
        for (int i = 0; i < 256; i++)
            outw(current_device->base + ATA_REG_DATA, data[i]);
        data += 256;
    }
}

void ata_primary_interrupt_handler(registers_t*)
{
    inb(ATA_PRIMARY_CONTROL);
    ata_interrupt_occurred_primary = 1;
    pic_sendeoi(14);
}

void ata_secondary_interrupt_handler(registers_t*)
{
    inb(ATA_SECONDARY_CONTROL);
    ata_interrupt_occurred_secondary = 1;
    pic_sendeoi(15);
}

void ata_init(uint8_t disk)
{
    ata_set_in_use_disk((disk_type_t)disk);

    irq_register_handler(14, ata_primary_interrupt_handler);
    irq_register_handler(15, ata_secondary_interrupt_handler);

    bool primary = (disk == DISK_PRIMARY_MASTER) || (disk == DISK_PRIMARY_SLAVE);
    bool master = (disk == DISK_PRIMARY_MASTER) || (disk == DISK_SECONDARY_MASTER);

    if (!ata_identify(primary, master))
        kprintf("No device found on %s %s\n", primary ? "primary" : "secondary", master ? "master" : "slave");
}