#pragma once

#include <stdint.h>
#include <stddef.h>

enum vnode_type {
    VNODE_FILE,
    VNODE_DIR,
    VNODE_BLOCK_DEVICE,
    VNODE_CHAR_DEVICE,
    VNODE_SYMLINK,
    VNODE_SOCKET,
    VNODE_BAD
};

struct vfs_operations {
    int (*mount)(struct vfs* vfs, const char* path, struct vnode* mountat);
    int (*unmount)(struct vfs* vfs);
    int (*groot)(struct vfs* vfs, struct vnode** root); // root will be filled with a pointer to the root node
    int (*sync)(struct vfs* vfs);
    int (*allocate_fd)(struct vfs* vfs, struct vnode* vnode, uint32_t flags, uint64_t* fd);
    int (*close_fd)(struct vfs* vfs, uint64_t fd);
};

struct vfs {
    uintptr_t vfs_data;                     // Filesystem dependant data
    struct vfs_operations* vfs_op;          // Standartised functions to operate on the filesystem
    struct vnode* mount;                    // Where the filesystem is mounted, nullptr for root vfs and non-mounted filesystems

    uint16_t flags;                         // VFS flags
    uint16_t block_size;                    // VFS block size, such as 512 for fdc and ata devices

    struct vfs* next;
};

struct vnode_operations {
    int (*open)(struct vnode* vnode, const char* name, uint32_t flags, struct vnode** out);
    int (*close)(struct vnode* vnode);
    int (*rw)(struct vnode* vnode, uint8_t* buffer, size_t len, uint32_t flags, bool write);
    int (*access)(struct vnode* vnode, uint32_t mode);
    int (*create)(struct vnode* vnode, const char* name, uint32_t mode, uint32_t flags);
    int (*remove)(struct vnode* vnode, const char* name);
    int (*rename)(struct vnode* vnode, const char* oldname, const char* newname);
    int (*mkdir)(struct vnode* vnode, const char* name, uint32_t mode);
    int (*rmdir)(struct vnode* vnode, const char* name);
    int (*readdir)(struct vnode* vnode, const char** names, size_t* count);

    int (*link)(struct vnode* vnode, const char* name, struct vnode* target);
    int (*unlink)(struct vnode* vnode);
    int (*symlink)(struct vnode* vnode, const char* name, uint32_t flags, struct vnode** out); // like open() but if used on a symlink gets what it points to.
    int (*fsync)(struct vnode* vnode); // like sync() but for a specific vnode
    
    int (*ioctl)(struct vnode* vnode, uint32_t request, void* arg);
};

struct vnode {
    enum vnode_type type;
    struct vfs* vfs;

    uint16_t slocks;                        // For flock()'s shared locks
    uint16_t elocks;                        // For flock()'s exclusive locks
    uint16_t refcount;

    struct vfs* mount;                      // If there is a filesystem mounted over me, this is it.
    struct vnode_operations* vnode_op;      // Standartised functions to operate on the vnode
};

extern struct vfs* root_vfs;

bool vfs_initroot();

bool vfs_mount(struct vfs* fs, const char* path, struct vnode* node);
bool vfs_unmount(struct vnode* node);

int vfs_resolve_path(const char* path, struct vnode** out_vnode);
int vfs_close_fd(struct vfs* vfs, uint64_t fd);

enum flock_type { // advisory locks :)
    FLOCK_SHARED,
    FLOCK_EXCLUSIVE,
    FLOCK_ULOCK_EXCLUSIVE,
    FLOCK_ULOCK_SHARED
};

int vfs_flock(struct vnode* vnode, enum flock_type lock_type);
int vfs_sync_all();

int vfs_walk(struct vnode* vnode, const char* path, struct vnode** result);

