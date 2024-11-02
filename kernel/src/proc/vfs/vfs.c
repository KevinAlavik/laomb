#include <proc/vfs.h>
#include <kprintf>
#include <kheap.h>
#include <string.h>

struct Fs *root_fs = NULL;
const FileHandle NULL_HANDLE = { .node = NULL, .fs = NULL };

int vfs_mount_root(struct Fs *fs, void *specific_data) {
    if (!fs || !fs->ops || !fs->ops->mount) return INVALID_ARGS;

    int status = fs->ops->mount(fs, specific_data);
    if (status) return status;

    root_fs = fs;

    struct Node *root_node;
    fs->ops->get_root(fs, &root_node);

    fs->root_cache = create_path_entry(root_node, NULL, "/", 1);
    kprintf("Mounted root filesystem <%s>\n", fs->name);

    return OK;
}

int vfs_init(struct Fs *fs, void *specific_data) {
    kprintf("Initializing VFS\n");

    if (root_fs) {
        kprintf("Root filesystem already mounted.\n");
        return MOUNTPOINT_INVALID;
    }

    int result = vfs_mount_root(fs, specific_data);
    if (result) return result;

    return OK;
}