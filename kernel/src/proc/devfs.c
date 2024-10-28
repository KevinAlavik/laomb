#include <proc/vfs.h>
#include <proc/devfs.h>
#include <driver/ata.h>
#include <proc/sched.h>
#include <driver/mbr.h>
#include <kheap.h>
#include <io.h>
#include <string.h>
#include <kprintf>

vfs_err_t devfs_read(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    struct vfs_devfs_node *devfs_node = (struct vfs_devfs_node *)node;
    if (!has_permission(node, VFS_READ)) return VFS_NOT_PERMITTED;

    uint32_t start_sector;
    uint32_t offset_within_sector;
    uint32_t num_sectors;
    uint32_t total_bytes_to_read;

    if (devfs_node->type == VFS_DEVFS_DEVICE_ATA) {
        start_sector = offset / 512;
        offset_within_sector = offset % 512;

        total_bytes_to_read = offset_within_sector + size;
        num_sectors = (total_bytes_to_read + 511) / 512;
    }
    else if (devfs_node->type == VFS_DEVFS_DEVICE_PARTITION) {
        struct partition *part = &partitions[devfs_node->drive & 0x03];
        start_sector = part->partition_offset + (offset / 512);
        offset_within_sector = offset % 512;

        total_bytes_to_read = offset_within_sector + size;
        num_sectors = (total_bytes_to_read + 511) / 512;
    }
    else {
        return VFS_NOT_PERMITTED;
    }

    uint8_t *sector_buffer = kmalloc(num_sectors * 512);
    if (!sector_buffer) return VFS_MEMORY_ERROR;

    if (ata_read(sector_buffer, start_sector, num_sectors, devfs_node->drive) == false) {
        kfree(sector_buffer);
        return VFS_IO_ERROR;
    }
    memcpy(buffer, sector_buffer + offset_within_sector, size);
    kfree(sector_buffer);

    return VFS_SUCCESS;
}

vfs_err_t devfs_write(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer)
{
    struct vfs_devfs_node *devfs_node = (struct vfs_devfs_node *)node;
    if (!has_permission(node, VFS_WRITE)) return VFS_NOT_PERMITTED;

    uint32_t start_sector;
    uint32_t offset_within_sector;

    if (devfs_node->type == VFS_DEVFS_DEVICE_ATA) {
        start_sector = offset / 512;
        offset_within_sector = offset % 512;
    }
    else if (devfs_node->type == VFS_DEVFS_DEVICE_PARTITION) {
        struct partition *part = &partitions[devfs_node->drive & 0x03];
        start_sector = part->partition_offset + (offset / 512);
        offset_within_sector = offset % 512;
    }
    else {
        return VFS_NOT_PERMITTED;
    }

    uint8_t *sector_buffer = kmalloc(512);
    if (!sector_buffer) return VFS_MEMORY_ERROR;

    if (offset_within_sector != 0) {
        if (!ata_read_one(sector_buffer, start_sector, devfs_node->drive)) {
            kfree(sector_buffer);
            return VFS_IO_ERROR;
        }
        uint32_t bytes_to_copy = (512 - offset_within_sector < size) ? 512 - offset_within_sector : size;
        memcpy(sector_buffer + offset_within_sector, buffer, bytes_to_copy);

        if (!ata_write_one(sector_buffer, start_sector, devfs_node->drive)) {
            kfree(sector_buffer);
            return VFS_IO_ERROR;
        }

        buffer += bytes_to_copy;
        size -= bytes_to_copy;
        start_sector++;
    }

    while (size >= 512) {
        memcpy(sector_buffer, buffer, 512);

        if (!ata_write_one(sector_buffer, start_sector, devfs_node->drive)) {
            kfree(sector_buffer);
            return VFS_IO_ERROR;
        }

        buffer += 512;
        size -= 512;
        start_sector++;
    }

    if (size > 0) {
        if (!ata_read_one(sector_buffer, start_sector, devfs_node->drive)) {
            kfree(sector_buffer);
            return VFS_IO_ERROR;
        }
        memcpy(sector_buffer, buffer, size);

        if (!ata_write_one(sector_buffer, start_sector, devfs_node->drive)) {
            kfree(sector_buffer);
            return VFS_IO_ERROR;
        }
    }

    kfree(sector_buffer);
    return VFS_SUCCESS;
}

vfs_err_t devfs_create(struct vfs_node *, const char *, enum VFS_TYPE , struct vfs_node **)
{
    return VFS_NOT_PERMITTED; // can not create new nodes
}
vfs_err_t devfs_remove(struct vfs_node *, struct vfs_node *)
{
    return VFS_NOT_PERMITTED; // can not remove nodes
}
vfs_err_t devfs_chmod(struct vfs_node *node, enum VFS_PERMISSIONS flags)
{
    if (node->type != VFS_RAMFS_FILE) return VFS_NOT_PERMITTED;

    if ((strcmp(current_job->job_owner_name, node->owner_name) != 0) && (strcmp(current_job->job_owner_name, "root") != 0)) return VFS_NOT_PERMITTED;

    node->flags = flags;
    return VFS_SUCCESS;
}
vfs_err_t devfs_chown(struct vfs_node *node, const char* name)
{
    if (node->type != VFS_RAMFS_FILE) return VFS_NOT_PERMITTED;

    if (strcmp(current_job->job_owner_name, "root") != 0) return VFS_NOT_PERMITTED;

    strncpy(node->owner_name, name, 70);
    return VFS_SUCCESS;
}

static struct vfs_tree* g_devfs_tree = nullptr;

struct vfs_tree* devfs_init()
{
    struct vfs_tree* fakeroot = vfs_initialize(); // this is some satanic shit coming to bite me in the ass for making vfs_create require a parent

    struct vfs_node* root = vfs_create_node("/", VFS_RAMFS_FOLDER, fakeroot->root);

    kfree(fakeroot->root->name);
    kfree(fakeroot->root);
    kfree(fakeroot);
    root->parent = nullptr;

    struct vfs_tree* tree = (struct vfs_tree*)kmalloc(sizeof(struct vfs_tree));
    tree->root = root;

    g_devfs_tree = tree;
    return tree;
}

vfs_err_t devfs_create_device(enum VFS_DEVFS_DEVICE_TYPE type, uint8_t drive)
{
    struct vfs_devfs_node* node = (struct vfs_devfs_node*)kmalloc(sizeof(struct vfs_devfs_node));
    memset(node, 0, sizeof(struct vfs_devfs_node));
    node->base.type = VFS_DEVFS_DEVICE;
    node->base.flags = VFS_READ;
    strncpy(node->base.owner_name, "root", 70); // devices are owned by root
    node->type = type;
    node->drive = drive;
    node->base.read = devfs_read;
    node->base.write = devfs_write;
    node->base.create = devfs_create;
    node->base.remove = devfs_remove;
    node->base.chmod = devfs_chmod;
    node->base.chown = devfs_chown;
    node->base.parent = g_devfs_tree->root;
    node->base.size = 0;
    node->base.creation_time = node->base.modification_time = get_rtc_timestamp();

    if (type == VFS_DEVFS_DEVICE_ATA) {
        switch (drive) {
            case ATA_PRIMARY << 1 | ATA_MASTER:
                node->base.name = strdup("80:0");
                break;
            case ATA_PRIMARY << 1 | ATA_SLAVE:
                node->base.name = strdup("80:1");
                break;
            case ATA_SECONDARY << 1 | ATA_MASTER:
                node->base.name = strdup("81:0");
                break;
            case ATA_SECONDARY << 1 | ATA_SLAVE:
                node->base.name = strdup("81:1");
                break;
        }
    } else if (type == VFS_DEVFS_DEVICE_PARTITION) {
        uint8_t bus = (drive >> 3) & 0x01;
        uint8_t device = (drive >> 2) & 0x01;
        uint8_t partition = drive & 0x03;
        char disk_name[8];
        ksnprintf(disk_name, sizeof(disk_name), "%d:%d:%d", 80 + bus, device, partition);
        node->base.name = strdup(disk_name);
    }
    else {
        node->base.name = "unknown";
    }

    vfs_insert_node(g_devfs_tree->root, (struct vfs_node*)node);
    return VFS_SUCCESS;
}