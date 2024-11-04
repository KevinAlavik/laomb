#pragma once

#include <stdint.h>
#include <stddef.h>
#include <spinlock.h>
#include <proc/sched.h>

struct timespec
{
    int32_t tv_sec;
    int32_t tv_nsec;
};
struct stat
{
    uint32_t st_dev;
    uint32_t st_ino;
    uint32_t st_nlink;
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t __pad0;
    uint32_t st_rdev;
    int64_t st_size;
    int32_t st_blksize;
    int64_t st_blocks;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    uint32_t __unused[6];
};

struct vnode
{
	struct handle* handle;
	struct filesystem* fs;
	struct vnode* parent;
	struct vnode* mount;
	struct vnode* hard_link;
	char* sym_link;
	char* name;
	bool initialised;

    struct vnode* first_child;
    struct vnode* next_sibling;
    // more?
};

struct filesystem
{
	char name[64];

	struct vnode* (*mount)(struct vnode* mount_point, const char* name, struct vnode* source);
	void (*init)(struct filesystem* self, struct vnode* parent);
	struct vnode* (*create)(struct filesystem* self, struct vnode* parent, const char* name, uint32_t mode);
	struct vnode* (*hard_link)(struct filesystem* self, struct vnode* parent, const char* name, struct vnode* target);
	struct vnode* (*sym_link)(struct filesystem* self, struct vnode* parent, const char* name, const char* target);

	bool initialised;
	struct filesystem* next;
};

struct fd
{
	struct handle* handle;
	size_t num_refs;
	size_t offset;
	struct vnode* node;
	struct spinlock lock;
};
struct fd* fd_from_num(struct JCB* proc, int fd);

struct handle
{
	struct spinlock lock;
	struct stat stat;

	int32_t (*read)(struct handle* self, struct fd* fd, void* output_buffer, size_t amount, int32_t offset);
	int32_t (*write)(struct handle* self, struct fd* fd, const void* input_buffer, size_t amount, int32_t offset);
	int32_t (*ioctl)(struct handle* self, struct fd* fd, uint32_t request, void* argument);
};
void* handle_new(size_t size);
size_t handle_new_device();

extern struct spinlock vfs_lock;

void vfs_init();
bool vfs_fs_register(struct filesystem* fs);
struct vnode* vfs_get_root();

struct vnode* vfs_node_new(struct filesystem* fs, struct vnode* parent, const char* name, bool is_dir);
struct vnode* vfs_node_add(struct vnode* parent, const char* name, uint32_t mode);
bool vfs_node_delete(struct vnode* node);
bool vfs_mount(struct vnode* parent, const char* src_path, const char* dest_path, const char* fs_name);
size_t vfs_get_path(struct vnode* target, char* buffer, size_t length);
bool vfs_create_dots(struct vnode* current, struct vnode* parent);
struct vnode* vfs_sym_link(struct vnode* parent, const char* path, const char* target);
struct vnode* vfs_resolve_node(struct vnode* node, bool follow_links);
struct vnode* vfs_get_node(struct vnode* parent, const char* path, bool follow_links);

#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)