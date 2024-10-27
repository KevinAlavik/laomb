#pragma once

#include <proc/vfs.h>

enum VFS_DEVFS_DEVICE_TYPE {
    VFS_DEVFS_DEVICE_ATA,
};

struct vfs_devfs_node {
    struct vfs_node base;
    enum VFS_DEVFS_DEVICE_TYPE type;
    uint8_t drive;
};
vfs_err_t devfs_read(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_err_t devfs_write(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
vfs_err_t devfs_create(struct vfs_node *parent, const char *name, enum VFS_TYPE type, struct vfs_node **new_node);
vfs_err_t devfs_remove(struct vfs_node *parent, struct vfs_node *node);
vfs_err_t devfs_chmod(struct vfs_node *node, enum VFS_PERMISSIONS);

vfs_err_t devfs_chown(struct vfs_node *node, const char*);
struct vfs_tree* devfs_init();
vfs_err_t devfs_create_device(enum VFS_DEVFS_DEVICE_TYPE type, uint8_t drive);