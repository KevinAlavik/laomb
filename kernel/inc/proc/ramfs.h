#pragma once

#include <proc/vfs.h>

#define VFS_RAMFS_FILE 1
#define VFS_RAMFS_FOLDER 2

struct vfs_ramfs_node {
    struct vfs_node base;

    uint8_t* data;          // nullptr if size is 0 !!!!!
};

