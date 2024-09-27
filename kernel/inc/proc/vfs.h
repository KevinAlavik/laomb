#pragma once

#include <stdint.h>
#include <string.h>

typedef enum {
    VFS_SUCCESS = 0,
    VFS_ERROR = 1,
    VFS_NOT_FOUND = 2,
    VFS_NOT_PERMITTED = 3
} vfs_err_t;

#define VFS_READ        0x1
#define VFS_WRITE       0x2
#define VFS_EXEC        0x4

typedef uint32_t vfs_node_type_t;
typedef uint32_t vfs_permissions_t;

struct vfs_node {
    vfs_node_type_t type;                // Type
    char *name;                          // Name of the file or directory
    vfs_permissions_t permissions;       // File permissions
    uint32_t size;                       // Size of the file (0 for directories)
    uint64_t creation_time;              // Creation timestamp
    uint64_t modification_time;          // Modification timestamp

    struct vfs_node *parent;             // Parent directory
    struct vfs_node *children;           // Pointer to first child
    struct vfs_node *next;               // Pointer to next sibling

    // VFS Operations
    vfs_err_t (*read)(struct vfs_node *, uint32_t, uint32_t, uint8_t *);
    vfs_err_t (*write)(struct vfs_node *, uint32_t, uint32_t, const uint8_t *);
    vfs_err_t (*remove)(struct vfs_node *, struct vfs_node *);
    vfs_err_t (*create)(struct vfs_node *, const char *, vfs_node_type_t, struct vfs_node ** /* will be filled */);
};

struct vfs_tree {
    struct vfs_node *root;               // Root directory node
};

struct vfs_tree *vfs_initialize(); 
struct vfs_node *vfs_create_node(const char *name, vfs_node_type_t type, struct vfs_node *parent);
void vfs_insert_node(struct vfs_node *parent, struct vfs_node *node);

void vfs_remove_node(struct vfs_node *parent, struct vfs_node *node);
struct vfs_node *vfs_search_node(struct vfs_node *parent, const char *name);
int vfs_read(struct vfs_node *file, uint32_t offset, uint32_t size, uint8_t *buffer);
int vfs_write(struct vfs_node *file, uint32_t offset, uint32_t size, const uint8_t *buffer);
struct vfs_node *vfs_traverse_path(struct vfs_tree *vfs, const char *path);

// Helpers to unify VFS node creation and manipulation
vfs_err_t remove(const char* path);

extern struct vfs_tree *vfs;
