#include <proc/vfs.h>
#include <proc/ramfs.h>
#include <kheap.h>
#include <string.h>
#include <io.h>

/**
 * 
 * 
 *          RAMFS
 * 
 * 
 */
vfs_err_t ramfs_read(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    struct vfs_ramfs_node *ramfs_node = (struct vfs_ramfs_node *)node;
    if (node->type != VFS_RAMFS_FILE) return VFS_ERROR;
    memcpy(buffer, ramfs_node->data + offset, size);
    return VFS_SUCCESS;
}

vfs_err_t ramfs_write(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    struct vfs_ramfs_node *ramfs_node = (struct vfs_ramfs_node *)node;
    if (node->type != VFS_RAMFS_FILE) return VFS_ERROR;

    if (ramfs_node->data == nullptr) {
        ramfs_node->data = (uint8_t *)kmalloc(size);
        ramfs_node->base.size = size;
    } else if (offset + size > ramfs_node->base.size) {
        ramfs_node->data = (uint8_t *)krealloc(ramfs_node->data, offset + size);
        ramfs_node->base.size = offset + size;
    } else if (offset + size < ramfs_node->base.size) {
        memset(ramfs_node->data + offset + size, 0, ramfs_node->base.size - offset - size);
    }

    memcpy(ramfs_node->data + offset, buffer, size);
    ramfs_node->base.modification_time = get_rtc_timestamp();
    return VFS_SUCCESS;
}

vfs_err_t ramfs_create(struct vfs_node *parent, const char *name, vfs_node_type_t type, struct vfs_node **new_node) {
    if (parent->type != VFS_RAMFS_FOLDER) return VFS_NOT_PERMITTED;

    *new_node = vfs_create_node(name, type, parent);
    struct vfs_ramfs_node *ramfs_node = (struct vfs_ramfs_node *)(*new_node);
    ramfs_node->data = nullptr;

    parent->modification_time = get_rtc_timestamp();            
    return VFS_SUCCESS;
}

vfs_err_t ramfs_remove(struct vfs_node *parent, struct vfs_node *node) {
    if (parent->type != VFS_RAMFS_FOLDER) return VFS_NOT_PERMITTED;

    vfs_remove_node(parent, node);

    struct vfs_ramfs_node *ramfs_node = (struct vfs_ramfs_node *)node;
    if (ramfs_node->data) kfree(ramfs_node->data);
    return VFS_SUCCESS;
}

/**
 * 
 * 
 *          VFS
 * 
 */
/**
 * Initialize the VFS tree with the root node.
 */
struct vfs_tree *vfs_initialize() {
    struct vfs_tree *vfs = (struct vfs_tree *)kmalloc(sizeof(struct vfs_tree));
    
    struct vfs_ramfs_node *ramfs_root = (struct vfs_ramfs_node *)kmalloc(sizeof(struct vfs_ramfs_node));
    struct vfs_node *root = &ramfs_root->base;

    root->name = strdup("/");
    root->type = VFS_RAMFS_FOLDER;
    root->size = 0;
    root->permissions = VFS_READ | VFS_WRITE;
    root->creation_time = root->modification_time = get_rtc_timestamp();
    root->parent = nullptr;
    root->children = nullptr;
    root->next = nullptr;

    root->read = ramfs_read;
    root->write = ramfs_write;
    root->create = ramfs_create;
    root->remove = ramfs_remove;

    ramfs_root->data = nullptr;

    vfs->root = root;
    return vfs;
}

struct vfs_node *vfs_create_node(const char *name, vfs_node_type_t type, struct vfs_node *parent) {
    struct vfs_node *node;

    if (type == VFS_RAMFS_FILE || type == VFS_RAMFS_FOLDER) {
        struct vfs_ramfs_node *ramfs_node = (struct vfs_ramfs_node *)kmalloc(sizeof(struct vfs_ramfs_node));
        node = &ramfs_node->base;
        ramfs_node->data = nullptr;
    } else {
        return nullptr;
    }

    node->name = strdup(name);
    node->type = type;
    node->size = 0;
    node->permissions = VFS_READ | VFS_WRITE;
    node->creation_time = node->modification_time = get_rtc_timestamp();
    node->parent = parent;
    node->children = nullptr;
    node->next = nullptr;

    if (type == VFS_RAMFS_FILE || type == VFS_RAMFS_FOLDER) {
        node->read = ramfs_read;
        node->write = ramfs_write;
        node->create = ramfs_create;
        node->remove = ramfs_remove;
    }

    vfs_insert_node(parent, node);
    return node;
}

void vfs_insert_node(struct vfs_node *parent, struct vfs_node *node) {
    if (!parent->children) {
        parent->children = node;
    } else {
        struct vfs_node *sibling = parent->children;
        while (sibling->next) {
            sibling = sibling->next;
        }
        sibling->next = node;
    }
}

void vfs_remove_node(struct vfs_node *parent, struct vfs_node *node) {
    if (parent->children == node) {
        parent->children = node->next;
    } else {
        struct vfs_node *prev = parent->children;
        while (prev && prev->next != node) {
            prev = prev->next;
        }
        if (prev) prev->next = node->next;  
    }

    if (node->children) {
        struct vfs_node *child = node->children;
        while (child) {
            struct vfs_node *next_child = child->next;
            vfs_remove_node(node, child);
            child = next_child;
        }   
    }

    memset(node, 0, sizeof(struct vfs_node));
    kfree(node->name);
    kfree(node);
}

struct vfs_node *vfs_search_node(struct vfs_node *parent, const char *name) {
    struct vfs_node *current = parent->children;
    while (current) {
        if (strcmp(current->name, name) == 0) return current;
        current = current->next;
    }
    return nullptr;
}

int vfs_read(struct vfs_node *file, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (file->read) return file->read(file, offset, size, buffer);
    return VFS_NOT_PERMITTED;
}

int vfs_write(struct vfs_node *file, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    if (file->write) return file->write(file, offset, size, buffer);
    return VFS_NOT_PERMITTED;
}

struct vfs_node *vfs_traverse_path(struct vfs_tree *vfs, const char *path) {
    if (!path || *path == '\0') return nullptr;

    char *path_copy = strdup(path);
    if (!path_copy) return nullptr;

    char *token = strtok(path_copy, "/");
    struct vfs_node *current = vfs->root;

    while (token) {
        current = vfs_search_node(current, token);
        if (!current) {
            kfree(path_copy);
            return nullptr;
        }
        token = strtok(nullptr, "/");
    }
    kfree(path_copy);
    return current;
}