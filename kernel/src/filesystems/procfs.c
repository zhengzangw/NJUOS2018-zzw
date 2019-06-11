#include <vfs.h>
#include <common.h>
#include <devices.h>

void procfs_init(filesystem_t *fs, const char *name, device_t *dev){
}
inode_t* procfs_lookup(filesystem_t *fs, const char *name, int flags){
    return NULL;
}
int procfs_inode_open(file_t *file, int flags){
    return 0;
}
int procfs_inode_close(file_t *file){
    return 0;
}
ssize_t procfs_inode_read(file_t *file, char *buf, size_t size){
    return 0;
}

fsops_t procfs_ops = {
    .init = procfs_init,
    .lookup = procfs_lookup,
    .close = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .unlink = NULL,
    .create = NULL,
};

inodeops_t procfs_inodeops = {
    .open = procfs_inode_open,
    .close = procfs_inode_close,
    .read = procfs_inode_read,
    .write = NULL,
    .link = NULL,
    .lseek = NULL,
};
