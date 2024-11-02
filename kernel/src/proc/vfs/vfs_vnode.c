#include <proc/vfs.h>
#include <kheap.h>

struct Node *vfs_node_alloc(struct Fs *fs, size_t flags, struct NodeOps *ops, enum NodeType type, void *data) {
    struct Node *node = kcalloc(1, sizeof(struct Node));
    node->type = type;
    node->flags = flags;
    node->ops = ops;
    node->filesystem = fs;
    node->data = data;
    return node;
}

void vfs_node_free(struct Node **node_ptr) {
    if (!node_ptr || !*node_ptr) return;
    kfree((*node_ptr)->data);
    kfree(*node_ptr);
    *node_ptr = NULL;
}
