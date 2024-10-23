#ifndef TAR_H
#define TAR_H

#include <stddef.h>
#include <stdint.h>

#define TAR_MAGIC "ustar"

#define TAR_HEADER_NAME_SIZE 100
#define TAR_HEADER_MODE_SIZE 8
#define TAR_HEADER_UID_SIZE 8
#define TAR_HEADER_GID_SIZE 8
#define TAR_HEADER_SIZE_SIZE 12
#define TAR_HEADER_MTIME_SIZE 12
#define TAR_HEADER_CHECKSUM_SIZE 8
#define TAR_HEADER_LINKNAME_SIZE 100
#define TAR_HEADER_MAGIC_SIZE 6
#define TAR_HEADER_VERSION_SIZE 2
#define TAR_HEADER_UNAME_SIZE 32
#define TAR_HEADER_GNAME_SIZE 32
#define TAR_HEADER_DEVMAJOR_SIZE 8
#define TAR_HEADER_DEVMINOR_SIZE 8
#define TAR_HEADER_PREFIX_SIZE 155
#define TAR_HEADER_PADDING_SIZE 12

struct tar_header {
    char name[TAR_HEADER_NAME_SIZE];
    char mode[TAR_HEADER_MODE_SIZE];
    char uid[TAR_HEADER_UID_SIZE];
    char gid[TAR_HEADER_GID_SIZE];
    char size[TAR_HEADER_SIZE_SIZE];
    char mtime[TAR_HEADER_MTIME_SIZE];
    char checksum[TAR_HEADER_CHECKSUM_SIZE];
    char typeflag;
    char linkname[TAR_HEADER_LINKNAME_SIZE];
    char magic[TAR_HEADER_MAGIC_SIZE];
    char version[TAR_HEADER_VERSION_SIZE];
    char uname[TAR_HEADER_UNAME_SIZE];
    char gname[TAR_HEADER_GNAME_SIZE];
    char devmajor[TAR_HEADER_DEVMAJOR_SIZE];
    char devminor[TAR_HEADER_DEVMINOR_SIZE];
    char prefix[TAR_HEADER_PREFIX_SIZE];
    char padding[TAR_HEADER_PADDING_SIZE];
};

struct tar_parser {
    const uint8_t* data;
    size_t size;
    size_t offset;
    const struct tar_header* current_header;
    size_t file_size;
};

void tar_parser_init(struct tar_parser* parser, const uint8_t* data, size_t size);
int tar_parser_next(struct tar_parser* parser);
const struct tar_header* tar_parser_current_header(const struct tar_parser* parser);
const uint8_t* tar_parser_current_file_data(const struct tar_parser* parser);
size_t tar_parser_current_file_size(const struct tar_parser* parser);
uint8_t* tar_parser_current_file_data_mut(struct tar_parser* parser);

#endif // TAR_H
