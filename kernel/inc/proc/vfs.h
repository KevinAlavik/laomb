#pragma once

#include <stdint.h>
#include <stddef.h>
#include <spinlock.h>
#include <proc/sched.h>

typedef enum {
    VFS_SUCCESS = 0,
    VFS_NOT_FOUND,
    VFS_NOT_PERMITTED,
    VFS_OVERFLOW,
    VFS_MEMORY_ERROR,
    VFS_IO_ERROR,
} vfs_err_t;

struct vnode {
    struct vnode* parent;
    struct vnode* next;
    struct vnode* child;

    uint16_t flags;
    char* name;
    uint64_t size;

    uint64_t creation_time;
    uint64_t modification_time;

    char owner_name[70];
    
	uint32_t reference_count;
	// todo flock() stuff

	struct vnode* hard_link; 		// nullptr if node is not a hard link
	char* soft_link; 				// nullptr if node is not a soft link
	struct vfs_tree* tree;			// If not nullptr, what filesystem is mounted over me?

	vfs_err_t (*open)(struct vnode* this, const char* name, uint16_t flags, struct vnode** out);
	vfs_err_t (*close)(struct vnode* this);
	vfs_err_t (*read)(struct vnode* this, uint32_t offset, uint32_t size, uint8_t* buffer);
	vfs_err_t (*write)(struct vnode* this, uint32_t offset, uint32_t size, const uint8_t* buffer);
	vfs_err_t (*remove)(struct vnode* this, const char* name);
	vfs_err_t (*ioctl)(struct vnode* this, uint32_t request, void* arg);
	vfs_err_t (*symlink)(struct vnode* this, struct vnode** out);
};

struct vfs_tree {
    struct vnode *root;
	struct vnode *mount_point; 		// nullptr for rootfs and unmounted systems.
	
	struct vfs_tree *next;				// next entry in the filesystems linked list
};

extern struct vfs_tree* rootfs;

bool vfs_initialize();
struct vnode *vfs_create_node(const char *name, uint32_t flags, struct vnode *parent);
void vfs_remove_node(struct vnode *parent, struct vnode *node);

struct vnode *vfs_search_node(struct vnode *parent, const char *name);
struct vnode *vfs_traverse_path(struct vfs_tree *vfs, const char *path);
struct vnode *vfs_find_node(const char* path);

int vfs_read(struct vnode *file, uint32_t offset, uint32_t size, uint8_t *buffer);
int vfs_write(struct vnode *file, uint32_t offset, uint32_t size, const uint8_t *buffer);

vfs_err_t vfs_mount(const char *target_path, struct vfs_tree *filesystem);
vfs_err_t vfs_unmount(const char *target_path);

#define PERM_OTHER(x)    ((x) & 0x7)         // "Others" permission bits (rwx)
#define PERM_GROUP(x)    (((x) & 0x7) << 3)  // "Group" permission bits (rwx)
#define PERM_OWNER(x)    (((x) & 0x7) << 6)  // "Owner" permission bits (rwx)

#define PERM_OTHER_READ    (0x1 << 0)
#define PERM_OTHER_WRITE   (0x2 << 0)
#define PERM_OTHER_EXEC    (0x4 << 0)

#define PERM_GROUP_READ    (0x1 << 3)
#define PERM_GROUP_WRITE   (0x2 << 3)
#define PERM_GROUP_EXEC    (0x4 << 3)

#define PERM_OWNER_READ    (0x1 << 6)
#define PERM_OWNER_WRITE   (0x2 << 6)
#define PERM_OWNER_EXEC    (0x4 << 6)

#define FILE_TYPE_REG    (0x1 << 9)   // Regular file
#define FILE_TYPE_DIR    (0x2 << 9)   // Directory
#define FILE_TYPE_LNK    (0x3 << 9)   // Symbolic link
#define FILE_TYPE_BLK    (0x4 << 9)   // Block device
#define FILE_TYPE_CHR    (0x5 << 9)   // Character device
#define FILE_TYPE_FIFO   (0x6 << 9)   // FIFO/pipe
#define FILE_TYPE_SOCK   (0x7 << 9)   // Socket

#define ACTION_RDONLY    (0x0 << 13)  // Read-only
#define ACTION_WRONLY    (0x1 << 13)  // Write-only
#define ACTION_RDWR      (0x2 << 13)  // Read-write
#define ACTION_CREAT     (0x3 << 13)  // Create
#define ACTION_APPEND    (0x4 << 13)  // Append
#define ACTION_TRUNC     (0x5 << 13)  // Truncate


#define CREATE_FLAGS(perm, type, action) \
    (PERM_OWNER(((perm) >> 6) & 0x7) | PERM_GROUP(((perm) >> 3) & 0x7) | PERM_OTHER((perm) & 0x7) | (type) | (action))
