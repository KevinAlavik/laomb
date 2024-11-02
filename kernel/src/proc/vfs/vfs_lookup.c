#include <proc/vfs.h>
#include <kprintf>
#include <kheap.h>
#include <string.h>

int vfs_lookup(FileHandle start, const char *path, FileHandle *result) {
    if (!path || !result) return INVALID_ARGS;

    FileHandle current = (path[0] == '/') ? (FileHandle){ .node = root_fs->root_cache->node, .fs = root_fs } : start;

    size_t len = 0;
    const char *section = path;
    while ((section = split_path(section + len, &len))) {
        struct Node *node = current.node;

        while (node->mounted_fs) {
            node = node->mounted_fs->root_cache->node;
        }

        current.node = lookup_path_entry(current.node, section, len);
        if (!current.node) {
            char name[len + 1];
            strncpy(name, section, len);
            name[len] = '\0';

            struct Node *new_node;
            int status = node->ops->lookup(node, name, &new_node); 
            if (status) return NO_ENTRY;

            current.node = add_path_entry(new_node, current.node, name, len);
        }
    }

    *result = current;
    return OK;
}