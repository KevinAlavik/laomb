#include <proc/vfs.h>
#include <proc/tmpfs.h>
#include <kheap.h>
#include <kprintf>
#include <spinlock.h>
#include <string.h>

static struct filesystem tmpfs;

static uint32_t device_id = 0;
static uint32_t inode_counter = 0;

static int32_t tmpfs_handle_read(struct handle* self, struct fd*, void* buffer, size_t amount, int32_t offset)
{
	spinlock_lock(&self->lock);

	struct tmpfs_handle* const handle = (struct tmpfs_handle*)self;
	int32_t total_read = amount;

	if ((amount + offset) >= self->stat.st_size)
		total_read -= ((amount + offset) - self->stat.st_size);

	memcpy(buffer, handle->buffer + offset, total_read);

	spinlock_unlock(&self->lock);
	return total_read;
}

static int32_t tmpfs_handle_write(struct handle* self, struct fd*, const void* buffer, size_t amount, int32_t offset)
{
	spinlock_lock(&self->lock);

	struct tmpfs_handle* const handle = (struct tmpfs_handle*)self;

	int32_t written = -1;

	if (offset + amount >= handle->buffer_cap)
	{
		size_t new_capacity = handle->buffer_cap;
		if (new_capacity == 0)
			new_capacity = 0x1000;
		while (offset + amount >= new_capacity)
			new_capacity *= 2;

		void* new_data = krealloc(handle->buffer, new_capacity);
		if (new_data == nullptr)
		{
			goto fail;
		}

		handle->buffer = new_data;
		handle->buffer_cap = new_capacity;
	}

	memcpy(handle->buffer + offset, buffer, amount);

	if ((amount + offset) >= self->stat.st_size)
	{
		self->stat.st_size = (uint32_t)(amount + offset);
        self->stat.st_blocks = (((self->stat.st_size) + (self->stat.st_blksize) - 1) & ~((self->stat.st_blksize) - 1));
	}

	written = amount;

fail:
	spinlock_unlock(&self->lock);
	return written;
}

static struct tmpfs_handle* tmpfs_handle_new(struct filesystem*, uint32_t mode)
{
	struct tmpfs_handle* result = handle_new(sizeof(struct tmpfs_handle));
	if (result == nullptr)
		return nullptr;

	if (S_ISREG(mode))
	{
		result->buffer_cap = 0x1000;
		result->buffer = kmalloc(result->buffer_cap);
	}

	result->handle.stat.st_size = 0;
	result->handle.stat.st_blocks = 0;
	result->handle.stat.st_blksize = 512;
	result->handle.stat.st_dev = device_id++;
	result->handle.stat.st_ino = inode_counter++;
	result->handle.stat.st_mode = mode;
	result->handle.stat.st_nlink = 1;

	result->handle.read = tmpfs_handle_read;
	result->handle.write = tmpfs_handle_write;
	result->handle.ioctl = nullptr;

	return result;
}

static struct vnode* tmpfs_hard_link(struct filesystem* self, struct vnode* parent, const char* name, struct vnode* target)
{
    if (!S_ISREG(target->handle->stat.st_mode) && !S_ISDIR(target->handle->stat.st_mode))
        return nullptr;

    struct vnode* link = vfs_node_new(self, parent, name, S_ISDIR(target->handle->stat.st_mode));
    if (!link)
        return nullptr;

    link->handle = target->handle;
    link->hard_link = target;
    target->handle->stat.st_nlink++;

    return link;
}

static struct vnode* tmpfs_sym_link(struct filesystem* self, struct vnode* parent, const char* name, const char* target)
{
    struct vnode* link = vfs_node_new(self, parent, name, false);
    if (!link)
        return nullptr;

    size_t target_len = strlen(target) + 1;
    link->sym_link = (char*)kmalloc(target_len);
    if (!link->sym_link)
    {
        kfree(link);
        return nullptr;
    }

    strcpy(link->sym_link, target);
    link->handle = handle_new(sizeof(struct handle));
    if (!link->handle)
    {
        kfree(link->sym_link);
        kfree(link);
        return nullptr;
    }

    link->handle->stat.st_mode = S_IFLNK | 0777;
    link->handle->stat.st_size = target_len - 1;

    return link;
}

static struct vnode* tmpfs_create(struct filesystem* self, struct vnode* parent, const char* name, uint32_t mode)
{
    struct vnode* result = nullptr;
    struct tmpfs_handle* handle = nullptr;

    result = vfs_node_new(self, parent, name, S_ISDIR(mode));
    if (result == nullptr) {
        kprintf("tmpfs_create: Failed to create vnode\n");
        goto fail;
    }

    handle = tmpfs_handle_new(self, mode);
    if (handle == nullptr) {
        kprintf("tmpfs_create: Failed to allocate handle\n");
        goto fail;
    }

    result->handle = (struct handle*)handle;
    return result;

fail:
    if (result != nullptr)
        kfree(result);
    if (handle != nullptr)
        kfree(handle);
    return nullptr;
}

static struct vnode* tmpfs_mount(struct vnode* mount_point, const char* name, struct vnode* source)
{
    struct vnode* result = tmpfs.create(&tmpfs, mount_point, name, 0644 | S_IFDIR);
    if (result == nullptr) {
        kprintf("tmpfs_mount: Failed to mount tmpfs at %s\n", name);
    }
    return result;
}

static struct filesystem tmpfs = {
	.name = "tmpfs",
	.mount = tmpfs_mount,
	.init = nullptr,
	.create = tmpfs_create,
	.hard_link = tmpfs_hard_link,
	.sym_link = tmpfs_sym_link,
};

bool tmpfs_init()
{
	return vfs_fs_register(&tmpfs);
}