#include <tar.h>
#include <stddef.h>
#include <stdint.h>
#include <kheap.h>
#include <string.h>
#include <kprintf>

struct tar_parser {
    const uint8_t* data;
    size_t size;
    size_t offset;
    const struct tar_header* current_header;
    size_t file_size;
};

void tar_parser_init(struct tar_parser* parser, const uint8_t* data, size_t size) {
    parser->data = data;
    parser->size = size;
    parser->offset = 0;
    parser->current_header = (const struct tar_header*)data;
    parser->file_size = 0;

    if (strncmp(parser->current_header->magic, TAR_MAGIC, 5) == 0) {
        parser->file_size = strtol(parser->current_header->size, NULL, 8);
    }

    kprintf(" -> Initialized TarParser with file: %s, size: %lu\n", parser->current_header->name, parser->file_size);
}

int tar_parser_next(struct tar_parser* parser) {
    if (parser->offset == 0) {
        parser->offset += sizeof(struct tar_header);
    } else {
        size_t current_file_total_size = parser->file_size + sizeof(struct tar_header);
        size_t padding = (512 - (parser->file_size % 512)) % 512;
        current_file_total_size += padding;
        parser->offset += current_file_total_size;
    }

    if (parser->offset >= parser->size || strlen((const char*)(parser->data + parser->offset)) == 0) {
        kprintf(" -> End of TAR archive reached or offset beyond size.\n");
        return 0;
    }

    parser->current_header = (const struct tar_header*)(parser->data + parser->offset);

    if (strncmp(parser->current_header->magic, TAR_MAGIC, 5) != 0) {
        kprintf(" -> Invalid TAR magic number: %s\n", parser->current_header->magic);
        return 0;
    }

    parser->file_size = strtol(parser->current_header->size, NULL, 8);
    return 1;
}

const struct tar_header* tar_parser_current_header(const struct tar_parser* parser) {
    return parser->current_header;
}

const uint8_t* tar_parser_current_file_data(const struct tar_parser* parser) {
    return parser->data + parser->offset + sizeof(struct tar_header);
}

size_t tar_parser_current_file_size(const struct tar_parser* parser) {
    return parser->file_size;
}

uint8_t* tar_parser_current_file_data_mut(struct tar_parser* parser) {
    return (uint8_t*)(parser->data + parser->offset + sizeof(struct tar_header));
}