#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    VFS_SUCCESS = 0,
    VFS_NOT_FOUND,
    VFS_NOT_PERMITTED,
} vfs_err_t;

enum VFS_TYPE {
    VFS_RAMFS_FOLDER,
    VFS_RAMFS_FILE,
};

enum VFS_PERMISSIONS {
    VFS_READ = 0x1,
    VFS_WRITE = 0x2,
    VFS_EXEC = 0x4
};

struct vfs_node {
    struct vfs_node* parent; // pointer to parent
    struct vfs_node* next; // pointer to next sibling
    struct vfs_node* children; // pointer to first child

    enum VFS_TYPE type;
    enum VFS_PERMISSIONS flags;

    char* name;
    uint64_t size;
    uint64_t creation_time;
    uint64_t modification_time;
    char owner_name[70];

    vfs_err_t (*read)(struct vfs_node *, uint32_t, uint32_t, uint8_t *);
    vfs_err_t (*write)(struct vfs_node *, uint32_t, uint32_t, const uint8_t *);
    vfs_err_t (*remove)(struct vfs_node *, struct vfs_node *);
    vfs_err_t (*create)(struct vfs_node *, const char *, enum VFS_TYPE, struct vfs_node ** /* will be filled */);
    vfs_err_t (*chmod)(struct vfs_node *, enum VFS_PERMISSIONS);
    vfs_err_t (*chown)(struct vfs_node *, const char *);
};

struct vfs_mount {
    struct vfs_node *mount_point;       // The directory in the main VFS where this is mounted
    struct vfs_node *mounted_root;      // The root node of the mounted filesystem
    struct vfs_mount *next;             // Pointer to next mount point
};

struct vfs_tree {
    struct vfs_node *root;
};
extern struct vfs_tree *g_Vfs;
extern struct vfs_mount *g_mounts;

struct vfs_tree *vfs_initialize();
struct vfs_node *vfs_create_node(const char *name, enum VFS_TYPE type, struct vfs_node *parent);
void vfs_insert_node(struct vfs_node *parent, struct vfs_node *node);

void vfs_remove_node(struct vfs_node *parent, struct vfs_node *node);
struct vfs_node *vfs_search_node(struct vfs_node *parent, const char *name);
int vfs_read(struct vfs_node *file, uint32_t offset, uint32_t size, uint8_t *buffer);
int vfs_write(struct vfs_node *file, uint32_t offset, uint32_t size, const uint8_t *buffer);
struct vfs_node *vfs_traverse_path(struct vfs_tree *vfs, const char *path);

