#include <fs/ata.h>
#include <kprintf>
#include <io.h>
#include <kheap.h>
#include <sys/idt.h>
#include <proc/sched.h>
#include <sys/pic.h>

static void ATA_wait_BSY()
{
	while (inb(0x1F7) & STATUS_BSY)
	;
}
static void ATA_wait_DRQ()
{
	while (!(inb(0x1F7) & STATUS_RDY))
	;
}


void ata_read_sectors_pio(uint8_t *target_address, uint32_t LBA, uint8_t sector_count) {
	ATA_wait_BSY();
	outb(ATA_MASTER_BASE + ATA_REG_HDDEVSEL, ATA_MASTER | ((LBA >> 24) & 0xF));
	outb(ATA_MASTER_BASE + ATA_REG_SECCOUNT0, sector_count);
	outb(ATA_MASTER_BASE + ATA_REG_LBA0, (uint8_t)LBA);
	outb(ATA_MASTER_BASE + ATA_REG_LBA1, (uint8_t)(LBA >> 8));
	outb(ATA_MASTER_BASE + ATA_REG_LBA2, (uint8_t)(LBA >> 16));
	outb(ATA_MASTER_BASE + ATA_REG_COMMAND, 0x20);

	uint16_t *target = (uint16_t *)target_address;

	for (int j = 0; j < sector_count; j++) {
		ATA_wait_BSY();
		ATA_wait_DRQ();
		for (int i = 0; i < 256; i++)
			target[i] = inw(0x1F0);
		target += 256;
	}
}

void ata_write_sectors_pio(uint32_t LBA, uint8_t sector_count, uint8_t *rawBytes) {
	ATA_wait_BSY();
	outb(0x1F6, 0xE0 | ((LBA >> 24) & 0xF));
	outb(0x1F2, sector_count);
	outb(0x1F3, (uint8_t)LBA);
	outb(0x1F4, (uint8_t)(LBA >> 8));
	outb(0x1F5, (uint8_t)(LBA >> 16));
	outb(0x1F7, 0x30);

	uint32_t *bytes = (uint32_t *)rawBytes;

	for (int j = 0; j < sector_count; j++) {
		ATA_wait_BSY();
		ATA_wait_DRQ();
		for (int i = 0; i < 256; i++) {
			outl(0x1F0, bytes[i]);
		}
		bytes += 256;
	}
}

void ata_interrupt_handler(registers_t*)
{}

void ata_init()
{
	irq_register_handler(14, ata_interrupt_handler);
}