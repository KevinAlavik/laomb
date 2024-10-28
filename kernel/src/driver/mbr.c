#include <driver/mbr.h>
#include <driver/ata.h>
#include <proc/vfs.h>
#include <proc/devfs.h>
#include <kprintf>
#include <string.h>
#include <kheap.h>

struct partition partitions[4] = {0};
uint8_t boot_partition_index = 0;

void mbr_init(struct ultra_kernel_info_attribute* info)
{
    uint8_t buffer[512];
    mbr_t* mbr = (mbr_t*)&buffer;
    
    struct vfs_node* n = vfs_traverse_path(g_Vfs, "/dev/80:0");
    if (n == nullptr) {
        kprintf("Disk /dev/80:0 not found\n");
        return;
    }

    if (vfs_read(n, 0, 512, buffer) != VFS_SUCCESS) {
        kprintf("Failed to read MBR from /dev/80:0\n");
        return;
    }

    if (mbr->signature != MBR_SIGNATURE) {
        kprintf("Invalid MBR signature: 0x%X\n", mbr->signature);
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (mbr->partitions[i].partition_type == 0) {
            kprintf("Partition %d on disk 80:0 is empty\n", i);
            continue;
        }

        partitions[i].partition_offset = mbr->partitions[i].start_sector_lba;
        partitions[i].partition_size = mbr->partitions[i].num_sectors;
        strncpy(partitions[i].disk, "80:0", 4);

        uint8_t drive = (ATA_PRIMARY << 3) | (ATA_MASTER << 2) | (i & 0x03);
        devfs_create_device(VFS_DEVFS_DEVICE_PARTITION, drive);

        kprintf("Partition %d: offset=%d size=%d sectors on disk %s\n",
                i, partitions[i].partition_offset, partitions[i].partition_size, partitions[i].disk);
    }

    if (info->partition_index < 4) {
        boot_partition_index = info->partition_index;
    } else {
        kprintf("Invalid boot partition index: %d\n", info->partition_index);
        boot_partition_index = 0;
    }
}

bool mbr_read_sectors(struct partition partition, uint8_t* buffer, uint32_t num_sectors, uint32_t lba)
{
    return ata_read(buffer, lba+partition.partition_offset, num_sectors, 0);
}
bool mbr_write_sectors(struct partition partition, uint8_t* buffer, uint32_t num_sectors, uint32_t lba)
{
    return ata_write(buffer, lba+partition.partition_offset, num_sectors, 0);
}