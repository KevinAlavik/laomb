#include <driver/initrd.h>
#include <proc/vfs.h>
#include <string.h>
#include <kheap.h>
#include <kprintf>
#include <tar.h>

void initrd_load(struct vfs_node* mount_point, const uint8_t* initrd_data, size_t initrd_size) {
    struct tar_parser parser;
    tar_parser_init(&parser, initrd_data, initrd_size);

    while (tar_parser_next(&parser)) {
        kprintf("Loading initrd entry: %s\n", tar_parser_current_header(&parser)->name);
        const struct tar_header* header = tar_parser_current_header(&parser);
        const uint8_t* file_data = tar_parser_current_file_data(&parser);
        size_t file_size = tar_parser_current_file_size(&parser);

        enum VFS_TYPE node_type = (header->typeflag == '5') ? VFS_RAMFS_FOLDER : VFS_RAMFS_FILE;

        struct vfs_node* node = vfs_create_node(header->name, node_type, mount_point);
        if (!node) {
            kprintf("Failed to create VFS node for initrd entry: %s\n", header->name);
            continue;
        }

        if (node_type == VFS_RAMFS_FILE) {
            node->write(node, 0, file_size, file_data);
        }
    }

    kprintf("Initrd mounted successfully.\n");
}
