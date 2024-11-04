#include <proc/vfs.h>
#include <proc/sched.h>
#include <stdint.h>
#include <stddef.h>
#include <io.h>
#include <string.h>
#include <kheap.h>
#include <kprintf>
#include <proc/tmpfs.h>

struct path_to_node
{
	struct vnode* target;
	struct vnode* parent;
	char* name;
};

struct spinlock vfs_lock;
static struct vnode* vfs_root = nullptr;
struct filesystem* filesystems;
static size_t device_counter = 1;
static struct spinlock device_counter_lock;

void vfs_init()
{
	vfs_root = vfs_node_new(nullptr, nullptr, "", true);
    tmpfs_init();
    
	vfs_mount(vfs_root, nullptr, "/", "tmpfs");

	vfs_node_add(vfs_root, "/tmp", 0755 | S_IFDIR);
    vfs_mount(vfs_root, nullptr, "/tmp", "tmpfs");
}

struct vnode* vfs_get_root()
{
	return vfs_root;
}

bool vfs_fs_register(struct filesystem* fs)
{
    if (!fs) return false;

	spinlock_lock(&vfs_lock);
    struct filesystem* curr = filesystems;
    if (curr == nullptr) {
        filesystems = fs;
        fs->next = nullptr;
        return true;
    } else {
        struct filesystem* curr = filesystems;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = fs;
        fs->next = nullptr;
    }

    fs->next = nullptr;
	spinlock_unlock(&vfs_lock);
    
    kprintf("Registered filesystem %s\n", fs->name);
	return true;
}

struct vnode* vfs_node_new(struct filesystem* fs, struct vnode* parent, const char* name, bool is_dir)
{
    if (!name) return nullptr;

	struct vnode* node = kmalloc(sizeof(struct vnode));
    if (!node) return nullptr;
    memset(node, 0, sizeof(struct vnode));

	node->parent = parent;
	node->fs = fs;

	if (is_dir) {
        node->first_child = nullptr;
        node->next_sibling = nullptr;
    }

	size_t name_len = strlen(name) + 1;
	node->name = kmalloc(name_len);
	if (!node->name) {
		kfree(node);
		return nullptr;
	}
	memcpy(node->name, name, name_len);

	if (parent) {
		struct vnode* curr = parent->first_child;
		if (curr == nullptr) {
			parent->first_child = node;
		} else {
			while (curr->next_sibling) {
				curr = curr->next_sibling;
			}
			curr->next_sibling = node;
		}
	}

	return node;
}


bool vfs_node_delete(struct vnode* node)
{
    if (!node) return false;
    spinlock_lock(&vfs_lock);

    struct vnode* child = node->first_child;
    while (child) {
        struct vnode* next_sibling = child->next_sibling;
        vfs_node_delete(child);
        child = next_sibling;
    }

    if (node->sym_link) {
        kfree(node->sym_link);
        node->sym_link = nullptr;
    }

    if (node->handle && node->handle->stat.st_nlink > 0) {
        node->handle->stat.st_nlink--;
        if (node->handle->stat.st_nlink == 0) {
            kfree(node->handle);
            node->handle = nullptr;
        }
    }

    if (node->name) {
        kfree(node->name);
        node->name = nullptr;
    }

    kfree(node);
    spinlock_unlock(&vfs_lock);
    return true;
}

bool vfs_create_dots(struct vnode* current, struct vnode* parent)
{
    if (!current || !parent) return false;

    struct vnode* dot = vfs_node_new(parent->fs, current, ".", true);
    if (!dot) return false;
    dot->hard_link = current;

    struct vnode* dot2 = vfs_node_new(parent->fs, parent, "..", true);
    if (!dot2) {
        kfree(dot->name);
        kfree(dot);
        return false;
    }
    dot2->hard_link = parent;

    if (current->first_child == nullptr) {
        current->first_child = dot;
        dot->next_sibling = dot2;
    } else {
        struct vnode* last = current->first_child;
        while (last->next_sibling) {
            last = last->next_sibling;
        }
        last->next_sibling = dot;
        dot->next_sibling = dot2;
    }

    return true;
}

static bool vfs_populate(struct vnode* directory)
{
	if (directory == nullptr)
		return false;

	if (directory->fs && directory->fs->initialised && directory->initialised == false && directory->handle &&
		S_ISDIR(directory->handle->stat.st_mode))
	{
		directory->fs->init(directory->fs, directory);
		return directory->initialised;
	}
	return true;
}

static struct path_to_node vfs_parse_path(struct vnode* parent, const char* path)
{
    if (path == nullptr || strlen(path) == 0)
        return (struct path_to_node) {nullptr, nullptr, nullptr};

    const size_t path_len = strlen(path);
    bool path_is_dir = path[path_len - 1] == '/';

    size_t i = 0;
    struct vnode* current_node = vfs_resolve_node(parent, false);
    if (!vfs_populate(current_node))
        return (struct path_to_node) {nullptr, nullptr, nullptr};

    if (path[i] == '/') {
        current_node = vfs_resolve_node(vfs_get_root(), false);
        while (path[i] == '/') {
            if (i == path_len - 1)
                return (struct path_to_node) {current_node, current_node, strdup("/")};
            i++;
        }
    }

    while (true) {
        const char* elem = &path[i];
        size_t part_length = 0;

        while (i < path_len && path[i] != '/')
            part_length++, i++;

        while (i < path_len && path[i] == '/')
            i++;

        bool last = (i == path_len);

        char* elem_str = kmalloc(part_length + 1);
        memcpy(elem_str, elem, part_length);

        current_node = vfs_resolve_node(current_node, false);
        struct vnode* new_node = nullptr;

        struct vnode* child = current_node->first_child;
        while (child) {
            if (strcmp(child->name, elem_str) == 0) {
                new_node = child;
                break;
            }
            child = child->next_sibling;
        }

        if (!new_node) {
            if (last)
                return (struct path_to_node) {nullptr, current_node, elem_str};
            kfree(elem_str);
            return (struct path_to_node) {nullptr, nullptr, nullptr};
        }

        new_node = vfs_resolve_node(new_node, false);
        if (!vfs_populate(new_node)) {
            kfree(elem_str);
            return (struct path_to_node) {nullptr, nullptr, nullptr};
        }

        if (last) {
            if (path_is_dir && !(new_node->handle->stat.st_mode & S_IFDIR))
                return (struct path_to_node) {nullptr, current_node, elem_str};
            return (struct path_to_node) {new_node, current_node, elem_str};
        }

        current_node = new_node;

        if (current_node->handle->stat.st_mode & S_IFLNK) {
            struct path_to_node result = vfs_parse_path(current_node->parent, current_node->sym_link);
            if (result.target == nullptr) {
                kfree(elem_str);
                return (struct path_to_node) {nullptr, nullptr, nullptr};
            }
            current_node = result.target;
        }

        if (!(current_node->handle->stat.st_mode & S_IFDIR)) {
            kfree(elem_str);
            return (struct path_to_node) {nullptr, nullptr, nullptr};
        }
    }

    return (struct path_to_node) {nullptr, nullptr, nullptr};
}

struct vnode* vfs_resolve_node(struct vnode* node, bool follow_links)
{
	if (node == nullptr)
		return nullptr;

	if (node->hard_link != nullptr)
		return vfs_resolve_node(node->hard_link, follow_links);

	if (node->mount != nullptr)
		return vfs_resolve_node(node->mount, follow_links);

	if (node->sym_link != nullptr && follow_links)
	{
		struct path_to_node parsed = vfs_parse_path(node->parent, node->sym_link);
		if (parsed.target == nullptr)
			return nullptr;
		return vfs_resolve_node(parsed.target, true);
	}

	return node;
}

struct vnode* vfs_node_add(struct vnode* parent, const char* name, uint32_t mode)
{
	spinlock_lock(&vfs_lock);

	struct vnode* node = nullptr;
	struct path_to_node parsed = vfs_parse_path(parent, name);

	if (parsed.parent == nullptr)
		goto leave;

	if (parsed.target != nullptr)
		goto leave;

	struct filesystem* const target_fs = parsed.parent->fs;

	struct vnode* target_node = target_fs->create(target_fs, parsed.parent, parsed.name, mode);

    struct vnode* curr = parsed.parent->first_child;
    if (curr == nullptr) {
        parsed.parent->first_child = target_node;
    } else {
        while (curr->next_sibling) {
            curr = curr->next_sibling;
        }
        curr->next_sibling = target_node;
    }
    target_node->parent = parsed.parent;

	if (S_ISDIR(target_node->handle->stat.st_mode))
		vfs_create_dots(target_node, parsed.parent);

	node = target_node;

leave:
	if (parsed.name != nullptr)
		kfree(parsed.name);
	spinlock_unlock(&vfs_lock);
	return node;
}

bool vfs_mount(struct vnode* parent, const char* src_path, const char* dest_path, const char* fs_name)
{
	bool result = false;
	struct path_to_node parsed = {0};
	struct vnode* source_node = nullptr;

	spinlock_lock(&vfs_lock);

	struct filesystem* fs = nullptr;
	for (struct filesystem* current_fs = filesystems; current_fs != nullptr; current_fs = current_fs->next) {
		if (strncmp(current_fs->name, fs_name, sizeof(current_fs->name)) == 0) {
			fs = current_fs;
			break;
		}
	}

	if (fs == nullptr)
	{
		kprintf("Unable to mount file system \"%s\": Not previously registered!\n", fs_name);
		goto leave;
	}

	if (src_path != nullptr && strlen(src_path) != 0)
	{
		parsed = vfs_parse_path(parent, src_path);
		source_node = parsed.target;
		if (source_node == nullptr)
			goto leave;
		if (!S_ISDIR(source_node->handle->stat.st_mode))
		{
			goto leave;
		}
	}

	parsed = vfs_parse_path(parent, dest_path);

	if (parsed.target == nullptr)
		goto leave;

	if (parsed.target != vfs_root && !S_ISDIR(parsed.target->handle->stat.st_mode))
	{
		goto leave;
	}

	struct vnode* mount_node = fs->mount(parsed.parent, parsed.name, source_node);
	if (mount_node == nullptr)
	{
		kprintf("Mounting \"%s\" failed!\n", dest_path);
		goto leave;
	}

	parsed.target->mount = mount_node;
	vfs_create_dots(mount_node, parsed.parent);

	if (src_path != nullptr && strlen(src_path) != 0)
		kprintf("Mounted \"%s\" on \"%s\" with file system \"%s\".\n", src_path, dest_path, fs_name);
	else
		kprintf("Mounted new file system \"%s\" on \"%s\".\n", fs_name, dest_path);

	result = true;

leave:
	if (parsed.name != nullptr)
		kfree(parsed.name);
	spinlock_unlock(&vfs_lock);
	return result;
}

struct vnode* vfs_sym_link(struct vnode* parent, const char*, const char* target)
{
	spinlock_lock(&vfs_lock);

	struct vnode* result = nullptr;
    struct path_to_node parsed = vfs_parse_path(parent, target);

	if (parsed.target == nullptr)
		goto leave;

	if (parsed.target->parent == nullptr)
		goto leave;

	if (parsed.target != nullptr)
	{
		goto leave;
	}

	struct filesystem* const target_fs = parsed.target->parent->fs;
	struct vnode* source_node = target_fs->sym_link(target_fs, parsed.target->parent, parsed.target->name, target);
    struct vnode* curr = parsed.target->parent->first_child;
    if (curr == nullptr) {
        parsed.target->parent->first_child = source_node;
    } else {
        while (curr->next_sibling) {
            curr = curr->next_sibling;
        }
        curr->next_sibling = source_node;
    }
    source_node->parent = parsed.target->parent;

	result = source_node;
leave:
	spinlock_unlock(&vfs_lock);
	return result;
}

size_t vfs_get_path(struct vnode* target, char* buffer, size_t length)
{
	if (target == nullptr)
		return 0;

	size_t offset = 0;
	if (target->parent != vfs_root && target->parent != nullptr)
	{
		struct vnode* parent = vfs_resolve_node(target->parent, false);

		if (parent != vfs_root && parent != nullptr)
		{
			offset += vfs_get_path(parent, buffer, length - offset - 1);
			buffer[offset++] = '/';
		}
	}

	if (memcmp(target->name, "/", 1) != 0)
	{
		memcpy(buffer + offset, target->name, length - offset);
		return strlen(target->name) + offset;
	}
	return offset;
}

struct vnode* vfs_get_node(struct vnode* parent, const char* path, bool follow_links)
{
	struct vnode* ret = nullptr;

	struct path_to_node r = vfs_parse_path(parent, path);
	if (r.target == nullptr)
		goto leave;

	if (follow_links)
	{
		ret = vfs_resolve_node(r.target, true);
		goto leave;
	}

	ret = r.target;

leave:
	if (r.name != nullptr)
		kfree(r.name);
	return ret;
}

struct fd* fd_from_num(struct JCB* proc, int fd)
{
	struct fd* result = nullptr;
	if (proc == nullptr)
		proc = sched_get_current_job();

	spinlock_lock(&proc->fd_lock);

	if (fd < 0 || fd >= 256)
	{
		spinlock_unlock(&proc->fd_lock);
		return nullptr;
	}

	result = proc->file_descs[fd];
	if (result == nullptr)
	{
		spinlock_unlock(&proc->fd_lock);
		return nullptr;
	}

	result->num_refs++;
	spinlock_unlock(&proc->fd_lock);

	return result;
}

static int32_t handle_default_read(struct handle*, struct fd*, void*, size_t, int32_t)
{
	return -1;
}

static int32_t handle_default_write(struct handle*, struct fd*, const void*, size_t, int32_t)
{
	return -1;
}

static int32_t handle_default_ioctl(struct handle*, struct fd*, uint32_t, void*)
{
	return -1;
}

void* handle_new(size_t size)
{
	if (size < sizeof(struct handle)) {
		kprintf("Can't allocate a handle with less than %zu bytes, but only got %zu!\n", sizeof(struct handle), size);
		return nullptr;
	}
	
	struct handle* result = kmalloc(size);

	result->read = handle_default_read;
	result->write = handle_default_write;
	result->ioctl = handle_default_ioctl;

	return result;
}

size_t handle_new_device()
{
	spinlock_lock(&device_counter_lock);
	size_t dev = device_counter++;
	spinlock_unlock(&device_counter_lock);
	return dev;
}

