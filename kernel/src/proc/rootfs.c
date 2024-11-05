#include <proc/vfs.h>
#include <spinlock.h>
#include <kheap.h>
#include <io.h>
#include <kprintf>
#include <proc/sched.h>
#include <string.h>

struct vfs_tree* rootfs;

static vfs_err_t rootfs_open(struct vnode* this, const char* name, uint16_t flags, struct vnode** out);
static vfs_err_t rootfs_close(struct vnode* this);
static vfs_err_t rootfs_read(struct vnode* this, uint32_t offset, uint32_t size, uint8_t* buffer);
static vfs_err_t rootfs_write(struct vnode* this, uint32_t offset, uint32_t size, const uint8_t* buffer);
static vfs_err_t rootfs_remove(struct vnode* this, const char* name);
static vfs_err_t rootfs_symlink(struct vnode* this, struct vnode** out);
static vfs_err_t rootfs_ioctl(struct vnode* this, uint32_t request, void* arg);

bool vfs_initialize() {
	rootfs = (struct vfs_tree*)kmalloc(sizeof(struct vfs_tree));
	rootfs->mount_point = nullptr;
	rootfs->next = nullptr;

	struct vnode* root = (struct vnode*)kmalloc(sizeof(struct vnode));
	root->parent = nullptr;
	root->next = nullptr;
	root->child = nullptr;
	root->flags = CREATE_FLAGS(PERM_OTHER_READ | PERM_GROUP_READ | PERM_OWNER_READ 
				| PERM_OWNER_WRITE, FILE_TYPE_DIR, 0);
	root->name = strdup("/");
	if (!root->name) return false;
	root->size = 0;

	root->creation_time = root->modification_time = get_rtc_timestamp();
	strncpy(root->owner_name, "root", 70);

	root->hard_link = nullptr;
	root->soft_link = nullptr;
	root->tree = nullptr;

	root->open = rootfs_open;
	root->close = rootfs_close;
	root->read = rootfs_read;
	root->write = rootfs_write;
	root->remove = rootfs_remove;
	root->symlink = rootfs_symlink;
	root->ioctl = rootfs_ioctl;

	rootfs->root = root;
	root->reference_count = 1;
	
	return true;
}

static vfs_err_t rootfs_open(struct vnode* this, const char* name, uint16_t flags, struct vnode** out) {
	bool create = flags & ACTION_CREAT ? true : false;
	
	struct vnode* current = this->child;
	struct vnode* last = nullptr;
	while (current) {
		if (strcmp(name, current->name) == 0) {
			goto found;
		}
		last = current;
		current = current->next;
	}

	if (create) {
		current = (struct vnode*)kmalloc(sizeof(struct vnode));
		if (!current) {
			return VFS_MEMORY_ERROR;
		}
		current->parent = this;
		current->next = nullptr;
		current->child = nullptr;
		current->flags = flags & ~ACTION_CREAT;
		current->name = strdup(name);
		if (!current->name) {
			return VFS_MEMORY_ERROR;
		}
		current->size = 0;
		current->creation_time = current->modification_time = get_rtc_timestamp();
		strncpy(current->owner_name, "root", 70);
		current->hard_link = nullptr;
		current->soft_link = nullptr;
		current->tree = nullptr;

		current->open = rootfs_open;
		current->close = rootfs_close;
		current->read = rootfs_read;
		current->write = rootfs_write;
		current->remove = rootfs_remove;
		current->symlink = rootfs_symlink;
		current->ioctl = rootfs_ioctl;

		if (last) {
			last->next = current;
		} else {
			this->child = current;
		}
		
		*out = current;
		current->reference_count = 2;
		return VFS_SUCCESS;
	} else {
		return VFS_NOT_FOUND;
	}

found:
	if (create) {
		return VFS_NOT_PERMITTED;
	} else {
		if (!(current->flags & FILE_TYPE_REG)) {
			return VFS_NOT_PERMITTED;
		} else {
			*out = current;
			current->reference_count++;
			return VFS_SUCCESS;
		}
	}
}

static vfs_err_t rootfs_close(struct vnode* this) {
	this->reference_count--;
	if (this->reference_count == 0) {
		kmfree(this->name);
		kmfree(this);
	}
	return VFS_SUCCESS;
}

static vfs_err_t rootfs_read(struct vnode* this, uint32_t offset, uint32_t size, uint8_t* buffer) {
	return VFS_IO_ERROR;
}

static vfs_err_t rootfs_write(struct vnode* this, uint32_t offset, uint32_t size, const uint8_t* buffer) {
	return VFS_IO_ERROR;
}

static vfs_err_t rootfs_remove(struct vnode* this, const char* name) {
	struct vnode* current = this->child;
	struct vnode* last = nullptr;
	while (current) {
		if (strcmp(name, current->name) == 0) {
			if (current->flags & FILE_TYPE_DIR) {
				return VFS_NOT_PERMITTED;
			} else {
				vfs_remove_node(this, current);
				return VFS_SUCCESS;
			}
		}
		last = current;
		current = current->next;
	}
	return VFS_NOT_FOUND;
}

static vfs_err_t rootfs_symlink(struct vnode* this, struct vnode** out) {
	if (this->flags & FILE_TYPE_LNK && this->soft_link != nullptr) {
		*out = vfs_find_node(this->soft_link);
		return VFS_SUCCESS;
	} else if (this->hard_link != nullptr) {
		*out = this->hard_link;
	} else {
		return VFS_NOT_FOUND;
	}
}

static vfs_err_t rootfs_ioctl(struct vnode* this, uint32_t request, void* arg) {
	return VFS_NOT_PERMITTED;
}