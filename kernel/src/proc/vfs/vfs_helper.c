#include <proc/vfs.h>
#include <string.h>
#include <kheap.h>

const char *split_path(const char *path, size_t *length) {
    if (!path || !*path) return NULL;
    *length = strcspn(path, "/");
    return path + *length + (*length ? 1 : 0);
}

struct PathEntry *create_path_entry(struct Node *node, struct PathEntry *parent, const char *name, size_t len) {
    struct PathEntry *entry = kcalloc(1, sizeof(struct PathEntry));
    entry->node = node;
    entry->parent = parent;
    entry->name = kcalloc(len + 1, sizeof(char));
    strncpy(entry->name, name, len);
    entry->name[len] = '\0';
    entry->name_length = len;
    return entry;
}

struct Node *lookup_path_entry(struct Node *start_node, const char *section, size_t len) {
    struct PathEntry *current_entry = start_node->filesystem->root_cache;

    while (current_entry) {
        if (current_entry->name_length == len && strncmp(current_entry->name, section, len) == 0) {
            return current_entry->node;
        }
        current_entry = current_entry->next;
    }
    return nullptr;
}

struct Node *add_path_entry(struct Node *new_node, struct Node *parent_node, const char *section, size_t len) {
    struct PathEntry *new_entry = create_path_entry(new_node, parent_node->filesystem->root_cache, section, len);

    new_entry->next = parent_node->filesystem->root_cache->next;
    parent_node->filesystem->root_cache->next = new_entry;

    return new_node;
}