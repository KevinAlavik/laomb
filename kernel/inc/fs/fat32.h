#pragma once

#include <stdint.h>
#include <fs/mbr.h>

#define FAT_LFN_LAST            0x40
#define SECTOR_SIZE             512
#define MAX_PATH_SIZE           256
#define MAX_FILE_HANDLES        10
#define ROOT_DIRECTORY_HANDLE   -1
#define FAT_CACHE_SIZE          5

typedef struct 
{
    uint8_t name[11];
    uint8_t attributes;
    uint8_t _reserved;
    uint8_t created_time_tenths;
    uint16_t created_time;
    uint16_t created_date;
    uint16_t accessed_date;
    uint16_t first_cluster_high;
    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t first_cluster_low;
    uint32_t size;
} __attribute__((packed)) fat_directory_t;

typedef struct 
{
    uint8_t order;
    int16_t chars1[5];
    uint8_t attributes;
    uint8_t long_entry_type;
    uint8_t checksum;
    int16_t chars2[6];
    uint16_t _reserved;
    int16_t chars3[2];
} __attribute__((packed)) fat_lfn_entry_t;

typedef struct 
{
    int handle;
    bool is_directory;
    uint32_t position;
    uint32_t size;
} fat_file_t;

enum FAT_Attributes
{
    FAT_ATTRIBUTE_READ_ONLY         = 0x01,
    FAT_ATTRIBUTE_HIDDEN            = 0x02,
    FAT_ATTRIBUTE_SYSTEM            = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08,
    FAT_ATTRIBUTE_DIRECTORY         = 0x10,
    FAT_ATTRIBUTE_ARCHIVE           = 0x20,
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

bool fat_init(partition_t* part);
fat_file_t * fat_open(partition_t* part, const char* path);
uint32_t fat_read(partition_t* part, fat_file_t* file, uint32_t byteCount, void* dataOut);
bool fat_read_entry(partition_t* part, fat_file_t* file, fat_directory_t* dirEntry);
void fat_close(fat_file_t* file);
