#pragma once

#include <stdint.h>
#include <proc/vfs.h>

void initrd_load(struct vfs_node* mount_point, const uint8_t* initrd_start, size_t initrd_size);
