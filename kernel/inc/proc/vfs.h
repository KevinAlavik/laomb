#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define OK                      0
#define INVALID_ARGS            1
#define MOUNTPOINT_INVALID      2
#define NO_ENTRY                3

#define ROOT_FLAG               (1 << 0)

enum NodeType {
    NODE_NONE,
    NODE_REG,
    NODE_DIR,
    NODE_BLK,
    NODE_CHR,
    NODE_LNK,
    NODE_SOCK,
    NODE_BAD
};

typedef struct {
    uint64_t id[2];
} FsId;

struct FileId {
    uint64_t len;
    uint8_t data[1];
};

struct IoOp {
    uint8_t *buffer;
    size_t remaining;
    size_t offset;
    bool is_write;
};

struct FsStats {
    uint64_t type;
    uint64_t block_size;
    uint64_t total_blocks;
    uint64_t free_blocks;
    uint64_t available_blocks;
    uint64_t total_files;
    uint64_t free_files;
    FsId fsid;
    uint64_t spare[7];
};

struct NodeAttr {
    enum NodeType type;
    uint16_t mode;
    int16_t uid;
    int16_t gid;
    size_t fs_id;
    size_t node_id;
    int16_t link_count;
    size_t size;
    size_t device_id;
    size_t blocks;
};

typedef struct FileHandle {
    struct Fs *fs;      
    struct Node *node;  
} FileHandle;

struct Fs {
    struct Fs *next;             
    struct FsOps *ops;

    size_t flags;
    size_t block_size;

    FileHandle mount_point;
    char *name;                   
    void *data;                   

    struct PathEntry *root_cache;
    FileHandle covered;
};

struct FsOps {
    int (*mount)(struct Fs *fs, void *specific_data);
    int (*unmount)(struct Fs *fs);
    int (*get_root)(struct Fs *fs, struct Node **result);
    int (*get_stats)(struct Fs *fs, struct FsStats *result);
    int (*sync)(struct Fs *fs);
    int (*get_file_id)(struct Fs *fs, struct Node *file, struct FileId **result);
    int (*get_node_by_id)(struct Fs *fs, struct Node **result, struct FileId *id);
};

struct Node {
    enum NodeType type;
    struct NodeOps *ops;

    struct Fs *filesystem;        
    struct Fs *mounted_fs;        

    size_t flags;
    size_t ref_count;
    void *data;
};

struct NodeOps {
    int (*open)(struct Node *node, uint64_t flags);
    int (*close)(struct Node *node, uint64_t flags);
    int (*read_write)(struct Node *node, struct IoOp *io, size_t flags);
    int (*get_attr)(struct Node *node, struct NodeAttr *attr);
    int (*set_attr)(struct Node *node, const struct NodeAttr *attr);
    int (*lookup)(struct Node *dir, const char *name, struct Node **result); // Modified to take three arguments
    int (*create)(struct Node *dir, const char *name, struct NodeAttr *attr, struct Node **result);
    int (*remove)(struct Node *dir, const char *name);
    int (*read_dir)(struct Node *dir, struct IoOp *io);
    int (*sync)(struct Node *node);
    int (*deactivate)(struct Node *node);
};

struct PathEntry {
    struct Node *node;           
    struct PathEntry *parent;   
    struct PathEntry *next, *prev;
    char *name;                  
    size_t name_length;
};

extern struct Fs *root_fs;
extern const FileHandle null_handle;
#define NULL_HANDLE (null_handle)

static inline uint64_t hash_path(const char *str, uint64_t len) {
    uint64_t hash = 0xcbf29ce484222325; // FNV-1a 64-bit offset basis
    const uint64_t fnv_prime = 0x100000001b3;

    for (uint64_t i = 0; i < len; i++) {
        hash ^= (unsigned char)str[i];
        hash *= fnv_prime;
    }

    return hash;
}

struct PathEntry *create_path_entry(struct Node *node, struct PathEntry *parent, const char *name, size_t len);
struct PathEntry *add_path_section(struct Node *node, struct PathEntry *dir, const char *section, size_t len);
void remove_path_section(struct PathEntry *entry);
struct PathEntry *find_path_section(struct PathEntry *dir, const char *section, size_t len);

// Helper functions to manage path caching
struct Node *lookup_path_entry(struct Node *start_node, const char *section, size_t len);
struct Node *add_path_entry(struct Node *new_node, struct Node *parent_node, const char *section, size_t len);

struct Node *allocate_node(struct Fs *fs, size_t flags, struct NodeOps *ops, enum NodeType type, void *data);
void initialize_vfs();

const char *split_path(const char *path, size_t *length);
