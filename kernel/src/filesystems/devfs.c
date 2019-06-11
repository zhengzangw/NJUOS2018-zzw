#include <common.h>
#include <vfs.h>
#include <devices.h>

void devfs_init(filesystem_t *fs, const char *name, device_t *dev){
}
inode_t* devfs_lookup(filesystem_t *fs, const char *name, int flags){
    return NULL;
}
int devfs_inode_open(file_t *file, int flags){
    return 0;
}
int devfs_inode_close(file_t *file){
    return 0;
}
ssize_t devfs_inode_read(file_t *file, char *buf, size_t size){
    return 0;
}
ssize_t devfs_inode_write(file_t *file, const char *buf, size_t size){
    return 0;
}

fsops_t devfs_ops = {
    .init = devfs_init,
    .lookup = devfs_lookup,
    .close = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .unlink = NULL,
    .create = NULL,
};

inodeops_t devfs_inodeops = {
    .open = devfs_inode_open,
    .close = devfs_inode_close,
    .read = devfs_inode_read,
    .write = devfs_inode_write,
    .link = NULL,
    .lseek = NULL,
};
