#include <fs/ata.h>
#include <fs/fat32.h>
#include <kprintf>
#include <kheap.h>
#include <string.h>

// this is surely already defined somewhere but I am not looking for it
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

typedef struct 
{
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_id[8];
} __attribute__((packed)) fat_extended_boot_record_t;

typedef struct 
{
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_directory_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved2[12];
    fat_extended_boot_record_t ebr;
} __attribute__((packed)) fat32_extended_boot_record_t;

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

    union {
        fat_extended_boot_record_t ebr1216;
        fat32_extended_boot_record_t ebr32;
    };

} __attribute__((packed)) fat_boot_sector_t;

typedef struct
{
    uint8_t buffer[SECTOR_SIZE];
    fat_file_t public;
    bool opened;
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t sector_in_clusteer;
} fat_file_data_t;

typedef struct {
    uint8_t order;
    int16_t chars[13];
} fat_lfn_block_t;

typedef struct
{
    union
    {
        fat_boot_sector_t boot_sector;
        uint8_t boot_sector_bytes[SECTOR_SIZE];
    } BS;

    fat_file_data_t root_directory;
    fat_file_data_t opened_files[MAX_FILE_HANDLES];

    uint8_t fat_cashe[FAT_CACHE_SIZE * SECTOR_SIZE];
    uint32_t fat_cashe_position;

    fat_lfn_block_t lfn_blocks[FAT_LFN_LAST];
    int lfn_count;

} FAT_Data;

static FAT_Data* g_data;
static uint32_t g_data_section_lba;
static uint32_t g_total_sectors;
static uint32_t g_sectors_per_fat;

static uint32_t cluster_to_lba(uint32_t cluster) {
    uint32_t lba = g_data_section_lba + (cluster - 2) * g_data->BS.boot_sector.sectors_per_cluster;
    return lba;
}

static bool read_bs(partition_t* disk)
{
    bool f = mbr_read_sector(disk, 0, 1, g_data->BS.boot_sector_bytes);
    return f;
}

static bool read_fat(partition_t* disk, size_t lbaIndex)
{
    return mbr_read_sector(disk, g_data->BS.boot_sector.reserved_sectors + lbaIndex, FAT_CACHE_SIZE, g_data->fat_cashe);
}

static char* get_short_name(const char* name)
{
    char* shortName = (char*)kmalloc(12);
    shortName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == nullptr)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        shortName[i] = toupper(name[i]);

    if (ext != name + 11)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            shortName[i + 8] = toupper(ext[i + 1]);
    }
    return shortName;
}

bool fat_init(partition_t* disk)
{
    g_data = (FAT_Data*)kmalloc(sizeof(FAT_Data));
    if (!g_data) {
        return false;
    }
    memset(g_data, 0, sizeof(FAT_Data));

    if (!read_bs(disk))
    {
        kprintf("FAT: failed to read boot sector\n");
        return false;
    }

    g_data->fat_cashe_position = 0xFFFFFFFF;

    g_total_sectors = g_data->BS.boot_sector.total_sectors;
    if (g_total_sectors == 0) {
        g_total_sectors = g_data->BS.boot_sector.large_sector_count;
    } else {
        kprintf("Not FAT32 Partition: %s\n", g_data->BS.boot_sector.total_sectors);
        return false;
    }

    g_sectors_per_fat = g_data->BS.boot_sector.sectors_per_fat;
    if (g_sectors_per_fat == 0) {
        g_sectors_per_fat = g_data->BS.boot_sector.ebr32.sectors_per_fat;
    } else {
        kprintf("Not FAT32 Partition: %s\n", g_data->BS.boot_sector.sectors_per_fat);
        return false;
    }
    
    uint32_t rootDirLba;
    g_data_section_lba = g_data->BS.boot_sector.reserved_sectors + g_sectors_per_fat * g_data->BS.boot_sector.fat_count;
    rootDirLba = cluster_to_lba( g_data->BS.boot_sector.ebr32.root_directory_cluster);

    g_data->root_directory.public.handle = ROOT_DIRECTORY_HANDLE;
    g_data->root_directory.public.is_directory = true;
    g_data->root_directory.public.position = 0;
    g_data->root_directory.public.size = sizeof(fat_directory_t) * g_data->BS.boot_sector.dir_entry_count;
    g_data->root_directory.opened = true;
    g_data->root_directory.first_cluster = rootDirLba;
    g_data->root_directory.current_cluster = rootDirLba;
    g_data->root_directory.sector_in_clusteer = 0;

    if (!mbr_read_sector(disk, rootDirLba, 1, g_data->root_directory.buffer))
    {
        kprintf("FAT: Failed to read root directory\n");
        return false;
    }

    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        g_data->opened_files[i].opened = false;
    g_data->lfn_count = 0;

    return true;
}

fat_file_t* fat_open_entry(partition_t* disk, fat_directory_t* entry)
{
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!g_data->opened_files[i].opened)
            handle = i;
    }

    if (handle < 0)
    {
        kprintf("FAT: open entry failed - no free handles\n");
        return nullptr;
    }

    fat_file_data_t* fd = &g_data->opened_files[handle];
    fd->public.handle = handle;
    fd->public.is_directory = (entry->attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->public.position = 0;
    fd->public.size = entry->size;
    fd->first_cluster = entry->first_cluster_low + ((uint32_t)entry->first_cluster_high << 16);
    fd->current_cluster = fd->first_cluster;
    fd->sector_in_clusteer = 0;

    if (!mbr_read_sector(disk, cluster_to_lba(fd->current_cluster), 1, fd->buffer))
    {
        kprintf("FAT: open entry failed - read error cluster=%u lba=%u\n  ", fd->current_cluster, cluster_to_lba(fd->current_cluster));
        for (int i = 0; i < 11; i++)
            kprintf("%c", entry->name[i]);
        kprintf("\n");
        return nullptr;
    }

    fd->opened = true;
    return &fd->public;
}

uint32_t fat_next_cluster(partition_t* disk, uint32_t currentCluster)
{    
    uint32_t fatIndex = currentCluster * 4;

    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    if (fatIndexSector < g_data->fat_cashe_position 
        || fatIndexSector >= g_data->fat_cashe_position + FAT_CACHE_SIZE)
    {
        read_fat(disk, fatIndexSector);
        g_data->fat_cashe_position = fatIndexSector;
    }

    fatIndex -= (g_data->fat_cashe_position * SECTOR_SIZE);

    uint32_t nextCluster = *(uint32_t*)(g_data->fat_cashe + fatIndex);

    return nextCluster;
}

uint32_t fat_read(partition_t* disk, fat_file_t* file, uint32_t byteCount, void* dataOut)
{
    fat_file_data_t* fd = (file->handle == ROOT_DIRECTORY_HANDLE) 
        ? &g_data->root_directory 
        : &g_data->opened_files[file->handle];

    uint8_t* u8DataOut = (uint8_t*)dataOut;

    if (!fd->public.is_directory || (fd->public.is_directory && fd->public.size != 0))
        byteCount = min(byteCount, fd->public.size - fd->public.position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->buffer + fd->public.position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->public.position += take;
        byteCount -= take;

        if (leftInBuffer == take)
        {
            if (fd->public.handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->current_cluster;

                if (!mbr_read_sector(disk, fd->current_cluster, 1, fd->buffer))
                {
                    kprintf("FAT: Failed to read root directory\n");
                    break;
                }
            }
            else
            {
                if (++fd->sector_in_clusteer >= g_data->BS.boot_sector.sectors_per_cluster)
                {
                    fd->sector_in_clusteer = 0;
                    fd->current_cluster = fat_next_cluster(disk, fd->current_cluster);
                }

                if (fd->current_cluster >= 0xFFFFFFF8)
                {
                    fd->public.size = fd->public.position;
                    break;
                }

                if (!mbr_read_sector(disk, cluster_to_lba(fd->current_cluster) + fd->sector_in_clusteer, 1, fd->buffer))
                {
                    kprintf("FAT: Failed to read cluster: %u\n", fd->current_cluster);
                    break;
                }
            }
        }
    }

    return u8DataOut - (uint8_t*)dataOut;
}

bool fat_read_entry(partition_t* disk, fat_file_t* file, fat_directory_t* dirEntry)
{
    return fat_read(disk, file, sizeof(fat_directory_t), dirEntry) == sizeof(fat_directory_t);
}

void FAT_Close(fat_file_t* file)
{
    if (file->handle == ROOT_DIRECTORY_HANDLE)
    {
        file->position = 0;
        g_data->root_directory.current_cluster = g_data->root_directory.first_cluster;
    }
    else
    {
        g_data->opened_files[file->handle].opened = false;
    }
}

bool FAT_FindFile(partition_t* disk, fat_file_t* file, const char* name, fat_directory_t* entryOut)
{
    char* shortName = get_short_name(name);
    fat_directory_t entry;

    while (fat_read_entry(disk, file, &entry))
    {
        if (memcmp(shortName, entry.name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }
    }
    
    return false;
}

fat_file_t* FAT_Open(partition_t* disk, const char* path)
{
    char name[MAX_PATH_SIZE];

    if (path[0] == '/')
        path++;

    fat_file_t* current = &g_data->root_directory.public;

    while (*path) {
        bool isLast = false;
        const char* token = strchr(path, '/');
        if (token != nullptr)
        {
            memcpy(name, path, token - path);
            name[token - path] = '\0';
            path = token + 1;
        }
        else
        {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }

        fat_directory_t entry;
        if (FAT_FindFile(disk, current, name, &entry))
        {
            FAT_Close(current);

            if (!isLast && (entry.attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)
            {
                kprintf("FAT: %s not a directory\n", name);
                return nullptr;
            }

            current = fat_open_entry(disk, &entry);
        }
        else
        {
            FAT_Close(current);

            kprintf("FAT: %s not found\n", name);
            return nullptr;
        }
    }

    return current;
}