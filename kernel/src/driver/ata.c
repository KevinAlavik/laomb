#include <kprintf>
#include <driver/ata.h>
#include <driver/ata_def.h>
#include <stdbool.h>
#include <kheap.h>
#include <io.h>
#include <proc/sched.h>
#include <sys/pic.h>
#include <sys/idt.h>
#include <proc/mutex.h>
#include <proc/devfs.h>

struct ata_device_list ata_device_list;
uint8_t* ide_buffer = nullptr;

void ide_primary_irq(registers_t*) { pic_sendeoi(14); }
void ide_secondary_irq(registers_t*) { pic_sendeoi(15); }

static void ide_400ns(uint8_t io) {
    for(int i = 0;i < 4; i++) {
		inb(io + ATA_PORT_ALTSTATUS);
    }
}

static bool ide_poll(uint16_t io)
{
	ide_400ns(io);

retry:
	uint8_t status = inb(io + ATA_PORT_STATUS);
	if (status & ATA_STATUS_BUSY) { 
        goto retry;
    }

retry2:
    status = inb(io + ATA_PORT_STATUS);
	
    if (status & ATA_STATUS_ERROR)
	{
		return false;
	}

	if(!(status & ATA_STATUS_DATA_REQUEST)) {
        goto retry2;
    }

	return true;
}

void ide_select_drive(uint8_t bus, uint8_t i)
{
    if (bus == ATA_PRIMARY) {
		if (i == ATA_MASTER) {
			outb(0x1F6, 0xA0);
        } else {
            outb(0x1F6, 0xB0);
        }
    } else {
		if (i == ATA_MASTER) {
			outb(0x176, 0xA0);
        } else {
            outb(0x176, 0xB0);
        }
    }
}

void ata_init()
{
    ide_buffer = (uint8_t*)kmalloc(512);
    idt_register_handler(0x20 + 14, ide_primary_irq);
    idt_register_handler(0x20 + 15, ide_secondary_irq);

    if (ide_identify(ATA_PRIMARY, ATA_MASTER)) {
        ata_device_list.primary_master = true;
        devfs_create_device(VFS_DEVFS_DEVICE_ATA, ATA_PRIMARY << 1 | ATA_MASTER);
    }
    if (ide_identify(ATA_PRIMARY, ATA_SLAVE)) {
        ata_device_list.primary_slave = true;
        devfs_create_device(VFS_DEVFS_DEVICE_ATA, ATA_PRIMARY << 1 | ATA_SLAVE);
    }
    if (ide_identify(ATA_SECONDARY, ATA_MASTER)) {
        ata_device_list.secondary_master = true;
        devfs_create_device(VFS_DEVFS_DEVICE_ATA, ATA_SECONDARY << 1 | ATA_MASTER);
    }
    if (ide_identify(ATA_SECONDARY, ATA_SLAVE)) {
        ata_device_list.secondary_slave = true;
        devfs_create_device(VFS_DEVFS_DEVICE_ATA, ATA_SECONDARY << 1 | ATA_SLAVE);
    }
}

struct Mutex identify_mutex;
uint8_t ide_identify(uint8_t bus, uint8_t drive)
{
    mutex_lock(&identify_mutex);
    uint16_t io = 0;
    ide_select_drive(bus, drive);
    if (bus == ATA_PRIMARY) {
        io = 0x1F0;
    } else {
        io = 0x170;
    }

    outb(io + ATA_PORT_SECTOR_COUNT0, 0);
    outb(io + ATA_PORT_LBA0, 0);
    outb(io + ATA_PORT_LBA1, 0);
    outb(io + ATA_PORT_LBA2, 0);

    outb(io + ATA_PORT_COMMAND, 0xEC);
    uint8_t status = inb(io + ATA_PORT_STATUS);
    if (status) {
        while ((inb(io + ATA_PORT_STATUS) & ATA_STATUS_BUSY) != 0) {
            yield();
        }
read:
        status = inb(io + ATA_PORT_STATUS);
        if (status & ATA_STATUS_ERROR) {
            if (inb(io + ATA_PORT_ERROR) == 0x04) { // device not present
                mutex_unlock(&identify_mutex);
                return 0;
            }
            kprintf("ATA: error %02x\n", inb(io + ATA_PORT_ERROR));
        }
        while (!(status & ATA_STATUS_DATA_REQUEST)) {
            goto read;
        }
        kprintf("%s%s ata device is online\n", bus == ATA_PRIMARY ? "Primary " : "Secondary ", drive == ATA_MASTER ? "master" : "slave");
        for (int i = 0; i < 256; i++)
        {
            *(uint16_t*)(ide_buffer + i*2) = inw(io + ATA_PORT_DATA);
        }
        mutex_unlock(&identify_mutex);
        return status;
    }

    mutex_unlock(&identify_mutex);
    return status;
}

struct Mutex ata_mutex;
uint8_t ata_read_one(uint8_t *buf, uint32_t lba, uint8_t drive)
{
    mutex_lock(&ata_mutex);
    uint16_t io = 0;

    switch(drive)
	{
		case (ATA_PRIMARY << 1 | ATA_MASTER):
			io = 0x1F0;
			drive = ATA_MASTER;
			break;
		case (ATA_PRIMARY << 1 | ATA_SLAVE):
			io = 0x1F0;
			drive = ATA_SLAVE;
			break;
		case (ATA_SECONDARY << 1 | ATA_MASTER):
			io = 0x170;
			drive = ATA_MASTER;
			break;
		case (ATA_SECONDARY << 1 | ATA_SLAVE):
			io = 0x170;
			drive = ATA_SLAVE;
			break;
		default:
			kprintf("ATA: unknown drive!\n");
            mutex_unlock(&ata_mutex);
			return 0;
	}

    uint8_t cmd = (drive == ATA_MASTER ? 0xE0 : 0xF0);

    outb(io + ATA_PORT_HDDEVSEL, (cmd | (uint8_t)((lba >> 24 & 0x0F))));
    outb(io + 0x1, 0x0);
    outb(io + ATA_PORT_SECTOR_COUNT0, 0x1); // we are reading one sector
    outb(io + ATA_PORT_LBA0, (uint8_t)lba & 0xFF);
    outb(io + ATA_PORT_LBA1, (uint8_t)((lba) >> 8));
    outb(io + ATA_PORT_LBA2, (uint8_t)((lba) >> 16));
    outb(io + ATA_PORT_COMMAND, 0x20);

    if (!ide_poll(io)) {
        mutex_unlock(&ata_mutex);
        return 0;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(io + ATA_PORT_DATA);
		*(uint16_t *)(buf + i * 2) = data;
    }
    ide_400ns(io);

    mutex_unlock(&ata_mutex);
    return 1;
}

bool ata_read(uint8_t *buf, uint32_t lba, uint32_t numsects, uint8_t index)
{
    for (uint32_t i = 0; i < numsects; i++)
	{
        if (ata_read_one(buf, lba + i, index) == 0) {
            return false;
        }
		buf += 512;
	}
    return true;
}

uint8_t ata_write_one(uint8_t *buf, uint32_t lba, uint8_t index)
{
    mutex_lock(&ata_mutex);
    uint16_t io = 0;

    switch(index)
    {
        case (ATA_PRIMARY << 1 | ATA_MASTER):
            io = 0x1F0;
            index = ATA_MASTER;
            break;
        case (ATA_PRIMARY << 1 | ATA_SLAVE):
            io = 0x1F0;
            index = ATA_SLAVE;
            break;
        case (ATA_SECONDARY << 1 | ATA_MASTER):
            io = 0x170;
            index = ATA_MASTER;
            break;
        case (ATA_SECONDARY << 1 | ATA_SLAVE):
            io = 0x170;
            index = ATA_SLAVE;
            break;
        default:
            kprintf("ATA: unknown drive!\n");
            mutex_unlock(&ata_mutex);
            return 0;
    }

    uint8_t cmd = (index == ATA_MASTER ? 0xE0 : 0xF0);

    outb(io + ATA_PORT_HDDEVSEL, (cmd | (uint8_t)((lba >> 24 & 0x0F))));
    outb(io + 0x1, 0x0);
    outb(io + ATA_PORT_SECTOR_COUNT0, 0x1); // we are writing one sector
    outb(io + ATA_PORT_LBA0, (uint8_t)lba & 0xFF);
    outb(io + ATA_PORT_LBA1, (uint8_t)((lba) >> 8));
    outb(io + ATA_PORT_LBA2, (uint8_t)((lba) >> 16));
    outb(io + ATA_PORT_COMMAND, 0x30);

    if (!ide_poll(io)) {
        mutex_unlock(&ata_mutex);
        return 0;
    }

    for (int i = 0; i < 256; i++) {
        outw(io + ATA_PORT_DATA, *(uint16_t *)(buf + i * 2));
    }
    ide_400ns(io);

    mutex_unlock(&ata_mutex);
    return 1;
}