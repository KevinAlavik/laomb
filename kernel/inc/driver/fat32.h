#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <proc/vfs.h>

#define MAX_DIR_ENTRIES 256

typedef struct 
{
    uint8_t name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t created_time_tenths;
    uint16_t created_time;
    uint16_t created_date;
    uint16_t accessed_date;
    uint16_t first_cluster_high;
    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t first_cluster_low;
    uint32_t size;
} __attribute__((packed)) fat_dir_entry_t;

typedef struct 
{
    uint8_t order;
    int16_t chars1[5];
    uint8_t attribute;
    uint8_t long_entry_type;
    uint8_t checksum;
    int16_t chars2[6];
    uint16_t reserved;
    int16_t chars3[2];
} __attribute__((packed)) fat_lfn_entry_t;

#define FAT_LFN_LAST            0x40

enum FAT_FILE_ATTRIBUTES
{
    FAT_ATTRIBUTE_READ_ONLY         = 0x01,
    FAT_ATTRIBUTE_HIDDEN            = 0x02,
    FAT_ATTRIBUTE_SYSTEM            = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08,
    FAT_ATTRIBUTE_DIRECTORY         = 0x10,
    FAT_ATTRIBUTE_ARCHIVE           = 0x20,
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

#define SECTOR_SIZE             512
#define MAX_PATH_SIZE           256
#define MAX_FILE_HANDLES        10
#define ROOT_DIRECTORY_HANDLE   -1
#define FAT_CACHE_SIZE          5

typedef struct 
{
    uint8_t drive_number;
    uint8_t reserved0;
    uint8_t signature0;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_id[8];
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_directory_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved_extended[12];
} __attribute__((packed)) fat32_extended_boot_record;

typedef struct 
{
    uint8_t boot_jump_instruction[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t dir_entry_count;
    uint16_t total_sectors;
    uint8_t media_descriptor_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;

    fat32_extended_boot_record ebr32;
} __attribute__((packed)) fat_boot_sector;

typedef struct
{
    uint8_t buffer[SECTOR_SIZE];
    struct vfs_fatfs_node* public;
    bool opened;
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t current_sector_in_cluster;
} fat_filedata_t;

typedef struct {
    uint8_t order;
    int16_t chars[13];
} fat_lfn_block_t;

extern fat_boot_sector* fat32_boot_sector;
extern fat_filedata_t root_filedata;
extern uint8_t fat_cache[FAT_CACHE_SIZE * SECTOR_SIZE];
extern uint32_t fat_cache_position;
extern fat_lfn_block_t lfn_block[FAT_LFN_LAST];
extern uint32_t lfn_count;
extern uint32_t data_section_lba;
extern uint32_t total_sectors;
extern uint32_t sectors_per_fat;

struct vfs_fatfs_node
{
    struct vfs_node base;
    uint32_t handle;
};

struct vfs_tree* fat32_init(char* parition);
vfs_err_t fatfs_read(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_err_t fatfs_write(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
vfs_err_t fatfs_create(struct vfs_node *parent, const char *name, enum VFS_TYPE type, struct vfs_node **new_node);
vfs_err_t fatfs_remove(struct vfs_node *parent, struct vfs_node *node);
vfs_err_t fatfs_chmod(struct vfs_node *node, enum VFS_PERMISSIONS);
vfs_err_t fatfs_chown(struct vfs_node *node, const char*);

