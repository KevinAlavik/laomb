#include <proc/vfs.h>
#include <kprintf>
#include <stdbool.h>
#include <kheap.h>
#include <string.h>
#include <proc/sched.h>

int vfs_mount(struct Fs *fs, FileHandle mountpoint, void *specific_data) {
    if (!fs || !fs->ops || !fs->ops->mount || !mountpoint.node) return INVALID_ARGS;

    int status = fs->ops->mount(fs, specific_data);
    if (status) return status;

    struct Node *mount_node = mountpoint.node;
    mount_node->mounted_fs = fs;

    fs->covered = mountpoint;

    struct Node *root_node;
    fs->ops->get_root(fs, &root_node);

    struct PathEntry *mountpoint_entry = mountpoint.node->filesystem->root_cache;
    if (mountpoint_entry) {
        fs->root_cache = create_path_entry(root_node, NULL, mountpoint_entry->name, mountpoint_entry->name_length);
    } else {
        kprintf("Failed to access mountpoint entry name.\n");
        return INVALID_ARGS;
    }

    kprintf("Mounted filesystem <%s> at %s\n", fs->name, mountpoint_entry->name);

    return OK;
}
