#include <driver/fat32.h>
#include <driver/mbr.h>
#include <kprintf>
#include <string.h>
#include <kheap.h>

fat_boot_sector* fat32_boot_sector;
fat_filedata_t root_filedata = {0};
uint8_t fat_cache[FAT_CACHE_SIZE * SECTOR_SIZE] = {0};
uint32_t fat_cache_position = 0xFFFFFFFF;
fat_lfn_block_t lfn_block[FAT_LFN_LAST] = {0};
uint32_t lfn_count = 0;
uint32_t data_section_lba = 0;
uint32_t total_sectors = 0;
uint32_t sectors_per_fat = 0;

static uint32_t cluster_to_lba(uint32_t cluster) {
    return cluster;
}

struct vfs_tree* fat32_init(char* parition)
{
    fat32_boot_sector = (fat_boot_sector*)kmalloc(sizeof(fat_boot_sector));
    uint8_t partition = 0;
    char *token = strtok(parition, ":");
    while (token) {
        partition = atoi(token);
        token = strtok(nullptr, ":");
    }
    if (mbr_read_sectors(partitions[partition], (uint8_t*)fat32_boot_sector, 1, 0) == false) {
        kprintf("Failed to read fat superblock\n");
        return nullptr;
    }
    total_sectors = fat32_boot_sector->total_sectors;
    sectors_per_fat = fat32_boot_sector->sectors_per_fat;
    
    uint32_t root_dir_lba;
    data_section_lba = fat32_boot_sector->reserved_sectors + sectors_per_fat * fat32_boot_sector->fat_count;
    root_dir_lba = cluster_to_lba(fat32_boot_sector->ebr32.root_directory_cluster);
    uint32_t root_dir_size = 0;

    root_filedata.opened = false;
    root_filedata.first_cluster = root_dir_lba;
    root_filedata.current_cluster = root_dir_lba;
    root_filedata.current_sector_in_cluster = 0;
    struct vfs_fatfs_node* root_node = (struct vfs_fatfs_node*)kmalloc(sizeof(struct vfs_fatfs_node));
    memset(root_node, 0, sizeof(struct vfs_fatfs_node));

    root_node->base.type = VFS_FATFS_FOLDER;
    root_node->base.name = strdup("/");
    root_node->base.flags = VFS_WRITE | VFS_READ;
    root_node->base.creation_time = root_node->base.modification_time = get_rtc_timestamp();
    strncpy(root_node->base.owner_name, "root", 70);
    
    root_node->base.read = fatfs_read;
    root_node->base.write = fatfs_write;
    root_node->base.create = fatfs_create;
    root_node->base.remove = fatfs_remove;
    root_node->base.chmod = fatfs_chmod;
    root_node->base.chown = fatfs_chown;

    root_filedata.public = root_node;
    root_node->handle = ROOT_DIRECTORY_HANDLE;

    struct vfs_tree* tree = (struct vfs_tree*)kmalloc(sizeof(struct vfs_tree));
    memset(tree, 0, sizeof(struct vfs_tree));
    tree->root = root_node;

    if (mbr_read_sectors(partitions[partition], (uint8_t*)root_filedata.buffer, 1, root_dir_lba) == false) {
        kprintf("Failed to read root directory\n");
        return nullptr;
    }

    return tree;
}

// TURNS OUT NON-POSIX VFS WAS A MISTAKE AND I CANT FIGURE OUT HOW TO CONTINUE. I WILL COMMIT EVERYTHING AND START REWRITING EVERYTHIGN FROM SCRATCH IN D

vfs_err_t fatfs_read(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_err_t fatfs_write(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
vfs_err_t fatfs_create(struct vfs_node *parent, const char *name, enum VFS_TYPE type, struct vfs_node **new_node);
vfs_err_t fatfs_remove(struct vfs_node *parent, struct vfs_node *node);
vfs_err_t fatfs_chmod(struct vfs_node *node, enum VFS_PERMISSIONS);
vfs_err_t fatfs_chown(struct vfs_node *node, const char*);

