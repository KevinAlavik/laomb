#include <fs/ata.h>
#include <fs/mbr.h>
#include <stdbool.h>
#include <kheap.h>
#include <kprintf>

partition_t mbr[4];
uint8_t boot_mbr;

void mbr_parse()
{
    boot_mbr = 0;
    mbr_header_t mbr_;
    ata_read_sectors_pio((uint8_t*)&mbr_, 0, 1);
    if (mbr_.signature != MBR_SIGNATURE) {
        kprintf("MBR signature not found: 0x%x\n", mbr_.signature);
    }
    for (int i = 0; i < 4; i++) {
        mbr[i].lba_offset = mbr_.partitions[i].start_sector_lba;
        mbr[i].part_size = mbr_.partitions[i].num_sectors;
        if (mbr_.partitions[i].boot_indicator == 0x80) {
            boot_mbr = i;
        }
    }
    kprintf("Booted of partition %d, lba offset %d\n", boot_mbr, mbr[boot_mbr].lba_offset);
}

bool mbr_read_sector(partition_t* disk, uint32_t lba, uint8_t sectors, uint8_t* lowerDataOut)
{
	if (lba+disk->lba_offset >=disk->part_size) {
		return false;
	}
	ata_read_sectors_pio(lowerDataOut, (lba+disk->lba_offset), sectors);
	return true;
}

bool mbr_write_sector(partition_t* disk, uint32_t lba, uint8_t sectors, uint8_t* lowerDataIn)
{
	if (lba+disk->lba_offset >=disk->part_size) {
		return false;
	}
	ata_write_sectors_pio(lba+disk->lba_offset, sectors, lowerDataIn);
	return true;
}