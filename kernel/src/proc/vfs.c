#include <proc/vfs.h>
#include <spinlock.h>
#include <kheap.h>
#include <io.h>
#include <kprintf>
#include <proc/sched.h>
#include <string.h>

struct vnode *vfs_create_node(const char *name, uint32_t flags, struct vnode *parent) {
    struct vnode *new_node = (struct vnode*)kmalloc(sizeof(struct vnode));
    if (!new_node) {
        return nullptr;
    }

    new_node->parent = parent;
    new_node->next = nullptr;
    new_node->child = nullptr;
    new_node->flags = flags;
    new_node->name = strdup(name);
    if (!new_node->name) {
        kfree(new_node);
        return nullptr;
    }

    new_node->size = 0;
    new_node->creation_time = new_node->modification_time = get_rtc_timestamp();
    strncpy(new_node->owner_name, "root", sizeof(new_node->owner_name) - 1);
    new_node->owner_name[sizeof(new_node->owner_name) - 1] = '\0';
    new_node->reference_count = 1;

    new_node->hard_link = nullptr;
    new_node->soft_link = nullptr;
    new_node->tree = nullptr;

    if (parent->child) {
        struct vnode *sibling = parent->child;
        while (sibling->next) {
            sibling = sibling->next;
        }
        sibling->next = new_node;
    } else {
        parent->child = new_node;
    }

    return new_node;
}

void vfs_remove_node(struct vnode *parent, struct vnode *node) {
    if (node->child) {
        return;
    }

    struct vnode *current = parent->child;
    struct vnode *previous = nullptr;

    while (current) {
        if (current == node) {
            if (previous) {
                previous->next = current->next;
            } else {
                parent->child = current->next;
            }
            kfree(current->name);
            kfree(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

struct vnode *vfs_search_node(struct vnode *parent, const char *name) {
    struct vnode *current = parent->child;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

struct vnode *vfs_traverse_path(struct vfs_tree *vfs, const char *path) {
    struct vnode *current = vfs->root;

    char *token = strtok(path, "/");
    while (token) {
        current = vfs_search_node(current, token);
        if (!current) {
            return nullptr;
        }
        token = strtok(nullptr, "/");
    }

    return current;
}

struct vnode *vfs_find_node(const char *path) {
    if (!path || path[0] != '/') {
        return nullptr;
    }
    return vfs_traverse_path(rootfs, path + 1);
}


int vfs_read(struct vnode *file, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (!file || !file->read) {
        return -1;
    }
    return file->read(file, offset, size, buffer);
}

int vfs_write(struct vnode *file, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    if (!file || !file->write) {
        return -1;
    }
    return file->write(file, offset, size, buffer);
}


vfs_err_t vfs_mount(const char *target_path, struct vfs_tree *filesystem) {
    struct vnode *mount_point = vfs_find_node(target_path);
    if (!mount_point || !(mount_point->flags & FILE_TYPE_DIR)) {
        return VFS_NOT_FOUND;
    }

    if (mount_point->tree) {
        return VFS_NOT_PERMITTED;
    }

    mount_point->tree = filesystem;
    filesystem->mount_point = mount_point;
    return VFS_SUCCESS;
}

vfs_err_t vfs_unmount(const char *target_path) {
    struct vnode *mount_point = vfs_find_node(target_path);
    if (!mount_point || !mount_point->tree) {
        return VFS_NOT_FOUND;
    }

    mount_point->tree->mount_point = nullptr;
    mount_point->tree = nullptr;
    return VFS_SUCCESS;
}