#include <proc/vfs.h>
#include <kprintf>
#include <stdbool.h>
#include <kheap.h>
#include <string.h>
#include <proc/sched.h>

int rootfs_mount(struct vfs* vfs, const char* path, struct vnode* mountat)
{
    return -1;
}

int rootfs_unmount(struct vfs* vfs)
{
    return -1;
}

int rootfs_groot(struct vfs* vfs, struct vnode** root)
{
    *root = (struct vnode*)vfs->vfs_data;    
}

int rootfs_sync(struct vfs* vfs)
{ // wtf we syncing? the ram?
    return 0;
}

int rootfs_get_vnode_fd(struct vfs* vfs, uint64_t fd, struct vnode** vnode) {
    struct JCB* job = sched_get_current_job();
    if (job->file_descriptors[fd] == nullptr) {
        return -1;
    }
    *vnode = job->file_descriptors[fd];
    return 0;
}

int rootfs_allocate_fd(struct vfs* vfs, struct vnode* vnode, uint32_t flags, uint64_t* fd) {
    struct JCB* job = sched_get_current_job();

    for (size_t i = 0; i < job->max_file_descriptors; ++i) {
        if (job->file_descriptors[i] == nullptr) {
            job->file_descriptors[i] = vnode;
            *fd = i;
            job->num_file_descriptors++;
            return 0;
        }
    }

    size_t new_max = job->max_file_descriptors * 2;
    struct vnode** new_fds = (struct vnode**)krealloc(job->file_descriptors, new_max * sizeof(struct vnode*));
    if (!new_fds) return -1;

    memset(&new_fds[job->max_file_descriptors], 0, (new_max - job->max_file_descriptors) * sizeof(struct vnode*));
    job->file_descriptors = new_fds;
    job->max_file_descriptors = new_max;

    job->file_descriptors[job->num_file_descriptors] = vnode;
    *fd = job->num_file_descriptors;
    job->num_file_descriptors++;
    return 0;
}

int rootfs_close_fd(struct vfs* vfs, uint64_t fd) {
    struct JCB* job = sched_get_current_job();

    if (fd >= job->max_file_descriptors || job->file_descriptors[fd] == nullptr) {
        return -1;
    }

    job->file_descriptors[fd] = nullptr;
    job->num_file_descriptors--;

    return 0;
}

int rootfsv_open(struct vnode* vnode, const char* name, uint32_t flags, struct vnode** out)
{

}

int rootfsv_close(struct vnode* vnode)
{
    
}

int rootfsv_rw(struct vnode* vnode, uint8_t* buffer, size_t len, uint32_t flags, bool write)
{
    
}

int rootfsv_access(struct vnode* vnode, uint32_t mode)
{
    
}

int rootfsv_create(struct vnode* vnode, const char* name, uint32_t mode, uint32_t flags)
{
    
}

int rootfsv_remove(struct vnode* vnode, const char* name)
{
    
}

int rootfsv_rename(struct vnode* vnode, const char* oldname, const char* newname)
{
    
}

int rootfsv_mkdir(struct vnode* vnode, const char* name, uint32_t mode)
{
    
}

int rootfsv_rmdir(struct vnode* vnode, const char* name)
{
    
}

int rootfsv_readdir(struct vnode* vnode, const char** names, size_t* count)
{
    
}

int rootfsv_link(struct vnode* vnode, const char* name, struct vnode* target)
{
    
}

int rootfsv_unlink(struct vnode* vnode)
{
    
}

int rootfsv_symlink(struct vnode* vnode, const char* name, uint32_t flags, struct vnode** out)
{
    
}

int rootfsv_fsync(struct vnode* vnode)
{
    
}

int rootfsv_ioctl(struct vnode* vnode, uint32_t request, void* arg)
{
    
}

bool vfs_initroot()
{
    root_vfs = (struct vfs*)kmalloc(sizeof(struct vfs));
    struct vfs_operations* op = (struct vfs_operations*)kmalloc(sizeof(struct vfs_operations));
    op->mount = rootfs_mount;
    op->unmount = rootfs_unmount;
    op->groot = rootfs_groot;
    op->sync = rootfs_sync;
    op->allocate_fd = rootfs_allocate_fd;
    op->close_fd = rootfs_close_fd;
    op->get_vnode_fd = rootfs_get_vnode_fd;
    root_vfs->vfs_op = op;
    root_vfs->mount = nullptr;
    root_vfs->next = nullptr;
    struct vnode* root = (struct vnode*)kmalloc(sizeof(struct vnode));
    memset(root, 0, sizeof(struct vnode));
    root->vfs = root_vfs;
    root->type = VNODE_DIR;
    struct vnode_operations* vnode_op = (struct vnode_operations*)kmalloc(sizeof(struct vnode_operations));
    vnode_op->open = rootfsv_open;
    vnode_op->close = rootfsv_close;
    vnode_op->rw = rootfsv_rw;
    vnode_op->access = rootfsv_access;
    vnode_op->create = rootfsv_create;
    vnode_op->remove = rootfsv_remove;
    vnode_op->rename = rootfsv_rename;
    vnode_op->mkdir = rootfsv_mkdir;
    vnode_op->rmdir = rootfsv_rmdir;
    vnode_op->readdir = rootfsv_readdir;
    vnode_op->link = rootfsv_link;
    vnode_op->unlink = rootfsv_unlink;
    vnode_op->symlink = rootfsv_symlink;
    vnode_op->fsync = rootfsv_fsync;
    vnode_op->ioctl = rootfsv_ioctl;
    root->vnode_op = vnode_op;
    root_vfs->vfs_data = (uintptr_t)root;
    root_vfs->flags = 0;
    return true;
}

bool vfs_mount(struct vfs* fs, const char* path, struct vnode* node)
{
    node->mount = fs;
    fs->vfs_op->mount(fs, path, node);
    fs->mount = node;
    return true;
}

bool vfs_unmount(struct vnode* node)
{
    if (node->mount != nullptr) {
        node->mount->vfs_op->unmount(node->mount);
    }
    node->mount->mount = nullptr;
    node->mount = nullptr;
    return true;
}

int vfs_resolve_path(const char* path, struct vnode** out_vnode)
{
    struct vnode* cur = nullptr;
    int status = root_vfs->vfs_op->groot(root_vfs, &cur);
    if (status != 0) {
        return status;
    }
    return vfs_walk(cur, path, out_vnode);
}

int vfs_close_fd(struct vfs* vfs, uint64_t fd)
{
    return vfs->vfs_op->close_fd(vfs, fd);
}

int vfs_flock(struct vnode* vnode, enum flock_type lock_type)
{
    switch (lock_type)
    {
    case FLOCK_SHARED:
        vnode->slocks++;
        break;
    case FLOCK_EXCLUSIVE:
        vnode->elocks++;
        break;
    case FLOCK_ULOCK_EXCLUSIVE:
        vnode->elocks--;
        break;
    case FLOCK_ULOCK_SHARED:
        vnode->slocks--;
        break;
    
    default:
        return -1;
    }

    return 0;
}

int vfs_sync_all()
{
    struct vfs* cur = root_vfs;
    while (cur != nullptr) {
        int status = cur->vfs_op->sync(cur);
        if (status != 0) {
            return status;
        }
        cur = cur->next;
    }
    return 0;
}

int vfs_walk(struct vnode* vnode, const char* path, struct vnode** result) {
    struct vnode* cur = vnode;
    int status = 0;
    
    while (*path != '\0') {
        if (*path == '/') {
            path++;
            continue;
        }

        const char* slash = strchr(path, '/');
        size_t len = slash ? (size_t)(slash - path) : strlen(path);
        
        char name[len + 1];
        memcpy(name, path, len);
        name[len] = '\0';

        struct vnode* next_vnode = nullptr;

        status = cur->vnode_op->readdir(cur, &name, &next_vnode);
        if (status != 0) {
            break;
        }

        cur = next_vnode;
        path = slash ? slash + 1 : path + len;
    }

    if (status == 0) {
        *result = cur;
    }

    return status;
}
